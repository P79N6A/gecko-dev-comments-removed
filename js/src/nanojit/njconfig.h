






































#ifndef __njconfig_h__
#define __njconfig_h__

#include "avmplus.h"



#ifdef FEATURE_NANOJIT

namespace nanojit
{
    















    struct Config
    {
    public:
        
        Config();

        
        uint8_t arm_arch;

        
        uint32_t cseopt:1;

        
        uint32_t i386_sse2:1;

        
        uint32_t i386_use_cmov:1;

        
        uint32_t i386_fixed_esp:1;

        
        uint32_t arm_vfp:1;

        
        uint32_t arm_show_stats:1;

        
        
        uint32_t soft_float:1;

        
        uint32_t harden_function_alignment:1;

        
        uint32_t harden_nop_insertion:1;
    };
}

#endif 
#endif 
