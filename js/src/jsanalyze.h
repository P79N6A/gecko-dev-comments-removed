







































#ifndef jsanalyze_h___
#define jsanalyze_h___

#include "jsarena.h"
#include "jscompartment.h"
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

    
    uint32 stackDepth;

    



    uint32 defineCount;
    uint32 *defineArray;

    Bytecode()
    {
        PodZero(this);
    }

  private:
    bool mergeDefines(JSContext *cx, Script *script, bool initial,
                      uint32 newDepth, uint32 *newArray, uint32 newCount);

    
    bool isDefined(uint32 slot)
    {
        JS_ASSERT(analyzed);
        for (unsigned i = 0; i < defineCount; i++) {
            if (defineArray[i] == slot)
                return true;
        }
        return false;
    }
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
    bool usesThis;
    bool hasCalls;

    bool isInlineable;

    JSPackedBool *closedVars;
    JSPackedBool *closedArgs;

  public:
    
    JSArenaPool pool;

    Script();
    ~Script();

    void analyze(JSContext *cx, JSScript *script);

    bool OOM() { return outOfMemory; }
    bool failed() { return hadFailure; }
    bool inlineable(uint32 argc) { return isInlineable && argc == script->fun->nargs; }

    
    bool usesReturnValue() const { return usesRval; }

    
    bool usesScopeChain() const { return usesScope; }

    bool usesThisValue() const { return usesThis; }
    bool hasFunctionCalls() const { return hasCalls; }

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
        JS_ASSERT(script->fun && arg < script->fun->nargs);
        return script->usesEval || script->usesArguments || script->compartment->debugMode ||
            closedArgs[arg];
    }

    bool localEscapes(unsigned local)
    {
        return script->usesEval || script->compartment->debugMode || local >= localCount() ||
            closedVars[local];
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
      default:
        JS_NOT_REACHED("unrecognized op");
        return op;
    }
}


struct UntrapOpcode
{
    jsbytecode *pc;
    bool trap;

    UntrapOpcode(JSContext *cx, JSScript *script, jsbytecode *pc)
        : pc(pc), trap(JSOp(*pc) == JSOP_TRAP)
    {
        if (trap)
            *pc = JS_GetTrapOpcode(cx, script, pc);
    }

    ~UntrapOpcode()
    {
        if (trap)
            *pc = JSOP_TRAP;
    }
};



















struct Lifetime
{
    



    uint32 start;
    uint32 end;

    




    bool loopTail;

    



    bool write;

    
    Lifetime *next;

    Lifetime(uint32 offset, Lifetime *next)
        : start(offset), end(offset), loopTail(false), write(false), next(next)
    {}
};


struct LifetimeLoop
{
    
    LifetimeLoop *parent;

    
    uint32 head;

    



    uint32 backedge;

    
    uint32 entry;

    





    uint32 lastBlock;

    






    enum { UNASSIGNED = uint32(-1) };
    uint32 testLHS;
    uint32 testRHS;
    int32 testConstant;
    bool testLessEqual;

    




    struct Increment {
        uint32 slot;
        uint32 offset;
    };
    Increment *increments;
    uint32 nIncrements;

    
    bool unknownModset;

    



    bool hasSafePoints;

    
    bool hasCallsLoops;

    



    types::TypeObject **growArrays;
    uint32 nGrowArrays;
};


struct LifetimeBytecode
{
    
    LifetimeLoop *loop;

    
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

    



    uint32 firstWrite(LifetimeLoop *loop) {
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= loop->backedge) {
            if (segment->start >= loop->head && segment->write)
                return segment->start;
            segment = segment->next;
        }
        return uint32(-1);
    }

    



    uint32 onlyWrite(LifetimeLoop *loop) {
        uint32 offset = uint32(-1);
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= loop->backedge) {
            if (segment->start >= loop->head && segment->write) {
                if (offset != uint32(-1))
                    return uint32(-1);
                offset = segment->start;
            }
            segment = segment->next;
        }
        return offset;
    }
};






class LifetimeScript
{
    analyze::Script *analysis;
    JSScript *script;

    LifetimeBytecode *codeArray;
    LifetimeVariable *lifetimes;
    uint32 nLifetimes;

    LifetimeVariable **saved;
    unsigned savedCount;

  public:
    JSArenaPool pool;

    LifetimeScript();
    ~LifetimeScript();

    bool analyze(JSContext *cx, analyze::Script *analysis, JSScript *script);

    LifetimeBytecode &getCode(uint32 offset) {
        JS_ASSERT(analysis->maybeCode(offset));
        return codeArray[offset];
    }
    LifetimeBytecode &getCode(jsbytecode *pc) { return getCode(pc - script->code); }

#ifdef DEBUG
    void dumpVariable(LifetimeVariable &var);
    void dumpSlot(unsigned slot) {
        JS_ASSERT(slot < nLifetimes);
        dumpVariable(lifetimes[slot]);
    }
#endif

    Lifetime * live(uint32 slot, uint32 offset) {
        JS_ASSERT(slot < nLifetimes);
        return lifetimes[slot].live(offset);
    }

    uint32 firstWrite(uint32 slot, LifetimeLoop *loop) {
        JS_ASSERT(slot < nLifetimes);
        return lifetimes[slot].firstWrite(loop);
    }

    uint32 onlyWrite(uint32 slot, LifetimeLoop *loop) {
        JS_ASSERT(slot < nLifetimes);
        return lifetimes[slot].onlyWrite(loop);
    }

    



    void analyzeLoopTest(LifetimeLoop *loop);
    bool analyzeLoopIncrements(JSContext *cx, LifetimeLoop *loop);
    bool analyzeLoopModset(JSContext *cx, LifetimeLoop *loop);

  private:

    inline bool addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset);
    inline bool killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset);
    inline bool extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end);

    bool loopVariableAccess(LifetimeLoop *loop, jsbytecode *pc);
    bool getLoopTestAccess(jsbytecode *pc, uint32 *slotp, int32 *constantp);
};

} 
} 

#endif 
