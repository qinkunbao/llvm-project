#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <vector>

struct alloc {
  size_t size, count;
};

size_t measure_wastage(const std::vector<alloc> &allocs,
                       const std::vector<size_t> &classes,
                       size_t page_size,
                       size_t header_size) {
  size_t total_wastage = 0;
  for (auto &a : allocs) {
    size_t size_plus_header = a.size + header_size;
    size_t wastage = -1ull;
    for (auto c : classes)
      if (c >= size_plus_header && c - size_plus_header < wastage)
        wastage = c - size_plus_header;
    if (wastage == -1ull)
      continue;
    if (wastage > 2 * page_size)
      wastage = 2 * page_size;
    total_wastage += wastage * a.count;
  }
  return total_wastage;
}

std::vector<alloc> read_allocs(FILE *f) {
  std::vector<alloc> allocs;
  const char header[] = "<malloc version=\"scudo-1\">\n";
  char buf[sizeof(header) - 1];
  if (fread(buf, 1, sizeof(header) - 1, f) != sizeof(header) - 1 ||
      memcmp(buf, header, sizeof(header) - 1) != 0) {
    fprintf(stderr, "compute_size_classes: invalid input format\n");
    exit(1);
  }

  alloc a;
  while (fscanf(f, "<alloc size=\"%zu\" count=\"%zu\"/>\n", &a.size, &a.count) == 2)
    allocs.push_back(a);
  return allocs;
}

size_t log2_floor(size_t x) { return sizeof(long) * 8 - 1 - __builtin_clzl(x); }

void usage() {
  fprintf(stderr, "usage: compute_size_classes [-p page_size] [-c largest_class] [-h header_size] [-n num_classes] [-b num_bits]\n");
  exit(1);
}

int main(int argc, char **argv) {
  size_t page_size = 4096;
  size_t largest_class = 65552;
  size_t header_size = 16;
  size_t num_classes = 32;
  size_t num_bits = 5;

  for (size_t i = 1; i != argc;) {
    if (i + 1 >= argc)
      usage();
    if (strcmp(argv[i], "-p") == 0) {
      page_size = atoi(argv[i + 1]);
      i += 2;
      continue;
    }
    if (strcmp(argv[i], "-c") == 0) {
      largest_class = atoi(argv[i + 1]);
      i += 2;
      continue;
    }
    if (strcmp(argv[i], "-h") == 0) {
      header_size = atoi(argv[i + 1]);
      i += 2;
      continue;
    }
    if (strcmp(argv[i], "-n") == 0) {
      num_classes = atoi(argv[i + 1]);
      i += 2;
      continue;
    }
    if (strcmp(argv[i], "-b") == 0) {
      num_bits = atoi(argv[i + 1]);
      i += 2;
      continue;
    }
    usage();
  }

  std::vector<alloc> allocs = read_allocs(stdin);
  std::vector<size_t> classes;
  classes.push_back(largest_class);
  for (size_t i = 1; i != num_classes; ++i) {
    size_t min_wastage = -1ull;
    size_t min_wastage_class;
    for (size_t new_class = 16; new_class != largest_class; new_class += 16) {
      if (__builtin_ctzl(new_class - header_size) +
              __builtin_clzl(new_class - header_size) <
          sizeof(long) * 8 - num_bits)
        continue;
      classes.push_back(new_class);
      size_t new_wastage = measure_wastage(allocs, classes, page_size, header_size);
      classes.pop_back();
      if (new_wastage < min_wastage) {
        min_wastage = new_wastage;
        min_wastage_class = new_class;
      }
    }
    classes.push_back(min_wastage_class);
  }

  std::sort(classes.begin(), classes.end());
  size_t min_size_log = log2_floor(header_size);
  size_t mid_size_index = 0;
  while (classes[mid_size_index + 1] - classes[mid_size_index] ==
         (1 << min_size_log))
    mid_size_index++;
  size_t mid_size_log = log2_floor(classes[mid_size_index] - header_size);
  size_t max_size_log = log2_floor(classes.back() - header_size - 1) + 1;

  printf(R"(// wastage = %zu

struct MySizeClassConfig {
  static const uptr NumBits = %zu;
  static const uptr MinSizeLog = %zu;
  static const uptr MidSizeLog = %zu;
  static const uptr MaxSizeLog = %zu;
  static const u32 MaxNumCachedHint = 14;
  static const uptr MaxBytesCachedLog = 14;

  static constexpr u32 Classes[] = {)",
         measure_wastage(allocs, classes, page_size, header_size), num_bits,
         min_size_log, mid_size_log, max_size_log);
  for (size_t i = 0; i != classes.size(); ++i) {
    if ((i % 8) == 0)
      printf("\n      ");
    else
      printf(" ");
    printf("0x%05zx,", classes[i]);
  }
  printf(R"(
  };
  static const uptr SizeDelta = %zu;
};
)", header_size);
}
