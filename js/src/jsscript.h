







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
#include "jstypes.h"

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
class LazyScript;
class RegExpObject;
struct SourceCompressionTask;
class Shape;
class WatchpointMap;
class NestedScopeObject;

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
    uint32_t        stackDepth; 
    uint32_t        start;      

    uint32_t        length;     
};

namespace js {














struct BlockScopeNote {
    static const uint32_t NoBlockScopeIndex = UINT32_MAX;

    uint32_t        index;      
                                
                                
    uint32_t        start;      
                                
    uint32_t        length;     
    uint32_t        parent;     
};

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

struct BlockScopeArray {
    BlockScopeNote *vector;     
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
    uint32_t numVars_;

    








    static const uintptr_t TEMPORARY_STORAGE_BIT = 0x1;
    bool bindingArrayUsingTemporaryStorage() const {
        return bindingArrayAndFlag_ & TEMPORARY_STORAGE_BIT;
    }

  public:

    Binding *bindingArray() const {
        return reinterpret_cast<Binding *>(bindingArrayAndFlag_ & ~TEMPORARY_STORAGE_BIT);
    }

    inline Bindings();

    





    static bool initWithTemporaryStorage(ExclusiveContext *cx, InternalBindingsHandle self,
                                         unsigned numArgs, uint32_t numVars,
                                         Binding *bindingArray);

    uint8_t *switchToScriptStorage(Binding *newStorage);

    



    static bool clone(JSContext *cx, InternalBindingsHandle self, uint8_t *dstScriptData,
                      HandleScript srcScript);

    unsigned numArgs() const { return numArgs_; }
    uint32_t numVars() const { return numVars_; }
    uint32_t count() const { return numArgs() + numVars(); }

    
    Shape *callObjShape() const { return callObjShape_; }

    
    static uint32_t argumentsVarIndex(ExclusiveContext *cx, InternalBindingsHandle);

    
    bool bindingIsAliased(uint32_t bindingIndex);

    
    bool hasAnyAliasedBindings() const {
        if (!callObjShape_)
            return false;

        return !callObjShape_->isEmptyShape();
    }

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
    ScriptCounts() : pcCountsVector(nullptr), ionCounts(nullptr) { }

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

class ScriptSource;

class SourceDataCache
{
    typedef HashMap<ScriptSource *,
                    const jschar *,
                    DefaultHasher<ScriptSource *>,
                    SystemAllocPolicy> Map;
    Map *map_;
    size_t numSuppressPurges_;

  public:
    SourceDataCache() : map_(nullptr), numSuppressPurges_(0) {}

    class AutoSuppressPurge
    {
        SourceDataCache &cache_;
        mozilla::DebugOnly<size_t> oldValue_;
      public:
        explicit AutoSuppressPurge(JSContext *cx);
        ~AutoSuppressPurge();
        SourceDataCache &cache() const { return cache_; }
    };

    const jschar *lookup(ScriptSource *ss, const AutoSuppressPurge &asp);
    bool put(ScriptSource *ss, const jschar *chars, const AutoSuppressPurge &asp);

    void purge();

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

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
    jschar *displayURL_;
    jschar *sourceMapURL_;
    JSPrincipals *originPrincipals_;

    
    
    
    uint32_t introductionOffset_;

    
    
    
    
    
    
    char *introducerFilename_;

    
    
    
    
    
    
    
    
    
    
    
    const char *introductionType_;

    
    
    
    bool sourceRetrievable_:1;
    bool argumentsNotIncluded_:1;
    bool ready_:1;
    bool hasIntroductionOffset_:1;

