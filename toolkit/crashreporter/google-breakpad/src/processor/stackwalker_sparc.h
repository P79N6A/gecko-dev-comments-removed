




































#ifndef PROCESSOR_STACKWALKER_SPARC_H__
#define PROCESSOR_STACKWALKER_SPARC_H__


#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"

namespace google_breakpad {

class CodeModules;

class StackwalkerSPARC : public Stackwalker {
 public:
  
  
  
  
  StackwalkerSPARC(const SystemInfo* system_info,
                   const MDRawContextSPARC* context,
                   MemoryRegion* memory,
                   const CodeModules* modules,
                   StackFrameSymbolizer* frame_symbolizer);

 private:
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(const CallStack* stack);

  
  
  const MDRawContextSPARC* context_;
};


}  


#endif  
