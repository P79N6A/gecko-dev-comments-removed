

































#include <cstdio>

#include "google_breakpad/processor/minidump.h"
#include "processor/logging.h"

namespace {

using google_breakpad::Minidump;
using google_breakpad::MinidumpThreadList;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpAssertion;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpMiscInfo;
using google_breakpad::MinidumpBreakpadInfo;

static bool PrintMinidumpDump(const char *minidump_file) {
  Minidump minidump(minidump_file);
  if (!minidump.Read()) {
    BPLOG(ERROR) << "minidump.Read() failed";
    return false;
  }
  minidump.Print();

  int errors = 0;

  MinidumpThreadList *thread_list = minidump.GetThreadList();
  if (!thread_list) {
    ++errors;
    BPLOG(ERROR) << "minidump.GetThreadList() failed";
  } else {
    thread_list->Print();
  }

  MinidumpModuleList *module_list = minidump.GetModuleList();
  if (!module_list) {
    ++errors;
    BPLOG(ERROR) << "minidump.GetModuleList() failed";
  } else {
    module_list->Print();
  }

  MinidumpMemoryList *memory_list = minidump.GetMemoryList();
  if (!memory_list) {
    ++errors;
    BPLOG(ERROR) << "minidump.GetMemoryList() failed";
  } else {
    memory_list->Print();
  }

  MinidumpException *exception = minidump.GetException();
  if (!exception) {
    BPLOG(INFO) << "minidump.GetException() failed";
  } else {
    exception->Print();
  }

  MinidumpAssertion *assertion = minidump.GetAssertion();
  if (!assertion) {
    BPLOG(INFO) << "minidump.GetAssertion() failed";
  } else {
    assertion->Print();
  }

  MinidumpSystemInfo *system_info = minidump.GetSystemInfo();
  if (!system_info) {
    ++errors;
    BPLOG(ERROR) << "minidump.GetSystemInfo() failed";
  } else {
    system_info->Print();
  }

  MinidumpMiscInfo *misc_info = minidump.GetMiscInfo();
  if (!misc_info) {
    ++errors;
    BPLOG(ERROR) << "minidump.GetMiscInfo() failed";
  } else {
    misc_info->Print();
  }

  MinidumpBreakpadInfo *breakpad_info = minidump.GetBreakpadInfo();
  if (!breakpad_info) {
    
    BPLOG(INFO) << "minidump.GetBreakpadInfo() failed";
  } else {
    breakpad_info->Print();
  }

  return errors == 0;
}

}  

int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    return 1;
  }

  return PrintMinidumpDump(argv[1]) ? 0 : 1;
}