  public:
    explicit ScriptSource(JSPrincipals *originPrincipals)
      : refs(0),
        length_(0),
        compressedLength_(0),
        filename_(nullptr),
        displayURL_(nullptr),
        sourceMapURL_(nullptr),
        originPrincipals_(originPrincipals),
        introductionOffset_(0),
        introducerFilename_(nullptr),
        introductionType_(nullptr),
        sourceRetrievable_(false),
        argumentsNotIncluded_(false),
        ready_(true),
        hasIntroductionOffset_(false)
    {
        data.source = nullptr;
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
    void setSource(const jschar *src, size_t length);
    bool ready() const { return ready_; }
    void setSourceRetrievable() { sourceRetrievable_ = true; }
    bool sourceRetrievable() const { return sourceRetrievable_; }
    bool hasSourceData() const { return !ready() || !!data.source; }
    uint32_t length() const {
        JS_ASSERT(hasSourceData());
        return length_;
    }
    bool argumentsNotIncluded() const {
        JS_ASSERT(hasSourceData());
        return argumentsNotIncluded_;
    }
    const jschar *chars(JSContext *cx, const SourceDataCache::AutoSuppressPurge &asp);
    JSFlatString *substring(JSContext *cx, uint32_t start, uint32_t stop);
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    
    template <XDRMode mode>
    bool performXDR(XDRState<mode> *xdr);

    bool setFilename(ExclusiveContext *cx, const char *filename);
    bool setIntroducedFilename(ExclusiveContext *cx,
                               const char *callerFilename, unsigned callerLineno,
                               const char *introductionType, const char *introducerFilename);
    const char *introducerFilename() const {
        return introducerFilename_;
    }
    bool hasIntroductionType() const {
        return introductionType_;
    }
    const char *introductionType() const {
        JS_ASSERT(hasIntroductionType());
        return introductionType_;
    }
    const char *filename() const {
        return filename_;
    }

    
    bool setDisplayURL(ExclusiveContext *cx, const jschar *displayURL);
    const jschar *displayURL();
    bool hasDisplayURL() const { return displayURL_ != nullptr; }

    
    bool setSourceMapURL(ExclusiveContext *cx, const jschar *sourceMapURL);
    const jschar *sourceMapURL();
    bool hasSourceMapURL() const { return sourceMapURL_ != nullptr; }

    JSPrincipals *originPrincipals() const { return originPrincipals_; }

    bool hasIntroductionOffset() const { return hasIntroductionOffset_; }
    uint32_t introductionOffset() const {
        JS_ASSERT(hasIntroductionOffset());
        return introductionOffset_;
    }
    void setIntroductionOffset(uint32_t offset) {
        JS_ASSERT(!hasIntroductionOffset());
        JS_ASSERT(offset <= (uint32_t)INT32_MAX);
        introductionOffset_ = offset;
        hasIntroductionOffset_ = true;
    }

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
    static ScriptSourceObject *create(ExclusiveContext *cx, ScriptSource *source,
                                      const ReadOnlyCompileOptions &options);

    ScriptSource *source() {
        return static_cast<ScriptSource *>(getReservedSlot(SOURCE_SLOT).toPrivate());
    }

    void setSource(ScriptSource *source);

    JSObject *element() const;
    void initElement(HandleObject element);
    const Value &elementAttributeName() const;

  private:
    static const uint32_t SOURCE_SLOT = 0;
    static const uint32_t ELEMENT_SLOT = 1;
    static const uint32_t ELEMENT_PROPERTY_SLOT = 2;
    static const uint32_t RESERVED_SLOTS = 3;
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






template<XDRMode mode>
bool
XDRScript(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript enclosingScript,
          HandleFunction fun, MutableHandleScript scriptp);

JSScript *
CloneScript(JSContext *cx, HandleObject enclosingScope, HandleFunction fun, HandleScript script,
            NewObjectKind newKind = GenericObject);




template<XDRMode mode>
bool
XDRScriptConst(XDRState<mode> *xdr, MutableHandleValue vp);

} 

class JSScript : public js::gc::BarrieredCell<JSScript>
{
    static const uint32_t stepFlagMask = 0x80000000U;
    static const uint32_t stepCountMask = 0x7fffffffU;

    template <js::XDRMode mode>
    friend
    bool
    js::XDRScript(js::XDRState<mode> *xdr, js::HandleObject enclosingScope, js::HandleScript enclosingScript,
                  js::HandleFunction fun, js::MutableHandleScript scriptp);

    friend JSScript *
    js::CloneScript(JSContext *cx, js::HandleObject enclosingScope, js::HandleFunction fun, js::HandleScript src,
                    js::NewObjectKind newKind);

  public:
    
    
    
    

    

  public:
    js::Bindings    bindings;   


