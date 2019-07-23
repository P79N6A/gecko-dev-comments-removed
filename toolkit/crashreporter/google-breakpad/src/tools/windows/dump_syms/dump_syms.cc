































#include <stdio.h>

#include <string>

#include "common/windows/pdb_source_line_writer.h"

using std::wstring;
using google_airbag::PDBSourceLineWriter;

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <pdb file>\n", argv[0]);
    return 1;
  }

  wchar_t filename[_MAX_PATH];
  if (mbstowcs_s(NULL, filename, argv[1], _MAX_PATH) == -1) {
    fprintf(stderr, "invalid multibyte character in %s\n", argv[1]);
    return 1;
  }

  PDBSourceLineWriter writer;
  if (!writer.Open(wstring(filename), PDBSourceLineWriter::PDB_FILE)) {
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
