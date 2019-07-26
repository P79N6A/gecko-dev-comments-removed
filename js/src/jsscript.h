







#ifndef jsscript_h
#define jsscript_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsatom.h"
#ifdef JS_THREADSAFE
#include "jslock.h"
#endif
#include "jsobj.h"
#include "jsopcode.h"

#include "gc/Barrier.h"
#include "gc/Rooting.h"
#include "jit/IonCode.h"
#include "vm/Shape.h"

namespace js {

namespace jit {
    struct BaselineScript;
    struct IonScriptCounts;
}

# define ION_DISABLED_SCRIPT ((js::jit::IonScript *)0x1)
# define ION_COMPILING_SCRIPT ((js::jit::IonScript *)0x2)

# define BASELINE_DISABLED_SCRIPT ((js::jit::BaselineScript *)0x1)

class BreakpointSite;
class BindingIter;
class RegExpObject;
struct SourceCompressionTask;
class Shape;
class WatchpointMap;

namespace analyze {
    class ScriptAnalysis;
}

namespace frontend {
    class BytecodeEmitter;
}

}







typedef enum JSTryNoteKind {
    JSTRY_CATCH,
    JSTRY_FINALLY,
    JSTRY_ITER,
    JSTRY_LOOP
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

    





    static bool initWithTemporaryStorage(ExclusiveContext *cx, InternalBindingsHandle self,
                                         unsigned numArgs, unsigned numVars,
                                         Binding *bindingArray);

    uint8_t *switchToScriptStorage(Binding *newStorage);

    



    static bool clone(JSContext *cx, InternalBindingsHandle self, uint8_t *dstScriptData,
                      HandleScript srcScript);

    unsigned numArgs() const { return numArgs_; }
    unsigned numVars() const { return numVars_; }
    unsigned count() const { return numArgs() + numVars(); }

    
    Shape *callObjShape() const { return callObjShape_; }

    
    static unsigned argumentsVarIndex(ExclusiveContext *cx, InternalBindingsHandle);

    
    bool bindingIsAliased(unsigned bindingIndex);

    
    bool hasAnyAliasedBindings() const { return callObjShape_ && !callObjShape_->isEmptyShape(); }

    void trace(JSTracer *trc);
};

template <>
struct GCMethods<Bindings> {
    static Bindings initial();
    static ThingRootKind kind() { return THING_ROOT_BINDINGS; }
    static bool poisoned(const Bindings &bindings) {
        return IsPoisonedPtr(static_cast<Shape *>(bindings.callObjShape()));
    }
};

class ScriptCounts
{
    friend class ::JSScript;
    friend struct ScriptAndCounts;

    




    PCCounts *pcCountsVector;

    
    jit::IonScriptCounts *ionCounts;

 public:
    ScriptCounts() : pcCountsVector(NULL), ionCounts(NULL) { }

    inline void destroy(FreeOp *fop);

    void set(js::ScriptCounts counts) {
        pcCountsVector = counts.pcCountsVector;
        ionCounts = counts.ionCounts;
    }
};

typedef HashMap<JSScript *,
                ScriptCounts,
                DefaultHasher<JSScript *>,
                SystemAllocPolicy> ScriptCountsMap;

class DebugScript
{
    friend class ::JSScript;

    






    uint32_t        stepMode;

    
    uint32_t        numSites;

    



    BreakpointSite  *breakpoints[1];
};

typedef HashMap<JSScript *,
                DebugScript *,
                DefaultHasher<JSScript *>,
                SystemAllocPolicy> DebugScriptMap;

class ScriptSource
{
    friend class SourceCompressionTask;

    union {
        
        
        
        
        
        
        
        
        jschar *source;
        unsigned char *compressed;
    } data;
    uint32_t refs;
    uint32_t length_;
    uint32_t compressedLength_;
    char *filename_;
    jschar *sourceMapURL_;
    JSPrincipals *originPrincipals_;

    
    
    
    bool sourceRetrievable_:1;
    bool argumentsNotIncluded_:1;
    bool ready_:1;

