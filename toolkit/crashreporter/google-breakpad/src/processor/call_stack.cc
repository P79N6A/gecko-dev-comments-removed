


































#include "google/call_stack.h"
#include "google/stack_frame.h"

namespace google_airbag {

CallStack::~CallStack() {
  for (vector<StackFrame *>::const_iterator iterator = frames_.begin();
       iterator != frames_.end();
       ++iterator) {
    delete *iterator;
  }
}

}  
