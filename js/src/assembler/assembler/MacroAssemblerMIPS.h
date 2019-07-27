

























#ifndef assembler_assembler_MacroAssemblerMIPS_h
#define assembler_assembler_MacroAssemblerMIPS_h

#if ENABLE(ASSEMBLER) && CPU(MIPS)

#include "assembler/wtf/Platform.h"

namespace JSC {

class MacroAssemblerMIPS {
public:
    static bool supportsFloatingPoint()
    {
#if (defined(__mips_hard_float) && !defined(__mips_single_float)) || defined(JS_MIPS_SIMULATOR)
        return true;
#else
        return false;
#endif
    }
};

}

#endif 

#endif 
