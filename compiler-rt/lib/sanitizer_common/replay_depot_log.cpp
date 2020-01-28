#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

#include "size_class_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <set>
#include <string>

using namespace __sanitizer;

int main(int argc, char **argv) {
  uptr size;
  uptr *log = (uptr *)MapFileToMemory(argv[1], &size);
  uptr *log_end = log + (size / sizeof(uptr));

#ifndef PERF
  char cmd[64];
  snprintf(cmd, 64, "grep RssAnon: /proc/%d/status", getpid());
  system(cmd);
#endif

  std::set<std::string> traces;
  uptr num_inserts = 0;
  uintptr_t total_alloc = 0;
  uintptr_t total_wastage = 0;
  uintptr_t pow2_wastage = 0;
  log += 1 + log[0] * 2;
  while (log < log_end) {
    StackTrace trace(log + 2, log[1]);
    StackDepotPutNoRecord(trace);
#if !defined(PERF) && !defined(MEM)
    traces.insert(std::string((const char *)(log + 1), (log[1] + 1) * sizeof(uptr)));
    if (log[0]) {
      scudo::AndroidSizeClassMap scm;
      size_t size_plus_header = log[0] + 16;
      size_t wastage = scm.getSizeByClassId(scm.getClassIdBySize(size_plus_header)) -
                       size_plus_header;
      total_alloc += size_plus_header;
      total_wastage += wastage;
      if (log[0] > 16 && (log[0] & (log[0] - 1)) == 0) {
        pow2_wastage += wastage;
        printf("%lu (%lu)\n", (unsigned long)log[0], (unsigned long)wastage);
      }
    }
#endif
    log += log[1] + 2;
    ++num_inserts;
  }

#ifndef PERF
  system(cmd);

#ifndef MEM
  size_t trace_size = 0;
  for (auto t : traces)
    trace_size += t.size();
  printf("%lu unique traces (%lu bytes)\n", traces.size(),
         (unsigned long)trace_size);

  printf("%lu bytes allocated, %lu bytes wasted (%lu pow2)\n",
         (unsigned long)total_alloc, (unsigned long)total_wastage,
         (unsigned long)pow2_wastage);
#endif

  uptr num_entries = 0;
  for (uptr i = 0; i != theDepot.kTabSize; ++i) {
    if (theDepot.tab[i].val_dont_use)
      ++num_entries;
  }

  printf("%lu/%lu/%lu\n", (unsigned long)num_entries,
         (unsigned long)num_inserts, (unsigned long)theDepot.kTabSize);
#endif
}
