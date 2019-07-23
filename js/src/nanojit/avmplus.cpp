

































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

using namespace avmplus;

Config AvmCore::config;
static GC _gc;
GC* AvmCore::gc = &_gc;
GCHeap GC::heap;
String* AvmCore::k_str[] = { (String*)"" };

void
avmplus::AvmLog(char const *msg, ...) {
}

#ifdef _DEBUG

void NanoAssertFail()
{
    #if defined(WIN32)
        DebugBreak();
        exit(3);
    #elif defined(XP_OS2) || (defined(__GNUC__) && defined(__i386))
        asm("int $3");
    #endif

    abort();
}
#endif

#ifdef WIN32
void
VMPI_setPageProtection(void *address,
                       size_t size,
                       bool executableFlag,
                       bool writeableFlag)
{
    DWORD oldProtectFlags = 0;
    DWORD newProtectFlags = 0;
    if ( executableFlag && writeableFlag ) {
        newProtectFlags = PAGE_EXECUTE_READWRITE;
    } else if ( executableFlag ) {
        newProtectFlags = PAGE_EXECUTE_READ;
    } else if ( writeableFlag ) {
        newProtectFlags = PAGE_READWRITE;
    } else {
        newProtectFlags = PAGE_READONLY;
    }

    BOOL retval;
    MEMORY_BASIC_INFORMATION mbi;
    do {
        VirtualQuery(address, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
        size_t markSize = size > mbi.RegionSize ? mbi.RegionSize : size;

        retval = VirtualProtect(address, markSize, newProtectFlags, &oldProtectFlags);
        NanoAssert(retval);

        address = (char*) address + markSize;
        size -= markSize;
    } while(size > 0 && retval);

    
    NanoAssert((oldProtectFlags & PAGE_GUARD) == 0);
}

#else 

void VMPI_setPageProtection(void *address,
                            size_t size,
                            bool executableFlag,
                            bool writeableFlag)
{
  int bitmask = sysconf(_SC_PAGESIZE) - 1;
  
  void *endAddress = (void*) ((char*)address + size);
  void *beginPage = (void*) ((size_t)address & ~bitmask);
  void *endPage   = (void*) (((size_t)endAddress + bitmask) & ~bitmask);
  size_t sizePaged = (size_t)endPage - (size_t)beginPage;

  int flags = PROT_READ;
  if (executableFlag) {
    flags |= PROT_EXEC;
  }
  if (writeableFlag) {
    flags |= PROT_WRITE;
  }
  int retval = mprotect((maddr_ptr)beginPage, (unsigned int)sizePaged, flags);
  AvmAssert(retval == 0);
  (void)retval;
}

#endif 



#ifdef WIN32
void*
nanojit::CodeAlloc::allocCodeChunk(size_t nbytes) {
    return VirtualAlloc(NULL,
                        nbytes,
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_EXECUTE_READWRITE);
}

void
nanojit::CodeAlloc::freeCodeChunk(void *p, size_t nbytes) {
    VirtualFree(p, 0, MEM_RELEASE);
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
    free(p);
}

#endif 