  public:
    ScriptSource(JSPrincipals *originPrincipals)
      : refs(0),
        length_(0),
        compressedLength_(0),
        filename_(NULL),
        sourceMapURL_(NULL),
        originPrincipals_(originPrincipals),
        sourceRetrievable_(false),
        argumentsNotIncluded_(false),
        ready_(true)
    {
        data.source = NULL;
        if (originPrincipals_)
            JS_HoldPrincipals(originPrincipals_);
    }
    void incref() { refs++; }
    void decref() {
        JS_ASSERT(refs != 0);
        if (--refs == 0)
            destroy();
    }
    bool setSourceCopy(ExclusiveContext *cx,
                       const jschar *src,
                       uint32_t length,
                       bool argumentsNotIncluded,
                       SourceCompressionTask *tok);
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
    const jschar *chars(JSContext *cx);
    JSStableString *substring(JSContext *cx, uint32_t start, uint32_t stop);
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    
    template <XDRMode mode>
    bool performXDR(XDRState<mode> *xdr);

    bool setFilename(ExclusiveContext *cx, const char *filename);
    const char *filename() const {
        return filename_;
    }

    
    bool setSourceMapURL(ExclusiveContext *cx, const jschar *sourceMapURL);
    const jschar *sourceMapURL();
    bool hasSourceMapURL() const { return sourceMapURL_ != NULL; }

    JSPrincipals *originPrincipals() const { return originPrincipals_; }

  private:
    void destroy();
    bool compressed() const { return compressedLength_ != 0; }
    size_t computedSizeOfData() const {
        return compressed() ? compressedLength_ : sizeof(jschar) * length_;
    }
    bool adjustDataSize(size_t nbytes);
    const jschar *getOffThreadCompressionChars(ExclusiveContext *cx);
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

class ScriptSourceObject : public JSObject
{
  public:
    static const Class class_;

    static void finalize(FreeOp *fop, JSObject *obj);
    static ScriptSourceObject *create(ExclusiveContext *cx, ScriptSource *source);

    ScriptSource *source() {
        return static_cast<ScriptSource *>(getReservedSlot(SOURCE_SLOT).toPrivate());
    }

    void setSource(ScriptSource *source);

  private:
    static const uint32_t SOURCE_SLOT = 0;
};

enum GeneratorKind { NotGenerator, LegacyGenerator, StarGenerator };

static inline unsigned
GeneratorKindAsBits(GeneratorKind generatorKind) {
    return static_cast<unsigned>(generatorKind);
}

static inline GeneratorKind
GeneratorKindFromBits(unsigned val) {
    JS_ASSERT(val <= StarGenerator);
    return static_cast<GeneratorKind>(val);
}

} 

class JSScript : public js::gc::Cell
{
    static const uint32_t stepFlagMask = 0x80000000U;
    static const uint32_t stepCountMask = 0x7fffffffU;

  public:
    
    
    
    

    

  public:
    js::Bindings    bindings;   


    

  public:
    jsbytecode      *code;      
    uint8_t         *data;      


    js::HeapPtrAtom *atoms;     

    JSCompartment   *compartment_;

    
    js::types::TypeScript *types;

  private:
    js::HeapPtrObject sourceObject_; 
    js::HeapPtrFunction function_;

    
    
    js::HeapPtrObject   enclosingScopeOrOriginalFunction_;

    
    js::jit::IonScript *ion;
    js::jit::BaselineScript *baseline;

    
    js::jit::IonScript *parallelIon;

    



    uint8_t *baselineOrIonRaw;
    uint8_t *baselineOrIonSkipArgCheck;

    

  public:
    uint32_t        length;     

    uint32_t        dataSize;   

    uint32_t        lineno;     
    uint32_t        column;     

    uint32_t        mainOffset; 


    uint32_t        natoms;     

    
    uint32_t        sourceStart;
    uint32_t        sourceEnd;

  private:
    uint32_t        useCount;   



#ifdef DEBUG
    
    
    uint32_t        id_;
  private:
    uint32_t        idpad;
#endif

    

  private:
    uint16_t        PADDING16;
    uint16_t        version;    

