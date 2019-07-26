



#ifndef BASE_BASE_PATHS_H_
#define BASE_BASE_PATHS_H_




#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_MACOSX)
#include "base/base_paths_mac.h"
#elif defined(OS_ANDROID)
#include "base/base_paths_android.h"
#endif

#if defined(OS_POSIX)
#include "base/base_paths_posix.h"
#endif

namespace base {

enum BasePathKey {
  PATH_START = 0,

  DIR_CURRENT,       
  DIR_EXE,           
  DIR_MODULE,        
  DIR_TEMP,          
  FILE_EXE,          
  FILE_MODULE,       
                     
                     
                     
  DIR_SOURCE_ROOT,   
                     
                     
  DIR_USER_DESKTOP,  

  DIR_TEST_DATA,     

  PATH_END
};

}  

#endif  
