






#ifndef jsscript_h___
#define jsscript_h___




#include "jsdbgapi.h"
#include "jsinfer.h"
#include "jsopcode.h"

#include "gc/Barrier.h"
#include "js/RootingAPI.h"
#include "vm/Shape.h"

ForwardDeclareJS(Script);

namespace js {

namespace ion {
    struct IonScript;
    struct IonScriptCounts;
}

# define ION_DISABLED_SCRIPT ((js::ion::IonScript *)0x1)
# define ION_COMPILING_SCRIPT ((js::ion::IonScript *)0x2)

class Shape;

class BindingIter;

namespace mjit {
    struct JITScript;
    class CallCompiler;
}

namespace analyze {
    class ScriptAnalysis;
}

}





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

namespace js {

struct ConstArray {
    js::HeapValue   *vector;    
    uint32_t        length;
};

struct ObjectArray {
    js::HeapPtrObject *vector;  
    uint32_t        length;     
};

struct TryNoteArray {
    JSTryNote       *vector;    
    uint32_t        length;     
};






enum BindingKind { ARGUMENT, VARIABLE, CONSTANT };

class Binding
{
    



    uintptr_t bits_;

    static const uintptr_t KIND_MASK = 0x3;
    static const uintptr_t ALIASED_BIT = 0x4;
    static const uintptr_t NAME_MASK = ~(KIND_MASK | ALIASED_BIT);

  public:
    explicit Binding() : bits_(0) {}

    Binding(PropertyName *name, BindingKind kind, bool aliased) {
        JS_STATIC_ASSERT(CONSTANT <= KIND_MASK);
        JS_ASSERT((uintptr_t(name) & ~NAME_MASK) == 0);
        JS_ASSERT((uintptr_t(kind) & ~KIND_MASK) == 0);
        bits_ = uintptr_t(name) | uintptr_t(kind) | (aliased ? ALIASED_BIT : 0);
    }

    PropertyName *name() const {
        return (PropertyName *)(bits_ & NAME_MASK);
    }

    BindingKind kind() const {
        return BindingKind(bits_ & KIND_MASK);
    }

    bool aliased() const {
        return bool(bits_ & ALIASED_BIT);
    }
};

JS_STATIC_ASSERT(sizeof(Binding) == sizeof(uintptr_t));

class Bindings;
typedef InternalHandle<Bindings *> InternalBindingsHandle;







class Bindings
{
    friend class BindingIter;
    friend class AliasedFormalIter;

    HeapPtr<Shape> callObjShape_;
    uintptr_t bindingArrayAndFlag_;
    uint16_t numArgs_;
    uint16_t numVars_;

    








    static const uintptr_t TEMPORARY_STORAGE_BIT = 0x1;
    bool bindingArrayUsingTemporaryStorage() const {
        return bindingArrayAndFlag_ & TEMPORARY_STORAGE_BIT;
    }
    Binding *bindingArray() const {
        return reinterpret_cast<Binding *>(bindingArrayAndFlag_ & ~TEMPORARY_STORAGE_BIT);
    }

  public:
    inline Bindings();

    





    static bool initWithTemporaryStorage(JSContext *cx, InternalBindingsHandle self,
                                         unsigned numArgs, unsigned numVars,
                                         Binding *bindingArray);

    uint8_t *switchToScriptStorage(Binding *newStorage);

    



    static bool clone(JSContext *cx, InternalBindingsHandle self, uint8_t *dstScriptData,
                      HandleScript srcScript);

    unsigned numArgs() const { return numArgs_; }
    unsigned numVars() const { return numVars_; }
    unsigned count() const { return numArgs() + numVars(); }

    
    RawShape callObjShape() const { return callObjShape_; }

    
    static unsigned argumentsVarIndex(JSContext *cx, InternalBindingsHandle);

    
    bool bindingIsAliased(unsigned bindingIndex);

    
    bool hasAnyAliasedBindings() const { return !callObjShape_->isEmptyShape(); }

