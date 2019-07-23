




























#include <string>

#include "common/linux/dump_symbols.h"

using namespace google_breakpad;

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3)  {
    fprintf(stderr,
            "Usage: %s <binary-with-stab-symbol> [output-symbol-file]\n",
            argv[0]);
    return 1;
  }

  const char *binary = argv[1];
  std::string symbol_file(binary);
  symbol_file += ".sym";
  if (argc == 3)
    symbol_file = argv[2];

  DumpSymbols dumper;
  if (dumper.WriteSymbolFile(binary, symbol_file))
    printf("Symbol file successfully written: %s\n", symbol_file.c_str());
  else
    printf("Failed to write symbol file.\n");
  return 0;
}
