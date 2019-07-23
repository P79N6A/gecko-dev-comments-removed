

































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

#elif defined(AVMPLUS_OS2)

void
VMPI_setPageProtection(void *address,
                       size_t size,
                       bool executableFlag,
                       bool writeableFlag)
{
    ULONG flags = PAG_READ;
    if (executableFlag) {
        flags |= PAG_EXECUTE;
    }
    if (writeableFlag) {
        flags |= PAG_WRITE;
    }
    address = (void*)((size_t)address & ~(0xfff));
    size = (size + 0xfff) & ~(0xfff);

    ULONG attribFlags = PAG_FREE;
    while (size) {
        ULONG attrib;
        ULONG range = size;
        ULONG retval = DosQueryMem(address, &range, &attrib);
        AvmAssert(retval == 0);

        
        if (attrib & attribFlags) {
            break;
        }
        attribFlags |= PAG_BASE;

        range = size > range ? range : size;
        retval = DosSetMem(address, range, flags);
        AvmAssert(retval == 0);

        address = (char*)address + range;
        size -= range;
    }
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