    bool hasAnyAliasedBindings() const {
        return bindings.hasAnyAliasedBindings();
    }

    js::Binding *bindingArray() const {
        return bindings.bindingArray();
    }

    unsigned numArgs() const {
        return bindings.numArgs();
    }

    js::Shape *callObjShape() const {
        return bindings.callObjShape();
    }

    

  private:
    jsbytecode      *code_;     
  public:
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

    
    js::LazyScript *lazyScript;

    



    uint8_t *baselineOrIonRaw;
    uint8_t *baselineOrIonSkipArgCheck;

    

    uint32_t        length_;    
    uint32_t        dataSize_;  

    uint32_t        lineno_;    
    uint32_t        column_;    

    uint32_t        mainOffset_;


    uint32_t        natoms_;    
    uint32_t        nslots_;    

    
    uint32_t        sourceStart_;
    uint32_t        sourceEnd_;

    uint32_t        useCount;   





#ifdef DEBUG
    
    
    uint32_t        id_;
    uint32_t        idpad;
#endif

    

    uint16_t        version;    

    uint16_t        funLength_; 

    uint16_t        nTypeSets_; 


    uint16_t        staticLevel_;

    

  public:
    
    enum ArrayKind {
        CONSTS,
        OBJECTS,
        REGEXPS,
        TRYNOTES,
        BLOCK_SCOPES,
        ARRAY_KIND_BITS
    };

  private:
    
    
    uint8_t         hasArrayBits:ARRAY_KIND_BITS;

    
    uint8_t         generatorKindBits_:2;

    

    
    bool noScriptRval_:1;

    
    bool savedCallerFun_:1;

    
    bool strict_:1;

    
    bool explicitUseStrict_:1;

    
    bool compileAndGo_:1;

    
    bool selfHosted_:1;

    
    bool bindingsAccessedDynamically_:1;
    bool funHasExtensibleScope_:1;
    bool funNeedsDeclEnvObject_:1;

    
    bool funHasAnyAliasedFormal_:1;

    
    bool warnedAboutUndefinedProp_:1;

    
    bool hasSingletons_:1;

    
    bool treatAsRunOnce_:1;

    
    bool hasRunOnce_:1;

    
    bool hasBeenCloned_:1;

    
    bool isActiveEval_:1;

    
    bool isCachedEval_:1;

    
    bool directlyInsideEval_:1;

    
    bool usesArgumentsAndApply_:1;

    



    bool shouldCloneAtCallsite_:1;
    bool isCallsiteClone_:1; 
    bool shouldInline_:1;    

    
    bool failedBoundsCheck_:1; 
    bool failedShapeGuard_:1; 
    bool hadFrequentBailouts_:1;
    bool uninlineable_:1;    

    
    bool invalidatedIdempotentCache_:1;

    
    
    bool isGeneratorExp_:1;

    
    bool hasScriptCounts_:1;

    
    bool hasDebugScript_:1;

    
    bool hasFreezeConstraints_:1;

    
    bool argsHasVarBinding_:1;
    bool needsArgsAnalysis_:1;
    bool needsArgsObj_:1;

    
    
    

  public:
    static JSScript *Create(js::ExclusiveContext *cx,
                            js::HandleObject enclosingScope, bool savedCallerFun,
                            const JS::ReadOnlyCompileOptions &options, unsigned staticLevel,
                            js::HandleObject sourceObject, uint32_t sourceStart,
                            uint32_t sourceEnd);

    void initCompartment(js::ExclusiveContext *cx);

    
    
    
    
    static bool partiallyInit(js::ExclusiveContext *cx, JS::Handle<JSScript*> script,
                              uint32_t nconsts, uint32_t nobjects, uint32_t nregexps,
                              uint32_t ntrynotes, uint32_t nblockscopes,
                              uint32_t nTypeSets);
    static bool fullyInitFromEmitter(js::ExclusiveContext *cx, JS::Handle<JSScript*> script,
                                     js::frontend::BytecodeEmitter *bce);
    
    static bool fullyInitTrivial(js::ExclusiveContext *cx, JS::Handle<JSScript*> script);

    inline JSPrincipals *principals();

    JSCompartment *compartment() const { return compartment_; }