    void trace(JSTracer *trc);
};

template <>
struct RootMethods<Bindings> {
    static Bindings initial();
    static ThingRootKind kind() { return THING_ROOT_BINDINGS; }
    static bool poisoned(const Bindings &bindings) {
        return IsPoisonedPtr(static_cast<RawShape>(bindings.callObjShape()));
    }
};

class ScriptCounts
{
    friend class ::JSScript;
    friend struct ScriptAndCounts;

    




    PCCounts *pcCountsVector;

    
    ion::IonScriptCounts *ionCounts;

 public:
    ScriptCounts() : pcCountsVector(NULL), ionCounts(NULL) { }

    inline void destroy(FreeOp *fop);

    void set(js::ScriptCounts counts) {
        pcCountsVector = counts.pcCountsVector;
        ionCounts = counts.ionCounts;
    }
};

typedef HashMap<RawScript,
                ScriptCounts,
                DefaultHasher<RawScript>,
                SystemAllocPolicy> ScriptCountsMap;

class DebugScript
{
    friend class ::JSScript;

    






    uint32_t        stepMode;

    
    uint32_t        numSites;

    



    BreakpointSite  *breakpoints[1];
};

typedef HashMap<RawScript,
                DebugScript *,
                DefaultHasher<RawScript>,
                SystemAllocPolicy> DebugScriptMap;

struct ScriptSource;

} 

class JSScript : public js::gc::Cell
{
    static const uint32_t stepFlagMask = 0x80000000U;
    static const uint32_t stepCountMask = 0x7fffffffU;

  public:
#ifdef JS_METHODJIT
    
    
    
    
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

    
    struct JITScriptSet
    {
        JITScriptHandle jitHandleNormal;          
        JITScriptHandle jitHandleNormalBarriered; 
        JITScriptHandle jitHandleCtor;            
        JITScriptHandle jitHandleCtorBarriered;   

        static size_t jitHandleOffset(bool constructing, bool barriers) {
            return constructing
                ? (barriers
                   ? offsetof(JITScriptSet, jitHandleCtorBarriered)
                   : offsetof(JITScriptSet, jitHandleCtor))
                : (barriers
                   ? offsetof(JITScriptSet, jitHandleNormalBarriered)
                   : offsetof(JITScriptSet, jitHandleNormal));
        }
    };

#endif  

    
    
    
    

    

  public:
    js::Bindings    bindings;   


    

  public:
    jsbytecode      *code;      
    uint8_t         *data;      


    js::HeapPtrAtom *atoms;     

    JSCompartment   *compartment_;
    JSPrincipals    *originPrincipals; 

    
    js::types::TypeScript *types;

  private:
    js::ScriptSource *scriptSource_; 
#ifdef JS_METHODJIT
    JITScriptSet *mJITInfo;
#else
    void         *mJITInfoPad;
#endif
    js::HeapPtrFunction function_;

    
    
    js::HeapPtrObject   enclosingScopeOrOriginalFunction_;

  public:
    
    js::HeapPtr<JSObject> asmJS;

#if JS_BYTES_PER_WORD == 4
    uint32_t        PADDING32;
#endif

    

  public:
    uint32_t        length;     

    uint32_t        dataSize;   

    uint32_t        lineno;     

    uint32_t        mainOffset; 


    uint32_t        natoms;     

    uint32_t        sourceStart;
    uint32_t        sourceEnd;

  private:
    uint32_t        useCount;   



    uint32_t        maxLoopCount; 
    uint32_t        loopCount;    


#ifdef DEBUG
    
    
    uint32_t        id_;
  private:
    uint32_t        idpad;
#endif

    

  private:
    uint16_t        PADDING16;

    uint16_t        version;    

  public:
    uint16_t        ndefaults;  

    uint16_t        nfixed;     


    uint16_t        nTypeSets;  


    uint16_t        nslots;     
    uint16_t        staticLevel;

    

  public:
    
    enum ArrayKind {
        CONSTS,
        OBJECTS,
        REGEXPS,
        TRYNOTES,
        LIMIT
    };

