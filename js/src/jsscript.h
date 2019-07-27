







#ifndef jsscript_h
#define jsscript_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"

#include "jsatom.h"
#include "jslock.h"
#include "jsopcode.h"
#include "jstypes.h"

#include "gc/Barrier.h"
#include "gc/Rooting.h"
#include "jit/IonCode.h"
#include "js/UbiNode.h"
#include "vm/NativeObject.h"
#include "vm/Shape.h"

namespace JS {
struct ScriptSourceInfo;
}

namespace js {

namespace jit {
    struct BaselineScript;
    struct IonScriptCounts;
}

# define ION_DISABLED_SCRIPT ((js::jit::IonScript*)0x1)
# define ION_COMPILING_SCRIPT ((js::jit::IonScript*)0x2)

# define BASELINE_DISABLED_SCRIPT ((js::jit::BaselineScript*)0x1)

class BreakpointSite;
class BindingIter;
class LazyScript;
class RegExpObject;
struct SourceCompressionTask;
class Shape;
class WatchpointMap;
class NestedScopeObject;

namespace frontend {
    struct BytecodeEmitter;
    class UpvarCookie;
}

}







enum JSTryNoteKind {
    JSTRY_CATCH,
    JSTRY_FINALLY,
    JSTRY_FOR_IN,
    JSTRY_FOR_OF,
    JSTRY_LOOP
};




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
    js::HeapValue*  vector;    
    uint32_t        length;
};

struct ObjectArray {
    js::HeapPtrObject* vector;  
    uint32_t        length;     
};

struct TryNoteArray {
    JSTryNote*      vector;    
    uint32_t        length;     
};

struct BlockScopeArray {
    BlockScopeNote* vector;     
    uint32_t        length;     
};

class YieldOffsetArray {
    uint32_t*       vector_;   
    uint32_t        length_;    

  public:
    void init(uint32_t* vector, uint32_t length) {
        vector_ = vector;
        length_ = length;
    }
    uint32_t& operator[](uint32_t index) {
        MOZ_ASSERT(index < length_);
        return vector_[index];
    }
    uint32_t length() const {
        return length_;
    }
};

class Binding
{
    
    
    uintptr_t bits_;

    static const uintptr_t KIND_MASK = 0x3;
    static const uintptr_t ALIASED_BIT = 0x4;
    static const uintptr_t NAME_MASK = ~(KIND_MASK | ALIASED_BIT);

  public:
    
    
    
    enum Kind { ARGUMENT, VARIABLE, CONSTANT };

    explicit Binding() : bits_(0) {}

    Binding(PropertyName* name, Kind kind, bool aliased) {
        JS_STATIC_ASSERT(CONSTANT <= KIND_MASK);
        MOZ_ASSERT((uintptr_t(name) & ~NAME_MASK) == 0);
        MOZ_ASSERT((uintptr_t(kind) & ~KIND_MASK) == 0);
        bits_ = uintptr_t(name) | uintptr_t(kind) | (aliased ? ALIASED_BIT : 0);
    }

    PropertyName* name() const {
        return (PropertyName*)(bits_ & NAME_MASK);
    }

    Kind kind() const {
        return Kind(bits_ & KIND_MASK);
    }

    bool aliased() const {
        return bool(bits_ & ALIASED_BIT);
    }
};

JS_STATIC_ASSERT(sizeof(Binding) == sizeof(uintptr_t));

class Bindings;
typedef InternalHandle<Bindings*> InternalBindingsHandle;







class Bindings
{
    friend class BindingIter;
    friend class AliasedFormalIter;

    RelocatablePtrShape callObjShape_;
    uintptr_t bindingArrayAndFlag_;
    uint16_t numArgs_;
    uint16_t numBlockScoped_;
    uint16_t numBodyLevelLexicals_;
    uint16_t aliasedBodyLevelLexicalBegin_;
    uint16_t numUnaliasedBodyLevelLexicals_;
    uint32_t numVars_;
    uint32_t numUnaliasedVars_;

#if JS_BITS_PER_WORD == 32
    
    
    uint32_t padding_;
#endif

    








    static const uintptr_t TEMPORARY_STORAGE_BIT = 0x1;
    bool bindingArrayUsingTemporaryStorage() const {
        return bindingArrayAndFlag_ & TEMPORARY_STORAGE_BIT;
    }

  public:

    Binding* bindingArray() const {
        return reinterpret_cast<Binding*>(bindingArrayAndFlag_ & ~TEMPORARY_STORAGE_BIT);
    }

    inline Bindings();

    






    static bool initWithTemporaryStorage(ExclusiveContext* cx, InternalBindingsHandle self,
                                         uint32_t numArgs, uint32_t numVars,
                                         uint32_t numBodyLevelLexicals, uint32_t numBlockScoped,
                                         uint32_t numUnaliasedVars, uint32_t numUnaliasedBodyLevelLexicals,
                                         Binding* bindingArray);

    
    
    
    
    
    
    void updateNumBlockScoped(unsigned numBlockScoped) {
        MOZ_ASSERT(!callObjShape_);
        MOZ_ASSERT(numVars_ == 0);
        MOZ_ASSERT(numBlockScoped < LOCALNO_LIMIT);
        MOZ_ASSERT(numBlockScoped >= numBlockScoped_);
        numBlockScoped_ = numBlockScoped;
    }

    void setAllLocalsAliased() {
        numBlockScoped_ = 0;
    }

    uint8_t* switchToScriptStorage(Binding* newStorage);

    



    static bool clone(JSContext* cx, InternalBindingsHandle self, uint8_t* dstScriptData,
                      HandleScript srcScript);

    uint32_t numArgs() const { return numArgs_; }
    uint32_t numVars() const { return numVars_; }
    uint32_t numBodyLevelLexicals() const { return numBodyLevelLexicals_; }
    uint32_t numBlockScoped() const { return numBlockScoped_; }
    uint32_t numBodyLevelLocals() const { return numVars_ + numBodyLevelLexicals_; }
    uint32_t numUnaliasedBodyLevelLocals() const { return numUnaliasedVars_ + numUnaliasedBodyLevelLexicals_; }
    uint32_t numAliasedBodyLevelLocals() const { return numBodyLevelLocals() - numUnaliasedBodyLevelLocals(); }
    uint32_t numLocals() const { return numVars() + numBodyLevelLexicals() + numBlockScoped(); }
    uint32_t numFixedLocals() const { return numUnaliasedVars() + numUnaliasedBodyLevelLexicals() + numBlockScoped(); }
    uint32_t lexicalBegin() const { return numArgs() + numVars(); }
    uint32_t aliasedBodyLevelLexicalBegin() const { return aliasedBodyLevelLexicalBegin_; }

