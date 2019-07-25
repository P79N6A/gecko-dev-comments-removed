



#include <stdio.h>

int main(int argc, char *argv[])
{
#ifdef __OPTIMIZE__
  printf("Using an optimization flag\n");
#else
  printf("Using no optimization flag\n");
#endif
  return 0;
}
