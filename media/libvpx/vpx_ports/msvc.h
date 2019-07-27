









#ifndef VPX_PORTS_MSVC_H_
#define VPX_PORTS_MSVC_H_
#ifdef _MSC_VER

#include "./vpx_config.h"

# if _MSC_VER < 1900  
#  define snprintf _snprintf
# endif  

#endif  
#endif  
