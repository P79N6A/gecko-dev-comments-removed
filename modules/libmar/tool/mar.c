





































#include <stdio.h>
#include "mar.h"

#ifdef XP_WIN
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

static void print_usage() {
    printf("usage: mar [-C dir] {-c|-x|-t} archive.mar [files...]\n");
}

static int mar_test_callback(MarFile *mar, const MarItem *item, void *unused) {
  printf("%u\t0%o\t%s\n", item->length, item->flags, item->name);
  return 0;
}

static int mar_test(const char *path) {
  MarFile *mar;

  mar = mar_open(path);
  if (!mar)
    return -1;

  printf("SIZE\tMODE\tNAME\n");
  mar_enum_items(mar, mar_test_callback, NULL);

  mar_close(mar);
  return 0;
}

int main(int argc, char **argv) {
  int command;

  if (argc < 3) {
    print_usage();
    return -1;
  }

  if (argv[1][1] == 'C') {
    chdir(argv[2]);
    argv += 2;
    argc -= 2;
  }

  switch (argv[1][1]) {
  case 'c':
    return mar_create(argv[2], argc - 3, argv + 3);
  case 't':
    return mar_test(argv[2]);
  case 'x':
    return mar_extract(argv[2]);
  default:
    print_usage();
    return -1;
  }

  return 0;
}
