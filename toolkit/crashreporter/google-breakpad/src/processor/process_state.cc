


































#include "google_airbag/processor/process_state.h"
#include "google_airbag/processor/call_stack.h"

namespace google_airbag {

ProcessState::~ProcessState() {
  for (vector<CallStack *>::const_iterator iterator = threads_.begin();
       iterator != threads_.end();
       ++iterator) {
    delete *iterator;
  }
}

}  
