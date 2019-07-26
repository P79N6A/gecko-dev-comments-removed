




























#include <stdio.h>
#include <string>

#include "common/linux/dump_symbols.h"

using google_breakpad::WriteSymbolFile;

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s <binary-with-debugging-info> "
            "[directory-for-debug-file]\n", argv[0]);
    return 1;
  }

  const char *binary = argv[1];
  std::string debug_dir;
  if (argc == 3)
    debug_dir = argv[2];

  if (!WriteSymbolFile(binary, debug_dir, stdout)) {
    fprintf(stderr, "Failed to write symbol file.\n");
    return 1;
  }

  return 0;
}
