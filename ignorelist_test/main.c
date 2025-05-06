#include <stdio.h>


extern void foo();
extern void bar();

int main() {
  foo();
  bar();
  printf("Hello world\n");
  return 0;
}