  public:
    uint16_t        funLength;  

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
        ARRAY_KIND_BITS
    };

  private:
    
    
    uint8_t         hasArrayBits:4;

    
    uint8_t         generatorKindBits_:4;

    

  public:
    bool            noScriptRval:1; 

    bool            savedCallerFun:1; 
    bool            strict:1; 
    bool            explicitUseStrict:1; 
    bool            compileAndGo:1;   
    bool            selfHosted:1;     
    bool            bindingsAccessedDynamically:1; 
    bool            funHasExtensibleScope:1;       
    bool            funNeedsDeclEnvObject:1;       
    bool            funHasAnyAliasedFormal:1;      
    bool            warnedAboutUndefinedProp:1; 


    bool            hasSingletons:1;  
    bool            treatAsRunOnce:1; 
    bool            hasRunOnce:1;     
    bool            hasBeenCloned:1;  
    bool            isActiveEval:1;   
    bool            isCachedEval:1;   

    
    bool directlyInsideEval:1;

    
    bool usesArgumentsAndApply:1;

    



    bool            shouldCloneAtCallsite:1;
    bool            isCallsiteClone:1; 
    bool            shouldInline:1;    
#ifdef JS_ION
    bool            failedBoundsCheck:1; 
    bool            failedShapeGuard:1; 
    bool            hadFrequentBailouts:1;
#else
    bool            failedBoundsCheckPad:1;
    bool            failedShapeGuardPad:1;
    bool            hadFrequentBailoutsPad:1;
