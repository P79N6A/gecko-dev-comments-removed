






































#ifndef PROCESSOR_STACKWALKER_ARM_H__
#define PROCESSOR_STACKWALKER_ARM_H__

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"

namespace google_breakpad {

class CodeModules;

class StackwalkerARM : public Stackwalker {
 public:
  
  
  
  
  StackwalkerARM(const SystemInfo* system_info,
                 const MDRawContextARM* context,
                 int fp_register,
                 MemoryRegion* memory,
                 const CodeModules* modules,
                 StackFrameSymbolizer* frame_symbolizer);

  
  
  
  void SetContextFrameValidity(int valid) { context_frame_validity_ = valid; }

 private:
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(const CallStack* stack);

  
  
  
  StackFrameARM* GetCallerByCFIFrameInfo(const vector<StackFrame*> &frames,
                                         CFIFrameInfo* cfi_frame_info);

  
  
  StackFrameARM* GetCallerByFramePointer(const vector<StackFrame*> &frames);

  
  
  StackFrameARM* GetCallerByStackScan(const vector<StackFrame*> &frames);

  
  
  const MDRawContextARM* context_;

  
  
  int fp_register_;

  
  
  
  int context_frame_validity_;
};


}  


#endif  
