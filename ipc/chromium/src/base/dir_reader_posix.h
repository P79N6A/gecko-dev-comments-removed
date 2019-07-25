



#ifndef BASE_DIR_READER_POSIX_H_
#define BASE_DIR_READER_POSIX_H_
#pragma once

#include "build/build_config.h"











#if defined(OS_LINUX) && !defined(OS_OPENBSD)
#include "base/dir_reader_linux.h"
#else
#include "base/dir_reader_fallback.h"
#endif

namespace base {

#if defined(OS_LINUX) && !defined(OS_OPENBSD)
typedef DirReaderLinux DirReaderPosix;
#else
typedef DirReaderFallback DirReaderPosix;
#endif

}  

#endif 