    uint32_t numUnaliasedVars() const { return numUnaliasedVars_; }
    uint32_t numUnaliasedBodyLevelLexicals() const { return numUnaliasedBodyLevelLexicals_; }

    
    uint32_t count() const { return numArgs() + numVars() + numBodyLevelLexicals(); }

    
    Shape* callObjShape() const { return callObjShape_; }

    
    static BindingIter argumentsBinding(ExclusiveContext* cx, InternalBindingsHandle);

    
    bool bindingIsAliased(uint32_t bindingIndex);

    
    bool hasAnyAliasedBindings() const {
        if (!callObjShape_)
            return false;

        return !callObjShape_->isEmptyShape();
    }

    Binding* begin() const { return bindingArray(); }
    Binding* end() const { return bindingArray() + count(); }

    static js::ThingRootKind rootKind() { return js::THING_ROOT_BINDINGS; }
    void trace(JSTracer* trc);
};

template <>
struct GCMethods<Bindings> {
    static Bindings initial();
};

class ScriptCounts
{
    friend class ::JSScript;
    friend struct ScriptAndCounts;

    




    PCCounts* pcCountsVector;

    
    jit::IonScriptCounts* ionCounts;

 public:
    ScriptCounts() : pcCountsVector(nullptr), ionCounts(nullptr) { }

    inline void destroy(FreeOp* fop);

    void set(js::ScriptCounts counts) {
        pcCountsVector = counts.pcCountsVector;
        ionCounts = counts.ionCounts;
    }
};

typedef HashMap<JSScript*,
                ScriptCounts,
                DefaultHasher<JSScript*>,
                SystemAllocPolicy> ScriptCountsMap;

class DebugScript
{
    friend class ::JSScript;

    






    uint32_t        stepMode;

    



    uint32_t        numSites;

    





    BreakpointSite* breakpoints[1];
};

typedef HashMap<JSScript*,
                DebugScript*,
                DefaultHasher<JSScript*>,
                SystemAllocPolicy> DebugScriptMap;

class ScriptSource;

class UncompressedSourceCache
{
    typedef HashMap<ScriptSource*,
                    const char16_t*,
                    DefaultHasher<ScriptSource*>,
                    SystemAllocPolicy> Map;

  public:
    
    class AutoHoldEntry
    {
        UncompressedSourceCache* cache_;
        ScriptSource* source_;
        const char16_t* charsToFree_;
      public:
        explicit AutoHoldEntry();
        ~AutoHoldEntry();
      private:
        void holdEntry(UncompressedSourceCache* cache, ScriptSource* source);
        void deferDelete(const char16_t* chars);
        ScriptSource* source() const { return source_; }
        friend class UncompressedSourceCache;
    };

  private:
    Map* map_;
    AutoHoldEntry* holder_;

  public:
    UncompressedSourceCache() : map_(nullptr), holder_(nullptr) {}

    const char16_t* lookup(ScriptSource* ss, AutoHoldEntry& asp);
    bool put(ScriptSource* ss, const char16_t* chars, AutoHoldEntry& asp);

    void purge();

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);

  private:
    void holdEntry(AutoHoldEntry& holder, ScriptSource* ss);
    void releaseEntry(AutoHoldEntry& holder);
};

class ScriptSource
{
    friend struct SourceCompressionTask;

    uint32_t refs;

    
    
    

    
    enum {
        DataMissing,
        DataUncompressed,
        DataCompressed,
        DataParent
    } dataType;

    union {
        struct {
            const char16_t* chars;
            bool ownsChars;
        } uncompressed;

        struct {
            void* raw;
            size_t nbytes;
            HashNumber hash;
        } compressed;

        ScriptSource* parent;
    } data;

    uint32_t length_;

    
    mozilla::UniquePtr<char[], JS::FreePolicy> filename_;

    mozilla::UniquePtr<char16_t[], JS::FreePolicy> displayURL_;
    mozilla::UniquePtr<char16_t[], JS::FreePolicy> sourceMapURL_;
    bool mutedErrors_;

    
    
    
    uint32_t introductionOffset_;

    
    
    
    
    
    
    
    
    
    
    mozilla::UniquePtr<char[], JS::FreePolicy> introducerFilename_;

    
    
    
    
    
    
    
    
    
    
    
    const char* introductionType_;

    
    
    
    bool sourceRetrievable_:1;
    bool argumentsNotIncluded_:1;
    bool hasIntroductionOffset_:1;

    
    bool inCompressedSourceSet:1;

  public:
    explicit ScriptSource()
      : refs(0),
        dataType(DataMissing),
        length_(0),
        filename_(nullptr),
        displayURL_(nullptr),
        sourceMapURL_(nullptr),
        mutedErrors_(false),
        introductionOffset_(0),
        introducerFilename_(nullptr),
        introductionType_(nullptr),
        sourceRetrievable_(false),
        argumentsNotIncluded_(false),
        hasIntroductionOffset_(false),
        inCompressedSourceSet(false)
    {
    }
    ~ScriptSource();
    void incref() { refs++; }
    void decref() {
        MOZ_ASSERT(refs != 0);
        if (--refs == 0)
            js_delete(this);
    }
    bool initFromOptions(ExclusiveContext* cx, const ReadOnlyCompileOptions& options);
    bool setSourceCopy(ExclusiveContext* cx,
                       JS::SourceBufferHolder& srcBuf,
                       bool argumentsNotIncluded,
                       SourceCompressionTask* tok);
    void setSourceRetrievable() { sourceRetrievable_ = true; }
    bool sourceRetrievable() const { return sourceRetrievable_; }
    bool hasSourceData() const { return dataType != DataMissing; }
    bool hasCompressedSource() const { return dataType == DataCompressed; }
    size_t length() const {
        MOZ_ASSERT(hasSourceData());
        return length_;
    }
    bool argumentsNotIncluded() const {
        MOZ_ASSERT(hasSourceData());
        return argumentsNotIncluded_;
    }
    const char16_t* chars(JSContext* cx, UncompressedSourceCache::AutoHoldEntry& asp);
    JSFlatString* substring(JSContext* cx, uint32_t start, uint32_t stop);
    JSFlatString* substringDontDeflate(JSContext* cx, uint32_t start, uint32_t stop);
    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                JS::ScriptSourceInfo* info) const;

    const char16_t* uncompressedChars() const {
        MOZ_ASSERT(dataType == DataUncompressed);
        return data.uncompressed.chars;
    }

    bool ownsUncompressedChars() const {
        MOZ_ASSERT(dataType == DataUncompressed);
        return data.uncompressed.ownsChars;
    }

    void* compressedData() const {
        MOZ_ASSERT(dataType == DataCompressed);
        return data.compressed.raw;
    }

    size_t compressedBytes() const {
        MOZ_ASSERT(dataType == DataCompressed);
        return data.compressed.nbytes;
    }

    HashNumber compressedHash() const {
        MOZ_ASSERT(dataType == DataCompressed);
        return data.compressed.hash;
    }

    ScriptSource* parent() const {
        MOZ_ASSERT(dataType == DataParent);
        return data.parent;
    }

    void setSource(const char16_t* chars, size_t length, bool ownsChars = true);
    void setCompressedSource(JSRuntime* maybert, void* raw, size_t nbytes, HashNumber hash);
    void updateCompressedSourceSet(JSRuntime* rt);
    bool ensureOwnsSource(ExclusiveContext* cx);

    
    template <XDRMode mode>
    bool performXDR(XDRState<mode>* xdr);

    bool setFilename(ExclusiveContext* cx, const char* filename);
    const char* introducerFilename() const {
        return introducerFilename_ ? introducerFilename_.get() : filename_.get();
    }
    bool hasIntroductionType() const {
        return introductionType_;
    }
    const char* introductionType() const {
        MOZ_ASSERT(hasIntroductionType());
        return introductionType_;
    }
    const char* filename() const {
        return filename_.get();
    }

    
    bool setDisplayURL(ExclusiveContext* cx, const char16_t* displayURL);
    bool hasDisplayURL() const { return displayURL_ != nullptr; }
    const char16_t * displayURL() {
        MOZ_ASSERT(hasDisplayURL());
        return displayURL_.get();
    }

    
    bool setSourceMapURL(ExclusiveContext* cx, const char16_t* sourceMapURL);
    bool hasSourceMapURL() const { return sourceMapURL_ != nullptr; }
    const char16_t * sourceMapURL() {
        MOZ_ASSERT(hasSourceMapURL());
        return sourceMapURL_.get();
    }

    bool mutedErrors() const { return mutedErrors_; }

    bool hasIntroductionOffset() const { return hasIntroductionOffset_; }
    uint32_t introductionOffset() const {
        MOZ_ASSERT(hasIntroductionOffset());
        return introductionOffset_;
    }
    void setIntroductionOffset(uint32_t offset) {
        MOZ_ASSERT(!hasIntroductionOffset());
        MOZ_ASSERT(offset <= (uint32_t)INT32_MAX);
        introductionOffset_ = offset;
        hasIntroductionOffset_ = true;
    }

  private:
    size_t computedSizeOfData() const;
};

