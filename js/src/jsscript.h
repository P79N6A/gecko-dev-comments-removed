







































#ifndef jsscript_h___
#define jsscript_h___



#include "jsatom.h"
#include "jsprvtd.h"
#include "jsdbgapi.h"
#include "jsclist.h"
#include "jsinfer.h"
#include "jsopcode.h"
#include "jsscope.h"

#include "gc/Barrier.h"

namespace js { namespace ion { struct IonScript; }}
# define ION_DISABLED_SCRIPT ((js::ion::IonScript *)0x1)





typedef enum JSTryNoteKind {
    JSTRY_CATCH,
    JSTRY_FINALLY,
    JSTRY_ITER
} JSTryNoteKind;




struct JSTryNote {
    uint8_t         kind;       
    uint8_t         padding;    
    uint16_t        stackDepth; 
    uint32_t        start;      

    uint32_t        length;     
};

typedef struct JSTryNoteArray {
    JSTryNote       *vector;    
    uint32_t        length;     
} JSTryNoteArray;

typedef struct JSObjectArray {
    js::HeapPtrObject *vector;  
    uint32_t        length;     
} JSObjectArray;

typedef struct JSConstArray {
    js::HeapValue   *vector;    
    uint32_t        length;
} JSConstArray;

namespace js {

struct GlobalSlotArray {
    struct Entry {
        uint32_t    atomIndex;  
        uint32_t    slot;       
    };
    Entry           *vector;
    uint32_t        length;
};

struct Shape;

enum BindingKind { NONE, ARGUMENT, VARIABLE, CONSTANT };







class Bindings
{
    HeapPtr<Shape> lastBinding;
    uint16_t nargs;
    uint16_t nvars;
    bool     hasDup_:1;     

    inline Shape *initialShape(JSContext *cx) const;
  public:
    inline Bindings(JSContext *cx);

    




    inline void transfer(JSContext *cx, Bindings *bindings);

    




    inline void clone(JSContext *cx, Bindings *bindings);

    uint16_t countArgs() const { return nargs; }
    uint16_t countVars() const { return nvars; }

    unsigned countLocalNames() const { return nargs + nvars; }

    bool hasLocalNames() const { return countLocalNames() > 0; }

    
    inline bool ensureShape(JSContext *cx);

    
    inline Shape *lastShape() const;

    



    Shape *callObjectShape(JSContext *cx) const;

    
    inline bool extensibleParents();
    bool setExtensibleParents(JSContext *cx);

    bool setParent(JSContext *cx, JSObject *obj);

    enum {
        
        BINDING_COUNT_LIMIT = 0xFFFF
    };

    














    bool add(JSContext *cx, JSAtom *name, BindingKind kind);

    
    bool addVariable(JSContext *cx, JSAtom *name) {
        return add(cx, name, VARIABLE);
    }
    bool addConstant(JSContext *cx, JSAtom *name) {
        return add(cx, name, CONSTANT);
    }
    bool addArgument(JSContext *cx, JSAtom *name, uint16_t *slotp) {
        JS_ASSERT(name != NULL); 
        *slotp = nargs;
        return add(cx, name, ARGUMENT);
    }
    bool addDestructuring(JSContext *cx, uint16_t *slotp) {
        *slotp = nargs;
        return add(cx, NULL, ARGUMENT);
    }

    void noteDup() { hasDup_ = true; }
    bool hasDup() const { return hasDup_; }

    





    BindingKind lookup(JSContext *cx, JSAtom *name, unsigned *indexp) const;

    
    bool hasBinding(JSContext *cx, JSAtom *name) const {
        return lookup(cx, name, NULL) != NONE;
    }

    








    bool getLocalNameArray(JSContext *cx, Vector<JSAtom *> *namesp);

    




    void makeImmutable();

    








    const js::Shape *lastArgument() const;
    const js::Shape *lastVariable() const;

    void trace(JSTracer *trc);

    
    struct StackRoot {
        RootShape root;
        StackRoot(JSContext *cx, Bindings *bindings)
            : root(cx, (Shape **) &bindings->lastBinding)
        {}
    };
};

} 

#define JS_OBJECT_ARRAY_SIZE(length)                                          \
    (offsetof(JSObjectArray, vector) + sizeof(JSObject *) * (length))

#ifdef JS_METHODJIT
namespace JSC {
    class ExecutablePool;
}

#define JS_UNJITTABLE_SCRIPT (reinterpret_cast<void*>(1))

namespace js { namespace mjit { struct JITScript; } }
namespace js { namespace ion { struct IonScript; } }
#endif

namespace js {
namespace ion {
    struct IonScript;
}
}

