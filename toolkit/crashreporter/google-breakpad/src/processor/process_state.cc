


































#include "google/process_state.h"
#include "google/call_stack.h"

namespace google_airbag {

ProcessState::~ProcessState() {
  for (vector<CallStack *>::const_iterator iterator = threads_.begin();
       iterator != threads_.end();
       ++iterator) {
    delete *iterator;
  }
}

}  
