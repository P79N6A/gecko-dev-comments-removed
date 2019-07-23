








































#include <windows.h>
#include "prlog.h"

extern PRLogModuleInfo* _pr_msgc_lm;

#define GC_VMBASE               0x40000000
#define GC_VMLIMIT              0x00FFFFFF








void *baseaddr = (void*) GC_VMBASE;
void *lastaddr = (void*) GC_VMBASE;

void _MD_InitGC() {}

void *_MD_GrowGCHeap(PRUint32 *sizep)
{
    void *addr;
    size_t size;

    
    if( lastaddr == baseaddr ) {
        addr = VirtualAlloc( (void *)GC_VMBASE, GC_VMLIMIT, MEM_RESERVE, PAGE_READWRITE );

        



        if (addr == NULL) {
            addr = VirtualAlloc( NULL, GC_VMLIMIT, MEM_RESERVE, PAGE_READWRITE );

            baseaddr = lastaddr = addr;
            if (addr == NULL) {
                PR_LOG(_pr_msgc_lm, PR_LOG_ALWAYS, ("GC: unable to allocate heap: LastError=%ld",
                       GetLastError()));
                return 0;
            }
        }
    }
    size = *sizep;

    
    addr = VirtualAlloc( lastaddr, size, MEM_COMMIT, PAGE_READWRITE );
    if (addr == NULL) {
        return 0;
    }

    lastaddr = ((char*)addr + size);
    PR_LOG(_pr_msgc_lm, PR_LOG_ALWAYS,
	   ("GC: heap extends from %08x to %08x",
	    baseaddr, (long)baseaddr + (char*)lastaddr - (char*)baseaddr));

    return addr;
}

PRBool _MD_ExtendGCHeap(char *base, PRInt32 oldSize, PRInt32 newSize) {
  void* addr;

  addr = VirtualAlloc( base + oldSize, newSize - oldSize,
		       MEM_COMMIT, PAGE_READWRITE );
  if (NULL == addr) {
    PR_LOG(_pr_msgc_lm, PR_LOG_ALWAYS, ("GC: unable to extend heap: LastError=%ld",
		     GetLastError()));
    return PR_FALSE;
  }
  if (base + oldSize != (char*)addr) {
    PR_LOG(_pr_msgc_lm, PR_LOG_ALWAYS, ("GC: segment extension returned %x instead of %x",
		     addr, base + oldSize));
    VirtualFree(addr, newSize - oldSize, MEM_DECOMMIT);
    return PR_FALSE;
  }
  lastaddr = base + newSize;
  PR_LOG(_pr_msgc_lm, PR_LOG_ALWAYS,
	 ("GC: heap now extends from %p to %p",
	  base, base + newSize));
  return PR_TRUE;
}


void _MD_FreeGCSegment(void *base, PRInt32 len)
{
     (void)VirtualFree(base, 0, MEM_RELEASE);
}
