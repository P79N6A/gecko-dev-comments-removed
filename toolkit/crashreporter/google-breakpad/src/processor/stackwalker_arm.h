






































#ifndef PROCESSOR_STACKWALKER_ARM_H__
#define PROCESSOR_STACKWALKER_ARM_H__


#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"

namespace google_breakpad {

class CodeModules;

class StackwalkerARM : public Stackwalker {
 public:
  
  
  
  
  StackwalkerARM(const SystemInfo *system_info,
                 const MDRawContextARM *context,
                 MemoryRegion *memory,
                 const CodeModules *modules,
                 SymbolSupplier *supplier,
                 SourceLineResolverInterface *resolver);

  
  
  
  void SetContextFrameValidity(int valid) { context_frame_validity_ = valid; }

 private:
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(const CallStack *stack);

  
  
  const MDRawContextARM *context_;

  
  
  
  int context_frame_validity_;
};


}  


#endif  
