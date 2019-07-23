




































#ifndef PROCESSOR_STACKWALKER_SPARC_H__
#define PROCESSOR_STACKWALKER_SPARC_H__


#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"

namespace google_breakpad {

class CodeModules;

class StackwalkerSPARC : public Stackwalker {
 public:
  
  
  
  
  StackwalkerSPARC(const SystemInfo *system_info,
                   const MDRawContextSPARC *context,
                   MemoryRegion *memory,
                   const CodeModules *modules,
                   SymbolSupplier *supplier,
                   SourceLineResolverInterface *resolver);

 private:
  
  
  
  
  
  
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info);

  
  
  const MDRawContextSPARC *context_;
};


}  


#endif  