class ScriptSourceHolder
{
    ScriptSource* ss;
  public:
    explicit ScriptSourceHolder(ScriptSource* ss)
      : ss(ss)
    {
        ss->incref();
    }
    ~ScriptSourceHolder()
    {
        ss->decref();
    }
};

struct CompressedSourceHasher
{
    typedef ScriptSource* Lookup;

    static HashNumber computeHash(const void* data, size_t nbytes) {
        return mozilla::HashBytes(data, nbytes);
    }

    static HashNumber hash(const ScriptSource* ss) {
        return ss->compressedHash();
    }

    static bool match(const ScriptSource* a, const ScriptSource* b) {
        return a->compressedBytes() == b->compressedBytes() &&
               a->compressedHash() == b->compressedHash() &&
               !memcmp(a->compressedData(), b->compressedData(), a->compressedBytes());
    }
};

typedef HashSet<ScriptSource*, CompressedSourceHasher, SystemAllocPolicy> CompressedSourceSet;

class ScriptSourceObject : public NativeObject
{
  public:
    static const Class class_;

    static void trace(JSTracer* trc, JSObject* obj);
    static void finalize(FreeOp* fop, JSObject* obj);
    static ScriptSourceObject* create(ExclusiveContext* cx, ScriptSource* source);

    
    
    static bool initFromOptions(JSContext* cx, HandleScriptSource source,
                                const ReadOnlyCompileOptions& options);

    ScriptSource* source() const {
        return static_cast<ScriptSource*>(getReservedSlot(SOURCE_SLOT).toPrivate());
    }
    JSObject* element() const {
        return getReservedSlot(ELEMENT_SLOT).toObjectOrNull();
    }
    const Value& elementAttributeName() const {
        MOZ_ASSERT(!getReservedSlot(ELEMENT_PROPERTY_SLOT).isMagic());
        return getReservedSlot(ELEMENT_PROPERTY_SLOT);
    }
    JSScript* introductionScript() const {
        if (getReservedSlot(INTRODUCTION_SCRIPT_SLOT).isUndefined())
            return nullptr;
        void* untyped = getReservedSlot(INTRODUCTION_SCRIPT_SLOT).toPrivate();
        MOZ_ASSERT(untyped);
        return static_cast<JSScript*>(untyped);
    }

  private:
    static const uint32_t SOURCE_SLOT = 0;
    static const uint32_t ELEMENT_SLOT = 1;
    static const uint32_t ELEMENT_PROPERTY_SLOT = 2;
    static const uint32_t INTRODUCTION_SCRIPT_SLOT = 3;
    static const uint32_t RESERVED_SLOTS = 4;
};

enum GeneratorKind { NotGenerator, LegacyGenerator, StarGenerator };

static inline unsigned
GeneratorKindAsBits(GeneratorKind generatorKind) {
    return static_cast<unsigned>(generatorKind);
}

static inline GeneratorKind
GeneratorKindFromBits(unsigned val) {
    MOZ_ASSERT(val <= StarGenerator);
    return static_cast<GeneratorKind>(val);
}






template<XDRMode mode>
bool
XDRScript(XDRState<mode>* xdr, HandleObject enclosingScope, HandleScript enclosingScript,
          HandleFunction fun, MutableHandleScript scriptp);

enum PollutedGlobalScopeOption {
    HasPollutedGlobalScope,
    HasCleanGlobalScope
};

JSScript*
CloneScript(JSContext* cx, HandleObject enclosingScope, HandleFunction fun, HandleScript script,
            PollutedGlobalScopeOption polluted = HasCleanGlobalScope,
            NewObjectKind newKind = GenericObject);

template<XDRMode mode>
bool
XDRLazyScript(XDRState<mode>* xdr, HandleObject enclosingScope, HandleScript enclosingScript,
              HandleFunction fun, MutableHandle<LazyScript*> lazy);




template<XDRMode mode>
bool
XDRScriptConst(XDRState<mode>* xdr, MutableHandleValue vp);

} 

class JSScript : public js::gc::TenuredCell
{
    template <js::XDRMode mode>
    friend
    bool
    js::XDRScript(js::XDRState<mode>* xdr, js::HandleObject enclosingScope, js::HandleScript enclosingScript,
                  js::HandleFunction fun, js::MutableHandleScript scriptp);

    friend JSScript*
    js::CloneScript(JSContext* cx, js::HandleObject enclosingScope, js::HandleFunction fun,
                    js::HandleScript src, js::PollutedGlobalScopeOption polluted,
                    js::NewObjectKind newKind);

  public:
    
    
    
    

    

  public:
    js::Bindings    bindings;   


    bool hasAnyAliasedBindings() const {
        return bindings.hasAnyAliasedBindings();
    }

    js::Binding* bindingArray() const {
        return bindings.bindingArray();
    }

    unsigned numArgs() const {
        return bindings.numArgs();
    }

    js::Shape* callObjShape() const {
        return bindings.callObjShape();
    }

    

  private:
    jsbytecode*     code_;     
  public:
    uint8_t*        data;      


