




































#ifndef PROCESSOR_STACKWALKER_X86_H__
#define PROCESSOR_STACKWALKER_X86_H__


#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"

namespace google_breakpad {

class CodeModules;


class StackwalkerX86 : public Stackwalker {
 public:
  
  
  
  
  StackwalkerX86(const SystemInfo *system_info,
                 const MDRawContextX86 *context,
                 MemoryRegion *memory,
                 const CodeModules *modules,
                 SymbolSupplier *supplier,
                 SourceLineResolverInterface *resolver);

 private:
  
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info);

  
  
  
  
  
  
  
  
  
  bool ScanForReturnAddress(u_int32_t location_start,
                            u_int32_t &location_found,
                            u_int32_t &eip_found);

  
  
  const MDRawContextX86 *context_;
};


}  


#endif  
