


































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
#include "common/logging.h"
#include "processor/stackwalker_ppc.h"
#include "processor/stackwalker_sparc.h"
#include "processor/stackwalker_x86.h"
#include "processor/stackwalker_amd64.h"
#include "processor/stackwalker_arm.h"

namespace google_breakpad {

const int Stackwalker::kRASearchWords = 30;

uint32_t Stackwalker::max_frames_ = 1024;
bool Stackwalker::max_frames_set_ = false;

uint32_t Stackwalker::max_frames_scanned_ = 1024;

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


bool Stackwalker::Walk(CallStack* stack,
                       vector<const CodeModule*>* modules_without_symbols) {
  BPLOG_IF(ERROR, !stack) << "Stackwalker::Walk requires |stack|";
  assert(stack);
  stack->Clear();

  BPLOG_IF(ERROR, !modules_without_symbols) << "Stackwalker::Walk requires "
                                            << "|modules_without_symbols|";
  assert(modules_without_symbols);

  
  

  
  
  uint32_t n_scanned_frames = 0;

  
  scoped_ptr<StackFrame> frame(GetContextFrame());

  while (frame.get()) {
    
    
    

    
    StackFrameSymbolizer::SymbolizerResult symbolizer_result =
        frame_symbolizer_->FillSourceLineInfo(modules_, system_info_,
                                             frame.get());
    if (symbolizer_result == StackFrameSymbolizer::kInterrupt) {
      BPLOG(INFO) << "Stack walk is interrupted.";
      return false;
    }

    
    if (symbolizer_result == StackFrameSymbolizer::kError &&
        frame->module != NULL) {
      bool found = false;
      vector<const CodeModule*>::iterator iter;
      for (iter = modules_without_symbols->begin();
           iter != modules_without_symbols->end();
           ++iter) {
        if (*iter == frame->module) {
          found = true;
          break;
        }
      }
      if (!found) {
        BPLOG(INFO) << "Couldn't load symbols for: "
                    << frame->module->debug_file() << "|"
                    << frame->module->debug_identifier();
        modules_without_symbols->push_back(frame->module);
      }
    }

    
    switch (frame.get()->trust) {
       case StackFrame::FRAME_TRUST_NONE:
       case StackFrame::FRAME_TRUST_SCAN:
       case StackFrame::FRAME_TRUST_CFI_SCAN:
         n_scanned_frames++;
         break;
      default:
        break;
    }

    
    
    stack->frames_.push_back(frame.release());
    if (stack->frames_.size() > max_frames_) {
      
      
      if (!max_frames_set_)
        BPLOG(ERROR) << "The stack is over " << max_frames_ << " frames.";
      break;
    }

    
    bool stack_scan_allowed = n_scanned_frames < max_frames_scanned_;
    frame.reset(GetCallerFrame(stack, stack_scan_allowed));
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

  uint32_t cpu = context->GetContextCPU();
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

bool Stackwalker::InstructionAddressSeemsValid(uint64_t address) {
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

  if (symbolizer_result != StackFrameSymbolizer::kNoError) {
    
    
    return true;
  }

  return !frame.function_name.empty();
}

}  