    js::HeapPtrAtom* atoms;     

    JSCompartment*  compartment_;

  private:
    
    js::TypeScript* types_;

    
    
    
    
    js::HeapPtrObject sourceObject_;

    js::HeapPtrFunction function_;
    js::HeapPtrObject   enclosingStaticScope_;

    
    js::jit::IonScript* ion;
    js::jit::BaselineScript* baseline;

    
    js::LazyScript* lazyScript;

    



    uint8_t* baselineOrIonRaw;
    uint8_t* baselineOrIonSkipArgCheck;

    

    uint32_t        length_;    
    uint32_t        dataSize_;  

    uint32_t        lineno_;    
    uint32_t        column_;    

    uint32_t        mainOffset_;


    uint32_t        natoms_;    
    uint32_t        nslots_;    

    
    uint32_t        sourceStart_;
    uint32_t        sourceEnd_;

    uint32_t        warmUpCount; 





    

    uint16_t        warmUpResetCount; 



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

    
    
    
    bool hasPollutedGlobalScope_:1;

    
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

    
    bool usesArgumentsApplyAndThis_:1;

    
    bool failedBoundsCheck_:1; 
    bool failedShapeGuard_:1; 
    bool hadFrequentBailouts_:1;
    bool uninlineable_:1;    

    
    bool invalidatedIdempotentCache_:1;

    
    bool failedLexicalCheck_:1;

    
    
    bool isGeneratorExp_:1;

    
    bool hasScriptCounts_:1;

    
    bool hasDebugScript_:1;

    
    bool hasFreezeConstraints_:1;

    
    bool argsHasVarBinding_:1;
    bool needsArgsAnalysis_:1;
    bool needsArgsObj_:1;

    
    
    
    
    
    bool typesGeneration_:1;

    
    
    
    
    
    
    bool doNotRelazify_:1;

    bool needsHomeObject_:1;

    
    
  protected:
#if JS_BITS_PER_WORD == 32
    uint32_t padding;
#endif

    
    
    

  public:
    static JSScript* Create(js::ExclusiveContext* cx,
                            js::HandleObject enclosingScope, bool savedCallerFun,
                            const JS::ReadOnlyCompileOptions& options, unsigned staticLevel,
                            js::HandleObject sourceObject, uint32_t sourceStart,
                            uint32_t sourceEnd);

    void initCompartment(js::ExclusiveContext* cx);

    
    
    
    
    static bool partiallyInit(js::ExclusiveContext* cx, JS::Handle<JSScript*> script,
                              uint32_t nconsts, uint32_t nobjects, uint32_t nregexps,
                              uint32_t ntrynotes, uint32_t nblockscopes, uint32_t nyieldoffsets,
                              uint32_t nTypeSets);
    static bool fullyInitFromEmitter(js::ExclusiveContext* cx, JS::Handle<JSScript*> script,
                                     js::frontend::BytecodeEmitter* bce);
    
    static bool fullyInitTrivial(js::ExclusiveContext* cx, JS::Handle<JSScript*> script);

    inline JSPrincipals* principals();

    JSCompartment* compartment() const { return compartment_; }

    void setVersion(JSVersion v) { version = v; }

    
    jsbytecode* code() const {
        return code_;
    }
    size_t length() const {
        return length_;
    }

    void setCode(jsbytecode* code) { code_ = code; }
    void setLength(size_t length) { length_ = length; }

    jsbytecode* codeEnd() const { return code() + length(); }

    jsbytecode* lastPC() const {
        jsbytecode* pc = codeEnd() - js::JSOP_RETRVAL_LENGTH;
        MOZ_ASSERT(*pc == JSOP_RETRVAL);
        return pc;
    }

    bool containsPC(const jsbytecode* pc) const {
        return pc >= code() && pc < codeEnd();
    }

    size_t pcToOffset(const jsbytecode* pc) const {
        MOZ_ASSERT(containsPC(pc));
        return size_t(pc - code());
    }

    jsbytecode* offsetToPC(size_t offset) const {
        MOZ_ASSERT(offset < length());
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
        return function_ ? bindings.numFixedLocals() : bindings.numBlockScoped();
    }

    
    
    size_t nfixedvars() const {
        return function_ ? bindings.numUnaliasedVars() : 0;
    }

    
    
    
    size_t nbodyfixed() const {
        return function_ ? bindings.numUnaliasedBodyLevelLocals() : 0;
    }

    
    size_t fixedLexicalBegin() const {
        return nfixedvars();
    }

    size_t fixedLexicalEnd() const {
        return nfixed();
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

    bool hasPollutedGlobalScope() const {
        return hasPollutedGlobalScope_;
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
        MOZ_ASSERT(isActiveEval() && !isCachedEval());
        isActiveEval_ = false;
        isCachedEval_ = true;
        
        
        
        hasRunOnce_ = false;
    }

    void uncacheForEval() {
        MOZ_ASSERT(isCachedEval() && !isActiveEval());
        isCachedEval_ = false;
        isActiveEval_ = true;
    }

    void setActiveEval() { isActiveEval_ = true; }
    void setDirectlyInsideEval() { directlyInsideEval_ = true; }

    bool usesArgumentsApplyAndThis() const {
        return usesArgumentsApplyAndThis_;
    }
    void setUsesArgumentsApplyAndThis() { usesArgumentsApplyAndThis_ = true; }

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
    bool failedLexicalCheck() const {
        return failedLexicalCheck_;
    }

    void setFailedBoundsCheck() { failedBoundsCheck_ = true; }
    void setFailedShapeGuard() { failedShapeGuard_ = true; }
    void setHadFrequentBailouts() { hadFrequentBailouts_ = true; }
    void setUninlineable() { uninlineable_ = true; }
    void setInvalidatedIdempotentCache() { invalidatedIdempotentCache_ = true; }
    void setFailedLexicalCheck() { failedLexicalCheck_ = true; }

    bool hasScriptCounts() const { return hasScriptCounts_; }

    bool hasFreezeConstraints() const { return hasFreezeConstraints_; }
    void setHasFreezeConstraints() { hasFreezeConstraints_ = true; }

    bool warnedAboutUndefinedProp() const { return warnedAboutUndefinedProp_; }
    void setWarnedAboutUndefinedProp() { warnedAboutUndefinedProp_ = true; }

    
    bool argumentsHasVarBinding() const {
        return argsHasVarBinding_;
    }
    jsbytecode* argumentsBytecode() const { MOZ_ASSERT(code()[0] == JSOP_ARGUMENTS); return code(); }
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
        
        
        MOZ_ASSERT(!isGenerator());
        generatorKindBits_ = GeneratorKindAsBits(kind);
    }

    void setNeedsHomeObject() {
        needsHomeObject_ = true;
    }
    bool needsHomeObject() const {
        return needsHomeObject_;
    }


    