    typedef uint8_t ArrayBitsT;

  private:
    
    
    ArrayBitsT      hasArrayBits;

    

  public:
    bool            noScriptRval:1; 

    bool            savedCallerFun:1; 
    bool            strict:1; 
    bool            explicitUseStrict:1; 
    bool            compileAndGo:1;   
    bool            selfHosted:1;     
    bool            bindingsAccessedDynamically:1; 
    bool            funHasExtensibleScope:1;       
    bool            funHasAnyAliasedFormal:1;      
    bool            warnedAboutTwoArgumentEval:1; 


    bool            warnedAboutUndefinedProp:1; 


    bool            hasSingletons:1;  
    bool            treatAsRunOnce:1; 
    bool            hasBeenCloned:1;  
    bool            isActiveEval:1;   
    bool            isCachedEval:1;   
    bool            uninlineable:1;   

    



    bool            shouldCloneAtCallsite:1;

    bool            isCallsiteClone:1; 
#ifdef JS_METHODJIT
    bool            debugMode:1;      
    bool            failedBoundsCheck:1; 
#else
    bool            debugModePad:1;
    bool            failedBoundsCheckPad:1;
#endif
#ifdef JS_ION
    bool            failedShapeGuard:1; 
#else
    bool            failedShapeGuardPad:1;
#endif
    bool            invalidatedIdempotentCache:1; 
    bool            isGenerator:1;    
    bool            isGeneratorExp:1; 
    bool            hasScriptCounts:1;

    bool            hasDebugScript:1; 

    bool            hasFreezeConstraints:1; 

    bool            userBit:1; 

  private:
    
    bool            argsHasVarBinding_:1;
    bool            needsArgsAnalysis_:1;
    bool            needsArgsObj_:1;

    
    
    

  public:
    static js::RawScript Create(JSContext *cx, js::HandleObject enclosingScope, bool savedCallerFun,
                                const JS::CompileOptions &options, unsigned staticLevel,
                                js::ScriptSource *ss, uint32_t sourceStart, uint32_t sourceEnd);

    
    
    
    
    static bool partiallyInit(JSContext *cx, JS::Handle<JSScript*> script,
                              uint32_t nobjects, uint32_t nregexps,
                              uint32_t ntrynotes, uint32_t nconsts, uint32_t nTypeSets);
    static bool fullyInitTrivial(JSContext *cx, JS::Handle<JSScript*> script);  
    static bool fullyInitFromEmitter(JSContext *cx, JS::Handle<JSScript*> script,
                                     js::frontend::BytecodeEmitter *bce);

    inline JSPrincipals *principals();

    JSCompartment *compartment() const { return compartment_; }

    void setVersion(JSVersion v) { version = v; }

    
    bool argumentsHasVarBinding() const { return argsHasVarBinding_; }
    jsbytecode *argumentsBytecode() const { JS_ASSERT(code[0] == JSOP_ARGUMENTS); return code; }
    void setArgumentsHasVarBinding();

    









    bool analyzedArgsUsage() const { return !needsArgsAnalysis_; }
    bool needsArgsObj() const { JS_ASSERT(analyzedArgsUsage()); return needsArgsObj_; }
    void setNeedsArgsObj(bool needsArgsObj);
    static bool argumentsOptimizationFailed(JSContext *cx, js::HandleScript script);

    








    bool argsObjAliasesFormals() const {
        return needsArgsObj() && !strict;
    }

    bool hasAnyIonScript() const {
        return hasIonScript() || hasParallelIonScript();
    }

    
    js::ion::IonScript *ion;

    bool hasIonScript() const {
        return ion && ion != ION_DISABLED_SCRIPT && ion != ION_COMPILING_SCRIPT;
    }

    bool canIonCompile() const {
        return ion != ION_DISABLED_SCRIPT;
    }

    bool isIonCompilingOffThread() const {
        return ion == ION_COMPILING_SCRIPT;
    }

    js::ion::IonScript *ionScript() const {
        JS_ASSERT(hasIonScript());
        return ion;
    }

    
    js::ion::IonScript *parallelIon;

