





#ifndef jsopcodeinlines_h__
#define jsopcodeinlines_h__

#include "jsautooplen.h"

#include "frontend/BytecodeEmitter.h"

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

static inline bool
IsJumpOpcode(JSOp op)
{
    uint32_t type = JOF_TYPE(js_CodeSpec[op].format);

    



    return type == JOF_JUMP && op != JSOP_LABEL;
}

static inline bool
BytecodeFallsThrough(JSOp op)
{
    switch (op) {
      case JSOP_GOTO:
      case JSOP_DEFAULT:
      case JSOP_RETURN:
      case JSOP_STOP:
      case JSOP_RETRVAL:
      case JSOP_THROW:
      case JSOP_TABLESWITCH:
        return false;
      case JSOP_GOSUB:
        
        return true;
      default:
        return true;
    }
}

static inline PropertyName *
GetNameFromBytecode(JSContext *cx, JSScript *script, jsbytecode *pc, JSOp op)
{
    if (op == JSOP_LENGTH)
        return cx->names().length;

    
    
    if (op == JSOP_INSTANCEOF)
        return cx->names().classPrototype;

    PropertyName *name;
    GET_NAME_FROM_BYTECODE(script, pc, 0, name);
    return name;
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

class SrcNoteLineScanner
{
    
    ptrdiff_t offset;

    
    jssrcnote *sn;

    
    uint32_t lineno;

    




    bool lineHeader;

public:
    SrcNoteLineScanner(jssrcnote *sn, uint32_t lineno)
        : offset(0), sn(sn), lineno(lineno)
    {
    }

    












    void advanceTo(ptrdiff_t relpc) {
        
        
        JS_ASSERT_IF(offset > 0, relpc > offset);

        
        JS_ASSERT_IF(offset > 0, SN_IS_TERMINATOR(sn) || SN_DELTA(sn) > 0);

        
        lineHeader = (offset == 0);

        if (SN_IS_TERMINATOR(sn))
            return;

        ptrdiff_t nextOffset;
        while ((nextOffset = offset + SN_DELTA(sn)) <= relpc && !SN_IS_TERMINATOR(sn)) {
            offset = nextOffset;
            SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
            if (type == SRC_SETLINE || type == SRC_NEWLINE) {
                if (type == SRC_SETLINE)
                    lineno = js_GetSrcNoteOffset(sn, 0);
                else
                    lineno++;

                if (offset == relpc)
                    lineHeader = true;
            }

            sn = SN_NEXT(sn);
        }
    }

    bool isLineHeader() const {
        return lineHeader;
    }

    uint32_t getLine() const { return lineno; }
};

}

#endif 
