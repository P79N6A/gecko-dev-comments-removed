



































#include "processor/postfix_evaluator-inl.h"

#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/logging.h"
#include "processor/scoped_ptr.h"
#include "processor/stackwalker_x86.h"
#include "processor/windows_frame_info.h"
#include "processor/cfi_frame_info.h"

namespace google_breakpad {


const StackwalkerX86::CFIWalker::RegisterSet
StackwalkerX86::cfi_register_map_[] = {
  
  
  
  
  
  { "$eip", ".ra",  false,
    StackFrameX86::CONTEXT_VALID_EIP, &MDRawContextX86::eip },
  { "$esp", ".cfa", false,
    StackFrameX86::CONTEXT_VALID_ESP, &MDRawContextX86::esp },
  { "$ebp", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EBP, &MDRawContextX86::ebp },
  { "$eax", NULL,   false,
    StackFrameX86::CONTEXT_VALID_EAX, &MDRawContextX86::eax },
  { "$ebx", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EBX, &MDRawContextX86::ebx },
  { "$ecx", NULL,   false,
    StackFrameX86::CONTEXT_VALID_ECX, &MDRawContextX86::ecx },
  { "$edx", NULL,   false,
    StackFrameX86::CONTEXT_VALID_EDX, &MDRawContextX86::edx },
  { "$esi", NULL,   true,
    StackFrameX86::CONTEXT_VALID_ESI, &MDRawContextX86::esi },
  { "$edi", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EDI, &MDRawContextX86::edi },
};

StackwalkerX86::StackwalkerX86(const SystemInfo *system_info,
                               const MDRawContextX86 *context,
                               MemoryRegion *memory,
                               const CodeModules *modules,
                               SymbolSupplier *supplier,
                               SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context),
      cfi_walker_(cfi_register_map_,
                  (sizeof(cfi_register_map_) / sizeof(cfi_register_map_[0]))) {
  if (memory_->GetBase() + memory_->GetSize() - 1 > 0xffffffff) {
    
    
    BPLOG(ERROR) << "Memory out of range for stackwalking: " <<
                    HexString(memory_->GetBase()) << "+" <<
                    HexString(memory_->GetSize());
    memory_ = NULL;
  }
}

StackFrameX86::~StackFrameX86() {
  if (windows_frame_info)
    delete windows_frame_info;
  windows_frame_info = NULL;
  if (cfi_frame_info)
    delete cfi_frame_info;
  cfi_frame_info = NULL;
}

StackFrame *StackwalkerX86::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameX86 *frame = new StackFrameX86();

  
  
  frame->context = *context_;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_ALL;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.eip;

  return frame;
}

