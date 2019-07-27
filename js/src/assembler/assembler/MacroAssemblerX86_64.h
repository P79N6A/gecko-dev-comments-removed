




























#ifndef assembler_assembler_MacroAssemblerX86_64_h
#define assembler_assembler_MacroAssemblerX86_64_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_CPU_X86_64

#include "assembler/assembler/MacroAssemblerX86Common.h"

namespace JSC {

class MacroAssemblerX86_64 : public MacroAssemblerX86Common {
public:
    static bool supportsFloatingPoint() { return true; }
};

} 

#endif 

#endif 
