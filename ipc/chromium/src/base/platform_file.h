



#ifndef BASE_PLATFORM_FILE_H_
#define BASE_PLATFORM_FILE_H_

#include "build/build_config.h"
#if defined(OS_WIN)
#include <windows.h>
#endif

#include <string>

namespace base {

#if defined(OS_WIN)
typedef HANDLE PlatformFile;
const PlatformFile kInvalidPlatformFileValue = INVALID_HANDLE_VALUE;
#elif defined(OS_POSIX)
typedef int PlatformFile;
const PlatformFile kInvalidPlatformFileValue = -1;
#endif

enum PlatformFileFlags {
  PLATFORM_FILE_OPEN = 1,
  PLATFORM_FILE_CREATE = 2,
  PLATFORM_FILE_OPEN_ALWAYS = 4,    
  PLATFORM_FILE_CREATE_ALWAYS = 8,  
  PLATFORM_FILE_READ = 16,
  PLATFORM_FILE_WRITE = 32,
  PLATFORM_FILE_EXCLUSIVE_READ = 64,  
  PLATFORM_FILE_EXCLUSIVE_WRITE = 128,
  PLATFORM_FILE_ASYNC = 256
};




PlatformFile CreatePlatformFile(const std::wstring& name,
                                int flags,
                                bool* created);

}  

#endif  
