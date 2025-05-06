#include <stdio.h>
#include <stdlib.h>

void foo() {
  int *a = (int*)malloc(41);
  a[10] = 1;
  free(a);
  printf("call from foo\n");
}

