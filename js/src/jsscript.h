






#ifndef jsscript_h___
#define jsscript_h___



#include "jsprvtd.h"
#include "jsdbgapi.h"
#include "jsclist.h"
#include "jsinfer.h"
#include "jsopcode.h"
#include "jsscope.h"

#include "gc/Barrier.h"

namespace js {

struct Shape;
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

    





    bool initWithTemporaryStorage(JSContext *cx, unsigned numArgs, unsigned numVars, Binding *bindingArray);
    uint8_t *switchToScriptStorage(Binding *newStorage);

    



    bool clone(JSContext *cx, uint8_t *dstScriptData, HandleScript srcScript);

    unsigned numArgs() const { return numArgs_; }
    unsigned numVars() const { return numVars_; }
    unsigned count() const { return numArgs() + numVars(); }

    
    Shape *callObjShape() const { return callObjShape_; }

    
    unsigned argumentsVarIndex(JSContext *cx) const;

    
    bool bindingIsAliased(unsigned bindingIndex);

    
    bool hasAnyAliasedBindings() const { return !callObjShape_->isEmptyShape(); }

    void trace(JSTracer *trc);
    class AutoRooter;
};

class Bindings::AutoRooter : private AutoGCRooter
{
  public:
    explicit AutoRooter(JSContext *cx, Bindings *bindings_
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, BINDINGS), bindings(bindings_), skip(cx, bindings_)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    void trace(JSTracer *trc);

  private:
    Bindings *bindings;
    SkipRoot skip;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class ScriptCounts
{
    friend struct ::JSScript;
    friend struct ScriptAndCounts;
    




    PCCounts *pcCountsVector;

 public:
    ScriptCounts() : pcCountsVector(NULL) { }

    inline void destroy(FreeOp *fop);

    void set(js::ScriptCounts counts) {
        pcCountsVector = counts.pcCountsVector;
    }
};

typedef HashMap<JSScript *,
                ScriptCounts,
                DefaultHasher<JSScript *>,
                SystemAllocPolicy> ScriptCountsMap;

class DebugScript
{
    friend struct ::JSScript;

    






    uint32_t        stepMode;

    
    uint32_t        numSites;

    



    BreakpointSite  *breakpoints[1];
};

typedef HashMap<JSScript *,
                DebugScript *,
                DefaultHasher<JSScript *>,
                SystemAllocPolicy> DebugScriptMap;

struct ScriptSource;

} 

struct JSScript : public js::gc::Cell
{
  private:
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


    const char      *filename;  
    js::HeapPtrAtom *atoms;     

    JSPrincipals    *principals;
    JSPrincipals    *originPrincipals; 

    
    js::types::TypeScript *types;

  private:
    js::ScriptSource *scriptSource_; 
#ifdef JS_METHODJIT
    JITScriptSet *mJITInfo;
#endif
    js::HeapPtrFunction function_;
    js::HeapPtrObject   enclosingScope_;

    

  public:
    uint32_t        length;     

    uint32_t        lineno;     

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

    uint32_t        PADDING;

    

  private:
    uint16_t        version;    

  public:
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
    bool            strictModeCode:1; 
    bool            explicitUseStrict:1; 
    bool            compileAndGo:1;   
    bool            bindingsAccessedDynamically:1; 
    bool            funHasExtensibleScope:1;       
    bool            funHasAnyAliasedFormal:1;      
    bool            warnedAboutTwoArgumentEval:1; 


    bool            warnedAboutUndefinedProp:1; 


    bool            hasSingletons:1;  
    bool            isActiveEval:1;   
    bool            isCachedEval:1;   
    bool            uninlineable:1;   
#ifdef JS_METHODJIT
    bool            debugMode:1;      
    bool            failedBoundsCheck:1; 
#endif
    bool            isGenerator:1;    
    bool            isGeneratorExp:1; 
    bool            hasScriptCounts:1;

    bool            hasDebugScript:1; 

    bool            hasFreezeConstraints:1; 


  private:
    
    bool            argsHasVarBinding_:1;
    bool            needsArgsAnalysis_:1;
    bool            needsArgsObj_:1;

    
    
    

