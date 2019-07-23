




































#ifndef PROCESSOR_STACKWALKER_X86_H__
#define PROCESSOR_STACKWALKER_X86_H__


#include "google/airbag_types.h"
#include "processor/stackwalker.h"
#include "processor/minidump_format.h"

namespace google_airbag {

class MinidumpContext;
class MinidumpModuleList;


class StackwalkerX86 : public Stackwalker {
 public:
  
  
  
  
  StackwalkerX86(const MDRawContextX86 *context,
                 MemoryRegion *memory,
                 MinidumpModuleList *modules,
                 SymbolSupplier *supplier);

 private:
  
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info);

  
  
  const MDRawContextX86 *context_;
};


}  


#endif  
