





#include <stdio.h>

void bar(void);
void car(void);
void dar(void);
void ear(void);

int main() {
  printf("{\n");
  bar();
  car();
  dar();
  ear();
  printf("}\n");
  return 0;
}
