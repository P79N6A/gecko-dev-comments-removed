


































#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_modules.h"

namespace google_breakpad {

ProcessState::~ProcessState() {
  Clear();
}

void ProcessState::Clear() {
  time_date_stamp_ = 0;
  crashed_ = false;
  crash_reason_.clear();
  crash_address_ = 0;
  requesting_thread_ = -1;
  for (vector<CallStack *>::const_iterator iterator = threads_.begin();
       iterator != threads_.end();
       ++iterator) {
    delete *iterator;
  }
  threads_.clear();
  system_info_.Clear();
  delete modules_;
  modules_ = NULL;
}

}  
