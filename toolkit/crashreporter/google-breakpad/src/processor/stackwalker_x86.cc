



































#include "processor/postfix_evaluator-inl.h"

#include "processor/stackwalker_x86.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/linked_ptr.h"
#include "processor/logging.h"
#include "processor/stack_frame_info.h"

namespace google_breakpad {


StackwalkerX86::StackwalkerX86(const SystemInfo *system_info,
                               const MDRawContextX86 *context,
                               MemoryRegion *memory,
                               const CodeModules *modules,
                               SymbolSupplier *supplier,
                               SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context) {
  if (memory_->GetBase() + memory_->GetSize() - 1 > 0xffffffff) {
    
    
    BPLOG(ERROR) << "Memory out of range for stackwalking: " <<
                    HexString(memory_->GetBase()) << "+" <<
                    HexString(memory_->GetSize());
    memory_ = NULL;
  }
}


StackFrame* StackwalkerX86::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameX86 *frame = new StackFrameX86();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_ALL;
  frame->trust = StackFrameX86::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.eip;

  return frame;
}


StackFrame* StackwalkerX86::GetCallerFrame(
    const CallStack *stack,
    const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }
  StackFrameX86::FrameTrust trust = StackFrameX86::FRAME_TRUST_NONE;
  StackFrameX86 *last_frame = static_cast<StackFrameX86*>(
      stack->frames()->back());
  StackFrameInfo *last_frame_info = stack_frame_info.back().get();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  int frames_already_walked = stack_frame_info.size();
  u_int32_t last_frame_callee_parameter_size = 0;
  if (frames_already_walked >= 2) {
    StackFrameInfo *last_frame_callee_info =
        stack_frame_info[frames_already_walked - 2].get();
    if (last_frame_callee_info &&
        last_frame_callee_info->valid & StackFrameInfo::VALID_PARAMETER_SIZE) {
      last_frame_callee_parameter_size =
          last_frame_callee_info->parameter_size;
    }
  }

  
  
  
  
  
  
  PostfixEvaluator<u_int32_t>::DictionaryType dictionary;
  dictionary["$ebp"] = last_frame->context.ebp;
  dictionary["$esp"] = last_frame->context.esp;
  dictionary[".cbCalleeParams"] = last_frame_callee_parameter_size;

  if (last_frame_info && last_frame_info->valid == StackFrameInfo::VALID_ALL) {
    
    dictionary[".cbSavedRegs"] = last_frame_info->saved_register_size;
    dictionary[".cbLocals"] = last_frame_info->local_size;
    dictionary[".raSearchStart"] = last_frame->context.esp +
                                   last_frame_callee_parameter_size +
                                   last_frame_info->local_size +
                                   last_frame_info->saved_register_size;
  }
  if (last_frame_info &&
      last_frame_info->valid & StackFrameInfo::VALID_PARAMETER_SIZE) {
    
    
    dictionary[".cbParams"] = last_frame_info->parameter_size;
  }

  
  
  
  
  
  
  
  
  
  string program_string;
  bool traditional_frame = true;
  bool recover_ebp = true;
  if (last_frame_info && last_frame_info->valid == StackFrameInfo::VALID_ALL) {
    
    traditional_frame = false;
    trust = StackFrameX86::FRAME_TRUST_CFI;
    if (!last_frame_info->program_string.empty()) {
      
      
      
      
      
      program_string = last_frame_info->program_string;
    } else if (last_frame_info->allocates_base_pointer) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      program_string = "$eip .raSearchStart ^ = "
                       "$ebp $esp .cbCalleeParams + .cbSavedRegs + 8 - ^ = "
                       "$esp .raSearchStart 4 + =";
    } else {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      program_string = "$eip .raSearchStart ^ = "
                       "$esp .raSearchStart 4 + =";
      recover_ebp = false;
    }
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    trust = StackFrameX86::FRAME_TRUST_FP;
    program_string = "$eip $ebp 4 + ^ = "
                     "$esp $ebp 8 + = "
                     "$ebp $ebp ^ =";
  }

  
  
  PostfixEvaluator<u_int32_t> evaluator =
      PostfixEvaluator<u_int32_t>(&dictionary, memory_);
  PostfixEvaluator<u_int32_t>::DictionaryValidityType dictionary_validity;
  if (!evaluator.Evaluate(program_string, &dictionary_validity) ||
      dictionary_validity.find("$eip") == dictionary_validity.end() ||
      dictionary_validity.find("$esp") == dictionary_validity.end()) {
    
    
    
    
    
    
    u_int32_t location_start = last_frame->context.esp;
    u_int32_t location, eip;
    if (!ScanForReturnAddress(location_start, location, eip)) {
      
      
      return NULL;
    }

    
    
    
    dictionary["$eip"] = eip;
    dictionary["$esp"] = location + 4;
    trust = StackFrameX86::FRAME_TRUST_SCAN;
  }

  
  
  
  
  
  
  
  if (!traditional_frame &&
      (dictionary["$eip"] != 0 || dictionary["$ebp"] != 0)) {
    int offset = 0;

    
    
    
    
    
    
    
    
    
    
    

    u_int32_t eip = dictionary["$eip"];
    if (modules_ && !modules_->GetModuleForAddress(eip)) {
      
      
      u_int32_t location_start = dictionary[".raSearchStart"] + 4;
      u_int32_t location;
      if (ScanForReturnAddress(location_start, location, eip)) {
        
        
        
        dictionary["$eip"] = eip;
        dictionary["$esp"] = location + 4;
        offset = location - location_start;
        trust = StackFrameX86::FRAME_TRUST_CFI_SCAN;
      }
    }

    
    
    
    
    
    
    
    u_int32_t ebp = dictionary["$ebp"];
    u_int32_t value;  
    if (recover_ebp && !memory_->GetMemoryAtAddress(ebp, &value)) {
      int fp_search_bytes = last_frame_info->saved_register_size + offset;
      u_int32_t location_end = last_frame->context.esp +
                               last_frame_callee_parameter_size;

      for (u_int32_t location = location_end + fp_search_bytes;
           location >= location_end;
           location -= 4) {
        if (!memory_->GetMemoryAtAddress(location, &ebp))
          break;

        if (memory_->GetMemoryAtAddress(ebp, &value)) {
          
          
          dictionary["$ebp"] = ebp;
          break;
        }
      }
    }
  }

  
  
  if (dictionary["$eip"] == 0 ||
      dictionary["$esp"] <= last_frame->context.esp) {
    return NULL;
  }

  
  
  StackFrameX86 *frame = new StackFrameX86();

  frame->trust = trust;
  frame->context = last_frame->context;
  frame->context.eip = dictionary["$eip"];
  frame->context.esp = dictionary["$esp"];
  frame->context.ebp = dictionary["$ebp"];
  frame->context_validity = StackFrameX86::CONTEXT_VALID_EIP |
                                StackFrameX86::CONTEXT_VALID_ESP |
                                StackFrameX86::CONTEXT_VALID_EBP;

  
  
  if (dictionary_validity.find("$ebx") != dictionary_validity.end()) {
    frame->context.ebx = dictionary["$ebx"];
    frame->context_validity |= StackFrameX86::CONTEXT_VALID_EBX;
  }
  if (dictionary_validity.find("$esi") != dictionary_validity.end()) {
    frame->context.esi = dictionary["$esi"];
    frame->context_validity |= StackFrameX86::CONTEXT_VALID_ESI;
  }
  if (dictionary_validity.find("$edi") != dictionary_validity.end()) {
    frame->context.edi = dictionary["$edi"];
    frame->context_validity |= StackFrameX86::CONTEXT_VALID_EDI;
  }

  
  
  
  
  
  
  
  
  frame->instruction = frame->context.eip - 1;

  return frame;
}

bool StackwalkerX86::ScanForReturnAddress(u_int32_t location_start,
                                          u_int32_t &location_found,
                                          u_int32_t &eip_found) {
  const int kRASearchWords = 15;
  for (u_int32_t location = location_start;
       location <= location_start + kRASearchWords * 4;
       location += 4) {
    u_int32_t eip;
    if (!memory_->GetMemoryAtAddress(location, &eip))
      break;

    if (modules_ && modules_->GetModuleForAddress(eip) &&
        InstructionAddressSeemsValid(eip)) {

      eip_found = eip;
      location_found = location;
      return true;
    }
  }
  
  return false;
}

}  
