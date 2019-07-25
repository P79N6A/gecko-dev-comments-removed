







































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

struct ClosedSlotArray {
    uint32_t        *vector;    
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

namespace js {
namespace mjit {
    struct JITScript;
    class CallCompiler;
}
}

#endif

namespace js {
namespace ion {
    struct IonScript;
}
}

namespace js {

namespace analyze { class ScriptAnalysis; }

class ScriptCounts
{
    friend struct ::JSScript;
    friend struct ScriptAndCounts;
    




    PCCounts *pcCountsVector;

 public:

    ScriptCounts() : pcCountsVector(NULL) {
    }

    inline void destroy(FreeOp *fop);

    void steal(ScriptCounts &other) {
        *this = other;
        js::PodZero(&other);
    }

    
    operator void*() const {
        return pcCountsVector;
    }
};

class DebugScript
{
    friend struct ::JSScript;

    






    uint32_t        stepMode;

    
    uint32_t        numSites;

    



    BreakpointSite  *breakpoints[1];
};

} 

static const uint32_t JS_SCRIPT_COOKIE = 0xc00cee;

struct JSScript : public js::gc::Cell
{
  private:
    static const uint32_t stepFlagMask = 0x80000000U;
    static const uint32_t stepCountMask = 0x7fffffffU;

  public:
    
    
    
    
    class JITScriptHandle
    {
        
        
        friend class js::mjit::CallCompiler;

        
        
        
        
        
        
        
        static const js::mjit::JITScript *UNJITTABLE;   
        js::mjit::JITScript *value;

      public:
        JITScriptHandle()       { value = NULL; }

        bool isEmpty()          { return value == NULL; }
        bool isUnjittable()     { return value == UNJITTABLE; }
        bool isValid()          { return value  > UNJITTABLE; }

        js::mjit::JITScript *getValid() {
            JS_ASSERT(isValid());
            return value;
        }

        void setEmpty()         { value = NULL; }
        void setUnjittable()    { value = const_cast<js::mjit::JITScript *>(UNJITTABLE); }
        void setValid(js::mjit::JITScript *jit) {
            value = jit;
            JS_ASSERT(isValid());
        }

        static void staticAsserts();
    };

    
    
    
    

    

  public:
    js::Bindings    bindings;   


    

  public:
    jsbytecode      *code;      
    uint8_t         *data;      


    const char      *filename;  
    js::HeapPtrAtom *atoms;     

    JSPrincipals    *principals;
    JSPrincipals    *originPrincipals; 

    jschar          *sourceMap; 

    










    js::HeapPtr<js::GlobalObject, JSScript*> globalObject;

    
    js::ScriptCounts scriptCounts;

    
    js::types::TypeScript *types;

  public:
#ifdef JS_METHODJIT
    JITScriptHandle jitHandleNormal; 
    JITScriptHandle jitHandleCtor;   
#endif

  private:
    js::DebugScript     *debug;
    js::HeapPtrFunction function_;

    size_t          useCount;   



    

  public:
    uint32_t        length;     

    uint32_t        lineno;     

    uint32_t        mainOffset; 


    uint32_t        natoms;     

#ifdef DEBUG
    
    
    uint32_t        id_;
  private:
    uint32_t        idpad;
  public:
#endif

    

  private:
    uint16_t        version;    

  public:
    uint16_t        nfixed;     


    uint16_t        nTypeSets;  


    uint16_t        nslots;     
    uint16_t        staticLevel;

    

  public:
    
    
    uint8_t         constsOffset;   
    uint8_t         objectsOffset;  


    uint8_t         regexpsOffset;  

    uint8_t         trynotesOffset; 
    uint8_t         globalsOffset;  
    uint8_t         closedArgsOffset; 
    uint8_t         closedVarsOffset; 

    

  public:
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
    static JSScript *NewScript(JSContext *cx, uint32_t length, uint32_t nsrcnotes, uint32_t natoms,
                               uint32_t nobjects, uint32_t nregexps,
                               uint32_t ntrynotes, uint32_t nconsts, uint32_t nglobals,
                               uint16_t nClosedArgs, uint16_t nClosedVars, uint32_t nTypeSets,
                               JSVersion version);
    static JSScript *NewScriptFromEmitter(JSContext *cx, js::BytecodeEmitter *bce);

    bool mayNeedArgsObj() const { return mayNeedArgsObj_; }
    bool analyzedArgsUsage() const { return analyzedArgsUsage_; }
    bool needsArgsObj() const { JS_ASSERT(analyzedArgsUsage()); return needsArgsObj_; }
    void setNeedsArgsObj(bool needsArgsObj);
    bool applySpeculationFailed(JSContext *cx);

    void setMayNeedArgsObj() {
        mayNeedArgsObj_ = true;
    }

    
    JSScript *&evalHashLink() { return *globalObject.unsafeGetUnioned(); }

    js::ion::IonScript *ion;          

    bool hasIonScript() const {
        return ion && ion != ION_DISABLED_SCRIPT;
    }
    bool canIonCompile() const {
        return ion != ION_DISABLED_SCRIPT;
    }
    js::ion::IonScript *ionScript() const {
        JS_ASSERT(hasIonScript());
        return ion;
    }

    



