



#include <stdio.h>

int func1(void) {
  return 42;
}

int main(int argc, char *argv[]) {
  printf("Hello, world!\n");
  printf("%d\n", func1());
  return 0;
}