    void setVersion(JSVersion v) { version = v; }

    
    jsbytecode *code() const {
        return code_;
    }
    size_t length() const {
        return length_;
    }

    void setCode(jsbytecode *code) { code_ = code; }
    void setLength(size_t length) { length_ = length; }

    jsbytecode *codeEnd() const { return code() + length(); }

    bool containsPC(const jsbytecode *pc) const {
        return pc >= code() && pc < codeEnd();
    }

    size_t pcToOffset(const jsbytecode *pc) const {
        JS_ASSERT(containsPC(pc));
        return size_t(pc - code());
    }

    jsbytecode *offsetToPC(size_t offset) const {
        JS_ASSERT(offset < length());
        return code() + offset;
    }

    size_t mainOffset() const {
        return mainOffset_;
    }

    size_t lineno() const {
        return lineno_;
    }

    size_t column() const {
        return column_;
    }

    void setColumn(size_t column) { column_ = column; }

    size_t nfixed() const {
        return function_ ? bindings.numVars() : 0;
    }

    size_t nslots() const {
        return nslots_;
    }

    size_t staticLevel() const {
        return staticLevel_;
    }

    size_t nTypeSets() const {
        return nTypeSets_;
    }

    size_t funLength() const {
        return funLength_;
    }

    size_t sourceStart() const {
        return sourceStart_;
    }

    size_t sourceEnd() const {
        return sourceEnd_;
    }

    bool noScriptRval() const {
        return noScriptRval_;
    }

    bool savedCallerFun() const { return savedCallerFun_; }

    bool strict() const {
        return strict_;
    }

    bool explicitUseStrict() const { return explicitUseStrict_; }

    bool compileAndGo() const {
        return compileAndGo_;
    }

    bool selfHosted() const { return selfHosted_; }
    bool bindingsAccessedDynamically() const { return bindingsAccessedDynamically_; }
    bool funHasExtensibleScope() const {
        return funHasExtensibleScope_;
    }
    bool funNeedsDeclEnvObject() const {
        return funNeedsDeclEnvObject_;
    }
    bool funHasAnyAliasedFormal() const {
        return funHasAnyAliasedFormal_;
    }

    bool hasSingletons() const { return hasSingletons_; }
    bool treatAsRunOnce() const {
        return treatAsRunOnce_;
    }
    bool hasRunOnce() const { return hasRunOnce_; }
    bool hasBeenCloned() const { return hasBeenCloned_; }

    void setTreatAsRunOnce() { treatAsRunOnce_ = true; }
    void setHasRunOnce() { hasRunOnce_ = true; }
    void setHasBeenCloned() { hasBeenCloned_ = true; }

    bool isActiveEval() const { return isActiveEval_; }
    bool isCachedEval() const { return isCachedEval_; }
    bool directlyInsideEval() const { return directlyInsideEval_; }

    void cacheForEval() {
        JS_ASSERT(isActiveEval() && !isCachedEval());
        isActiveEval_ = false;
        isCachedEval_ = true;
    }

    void uncacheForEval() {
        JS_ASSERT(isCachedEval() && !isActiveEval());
        isCachedEval_ = false;
        isActiveEval_ = true;
    }

    void setActiveEval() { isActiveEval_ = true; }
    void setDirectlyInsideEval() { directlyInsideEval_ = true; }

    bool usesArgumentsAndApply() const {
        return usesArgumentsAndApply_;
    }
    void setUsesArgumentsAndApply() { usesArgumentsAndApply_ = true; }

    bool shouldCloneAtCallsite() const {
        return shouldCloneAtCallsite_;
    }
    bool shouldInline() const {
        return shouldInline_;
    }

    void setShouldCloneAtCallsite() { shouldCloneAtCallsite_ = true; }
    void setShouldInline() { shouldInline_ = true; }

    bool isCallsiteClone() const {
        return isCallsiteClone_;
    }
    bool isGeneratorExp() const { return isGeneratorExp_; }

    bool failedBoundsCheck() const {
        return failedBoundsCheck_;
    }
    bool failedShapeGuard() const {
        return failedShapeGuard_;
    }
    bool hadFrequentBailouts() const {
        return hadFrequentBailouts_;
    }
    bool uninlineable() const {
        return uninlineable_;
    }
    bool invalidatedIdempotentCache() const {
        return invalidatedIdempotentCache_;
    }

