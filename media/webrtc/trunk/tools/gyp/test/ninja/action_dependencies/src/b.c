




#include <stdio.h>

#include "b.h"

int main(int argc, char** argv) {
  FILE* f;
  if (argc < 2)
    return 1;
  f = fopen(argv[1], "wt");
  fprintf(f, "#define VALUE %d\n", funcA());
  fclose(f);
  return 0;
}
