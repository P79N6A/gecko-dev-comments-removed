


































#include <assert.h>

#include "common/scoped_ptr.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/cfi_frame_info.h"
#include "common/logging.h"
#include "processor/stackwalker_amd64.h"

namespace google_breakpad {


const StackwalkerAMD64::CFIWalker::RegisterSet
StackwalkerAMD64::cfi_register_map_[] = {
  
  
  
  
  
  { ToUniqueString("$rax"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RAX, &MDRawContextAMD64::rax },
  { ToUniqueString("$rdx"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RDX, &MDRawContextAMD64::rdx },
  { ToUniqueString("$rcx"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RCX, &MDRawContextAMD64::rcx },
  { ToUniqueString("$rbx"), NULL, true,
    StackFrameAMD64::CONTEXT_VALID_RBX, &MDRawContextAMD64::rbx },
  { ToUniqueString("$rsi"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RSI, &MDRawContextAMD64::rsi },
  { ToUniqueString("$rdi"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RDI, &MDRawContextAMD64::rdi },
  { ToUniqueString("$rbp"), NULL, true,
    StackFrameAMD64::CONTEXT_VALID_RBP, &MDRawContextAMD64::rbp },
  { ToUniqueString("$rsp"), ToUniqueString(".cfa"), false,
    StackFrameAMD64::CONTEXT_VALID_RSP, &MDRawContextAMD64::rsp },
  { ToUniqueString("$r8"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R8,  &MDRawContextAMD64::r8 },
  { ToUniqueString("$r9"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R9,  &MDRawContextAMD64::r9 },
  { ToUniqueString("$r10"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R10, &MDRawContextAMD64::r10 },
  { ToUniqueString("$r11"), NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R11, &MDRawContextAMD64::r11 },
  { ToUniqueString("$r12"), NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R12, &MDRawContextAMD64::r12 },
  { ToUniqueString("$r13"), NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R13, &MDRawContextAMD64::r13 },
  { ToUniqueString("$r14"), NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R14, &MDRawContextAMD64::r14 },
  { ToUniqueString("$r15"), NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R15, &MDRawContextAMD64::r15 },
  { ToUniqueString("$rip"), ToUniqueString(".ra"), false,
    StackFrameAMD64::CONTEXT_VALID_RIP, &MDRawContextAMD64::rip },
};

StackwalkerAMD64::StackwalkerAMD64(const SystemInfo* system_info,
                                   const MDRawContextAMD64* context,
                                   MemoryRegion* memory,
                                   const CodeModules* modules,
                                   StackFrameSymbolizer* resolver_helper)
    : Stackwalker(system_info, memory, modules, resolver_helper),
      context_(context),
      cfi_walker_(cfi_register_map_,
                  (sizeof(cfi_register_map_) / sizeof(cfi_register_map_[0]))) {
}

uint64_t StackFrameAMD64::ReturnAddress() const
{
  assert(context_validity & StackFrameAMD64::CONTEXT_VALID_RIP);
  return context.rip;   
}

StackFrame* StackwalkerAMD64::GetContextFrame() {
  if (!context_) {
    BPLOG(ERROR) << "Can't get context frame without context";
    return NULL;
  }

  StackFrameAMD64* frame = new StackFrameAMD64();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameAMD64::CONTEXT_VALID_ALL;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.rip;

  return frame;
}

StackFrameAMD64* StackwalkerAMD64::GetCallerByCFIFrameInfo(
    const vector<StackFrame*> &frames,
    CFIFrameInfo* cfi_frame_info) {
  StackFrameAMD64* last_frame = static_cast<StackFrameAMD64*>(frames.back());

  scoped_ptr<StackFrameAMD64> frame(new StackFrameAMD64());
  if (!cfi_walker_
      .FindCallerRegisters(*memory_, *cfi_frame_info,
                           last_frame->context, last_frame->context_validity,
                           &frame->context, &frame->context_validity))
    return NULL;

  
  static const int essentials = (StackFrameAMD64::CONTEXT_VALID_RIP
                                 | StackFrameAMD64::CONTEXT_VALID_RSP);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  frame->trust = StackFrame::FRAME_TRUST_CFI;
  return frame.release();
}

StackFrameAMD64* StackwalkerAMD64::GetCallerByStackScan(
    const vector<StackFrame*> &frames) {
  StackFrameAMD64* last_frame = static_cast<StackFrameAMD64*>(frames.back());
  uint64_t last_rsp = last_frame->context.rsp;
  uint64_t caller_rip_address, caller_rip;

  if (!ScanForReturnAddress(last_rsp, &caller_rip_address, &caller_rip)) {
    
    return NULL;
  }

  
  
  StackFrameAMD64* frame = new StackFrameAMD64();

  frame->trust = StackFrame::FRAME_TRUST_SCAN;
  frame->context = last_frame->context;
  frame->context.rip = caller_rip;
  
  
  frame->context.rsp = caller_rip_address + 8;
  frame->context_validity = StackFrameAMD64::CONTEXT_VALID_RIP |
                            StackFrameAMD64::CONTEXT_VALID_RSP;

  
  
  if (last_frame->context_validity & StackFrameAMD64::CONTEXT_VALID_RBP) {
    
    
    
    
    if (caller_rip_address - 8 == last_frame->context.rbp) {
      uint64_t caller_rbp = 0;
      if (memory_->GetMemoryAtAddress(last_frame->context.rbp, &caller_rbp) &&
          caller_rbp > caller_rip_address) {
        frame->context.rbp = caller_rbp;
        frame->context_validity |= StackFrameAMD64::CONTEXT_VALID_RBP;
      }
    } else if (last_frame->context.rbp >= caller_rip_address + 8) {
      
      
      frame->context.rbp = last_frame->context.rbp;
      frame->context_validity |= StackFrameAMD64::CONTEXT_VALID_RBP;
    }
  }

  return frame;
}

StackFrame* StackwalkerAMD64::GetCallerFrame(const CallStack* stack,
                                             bool stack_scan_allowed) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  const vector<StackFrame*> &frames = *stack->frames();
  StackFrameAMD64* last_frame = static_cast<StackFrameAMD64*>(frames.back());
  scoped_ptr<StackFrameAMD64> new_frame;

  
  scoped_ptr<CFIFrameInfo> cfi_frame_info(
      frame_symbolizer_->FindCFIFrameInfo(last_frame));
  if (cfi_frame_info.get())
    new_frame.reset(GetCallerByCFIFrameInfo(frames, cfi_frame_info.get()));

  
  
  if (stack_scan_allowed && !new_frame.get()) {
    new_frame.reset(GetCallerByStackScan(frames));
  }

  
  if (!new_frame.get())
    return NULL;

  
  if (new_frame->context.rip == 0)
    return NULL;

  
  
  
  if (new_frame->context.rsp <= last_frame->context.rsp)
    return NULL;

  
  
  
  
  
  new_frame->instruction = new_frame->context.rip - 1;

  return new_frame.release();
}

}  
