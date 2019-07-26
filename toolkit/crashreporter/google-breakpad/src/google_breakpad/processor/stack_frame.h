




























#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__

#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

class CodeModule;

struct StackFrame {
  
  
  
  
  enum FrameTrust {
    FRAME_TRUST_NONE,     
    FRAME_TRUST_SCAN,     
    FRAME_TRUST_CFI_SCAN, 
    FRAME_TRUST_FP,       
    FRAME_TRUST_CFI,      
    FRAME_TRUST_CONTEXT   
  };

  StackFrame()
      : instruction(),
        module(NULL),
        function_name(),
        function_base(),
        source_file_name(),
        source_line(),
        source_line_base(),
        trust(FRAME_TRUST_NONE) {}
  virtual ~StackFrame() {}

  
  
  string trust_description() const {
    switch (trust) {
      case StackFrame::FRAME_TRUST_CONTEXT:
        return "given as instruction pointer in context";
      case StackFrame::FRAME_TRUST_CFI:
        return "call frame info";
      case StackFrame::FRAME_TRUST_CFI_SCAN:
        return "call frame info with scanning";
      case StackFrame::FRAME_TRUST_FP:
        return "previous frame's frame pointer";
      case StackFrame::FRAME_TRUST_SCAN:
        return "stack scanning";
      default:
        return "unknown";
    }
  };

  
  
  virtual uint64_t ReturnAddress() const { return instruction; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64_t instruction;

  
  const CodeModule *module;

  
  string function_name;

  
  
  uint64_t function_base;

  
  string source_file_name;

  
  
  int source_line;

  
  
  uint64_t source_line_base;

  
  
  FrameTrust trust;
};

}  

#endif  