    bool hasParallelIonScript() const {
        return parallelIon && parallelIon != ION_DISABLED_SCRIPT && parallelIon != ION_COMPILING_SCRIPT;
    }

    bool canParallelIonCompile() const {
        return parallelIon != ION_DISABLED_SCRIPT;
    }

    bool isParallelIonCompilingOffThread() const {
        return parallelIon == ION_COMPILING_SCRIPT;
    }

    js::ion::IonScript *parallelIonScript() const {
        JS_ASSERT(hasParallelIonScript());
        return parallelIon;
    }

    



    JSFunction *function() const { return function_; }
    void setFunction(JSFunction *fun);

    JSFunction *originalFunction() const;
    void setOriginalFunctionObject(JSObject *fun);

    JSFlatString *sourceData(JSContext *cx);

    static bool loadSource(JSContext *cx, js::HandleScript scr, bool *worked);

    js::ScriptSource *scriptSource() const {
        return scriptSource_;
    }

    void setScriptSource(js::ScriptSource *ss);

    inline const char *filename() const;

  public:

    
    bool isForEval() { return isCachedEval || isActiveEval; }

#ifdef DEBUG
    unsigned id();
#else
    unsigned id() { return 0; }
#endif

    
    inline bool ensureHasTypes(JSContext *cx);

    



    inline bool ensureRanAnalysis(JSContext *cx);

    
    inline bool ensureRanInference(JSContext *cx);

    inline bool hasAnalysis();
    inline void clearAnalysis();
    inline js::analyze::ScriptAnalysis *analysis();

    
    bool isShortRunning();

    inline void clearPropertyReadTypes();

    inline js::GlobalObject &global() const;

    
    JSObject *enclosingStaticScope() const {
        if (isCallsiteClone)
            return NULL;
        return enclosingScopeOrOriginalFunction_;
    }

    









    bool enclosingScriptsCompiledSuccessfully() const;

  private:
    bool makeTypes(JSContext *cx);
    bool makeAnalysis(JSContext *cx);

#ifdef JS_METHODJIT
  private:
    
    
    friend class js::mjit::CallCompiler;

  public:
    bool hasMJITInfo() {
        return mJITInfo != NULL;
    }

    static size_t offsetOfMJITInfo() { return offsetof(JSScript, mJITInfo); }

    inline bool ensureHasMJITInfo(JSContext *cx);
    inline void destroyMJITInfo(js::FreeOp *fop);

    JITScriptHandle *jitHandle(bool constructing, bool barriers) {
        JS_ASSERT(mJITInfo);
        return constructing
               ? (barriers ? &mJITInfo->jitHandleCtorBarriered : &mJITInfo->jitHandleCtor)
               : (barriers ? &mJITInfo->jitHandleNormalBarriered : &mJITInfo->jitHandleNormal);
    }

    js::mjit::JITScript *getJIT(bool constructing, bool barriers) {
        if (!mJITInfo)
            return NULL;
        JITScriptHandle *jith = jitHandle(constructing, barriers);
        return jith->isValid() ? jith->getValid() : NULL;
    }

    static void ReleaseCode(js::FreeOp *fop, JITScriptHandle *jith);

    
    inline void **nativeMap(bool constructing);
    inline void *nativeCodeForPC(bool constructing, jsbytecode *pc);

    uint32_t getUseCount() const  { return useCount; }
    uint32_t incUseCount(uint32_t amount = 1) { return useCount += amount; }
    uint32_t *addressOfUseCount() { return &useCount; }
    void resetUseCount() { useCount = 0; }

    void resetLoopCount() {
        if (loopCount > maxLoopCount)
            maxLoopCount = loopCount;
        loopCount = 0;
    }

    void incrLoopCount() {
        ++loopCount;
    }

    uint32_t getMaxLoopCount() {
        if (loopCount > maxLoopCount)
            maxLoopCount = loopCount;
        return maxLoopCount;
    }

    




