



































#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/cfi_frame_info.h"
#include "processor/logging.h"
#include "processor/scoped_ptr.h"
#include "processor/stackwalker_arm.h"

namespace google_breakpad {


StackwalkerARM::StackwalkerARM(const SystemInfo *system_info,
                               const MDRawContextARM *context,
                               MemoryRegion *memory,
                               const CodeModules *modules,
                               SymbolSupplier *supplier,
                               SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context), 
      context_frame_validity_(StackFrameARM::CONTEXT_VALID_ALL) { }


StackFrame* StackwalkerARM::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameARM *frame = new StackFrameARM();

  
  
  frame->context = *context_;
  frame->context_validity = context_frame_validity_;
  frame->instruction = frame->context.iregs[15];

  return frame;
}


StackFrame* StackwalkerARM::GetCallerFrame(const CallStack *stack) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  const vector<StackFrame *> &frames = *stack->frames();
  StackFrameARM *last_frame = static_cast<StackFrameARM *>(frames.back());

  
  scoped_ptr<CFIFrameInfo> cfi_frame_info(resolver_
                                          ->FindCFIFrameInfo(last_frame));
  if (cfi_frame_info == NULL)
    
    
    return NULL;

  static const char *register_names[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "sp",  "lr",  "pc",
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    "fps", "cpsr",
    NULL
  };

  
  CFIFrameInfo::RegisterValueMap<u_int32_t> callee_registers;
  for (int i = 0; register_names[i]; i++)
    if (last_frame->context_validity & StackFrameARM::RegisterValidFlag(i))
      callee_registers[register_names[i]] = last_frame->context.iregs[i];

  
  CFIFrameInfo::RegisterValueMap<u_int32_t> caller_registers;
  if (!cfi_frame_info->FindCallerRegs(callee_registers, *memory_,
                                      &caller_registers))
    return NULL;

  
  scoped_ptr<StackFrameARM> frame(new StackFrameARM());
  for (int i = 0; register_names[i]; i++) {
    CFIFrameInfo::RegisterValueMap<u_int32_t>::iterator entry =
      caller_registers.find(register_names[i]);
    if (entry != caller_registers.end()) {
      
      
      frame->context_validity |= StackFrameARM::RegisterValidFlag(i);
      frame->context.iregs[i] = entry->second;
    } else if (4 <= i && i <= 11 && (last_frame->context_validity &
                                     StackFrameARM::RegisterValidFlag(i))) {
      
      
      
      
      frame->context_validity |= StackFrameARM::RegisterValidFlag(i);
      frame->context.iregs[i] = last_frame->context.iregs[i];
    }
  }
  
  if (! (frame->context_validity & StackFrameARM::CONTEXT_VALID_PC)) {
    CFIFrameInfo::RegisterValueMap<u_int32_t>::iterator entry =
      caller_registers.find(".ra");
    if (entry != caller_registers.end()) {
      frame->context_validity |= StackFrameARM::CONTEXT_VALID_PC;
      frame->context.iregs[MD_CONTEXT_ARM_REG_PC] = entry->second;
    }
  }
  
  if (! (frame->context_validity & StackFrameARM::CONTEXT_VALID_SP)) {
    CFIFrameInfo::RegisterValueMap<u_int32_t>::iterator entry =
      caller_registers.find(".cfa");
    if (entry != caller_registers.end()) {
      frame->context_validity |= StackFrameARM::CONTEXT_VALID_SP;
      frame->context.iregs[MD_CONTEXT_ARM_REG_SP] = entry->second;
    }
  }

  
  static const int essentials = (StackFrameARM::CONTEXT_VALID_SP
                                 | StackFrameARM::CONTEXT_VALID_PC);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  
  if (frame->context.iregs[MD_CONTEXT_ARM_REG_PC] == 0)
    return NULL;

  
  
  
  if (frame->context.iregs[MD_CONTEXT_ARM_REG_SP]
      < last_frame->context.iregs[MD_CONTEXT_ARM_REG_SP])
    return NULL;

  
  
  
  
  
  
  
  
  frame->instruction = frame->context.iregs[MD_CONTEXT_ARM_REG_PC] - 1;

  return frame.release();
}


}  
