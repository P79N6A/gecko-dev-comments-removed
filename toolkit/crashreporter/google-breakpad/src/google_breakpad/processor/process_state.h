
































#ifndef GOOGLE_BREAKPAD_PROCESSOR_PROCESS_STATE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_PROCESS_STATE_H__

#include <string>
#include <vector>
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/system_info.h"
#include "google_breakpad/processor/minidump.h"

namespace google_breakpad {

using std::string;
using std::vector;

class CallStack;
class CodeModules;

class ProcessState {
 public:
  ProcessState() : modules_(NULL) { Clear(); }
  ~ProcessState();

  
  void Clear();

  
  u_int32_t time_date_stamp() const { return time_date_stamp_; }
  bool crashed() const { return crashed_; }
  string crash_reason() const { return crash_reason_; }
  u_int64_t crash_address() const { return crash_address_; }
  string assertion() const { return assertion_; }
  int requesting_thread() const { return requesting_thread_; }
  const vector<CallStack*>* threads() const { return &threads_; }
  const vector<MinidumpMemoryRegion*>* thread_memory_regions() const {
    return &thread_memory_regions_;
  }
  const SystemInfo* system_info() const { return &system_info_; }
  const CodeModules* modules() const { return modules_; }

 private:
  
  friend class MinidumpProcessor;

  
  u_int32_t time_date_stamp_;

  
  
  bool crashed_;

  
  
  
  
  string crash_reason_;

  
  
  
  
  u_int64_t crash_address_;

  
  
  
  string assertion_;

  
  
  
  
  
  
  
  
  int requesting_thread_;

  
  
  vector<CallStack*> threads_;
  vector<MinidumpMemoryRegion*> thread_memory_regions_;

  
  SystemInfo system_info_;

  
  
  const CodeModules *modules_;
};

}  

#endif  
