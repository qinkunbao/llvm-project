#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

void foo() {
  puts("hello from foo");
}

extern char __ehdr_start[];

int main() {
  int fd = open("hello", O_RDONLY);
  mmap(__ehdr_start + 0x6000, 0x1000, PROT_EXEC | PROT_READ, MAP_PRIVATE, fd, 0x5000);
  void (*foo_ptr)() = __ehdr_start + 0x6000;
  foo_ptr();
}
