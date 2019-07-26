




























#include <stdio.h>

#include <cstring>
#include <iostream>
#include <string>

#include "common/linux/dump_symbols.h"

using google_breakpad::WriteSymbolFile;

int usage(const char* self) {
  fprintf(stderr, "Usage: %s [OPTION] <binary-with-debugging-info> "
          "[directory-for-debug-file]\n\n", self);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c    Do not generate CFI section\n");
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 4)
    return usage(argv[0]);

  bool cfi = true;
  if (strcmp("-c", argv[1]) == 0)
    cfi = false;
  if (!cfi && argc == 2)
    return usage(argv[0]);

  const char *binary;
  std::string debug_dir;
  if (cfi) {
    binary = argv[1];
    if (argc == 3)
      debug_dir = argv[2];
  } else {
    binary = argv[2];
    if (argc == 4)
      debug_dir = argv[3];
  }

  if (!WriteSymbolFile(binary, debug_dir, cfi, std::cout)) {
    fprintf(stderr, "Failed to write symbol file.\n");
    return 1;
  }

  return 0;
}
