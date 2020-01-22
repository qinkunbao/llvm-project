#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

#include <stdio.h>
#include <vector>

using namespace __sanitizer;

struct LossyStackDepot {
  enum { kNumBits = 16, kTabSize = 1 << kNumBits, kTabMask = kTabSize - 1 };
  uptr tab[kTabSize];

  __attribute__((noinline)) u32 insert(uptr *begin, uptr *end) {
    MurMur2HashBuilder b;
    for (uptr *i = begin; i != end; ++i)
      b.add(*i);
    u32 hash = b.get();
    u32 pos = hash & kTabMask;
    uptr entry = tab[pos];
    uptr hash_with_tag_bit = (uptr(hash) << 1) | 1;
    if ((entry & 0x1ffffffff) == hash_with_tag_bit)
      return hash;
    tab[pos] = hash_with_tag_bit | ((end - begin) << 33);
    for (uptr *i = begin; i != end; ++i) {
      pos = (pos + 1) & kTabMask;
      tab[pos] = *i;
    }
    return hash;
  }

  bool find(u32 hash) {
    u32 pos = hash & kTabMask;
    uptr entry = tab[pos];
    uptr hash_with_tag_bit = (uptr(hash) << 1) | 1;
    if ((entry & 0x1ffffffff) != hash_with_tag_bit)
      return false;
    u32 size = entry >> 33;
    MurMur2HashBuilder b;
    for (uptr i = 0; i != size; ++i) {
      pos = (pos + 1) & kTabMask;
      b.add(tab[pos]);
    }
    return b.get() == hash;
  }
};

int main(int argc, char **argv) {
  uptr size;
  uptr *log = (uptr *)MapFileToMemory(argv[1], &size);
  uptr *log_end = log + (size / sizeof(uptr));

  LossyStackDepot depot;
  std::vector<u32> hashes;
  while (log < log_end) {
    hashes.push_back(depot.insert(log + 1, log + 1 + log[0]));
    log += log[0] + 1;
  }

#if 0
  uptr num_recoverable = 0;
  for (u32 hash : hashes) {
    if (depot.find(hash))
      num_recoverable++;
  }

  printf("%lu/%lu\n", (unsigned long)num_recoverable, (unsigned long)hashes.size());
#endif
}
