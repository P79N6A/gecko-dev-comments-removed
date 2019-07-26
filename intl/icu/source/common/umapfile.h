





















#ifndef __UMAPFILE_H__
#define __UMAPFILE_H__

#include "unicode/putil.h"
#include "unicode/udata.h"
#include "putilimp.h"

U_CFUNC UBool uprv_mapFile(UDataMemory *pdm, const char *path);
U_CFUNC void  uprv_unmapFile(UDataMemory *pData);


#define MAP_NONE        0
#define MAP_WIN32       1
#define MAP_POSIX       2
#define MAP_STDIO       3
#define MAP_390DLL      4

#if UCONFIG_NO_FILE_IO
#   define MAP_IMPLEMENTATION MAP_NONE
#elif U_PLATFORM_USES_ONLY_WIN32_API
#   define MAP_IMPLEMENTATION MAP_WIN32
#elif U_HAVE_MMAP || U_PLATFORM == U_PF_OS390
#   if U_PLATFORM == U_PF_OS390 && defined (OS390_STUBDATA)
        
#       define MAP_IMPLEMENTATION MAP_390DLL
#   else
#       define MAP_IMPLEMENTATION MAP_POSIX
#   endif
#else 
#   define MAP_IMPLEMENTATION MAP_STDIO
#endif

#endif
