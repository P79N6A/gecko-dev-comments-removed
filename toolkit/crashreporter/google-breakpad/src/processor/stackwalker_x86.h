






































#ifndef PROCESSOR_STACKWALKER_X86_H__
#define PROCESSOR_STACKWALKER_X86_H__

#include <vector>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stackwalker.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/cfi_frame_info.h"

namespace google_breakpad {

class CodeModules;


class StackwalkerX86 : public Stackwalker {
 public:
  
  
  
  
  StackwalkerX86(const SystemInfo* system_info,
                 const MDRawContextX86* context,
                 MemoryRegion* memory,
                 const CodeModules* modules,
                 StackFrameSymbolizer* frame_symbolizer);

 private:
  
  typedef SimpleCFIWalker<uint32_t, MDRawContextX86> CFIWalker;

  
  
  
  
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(const CallStack* stack);

  
  
  
  StackFrameX86* GetCallerByWindowsFrameInfo(
      const vector<StackFrame*> &frames,
      WindowsFrameInfo* windows_frame_info);

  
  
  
  StackFrameX86* GetCallerByCFIFrameInfo(const vector<StackFrame*> &frames,
                                         CFIFrameInfo* cfi_frame_info);

  
  
  
  
  
  StackFrameX86* GetCallerByEBPAtBase(const vector<StackFrame*> &frames);

  
  
  const MDRawContextX86* context_;

  
  static const CFIWalker::RegisterSet cfi_register_map_[];

  
  const CFIWalker cfi_walker_;
};


}  


#endif  