#endif
    bool            invalidatedIdempotentCache:1; 

    
    
    bool            isGeneratorExp:1;

    bool            hasScriptCounts:1;

    bool            hasDebugScript:1; 

    bool            hasFreezeConstraints:1; 


  private:
    
    bool            argsHasVarBinding_:1;
    bool            needsArgsAnalysis_:1;
    bool            needsArgsObj_:1;

    
    
    

  public:
    static JSScript *Create(js::ExclusiveContext *cx,
                            js::HandleObject enclosingScope, bool savedCallerFun,
                            const JS::CompileOptions &options, unsigned staticLevel,
                            js::HandleScriptSource sourceObject, uint32_t sourceStart,
                            uint32_t sourceEnd);

    void initCompartment(js::ExclusiveContext *cx);

    
    
    
    
    static bool partiallyInit(js::ExclusiveContext *cx, JS::Handle<JSScript*> script,
                              uint32_t nobjects, uint32_t nregexps,
                              uint32_t ntrynotes, uint32_t nconsts, uint32_t nTypeSets);
    static bool fullyInitFromEmitter(js::ExclusiveContext *cx, JS::Handle<JSScript*> script,
                                     js::frontend::BytecodeEmitter *bce);
    
    static bool fullyInitTrivial(js::ExclusiveContext *cx, JS::Handle<JSScript*> script);

    inline JSPrincipals *principals();

    JSCompartment *compartment() const { return compartment_; }

    void setVersion(JSVersion v) { version = v; }

    
    bool argumentsHasVarBinding() const { return argsHasVarBinding_; }
    jsbytecode *argumentsBytecode() const { JS_ASSERT(code[0] == JSOP_ARGUMENTS); return code; }
    void setArgumentsHasVarBinding();

    js::GeneratorKind generatorKind() const {
        return js::GeneratorKindFromBits(generatorKindBits_);
    }
    bool isGenerator() const { return generatorKind() != js::NotGenerator; }
    bool isLegacyGenerator() const { return generatorKind() == js::LegacyGenerator; }
    bool isStarGenerator() const { return generatorKind() == js::StarGenerator; }
    void setGeneratorKind(js::GeneratorKind kind) {
        
        
        JS_ASSERT(!isGenerator());
        generatorKindBits_ = GeneratorKindAsBits(kind);
    }

    









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

    bool hasIonScript() const {
        return ion && ion != ION_DISABLED_SCRIPT && ion != ION_COMPILING_SCRIPT;
    }
    bool canIonCompile() const {
        return ion != ION_DISABLED_SCRIPT;
    }

    bool isIonCompilingOffThread() const {
        return ion == ION_COMPILING_SCRIPT;
    }

    js::jit::IonScript *ionScript() const {
        JS_ASSERT(hasIonScript());
        return ion;
    }
    js::jit::IonScript *maybeIonScript() const {
        return ion;
    }
    js::jit::IonScript *const *addressOfIonScript() const {
        return &ion;
    }
    void setIonScript(js::jit::IonScript *ionScript) {
        if (hasIonScript())
            js::jit::IonScript::writeBarrierPre(tenuredZone(), ion);
        ion = ionScript;
        updateBaselineOrIonRaw();
    }

    bool hasBaselineScript() const {
        return baseline && baseline != BASELINE_DISABLED_SCRIPT;
    }
    bool canBaselineCompile() const {
        return baseline != BASELINE_DISABLED_SCRIPT;
    }
    js::jit::BaselineScript *baselineScript() const {
        JS_ASSERT(hasBaselineScript());
        return baseline;
    }
    inline void setBaselineScript(js::jit::BaselineScript *baselineScript);

    void updateBaselineOrIonRaw();

    bool hasParallelIonScript() const {
        return parallelIon && parallelIon != ION_DISABLED_SCRIPT && parallelIon != ION_COMPILING_SCRIPT;
    }

    bool canParallelIonCompile() const {
        return parallelIon != ION_DISABLED_SCRIPT;
    }

    bool isParallelIonCompilingOffThread() const {
        return parallelIon == ION_COMPILING_SCRIPT;
    }

    js::jit::IonScript *parallelIonScript() const {
        JS_ASSERT(hasParallelIonScript());
        return parallelIon;
    }
    js::jit::IonScript *maybeParallelIonScript() const {
        return parallelIon;
    }
    void setParallelIonScript(js::jit::IonScript *ionScript) {
        if (hasParallelIonScript())
            js::jit::IonScript::writeBarrierPre(tenuredZone(), parallelIon);
        parallelIon = ionScript;
    }

    static size_t offsetOfBaselineScript() {
        return offsetof(JSScript, baseline);
    }
    static size_t offsetOfIonScript() {
        return offsetof(JSScript, ion);
    }
    static size_t offsetOfParallelIonScript() {
        return offsetof(JSScript, parallelIon);
    }
    static size_t offsetOfBaselineOrIonRaw() {
        return offsetof(JSScript, baselineOrIonRaw);
    }
    static size_t offsetOfBaselineOrIonSkipArgCheck() {
        return offsetof(JSScript, baselineOrIonSkipArgCheck);
    }

    



    JSFunction *function() const { return function_; }
    inline void setFunction(JSFunction *fun);

    JSFunction *originalFunction() const;
    void setOriginalFunctionObject(JSObject *fun);

    JSFlatString *sourceData(JSContext *cx);

    static bool loadSource(JSContext *cx, js::HandleScript scr, bool *worked);

    void setSourceObject(js::ScriptSourceObject *object);
    js::ScriptSourceObject *sourceObject() const;
    js::ScriptSource *scriptSource() const { return sourceObject()->source(); }
    JSPrincipals *originPrincipals() const { return scriptSource()->originPrincipals(); }
    const char *filename() const { return scriptSource()->filename(); }

  public:

    
    bool isForEval() { return isCachedEval || isActiveEval; }

#ifdef DEBUG
    unsigned id();
#else
    unsigned id() { return 0; }
