#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

#include <stdio.h>

using namespace __sanitizer;

int main(int argc, char **argv) {
  uptr size;
  uptr *log = (uptr *)MapFileToMemory(argv[1], &size);
  uptr *log_end = log + (size / sizeof(uptr));

  uptr num_inserts = 0;
  while (log < log_end) {
    StackTrace trace(log + 1, log[0]);
    StackDepotPutNoRecord(trace);
    log += log[0] + 1;
    ++num_inserts;
  }

  uptr num_entries = 0;
  for (uptr i = 0; i != theDepot.kTabSize; ++i) {
    if (theDepot.tab[i].val_dont_use)
      ++num_entries;
  }

  printf("%lu/%lu/%lu\n", (unsigned long)num_entries,
         (unsigned long)num_inserts, (unsigned long)theDepot.kTabSize);
}
