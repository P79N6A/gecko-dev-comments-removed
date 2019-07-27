





#ifndef jit_none_BaseMacroAssembler_none_h
#define jit_none_BaseMacroAssembler_none_h





#include "jit/IonSpewer.h"

namespace JSC {

class MacroAssemblerNone
{
  public:
    static bool supportsFloatingPoint() { return false; }
};

} 

#endif 
