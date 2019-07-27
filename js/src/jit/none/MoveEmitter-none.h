





#ifndef jit_none_MoveEmitter_none_h
#define jit_none_MoveEmitter_none_h

#include "jit/IonMacroAssembler.h"
#include "jit/MoveResolver.h"

namespace js {
namespace jit {

class MoveEmitterNone
{
  public:
    MoveEmitterNone(MacroAssemblerNone &) { MOZ_CRASH(); }
    void emit(const MoveResolver &) { MOZ_CRASH(); }
    void finish() { MOZ_CRASH(); }
};

typedef MoveEmitterNone MoveEmitter;

} 
} 

#endif 
