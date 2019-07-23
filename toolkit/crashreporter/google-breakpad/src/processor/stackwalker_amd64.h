




































#ifndef PROCESSOR_STACKWALKER_AMD64_H__
#define PROCESSOR_STACKWALKER_AMD64_H__


#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"

namespace google_breakpad {

class CodeModules;

class StackwalkerAMD64 : public Stackwalker {
 public:
  
  
  
  
  StackwalkerAMD64(const SystemInfo *system_info,
                   const MDRawContextAMD64 *context,
                   MemoryRegion *memory,
                   const CodeModules *modules,
                   SymbolSupplier *supplier,
                   SourceLineResolverInterface *resolver);

 private:
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info);

  
  
  const MDRawContextAMD64 *context_;
};


}  


#endif  
