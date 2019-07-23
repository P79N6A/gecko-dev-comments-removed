



































#include "processor/stackwalker_arm.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/logging.h"

namespace google_breakpad {


StackwalkerARM::StackwalkerARM(const SystemInfo *system_info,
                               const MDRawContextARM *context,
                               MemoryRegion *memory,
                               const CodeModules *modules,
                               SymbolSupplier *supplier,
                               SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context) {
}


StackFrame* StackwalkerARM::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameARM *frame = new StackFrameARM();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameARM::CONTEXT_VALID_ALL;
  frame->instruction = frame->context.iregs[15];

  return frame;
}


StackFrame* StackwalkerARM::GetCallerFrame(
    const CallStack *stack,
    const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  StackFrameARM *last_frame = static_cast<StackFrameARM*>(
      stack->frames()->back());

  
  
  return NULL;
}


}  
