


































#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/stack_frame.h"

namespace google_breakpad {

CallStack::~CallStack() {
  Clear();
}

void CallStack::Clear() {
  for (vector<StackFrame *>::const_iterator iterator = frames_.begin();
       iterator != frames_.end();
       ++iterator) {
    delete *iterator;
  }
}

}  
