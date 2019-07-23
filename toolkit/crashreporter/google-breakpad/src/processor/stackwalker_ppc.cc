



































#include "processor/stackwalker_ppc.h"
#include "google/call_stack.h"
#include "google/stack_frame_cpu.h"
#include "processor/minidump.h"

namespace google_airbag {


StackwalkerPPC::StackwalkerPPC(const MDRawContextPPC *context,
                               MemoryRegion *memory,
                               MinidumpModuleList *modules,
                               SymbolSupplier *supplier)
    : Stackwalker(memory, modules, supplier),
      context_(context) {
  if (memory_->GetBase() + memory_->GetSize() - 1 > 0xffffffff) {
    
    
    
    memory_ = NULL;
  }
}


StackFrame* StackwalkerPPC::GetContextFrame() {
  if (!context_ || !memory_)
    return NULL;

  StackFramePPC *frame = new StackFramePPC();

  
  
  frame->context = *context_;
  frame->context_validity = StackFramePPC::CONTEXT_VALID_ALL;
  frame->instruction = frame->context.srr0;

  return frame;
}


StackFrame* StackwalkerPPC::GetCallerFrame(
    const CallStack *stack,
    const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) {
  if (!memory_ || !stack)
    return NULL;

  
  
  
  
  
  
  
  

  StackFramePPC *last_frame = static_cast<StackFramePPC*>(
      stack->frames()->back());

  
  
  
  u_int32_t stack_pointer;
  if (!memory_->GetMemoryAtAddress(last_frame->context.gpr[1],
                                   &stack_pointer) ||
      stack_pointer <= last_frame->context.gpr[1]) {
    return NULL;
  }

  
  
  
  
  
  u_int32_t instruction;
  if (!memory_->GetMemoryAtAddress(stack_pointer + 8, &instruction) ||
      instruction <= 1) {
    return NULL;
  }

  StackFramePPC *frame = new StackFramePPC();

  frame->context = last_frame->context;
  frame->context.srr0 = instruction;
  frame->context.gpr[1] = stack_pointer;
  frame->context_validity = StackFramePPC::CONTEXT_VALID_SRR0 |
                            StackFramePPC::CONTEXT_VALID_GPR1;

  
  
  
  
  
  
  
  frame->instruction = frame->context.srr0 - 4;

  return frame;
}


}  
