

































#include <signal.h>
#include "nanojit.h"

#ifdef SOLARIS
    typedef caddr_t maddr_ptr;
#else
    typedef void *maddr_ptr;
#endif

using namespace avmplus;

void
avmplus::AvmLog(char const *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    VMPI_vfprintf(stderr, msg, ap);
    va_end(ap);
}

#ifdef _DEBUG
namespace avmplus {
    void AvmAssertFail(const char* ) {
        fflush(stderr);
#if defined(WIN32)
        DebugBreak();
        exit(3);
#elif defined(__APPLE__)
        



        *((int *) NULL) = 0;  
        raise(SIGABRT);  
#else
        raise(SIGABRT);  
#endif
    }
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
    VMPI_setPageProtection(buffer, nbytes, true , true );
    return buffer;
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    VMPI_setPageProtection(p, nbytes, false , true );
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
    void* mem = valloc(nbytes);
    VMPI_setPageProtection(mem, nbytes, true , true );
    return mem;
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    VMPI_setPageProtection(p, nbytes, false , true );
    ::free(p);
}

#endif 





void
nanojit::CodeAlloc::markCodeChunkWrite(void*, size_t)
{}

void
nanojit::CodeAlloc::markCodeChunkExec(void*, size_t)
{}

bool
nanojit::CodeAlloc::checkChunkMark(void* , size_t , bool ) { 
    return true; 
}
