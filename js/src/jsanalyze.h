







































#ifndef jsanalyze_h___
#define jsanalyze_h___

#include "jsarena.h"
#include "jscntxt.h"
#include "jsinfer.h"
#include "jsscript.h"

struct JSScript;


namespace js { namespace mjit { struct RegisterAllocation; } }

namespace js {
namespace analyze {

class Script;


struct Bytecode
{
    friend class Script;

    
    bool jumpTarget : 1;    

    
    bool fallthrough : 1;

    
    bool jumpFallthrough : 1;

    
    bool switchTarget : 1;

    
    bool analyzed : 1;

    
    bool exceptionEntry : 1;

    
    bool inTryBlock : 1;

    
    bool safePoint : 1;

    



    bool monitorNeeded : 1;

    
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

    
    types::TypeObject *initArray;
    types::TypeObject *initObject;

    



    bool hasIncDecOverflow : 1;

    
    inline JSArenaPool &pool();

    
    inline types::TypeSet *popped(unsigned num);

    
    inline types::TypeSet *pushed(unsigned num);

    
    inline void setFixed(JSContext *cx, unsigned num, types::jstype type);

    



    inline types::TypeObject* getInitObject(JSContext *cx, bool isArray);

#ifdef DEBUG
    void print(JSContext *cx);
#endif

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

    bool monitored(uint32 offset) { return getCode(offset).monitorNeeded; }
    bool monitored(const jsbytecode *pc) { return getCode(pc).monitorNeeded; }

    

    unsigned localCount() {
        return (script->nfixed >= LOCAL_LIMIT) ? LOCAL_LIMIT : script->nfixed;
    }

    bool localHasUseBeforeDef(uint32 local) {
        JS_ASSERT(!failed());
        return local >= localCount() || locals[local] == LOCAL_USE_BEFORE_DEF;
    }

    
    bool localDefined(uint32 local, uint32 offset) {
        return localHasUseBeforeDef(local) || (locals[local] <= offset) ||
            getCode(offset).isDefined(local);
    }
    bool localDefined(uint32 local, jsbytecode *pc) {
        return localDefined(local, pc - script->code);
    }

    bool argEscapes(unsigned arg)
    {
        for (unsigned i = 0; i < script->nClosedArgs; i++) {
            if (arg == script->getClosedArg(i))
                return true;
        }
        return false;
    }

    bool localEscapes(unsigned local)
    {
        if (local >= localCount())
            return true;
        for (unsigned i = 0; i < script->nClosedVars; i++) {
            if (local == script->getClosedVar(i))
                return true;
        }
        return false;
    }

    void trace(JSTracer *trc);
    void sweep(JSContext *cx);
    void detach();

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

    
    JSFunction *fun;

    
    JSObject *global;

    
    types::TypeObject *objects;

    



    JSScript *parent;
    const jsbytecode *parentpc;

    



    types::Variable **variableSet;
    unsigned variableCount;

    
    types::TypeSet thisTypes;

    
    jsuword *localNames;

    void setFunction(JSContext *cx, JSFunction *fun);

    bool isEval() { return parent && !fun; }
    bool isGlobal() { return !parent || (!fun && !parent->analysis->parent); }

    unsigned argCount() { return fun ? fun->nargs : 0; }
    types::TypeFunction *function() { return fun->getType()->asFunction(); }

    inline JSObject *getGlobal();
    inline types::TypeObject *getGlobalType();

    



    inline Script *evalParent();

    
    inline Bytecode *parentCode();

    
    void finish(JSContext *cx);

    

    

    struct AnalyzeStateStack {
        
        bool isForEach;

        
        Script *scope;

        
        bool hasDouble;
        double doubleValue;

        
        bool isZero;

        
        bool isConstant;
    };

    struct AnalyzeState {
        AnalyzeStateStack *stack;

        
        unsigned stackDepth;

        
        bool hasGetSet;

        
        bool hasHole;

        
        bool zeroLocals[4];
        uint32 constLocals[4];
        unsigned numConstLocals;

        AnalyzeState()
            : stack(NULL), stackDepth(0), hasGetSet(false), hasHole(false), numConstLocals(0)
        {}

        bool init(JSContext *cx, JSScript *script)
        {
            if (script->nslots) {
                stack = (AnalyzeStateStack *)
                    cx->calloc(script->nslots * sizeof(AnalyzeStateStack));
                return (stack != NULL);
            }
            return true;
        }