    size_t sizeOfJitScripts(JSMallocSizeOfFun mallocSizeOf);
#endif

  public:
    bool initScriptCounts(JSContext *cx);
    js::PCCounts getPCCounts(jsbytecode *pc);
    void addIonCounts(js::ion::IonScriptCounts *ionCounts);
    js::ion::IonScriptCounts *getIonCounts();
    js::ScriptCounts releaseScriptCounts();
    void destroyScriptCounts(js::FreeOp *fop);

    jsbytecode *main() {
        return code + mainOffset;
    }

    




    size_t computedSizeOfData();
    size_t sizeOfData(JSMallocSizeOfFun mallocSizeOf);

    uint32_t numNotes();  

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    bool hasArray(ArrayKind kind)           { return (hasArrayBits & (1 << kind)); }
    void setHasArray(ArrayKind kind)        { hasArrayBits |= (1 << kind); }
    void cloneHasArray(js::RawScript script) { hasArrayBits = script->hasArrayBits; }

    bool hasConsts()        { return hasArray(CONSTS);      }
    bool hasObjects()       { return hasArray(OBJECTS);     }
    bool hasRegexps()       { return hasArray(REGEXPS);     }
    bool hasTrynotes()      { return hasArray(TRYNOTES);    }

    #define OFF(fooOff, hasFoo, t)   (fooOff() + (hasFoo() ? sizeof(t) : 0))

    size_t constsOffset()     { return 0; }
    size_t objectsOffset()    { return OFF(constsOffset,     hasConsts,     js::ConstArray);      }
    size_t regexpsOffset()    { return OFF(objectsOffset,    hasObjects,    js::ObjectArray);     }
    size_t trynotesOffset()   { return OFF(regexpsOffset,    hasRegexps,    js::ObjectArray);     }

    js::ConstArray *consts() {
        JS_ASSERT(hasConsts());
        return reinterpret_cast<js::ConstArray *>(data + constsOffset());
    }

    js::ObjectArray *objects() {
        JS_ASSERT(hasObjects());
        return reinterpret_cast<js::ObjectArray *>(data + objectsOffset());
    }

    js::ObjectArray *regexps() {
        JS_ASSERT(hasRegexps());
        return reinterpret_cast<js::ObjectArray *>(data + regexpsOffset());
    }

    js::TryNoteArray *trynotes() {
        JS_ASSERT(hasTrynotes());
        return reinterpret_cast<js::TryNoteArray *>(data + trynotesOffset());
    }

    js::HeapPtrAtom &getAtom(size_t index) const {
        JS_ASSERT(index < natoms);
        return atoms[index];
    }

    js::HeapPtrAtom &getAtom(jsbytecode *pc) const {
        JS_ASSERT(pc >= code && pc + sizeof(uint32_t) < code + length);
        return getAtom(GET_UINT32_INDEX(pc));
    }

    js::PropertyName *getName(size_t index) {
        return getAtom(index)->asPropertyName();
    }

    js::PropertyName *getName(jsbytecode *pc) const {
        JS_ASSERT(pc >= code && pc + sizeof(uint32_t) < code + length);
        return getAtom(GET_UINT32_INDEX(pc))->asPropertyName();
    }