    void setFailedBoundsCheck() { failedBoundsCheck_ = true; }
    void setFailedShapeGuard() { failedShapeGuard_ = true; }
    void setHadFrequentBailouts() { hadFrequentBailouts_ = true; }
    void setUninlineable() { uninlineable_ = true; }
    void setInvalidatedIdempotentCache() { invalidatedIdempotentCache_ = true; }

    bool hasScriptCounts() const { return hasScriptCounts_; }

    bool hasFreezeConstraints() const { return hasFreezeConstraints_; }
    void setHasFreezeConstraints() { hasFreezeConstraints_ = true; }
    void clearHasFreezeConstraints() { hasFreezeConstraints_ = false; }

    bool warnedAboutUndefinedProp() const { return warnedAboutUndefinedProp_; }
    void setWarnedAboutUndefinedProp() { warnedAboutUndefinedProp_ = true; }

    
    bool argumentsHasVarBinding() const {
        return argsHasVarBinding_;
    }
    jsbytecode *argumentsBytecode() const { JS_ASSERT(code()[0] == JSOP_ARGUMENTS); return code(); }
    void setArgumentsHasVarBinding();
    bool argumentsAliasesFormals() const {
        return argumentsHasVarBinding() && !strict();
    }

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
    bool needsArgsObj() const {
        JS_ASSERT(analyzedArgsUsage());
        return needsArgsObj_;
    }
    void setNeedsArgsObj(bool needsArgsObj);
    static bool argumentsOptimizationFailed(JSContext *cx, js::HandleScript script);

    








    bool argsObjAliasesFormals() const {
        return needsArgsObj() && !strict();
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
    inline void setBaselineScript(JSContext *maybecx, js::jit::BaselineScript *baselineScript);

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

    bool isRelazifiable() const {
        return (selfHosted() || lazyScript) &&
               !isGenerator() && !hasBaselineScript() && !hasAnyIonScript();
    }
    void setLazyScript(js::LazyScript *lazy) {
        lazyScript = lazy;
    }
    js::LazyScript *maybeLazyScript() {
        return lazyScript;
    }

    







    inline JSFunction *functionDelazifying() const;
    JSFunction *functionNonDelazifying() const {
        return function_;
    }
    inline void setFunction(JSFunction *fun);
    



    inline void ensureNonLazyCanonicalFunction(JSContext *cx);

    


    JSFunction *donorFunction() const;
    void setIsCallsiteClone(JSObject *fun);

    JSFlatString *sourceData(JSContext *cx);

    static bool loadSource(JSContext *cx, js::ScriptSource *ss, bool *worked);

    void setSourceObject(JSObject *object);
    JSObject *sourceObject() const {
        return sourceObject_;
    }
    js::ScriptSource *scriptSource() const;
    JSPrincipals *originPrincipals() const { return scriptSource()->originPrincipals(); }
    const char *filename() const { return scriptSource()->filename(); }

  public:

    
    bool isForEval() { return isCachedEval() || isActiveEval(); }

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

    inline js::GlobalObject &global() const;
    js::GlobalObject &uninlinedGlobal() const;

    
    JSObject *enclosingStaticScope() const {
        if (isCallsiteClone())
            return nullptr;
        return enclosingScopeOrOriginalFunction_;
    }

  private:
    bool makeTypes(JSContext *cx);
    bool makeAnalysis(JSContext *cx);

  public:
    uint32_t getUseCount() const {
        return useCount;
    }
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
        return code() + mainOffset();
    }

    




    size_t computedSizeOfData() const;
    size_t sizeOfData(mozilla::MallocSizeOf mallocSizeOf) const;
    size_t sizeOfTypeScript(mozilla::MallocSizeOf mallocSizeOf) const;

    uint32_t numNotes();  

    
    jssrcnote *notes() { return (jssrcnote *)(code() + length()); }