        void destroy(JSContext *cx)
        {
            cx->free(stack);
        }

        AnalyzeStateStack &popped(unsigned i) {
            JS_ASSERT(i < stackDepth);
            return stack[stackDepth - 1 - i];
        }

        const AnalyzeStateStack &popped(unsigned i) const {
            JS_ASSERT(i < stackDepth);
            return stack[stackDepth - 1 - i];
        }

        void addConstLocal(uint32 local, bool zero) {
            if (numConstLocals == JS_ARRAY_LENGTH(constLocals))
                return;
            if (maybeLocalConst(local, false))
                return;
            zeroLocals[numConstLocals] = zero;
            constLocals[numConstLocals++] = local;
        }

        bool maybeLocalConst(uint32 local, bool zero) {
            for (unsigned i = 0; i < numConstLocals; i++) {
                if (constLocals[i] == local)
                    return !zero || zeroLocals[i];
            }
            return false;
        }

        void clearLocal(uint32 local) {
            for (unsigned i = 0; i < numConstLocals; i++) {
                if (constLocals[i] == local) {
                    constLocals[i] = constLocals[--numConstLocals];
                    return;
                }
            }
        }
    };

    
    void analyzeTypes(JSContext *cx, Bytecode *code, AnalyzeState &state);

    
    inline js::types::TypeObject *getTypeNewObject(JSContext *cx, JSProtoKey key);

    inline jsid getLocalId(unsigned index, Bytecode *code);
    inline jsid getArgumentId(unsigned index);

    inline types::TypeSet *getVariable(JSContext *cx, jsid id, bool localName = false);

    
    inline types::TypeSet *getStackTypes(unsigned index, Bytecode *code);

    inline JSValueType knownArgumentTypeTag(JSContext *cx, JSScript *script, unsigned arg);
    inline JSValueType knownLocalTypeTag(JSContext *cx, JSScript *script, unsigned local);

  private:
    void addVariable(JSContext *cx, jsid id, types::Variable *&var, bool localName);

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
      case JSOP_FILTER:
        return 2;
      default:
        return js_CodeSpec[*pc].ndefs;
    }
}



















struct Lifetime
{
    



    uint32 start;
    uint32 end;

    




    bool loopTail;

    
    Lifetime *next;

    Lifetime(uint32 offset, Lifetime *next)
        : start(offset), end(offset), loopTail(false), next(next)
    {}
};


struct LifetimeBytecode
{
    
    uint32 loopBackedge;

    
    mjit::RegisterAllocation *allocation;
};


struct LifetimeVariable
{
    
    Lifetime *lifetime;

    
    Lifetime *saved;

    
    uint32 savedEnd;

    Lifetime * live(uint32 offset) {
        if (lifetime && lifetime->end >= offset)
            return lifetime;
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= offset) {
            if (segment->end >= offset)
                return segment;
            segment = segment->next;
        }
        return NULL;
    }
};






class LifetimeScript
{
    analyze::Script *analysis;
    JSScript *script;
    JSFunction *fun;

    LifetimeBytecode *codeArray;
    LifetimeVariable *locals;
    LifetimeVariable *args;
    LifetimeVariable thisVar;

    LifetimeVariable **saved;
    unsigned savedCount;

  public:
    JSArenaPool pool;

    LifetimeScript();
    ~LifetimeScript();

    bool analyze(JSContext *cx, analyze::Script *analysis, JSScript *script, JSFunction *fun);

    LifetimeBytecode &getCode(uint32 offset) {
        JS_ASSERT(analysis->maybeCode(offset));
        return codeArray[offset];
    }
    LifetimeBytecode &getCode(jsbytecode *pc) { return getCode(pc - script->code); }

#ifdef DEBUG
    void dumpVariable(LifetimeVariable &var);
    void dumpLocal(unsigned i) { dumpVariable(locals[i]); }
    void dumpArg(unsigned i) { dumpVariable(args[i]); }
#endif

    Lifetime * argLive(uint32 arg, uint32 offset) { return args[arg].live(offset); }
    Lifetime * localLive(uint32 local, uint32 offset) { return locals[local].live(offset); }
    Lifetime * thisLive(uint32 offset) { return thisVar.live(offset); }

  private:

    inline bool addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset);
    inline void killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset);
    inline bool extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end);
};

} 
} 

#endif 