  public:
    static JSScript *Create(JSContext *cx, js::HandleObject enclosingScope, bool savedCallerFun,
                            const JS::CompileOptions &options, unsigned staticLevel,
                            js::ScriptSource *ss, uint32_t sourceStart, uint32_t sourceEnd);

    
    
    
    
    static bool partiallyInit(JSContext *cx, JS::Handle<JSScript*> script,
                              uint32_t length, uint32_t nsrcnotes, uint32_t natoms,
                              uint32_t nobjects, uint32_t nregexps, uint32_t ntrynotes, uint32_t nconsts,
                              uint32_t nTypeSets);
    static bool fullyInitTrivial(JSContext *cx, JS::Handle<JSScript*> script);  
    static bool fullyInitFromEmitter(JSContext *cx, JS::Handle<JSScript*> script,
                                     js::frontend::BytecodeEmitter *bce);

    void setVersion(JSVersion v) { version = v; }

    
    bool argumentsHasVarBinding() const { return argsHasVarBinding_; }
    jsbytecode *argumentsBytecode() const { JS_ASSERT(code[0] == JSOP_ARGUMENTS); return code; }
    void setArgumentsHasVarBinding();

    









    bool analyzedArgsUsage() const { return !needsArgsAnalysis_; }
    bool needsArgsObj() const { JS_ASSERT(analyzedArgsUsage()); return needsArgsObj_; }
    void setNeedsArgsObj(bool needsArgsObj);
    static bool argumentsOptimizationFailed(JSContext *cx, JSScript *script);

    








    bool argsObjAliasesFormals() const {
        return needsArgsObj() && !strictModeCode;
    }

    



    JSFunction *function() const { return function_; }
    void setFunction(JSFunction *fun);

    JSFixedString *sourceData(JSContext *cx);

    bool loadSource(JSContext *cx, bool *worked);

    js::ScriptSource *scriptSource() {
        return scriptSource_;
    }

    void setScriptSource(JSContext *cx, js::ScriptSource *ss);

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

    inline void clearPropertyReadTypes();

    inline bool hasGlobal() const;
    inline bool hasClearedGlobal() const;

    inline js::GlobalObject &global() const;

    
    JSObject *enclosingStaticScope() const {
        JS_ASSERT(enclosingScriptsCompiledSuccessfully());
        return enclosingScope_;
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
    uint32_t incUseCount() { return ++useCount; }
    uint32_t *addressOfUseCount() { return &useCount; }
    void resetUseCount() { useCount = 0; }

    




    size_t sizeOfJitScripts(JSMallocSizeOfFun mallocSizeOf);
#endif

  public:
    bool initScriptCounts(JSContext *cx);
    js::PCCounts getPCCounts(jsbytecode *pc);
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
    void cloneHasArray(JSScript *script)    { hasArrayBits = script->hasArrayBits; }

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

    inline JSObject *getRegExp(size_t index);

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
    bool hasBreakpointsAt(jsbytecode *pc) { return !!getBreakpointSite(pc); }
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

    static inline void writeBarrierPre(JSScript *script);
    static inline void writeBarrierPost(JSScript *script, void *addr);

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SCRIPT; }

    static JSPrincipals *normalizeOriginPrincipals(JSPrincipals *principals,
                                                   JSPrincipals *originPrincipals) {
        return originPrincipals ? originPrincipals : principals;
    }

    void markChildren(JSTracer *trc);
};

JS_STATIC_ASSERT(sizeof(JSScript::ArrayBitsT) * 8 >= JSScript::LIMIT);


JS_STATIC_ASSERT(sizeof(JSScript) % js::gc::Cell::CellSize == 0);

namespace js {







class BindingIter
{
    const Bindings *bindings_;
    unsigned i_;

    friend class Bindings;
    BindingIter(const Bindings &bindings, unsigned i) : bindings_(&bindings), i_(i) {}

