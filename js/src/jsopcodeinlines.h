





#ifndef jsopcodeinlines_h
#define jsopcodeinlines_h

#include "jsopcode.h"

#include "jsscript.h"

namespace js {

static inline unsigned
GetDefCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;

    



    switch (JSOp(*pc)) {
      case JSOP_OR:
      case JSOP_AND:
        return 1;
      case JSOP_PICK:
        





        return (pc[1] + 1);
      default:
        return StackDefs(script, pc);
    }
}

static inline unsigned
GetUseCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;

    if (JSOp(*pc) == JSOP_PICK)
        return (pc[1] + 1);
    if (js_CodeSpec[*pc].nuses == -1)
        return StackUses(script, pc);
    return js_CodeSpec[*pc].nuses;
}

class BytecodeRange {
  public:
    BytecodeRange(JSContext *cx, JSScript *script)
      : script(cx, script), pc(script->code), end(pc + script->length) {}
    bool empty() const { return pc == end; }
    jsbytecode *frontPC() const { return pc; }
    JSOp frontOpcode() const { return JSOp(*pc); }
    size_t frontOffset() const { return pc - script->code; }
    void popFront() { pc += GetBytecodeLength(pc); }

  private:
    RootedScript script;
    jsbytecode *pc, *end;
};

}

#endif 
