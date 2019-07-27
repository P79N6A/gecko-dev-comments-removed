






























#ifndef assembler_assembler_MacroAssemblerARM_h
#define assembler_assembler_MacroAssemblerARM_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_CPU_ARM_TRADITIONAL

namespace JSC {

class MacroAssemblerARM {
public:
    static bool supportsFloatingPoint() { return s_isVFPPresent; }
    static const bool s_isVFPPresent;
};

}

#endif 

#endif 
