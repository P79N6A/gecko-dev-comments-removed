




























#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__

#include <string>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

class CodeModule;

using std::string;

struct StackFrame {
  StackFrame()
      : instruction(),
        module(NULL),
        function_name(),
        function_base(),
        source_file_name(),
        source_line(),
        source_line_base() {}
  virtual ~StackFrame() {}

  
  
  
  
  
  u_int64_t instruction;

  
  const CodeModule *module;

  
  string function_name;

  
  
  u_int64_t function_base;

  
  string source_file_name;

  
  
  int source_line;

  
  
  u_int64_t source_line_base;
};

}  

#endif  
