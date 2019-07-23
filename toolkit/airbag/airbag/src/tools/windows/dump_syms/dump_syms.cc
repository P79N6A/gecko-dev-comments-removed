































#include <stdio.h>

#include <string>

#include "common/windows/pdb_source_line_writer.h"

using std::wstring;
using google_breakpad::PDBSourceLineWriter;

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %ws <file.[pdb|exe|dll]>\n", argv[0]);
    return 1;
  }

  PDBSourceLineWriter writer;
  if (!writer.Open(wstring(argv[1]), PDBSourceLineWriter::ANY_FILE)) {
    fprintf(stderr, "Open failed\n");
    return 1;
  }

  if (!writer.WriteMap(stdout)) {
    fprintf(stderr, "WriteMap failed\n");
    return 1;
  }

  writer.Close();
  return 0;
}