    bool analyzedArgsUsage() const { return !needsArgsAnalysis_; }
    inline bool ensureHasAnalyzedArgsUsage(JSContext* cx);
    bool needsArgsObj() const {
        MOZ_ASSERT(analyzedArgsUsage());
        return needsArgsObj_;
    }
    void setNeedsArgsObj(bool needsArgsObj);
    static bool argumentsOptimizationFailed(JSContext* cx, js::HandleScript script);

    








    bool argsObjAliasesFormals() const {
        return needsArgsObj() && !strict();
    }

    uint32_t typesGeneration() const {
        return (uint32_t) typesGeneration_;
    }

    void setTypesGeneration(uint32_t generation) {
        MOZ_ASSERT(generation <= 1);
        typesGeneration_ = (bool) generation;
    }

    void setDoNotRelazify(bool b) {
        doNotRelazify_ = b;
    }

    bool hasAnyIonScript() const {
        return hasIonScript();
    }

    bool hasIonScript() const {
        bool res = ion && ion != ION_DISABLED_SCRIPT && ion != ION_COMPILING_SCRIPT;
        MOZ_ASSERT_IF(res, baseline);
        return res;
    }
    bool canIonCompile() const {
        return ion != ION_DISABLED_SCRIPT;
    }

    bool isIonCompilingOffThread() const {
        return ion == ION_COMPILING_SCRIPT;
    }

    js::jit::IonScript* ionScript() const {
        MOZ_ASSERT(hasIonScript());
        return ion;
    }
    js::jit::IonScript* maybeIonScript() const {
        return ion;
    }
    js::jit::IonScript* const* addressOfIonScript() const {
        return &ion;
    }
    void setIonScript(JSContext* maybecx, js::jit::IonScript* ionScript) {
        if (hasIonScript())
            js::jit::IonScript::writeBarrierPre(zone(), ion);
        ion = ionScript;
        resetWarmUpResetCounter();
        MOZ_ASSERT_IF(hasIonScript(), hasBaselineScript());
        updateBaselineOrIonRaw(maybecx);
    }

    bool hasBaselineScript() const {
        bool res = baseline && baseline != BASELINE_DISABLED_SCRIPT;
        MOZ_ASSERT_IF(!res, !ion || ion == ION_DISABLED_SCRIPT);
        return res;
    }
    bool canBaselineCompile() const {
        return baseline != BASELINE_DISABLED_SCRIPT;
    }
    js::jit::BaselineScript* baselineScript() const {
        MOZ_ASSERT(hasBaselineScript());
        return baseline;
    }
    inline void setBaselineScript(JSContext* maybecx, js::jit::BaselineScript* baselineScript);

    void updateBaselineOrIonRaw(JSContext* maybecx);

    void setPendingIonBuilder(JSContext* maybecx, js::jit::IonBuilder* builder) {
        MOZ_ASSERT(!builder || !ion->pendingBuilder());
        ion->setPendingBuilderPrivate(builder);
        updateBaselineOrIonRaw(maybecx);
    }
    js::jit::IonBuilder* pendingIonBuilder() {
        MOZ_ASSERT(hasIonScript());
        return ion->pendingBuilder();
    }

    static size_t offsetOfBaselineScript() {
        return offsetof(JSScript, baseline);
    }
    static size_t offsetOfIonScript() {
        return offsetof(JSScript, ion);
    }
    static size_t offsetOfBaselineOrIonRaw() {
        return offsetof(JSScript, baselineOrIonRaw);
    }
    uint8_t* baselineOrIonRawPointer() const {
        return baselineOrIonRaw;
    }
    static size_t offsetOfBaselineOrIonSkipArgCheck() {
        return offsetof(JSScript, baselineOrIonSkipArgCheck);
    }

    bool isRelazifiable() const {
        return (selfHosted() || lazyScript) && !types_ &&
               !isGenerator() && !hasBaselineScript() && !hasAnyIonScript() &&
               !hasScriptCounts() && !doNotRelazify_;
    }
    void setLazyScript(js::LazyScript* lazy) {
        lazyScript = lazy;
    }
    js::LazyScript* maybeLazyScript() {
        return lazyScript;
    }

    







    inline JSFunction* functionDelazifying() const;
    JSFunction* functionNonDelazifying() const {
        return function_;
    }
    inline void setFunction(JSFunction* fun);
    



    inline void ensureNonLazyCanonicalFunction(JSContext* cx);

    JSFlatString* sourceData(JSContext* cx);

    static bool loadSource(JSContext* cx, js::ScriptSource* ss, bool* worked);

    void setSourceObject(JSObject* object);
    JSObject* sourceObject() const {
        return sourceObject_;
    }
    js::ScriptSourceObject& scriptSourceUnwrap() const;
    js::ScriptSource* scriptSource() const;
    js::ScriptSource* maybeForwardedScriptSource() const;
    bool mutedErrors() const { return scriptSource()->mutedErrors(); }
    const char* filename() const { return scriptSource()->filename(); }
    const char* maybeForwardedFilename() const { return maybeForwardedScriptSource()->filename(); }

  public:

    
    bool isForEval() { return isCachedEval() || isActiveEval(); }

    
    inline bool ensureHasTypes(JSContext* cx);

    inline js::TypeScript* types();

    void maybeSweepTypes(js::AutoClearTypeInferenceStateOnOOM* oom);

    inline js::GlobalObject& global() const;
    js::GlobalObject& uninlinedGlobal() const;

    
    JSObject* enclosingStaticScope() const {
        return enclosingStaticScope_;
    }

  private:
    bool makeTypes(JSContext* cx);

  public:
    uint32_t getWarmUpCount() const { return warmUpCount; }
    uint32_t incWarmUpCounter(uint32_t amount = 1) { return warmUpCount += amount; }
    uint32_t* addressOfWarmUpCounter() { return &warmUpCount; }
    static size_t offsetOfWarmUpCounter() { return offsetof(JSScript, warmUpCount); }
    void resetWarmUpCounter() { incWarmUpResetCounter(); warmUpCount = 0; }

    uint16_t getWarmUpResetCount() const { return warmUpResetCount; }
    uint16_t incWarmUpResetCounter(uint16_t amount = 1) { return warmUpResetCount += amount; }
    void resetWarmUpResetCounter() { warmUpResetCount = 0; }

  public:
    bool initScriptCounts(JSContext* cx);
    js::PCCounts getPCCounts(jsbytecode* pc);
    void addIonCounts(js::jit::IonScriptCounts* ionCounts);
    js::jit::IonScriptCounts* getIonCounts();
    js::ScriptCounts releaseScriptCounts();
    void destroyScriptCounts(js::FreeOp* fop);

    jsbytecode* main() {
        return code() + mainOffset();
    }

    




    size_t computedSizeOfData() const;
    size_t sizeOfData(mozilla::MallocSizeOf mallocSizeOf) const;
    size_t sizeOfTypeScript(mozilla::MallocSizeOf mallocSizeOf) const;

    uint32_t numNotes();  

    
    jssrcnote* notes() { return (jssrcnote*)(code() + length()); }

