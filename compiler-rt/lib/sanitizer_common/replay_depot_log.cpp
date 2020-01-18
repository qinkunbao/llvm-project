#include "sanitizer_common.h"
#include "sanitizer_file.h"
#include "sanitizer_stackdepot.h"

using namespace __sanitizer;

int main(int argc, char **argv) {
  uptr size;
  uptr *log = (uptr *)MapFileToMemory(argv[1], &size);
  uptr *log_end = log + (size / sizeof(uptr));

  while (log < log_end) {
    StackTrace trace(log + 1, log[0]);
    StackDepotPutNoRecord(trace);
    log += log[0] + 1;
  }
}