#endif

    
    inline bool ensureHasTypes(JSContext *cx);

    
    inline bool ensureHasBytecodeTypeMap(JSContext *cx);

    



    inline bool ensureRanAnalysis(JSContext *cx);

    
    inline bool ensureRanInference(JSContext *cx);

    inline bool hasAnalysis();
    inline void clearAnalysis();
    inline js::analyze::ScriptAnalysis *analysis();

    inline void clearPropertyReadTypes();

    inline js::GlobalObject &global() const;
    js::GlobalObject &uninlinedGlobal() const;

    
    JSObject *enclosingStaticScope() const {
        if (isCallsiteClone)
            return NULL;
        return enclosingScopeOrOriginalFunction_;
    }

    









    bool enclosingScriptsCompiledSuccessfully() const;

  private:
    bool makeTypes(JSContext *cx);
    bool makeBytecodeTypeMap(JSContext *cx);
    bool makeAnalysis(JSContext *cx);

  public:
    uint32_t getUseCount() const  { return useCount; }
    uint32_t incUseCount(uint32_t amount = 1) { return useCount += amount; }
    uint32_t *addressOfUseCount() { return &useCount; }
    static size_t offsetOfUseCount() { return offsetof(JSScript, useCount); }
    void resetUseCount() { useCount = 0; }

  public:
    bool initScriptCounts(JSContext *cx);
    js::PCCounts getPCCounts(jsbytecode *pc);
    void addIonCounts(js::jit::IonScriptCounts *ionCounts);
    js::jit::IonScriptCounts *getIonCounts();
    js::ScriptCounts releaseScriptCounts();
    void destroyScriptCounts(js::FreeOp *fop);

    jsbytecode *main() {
        return code + mainOffset;
    }

    




    size_t computedSizeOfData();
    size_t sizeOfData(mozilla::MallocSizeOf mallocSizeOf);

    uint32_t numNotes();  

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    bool hasArray(ArrayKind kind)           { return (hasArrayBits & (1 << kind)); }
    void setHasArray(ArrayKind kind)        { hasArrayBits |= (1 << kind); }
    void cloneHasArray(JSScript *script) { hasArrayBits = script->hasArrayBits; }

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

    size_t innerObjectsStart() {
        
        return savedCallerFun ? 1 : 0;
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

    




    bool isEmpty() const {
        if (length > 3)
            return false;

        jsbytecode *pc = code;
        if (noScriptRval && JSOp(*pc) == JSOP_FALSE)
            ++pc;
        return JSOp(*pc) == JSOP_STOP;
    }

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

    void clearBreakpointsIn(js::FreeOp *fop, js::Debugger *dbg, JSObject *handler);
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
    JS::shadow::Zone *shadowZone() const { return JS::shadow::Zone::asShadowZone(zone()); }

    static void writeBarrierPre(JSScript *script) {
#ifdef JSGC_INCREMENTAL
        if (!script || !script->shadowRuntimeFromAnyThread()->needsBarrier())
            return;

        JS::shadow::Zone *shadowZone = script->shadowZone();
        if (shadowZone->needsBarrier()) {
            MOZ_ASSERT(!js::RuntimeFromMainThreadIsHeapMajorCollecting(shadowZone));
            JSScript *tmp = script;
            js::gc::MarkScriptUnbarriered(shadowZone->barrierTracer(), &tmp, "write barrier");
            JS_ASSERT(tmp == script);
        }
#endif
    }

    static void writeBarrierPost(JSScript *script, void *addr) {}

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SCRIPT; }

    void markChildren(JSTracer *trc);
};


JS_STATIC_ASSERT(JSScript::ARRAY_KIND_BITS <= 4);


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
    explicit inline AliasedFormalIter(JSScript *script);

    bool done() const { return p_ == end_; }
    operator bool() const { return !done(); }
    void operator++(int) { JS_ASSERT(!done()); p_++; slot_++; settle(); }

    const Binding &operator*() const { JS_ASSERT(!done()); return *p_; }
    const Binding *operator->() const { JS_ASSERT(!done()); return p_; }
    unsigned frameIndex() const { JS_ASSERT(!done()); return p_ - begin_; }
    unsigned scopeSlot() const { JS_ASSERT(!done()); return slot_; }
};



class LazyScript : public js::gc::Cell
{
    
    
    HeapPtrScript script_;

    
    HeapPtrFunction function_;

    
    HeapPtrObject enclosingScope_;

    
    
    HeapPtrObject sourceObject_;

    
    void *table_;

#if JS_BITS_PER_WORD == 32
    uint32_t padding;
#endif

    
    uint32_t version_ : 8;

    uint32_t numFreeVariables_ : 24;
    uint32_t numInnerFunctions_ : 24;

    uint32_t generatorKindBits_:2;

    
    uint32_t strict_ : 1;
    uint32_t bindingsAccessedDynamically_ : 1;
    uint32_t hasDebuggerStatement_ : 1;
    uint32_t directlyInsideEval_:1;
    uint32_t usesArgumentsAndApply_:1;
    uint32_t hasBeenCloned_:1;

    
    uint32_t begin_;
    uint32_t end_;
    uint32_t lineno_;
    uint32_t column_;

