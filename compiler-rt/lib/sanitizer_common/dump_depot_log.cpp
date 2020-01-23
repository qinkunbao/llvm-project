#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <vector>
#include <memory>

struct StackTraceLog {
  uintptr_t next;
  uintptr_t tid;
  uintptr_t size;
};

int main(int argc, char **argv) {
  long addr = 0;

  char proc_name[64];
  snprintf(proc_name, 64, "/proc/%s/maps", argv[1]);
  FILE *maps = fopen(proc_name, "r");
  long begin, end;
  char desc[512];
  while (fscanf(maps, "%lx-%lx %512[^\n]\n", &begin, &end, desc) == 3) {
    if (strstr(desc, "[anon:depot log]")) {
      addr = begin;
      break;
    }
  }
  fclose(maps);

  if (!addr) {
    puts("not found");
    return 1;
  }

  snprintf(proc_name, 64, "/proc/%s/mem", argv[1]);
  int fd = open(proc_name, O_RDONLY);
  lseek(fd, addr, SEEK_SET);
  uintptr_t log_addr;
  read(fd, &log_addr, sizeof(log_addr));

  printf("addr = %lx\n", addr);
  printf("log_addr = %lx\n", log_addr);

  std::vector<std::unique_ptr<uintptr_t[]>> blobs;
  StackTraceLog header;
  while (log_addr) {
    lseek(fd, log_addr, SEEK_SET);
    read(fd, &header, sizeof(header));
    log_addr = header.next;

    auto blob = std::make_unique<uintptr_t[]>(header.size + 2);
    blob[0] = header.tid;
    blob[1] = header.size;
    read(fd, blob.get() + 2, header.size * sizeof(uintptr_t));
    blobs.push_back(std::move(blob));
  }
  close(fd);

  fd = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC, 0777);
  for (auto i = blobs.rbegin(); i != blobs.rend(); ++i)
    write(fd, i->get(), ((*i)[1] + 2) * sizeof(uintptr_t));
  close(fd);
}
