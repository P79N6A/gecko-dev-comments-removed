






































#include "nanojit.h"

#ifdef FEATURE_NANOJIT

namespace nanojit
{
#ifdef NANOJIT_IA32
    static int getCpuFeatures()
    {
        int features = 0;
    #if defined _MSC_VER
        __asm
        {
            pushad
            mov eax, 1
            cpuid
            mov features, edx
            popad
        }
    #elif defined __GNUC__
        asm("xchg %%esi, %%ebx\n" 
            "mov $0x01, %%eax\n"
            "cpuid\n"
            "mov %%edx, %0\n"
            "xchg %%esi, %%ebx\n"
            : "=m" (features)
            : 
            : "%eax", "%esi", "%ecx", "%edx"
           );
    #elif defined __SUNPRO_C || defined __SUNPRO_CC
        asm("push %%ebx\n"
            "mov $0x01, %%eax\n"
            "cpuid\n"
            "pop %%ebx\n"
            : "=d" (features)
            : 
            : "%eax", "%ecx"
           );
    #endif
        return features;
    }
#endif

    Config::Config()
    {
        VMPI_memset(this, 0, sizeof(*this));

        cseopt = true;

#ifdef NANOJIT_IA32
        int const features = getCpuFeatures();
        i386_sse2 = (features & (1<<26)) != 0;
        i386_use_cmov = (features & (1<<15)) != 0;
        i386_fixed_esp = false;
#endif

#if defined(NANOJIT_ARM)

        
        

        arm_arch = NJ_COMPILER_ARM_ARCH;
        arm_vfp = (arm_arch >= 7);

    #if defined(DEBUG) || defined(_DEBUG)
        arm_show_stats = true;
    #else
        arm_show_stats = false;
    #endif

        soft_float = !arm_vfp;

#endif 
    }
}
#endif 
