
































#ifndef GOOGLE_BREAKPAD_PROCESSOR_PROCESS_STATE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_PROCESS_STATE_H__

#include <string>
#include <vector>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/system_info.h"
#include "google_breakpad/processor/minidump.h"

namespace google_breakpad {

using std::vector;

class CallStack;
class CodeModules;

enum ExploitabilityRating {
  EXPLOITABILITY_HIGH,                    
                                          
                                          

  EXPLOITABLITY_MEDIUM,                   
                                          
                                          

  EXPLOITABILITY_LOW,                     
                                          
                                          
                                          
                                          

  EXPLOITABILITY_INTERESTING,             
                                          
                                          
                                          

  EXPLOITABILITY_NONE,                    
                                          

  EXPLOITABILITY_NOT_ANALYZED,            
                                          
                                          

  EXPLOITABILITY_ERR_NOENGINE,            
                                          
                                          

  EXPLOITABILITY_ERR_PROCESSING           
                                          
                                          
};

class ProcessState {
 public:
  ProcessState() : modules_(NULL) { Clear(); }
  ~ProcessState();

  
  void Clear();

  
  uint32_t time_date_stamp() const { return time_date_stamp_; }
  bool crashed() const { return crashed_; }
  string crash_reason() const { return crash_reason_; }
  uint64_t crash_address() const { return crash_address_; }
  string assertion() const { return assertion_; }
  int requesting_thread() const { return requesting_thread_; }
  const vector<CallStack*>* threads() const { return &threads_; }
  const vector<MinidumpMemoryRegion*>* thread_memory_regions() const {
    return &thread_memory_regions_;
  }
  const SystemInfo* system_info() const { return &system_info_; }
  const CodeModules* modules() const { return modules_; }
  const vector<const CodeModule*>* modules_without_symbols() const {
    return &modules_without_symbols_;
  }
  ExploitabilityRating exploitability() const { return exploitability_; }

 private:
  
  friend class MinidumpProcessor;

  
  uint32_t time_date_stamp_;

  
  
  bool crashed_;

  
  
  
  
  string crash_reason_;

  
  
  
  
  uint64_t crash_address_;

  
  
  
  string assertion_;

  
  
  
  
  
  
  
  
  int requesting_thread_;

  
  
  vector<CallStack*> threads_;
  vector<MinidumpMemoryRegion*> thread_memory_regions_;

  
  SystemInfo system_info_;

  
  
  const CodeModules *modules_;

  
  vector<const CodeModule*> modules_without_symbols_;

  
  
  
  ExploitabilityRating exploitability_;
};

}  

#endif  
