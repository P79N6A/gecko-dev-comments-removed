




























#include <stdio.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "common/linux/dump_symbols.h"

using google_breakpad::WriteSymbolFile;

int usage(const char* self) {
  fprintf(stderr, "Usage: %s [OPTION] <binary-with-debugging-info> "
          "[directories-for-debug-file]\n\n", self);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c    Do not generate CFI section\n");
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 4)
    return usage(argv[0]);

  bool cfi = true;
  int binary_index = 1;
  if (strcmp("-c", argv[1]) == 0) {
    cfi = false;
    ++binary_index;
  }
  if (!cfi && argc == 2)
    return usage(argv[0]);

  const char *binary;
  std::vector<string> debug_dirs;
  binary = argv[binary_index];
  for (int debug_dir_index = binary_index + 1;
       debug_dir_index < argc;
       ++debug_dir_index) {
    debug_dirs.push_back(argv[debug_dir_index]);
  }

  SymbolData symbol_data = cfi ? ALL_SYMBOL_DATA : NO_CFI;
  if (!WriteSymbolFile(binary, debug_dirs, symbol_data, std::cout)) {
    fprintf(stderr, "Failed to write symbol file.\n");
    return 1;
  }

  return 0;
}
