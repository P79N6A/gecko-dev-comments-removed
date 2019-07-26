

































#include <stdio.h>
#include <string.h>

#include "common/scoped_ptr.h"
#include "google_breakpad/processor/minidump.h"
#include "processor/logging.h"

namespace {

using google_breakpad::Minidump;
using google_breakpad::MinidumpThreadList;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpMemoryInfoList;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpAssertion;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpMiscInfo;
using google_breakpad::MinidumpBreakpadInfo;

static void DumpRawStream(Minidump *minidump,
                          uint32_t stream_type,
                          const char *stream_name,
                          int *errors) {
  uint32_t length = 0;
  if (!minidump->SeekToStreamType(stream_type, &length)) {
    return;
  }

  printf("Stream %s:\n", stream_name);

  if (length == 0) {
    printf("\n");
    return;
  }
  std::vector<char> contents(length);
  if (!minidump->ReadBytes(&contents[0], length)) {
    ++*errors;
    BPLOG(ERROR) << "minidump.ReadBytes failed";
    return;
  }
  size_t current_offset = 0;
  while (current_offset < length) {
    size_t remaining = length - current_offset;
    
    
    uint32_t int_remaining = remaining;
    printf("%.*s", int_remaining, &contents[current_offset]);
    char *next_null = reinterpret_cast<char *>(
        memchr(&contents[current_offset], 0, remaining));
    if (next_null == NULL)
      break;
    printf("\\0\n");
    size_t null_offset = next_null - &contents[0];
    current_offset = null_offset + 1;
  }
  printf("\n\n");
}

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

  MinidumpMemoryInfoList *memory_info_list = minidump.GetMemoryInfoList();
  if (!memory_info_list) {
    ++errors;
    BPLOG(ERROR) << "minidump.GetMemoryInfoList() failed";
  } else {
    memory_info_list->Print();
  }

  DumpRawStream(&minidump,
                MD_LINUX_CMD_LINE,
                "MD_LINUX_CMD_LINE",
                &errors);
  DumpRawStream(&minidump,
                MD_LINUX_ENVIRON,
                "MD_LINUX_ENVIRON",
                &errors);
  DumpRawStream(&minidump,
                MD_LINUX_LSB_RELEASE,
                "MD_LINUX_LSB_RELEASE",
                &errors);
  DumpRawStream(&minidump,
                MD_LINUX_PROC_STATUS,
                "MD_LINUX_PROC_STATUS",
                &errors);
  DumpRawStream(&minidump,
                MD_LINUX_CPU_INFO,
                "MD_LINUX_CPU_INFO",
                &errors);
  DumpRawStream(&minidump,
                MD_LINUX_MAPS,
                "MD_LINUX_MAPS",
                &errors);

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
