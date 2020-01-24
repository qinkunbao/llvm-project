#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace __sanitizer;

struct LossyStackDepot {
  enum { kNumBits = 19, kTabSize = 1 << kNumBits, kTabMask = kTabSize - 1 };
  uptr tab[kTabSize];

  __attribute__((noinline)) u32 insert(uptr *begin, uptr *end) {
    MurMur2HashBuilder b;
    for (uptr *i = begin; i != end; ++i)
      b.add(*i);
    u32 hash = b.get();
    u32 pos = hash & kTabMask & ~15;
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
    u32 pos = hash & kTabMask & ~15;
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

#ifndef PERF
  char cmd[64];
  snprintf(cmd, 64, "grep RssAnon: /proc/%d/status", getpid());
  system(cmd);
#endif

  auto depot = reinterpret_cast<LossyStackDepot *>(
      mmap(nullptr, (sizeof(LossyStackDepot) + 4095) & ~4095,
           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  std::vector<u32> hashes;
  std::mutex hashes_mu;

  std::vector<std::thread> insert_threads;
  for (unsigned i = 0; i != log[0]; ++i) {
    uptr *thread_log_begin = log + log[1 + 2 * i] / sizeof(uptr);
    uptr *thread_log_end = log + log[1 + 2 * i + 1] / sizeof(uptr);
    insert_threads.push_back(std::thread([thread_log_begin, thread_log_end, depot,
                                          &hashes_mu, &hashes]() {
      uptr *thread_log = thread_log_begin;
      while (thread_log < thread_log_end) {
        u32 hash =
            depot->insert(thread_log + 1, thread_log + 1 + thread_log[0]);
        (void)hash;
        (void)hashes;
        (void)hashes_mu;
#if !defined(PERF) && !defined(MEM)
        {
          std::lock_guard<std::mutex> x(hashes_mu);
          hashes.push_back(hash);
        }
#endif
        thread_log += thread_log[0] + 1;
      }
    }));
  }

  for (std::thread &t : insert_threads) t.join();

#ifndef PERF
  system(cmd);

#ifndef MEM
  uptr num_recoverable = 0;
  for (u32 hash : hashes) {
    if (depot->find(hash))
      num_recoverable++;
  }
  printf("recall: %lu/%lu\n", (unsigned long)num_recoverable, (unsigned long)hashes.size());

  enum { kLineLength = 160 };
  for (uptr i = 0; i != kLineLength; ++i) {
    uptr seg_recoverable = 0;
    for (uptr j = hashes.size() * i / kLineLength,
              e = hashes.size() * (i + 1) / kLineLength;
         j != e; ++j) {
      if (depot->find(hashes[j]))
        seg_recoverable++;
    }
    u8 seg_recoverable_char = 'a' + seg_recoverable * 26 / ((hashes.size() / kLineLength) + 1);
    printf("%c", seg_recoverable_char);
  }
  puts("");
#endif
  
  uptr num_used = 0;
  for (uptr v : depot->tab) {
    if (v)
      num_used++;
  }
  printf("usage: %lu/%lu\n", (unsigned long)num_used, (unsigned long)depot->kTabSize);
#endif
}