    LazyScript(JSFunction *fun, void *table,
               uint32_t numFreeVariables, uint32_t numInnerFunctions, JSVersion version,
               uint32_t begin, uint32_t end, uint32_t lineno, uint32_t column);

  public:
    static LazyScript *Create(ExclusiveContext *cx, HandleFunction fun,
                              uint32_t numFreeVariables, uint32_t numInnerFunctions,
                              JSVersion version, uint32_t begin, uint32_t end,
                              uint32_t lineno, uint32_t column);

    JSFunction *function() const {
        return function_;
    }

    void initScript(JSScript *script);
    JSScript *maybeScript() {
        return script_;
    }

    JSObject *enclosingScope() const {
        return enclosingScope_;
    }
    ScriptSourceObject *sourceObject() const;
    ScriptSource *scriptSource() const {
        return sourceObject()->source();
    }
    JSPrincipals *originPrincipals() const {
        return scriptSource()->originPrincipals();
    }
    JSVersion version() const {
        JS_STATIC_ASSERT(JSVERSION_UNKNOWN == -1);
        return (version_ == JS_BIT(8) - 1) ? JSVERSION_UNKNOWN : JSVersion(version_);
    }

    void setParent(JSObject *enclosingScope, ScriptSourceObject *sourceObject);

    uint32_t numFreeVariables() const {
        return numFreeVariables_;
    }
    HeapPtrAtom *freeVariables() {
        return (HeapPtrAtom *)table_;
    }

    uint32_t numInnerFunctions() const {
        return numInnerFunctions_;
    }
    HeapPtrFunction *innerFunctions() {
        return (HeapPtrFunction *)&freeVariables()[numFreeVariables()];
    }

    GeneratorKind generatorKind() const { return GeneratorKindFromBits(generatorKindBits_); }

    bool isGenerator() const { return generatorKind() != NotGenerator; }

    bool isLegacyGenerator() const { return generatorKind() == LegacyGenerator; }

    bool isStarGenerator() const { return generatorKind() == StarGenerator; }

    void setGeneratorKind(GeneratorKind kind) {
        
        
        JS_ASSERT(!isGenerator());
        
        JS_ASSERT(kind != LegacyGenerator);
        generatorKindBits_ = GeneratorKindAsBits(kind);
    }

    bool strict() const {
        return strict_;
    }
    void setStrict() {
        strict_ = true;
    }

    bool bindingsAccessedDynamically() const {
        return bindingsAccessedDynamically_;
    }
    void setBindingsAccessedDynamically() {
        bindingsAccessedDynamically_ = true;
    }

    bool hasDebuggerStatement() const {
        return hasDebuggerStatement_;
    }
    void setHasDebuggerStatement() {
        hasDebuggerStatement_ = true;
    }

    bool directlyInsideEval() const {
        return directlyInsideEval_;
    }
    void setDirectlyInsideEval() {
        directlyInsideEval_ = true;
    }

    bool usesArgumentsAndApply() const {
        return usesArgumentsAndApply_;
    }
    void setUsesArgumentsAndApply() {
        usesArgumentsAndApply_ = true;
    }

    bool hasBeenCloned() const {
        return hasBeenCloned_;
    }
    void setHasBeenCloned() {
        hasBeenCloned_ = true;
    }

    ScriptSource *source() const {
        return sourceObject()->source();
    }
    uint32_t begin() const {
        return begin_;
    }
    uint32_t end() const {
        return end_;
    }
    uint32_t lineno() const {
        return lineno_;
    }
    uint32_t column() const {
        return column_;
    }

    uint32_t staticLevel(JSContext *cx) const;

    Zone *zone() const { return tenuredZone(); }
    JS::shadow::Zone *shadowZone() const { return JS::shadow::Zone::asShadowZone(zone()); }

    void markChildren(JSTracer *trc);
    void finalize(js::FreeOp *fop);

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
    {
        return mallocSizeOf(table_);
    }

