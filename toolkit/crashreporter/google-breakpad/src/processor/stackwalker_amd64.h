




































#ifndef PROCESSOR_STACKWALKER_AMD64_H__
#define PROCESSOR_STACKWALKER_AMD64_H__

#include <vector>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/cfi_frame_info.h"

namespace google_breakpad {

class CodeModules;

class StackwalkerAMD64 : public Stackwalker {
 public:
  
  
  
  
  StackwalkerAMD64(const SystemInfo* system_info,
                   const MDRawContextAMD64* context,
                   MemoryRegion* memory,
                   const CodeModules* modules,
                   StackFrameSymbolizer* frame_symbolizer);

 private:
  
  typedef SimpleCFIWalker<uint64_t, MDRawContextAMD64> CFIWalker;

  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(const CallStack* stack,
                                     bool stack_scan_allowed);

  
  
  
  StackFrameAMD64* GetCallerByCFIFrameInfo(const vector<StackFrame*> &frames,
                                           CFIFrameInfo* cfi_frame_info);

  
  
  StackFrameAMD64* GetCallerByStackScan(const vector<StackFrame*> &frames);

  
  
  const MDRawContextAMD64* context_;

  
  static const CFIWalker::RegisterSet cfi_register_map_[];

  
  const CFIWalker cfi_walker_;
};


}  


#endif  