    bool hasArray(ArrayKind kind) {
        return (hasArrayBits & (1 << kind));
    }
    void setHasArray(ArrayKind kind) { hasArrayBits |= (1 << kind); }
    void cloneHasArray(JSScript *script) { hasArrayBits = script->hasArrayBits; }

    bool hasConsts()        { return hasArray(CONSTS);      }
    bool hasObjects()       { return hasArray(OBJECTS);     }
    bool hasRegexps()       { return hasArray(REGEXPS);     }
    bool hasTrynotes()      { return hasArray(TRYNOTES);    }
    bool hasBlockScopes()   { return hasArray(BLOCK_SCOPES); }

    #define OFF(fooOff, hasFoo, t)   (fooOff() + (hasFoo() ? sizeof(t) : 0))

    size_t constsOffset()     { return 0; }
    size_t objectsOffset()    { return OFF(constsOffset,     hasConsts,     js::ConstArray);      }
    size_t regexpsOffset()    { return OFF(objectsOffset,    hasObjects,    js::ObjectArray);     }
    size_t trynotesOffset()   { return OFF(regexpsOffset,    hasRegexps,    js::ObjectArray);     }
    size_t blockScopesOffset(){ return OFF(trynotesOffset,   hasTrynotes,   js::TryNoteArray);    }

    size_t dataSize() const { return dataSize_; }

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

    js::BlockScopeArray *blockScopes() {
        JS_ASSERT(hasBlockScopes());
        return reinterpret_cast<js::BlockScopeArray *>(data + blockScopesOffset());
    }

    bool hasLoops();

    size_t natoms() const { return natoms_; }

    js::HeapPtrAtom &getAtom(size_t index) const {
        JS_ASSERT(index < natoms());
        return atoms[index];
    }

    js::HeapPtrAtom &getAtom(jsbytecode *pc) const {
        JS_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
        return getAtom(GET_UINT32_INDEX(pc));
    }

    js::PropertyName *getName(size_t index) {
        return getAtom(index)->asPropertyName();
    }

    js::PropertyName *getName(jsbytecode *pc) const {
        JS_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
        return getAtom(GET_UINT32_INDEX(pc))->asPropertyName();
    }

    JSObject *getObject(size_t index) {
        js::ObjectArray *arr = objects();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    size_t innerObjectsStart() {
        
        return savedCallerFun() ? 1 : 0;
    }

    JSObject *getObject(jsbytecode *pc) {
        JS_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
        return getObject(GET_UINT32_INDEX(pc));
    }

    JSVersion getVersion() const {
        return JSVersion(version);
    }

    inline JSFunction *getFunction(size_t index);
    inline JSFunction *getCallerFunction();
    inline JSFunction *functionOrCallerFunction();

    inline js::RegExpObject *getRegExp(size_t index);
    inline js::RegExpObject *getRegExp(jsbytecode *pc);

    const js::Value &getConst(size_t index) {
        js::ConstArray *arr = consts();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    js::NestedScopeObject *getStaticScope(jsbytecode *pc);

    




    bool isEmpty() const {
        if (length() > 3)
            return false;

        jsbytecode *pc = code();
        if (noScriptRval() && JSOp(*pc) == JSOP_FALSE)
            ++pc;
        return JSOp(*pc) == JSOP_RETRVAL;
    }

    bool varIsAliased(uint32_t varSlot);
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
    bool hasAnyBreakpointsOrStepMode() { return hasDebugScript_; }

    js::BreakpointSite *getBreakpointSite(jsbytecode *pc)
    {
        return hasDebugScript_ ? debugScript()->breakpoints[pcToOffset(pc)] : nullptr;
    }

    js::BreakpointSite *getOrCreateBreakpointSite(JSContext *cx, jsbytecode *pc);

    void destroyBreakpointSite(js::FreeOp *fop, jsbytecode *pc);

    void clearBreakpointsIn(js::FreeOp *fop, js::Debugger *dbg, JSObject *handler);
    void clearTraps(js::FreeOp *fop);

    void markTrapClosures(JSTracer *trc);

    





    bool setStepModeFlag(JSContext *cx, bool step);

    





    bool changeStepModeCount(JSContext *cx, int delta);

    bool stepModeEnabled() { return hasDebugScript_ && !!debugScript()->stepMode; }

#ifdef DEBUG
    uint32_t stepModeCount() { return hasDebugScript_ ? (debugScript()->stepMode & stepCountMask) : 0; }
#endif

    void finalize(js::FreeOp *fop);

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SCRIPT; }

    void markChildren(JSTracer *trc);
};


static_assert(sizeof(JSScript) % js::gc::CellSize == 0,
              "Size of JSScript must be an integral multiple of js::gc::CellSize");

namespace js {







class BindingIter
{
    const InternalBindingsHandle bindings_;
    uint32_t i_;

