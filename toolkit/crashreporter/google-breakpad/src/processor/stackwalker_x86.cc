



































#include "processor/postfix_evaluator-inl.h"

#include "processor/stackwalker_x86.h"
#include "google_airbag/processor/call_stack.h"
#include "google_airbag/processor/minidump.h"
#include "google_airbag/processor/stack_frame_cpu.h"
#include "processor/linked_ptr.h"
#include "processor/stack_frame_info.h"

namespace google_airbag {


StackwalkerX86::StackwalkerX86(const MDRawContextX86 *context,
                               MemoryRegion *memory,
                               MinidumpModuleList *modules,
                               SymbolSupplier *supplier)
    : Stackwalker(memory, modules, supplier),
      context_(context) {
  if (memory_->GetBase() + memory_->GetSize() - 1 > 0xffffffff) {
    
    
    memory_ = NULL;
  }
}


StackFrame* StackwalkerX86::GetContextFrame() {
  if (!context_ || !memory_)
    return NULL;

  StackFrameX86 *frame = new StackFrameX86();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_ALL;
  frame->instruction = frame->context.eip;

  return frame;
}


StackFrame* StackwalkerX86::GetCallerFrame(
    const CallStack *stack,
    const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) {
  if (!memory_ || !stack)
    return NULL;

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
  if (last_frame_info && last_frame_info->valid == StackFrameInfo::VALID_ALL) {
    
    if (!last_frame_info->program_string.empty()) {
      
      
      
      
      program_string = last_frame_info->program_string;
    } else if (last_frame_info->allocates_base_pointer) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      program_string = "$eip .raSearchStart ^ = "
                       "$ebp $esp .cbCalleeParams + .cbSavedRegs + 8 - ^ = "
                       "$esp .raSearchStart 4 + =";
    } else {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      program_string = "$eip .raSearchStart ^ = "
                       "$esp .raSearchStart 4 + = "
                       "$ebp $ebp =";
    }
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    program_string = "$eip $ebp 4 + ^ = "
                     "$esp $ebp 8 + = "
                     "$ebp $ebp ^ =";
  }

  
  
  PostfixEvaluator<u_int32_t> evaluator =
      PostfixEvaluator<u_int32_t>(&dictionary, memory_);
  PostfixEvaluator<u_int32_t>::DictionaryValidityType dictionary_validity;
  if (!evaluator.Evaluate(program_string, &dictionary_validity) ||
      dictionary_validity.find("$eip") == dictionary_validity.end() ||
      dictionary_validity.find("$esp") == dictionary_validity.end() ||
      dictionary_validity.find("$ebp") == dictionary_validity.end()) {
    return NULL;
  }

  
  
  if (dictionary["$eip"] == 0 ||
      dictionary["$esp"] <= last_frame->context.esp) {
    return NULL;
  }

  
  
  StackFrameX86 *frame = new StackFrameX86();

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


}  
