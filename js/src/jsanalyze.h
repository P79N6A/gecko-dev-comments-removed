







































#ifndef jsanalyze_h___
#define jsanalyze_h___

#include "jsarena.h"
#include "jscntxt.h"
#include "jsscript.h"

struct JSScript;

namespace js {
namespace analyze {

class Script;


struct Bytecode
{
    friend class Script;

    
    bool jumpTarget : 1;

    
    bool analyzed : 1;

    
    bool exceptionEntry : 1;

    
    bool inTryBlock : 1;

    
    bool safePoint : 1;

    
    uint32 stackDepth;

    



    uint32 defineCount;
    uint32 *defineArray;

    Bytecode()
    {
        PodZero(this);
    }

  private:
    bool mergeDefines(JSContext *cx,
                      Script *script, bool initial, uint32 newDepth,
                      uint32 *newArray, uint32 newCount);

    
    bool isDefined(uint32 slot)
    {
        JS_ASSERT(analyzed);
        for (size_t ind = 0; ind < defineCount; ind++) {
            if (defineArray[ind] == slot)
                return true;
        }
        return false;
    }
};


class Script
{
    friend struct Bytecode;

    JSScript *script;
    Bytecode **code;

    
    static const unsigned LOCAL_LIMIT = 50;

    
    uint32 *locals;

    static const uint32 LOCAL_USE_BEFORE_DEF = uint32(-1);
    static const uint32 LOCAL_CONDITIONALLY_DEFINED = uint32(-2);

    bool outOfMemory;
    bool hadFailure;
    bool usesRval;
    bool usesScope;

  public:
    
    JSArenaPool pool;

    void analyze(JSContext *cx, JSScript *script);
    void destroy();

    



    ~Script() { destroy(); }

    
    bool OOM() { return outOfMemory; }

    
    bool failed() { return hadFailure; }

    
    bool usesReturnValue() const { return usesRval; }

    
    bool usesScopeChain() const { return usesScope; }

    

    Bytecode& getCode(uint32 offset) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT(code[offset]);
        return *code[offset];
    }
    Bytecode& getCode(jsbytecode *pc) { return getCode(pc - script->code); }

    Bytecode* maybeCode(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return code[offset];
    }
    Bytecode* maybeCode(jsbytecode *pc) { return maybeCode(pc - script->code); }

    bool jumpTarget(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return code[offset] && code[offset]->jumpTarget;
    }
    bool jumpTarget(jsbytecode *pc) { return jumpTarget(pc - script->code); }

    

    unsigned localCount() {
        return (script->nfixed >= LOCAL_LIMIT) ? LOCAL_LIMIT : script->nfixed;
    }

    bool localHasUseBeforeDef(uint32 local) {
        JS_ASSERT(local < script->nfixed && !failed());
        return local >= localCount() || locals[local] == LOCAL_USE_BEFORE_DEF;
    }

    
    bool localDefined(uint32 local, uint32 offset) {
        return localHasUseBeforeDef(local) || (locals[local] <= offset) ||
            getCode(offset).isDefined(local);
    }
    bool localDefined(uint32 local, jsbytecode *pc) {
        return localDefined(local, pc - script->code);
    }

  private:
    void setOOM(JSContext *cx) {
        if (!outOfMemory)
            js_ReportOutOfMemory(cx);
        outOfMemory = true;
        hadFailure = true;
    }

    inline bool addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump,
                        unsigned stackDepth, uint32 *defineArray, unsigned defineCount);

    inline void setLocal(uint32 local, uint32 offset);
};

} 
} 

#endif 
