




























#include "PageBlock.h"
#include "wtf/Assertions.h"

#if WTF_OS_UNIX && !WTF_OS_SYMBIAN
#include <unistd.h>
#endif

#if WTF_OS_WINDOWS
#include <malloc.h>
#include <windows.h>
#endif

#if WTF_OS_SYMBIAN
#include <e32hal.h>
#include <e32std.h>
#endif

namespace WTF {

static size_t s_pageSize;

#if WTF_OS_UNIX && !WTF_OS_SYMBIAN

inline size_t systemPageSize()
{
    return getpagesize();
}

#elif WTF_OS_WINDOWS

inline size_t systemPageSize()
{
    static size_t size = 0;
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    size = system_info.dwPageSize;
    return size;
}

#elif WTF_OS_SYMBIAN

inline size_t systemPageSize()
{
    static TInt page_size = 0;
    UserHal::PageSizeInBytes(page_size);
    return page_size;
}

#endif

size_t pageSize()
{
    if (!s_pageSize)
        s_pageSize = systemPageSize();
    ASSERT(isPowerOfTwo(s_pageSize));
    return s_pageSize;
}

} 
