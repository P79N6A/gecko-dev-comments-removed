







































#ifndef jsanalyze_h___
#define jsanalyze_h___

#include "jsarena.h"
#include "jscntxt.h"
#include "jsinfer.h"
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

    Bytecode(Script *script, unsigned offset)
    {
        PodZero(this);

#ifdef JS_TYPE_INFERENCE
        this->script = script;
        this->offset = offset;
#endif
    }

  private:
    bool mergeDefines(JSContext *cx, Script *script, bool initial,
                      uint32 newDepth, types::TypeStack *newStack,
                      uint32 *newArray, uint32 newCount);

    
    bool isDefined(uint32 slot)
    {
        JS_ASSERT(analyzed);
        for (unsigned ind = 0; ind < defineCount; ind++) {
            if (defineArray[ind] == slot)
                return true;
        }
        return false;
    }

#ifdef JS_TYPE_INFERENCE
  public:

    Script *script;
    unsigned offset;

    
    types::TypeStack *inStack;

    
    types::TypeStack *pushedArray;

    
    types::TypeObject *initObject;

    
    bool monitorNeeded;

    



    bool missingTypes;

    
    inline JSArenaPool &pool();

    
    inline types::TypeSet *popped(unsigned num);

    
    inline types::TypeSet *pushed(unsigned num);

    
    inline void setFixed(JSContext *cx, unsigned num, types::jstype type);

    



    inline types::TypeObject* getInitObject(JSContext *cx, bool isArray);

    void print(JSContext *cx, FILE *out);

#endif 

};


class Script
{
    friend struct Bytecode;

    JSScript *script;
    Bytecode **codeArray;

    
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

    void init(JSScript *script);
    void analyze(JSContext *cx);
    void destroy();

    



    ~Script() { destroy(); }

    
    bool OOM() { return outOfMemory; }

    
    bool failed() { return hadFailure; }

    
    bool usesReturnValue() const { return usesRval; }

    
    bool usesScopeChain() const { return usesScope; }

    
    bool hasAnalyzed() const { return !!codeArray; }

    
    JSScript *getScript() const { return script; }

    

    Bytecode& getCode(uint32 offset) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT(codeArray[offset]);
        return *codeArray[offset];
    }
    Bytecode& getCode(const jsbytecode *pc) { return getCode(pc - script->code); }

    Bytecode* maybeCode(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return codeArray[offset];
    }
    Bytecode* maybeCode(const jsbytecode *pc) { return maybeCode(pc - script->code); }

    bool jumpTarget(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return codeArray[offset] && codeArray[offset]->jumpTarget;
    }
    bool jumpTarget(const jsbytecode *pc) { return jumpTarget(pc - script->code); }

    

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
                        unsigned stackDepth, types::TypeStack *stack,
                        uint32 *defineArray, unsigned defineCount);

    inline void setLocal(uint32 local, uint32 offset);

#ifdef JS_TYPE_INFERENCE
  public:

    
    unsigned id;

    
    types::TypeFunction *function;

    
    unsigned argCount;
    jsid thisName;

    
    types::TypeObject *objects;

    



    JSScript *parent;
    const jsbytecode *parentpc;

    




    types::VariableSet localTypes;

    
    types::TypeSet thisTypes;

    
    jsuword *localNames;

    
    bool compiled;

    
    bool recompileNeeded;

    void setFunction(JSContext *cx, JSFunction *fun);

    inline bool isEval() { return parent && !function; }

    



    inline Script *evalParent();

    
    inline Bytecode *parentCode();

    void print(JSContext *cx);

    

    
    void analyzeTypes(JSContext *cx, Bytecode *codeType);

    




    void freezeTypes(JSContext *cx, Bytecode *codeType);
    void freezeAllTypes(JSContext *cx);

    



    inline jsid getLocalId(unsigned index, types::TypeStack *stack);

    
    inline jsid getArgumentId(unsigned index);

    
    inline types::TypeSet *getStackTypes(unsigned index, types::TypeStack *stack);

#endif 

};

static inline unsigned
GetBytecodeLength(jsbytecode *pc)
{
    JSOp op = (JSOp)*pc;
    JS_ASSERT(op < JSOP_LIMIT);
    JS_ASSERT(op != JSOP_TRAP);

    if (js_CodeSpec[op].length != -1)
        return js_CodeSpec[op].length;
    return js_GetVariableBytecodeLength(pc);
}

static inline unsigned
GetUseCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;
    if (js_CodeSpec[*pc].nuses == -1)
        return js_GetVariableStackUses(JSOp(*pc), pc);
    return js_CodeSpec[*pc].nuses;
}

static inline unsigned
GetDefCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;
    if (js_CodeSpec[*pc].ndefs == -1)
        return js_GetEnterBlockStackDefs(NULL, script, pc);

    



    switch (JSOp(*pc)) {
      case JSOP_OR:
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
        return 1;
      default:
        return js_CodeSpec[*pc].ndefs;
    }
}

} 
} 

#endif 