  public:
    explicit BindingIter(const Bindings &bindings) : bindings_(&bindings), i_(0) {}

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
FillBindingVector(Bindings &bindings, BindingVector *vec);






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

}  








extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

namespace js {

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
    jschar *sourceMap_;

    
    
    
    bool sourceRetrievable_:1;
    bool argumentsNotIncluded_:1;
#ifdef DEBUG
    bool ready_:1;
#endif

  public:
    ScriptSource()
      : refs(0),
        length_(0),
        compressedLength_(0),
        sourceMap_(NULL),
        sourceRetrievable_(false),
        argumentsNotIncluded_(false)
#ifdef DEBUG
       ,ready_(true)
#endif
    {
        data.source = NULL;
    }
    void incref() { refs++; }
    void decref(JSRuntime *rt) {
        JS_ASSERT(refs != 0);
        if (--refs == 0)
            destroy(rt);
    }
    bool setSourceCopy(JSContext *cx,
                       const jschar *src,
                       uint32_t length,
                       bool argumentsNotIncluded,
                       SourceCompressionToken *tok);
    void setSource(const jschar *src, uint32_t length);
#ifdef DEBUG
    bool ready() const { return ready_; }
#endif
    void setSourceRetrievable() { sourceRetrievable_ = true; }
    bool sourceRetrievable() const { return sourceRetrievable_; }
    bool hasSourceData() const { return !!data.source; }
    uint32_t length() const {
        JS_ASSERT(hasSourceData());
        return length_;
    }
    bool argumentsNotIncluded() const {
        JS_ASSERT(hasSourceData());
        return argumentsNotIncluded_;
    }
    JSFixedString *substring(JSContext *cx, uint32_t start, uint32_t stop);
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf);

    
    template <XDRMode mode>
    bool performXDR(XDRState<mode> *xdr);

    
    bool setSourceMap(JSContext *cx, jschar *sourceMapURL, const char *filename);
    const jschar *sourceMap();
    bool hasSourceMap() const { return sourceMap_ != NULL; }

  private:
    void destroy(JSRuntime *rt);
    bool compressed() { return compressedLength_ != 0; }
};

class ScriptSourceHolder
{
    JSRuntime *rt;
    ScriptSource *ss;
  public:
    ScriptSourceHolder(JSRuntime *rt, ScriptSource *ss)
      : rt(rt),
        ss(ss)
    {
        ss->incref();
    }
    ~ScriptSourceHolder()
    {
        ss->decref(rt);
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

    void threadLoop();
    static void compressorThread(void *arg);

  public:
    explicit SourceCompressorThread(JSRuntime *rt)
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
  public:
    SourceCompressionToken(JSContext *cx)
      : cx(cx), ss(NULL), chars(NULL) {}
    ~SourceCompressionToken()
    {
        JS_ASSERT_IF(!ss, !chars);
        if (ss)
            ensureReady();
    }

    void ensureReady();
    void abort();
};

extern void
CallDestroyScriptHook(FreeOp *fop, JSScript *script);

extern const char *
SaveScriptFilename(JSContext *cx, const char *filename);

struct ScriptFilenameEntry
{
    bool marked;
    char filename[1];

    static ScriptFilenameEntry *fromFilename(const char *filename) {
        return (ScriptFilenameEntry *)(filename - offsetof(ScriptFilenameEntry, filename));
    }
};

struct ScriptFilenameHasher
{
    typedef const char *Lookup;
    static HashNumber hash(const char *l) { return mozilla::HashString(l); }
    static bool match(const ScriptFilenameEntry *e, const char *l) {
        return strcmp(e->filename, l) == 0;
    }
};

typedef HashSet<ScriptFilenameEntry *,
                ScriptFilenameHasher,
                SystemAllocPolicy> ScriptFilenameTable;

inline void
MarkScriptFilename(JSRuntime *rt, const char *filename);

extern void
SweepScriptFilenames(JSRuntime *rt);

extern void
FreeScriptFilenames(JSRuntime *rt);

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
CloneScript(JSContext *cx, HandleObject enclosingScope, HandleFunction fun, HandleScript script);






template<XDRMode mode>
bool
XDRScript(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript enclosingScript,
          HandleFunction fun, JSScript **scriptp);

} 

#endif 
