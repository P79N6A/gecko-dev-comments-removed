




























#ifndef assembler_assembler_MacroAssemblerX86_h
#define assembler_assembler_MacroAssemblerX86_h

#include "assembler/assembler/MacroAssemblerX86Common.h"

namespace JSC {

class MacroAssemblerX86 : public MacroAssemblerX86Common {
public:
    static bool supportsFloatingPoint() { return isSSE2Present(); }
};

} 

#endif 