    JSObject *getObject(size_t index) {
        js::ObjectArray *arr = objects();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    JSObject *getObject(jsbytecode *pc) {
        JS_ASSERT(pc >= code && pc + sizeof(uint32_t) < code + length);
        return getObject(GET_UINT32_INDEX(pc));
    }

    JSVersion getVersion() const {
        return JSVersion(version);
    }

    inline JSFunction *getFunction(size_t index);
    inline JSFunction *getCallerFunction();
    inline JSFunction *functionOrCallerFunction();

    inline js::RegExpObject *getRegExp(size_t index);

    const js::Value &getConst(size_t index) {
        js::ConstArray *arr = consts();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    




    inline bool isEmpty() const;

    bool varIsAliased(unsigned varSlot);
    bool formalIsAliased(unsigned argSlot);
    bool formalLivesInArgumentsObject(unsigned argSlot);

  private:
    



    void recompileForStepMode(js::FreeOp *fop);

    
    bool tryNewStepMode(JSContext *cx, uint32_t newValue);

    bool ensureHasDebugScript(JSContext *cx);
    js::DebugScript *debugScript();
    js::DebugScript *releaseDebugScript();
    void destroyDebugScript(js::FreeOp *fop);

  public:
    bool hasBreakpointsAt(jsbytecode *pc);
    bool hasAnyBreakpointsOrStepMode() { return hasDebugScript; }

    js::BreakpointSite *getBreakpointSite(jsbytecode *pc)
    {
        JS_ASSERT(size_t(pc - code) < length);
        return hasDebugScript ? debugScript()->breakpoints[pc - code] : NULL;
    }

    js::BreakpointSite *getOrCreateBreakpointSite(JSContext *cx, jsbytecode *pc);

    void destroyBreakpointSite(js::FreeOp *fop, jsbytecode *pc);

    void clearBreakpointsIn(js::FreeOp *fop, js::Debugger *dbg, js::RawObject handler);
    void clearTraps(js::FreeOp *fop);

    void markTrapClosures(JSTracer *trc);

    





    bool setStepModeFlag(JSContext *cx, bool step);

    





    bool changeStepModeCount(JSContext *cx, int delta);

    bool stepModeEnabled() { return hasDebugScript && !!debugScript()->stepMode; }

#ifdef DEBUG
    uint32_t stepModeCount() { return hasDebugScript ? (debugScript()->stepMode & stepCountMask) : 0; }
#endif

    void finalize(js::FreeOp *fop);

    JS::Zone *zone() const { return tenuredZone(); }

    static inline void writeBarrierPre(js::RawScript script);
    static inline void writeBarrierPost(js::RawScript script, void *addr);

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SCRIPT; }

    static JSPrincipals *normalizeOriginPrincipals(JSPrincipals *principals,
                                                   JSPrincipals *originPrincipals) {
        return originPrincipals ? originPrincipals : principals;
    }

    void markChildren(JSTracer *trc);
};

JS_STATIC_ASSERT(sizeof(JSScript::ArrayBitsT) * 8 >= JSScript::LIMIT);


JS_STATIC_ASSERT(sizeof(JSScript) % js::gc::CellSize == 0);

namespace js {







class BindingIter
{
    const InternalBindingsHandle bindings_;
    unsigned i_;

    friend class Bindings;

  public:
    explicit BindingIter(const InternalBindingsHandle &bindings) : bindings_(bindings), i_(0) {}
    explicit BindingIter(const HandleScript &script) : bindings_(script, &script->bindings), i_(0) {}

    bool done() const { return i_ == bindings_->count(); }
    operator bool() const { return !done(); }
    void operator++(int) { JS_ASSERT(!done()); i_++; }
    BindingIter &operator++() { (*this)++; return *this; }

    unsigned frameIndex() const {
        JS_ASSERT(!done());
        return i_ < bindings_->numArgs() ? i_ : i_ - bindings_->numArgs();
    }

    const Binding &operator*() const { JS_ASSERT(!done()); return bindings_->bindingArray()[i_]; }
    const Binding *operator->() const { JS_ASSERT(!done()); return &bindings_->bindingArray()[i_]; }
};






typedef Vector<Binding, 32> BindingVector;

extern bool
FillBindingVector(HandleScript fromScript, BindingVector *vec);






class AliasedFormalIter
{
    const Binding *begin_, *p_, *end_;
    unsigned slot_;

    void settle() {
        while (p_ != end_ && !p_->aliased())
            p_++;
    }

  public:
    explicit inline AliasedFormalIter(js::RawScript script);

    bool done() const { return p_ == end_; }
    operator bool() const { return !done(); }
    void operator++(int) { JS_ASSERT(!done()); p_++; slot_++; settle(); }