    JSFunction *function() const { return function_; }
    void setFunction(JSFunction *fun);

#ifdef DEBUG
    unsigned id();
#else
    unsigned id() { return 0; }
#endif

    
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

#ifdef JS_METHODJIT
  private:
    
    
    friend class js::mjit::CallCompiler;

    static size_t jitHandleOffset(bool constructing) {
        return constructing ? offsetof(JSScript, jitHandleCtor)
                            : offsetof(JSScript, jitHandleNormal);
    }

  public:
    bool hasJITCode()   { return jitHandleNormal.isValid() || jitHandleCtor.isValid(); }

    JITScriptHandle *jitHandle(bool constructing) {
        return constructing ? &jitHandleCtor : &jitHandleNormal;
    }

    js::mjit::JITScript *getJIT(bool constructing) {
        JITScriptHandle *jith = jitHandle(constructing);
        return jith->isValid() ? jith->getValid() : NULL;
    }

    static void ReleaseCode(js::FreeOp *fop, JITScriptHandle *jith);

    
    inline void **nativeMap(bool constructing);
    inline void *nativeCodeForPC(bool constructing, jsbytecode *pc);

    size_t getUseCount() const  { return useCount; }
    size_t incUseCount() { return ++useCount; }
    size_t *addressOfUseCount() { return &useCount; }
    void resetUseCount() { useCount = 0; }

    




    size_t sizeOfJitScripts(JSMallocSizeOfFun mallocSizeOf);
#endif

  public:
    js::PCCounts getPCCounts(jsbytecode *pc) {
        JS_ASSERT(size_t(pc - code) < length);
        return scriptCounts.pcCountsVector[pc - code];
    }

    bool initScriptCounts(JSContext *cx);
    void destroyScriptCounts(js::FreeOp *fop);

    jsbytecode *main() {
        return code + mainOffset;
    }

    




    size_t computedSizeOfData();
    size_t sizeOfData(JSMallocSizeOfFun mallocSizeOf);

    uint32_t numNotes();  

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    static const uint8_t INVALID_OFFSET = 0xFF;
    static bool isValidOffset(uint8_t offset) { return offset != INVALID_OFFSET; }

    JSConstArray *consts() {
        JS_ASSERT(isValidOffset(constsOffset));
        return reinterpret_cast<JSConstArray *>(data + constsOffset);
    }

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

    js::ClosedSlotArray *closedArgs() {
        JS_ASSERT(isValidOffset(closedArgsOffset));
        return reinterpret_cast<js::ClosedSlotArray *>(data + closedArgsOffset);
    }

    js::ClosedSlotArray *closedVars() {
        JS_ASSERT(isValidOffset(closedVarsOffset));
        return reinterpret_cast<js::ClosedSlotArray *>(data + closedVarsOffset);
    }

    uint32_t nClosedArgs() {
        return isValidOffset(closedArgsOffset) ? closedArgs()->length : 0;
    }

    uint32_t nClosedVars() {
        return isValidOffset(closedVarsOffset) ? closedVars()->length : 0;
    }

    js::HeapPtrAtom &getAtom(size_t index) const {
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
        js::ClosedSlotArray *arr = closedArgs();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    uint32_t getClosedVar(uint32_t index) {
        js::ClosedSlotArray *arr = closedVars();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

  private:
    



    void recompileForStepMode(js::FreeOp *fop);

    
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

    void destroyBreakpointSite(js::FreeOp *fop, jsbytecode *pc);

    void clearBreakpointsIn(js::FreeOp *fop, js::Debugger *dbg, JSObject *handler);
    void clearTraps(js::FreeOp *fop);

    void markTrapClosures(JSTracer *trc);

    





    bool setStepModeFlag(JSContext *cx, bool step);

    





    bool changeStepModeCount(JSContext *cx, int delta);

    bool stepModeEnabled() { return debug && !!debug->stepMode; }

#ifdef DEBUG
    uint32_t stepModeCount() { return debug ? (debug->stepMode & stepCountMask) : 0; }
#endif

    void finalize(js::FreeOp *fop);

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







extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

namespace js {

extern void
CallDestroyScriptHook(FreeOp *fop, JSScript *script);

extern const char *
SaveScriptFilename(JSContext *cx, const char *filename);

extern void
MarkScriptFilename(const char *filename);

extern void
SweepScriptFilenames(JSCompartment *comp);

extern void
FreeScriptFilenames(JSCompartment *comp);

struct ScriptAndCounts
{
    JSScript *script;
    ScriptCounts scriptCounts;

    PCCounts &getPCCounts(jsbytecode *pc) const {
        JS_ASSERT(unsigned(pc - script->code) < script->length);
        return scriptCounts.pcCountsVector[pc - script->code];
    }
};

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






template<XDRMode mode>
bool
XDRScript(XDRState<mode> *xdr, JSScript **scriptp, JSScript *parentScript);

} 

#endif 