namespace js {

namespace analyze { class ScriptAnalysis; }

class ScriptOpcodeCounts
{
    friend struct ::JSScript;
    friend struct ScriptOpcodeCountsPair;
    OpcodeCounts *counts;

 public:

    ScriptOpcodeCounts() : counts(NULL) {
    }

    inline void destroy(JSContext *cx);

    void steal(ScriptOpcodeCounts &other) {
        *this = other;
        js::PodZero(&other);
    }

    
    operator void*() const {
        return counts;
    }
};

class DebugScript
{
    friend struct ::JSScript;

    






    uint32_t        stepMode;

    
    uint32_t        numSites;

    



    BreakpointSite  *breakpoints[1];
};






extern JSBool
XDRScript(JSXDRState *xdr, JSScript **scriptp);

} 

static const uint32_t JS_SCRIPT_COOKIE = 0xc00cee;

struct JSScript : public js::gc::Cell
{
    










    static JSScript *NewScript(JSContext *cx, uint32_t length, uint32_t nsrcnotes, uint32_t natoms,
                               uint32_t nobjects, uint32_t nregexps,
                               uint32_t ntrynotes, uint32_t nconsts, uint32_t nglobals,
                               uint16_t nClosedArgs, uint16_t nClosedVars, uint32_t nTypeSets,
                               JSVersion version);

    static JSScript *NewScriptFromEmitter(JSContext *cx, js::BytecodeEmitter *bce);

    friend JSBool js::XDRScript(JSXDRState *, JSScript **);

#ifdef JS_CRASH_DIAGNOSTICS
    



    uint32_t        cookie1[Cell::CellSize / sizeof(uint32_t)];
#endif
    jsbytecode      *code;      
    uint8_t         *data;      

    uint32_t        length;     
  private:
    uint16_t        version;    

  public:
    uint16_t        nfixed;     

    



    uint8_t         objectsOffset;  


    uint8_t         regexpsOffset;  

    uint8_t         trynotesOffset; 
    uint8_t         globalsOffset;  
    uint8_t         constOffset;    

    uint16_t        nTypeSets;      


    uint32_t        lineno;     

    uint32_t        mainOffset; 

    bool            noScriptRval:1; 

    bool            savedCallerFun:1; 
    bool            strictModeCode:1; 
    bool            compileAndGo:1;   
    bool            usesEval:1;       
    bool            warnedAboutTwoArgumentEval:1; 


    bool            warnedAboutUndefinedProp:1; 


    bool            hasSingletons:1;  
    bool            isOuterFunction:1; 
    bool            isInnerFunction:1; 

    bool            isActiveEval:1;   
    bool            isCachedEval:1;   
    bool            uninlineable:1;   
    bool            reentrantOuterFunction:1; 
    bool            typesPurged:1;    
#ifdef JS_METHODJIT
    bool            debugMode:1;      
    bool            failedBoundsCheck:1; 
#endif
    bool            callDestroyHook:1;

    











  private:
    bool            mayNeedArgsObj_:1;
    bool            analyzedArgsUsage_:1;
    bool            needsArgsObj_:1;
  public:
    bool mayNeedArgsObj() const { return mayNeedArgsObj_; }
    bool analyzedArgsUsage() const { return analyzedArgsUsage_; }
    bool needsArgsObj() const { JS_ASSERT(analyzedArgsUsage()); return needsArgsObj_; }
    void setNeedsArgsObj(bool needsArgsObj);
    bool applySpeculationFailed(JSContext *cx);

    uint32_t        natoms;     
    uint16_t        nslots;     
    uint16_t        staticLevel;

    uint16_t        nClosedArgs; 
    uint16_t        nClosedVars; 

    




#if JS_BITS_PER_WORD == 64
#define JS_SCRIPT_INLINE_DATA_LIMIT 4
    uint8_t         inlineData[JS_SCRIPT_INLINE_DATA_LIMIT];
#endif

    const char      *filename;  
    JSAtom          **atoms;    
  private:
    size_t          useCount;  


  public:
    js::Bindings    bindings;   

    JSPrincipals    *principals;
    JSPrincipals    *originPrincipals; 
    jschar          *sourceMap; 

    










    js::HeapPtr<js::GlobalObject, JSScript*> globalObject;

    
    JSScript        *&evalHashLink() { return *globalObject.unsafeGetUnioned(); }

    uint32_t        *closedSlots; 

    
    js::ScriptOpcodeCounts pcCounters;
	
    js::ion::IonScript *ion;          

