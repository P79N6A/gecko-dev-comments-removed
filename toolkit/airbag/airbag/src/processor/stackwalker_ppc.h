




































#ifndef PROCESSOR_STACKWALKER_PPC_H__
#define PROCESSOR_STACKWALKER_PPC_H__


#include "google_airbag/common/airbag_types.h"
#include "google_airbag/common/minidump_format.h"
#include "google_airbag/processor/stackwalker.h"

namespace google_airbag {

class MinidumpContext;
class MinidumpModuleList;


class StackwalkerPPC : public Stackwalker {
 public:
  
  
  
  
  StackwalkerPPC(const MDRawContextPPC *context,
                 MemoryRegion *memory,
                 MinidumpModuleList *modules,
                 SymbolSupplier *supplier);

 private:
  
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info);

  
  
  const MDRawContextPPC *context_;
};


}  


#endif  
