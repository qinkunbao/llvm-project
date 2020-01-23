#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <vector>

struct StackTraceLog {
  uintptr_t next;
  uintptr_t tid;
  uintptr_t size;
};

struct ThreadHeader {
  uintptr_t begin, end;
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

  std::vector<std::vector<std::unique_ptr<uintptr_t[]>>> blobs_by_thread;
  std::map<uintptr_t, uintptr_t> thread_ids;
  StackTraceLog header;
  while (log_addr) {
    lseek(fd, log_addr, SEEK_SET);
    read(fd, &header, sizeof(header));
    log_addr = header.next;

    auto blob = std::make_unique<uintptr_t[]>(header.size + 2);
    auto tid_i = thread_ids.find(header.tid);
    uintptr_t tid;
    if (tid_i == thread_ids.end()) {
      tid = thread_ids.size();
      thread_ids[header.tid] = tid;
      blobs_by_thread.emplace_back();
    } else {
      tid = tid_i->second;
    }
    blob[0] = header.size;
    read(fd, blob.get() + 1, header.size * sizeof(uintptr_t));
    blobs_by_thread[tid].push_back(std::move(blob));
  }
  close(fd);

  fd = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC, 0777);
  uintptr_t num_tids = thread_ids.size();
  write(fd, &num_tids, sizeof(num_tids));
  lseek(fd, sizeof(uintptr_t) * 2 * thread_ids.size(), SEEK_END);
  for (size_t tid = 0; tid != blobs_by_thread.size(); ++tid) {
    ThreadHeader hdr;
    hdr.begin = lseek(fd, 0, SEEK_CUR);
    for (auto i = blobs_by_thread[tid].rbegin();
         i != blobs_by_thread[tid].rend(); ++i) {
      write(fd, i->get(), ((*i)[0] + 1) * sizeof(uintptr_t));
    }
    hdr.end = lseek(fd, 0, SEEK_CUR);
    lseek(fd, sizeof(uintptr_t) * (1 + 2 * tid), SEEK_SET);
    write(fd, &hdr, sizeof(hdr));
    lseek(fd, 0, SEEK_END);
  }
  close(fd);
}