StackFrameX86 *StackwalkerX86::GetCallerByWindowsFrameInfo(
    const vector<StackFrame *> &frames,
    WindowsFrameInfo *last_frame_info) {
  StackFrame::FrameTrust trust = StackFrame::FRAME_TRUST_NONE;

  StackFrameX86 *last_frame = static_cast<StackFrameX86 *>(frames.back());

  
  
  last_frame->windows_frame_info = last_frame_info;

  
  
  
  if (last_frame_info->valid != WindowsFrameInfo::VALID_ALL)
    return NULL;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  u_int32_t last_frame_callee_parameter_size = 0;
  int frames_already_walked = frames.size();
  if (frames_already_walked >= 2) {
    const StackFrameX86 *last_frame_callee
        = static_cast<StackFrameX86 *>(frames[frames_already_walked - 2]);
    WindowsFrameInfo *last_frame_callee_info
        = last_frame_callee->windows_frame_info;
    if (last_frame_callee_info &&
        (last_frame_callee_info->valid
         & WindowsFrameInfo::VALID_PARAMETER_SIZE)) {
      last_frame_callee_parameter_size =
          last_frame_callee_info->parameter_size;
    }
  }

  
  
  
  PostfixEvaluator<u_int32_t>::DictionaryType dictionary;
  
  dictionary["$ebp"] = last_frame->context.ebp;
  dictionary["$esp"] = last_frame->context.esp;
  
  
  
  
  
  dictionary[".cbCalleeParams"] = last_frame_callee_parameter_size;
  dictionary[".cbSavedRegs"] = last_frame_info->saved_register_size;
  dictionary[".cbLocals"] = last_frame_info->local_size;

  u_int32_t raSearchStart = last_frame->context.esp +
                            last_frame_callee_parameter_size +
                            last_frame_info->local_size +
                            last_frame_info->saved_register_size;

  u_int32_t raSearchStartOld = raSearchStart;
  u_int32_t found = 0; 
  
  
  if (ScanForReturnAddress(raSearchStart, &raSearchStart, &found, 3) &&
      last_frame->trust == StackFrame::FRAME_TRUST_CONTEXT &&
      last_frame->windows_frame_info != NULL &&
      last_frame_info->type_ == WindowsFrameInfo::STACK_INFO_FPO &&
      raSearchStartOld == raSearchStart &&
      found == last_frame->context.eip) {
    
    
    
    
    
    
    raSearchStart += 4;
    ScanForReturnAddress(raSearchStart, &raSearchStart, &found, 3);
  }

  
  
  dictionary[".raSearchStart"] = raSearchStart;
  dictionary[".raSearch"] = raSearchStart;

  dictionary[".cbParams"] = last_frame_info->parameter_size;

  
  
  
  
  
  
  
  string program_string;
  bool recover_ebp = true;

  trust = StackFrame::FRAME_TRUST_CFI;
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

  
  
  PostfixEvaluator<u_int32_t> evaluator =
      PostfixEvaluator<u_int32_t>(&dictionary, memory_);
  PostfixEvaluator<u_int32_t>::DictionaryValidityType dictionary_validity;
  if (!evaluator.Evaluate(program_string, &dictionary_validity) ||
      dictionary_validity.find("$eip") == dictionary_validity.end() ||
      dictionary_validity.find("$esp") == dictionary_validity.end()) {
    
    
    
    
    
    
    u_int32_t location_start = last_frame->context.esp;
    u_int32_t location, eip;
    if (!ScanForReturnAddress(location_start, &location, &eip)) {
      
      
      return NULL;
    }

    
    
    
    dictionary["$eip"] = eip;
    dictionary["$esp"] = location + 4;
    trust = StackFrame::FRAME_TRUST_SCAN;
  }

  
  
  
  
  
  
  
  if (dictionary["$eip"] != 0 || dictionary["$ebp"] != 0) {
    int offset = 0;

    
    
    
    
    
    
    
    
    
    
    

    u_int32_t eip = dictionary["$eip"];
    if (modules_ && !modules_->GetModuleForAddress(eip)) {
      
      
      u_int32_t location_start = dictionary[".raSearchStart"] + 4;
      u_int32_t location;
      if (ScanForReturnAddress(location_start, &location, &eip)) {
        
        
        
        dictionary["$eip"] = eip;
        dictionary["$esp"] = location + 4;
        offset = location - location_start;
        trust = StackFrame::FRAME_TRUST_CFI_SCAN;
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

  return frame;
}

StackFrameX86 *StackwalkerX86::GetCallerByCFIFrameInfo(
    const vector<StackFrame*> &frames,
    CFIFrameInfo *cfi_frame_info) {
  StackFrameX86 *last_frame = static_cast<StackFrameX86*>(frames.back());
  last_frame->cfi_frame_info = cfi_frame_info;

  scoped_ptr<StackFrameX86> frame(new StackFrameX86());
  if (!cfi_walker_
      .FindCallerRegisters(*memory_, *cfi_frame_info,
                           last_frame->context, last_frame->context_validity,
                           &frame->context, &frame->context_validity))
    return NULL;
  
  
  static const int essentials = (StackFrameX86::CONTEXT_VALID_EIP
                                 | StackFrameX86::CONTEXT_VALID_ESP
                                 | StackFrameX86::CONTEXT_VALID_EBP);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  frame->trust = StackFrame::FRAME_TRUST_CFI;

  return frame.release();
}

StackFrameX86 *StackwalkerX86::GetCallerByEBPAtBase(
    const vector<StackFrame *> &frames) {
  StackFrame::FrameTrust trust;
  StackFrameX86 *last_frame = static_cast<StackFrameX86 *>(frames.back());
  u_int32_t last_esp = last_frame->context.esp;
  u_int32_t last_ebp = last_frame->context.ebp;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  u_int32_t caller_eip, caller_esp, caller_ebp;

  if (memory_->GetMemoryAtAddress(last_ebp + 4, &caller_eip) &&
      memory_->GetMemoryAtAddress(last_ebp, &caller_ebp)) {
    caller_esp = last_ebp + 8;
    trust = StackFrame::FRAME_TRUST_FP;
  } else {
    
    
    
    
    
    if (!ScanForReturnAddress(last_esp, &caller_esp, &caller_eip)) {
      
      
      return NULL;
    }

    
    
    
    caller_esp += 4;
    caller_ebp = last_ebp;

    trust = StackFrame::FRAME_TRUST_SCAN;
  }

  
  
  StackFrameX86 *frame = new StackFrameX86();

  frame->trust = trust;
  frame->context = last_frame->context;
  frame->context.eip = caller_eip;
  frame->context.esp = caller_esp;
  frame->context.ebp = caller_ebp;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_EIP |
                            StackFrameX86::CONTEXT_VALID_ESP |
                            StackFrameX86::CONTEXT_VALID_EBP;

  return frame;
}

StackFrame *StackwalkerX86::GetCallerFrame(const CallStack *stack) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  const vector<StackFrame *> &frames = *stack->frames();
  StackFrameX86 *last_frame = static_cast<StackFrameX86 *>(frames.back());
  scoped_ptr<StackFrameX86> new_frame;

  
  WindowsFrameInfo *windows_frame_info
      = resolver_ ? resolver_->FindWindowsFrameInfo(last_frame) : NULL;
  if (windows_frame_info)
    new_frame.reset(GetCallerByWindowsFrameInfo(frames, windows_frame_info));

  
  if (!new_frame.get()) {
    CFIFrameInfo *cfi_frame_info = 
        resolver_ ? resolver_->FindCFIFrameInfo(last_frame) : NULL;
    if (cfi_frame_info)
      new_frame.reset(GetCallerByCFIFrameInfo(frames, cfi_frame_info));
  }

  
  if (!new_frame.get())
    new_frame.reset(GetCallerByEBPAtBase(frames));

  
  if (!new_frame.get())
    return NULL;
  
  
  if (new_frame->context.eip == 0)
    return NULL;

  
  
  
  if (new_frame->context.esp <= last_frame->context.esp)
    return NULL;

  
  
  
  
  
  
  
  
  new_frame->instruction = new_frame->context.eip - 1;

  return new_frame.release();
}

}  
