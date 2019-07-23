




































#ifndef PROCESSOR_STACK_FRAME_INFO_H__
#define PROCESSOR_STACK_FRAME_INFO_H__

#include <string>

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

struct StackFrameInfo {
 public:
  enum Validity {
    VALID_NONE           = 0,
    VALID_PARAMETER_SIZE = 1,
    VALID_ALL            = -1
  };

  StackFrameInfo() : valid(VALID_NONE),
                     prolog_size(0),
                     epilog_size(0),
                     parameter_size(0),
                     saved_register_size(0),
                     local_size(0),
                     max_stack_size(0),
                     allocates_base_pointer(0),
                     program_string() {}

  StackFrameInfo(u_int32_t set_prolog_size,
                 u_int32_t set_epilog_size,
                 u_int32_t set_parameter_size,
                 u_int32_t set_saved_register_size,
                 u_int32_t set_local_size,
                 u_int32_t set_max_stack_size,
                 int set_allocates_base_pointer,
                 const std::string set_program_string)
      : valid(VALID_ALL),
        prolog_size(set_prolog_size),
        epilog_size(set_epilog_size),
        parameter_size(set_parameter_size),
        saved_register_size(set_saved_register_size),
        local_size(set_local_size),
        max_stack_size(set_max_stack_size),
        allocates_base_pointer(set_allocates_base_pointer),
        program_string(set_program_string) {}

  
  void CopyFrom(const StackFrameInfo &that) {
    valid = that.valid;
    prolog_size = that.prolog_size;
    epilog_size = that.epilog_size;
    parameter_size = that.parameter_size;
    saved_register_size = that.saved_register_size;
    local_size = that.local_size;
    max_stack_size = that.max_stack_size;
    allocates_base_pointer = that.allocates_base_pointer;
    program_string = that.program_string;
  }

  
  
  void Clear() {
    valid = VALID_NONE;
    program_string.erase();
  }

  
  
  
  
  int valid;

  
  u_int32_t prolog_size;
  u_int32_t epilog_size;
  u_int32_t parameter_size;
  u_int32_t saved_register_size;
  u_int32_t local_size;
  u_int32_t max_stack_size;

  
  
  bool allocates_base_pointer;
  std::string program_string;
};

}  


#endif  
