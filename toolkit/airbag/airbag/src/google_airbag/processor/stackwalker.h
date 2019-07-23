







































#ifndef GOOGLE_AIRBAG_PROCESSOR_STACKWALKER_H__
#define GOOGLE_AIRBAG_PROCESSOR_STACKWALKER_H__

#include <vector>

namespace google_airbag {

class CallStack;
template<typename T> class linked_ptr;
class MemoryRegion;
class MinidumpContext;
class MinidumpModuleList;
struct StackFrame;
struct StackFrameInfo;
class SymbolSupplier;

using std::vector;


class Stackwalker {
 public:
  virtual ~Stackwalker() {}

  
  
  
  CallStack* Walk();

  
  
  
  static Stackwalker* StackwalkerForCPU(MinidumpContext *context,
                                        MemoryRegion *memory,
                                        MinidumpModuleList *modules,
                                        SymbolSupplier *supplier);

 protected:
  
  
  
  
  
  
  Stackwalker(MemoryRegion *memory,
              MinidumpModuleList *modules,
              SymbolSupplier *supplier);

  
  
  MemoryRegion *memory_;

 private:
  
  
  
  
  virtual StackFrame* GetContextFrame() = 0;

  
  
  
  
  
  
  
  
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) = 0;

  
  
  MinidumpModuleList *modules_;

  
  SymbolSupplier *supplier_;
};


}  


#endif  
