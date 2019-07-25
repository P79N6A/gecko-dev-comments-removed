





























































#if defined(__APPLE__)
#   include <libkern/OSAtomic.h>
    


#   define PaUtil_FullMemoryBarrier()  OSMemoryBarrier()
#   define PaUtil_ReadMemoryBarrier()  OSMemoryBarrier()
#   define PaUtil_WriteMemoryBarrier() OSMemoryBarrier()
#elif defined(__GNUC__)
    
#   if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#      define PaUtil_FullMemoryBarrier()  __sync_synchronize()
#      define PaUtil_ReadMemoryBarrier()  __sync_synchronize()
#      define PaUtil_WriteMemoryBarrier() __sync_synchronize()
    

    

#   elif defined( __ppc__ ) || defined( __powerpc__) || defined( __PPC__ )
#      define PaUtil_FullMemoryBarrier()  asm volatile("sync":::"memory")
#      define PaUtil_ReadMemoryBarrier()  asm volatile("sync":::"memory")
#      define PaUtil_WriteMemoryBarrier() asm volatile("sync":::"memory")
#   elif defined( __i386__ ) || defined( __i486__ ) || defined( __i586__ ) || \
         defined( __i686__ ) || defined( __x86_64__ )
#      define PaUtil_FullMemoryBarrier()  asm volatile("mfence":::"memory")
#      define PaUtil_ReadMemoryBarrier()  asm volatile("lfence":::"memory")
#      define PaUtil_WriteMemoryBarrier() asm volatile("sfence":::"memory")
#   else
#      ifdef ALLOW_SMP_DANGERS
#         warning Memory barriers not defined on this system or system unknown
#         warning For SMP safety, you should fix this.
#         define PaUtil_FullMemoryBarrier()
#         define PaUtil_ReadMemoryBarrier()
#         define PaUtil_WriteMemoryBarrier()
#      else
#         error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#      endif
#   endif
#elif (_MSC_VER >= 1400) && !defined(_WIN32_WCE)
#   include <intrin.h>
#   pragma intrinsic(_ReadWriteBarrier)
#   pragma intrinsic(_ReadBarrier)
#   pragma intrinsic(_WriteBarrier)
#   define PaUtil_FullMemoryBarrier()  _ReadWriteBarrier()
#   define PaUtil_ReadMemoryBarrier()  _ReadBarrier()
#   define PaUtil_WriteMemoryBarrier() _WriteBarrier()
#elif defined(_WIN32_WCE)
#   define PaUtil_FullMemoryBarrier()
#   define PaUtil_ReadMemoryBarrier()
#   define PaUtil_WriteMemoryBarrier()
#elif defined(_MSC_VER) || defined(__BORLANDC__)
#   define PaUtil_FullMemoryBarrier()  _asm { lock add    [esp], 0 }
#   define PaUtil_ReadMemoryBarrier()  _asm { lock add    [esp], 0 }
#   define PaUtil_WriteMemoryBarrier() _asm { lock add    [esp], 0 }
#else
#   ifdef ALLOW_SMP_DANGERS
#      warning Memory barriers not defined on this system or system unknown
#      warning For SMP safety, you should fix this.
#      define PaUtil_FullMemoryBarrier()
#      define PaUtil_ReadMemoryBarrier()
#      define PaUtil_WriteMemoryBarrier()
#   else
#      error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#   endif
#endif