    bool hasArray(ArrayKind kind) {
        return hasArrayBits & (1 << kind);
    }
    void setHasArray(ArrayKind kind) { hasArrayBits |= (1 << kind); }
    void cloneHasArray(JSScript* script) { hasArrayBits = script->hasArrayBits; }

    bool hasConsts()        { return hasArray(CONSTS);      }
    bool hasObjects()       { return hasArray(OBJECTS);     }
    bool hasRegexps()       { return hasArray(REGEXPS);     }
    bool hasTrynotes()      { return hasArray(TRYNOTES);    }
    bool hasBlockScopes()   { return hasArray(BLOCK_SCOPES); }
    bool hasYieldOffsets()  { return isGenerator(); }

    #define OFF(fooOff, hasFoo, t)   (fooOff() + (hasFoo() ? sizeof(t) : 0))

    size_t constsOffset()       { return 0; }
    size_t objectsOffset()      { return OFF(constsOffset,      hasConsts,      js::ConstArray);      }
    size_t regexpsOffset()      { return OFF(objectsOffset,     hasObjects,     js::ObjectArray);     }
    size_t trynotesOffset()     { return OFF(regexpsOffset,     hasRegexps,     js::ObjectArray);     }
    size_t blockScopesOffset()  { return OFF(trynotesOffset,    hasTrynotes,    js::TryNoteArray);    }
    size_t yieldOffsetsOffset() { return OFF(blockScopesOffset, hasBlockScopes, js::BlockScopeArray); }

    size_t dataSize() const { return dataSize_; }

    js::ConstArray* consts() {
        MOZ_ASSERT(hasConsts());
        return reinterpret_cast<js::ConstArray*>(data + constsOffset());
    }

    js::ObjectArray* objects() {
        MOZ_ASSERT(hasObjects());
        return reinterpret_cast<js::ObjectArray*>(data + objectsOffset());
    }

    js::ObjectArray* regexps() {
        MOZ_ASSERT(hasRegexps());
        return reinterpret_cast<js::ObjectArray*>(data + regexpsOffset());
    }

    js::TryNoteArray* trynotes() {
        MOZ_ASSERT(hasTrynotes());
        return reinterpret_cast<js::TryNoteArray*>(data + trynotesOffset());
    }

    js::BlockScopeArray* blockScopes() {
        MOZ_ASSERT(hasBlockScopes());
        return reinterpret_cast<js::BlockScopeArray*>(data + blockScopesOffset());
    }

    js::YieldOffsetArray& yieldOffsets() {
        MOZ_ASSERT(hasYieldOffsets());
        return *reinterpret_cast<js::YieldOffsetArray*>(data + yieldOffsetsOffset());
    }

    bool hasLoops();

    size_t natoms() const { return natoms_; }

    js::HeapPtrAtom& getAtom(size_t index) const {
        MOZ_ASSERT(index < natoms());
        return atoms[index];
    }

    js::HeapPtrAtom& getAtom(jsbytecode* pc) const {
        MOZ_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
        return getAtom(GET_UINT32_INDEX(pc));
    }

    js::PropertyName* getName(size_t index) {
        return getAtom(index)->asPropertyName();
    }

    js::PropertyName* getName(jsbytecode* pc) const {
        MOZ_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
        return getAtom(GET_UINT32_INDEX(pc))->asPropertyName();
    }

    JSObject* getObject(size_t index) {
        js::ObjectArray* arr = objects();
        MOZ_ASSERT(index < arr->length);
        MOZ_ASSERT(arr->vector[index]->isTenured());
        return arr->vector[index];
    }

    size_t innerObjectsStart() {
        
        return savedCallerFun() ? 1 : 0;
    }

    JSObject* getObject(jsbytecode* pc) {
        MOZ_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
        return getObject(GET_UINT32_INDEX(pc));
    }

    JSVersion getVersion() const {
        return JSVersion(version);
    }

    inline JSFunction* getFunction(size_t index);
    inline JSFunction* getCallerFunction();
    inline JSFunction* functionOrCallerFunction();

    inline js::RegExpObject* getRegExp(size_t index);
    inline js::RegExpObject* getRegExp(jsbytecode* pc);

    const js::Value& getConst(size_t index) {
        js::ConstArray* arr = consts();
        MOZ_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    js::NestedScopeObject* getStaticBlockScope(jsbytecode* pc);

    
    
    JSObject* innermostStaticScopeInScript(jsbytecode* pc);

    
    
    JSObject* innermostStaticScope(jsbytecode* pc);

    




    bool isEmpty() const {
        if (length() > 3)
            return false;

        jsbytecode* pc = code();
        if (noScriptRval() && JSOp(*pc) == JSOP_FALSE)
            ++pc;
        return JSOp(*pc) == JSOP_RETRVAL;
    }

    bool bindingIsAliased(const js::BindingIter& bi);
    bool formalIsAliased(unsigned argSlot);
    bool formalLivesInArgumentsObject(unsigned argSlot);

    
    bool cookieIsAliased(const js::frontend::UpvarCookie& cookie);

  private:
    
    void setNewStepMode(js::FreeOp* fop, uint32_t newValue);

    bool ensureHasDebugScript(JSContext* cx);
    js::DebugScript* debugScript();
    js::DebugScript* releaseDebugScript();
    void destroyDebugScript(js::FreeOp* fop);

  public:
    bool hasBreakpointsAt(jsbytecode* pc);
    bool hasAnyBreakpointsOrStepMode() { return hasDebugScript_; }

    
    
    inline bool isDebuggee() const;

    js::BreakpointSite* getBreakpointSite(jsbytecode* pc)
    {
        return hasDebugScript_ ? debugScript()->breakpoints[pcToOffset(pc)] : nullptr;
    }

    js::BreakpointSite* getOrCreateBreakpointSite(JSContext* cx, jsbytecode* pc);

    void destroyBreakpointSite(js::FreeOp* fop, jsbytecode* pc);

    void clearBreakpointsIn(js::FreeOp* fop, js::Debugger* dbg, JSObject* handler);

    





    bool incrementStepModeCount(JSContext* cx);
    void decrementStepModeCount(js::FreeOp* fop);

    bool stepModeEnabled() { return hasDebugScript_ && !!debugScript()->stepMode; }

#ifdef DEBUG
    uint32_t stepModeCount() { return hasDebugScript_ ? debugScript()->stepMode : 0; }
#endif

    void finalize(js::FreeOp* fop);
    void fixupAfterMovingGC() {}

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SCRIPT; }

    void markChildren(JSTracer* trc);

    
    
    class AutoDelazify;
    friend class AutoDelazify;

    class AutoDelazify
    {
        JS::RootedScript script_;
        JSContext* cx_;
        bool oldDoNotRelazify_;
      public:
        explicit AutoDelazify(JSContext* cx, JS::HandleFunction fun = JS::NullPtr())
            : script_(cx)
            , cx_(cx)
        {
            holdScript(fun);
        }