    const Binding &operator*() const { JS_ASSERT(!done()); return *p_; }
    const Binding *operator->() const { JS_ASSERT(!done()); return p_; }
    unsigned frameIndex() const { JS_ASSERT(!done()); return p_ - begin_; }
    unsigned scopeSlot() const { JS_ASSERT(!done()); return slot_; }
};

struct SourceCompressionToken;

struct ScriptSource
{
    friend class SourceCompressorThread;
  private:
    union {
        
        
        
        
        
        
        
        
        jschar *source;
        unsigned char *compressed;
    } data;
    uint32_t refs;
    uint32_t length_;
    uint32_t compressedLength_;
    char *filename_;
    jschar *sourceMap_;

    
    
    
    bool sourceRetrievable_:1;
    bool argumentsNotIncluded_:1;
    bool ready_:1;

  public:
    ScriptSource()
      : refs(0),
        length_(0),
        compressedLength_(0),
        filename_(NULL),
        sourceMap_(NULL),
        sourceRetrievable_(false),
        argumentsNotIncluded_(false),
        ready_(true)
    {
        data.source = NULL;
    }
    void incref() { refs++; }
    void decref() {
        JS_ASSERT(refs != 0);
        if (--refs == 0)
            destroy();
    }
    bool setSourceCopy(JSContext *cx,
                       const jschar *src,
                       uint32_t length,
                       bool argumentsNotIncluded,
                       SourceCompressionToken *tok);
    void setSource(const jschar *src, uint32_t length);
    bool ready() const { return ready_; }
    void setSourceRetrievable() { sourceRetrievable_ = true; }
    bool sourceRetrievable() const { return sourceRetrievable_; }
    bool hasSourceData() const { return !!data.source || !ready(); }
    uint32_t length() const {
        JS_ASSERT(hasSourceData());
        return length_;
    }
    bool argumentsNotIncluded() const {
        JS_ASSERT(hasSourceData());
        return argumentsNotIncluded_;
    }
    JSFlatString *substring(JSContext *cx, uint32_t start, uint32_t stop);
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf);

    
    template <XDRMode mode>
    bool performXDR(XDRState<mode> *xdr);

    bool setFilename(JSContext *cx, const char *filename);
    const char *filename() const {
        return filename_;
    }

    
    bool setSourceMap(JSContext *cx, jschar *sourceMapURL, const char *filename);
    const jschar *sourceMap();
    bool hasSourceMap() const { return sourceMap_ != NULL; }

  private:
    void destroy();
    bool compressed() const { return compressedLength_ != 0; }
    size_t computedSizeOfData() const {
        return compressed() ? compressedLength_ : sizeof(jschar) * length_;
    }
    bool adjustDataSize(size_t nbytes);
};

class ScriptSourceHolder
{
    ScriptSource *ss;
  public:
    explicit ScriptSourceHolder(ScriptSource *ss)
      : ss(ss)
    {
        ss->incref();
    }
    ~ScriptSourceHolder()
    {
        ss->decref();
    }
};

#ifdef JS_THREADSAFE












class SourceCompressorThread
{
  private:
    enum {
        
        COMPRESSING,
        
        
        IDLE,
        
        SHUTDOWN
    } state;
    SourceCompressionToken *tok;
    PRThread *thread;
    
    PRLock *lock;
    
    
    
    PRCondVar *wakeup;
    
    PRCondVar *done;
    
    volatile bool stop;

    bool internalCompress();
    void threadLoop();
    static void compressorThread(void *arg);

  public:
    explicit SourceCompressorThread()
    : state(IDLE),
      tok(NULL),
      thread(NULL),
      lock(NULL),
      wakeup(NULL),
      done(NULL) {}
    void finish();
    bool init();
    void compress(SourceCompressionToken *tok);
    void waitOnCompression(SourceCompressionToken *userTok);
    void abort(SourceCompressionToken *userTok);
    const jschar *currentChars() const;
};
#endif

struct SourceCompressionToken
{
    friend struct ScriptSource;
    friend class SourceCompressorThread;
  private:
    JSContext *cx;
    ScriptSource *ss;
    const jschar *chars;
    bool oom;
  public:
    explicit SourceCompressionToken(JSContext *cx)
       : cx(cx), ss(NULL), chars(NULL), oom(false) {}
    ~SourceCompressionToken()
    {
        complete();
    }

