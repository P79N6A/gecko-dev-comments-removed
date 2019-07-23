



#ifndef BASE_BASE_PATHS_H_
#define BASE_BASE_PATHS_H_




#include "base/basictypes.h"
#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_MACOSX)
#include "base/base_paths_mac.h"
#elif defined(OS_LINUX)
#include "base/base_paths_linux.h"
#endif
#include "base/path_service.h"

namespace base {

enum {
  PATH_START = 0,

  DIR_CURRENT,  
  DIR_EXE,      
  DIR_MODULE,   
  DIR_TEMP,     
  PATH_END
};

}  

#endif  
