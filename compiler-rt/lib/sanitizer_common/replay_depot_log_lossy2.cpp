#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <memory>
#include <vector>

using namespace __sanitizer;

struct LossyStackDepot {
  u32 ring_end;

  enum { kTabBits = 19, kTabSize = 1 << kTabBits, kTabMask = kTabSize - 1 };
  u32 tab[kTabSize];

  enum { kRingSize = 1 << 21, kRingMask = kRingSize - 1 };
  uptr ring[kRingSize];

  __attribute__((noinline)) u32 insert(uptr *begin, uptr *end) {
    MurMur2HashBuilder b;
    for (uptr *i = begin; i != end; ++i)
      b.add(*i);
    u32 hash = b.get();

    u32 pos = hash & kTabMask;
    u32 ring_pos = tab[pos];
    uptr entry = ring[ring_pos];
    uptr id = ((end - begin) << 33) | (uptr(hash) << 1) | 1;
    if (entry == id)
      return hash;

    ring_pos = ring_end;
    tab[pos] = ring_pos;
    ring[ring_pos] = id;
    for (uptr *i = begin; i != end; ++i) {
      ring_pos = (ring_pos + 1) & kRingMask;
      ring[ring_pos] = *i;
    }
    ring_end = (ring_pos + 1) & kRingMask;
    return hash;
  }

  bool find(u32 hash) {
    u32 pos = hash & kTabMask;
    u32 ring_pos = tab[pos];
    uptr entry = ring[ring_pos];
    uptr hash_with_tag_bit = (uptr(hash) << 1) | 1;
    if ((entry & 0x1ffffffff) != hash_with_tag_bit)
      return false;
    u32 size = entry >> 33;
    MurMur2HashBuilder b;
    for (uptr i = 0; i != size; ++i) {
      ring_pos = (ring_pos + 1) & kRingMask;
      b.add(ring[ring_pos]);
    }
    return b.get() == hash;
  }
};

int main(int argc, char **argv) {
  uptr size;
  uptr *log = (uptr *)MapFileToMemory(argv[1], &size);
  uptr *log_end = log + (size / sizeof(uptr));

#ifndef PERF
  char cmd[64];
  snprintf(cmd, 64, "grep RssAnon: /proc/%d/status", getpid());
  system(cmd);
#endif

  auto depot = reinterpret_cast<LossyStackDepot *>(
      mmap(nullptr, (sizeof(LossyStackDepot) + 4095) & ~4095,
           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  std::vector<u32> hashes;
  while (log < log_end) {
    u32 hash = depot->insert(log + 2, log + 2 + log[1]);
#if !defined(PERF) && !defined(MEM)
    hashes.push_back(hash);
#endif
    log += log[1] + 2;
  }

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
  
  uptr ring_num_used = 0;
  for (uptr v : depot->ring) {
    if (v)
      ring_num_used++;
  }
  printf("ring usage: %lu/%lu\n", (unsigned long)ring_num_used, (unsigned long)depot->kRingSize);
#endif
}