    bool complete();
    void abort();
    bool active() const { return !!ss; }
};








extern void
CallNewScriptHook(JSContext *cx, JS::HandleScript script, JS::HandleFunction fun);

extern void
CallDestroyScriptHook(FreeOp *fop, js::RawScript script);

struct SharedScriptData
{
    bool marked;
    uint32_t length;
    jsbytecode data[1];

    static SharedScriptData *new_(JSContext *cx, uint32_t codeLength,
                                             uint32_t srcnotesLength, uint32_t natoms);

    HeapPtrAtom *atoms(uint32_t codeLength, uint32_t srcnotesLength) {
        uint32_t length = codeLength + srcnotesLength;
        return reinterpret_cast<HeapPtrAtom *>(data + length + sizeof(JSAtom *) -
                                               length % sizeof(JSAtom *));
    }

    static SharedScriptData *fromBytecode(const jsbytecode *bytecode) {
        return (SharedScriptData *)(bytecode - offsetof(SharedScriptData, data));
    }
};







extern bool
SaveSharedScriptData(JSContext *cx, Handle<JSScript *> script, SharedScriptData *ssd);

struct ScriptBytecodeHasher
{
    struct Lookup
    {
        jsbytecode          *code;
        uint32_t            length;

        Lookup(SharedScriptData *ssd) : code(ssd->data), length(ssd->length) {}
    };
    static HashNumber hash(const Lookup &l) { return mozilla::HashBytes(l.code, l.length); }
    static bool match(SharedScriptData *entry, const Lookup &lookup) {
        if (entry->length != lookup.length)
            return false;
        return PodEqual<jsbytecode>(entry->data, lookup.code, lookup.length);
    }
};

typedef HashSet<SharedScriptData*,
                ScriptBytecodeHasher,
                SystemAllocPolicy> ScriptDataTable;

inline void
MarkScriptBytecode(JSRuntime *rt, const jsbytecode *bytecode);

extern void
SweepScriptData(JSRuntime *rt);

extern void
FreeScriptData(JSRuntime *rt);

struct ScriptAndCounts
{
    
    js::RawScript script;
    ScriptCounts scriptCounts;

    PCCounts &getPCCounts(jsbytecode *pc) const {
        JS_ASSERT(unsigned(pc - script->code) < script->length);
        return scriptCounts.pcCountsVector[pc - script->code];
    }

    ion::IonScriptCounts *getIonCounts() const {
        return scriptCounts.ionCounts;
    }
};

} 

extern jssrcnote *
js_GetSrcNote(JSContext *cx, js::RawScript script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(js::RawScript script, unsigned lineno);

extern JS_FRIEND_API(unsigned)
js_GetScriptLineExtent(js::RawScript script);

namespace js {

extern unsigned
PCToLineNumber(js::RawScript script, jsbytecode *pc, unsigned *columnp = NULL);

extern unsigned
PCToLineNumber(unsigned startLine, jssrcnote *notes, jsbytecode *code, jsbytecode *pc,
               unsigned *columnp = NULL);

extern unsigned
CurrentLine(JSContext *cx);










enum LineOption {
    CALLED_FROM_JSOP_EVAL,
    NOT_CALLED_FROM_JSOP_EVAL
};

inline void
CurrentScriptFileLineOrigin(JSContext *cx, unsigned *linenop, LineOption = NOT_CALLED_FROM_JSOP_EVAL);

extern RawScript
CloneScript(JSContext *cx, HandleObject enclosingScope, HandleFunction fun, HandleScript script);

bool
CloneFunctionScript(JSContext *cx, HandleFunction original, HandleFunction clone);






template<XDRMode mode>
bool
XDRScript(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript enclosingScript,
          HandleFunction fun, MutableHandleScript scriptp);

} 

#endif 
