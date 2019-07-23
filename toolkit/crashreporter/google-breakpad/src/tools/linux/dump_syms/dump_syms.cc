




























#include <string>
#include <cstdio>

#include "common/linux/dump_symbols.h"

using namespace google_breakpad;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <binary-with-stab-symbol>\n", argv[0]);
    return 1;
  }

  const char *binary = argv[1];

  DumpSymbols dumper;
  if (!dumper.WriteSymbolFile(binary, fileno(stdout))) {
    fprintf(stderr, "Failed to write symbol file.\n");
    return 1;
  }

  return 0;
}
