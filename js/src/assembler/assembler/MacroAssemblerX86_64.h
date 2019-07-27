




























#ifndef assembler_assembler_MacroAssemblerX86_64_h
#define assembler_assembler_MacroAssemblerX86_64_h

#include "assembler/assembler/MacroAssemblerX86Common.h"

namespace JSC {

class MacroAssemblerX86_64 : public MacroAssemblerX86Common {
public:
    static bool supportsFloatingPoint() { return true; }
};

} 

#endif 