        ~AutoDelazify()
        {
            dropScript();
        }

        void operator=(JS::HandleFunction fun)
        {
            dropScript();
            holdScript(fun);
        }

        operator JS::HandleScript() const { return script_; }
        explicit operator bool() const { return script_; }

      private:
        void holdScript(JS::HandleFunction fun);

        void dropScript()
        {
            if (script_) {
                script_->setDoNotRelazify(oldDoNotRelazify_);
                script_ = nullptr;
            }
        }
    };
};


static_assert(sizeof(JSScript) % js::gc::CellSize == 0,
              "Size of JSScript must be an integral multiple of js::gc::CellSize");

namespace js {







class BindingIter
{
    const InternalBindingsHandle bindings_;
    uint32_t i_;
    uint32_t unaliasedLocal_;

    friend class ::JSScript;
    friend class Bindings;

  public:
    explicit BindingIter(const InternalBindingsHandle& bindings)
      : bindings_(bindings), i_(0), unaliasedLocal_(0) {}
    explicit BindingIter(const HandleScript& script)
      : bindings_(script, &script->bindings), i_(0), unaliasedLocal_(0) {}

    bool done() const { return i_ == bindings_->count(); }
    explicit operator bool() const { return !done(); }
    BindingIter& operator++() { (*this)++; return *this; }

    void operator++(int) {
        MOZ_ASSERT(!done());
        const Binding& binding = **this;
        if (binding.kind() != Binding::ARGUMENT && !binding.aliased())
            unaliasedLocal_++;
        i_++;
    }

    
    
    
    
    uint32_t frameIndex() const {
        MOZ_ASSERT(!done());
        if (i_ < bindings_->numArgs())
            return i_;
        MOZ_ASSERT(!(*this)->aliased());
        return unaliasedLocal_;
    }

    
    
    
    uint32_t argIndex() const {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(i_ < bindings_->numArgs());
        return i_;
    }
    uint32_t argOrLocalIndex() const {
        MOZ_ASSERT(!done());
        return i_ < bindings_->numArgs() ? i_ : i_ - bindings_->numArgs();
    }
    uint32_t localIndex() const {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(i_ >= bindings_->numArgs());
        return i_ - bindings_->numArgs();
    }
    bool isBodyLevelLexical() const {
        MOZ_ASSERT(!done());
        const Binding& binding = **this;
        return binding.kind() != Binding::ARGUMENT;
    }

    const Binding& operator*() const { MOZ_ASSERT(!done()); return bindings_->bindingArray()[i_]; }
    const Binding* operator->() const { MOZ_ASSERT(!done()); return &bindings_->bindingArray()[i_]; }
};






class AliasedFormalIter
{
    const Binding* begin_;
    const Binding* p_;
    const Binding* end_;
    unsigned slot_;

    void settle() {
        while (p_ != end_ && !p_->aliased())
            p_++;
    }

  public:
    explicit inline AliasedFormalIter(JSScript* script);

    bool done() const { return p_ == end_; }
    explicit operator bool() const { return !done(); }
    void operator++(int) { MOZ_ASSERT(!done()); p_++; slot_++; settle(); }

    const Binding& operator*() const { MOZ_ASSERT(!done()); return *p_; }
    const Binding* operator->() const { MOZ_ASSERT(!done()); return p_; }
    unsigned frameIndex() const { MOZ_ASSERT(!done()); return p_ - begin_; }
    unsigned scopeSlot() const { MOZ_ASSERT(!done()); return slot_; }
};



class LazyScript : public gc::TenuredCell
{
  public:
    class FreeVariable
    {
        
        uintptr_t bits_;

        static const uintptr_t HOISTED_USE_BIT = 0x1;
        static const uintptr_t MASK = ~HOISTED_USE_BIT;

      public:
        explicit FreeVariable()
          : bits_(0)
        { }

        explicit FreeVariable(JSAtom* name)
          : bits_(uintptr_t(name))
        {
            
            
            
            MOZ_ASSERT(!IsInsideNursery(name));
        }

        JSAtom* atom() const { return (JSAtom*)(bits_ & MASK); }
        void setIsHoistedUse() { bits_ |= HOISTED_USE_BIT; }
        bool isHoistedUse() const { return bool(bits_ & HOISTED_USE_BIT); }
    };

  private:
    
    
    HeapPtrScript script_;

    
    HeapPtrFunction function_;

    
    HeapPtrObject enclosingScope_;

    
    
    
    HeapPtrObject sourceObject_;

    
    void* table_;

    
    
  protected:
#if JS_BITS_PER_WORD == 32
    uint32_t padding;
#endif
  private:

    struct PackedView {
        
        uint32_t version : 8;

        uint32_t numFreeVariables : 24;
        uint32_t numInnerFunctions : 22;

        uint32_t generatorKindBits : 2;

        
        uint32_t strict : 1;
        uint32_t bindingsAccessedDynamically : 1;
        uint32_t hasDebuggerStatement : 1;
        uint32_t hasDirectEval : 1;
        uint32_t directlyInsideEval : 1;
        uint32_t usesArgumentsApplyAndThis : 1;
        uint32_t hasBeenCloned : 1;
        uint32_t treatAsRunOnce : 1;
    };

    union {
        PackedView p_;
        uint64_t packedFields_;
    };

    
    uint32_t begin_;
    uint32_t end_;
    uint32_t lineno_;
    uint32_t column_;

    LazyScript(JSFunction* fun, void* table, uint64_t packedFields,
               uint32_t begin, uint32_t end, uint32_t lineno, uint32_t column);

    
    
    
    static LazyScript* CreateRaw(ExclusiveContext* cx, HandleFunction fun,
                                 uint64_t packedData, uint32_t begin, uint32_t end,
                                 uint32_t lineno, uint32_t column);

  public:
    
    
    
    static LazyScript* CreateRaw(ExclusiveContext* cx, HandleFunction fun,
                                 uint32_t numFreeVariables, uint32_t numInnerFunctions,
                                 JSVersion version, uint32_t begin, uint32_t end,
                                 uint32_t lineno, uint32_t column);

    
    
    
    
    
    
    
    
    
    static LazyScript* Create(ExclusiveContext* cx, HandleFunction fun,
                              HandleScript script, HandleObject enclosingScope,
                              HandleScript sourceObjectScript,
                              uint64_t packedData, uint32_t begin, uint32_t end,
                              uint32_t lineno, uint32_t column);

    void initRuntimeFields(uint64_t packedFields);

    inline JSFunction* functionDelazifying(JSContext* cx) const;
    JSFunction* functionNonDelazifying() const {
        return function_;
    }

    void initScript(JSScript* script);
    void resetScript();
    JSScript* maybeScript() {
        return script_;
    }

