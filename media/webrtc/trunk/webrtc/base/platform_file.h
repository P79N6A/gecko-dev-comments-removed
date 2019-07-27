









#ifndef WEBRTC_BASE_PLATFORM_FILE_H_
#define WEBRTC_BASE_PLATFORM_FILE_H_

#include <stdio.h>

#if defined(WEBRTC_WIN)
#include <windows.h>
#endif

namespace rtc {

#if defined(WEBRTC_WIN)
typedef HANDLE PlatformFile;
#elif defined(WEBRTC_POSIX)
typedef int PlatformFile;
#else
#error Unsupported platform
#endif

extern const PlatformFile kInvalidPlatformFileValue;




FILE* FdopenPlatformFileForWriting(PlatformFile file);




bool ClosePlatformFile(PlatformFile file);

}  

#endif  