    bool hasIonScript() const {
        return ion && ion != ION_DISABLED_SCRIPT;
    }
    js::ion::IonScript *ionScript() const {
        JS_ASSERT(hasIonScript());
        return ion;
    }

  private:
    js::DebugScript     *debug;
    js::HeapPtrFunction function_;
  public:

    



    JSFunction *function() const { return function_; }
    void setFunction(JSFunction *fun);

#ifdef JS_CRASH_DIAGNOSTICS
    
    uint32_t        cookie2[Cell::CellSize / sizeof(uint32_t)];

    void CheckScript(JSScript *prev);
#else
    void CheckScript(JSScript *prev) {}
#endif 

#ifdef DEBUG
    



    uint32_t id_;
    uint32_t idpad;
    unsigned id();
#else
    unsigned id() { return 0; }
#endif

    
    js::types::TypeScript *types;

    
    inline bool ensureHasTypes(JSContext *cx);

    





    inline bool ensureRanAnalysis(JSContext *cx, JSObject *scope);

    
    inline bool ensureRanInference(JSContext *cx);

    inline bool hasAnalysis();
    inline void clearAnalysis();
    inline js::analyze::ScriptAnalysis *analysis();

    



    bool typeSetFunction(JSContext *cx, JSFunction *fun, bool singleton = false);

    inline bool hasGlobal() const;
    inline bool hasClearedGlobal() const;

    inline js::GlobalObject *global() const;
    inline js::types::TypeScriptNesting *nesting() const;

    inline void clearNesting();

    
    js::GlobalObject *getGlobalObjectOrNull() const {
        return isCachedEval ? NULL : globalObject.get();
    }

  private:
    bool makeTypes(JSContext *cx);
    bool makeAnalysis(JSContext *cx);
  public:

#ifdef JS_METHODJIT
    
    
    
    
    void *jitArityCheckNormal;
    void *jitArityCheckCtor;

    js::mjit::JITScript *jitNormal;   
    js::mjit::JITScript *jitCtor;     
#endif

#ifdef JS_METHODJIT
    bool hasJITCode() {
        return jitNormal || jitCtor;
    }

    
    inline void **nativeMap(bool constructing);
    inline void *nativeCodeForPC(bool constructing, jsbytecode *pc);

    js::mjit::JITScript *getJIT(bool constructing) {
        return constructing ? jitCtor : jitNormal;
    }

    size_t getUseCount() const  { return useCount; }
    size_t incUseCount() { return ++useCount; }
    size_t *addressOfUseCount() { return &useCount; }
    void resetUseCount() { useCount = 0; }

    




    size_t sizeOfJitScripts(JSMallocSizeOfFun mallocSizeOf);

#endif

    
    js::OpcodeCounts getCounts(jsbytecode *pc) {
        JS_ASSERT(size_t(pc - code) < length);
        return pcCounters.counts[pc - code];
    }

    bool initCounts(JSContext *cx);
    void destroyCounts(JSContext *cx);

    jsbytecode *main() {
        return code + mainOffset;
    }

    




    size_t computedSizeOfData();
    size_t sizeOfData(JSMallocSizeOfFun mallocSizeOf);

    uint32_t numNotes();  

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    static const uint8_t INVALID_OFFSET = 0xFF;
    static bool isValidOffset(uint8_t offset) { return offset != INVALID_OFFSET; }

    JSObjectArray *objects() {
        JS_ASSERT(isValidOffset(objectsOffset));
        return reinterpret_cast<JSObjectArray *>(data + objectsOffset);
    }

    JSObjectArray *regexps() {
        JS_ASSERT(isValidOffset(regexpsOffset));
        return reinterpret_cast<JSObjectArray *>(data + regexpsOffset);
    }

    JSTryNoteArray *trynotes() {
        JS_ASSERT(isValidOffset(trynotesOffset));
        return reinterpret_cast<JSTryNoteArray *>(data + trynotesOffset);
    }

    js::GlobalSlotArray *globals() {
        JS_ASSERT(isValidOffset(globalsOffset));
        return reinterpret_cast<js::GlobalSlotArray *>(data + globalsOffset);
    }

    JSConstArray *consts() {
        JS_ASSERT(isValidOffset(constOffset));
        return reinterpret_cast<JSConstArray *>(data + constOffset);
    }

    JSAtom *getAtom(size_t index) {
        JS_ASSERT(index < natoms);
        return atoms[index];
    }

    js::PropertyName *getName(size_t index) {
        return getAtom(index)->asPropertyName();
    }

