











































#ifndef GOOGLE_AIRBAG_PROCESSOR_CALL_STACK_H__
#define GOOGLE_AIRBAG_PROCESSOR_CALL_STACK_H__

#include <vector>

namespace google_airbag {

using std::vector;

struct StackFrame;
template<typename T> class linked_ptr;

class CallStack {
 public:
  ~CallStack();

  const vector<StackFrame*>* frames() const { return &frames_; }

 private:
  
  friend class Stackwalker;

  
  CallStack() : frames_() {}

  
  vector<StackFrame*> frames_;
};

}  

#endif  
