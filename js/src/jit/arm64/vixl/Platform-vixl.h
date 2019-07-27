

























#ifndef VIXL_PLATFORM_H
#define VIXL_PLATFORM_H



#include <signal.h>

#include "js-config.h"

namespace vixl {

inline void HostBreakpoint(int64_t code = 0) { raise(SIGINT); }

} 

#endif 
