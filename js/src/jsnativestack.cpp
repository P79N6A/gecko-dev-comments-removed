





































#include <stdlib.h>
#include "jstypes.h"
#include "jsnativestack.h"

#ifdef XP_WIN
# include <windows.h>

#elif defined(XP_MACOSX) || defined(DARWIN) || defined(XP_UNIX)
# include <pthread.h>

#else
# error "Unsupported platform"

#endif

namespace js {

#if defined(XP_WIN) && defined(WINCE)

inline bool
isPageWritable(void *page)
{
    MEMORY_BASIC_INFORMATION memoryInformation;
    jsuword result = VirtualQuery(page, &memoryInformation, sizeof(memoryInformation));

    
    if (result != sizeof(memoryInformation))
        return false;

    jsuword protect = memoryInformation.Protect & ~(PAGE_GUARD | PAGE_NOCACHE);
    return protect == PAGE_READWRITE ||
           protect == PAGE_WRITECOPY ||
           protect == PAGE_EXECUTE_READWRITE ||
           protect == PAGE_EXECUTE_WRITECOPY;
}

void *
GetNativeStackBase()
{
    
    bool isGrowingDownward = JS_STACK_GROWTH_DIRECTION < 0;
    void *thisFrame = (void *)(&isGrowingDownward);

    static jsuword pageSize = 0;
    if (!pageSize) {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        pageSize = systemInfo.dwPageSize;
    }

    
    register char *currentPage = (char *)((jsuword)thisFrame & ~(pageSize - 1));
    if (isGrowingDownward) {
        while (currentPage > 0) {
            
            if (currentPage >= (char *)pageSize)
                currentPage -= pageSize;
            else
                currentPage = 0;
            if (!isPageWritable(currentPage))
                return currentPage + pageSize;
        }
        return 0;
    } else {
        while (true) {
            
            currentPage += pageSize;
            if (!isPageWritable(currentPage))
                return currentPage;
        }
    }
}

#elif defined(XP_WIN)

void *
GetNativeStackBaseImpl()
{
# if defined(_M_IX86) && defined(_MSC_VER)
    



    NT_TIB* pTib;
    __asm {
        MOV EAX, FS:[18h]
        MOV pTib, EAX
    }
    return static_cast<void*>(pTib->StackBase);

# elif defined(_M_X64) && defined(_MSC_VER)
    PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());
    return reinterpret_cast<void*>(pTib->StackBase);

# elif defined(_WIN32) && defined(__GNUC__)
    NT_TIB* pTib;
    asm ("movl %%fs:0x18, %0\n" : "=r" (pTib));
    return static_cast<void*>(pTib->StackBase);

# endif
}

#else 

void *
GetNativeStackBaseImpl()
{
    pthread_t thread = pthread_self();
# if defined(XP_MACOSX) || defined(DARWIN)
    return pthread_get_stackaddr_np(thread);

# else
    pthread_attr_t sattr;
    pthread_attr_init(&sattr);
#  if defined(PTHREAD_NP_H) || defined(NETBSD)
    
    pthread_attr_get_np(thread, &sattr);
#  else
    



    pthread_getattr_np(thread, &sattr);
#  endif

    void *stackBase = 0;
    size_t stackSize = 0;
#  ifdef DEBUG
    int rc = 
#  endif
        pthread_attr_getstack(&sattr, &stackBase, &stackSize);
    JS_ASSERT(!rc);
    JS_ASSERT(stackBase);
    pthread_attr_destroy(&sattr);

#  if JS_STACK_GROWTH_DIRECTION > 0
    return stackBase;
#  else
    return static_cast<char*>(stackBase) + stackSize;
#  endif
# endif
}

#endif 

} 
