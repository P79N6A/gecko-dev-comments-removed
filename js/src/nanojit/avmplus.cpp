

































#include "nanojit.h"

#ifdef SOLARIS
	#include <ucontext.h>
	#include <dlfcn.h>
	#include <procfs.h>
	#include <sys/stat.h>
    extern "C" caddr_t _getfp(void);
    typedef caddr_t maddr_ptr;
#else
    typedef void *maddr_ptr;
#endif

#if defined(AVMPLUS_ARM) && defined(UNDER_CE)
extern "C" bool
blx_lr_broken() {
    return false;
}
#endif

using namespace avmplus;

Config AvmCore::config;

void
avmplus::AvmLog(char const *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    VMPI_vfprintf(stderr, msg, ap);
    va_end(ap);
}

#ifdef _DEBUG
void NanoAssertFail()
{
    #if defined(WIN32)
        DebugBreak();
        exit(3);
    #elif defined(XP_OS2) || (defined(__GNUC__) && defined(__i386))
        asm("int $3");
        abort();
    #else
        abort();
    #endif
}
#endif

#ifdef WINCE






#ifndef MOZ_MEMORY
#error MOZ_MEMORY required for building on WINCE
#endif

void*
nanojit::CodeAlloc::allocCodeChunk(size_t nbytes) {
    void * buffer;
    posix_memalign(&buffer, 4096, nbytes);
    return buffer;
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    ::free(p);
}

#elif defined(WIN32)

void*
nanojit::CodeAlloc::allocCodeChunk(size_t nbytes) {
    return VirtualAlloc(NULL,
                        nbytes,
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_EXECUTE_READWRITE);
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t) {
    VirtualFree(p, 0, MEM_RELEASE);
}

#elif defined(AVMPLUS_OS2)

void*
nanojit::CodeAlloc::allocCodeChunk(size_t nbytes) {

    
    void * addr;
    if (DosAllocMem(&addr, nbytes, OBJ_ANY |
                    PAG_COMMIT | PAG_READ | PAG_WRITE | PAG_EXECUTE)) {
        if (DosAllocMem(&addr, nbytes,
                        PAG_COMMIT | PAG_READ | PAG_WRITE | PAG_EXECUTE)) {
            return 0;
        }
    }
    return addr;
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    DosFreeMem(p);
}

#elif defined(AVMPLUS_UNIX)

void*
nanojit::CodeAlloc::allocCodeChunk(size_t nbytes) {
    return mmap(NULL,
                nbytes,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANON,
                -1,
                0);
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    munmap((maddr_ptr)p, nbytes);
}

#else 

void*
nanojit::CodeAlloc::allocCodeChunk(size_t nbytes) {
    return valloc(nbytes);
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    ::free(p);
}

#endif 

