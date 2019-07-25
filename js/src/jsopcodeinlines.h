





































#include "jsautooplen.h"

namespace js {

class BytecodeRange {
  public:
    BytecodeRange(JSScript *script)
      : script(script), pc(script->code), end(pc + script->length) {}
    bool empty() const { return pc == end; }
    jsbytecode *frontPC() const { return pc; }
    JSOp frontOpcode() const { return JSOp(*pc); }
    size_t frontOffset() const { return pc - script->code; }
    void popFront() { pc += GetBytecodeLength(pc); }

  private:
    JSScript *script;
    jsbytecode *pc, *end;
};





JS_ALWAYS_INLINE jsbytecode *
AdvanceOverBlockchainOp(jsbytecode *pc)
{
    if (*pc == JSOP_NULLBLOCKCHAIN)
        return pc + JSOP_NULLBLOCKCHAIN_LENGTH;
    if (*pc == JSOP_BLOCKCHAIN)
        return pc + JSOP_BLOCKCHAIN_LENGTH;
    return pc;
}

}
