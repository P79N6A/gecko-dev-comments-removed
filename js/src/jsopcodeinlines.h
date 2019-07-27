





#ifndef jsopcodeinlines_h
#define jsopcodeinlines_h

#include "jsopcode.h"

#include "jsscript.h"

namespace js {

static inline unsigned
GetDefCount(JSScript* script, unsigned offset)
{
    jsbytecode* pc = script->offsetToPC(offset);

    



    switch (JSOp(*pc)) {
      case JSOP_OR:
      case JSOP_AND:
        return 1;
      case JSOP_PICK:
        





        return pc[1] + 1;
      default:
        return StackDefs(script, pc);
    }
}

static inline unsigned
GetUseCount(JSScript* script, unsigned offset)
{
    jsbytecode* pc = script->offsetToPC(offset);

    if (JSOp(*pc) == JSOP_PICK)
        return pc[1] + 1;
    if (js_CodeSpec[*pc].nuses == -1)
        return StackUses(script, pc);
    return js_CodeSpec[*pc].nuses;
}

static inline JSOp
ReverseCompareOp(JSOp op)
{
    switch (op) {
      case JSOP_GT:
        return JSOP_LT;
      case JSOP_GE:
        return JSOP_LE;
      case JSOP_LT:
        return JSOP_GT;
      case JSOP_LE:
        return JSOP_GE;
      case JSOP_EQ:
      case JSOP_NE:
      case JSOP_STRICTEQ:
      case JSOP_STRICTNE:
        return op;
      default:
        MOZ_CRASH("unrecognized op");
    }
}

static inline JSOp
NegateCompareOp(JSOp op)
{
    switch (op) {
      case JSOP_GT:
        return JSOP_LE;
      case JSOP_GE:
        return JSOP_LT;
      case JSOP_LT:
        return JSOP_GE;
      case JSOP_LE:
        return JSOP_GT;
      case JSOP_EQ:
        return JSOP_NE;
      case JSOP_NE:
        return JSOP_EQ;
      case JSOP_STRICTNE:
        return JSOP_STRICTEQ;
      case JSOP_STRICTEQ:
        return JSOP_STRICTNE;
      default:
        MOZ_CRASH("unrecognized op");
    }
}

class BytecodeRange {
  public:
    BytecodeRange(JSContext* cx, JSScript* script)
      : script(cx, script), pc(script->code()), end(pc + script->length())
    {}
    bool empty() const { return pc == end; }
    jsbytecode* frontPC() const { return pc; }
    JSOp frontOpcode() const { return JSOp(*pc); }
    size_t frontOffset() const { return script->pcToOffset(pc); }
    void popFront() { pc += GetBytecodeLength(pc); }

  private:
    RootedScript script;
    jsbytecode* pc;
    jsbytecode* end;
};

}

#endif 
