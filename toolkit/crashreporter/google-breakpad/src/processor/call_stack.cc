


































#include "google_airbag/processor/call_stack.h"
#include "google_airbag/processor/stack_frame.h"

namespace google_airbag {

CallStack::~CallStack() {
  for (vector<StackFrame *>::const_iterator iterator = frames_.begin();
       iterator != frames_.end();
       ++iterator) {
    delete *iterator;
  }
}

}  