    friend class Bindings;

  public:
    explicit BindingIter(const InternalBindingsHandle &bindings) : bindings_(bindings), i_(0) {}
    explicit BindingIter(const HandleScript &script) : bindings_(script, &script->bindings), i_(0) {}

    bool done() const { return i_ == bindings_->count(); }
    operator bool() const { return !done(); }
    void operator++(int) { JS_ASSERT(!done()); i_++; }
    BindingIter &operator++() { (*this)++; return *this; }

    uint32_t frameIndex() const {
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



class LazyScript : public gc::BarrieredCell<LazyScript>
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
    uint32_t numInnerFunctions_ : 23;

    uint32_t generatorKindBits_:2;

    
    uint32_t strict_ : 1;
    uint32_t bindingsAccessedDynamically_ : 1;
    uint32_t hasDebuggerStatement_ : 1;
    uint32_t directlyInsideEval_:1;
    uint32_t usesArgumentsAndApply_:1;
    uint32_t hasBeenCloned_:1;
    uint32_t treatAsRunOnce_:1;

    
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

    inline JSFunction *functionDelazifying(JSContext *cx) const;
    JSFunction *functionNonDelazifying() const {
        return function_;
    }

    void initScript(JSScript *script);
    void resetScript();
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

    bool treatAsRunOnce() const {
        return treatAsRunOnce_;
    }
    void setTreatAsRunOnce() {
        treatAsRunOnce_ = true;
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

    void markChildren(JSTracer *trc);
    void finalize(js::FreeOp *fop);

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_LAZY_SCRIPT; }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
    {
        return mallocSizeOf(table_);
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
            return nullptr;
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
UnmarkScriptData(JSRuntime *rt);

extern void
SweepScriptData(JSRuntime *rt);

extern void
FreeScriptData(JSRuntime *rt);

struct ScriptAndCounts
{
    
    JSScript *script;
    ScriptCounts scriptCounts;

    PCCounts &getPCCounts(jsbytecode *pc) const {
        return scriptCounts.pcCountsVector[script->pcToOffset(pc)];
    }

    jit::IonScriptCounts *getIonCounts() const {
        return scriptCounts.ionCounts;
    }
};

struct GSNCache;

jssrcnote *
GetSrcNote(GSNCache &cache, JSScript *script, jsbytecode *pc);

} 

extern jssrcnote *
js_GetSrcNote(JSContext *cx, JSScript *script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(JSScript *script, unsigned lineno);

extern JS_FRIEND_API(unsigned)
js_GetScriptLineExtent(JSScript *script);

namespace js {

extern unsigned
PCToLineNumber(JSScript *script, jsbytecode *pc, unsigned *columnp = nullptr);

extern unsigned
PCToLineNumber(unsigned startLine, jssrcnote *notes, jsbytecode *code, jsbytecode *pc,
               unsigned *columnp = nullptr);










enum LineOption {
    CALLED_FROM_JSOP_EVAL,
    NOT_CALLED_FROM_JSOP_EVAL
};

extern void
CurrentScriptFileLineOrigin(JSContext *cx, JSScript **script,
                            const char **file, unsigned *linenop,
                            uint32_t *pcOffset, JSPrincipals **origin,
                            LineOption opt = NOT_CALLED_FROM_JSOP_EVAL);

bool
CloneFunctionScript(JSContext *cx, HandleFunction original, HandleFunction clone,
                    NewObjectKind newKind = GenericObject);








static inline JSPrincipals *
NormalizeOriginPrincipals(JSPrincipals *principals, JSPrincipals *originPrincipals)
{
    return originPrincipals ? originPrincipals : principals;
}

} 

#endif 
