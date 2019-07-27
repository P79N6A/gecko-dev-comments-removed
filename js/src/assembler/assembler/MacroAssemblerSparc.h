





#ifndef assembler_assembler_MacroAssemblerSparc_h
#define assembler_assembler_MacroAssemblerSparc_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_CPU_SPARC

namespace JSC {

class MacroAssemblerSparc {
public:
    static bool supportsFloatingPoint() { return true; }
};

}

#endif 

#endif 
