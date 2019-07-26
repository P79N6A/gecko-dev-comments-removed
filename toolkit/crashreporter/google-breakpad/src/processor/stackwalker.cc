


































#include "google_breakpad/processor/stackwalker.h"

#include <assert.h>

#include "common/scoped_ptr.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/stack_frame_symbolizer.h"
#include "google_breakpad/processor/system_info.h"
#include "processor/linked_ptr.h"
#include "processor/logging.h"
#include "processor/stackwalker_ppc.h"
#include "processor/stackwalker_sparc.h"
#include "processor/stackwalker_x86.h"
#include "processor/stackwalker_amd64.h"
#include "processor/stackwalker_arm.h"

namespace google_breakpad {

const int Stackwalker::kRASearchWords = 30;
u_int32_t Stackwalker::max_frames_ = 1024;

Stackwalker::Stackwalker(const SystemInfo* system_info,
                         MemoryRegion* memory,
                         const CodeModules* modules,
                         StackFrameSymbolizer* frame_symbolizer)
    : system_info_(system_info),
      memory_(memory),
      modules_(modules),
      frame_symbolizer_(frame_symbolizer) {
  assert(frame_symbolizer_);
}


bool Stackwalker::Walk(CallStack* stack) {
  BPLOG_IF(ERROR, !stack) << "Stackwalker::Walk requires |stack|";
  assert(stack);
  stack->Clear();

  
  

  
  scoped_ptr<StackFrame> frame(GetContextFrame());

  while (frame.get()) {
    
    
    

    
    StackFrameSymbolizer::SymbolizerResult symbolizer_result =
        frame_symbolizer_->FillSourceLineInfo(modules_, system_info_,
                                             frame.get());
    if (symbolizer_result == StackFrameSymbolizer::INTERRUPT) {
      BPLOG(INFO) << "Stack walk is interrupted.";
      return false;
    }

    
    
    stack->frames_.push_back(frame.release());
    if (stack->frames_.size() > max_frames_) {
      BPLOG(ERROR) << "The stack is over " << max_frames_ << " frames.";
      break;
    }

    
    frame.reset(GetCallerFrame(stack));
  }

  return true;
}



Stackwalker* Stackwalker::StackwalkerForCPU(
    const SystemInfo* system_info,
    MinidumpContext* context,
    MemoryRegion* memory,
    const CodeModules* modules,
    StackFrameSymbolizer* frame_symbolizer) {
  if (!context) {
    BPLOG(ERROR) << "Can't choose a stackwalker implementation without context";
    return NULL;
  }

  Stackwalker* cpu_stackwalker = NULL;

  u_int32_t cpu = context->GetContextCPU();
  switch (cpu) {
    case MD_CONTEXT_X86:
      cpu_stackwalker = new StackwalkerX86(system_info,
                                           context->GetContextX86(),
                                           memory, modules, frame_symbolizer);
      break;

    case MD_CONTEXT_PPC:
      cpu_stackwalker = new StackwalkerPPC(system_info,
                                           context->GetContextPPC(),
                                           memory, modules, frame_symbolizer);
      break;

    case MD_CONTEXT_AMD64:
      cpu_stackwalker = new StackwalkerAMD64(system_info,
                                             context->GetContextAMD64(),
                                             memory, modules, frame_symbolizer);
      break;

    case MD_CONTEXT_SPARC:
      cpu_stackwalker = new StackwalkerSPARC(system_info,
                                             context->GetContextSPARC(),
                                             memory, modules, frame_symbolizer);
      break;

    case MD_CONTEXT_ARM:
      int fp_register = -1;
      if (system_info->os_short == "ios")
        fp_register = MD_CONTEXT_ARM_REG_IOS_FP;
      cpu_stackwalker = new StackwalkerARM(system_info,
                                           context->GetContextARM(),
                                           fp_register, memory, modules,
                                           frame_symbolizer);
      break;
  }

  BPLOG_IF(ERROR, !cpu_stackwalker) << "Unknown CPU type " << HexString(cpu) <<
                                       ", can't choose a stackwalker "
                                       "implementation";
  return cpu_stackwalker;
}

bool Stackwalker::InstructionAddressSeemsValid(u_int64_t address) {
  StackFrame frame;
  frame.instruction = address;
  StackFrameSymbolizer::SymbolizerResult symbolizer_result =
      frame_symbolizer_->FillSourceLineInfo(modules_, system_info_, &frame);

  if (!frame.module) {
    
    return false;
  }

  if (!frame_symbolizer_->HasImplementation()) {
    
    
    return true;
  }

  if (symbolizer_result != StackFrameSymbolizer::NO_ERROR) {
    
    
    return true;
  }

  return !frame.function_name.empty();
}

}  
