



#ifndef BASE_DIR_READER_POSIX_H_
#define BASE_DIR_READER_POSIX_H_
#pragma once

#include "build/build_config.h"











#if defined(OS_LINUX)
#include "base/dir_reader_linux.h"
#elif defined(OS_BSD)
#include "base/dir_reader_bsd.h"
#else
#include "base/dir_reader_fallback.h"
#endif

namespace base {

#if defined(OS_LINUX)
typedef DirReaderLinux DirReaderPosix;
#elif defined(OS_BSD)
typedef DirReaderBSD DirReaderPosix;
#else
typedef DirReaderFallback DirReaderPosix;
#endif

}  

#endif 
