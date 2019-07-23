




























#ifndef GOOGLE_AIRBAG_PROCESSOR_STACK_FRAME_H__
#define GOOGLE_AIRBAG_PROCESSOR_STACK_FRAME_H__

#include <string>
#include "google_airbag/common/airbag_types.h"

namespace google_airbag {

using std::string;

struct StackFrame {
  StackFrame()
      : instruction(),
        module_base(),
        module_name(),
        function_base(),
        function_name(),
        source_file_name(),
        source_line(),
        source_line_base() {}
  virtual ~StackFrame() {}

  
  
  
  
  
  u_int64_t instruction;

  
  u_int64_t module_base;

  
  string module_name;

  
  
  u_int64_t function_base;

  
  string function_name;

  
  string source_file_name;

  
  
  int source_line;

  
  
  u_int64_t source_line_base;
};

}  

#endif  
