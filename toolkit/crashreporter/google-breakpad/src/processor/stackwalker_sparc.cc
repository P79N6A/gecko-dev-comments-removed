



































#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "common/logging.h"
#include "processor/stackwalker_sparc.h"

namespace google_breakpad {


StackwalkerSPARC::StackwalkerSPARC(const SystemInfo* system_info,
                                   const MDRawContextSPARC* context,
                                   MemoryRegion* memory,
                                   const CodeModules* modules,
                                   StackFrameSymbolizer* resolver_helper)
    : Stackwalker(system_info, memory, modules, resolver_helper),
      context_(context) {
}


StackFrame* StackwalkerSPARC::GetContextFrame() {
  if (!context_) {
    BPLOG(ERROR) << "Can't get context frame without context";
    return NULL;
  }

  StackFrameSPARC* frame = new StackFrameSPARC();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameSPARC::CONTEXT_VALID_ALL;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.pc;

  return frame;
}


StackFrame* StackwalkerSPARC::GetCallerFrame(const CallStack* stack,
                                             bool stack_scan_allowed) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  StackFrameSPARC* last_frame = static_cast<StackFrameSPARC*>(
      stack->frames()->back());

  
  
  
  
  
  
  
  

  
  
  
  uint64_t stack_pointer = last_frame->context.g_r[30];
  if (stack_pointer <= last_frame->context.g_r[14]) {
    return NULL;
  }

  uint32_t instruction;
  if (!memory_->GetMemoryAtAddress(stack_pointer + 60,
                     &instruction) || instruction <= 1) {
    return NULL;
  }

  uint32_t stack_base;
  if (!memory_->GetMemoryAtAddress(stack_pointer + 56,
                     &stack_base) || stack_base <= 1) {
    return NULL;
  }

  StackFrameSPARC* frame = new StackFrameSPARC();

  frame->context = last_frame->context;
  frame->context.g_r[14] = stack_pointer;
  frame->context.g_r[30] = stack_base;

  
  
  
  
  
  
  
  
  frame->context.pc = instruction + 8;
  frame->instruction = instruction;
  frame->context_validity = StackFrameSPARC::CONTEXT_VALID_PC |
                            StackFrameSPARC::CONTEXT_VALID_SP |
                            StackFrameSPARC::CONTEXT_VALID_FP;
  frame->trust = StackFrame::FRAME_TRUST_FP;

  return frame;
}


}  
