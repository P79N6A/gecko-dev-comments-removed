




































#ifndef PROCESSOR_WINDOWS_FRAME_INFO_H__
#define PROCESSOR_WINDOWS_FRAME_INFO_H__

#include <string.h>

#include <string>
#include <vector>

#include "google_breakpad/common/breakpad_types.h"
#include "processor/logging.h"
#include "processor/tokenize.h"

namespace google_breakpad {

struct WindowsFrameInfo {
 public:
  enum Validity {
    VALID_NONE           = 0,
    VALID_PARAMETER_SIZE = 1,
    VALID_ALL            = -1
  };

  
  
  
  
  enum StackInfoTypes {
    STACK_INFO_FPO = 0,
    STACK_INFO_TRAP,  
    STACK_INFO_TSS,   
    STACK_INFO_STANDARD,
    STACK_INFO_FRAME_DATA,
    STACK_INFO_LAST,  
    STACK_INFO_UNKNOWN = -1
  };

  WindowsFrameInfo() : valid(VALID_NONE),
                     prolog_size(0),
                     epilog_size(0),
                     parameter_size(0),
                     saved_register_size(0),
                     local_size(0),
                     max_stack_size(0),
                     allocates_base_pointer(0),
                     program_string() {}

  WindowsFrameInfo(u_int32_t set_prolog_size,
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

  
  
  
  
  static WindowsFrameInfo *ParseFromString(const std::string string,
                                           int &type,
                                           u_int64_t &rva,
                                           u_int64_t &code_size) {
    
    
    

    std::vector<char>  buffer;
    StringToVector(string, buffer);
    std::vector<char*> tokens;
    if (!Tokenize(&buffer[0], " \r\n", 11, &tokens))
      return NULL;

    type = strtol(tokens[0], NULL, 16);
    if (type < 0 || type > STACK_INFO_LAST - 1)
      return NULL;

    rva                           = strtoull(tokens[1],  NULL, 16);
    code_size                     = strtoull(tokens[2],  NULL, 16);
    u_int32_t prolog_size         =  strtoul(tokens[3],  NULL, 16);
    u_int32_t epilog_size         =  strtoul(tokens[4],  NULL, 16);
    u_int32_t parameter_size      =  strtoul(tokens[5],  NULL, 16);
    u_int32_t saved_register_size =  strtoul(tokens[6],  NULL, 16);
    u_int32_t local_size          =  strtoul(tokens[7],  NULL, 16);
    u_int32_t max_stack_size      =  strtoul(tokens[8],  NULL, 16);
    int has_program_string        =  strtoul(tokens[9], NULL, 16);

    const char *program_string = "";
    int allocates_base_pointer = 0;
    if (has_program_string) {
      program_string = tokens[10];
    } else {
      allocates_base_pointer = strtoul(tokens[10], NULL, 16);
    }

    return new WindowsFrameInfo(prolog_size,
                                epilog_size,
                                parameter_size,
                                saved_register_size,
                                local_size,
                                max_stack_size,
                                allocates_base_pointer,
                                program_string);
  }

  
  void CopyFrom(const WindowsFrameInfo &that) {
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
