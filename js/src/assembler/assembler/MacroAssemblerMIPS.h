

























#ifndef assembler_assembler_MacroAssemblerMIPS_h
#define assembler_assembler_MacroAssemblerMIPS_h

#if ENABLE(ASSEMBLER) && CPU(MIPS)

#include "assembler/wtf/Platform.h"

namespace JSC {

class MacroAssemblerMIPS {
public:
    static bool supportsFloatingPoint()
    {
#if WTF_MIPS_DOUBLE_FLOAT
        return true;
#else
        return false;
#endif
    }
};

}

#endif 

#endif 
