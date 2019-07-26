


































#include <vector>

#include "common/scoped_ptr.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/cfi_frame_info.h"
#include "common/logging.h"
#include "processor/stackwalker_arm.h"

namespace google_breakpad {


StackwalkerARM::StackwalkerARM(const SystemInfo* system_info,
                               const MDRawContextARM* context,
                               int fp_register,
                               MemoryRegion* memory,
                               const CodeModules* modules,
                               StackFrameSymbolizer* resolver_helper)
    : Stackwalker(system_info, memory, modules, resolver_helper),
      context_(context), fp_register_(fp_register),
      context_frame_validity_(StackFrameARM::CONTEXT_VALID_ALL) { }


StackFrame* StackwalkerARM::GetContextFrame() {
  if (!context_) {
    BPLOG(ERROR) << "Can't get context frame without context";
    return NULL;
  }

  StackFrameARM* frame = new StackFrameARM();

  
  
  frame->context = *context_;
  frame->context_validity = context_frame_validity_;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.iregs[MD_CONTEXT_ARM_REG_PC];

  return frame;
}

StackFrameARM* StackwalkerARM::GetCallerByCFIFrameInfo(
    const vector<StackFrame*> &frames,
    CFIFrameInfo* cfi_frame_info) {
  StackFrameARM* last_frame = static_cast<StackFrameARM*>(frames.back());

  static const UniqueString *register_names[] = {
    ToUniqueString("r0"),  ToUniqueString("r1"),
    ToUniqueString("r2"),  ToUniqueString("r3"),
    ToUniqueString("r4"),  ToUniqueString("r5"),
    ToUniqueString("r6"),  ToUniqueString("r7"),
    ToUniqueString("r8"),  ToUniqueString("r9"),
    ToUniqueString("r10"), ToUniqueString("r11"),
    ToUniqueString("r12"), ToUniqueString("sp"),
    ToUniqueString("lr"),  ToUniqueString("pc"),
    ToUniqueString("f0"),  ToUniqueString("f1"),
    ToUniqueString("f2"),  ToUniqueString("f3"),
    ToUniqueString("f4"),  ToUniqueString("f5"),
    ToUniqueString("f6"),  ToUniqueString("f7"),
    ToUniqueString("fps"), ToUniqueString("cpsr"),
    NULL
  };

  
  CFIFrameInfo::RegisterValueMap<uint32_t> callee_registers;
  for (int i = 0; register_names[i]; i++)
    if (last_frame->context_validity & StackFrameARM::RegisterValidFlag(i))
      callee_registers.set(register_names[i], last_frame->context.iregs[i]);

  
  CFIFrameInfo::RegisterValueMap<uint32_t> caller_registers;
  if (!cfi_frame_info->FindCallerRegs(callee_registers, *memory_,
                                      &caller_registers))
    return NULL;

  
  scoped_ptr<StackFrameARM> frame(new StackFrameARM());
  for (int i = 0; register_names[i]; i++) {
    bool found = false;
    uint32_t v = caller_registers.get(&found, register_names[i]);
    if (found) {
      
      
      frame->context_validity |= StackFrameARM::RegisterValidFlag(i);
      frame->context.iregs[i] = v;
    } else if (4 <= i && i <= 11 && (last_frame->context_validity &
                                     StackFrameARM::RegisterValidFlag(i))) {
      
      
      
      
      frame->context_validity |= StackFrameARM::RegisterValidFlag(i);
      frame->context.iregs[i] = last_frame->context.iregs[i];
    }
  }
  
  if (!(frame->context_validity & StackFrameARM::CONTEXT_VALID_PC)) {
    bool found = false;
    uint32_t v = caller_registers.get(&found, ustr__ZDra());
    if (found) {
      if (fp_register_ == -1) {
        frame->context_validity |= StackFrameARM::CONTEXT_VALID_PC;
        frame->context.iregs[MD_CONTEXT_ARM_REG_PC] = v;
      } else {
        
        
        frame->context_validity |= StackFrameARM::CONTEXT_VALID_PC;
        frame->context_validity |= StackFrameARM::CONTEXT_VALID_LR;
        frame->context.iregs[MD_CONTEXT_ARM_REG_LR] = v;
        frame->context.iregs[MD_CONTEXT_ARM_REG_PC] =
            last_frame->context.iregs[MD_CONTEXT_ARM_REG_LR];
      }
    }
  }
  
  if (!(frame->context_validity & StackFrameARM::CONTEXT_VALID_SP)) {
    bool found = false;
    uint32_t v = caller_registers.get(&found, ustr__ZDcfa());
    if (found) {
      frame->context_validity |= StackFrameARM::CONTEXT_VALID_SP;
      frame->context.iregs[MD_CONTEXT_ARM_REG_SP] = v;
    }
  }

  
  static const int essentials = (StackFrameARM::CONTEXT_VALID_SP
                                 | StackFrameARM::CONTEXT_VALID_PC);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  frame->trust = StackFrame::FRAME_TRUST_CFI;
  return frame.release();
}

StackFrameARM* StackwalkerARM::GetCallerByStackScan(
    const vector<StackFrame*> &frames) {
  StackFrameARM* last_frame = static_cast<StackFrameARM*>(frames.back());
  uint32_t last_sp = last_frame->context.iregs[MD_CONTEXT_ARM_REG_SP];
  uint32_t caller_sp, caller_pc;

  
  
  const int kRASearchWords = frames.size() == 1 ?
    Stackwalker::kRASearchWords * 4 :
    Stackwalker::kRASearchWords;

  if (!ScanForReturnAddress(last_sp, &caller_sp, &caller_pc,
                            kRASearchWords)) {
    
    return NULL;
  }

  
  
  
  caller_sp += 4;

  
  
  StackFrameARM* frame = new StackFrameARM();

  frame->trust = StackFrame::FRAME_TRUST_SCAN;
  frame->context = last_frame->context;
  frame->context.iregs[MD_CONTEXT_ARM_REG_PC] = caller_pc;
  frame->context.iregs[MD_CONTEXT_ARM_REG_SP] = caller_sp;
  frame->context_validity = StackFrameARM::CONTEXT_VALID_PC |
                            StackFrameARM::CONTEXT_VALID_SP;

  return frame;
}

StackFrameARM* StackwalkerARM::GetCallerByFramePointer(
    const vector<StackFrame*> &frames) {
  StackFrameARM* last_frame = static_cast<StackFrameARM*>(frames.back());

  if (!(last_frame->context_validity &
        StackFrameARM::RegisterValidFlag(fp_register_))) {
    return NULL;
  }

  uint32_t last_fp = last_frame->context.iregs[fp_register_];

  uint32_t caller_fp = 0;
  if (last_fp && !memory_->GetMemoryAtAddress(last_fp, &caller_fp)) {
    BPLOG(ERROR) << "Unable to read caller_fp from last_fp: 0x"
                 << std::hex << last_fp;
    return NULL;
  }

  uint32_t caller_lr = 0;
  if (last_fp && !memory_->GetMemoryAtAddress(last_fp + 4, &caller_lr)) {
    BPLOG(ERROR) << "Unable to read caller_lr from last_fp + 4: 0x"
                 << std::hex << (last_fp + 4);
    return NULL;
  }

  uint32_t caller_sp = last_fp ? last_fp + 8 :
      last_frame->context.iregs[MD_CONTEXT_ARM_REG_SP];

  
  
  StackFrameARM* frame = new StackFrameARM();

  frame->trust = StackFrame::FRAME_TRUST_FP;
  frame->context = last_frame->context;
  frame->context.iregs[fp_register_] = caller_fp;
  frame->context.iregs[MD_CONTEXT_ARM_REG_SP] = caller_sp;
  frame->context.iregs[MD_CONTEXT_ARM_REG_PC] =
      last_frame->context.iregs[MD_CONTEXT_ARM_REG_LR];
  frame->context.iregs[MD_CONTEXT_ARM_REG_LR] = caller_lr;
  frame->context_validity = StackFrameARM::CONTEXT_VALID_PC |
                            StackFrameARM::CONTEXT_VALID_LR |
                            StackFrameARM::RegisterValidFlag(fp_register_) |
                            StackFrameARM::CONTEXT_VALID_SP;
  return frame;
}

StackFrame* StackwalkerARM::GetCallerFrame(const CallStack* stack,
                                           bool stack_scan_allowed) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  const vector<StackFrame*> &frames = *stack->frames();
  StackFrameARM* last_frame = static_cast<StackFrameARM*>(frames.back());
  scoped_ptr<StackFrameARM> frame;

  
  scoped_ptr<CFIFrameInfo> cfi_frame_info(
      frame_symbolizer_->FindCFIFrameInfo(last_frame));
  if (cfi_frame_info.get())
    frame.reset(GetCallerByCFIFrameInfo(frames, cfi_frame_info.get()));

  
  
  if (fp_register_ >= 0 && !frame.get())
    frame.reset(GetCallerByFramePointer(frames));

  
  if (stack_scan_allowed && !frame.get())
    frame.reset(GetCallerByStackScan(frames));

  
  if (!frame.get())
    return NULL;


  
  if (frame->context.iregs[MD_CONTEXT_ARM_REG_PC] == 0)
    return NULL;

  
  
  
  if (frame->context.iregs[MD_CONTEXT_ARM_REG_SP]
      < last_frame->context.iregs[MD_CONTEXT_ARM_REG_SP])
    return NULL;

  
  
  
  
  
  
  
  
  frame->instruction = frame->context.iregs[MD_CONTEXT_ARM_REG_PC] - 2;

  return frame.release();
}


}  
