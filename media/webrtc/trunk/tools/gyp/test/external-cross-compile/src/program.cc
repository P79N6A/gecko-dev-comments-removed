





#include <stdio.h>

static char data[] = {
#include "cross_program.h"
};

int main(int argc, char *argv[]) {
  fwrite(data, 1, sizeof(data), stdout);
  return 0;
}
