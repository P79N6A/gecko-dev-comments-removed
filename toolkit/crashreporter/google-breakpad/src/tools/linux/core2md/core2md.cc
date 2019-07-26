






























#include <stdio.h>

#include "client/linux/minidump_writer/minidump_writer.h"
#include "client/linux/minidump_writer/linux_core_dumper.h"

using google_breakpad::AppMemoryList;
using google_breakpad::MappingList;
using google_breakpad::LinuxCoreDumper;

static int ShowUsage(const char* argv0) {
  fprintf(stderr, "Usage: %s <core file> <procfs dir> <output>\n", argv0);
  return 1;
}

bool WriteMinidumpFromCore(const char* filename,
                           const char* core_path,
                           const char* procfs_override) {
  MappingList mappings;
  AppMemoryList memory_list;
  LinuxCoreDumper dumper(0, core_path, procfs_override);
  return google_breakpad::WriteMinidump(filename, mappings, memory_list,
                                        &dumper);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    return ShowUsage(argv[0]);
  }

  const char* core_file = argv[1];
  const char* procfs_dir = argv[2];
  const char* minidump_file = argv[3];
  if (!WriteMinidumpFromCore(minidump_file,
                             core_file,
                             procfs_dir)) {
    fprintf(stderr, "Unable to generate minidump.\n");
    return 1;
  }

  return 0;
}