    static void writeBarrierPre(LazyScript *lazy) {
#ifdef JSGC_INCREMENTAL
        if (!lazy || !lazy->shadowRuntimeFromAnyThread()->needsBarrier())
            return;

        JS::shadow::Zone *shadowZone = lazy->shadowZone();
        if (shadowZone->needsBarrier()) {
            MOZ_ASSERT(!js::RuntimeFromMainThreadIsHeapMajorCollecting(shadowZone));
            js::LazyScript *tmp = lazy;
            MarkLazyScriptUnbarriered(shadowZone->barrierTracer(), &tmp, "write barrier");
            JS_ASSERT(tmp == lazy);
        }
#endif
    }
};


JS_STATIC_ASSERT(sizeof(LazyScript) % js::gc::CellSize == 0);








extern void
CallNewScriptHook(JSContext *cx, JS::HandleScript script, JS::HandleFunction fun);

extern void
CallDestroyScriptHook(FreeOp *fop, JSScript *script);

struct SharedScriptData
{
    uint32_t length;
    uint32_t natoms;
    bool marked;
    jsbytecode data[1];

    static SharedScriptData *new_(ExclusiveContext *cx, uint32_t codeLength,
                                  uint32_t srcnotesLength, uint32_t natoms);

    HeapPtrAtom *atoms() {
        if (!natoms)
            return NULL;
        return reinterpret_cast<HeapPtrAtom *>(data + length - sizeof(JSAtom *) * natoms);
    }

    static SharedScriptData *fromBytecode(const jsbytecode *bytecode) {
        return (SharedScriptData *)(bytecode - offsetof(SharedScriptData, data));
    }

  private:
    SharedScriptData() MOZ_DELETE;
    SharedScriptData(const SharedScriptData&) MOZ_DELETE;
};

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
        return mozilla::PodEqual<jsbytecode>(entry->data, lookup.code, lookup.length);
    }
};

typedef HashSet<SharedScriptData*,
                ScriptBytecodeHasher,
                SystemAllocPolicy> ScriptDataTable;

extern void
SweepScriptData(JSRuntime *rt);

extern void
FreeScriptData(JSRuntime *rt);

struct ScriptAndCounts
{
    
    JSScript *script;
    ScriptCounts scriptCounts;

    PCCounts &getPCCounts(jsbytecode *pc) const {
        JS_ASSERT(unsigned(pc - script->code) < script->length);
        return scriptCounts.pcCountsVector[pc - script->code];
    }

    jit::IonScriptCounts *getIonCounts() const {
        return scriptCounts.ionCounts;
    }
};

} 

extern jssrcnote *
js_GetSrcNote(JSContext *cx, JSScript *script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(JSScript *script, unsigned lineno);

extern JS_FRIEND_API(unsigned)
js_GetScriptLineExtent(JSScript *script);

namespace js {

extern unsigned
PCToLineNumber(JSScript *script, jsbytecode *pc, unsigned *columnp = NULL);

extern unsigned
PCToLineNumber(unsigned startLine, jssrcnote *notes, jsbytecode *code, jsbytecode *pc,
               unsigned *columnp = NULL);










enum LineOption {
    CALLED_FROM_JSOP_EVAL,
    NOT_CALLED_FROM_JSOP_EVAL
};

extern void
CurrentScriptFileLineOrigin(JSContext *cx, const char **file, unsigned *linenop,
                            JSPrincipals **origin, LineOption opt = NOT_CALLED_FROM_JSOP_EVAL);

extern JSScript *
CloneScript(JSContext *cx, HandleObject enclosingScope, HandleFunction fun, HandleScript script,
            NewObjectKind newKind = GenericObject);

bool
CloneFunctionScript(JSContext *cx, HandleFunction original, HandleFunction clone,
                    NewObjectKind newKind = GenericObject);








static inline JSPrincipals *
NormalizeOriginPrincipals(JSPrincipals *principals, JSPrincipals *originPrincipals)
{
    return originPrincipals ? originPrincipals : principals;
}






template<XDRMode mode>
bool
XDRScript(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript enclosingScript,
          HandleFunction fun, MutableHandleScript scriptp);

} 

#endif 
