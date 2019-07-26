








































#include "cpu_detect.h"
#include "STTypes.h"

#if defined(SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS)
    #if defined(__GNUC__) && defined (HAVE_CPUID_H)
        
        #include "cpuid.h"
    #elif defined(_M_IX86)
        
        #include <intrin.h>
    #endif
    
    #ifndef bit_MMX
        #define bit_MMX (1 << 23)
    #endif
    #ifndef bit_SSE
       #define bit_SSE (1 << 25)
    #endif
    #ifndef bit_SSE2
        #define bit_SSE2 (1 << 26)
    #endif
#endif









static uint _dwDisabledISA = 0x00;      


void disableExtensions(uint dwDisableMask)
{
    _dwDisabledISA = dwDisableMask;
}




uint detectCPUextensions(void)
{



#if ((defined(__GNUC__) && defined(__x86_64__)) \
    || defined(_M_X64))  \
    && defined(SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS)
    return 0x19 & ~_dwDisabledISA;



#elif ((defined(__GNUC__) && defined(__i386__)) \
    || defined(_M_IX86))  \
    && defined(SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS)

    if (_dwDisabledISA == 0xffffffff) return 0;
 
    uint res = 0;
 
#if !defined(__GNUC__)
    
    
    int reg[4] = {-1};

    
    __cpuid(reg,0);
    if ((unsigned int)reg[0] == 0) return 0; 

    __cpuid(reg,1);
    if ((unsigned int)reg[3] & bit_MMX)  res = res | SUPPORT_MMX;
    if ((unsigned int)reg[3] & bit_SSE)  res = res | SUPPORT_SSE;
    if ((unsigned int)reg[3] & bit_SSE2) res = res | SUPPORT_SSE2;
#elif defined(HAVE_CPUID_H)
    
    uint eax, ebx, ecx, edx;  

    
    if (!__get_cpuid (1, &eax, &ebx, &ecx, &edx)) return 0; 

    if (edx & bit_MMX)  res = res | SUPPORT_MMX;
    if (edx & bit_SSE)  res = res | SUPPORT_SSE;
    if (edx & bit_SSE2) res = res | SUPPORT_SSE2;
#else
    
    return 0;
#endif

    return res & ~_dwDisabledISA;

#else





    return 0;

#endif
}