    JSObject *getObject(size_t index) {
        JSObjectArray *arr = objects();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    JSVersion getVersion() const {
        return JSVersion(version);
    }

    inline JSFunction *getFunction(size_t index);
    inline JSFunction *getCallerFunction();

    inline js::RegExpObject *getRegExp(size_t index);

    const js::Value &getConst(size_t index) {
        JSConstArray *arr = consts();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    




    inline bool isEmpty() const;

    uint32_t getClosedArg(uint32_t index) {
        JS_ASSERT(index < nClosedArgs);
        return closedSlots[index];
    }

    uint32_t getClosedVar(uint32_t index) {
        JS_ASSERT(index < nClosedVars);
        return closedSlots[nClosedArgs + index];
    }

    void copyClosedSlotsTo(JSScript *other);

  private:
    static const uint32_t stepFlagMask = 0x80000000U;
    static const uint32_t stepCountMask = 0x7fffffffU;

    



    bool recompileForStepMode(JSContext *cx);

    
    bool tryNewStepMode(JSContext *cx, uint32_t newValue);

    bool ensureHasDebug(JSContext *cx);

  public:
    bool hasBreakpointsAt(jsbytecode *pc) { return !!getBreakpointSite(pc); }
    bool hasAnyBreakpointsOrStepMode() { return !!debug; }

    js::BreakpointSite *getBreakpointSite(jsbytecode *pc)
    {
        JS_ASSERT(size_t(pc - code) < length);
        return debug ? debug->breakpoints[pc - code] : NULL;
    }

    js::BreakpointSite *getOrCreateBreakpointSite(JSContext *cx, jsbytecode *pc,
                                                  js::GlobalObject *scriptGlobal);

    void destroyBreakpointSite(JSRuntime *rt, jsbytecode *pc);

    void clearBreakpointsIn(JSContext *cx, js::Debugger *dbg, JSObject *handler);
    void clearTraps(JSContext *cx);

    void markTrapClosures(JSTracer *trc);

    





    bool setStepModeFlag(JSContext *cx, bool step);

    





    bool changeStepModeCount(JSContext *cx, int delta);

    bool stepModeEnabled() { return debug && !!debug->stepMode; }

#ifdef DEBUG
    uint32_t stepModeCount() { return debug ? (debug->stepMode & stepCountMask) : 0; }
#endif

    void finalize(JSContext *cx, bool background);

    static inline void writeBarrierPre(JSScript *script);
    static inline void writeBarrierPost(JSScript *script, void *addr);

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SCRIPT; }

    static JSPrincipals *normalizeOriginPrincipals(JSPrincipals *principals,
                                                   JSPrincipals *originPrincipals) {
        return originPrincipals ? originPrincipals : principals;
    }

    void markChildren(JSTracer *trc);
};


JS_STATIC_ASSERT(sizeof(JSScript) % js::gc::Cell::CellSize == 0);

static JS_INLINE unsigned
StackDepth(JSScript *script)
{
    return script->nslots - script->nfixed;
}

extern void
js_MarkScriptFilename(const char *filename);

extern void
js_SweepScriptFilenames(JSCompartment *comp);







extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

extern void
js_CallDestroyScriptHook(JSContext *cx, JSScript *script);

namespace js {

struct ScriptOpcodeCountsPair
{
    JSScript *script;
    ScriptOpcodeCounts counters;

    OpcodeCounts &getCounts(jsbytecode *pc) const {
        JS_ASSERT(unsigned(pc - script->code) < script->length);
        return counters.counts[pc - script->code];
    }
};

#ifdef JS_CRASH_DIAGNOSTICS

void
CheckScript(JSScript *script, JSScript *prev);

#else

inline void
CheckScript(JSScript *script, JSScript *prev)
{
}

#endif 

} 






#define js_GetSrcNote(script,pc) js_GetSrcNoteCached(cx, script, pc)

extern jssrcnote *
js_GetSrcNoteCached(JSContext *cx, JSScript *script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(JSScript *script, unsigned lineno);

extern JS_FRIEND_API(unsigned)
js_GetScriptLineExtent(JSScript *script);

namespace js {

extern unsigned
PCToLineNumber(JSScript *script, jsbytecode *pc);

extern unsigned
PCToLineNumber(unsigned startLine, jssrcnote *notes, jsbytecode *code, jsbytecode *pc);

extern unsigned
CurrentLine(JSContext *cx);










enum LineOption {
    CALLED_FROM_JSOP_EVAL,
    NOT_CALLED_FROM_JSOP_EVAL
};

inline void
CurrentScriptFileLineOrigin(JSContext *cx, unsigned *linenop, LineOption = NOT_CALLED_FROM_JSOP_EVAL);

extern JSScript *
CloneScript(JSContext *cx, JSScript *script);

}

#endif
