

































































































































#ifndef GOOGLE_BREAKPAD_PROCESSOR_NETWORK_SOURCE_LINE_PROTOCOL_H_
#define GOOGLE_BREAKPAD_PROCESSOR_NETWORK_SOURCE_LINE_PROTOCOL_H_

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {
namespace source_line_protocol {


const u_int8_t OK    = 1;
const u_int8_t ERROR = 0;


const u_int8_t HAS          = 1;
const u_int8_t LOAD         = 2;
const u_int8_t GET          = 3;
const u_int8_t GETSTACKWIN  = 4;
const u_int8_t GETSTACKCFI  = 5;


const u_int8_t MODULE_NOT_LOADED  = 0;
const u_int8_t MODULE_LOADED      = 1;


const u_int8_t LOAD_NOT_FOUND = 0;
const u_int8_t LOAD_INTERRUPT = 1;
const u_int8_t LOAD_FAIL      = 2;
const u_int8_t LOAD_OK        = 3;

}  
}  
#endif  
