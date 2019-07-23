



































#include "processor/stackwalker_amd64.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/logging.h"

namespace google_breakpad {


StackwalkerAMD64::StackwalkerAMD64(const SystemInfo *system_info,
                                   const MDRawContextAMD64 *context,
                                   MemoryRegion *memory,
                                   const CodeModules *modules,
                                   SymbolSupplier *supplier,
                                   SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context) {
}


StackFrame* StackwalkerAMD64::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameAMD64 *frame = new StackFrameAMD64();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameAMD64::CONTEXT_VALID_ALL;
  frame->instruction = frame->context.rip;

  return frame;
}


StackFrame* StackwalkerAMD64::GetCallerFrame(
    const CallStack *stack,
    const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  StackFrameAMD64 *last_frame = static_cast<StackFrameAMD64*>(
      stack->frames()->back());

  
  
  
  
  
  

  
  
  
  u_int64_t stack_pointer = last_frame->context.rbp + 16;
  if (stack_pointer <= last_frame->context.rsp) {
    return NULL;
  }

  u_int64_t instruction;
  if (!memory_->GetMemoryAtAddress(last_frame->context.rbp + 8,
                                   &instruction) ||
      instruction <= 1) {
    return NULL;
  }

  u_int64_t stack_base;
  if (!memory_->GetMemoryAtAddress(last_frame->context.rbp,
                                   &stack_base) ||
      stack_base <= 1) {
    return NULL;
  }

  StackFrameAMD64 *frame = new StackFrameAMD64();

  frame->context = last_frame->context;
  frame->context.rip = instruction;
  frame->context.rsp = stack_pointer;
  frame->context.rbp = stack_base;
  frame->context_validity = StackFrameAMD64::CONTEXT_VALID_RIP |
                            StackFrameAMD64::CONTEXT_VALID_RSP |
                            StackFrameAMD64::CONTEXT_VALID_RBP;

  frame->instruction = frame->context.rip - 1;

  return frame;
}


}  
