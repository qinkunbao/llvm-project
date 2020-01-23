#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <set>
#include <string>

using namespace __sanitizer;

#undef PERF
#undef MEM

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
  while (log < log_end) {
    StackTrace trace(log + 2, log[1]);
    StackDepotPutNoRecord(trace);
#if !defined(PERF) && !defined(MEM)
    traces.insert(std::string((const char *)(log + 1), (log[1] + 1) * sizeof(uptr)));
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
