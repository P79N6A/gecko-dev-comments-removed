











































#ifndef GOOGLE_BREAKPAD_PROCESSOR_CALL_STACK_H__
#define GOOGLE_BREAKPAD_PROCESSOR_CALL_STACK_H__

#include <vector>

namespace google_breakpad {

using std::vector;

struct StackFrame;
template<typename T> class linked_ptr;

class CallStack {
 public:
  CallStack() { Clear(); }
  ~CallStack();

  
  void Clear();
  
  const vector<StackFrame*>* frames() const { return &frames_; }

 private:
  
  friend class Stackwalker;

  
  vector<StackFrame*> frames_;
};

}  

#endif  