    JSObject* enclosingScope() const {
        return enclosingScope_;
    }
    ScriptSourceObject* sourceObject() const;
    ScriptSource* scriptSource() const {
        return sourceObject()->source();
    }
    ScriptSource* maybeForwardedScriptSource() const;
    bool mutedErrors() const {
        return scriptSource()->mutedErrors();
    }
    JSVersion version() const {
        JS_STATIC_ASSERT(JSVERSION_UNKNOWN == -1);
        return (p_.version == JS_BIT(8) - 1) ? JSVERSION_UNKNOWN : JSVersion(p_.version);
    }

    void setParent(JSObject* enclosingScope, ScriptSourceObject* sourceObject);

    uint32_t numFreeVariables() const {
        return p_.numFreeVariables;
    }
    FreeVariable* freeVariables() {
        return (FreeVariable*)table_;
    }

    uint32_t numInnerFunctions() const {
        return p_.numInnerFunctions;
    }
    HeapPtrFunction* innerFunctions() {
        return (HeapPtrFunction*)&freeVariables()[numFreeVariables()];
    }

    GeneratorKind generatorKind() const { return GeneratorKindFromBits(p_.generatorKindBits); }

    bool isGenerator() const { return generatorKind() != NotGenerator; }

    bool isLegacyGenerator() const { return generatorKind() == LegacyGenerator; }

    bool isStarGenerator() const { return generatorKind() == StarGenerator; }

    void setGeneratorKind(GeneratorKind kind) {
        
        
        MOZ_ASSERT(!isGenerator());
        
        MOZ_ASSERT(kind != LegacyGenerator);
        p_.generatorKindBits = GeneratorKindAsBits(kind);
    }

    bool strict() const {
        return p_.strict;
    }
    void setStrict() {
        p_.strict = true;
    }

    bool bindingsAccessedDynamically() const {
        return p_.bindingsAccessedDynamically;
    }
    void setBindingsAccessedDynamically() {
        p_.bindingsAccessedDynamically = true;
    }

    bool hasDebuggerStatement() const {
        return p_.hasDebuggerStatement;
    }
    void setHasDebuggerStatement() {
        p_.hasDebuggerStatement = true;
    }

    bool hasDirectEval() const {
        return p_.hasDirectEval;
    }
    void setHasDirectEval() {
        p_.hasDirectEval = true;
    }

    bool directlyInsideEval() const {
        return p_.directlyInsideEval;
    }
    void setDirectlyInsideEval() {
        p_.directlyInsideEval = true;
    }

    bool usesArgumentsApplyAndThis() const {
        return p_.usesArgumentsApplyAndThis;
    }
    void setUsesArgumentsApplyAndThis() {
        p_.usesArgumentsApplyAndThis = true;
    }

    bool hasBeenCloned() const {
        return p_.hasBeenCloned;
    }
    void setHasBeenCloned() {
        p_.hasBeenCloned = true;
    }

    bool treatAsRunOnce() const {
        return p_.treatAsRunOnce;
    }
    void setTreatAsRunOnce() {
        p_.treatAsRunOnce = true;
    }

    const char* filename() const {
        return scriptSource()->filename();
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

    bool hasUncompiledEnclosingScript() const;
    uint32_t staticLevel(JSContext* cx) const;

    void markChildren(JSTracer* trc);
    void finalize(js::FreeOp* fop);
    void fixupAfterMovingGC() {}

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_LAZY_SCRIPT; }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
    {
        return mallocSizeOf(table_);
    }

    uint64_t packedFields() const {
        return packedFields_;
    }
};


JS_STATIC_ASSERT(sizeof(LazyScript) % js::gc::CellSize == 0);

struct SharedScriptData
{
    uint32_t length;
    uint32_t natoms;
    bool marked;
    jsbytecode data[1];

    static SharedScriptData* new_(ExclusiveContext* cx, uint32_t codeLength,
                                  uint32_t srcnotesLength, uint32_t natoms);

    HeapPtrAtom* atoms() {
        if (!natoms)
            return nullptr;
        return reinterpret_cast<HeapPtrAtom*>(data + length - sizeof(JSAtom*) * natoms);
    }

    static SharedScriptData* fromBytecode(const jsbytecode* bytecode) {
        return (SharedScriptData*)(bytecode - offsetof(SharedScriptData, data));
    }

  private:
    SharedScriptData() = delete;
    SharedScriptData(const SharedScriptData&) = delete;
};

struct ScriptBytecodeHasher
{
    struct Lookup
    {
        jsbytecode*         code;
        uint32_t            length;

        explicit Lookup(SharedScriptData* ssd) : code(ssd->data), length(ssd->length) {}
    };
    static HashNumber hash(const Lookup& l) { return mozilla::HashBytes(l.code, l.length); }
    static bool match(SharedScriptData* entry, const Lookup& lookup) {
        if (entry->length != lookup.length)
            return false;
        return mozilla::PodEqual<jsbytecode>(entry->data, lookup.code, lookup.length);
    }
};

typedef HashSet<SharedScriptData*,
                ScriptBytecodeHasher,
                SystemAllocPolicy> ScriptDataTable;

extern void
UnmarkScriptData(JSRuntime* rt);

extern void
SweepScriptData(JSRuntime* rt);

extern void
FreeScriptData(JSRuntime* rt);

struct ScriptAndCounts
{
    
    JSScript* script;
    ScriptCounts scriptCounts;

    PCCounts& getPCCounts(jsbytecode* pc) const {
        return scriptCounts.pcCountsVector[script->pcToOffset(pc)];
    }

    jit::IonScriptCounts* getIonCounts() const {
        return scriptCounts.ionCounts;
    }
};

struct GSNCache;

jssrcnote*
GetSrcNote(GSNCache& cache, JSScript* script, jsbytecode* pc);

extern jssrcnote*
GetSrcNote(JSContext* cx, JSScript* script, jsbytecode* pc);

extern jsbytecode*
LineNumberToPC(JSScript* script, unsigned lineno);

extern JS_FRIEND_API(unsigned)
GetScriptLineExtent(JSScript* script);

} 

namespace js {

extern unsigned
PCToLineNumber(JSScript* script, jsbytecode* pc, unsigned* columnp = nullptr);

extern unsigned
PCToLineNumber(unsigned startLine, jssrcnote* notes, jsbytecode* code, jsbytecode* pc,
               unsigned* columnp = nullptr);










enum LineOption {
    CALLED_FROM_JSOP_EVAL,
    NOT_CALLED_FROM_JSOP_EVAL
};

extern void
DescribeScriptedCallerForCompilation(JSContext* cx, MutableHandleScript maybeScript,
                                     const char** file, unsigned* linenop,
                                     uint32_t* pcOffset, bool* mutedErrors,
                                     LineOption opt = NOT_CALLED_FROM_JSOP_EVAL);

bool
CloneFunctionScript(JSContext* cx, HandleFunction original, HandleFunction clone,
                    PollutedGlobalScopeOption polluted, NewObjectKind newKind);

} 



namespace JS {
namespace ubi {
template<> struct Concrete<js::LazyScript> : TracerConcrete<js::LazyScript> { };
}
}

#endif 
