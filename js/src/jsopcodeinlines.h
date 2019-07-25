





































#include "jsautooplen.h"

#include "frontend/BytecodeEmitter.h"

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
