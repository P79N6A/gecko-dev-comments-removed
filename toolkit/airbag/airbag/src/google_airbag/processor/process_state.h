
































#ifndef GOOGLE_AIRBAG_PROCESSOR_PROCESS_STATE_H__
#define GOOGLE_AIRBAG_PROCESSOR_PROCESS_STATE_H__

#include <string>
#include <vector>

namespace google_airbag {

using std::string;
using std::vector;

class CallStack;

class ProcessState {
 public:
  ~ProcessState();

  
  bool crashed() const { return crashed_; }
  string crash_reason() const { return crash_reason_; }
  u_int64_t crash_address() const { return crash_address_; }
  int requesting_thread() const { return requesting_thread_; }
  const vector<CallStack*>* threads() const { return &threads_; }
  string os() const { return os_; }
  string os_version() const { return os_version_; }
  string cpu() const { return cpu_; }
  string cpu_info() const { return cpu_info_; }

 private:
  
  friend class MinidumpProcessor;

  
  ProcessState() : crashed_(false), crash_reason_(), crash_address_(0),
                   requesting_thread_(-1), threads_(), os_(), os_version_(),
                   cpu_(), cpu_info_() {}

  
  
  bool crashed_;

  
  
  
  
  string crash_reason_;

  
  
  
  
  u_int64_t crash_address_;

  
  
  
  
  
  
  
  
  int requesting_thread_;

  
  
  vector<CallStack*> threads_;

  
  
  
  
  string os_;

  
  
  
  string os_version_;

  
  
  
  
  string cpu_;

  
  
  
  
  string cpu_info_;
};

}  

#endif  
