

































#include <cstdio>

#include "google_breakpad/processor/minidump.h"

namespace {

using google_breakpad::Minidump;
using google_breakpad::MinidumpThreadList;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpMiscInfo;
using google_breakpad::MinidumpBreakpadInfo;

static bool PrintMinidumpDump(const char *minidump_file) {
  Minidump minidump(minidump_file);
  if (!minidump.Read()) {
    fprintf(stderr, "minidump.Read() failed\n");
    return false;
  }
  minidump.Print();

  int errors = 0;

  MinidumpThreadList *thread_list = minidump.GetThreadList();
  if (!thread_list) {
    ++errors;
    printf("minidump.GetThreadList() failed\n");
  } else {
    thread_list->Print();
  }

  MinidumpModuleList *module_list = minidump.GetModuleList();
  if (!module_list) {
    ++errors;
    printf("minidump.GetModuleList() failed\n");
  } else {
    module_list->Print();
  }

  MinidumpMemoryList *memory_list = minidump.GetMemoryList();
  if (!memory_list) {
    ++errors;
    printf("minidump.GetMemoryList() failed\n");
  } else {
    memory_list->Print();
  }

  MinidumpException *exception = minidump.GetException();
  if (!exception) {
    
    printf("minidump.GetException() failed\n");
  } else {
    exception->Print();
  }

  MinidumpSystemInfo *system_info = minidump.GetSystemInfo();
  if (!system_info) {
    ++errors;
    printf("minidump.GetSystemInfo() failed\n");
  } else {
    system_info->Print();
  }

  MinidumpMiscInfo *misc_info = minidump.GetMiscInfo();
  if (!misc_info) {
    ++errors;
    printf("minidump.GetMiscInfo() failed\n");
  } else {
    misc_info->Print();
  }

  MinidumpBreakpadInfo *breakpad_info = minidump.GetBreakpadInfo();
  if (!breakpad_info) {
    
    printf("minidump.GetBreakpadInfo() failed\n");
  } else {
    breakpad_info->Print();
  }

  return errors == 0;
}

}  

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    return 1;
  }

  return PrintMinidumpDump(argv[1]) ? 0 : 1;
}
