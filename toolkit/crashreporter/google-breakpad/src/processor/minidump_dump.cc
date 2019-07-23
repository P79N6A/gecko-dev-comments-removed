

































#include <cstdio>

#include "google_airbag/processor/minidump.h"

namespace {

using google_airbag::Minidump;
using google_airbag::MinidumpThreadList;
using google_airbag::MinidumpModuleList;
using google_airbag::MinidumpMemoryList;
using google_airbag::MinidumpException;
using google_airbag::MinidumpSystemInfo;
using google_airbag::MinidumpMiscInfo;
using google_airbag::MinidumpAirbagInfo;

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

  MinidumpAirbagInfo *airbag_info = minidump.GetAirbagInfo();
  if (!airbag_info) {
    
    printf("minidump.GetAirbagInfo() failed\n");
  } else {
    airbag_info->Print();
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
