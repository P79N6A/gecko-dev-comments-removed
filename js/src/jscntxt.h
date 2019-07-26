







#ifndef jscntxt_h
#define jscntxt_h

#include "mozilla/MemoryReporting.h"

#include "js/Vector.h"
#include "vm/Runtime.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif

struct DtoaState;

extern void
js_ReportOutOfMemory(js::ThreadSafeContext *cx);

extern void
js_ReportAllocationOverflow(js::ThreadSafeContext *cx);

extern void
js_ReportOverRecursed(js::ThreadSafeContext *cx);

namespace js {

namespace jit { class IonContext; }

struct CallsiteCloneKey {
    
    JSFunction *original;

    
    JSScript *script;

    
    uint32_t offset;

    CallsiteCloneKey(JSFunction *f, JSScript *s, uint32_t o) : original(f), script(s), offset(o) {}

    typedef CallsiteCloneKey Lookup;

    static inline uint32_t hash(CallsiteCloneKey key) {
        return uint32_t(size_t(key.script->code + key.offset) ^ size_t(key.original));
    }

    static inline bool match(const CallsiteCloneKey &a, const CallsiteCloneKey &b) {
        return a.script == b.script && a.offset == b.offset && a.original == b.original;
    }
};

typedef HashMap<CallsiteCloneKey,
                ReadBarriered<JSFunction>,
                CallsiteCloneKey,
                SystemAllocPolicy> CallsiteCloneTable;

JSFunction *CloneFunctionAtCallsite(JSContext *cx, HandleFunction fun,
                                    HandleScript script, jsbytecode *pc);

typedef HashSet<JSObject *> ObjectSet;
typedef HashSet<Shape *> ShapeSet;


class AutoCycleDetector
{
    JSContext *cx;
    RootedObject obj;
    bool cyclic;
    uint32_t hashsetGenerationAtInit;
    ObjectSet::AddPtr hashsetAddPointer;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoCycleDetector(JSContext *cx, HandleObject objArg
                      MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), obj(cx, objArg), cyclic(true)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoCycleDetector();

    bool init();

    bool foundCycle() { return cyclic; }
};


extern void
TraceCycleDetectionSet(JSTracer *trc, ObjectSet &set);

struct AutoResolving;
class DtoaCache;
class ForkJoinSlice;
class RegExpCompartment;
class RegExpStatics;
class ForkJoinSlice;

namespace frontend { struct CompileError; }






























struct ThreadSafeContext : ContextFriendFields,
                           public MallocProvider<ThreadSafeContext>
{
  public:
    enum ContextKind {
        Context_JS,
        Context_Exclusive,
        Context_ForkJoin
    };

  private:
    ContextKind contextKind_;

  public:
    PerThreadData *perThreadData;

    ThreadSafeContext(JSRuntime *rt, PerThreadData *pt, ContextKind kind);

    bool isJSContext() const {
        return contextKind_ == Context_JS;
    }

    JSContext *maybeJSContext() const {
        if (isJSContext())
            return (JSContext *) this;
        return nullptr;
    }

    JSContext *asJSContext() const {
        
        
        
        
        JS_ASSERT(isJSContext());
        return maybeJSContext();
    }

    
    
    
    
    
    
    
    bool shouldBeJSContext() const {
        JS_ASSERT(isJSContext());
        return isJSContext();
    }

    bool isExclusiveContext() const {
        return contextKind_ == Context_JS || contextKind_ == Context_Exclusive;
    }

    ExclusiveContext *maybeExclusiveContext() const {
        if (isExclusiveContext())
            return (ExclusiveContext *) this;
        return nullptr;
    }

    ExclusiveContext *asExclusiveContext() const {
        JS_ASSERT(isExclusiveContext());
        return maybeExclusiveContext();
    }

    bool isForkJoinSlice() const;
    ForkJoinSlice *asForkJoinSlice();

    
#ifdef JSGC_GENERATIONAL
    inline bool hasNursery() const {
        return isJSContext();
    }

    inline js::Nursery &nursery() {
        JS_ASSERT(hasNursery());
        return runtime_->gcNursery;
    }
#endif

    










  protected:
    Allocator *allocator_;

  public:
    static size_t offsetOfAllocator() { return offsetof(ThreadSafeContext, allocator_); }

    inline Allocator *const allocator();

    
    inline AllowGC allowGC() const { return isJSContext() ? CanGC : NoGC; }

    template <typename T>
    bool isInsideCurrentZone(T thing) const {
        return thing->zoneFromAnyThread() == zone_;
    }

    template <typename T>
    inline bool isInsideCurrentCompartment(T thing) const {
        return thing->compartment() == compartment_;
    }

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtime_->onOutOfMemory(p, nbytes, maybeJSContext());
    }

    inline void updateMallocCounter(size_t nbytes) {
        
        runtime_->updateMallocCounter(zone_, nbytes);
    }

    void reportAllocationOverflow() {
        js_ReportAllocationOverflow(this);
    }

    
    JSAtomState &names() { return runtime_->atomState; }
    StaticStrings &staticStrings() { return runtime_->staticStrings; }
    PropertyName *emptyString() { return runtime_->emptyString; }
    FreeOp *defaultFreeOp() { return runtime_->defaultFreeOp(); }
    bool useHelperThreads() { return runtime_->useHelperThreads(); }
    size_t helperThreadCount() { return runtime_->helperThreadCount(); }
    void *runtimeAddressForJit() { return runtime_; }
    void *stackLimitAddress(StackKind kind) { return &runtime_->mainThread.nativeStackLimit[kind]; }

    
    uint64_t gcNumber() { return runtime_->gcNumber; }
    size_t gcSystemPageSize() { return runtime_->gcSystemPageSize; }
    bool isHeapBusy() { return runtime_->isHeapBusy(); }
    bool signalHandlersInstalled() const { return runtime_->signalHandlersInstalled(); }
    bool jitSupportsFloatingPoint() const { return runtime_->jitSupportsFloatingPoint; }

    
    DtoaState *dtoaState() {
        return perThreadData->dtoaState;
    }
};

struct WorkerThread;

class ExclusiveContext : public ThreadSafeContext
{
    friend class gc::ArenaLists;
    friend class AutoCompartment;
    friend class AutoLockForExclusiveAccess;
    friend struct StackBaseShape;
    friend void JSScript::initCompartment(ExclusiveContext *cx);
    friend class jit::IonContext;

    
    WorkerThread *workerThread_;

  public:

    ExclusiveContext(JSRuntime *rt, PerThreadData *pt, ContextKind kind)
      : ThreadSafeContext(rt, pt, kind),
        workerThread_(nullptr),
        enterCompartmentDepth_(0)
    {}

    














  protected:
    unsigned            enterCompartmentDepth_;
    inline void setCompartment(JSCompartment *comp);
  public:
    bool hasEnteredCompartment() const {
        return enterCompartmentDepth_ > 0;
    }
#ifdef DEBUG
    unsigned getEnterCompartmentDepth() const {
        return enterCompartmentDepth_;
    }
#endif

    inline void enterCompartment(JSCompartment *c);
    inline void leaveCompartment(JSCompartment *oldCompartment);

    void setWorkerThread(WorkerThread *workerThread);
    WorkerThread *workerThread() const { return workerThread_; }

    
    inline void maybePause() const;

    
    
    JSCompartment *compartment() const {
        JS_ASSERT_IF(runtime_->isAtomsCompartment(compartment_),
                     runtime_->currentThreadHasExclusiveAccess());
        return compartment_;
    }
    JS::Zone *zone() const {
        JS_ASSERT_IF(!compartment(), !zone_);
        JS_ASSERT_IF(compartment(), js::GetCompartmentZone(compartment()) == zone_);
        return zone_;
    }

    
    inline bool typeInferenceEnabled() const;
    types::TypeObject *getNewType(const Class *clasp, TaggedProto proto, JSFunction *fun = nullptr);
    types::TypeObject *getLazyType(const Class *clasp, TaggedProto proto);
    inline js::LifoAlloc &typeLifoAlloc();

    
    
    inline js::Handle<js::GlobalObject*> global() const;

    
    frontend::ParseMapPool &parseMapPool() {
        return runtime_->parseMapPool();
    }
    AtomSet &atoms() {
        return runtime_->atoms();
    }
    JSCompartment *atomsCompartment() {
        return runtime_->atomsCompartment();
    }
    ScriptDataTable &scriptDataTable() {
        return runtime_->scriptDataTable();
    }

#ifdef JS_WORKER_THREADS
    
    
    
    WorkerThreadState *workerThreadState() {
        return runtime_->workerThreadState;
    }
#endif

    
    frontend::CompileError &addPendingCompileError();
};

inline void
MaybeCheckStackRoots(ExclusiveContext *cx)
{
    MaybeCheckStackRoots(cx->maybeJSContext());
}

} 

struct JSContext : public js::ExclusiveContext,
                   public mozilla::LinkedListElement<JSContext>
{
    explicit JSContext(JSRuntime *rt);
    ~JSContext();

    JSRuntime *runtime() const { return runtime_; }
    js::PerThreadData &mainThread() const { return runtime()->mainThread; }

    friend class js::ExclusiveContext;

  private:
    
    bool                throwing;            
    js::Value           exception;           

    
    unsigned            options_;            

  public:
    int32_t             reportGranularity;  

    js::AutoResolving   *resolvingList;

    
    bool                generatingError;

    
  private:
    struct SavedFrameChain {
        SavedFrameChain(JSCompartment *comp, unsigned count)
          : compartment(comp), enterCompartmentCount(count) {}
        JSCompartment *compartment;
        unsigned enterCompartmentCount;
    };
    typedef js::Vector<SavedFrameChain, 1, js::SystemAllocPolicy> SaveStack;
    SaveStack           savedFrameChains_;
  public:
    bool saveFrameChain();
    void restoreFrameChain();

    




  private:
    JSObject *defaultCompartmentObject_;
  public:
    inline void setDefaultCompartmentObject(JSObject *obj);
    inline void setDefaultCompartmentObjectIfUnset(JSObject *obj);
    JSObject *maybeDefaultCompartmentObject() const {
        JS_ASSERT(!hasOption(JSOPTION_NO_DEFAULT_COMPARTMENT_OBJECT));
        return defaultCompartmentObject_;
    }

    
    void wrapPendingException();

    
    js::ObjectSet       cycleDetectorSet;

    
    JSErrorReporter     errorReporter;

    
    void                *data;
    void                *data2;

  public:

    







    JSVersion findVersion() const;

    void setOptions(unsigned opts) {
        JS_ASSERT((opts & JSOPTION_MASK) == opts);
        options_ = opts;
    }

    unsigned options() const { return options_; }

    bool hasOption(unsigned opt) const {
        JS_ASSERT((opt & JSOPTION_MASK) == opt);
        return !!(options_ & opt);
    }

    bool hasExtraWarningsOption() const { return hasOption(JSOPTION_EXTRA_WARNINGS); }
    bool hasWErrorOption() const { return hasOption(JSOPTION_WERROR); }

    js::LifoAlloc &tempLifoAlloc() { return runtime()->tempLifoAlloc; }

#ifdef JS_THREADSAFE
    unsigned            outstandingRequests;


#endif

    
    unsigned               resolveFlags;

    
    js::Value           iterValue;

    bool jitIsBroken;

    void updateJITEnabled();

    
    bool currentlyRunning() const;

    bool currentlyRunningInInterpreter() const {
        return mainThread().activation()->isInterpreter();
    }
    bool currentlyRunningInJit() const {
        return mainThread().activation()->isJit();
    }
    js::StackFrame *interpreterFrame() const {
        return mainThread().activation()->asInterpreter()->current();
    }
    js::FrameRegs &interpreterRegs() const {
        return mainThread().activation()->asInterpreter()->regs();
    }

    





    enum MaybeAllowCrossCompartment {
        DONT_ALLOW_CROSS_COMPARTMENT = false,
        ALLOW_CROSS_COMPARTMENT = true
    };
    inline JSScript *currentScript(jsbytecode **pc = nullptr,
                                   MaybeAllowCrossCompartment = DONT_ALLOW_CROSS_COMPARTMENT) const;

#ifdef MOZ_TRACE_JSCALLS
    
    JSFunctionCallback    functionCallback;

    void doFunctionCallback(const JSFunction *fun,
                            const JSScript *scr,
                            int entering) const
    {
        if (functionCallback)
            functionCallback(fun, scr, this, entering);
    }
#endif

  private:
    
    JSGenerator *innermostGenerator_;
  public:
    JSGenerator *innermostGenerator() const { return innermostGenerator_; }
    void enterGenerator(JSGenerator *gen);
    void leaveGenerator(JSGenerator *gen);

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtime()->onOutOfMemory(p, nbytes, this);
    }
    void updateMallocCounter(size_t nbytes) {
        runtime()->updateMallocCounter(zone(), nbytes);
    }
    void reportAllocationOverflow() {
        js_ReportAllocationOverflow(this);
    }

    bool isExceptionPending() {
        return throwing;
    }

    js::Value getPendingException() {
        JS_ASSERT(throwing);
        return exception;
    }

    void setPendingException(js::Value v);

    void clearPendingException() {
        throwing = false;
        exception.setUndefined();
    }

#ifdef DEBUG
    



    bool stackIterAssertionEnabled;
#endif

    



    bool runningWithTrustedPrincipals() const;

    JS_FRIEND_API(size_t) sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    void mark(JSTracer *trc);

  private:
    





    JS_FRIEND_API(void) checkMallocGCPressure(void *p);
}; 

namespace js {

struct AutoResolving {
  public:
    enum Kind {
        LOOKUP,
        WATCH
    };

    AutoResolving(JSContext *cx, HandleObject obj, HandleId id, Kind kind = LOOKUP
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : context(cx), object(obj), id(id), kind(kind), link(cx->resolvingList)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
        cx->resolvingList = this;
    }

    ~AutoResolving() {
        JS_ASSERT(context->resolvingList == this);
        context->resolvingList = link;
    }

    bool alreadyStarted() const {
        return link && alreadyStartedSlow();
    }

  private:
    bool alreadyStartedSlow() const;

    JSContext           *const context;
    HandleObject        object;
    HandleId            id;
    Kind                const kind;
    AutoResolving       *const link;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, unsigned flags
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx), mSaved(cx->resolveFlags)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    unsigned mSaved;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

namespace js {




class ContextIter {
    JSContext *iter;

public:
    explicit ContextIter(JSRuntime *rt) {
        iter = rt->contextList.getFirst();
    }

    bool done() const {
        return !iter;
    }

    void next() {
        JS_ASSERT(!done());
        iter = iter->getNext();
    }

    JSContext *get() const {
        JS_ASSERT(!done());
        return iter;
    }

    operator JSContext *() const {
        return get();
    }

    JSContext *operator ->() const {
        return get();
    }
};





extern JSContext *
NewContext(JSRuntime *rt, size_t stackChunkSize);

enum DestroyContextMode {
    DCM_NO_GC,
    DCM_FORCE_GC,
    DCM_NEW_FAILED
};

extern void
DestroyContext(JSContext *cx, DestroyContextMode mode);

enum ErrorArgumentsType {
    ArgumentsAreUnicode,
    ArgumentsAreASCII
};

} 

#ifdef va_start
extern bool
js_ReportErrorVA(JSContext *cx, unsigned flags, const char *format, va_list ap);

extern bool
js_ReportErrorNumberVA(JSContext *cx, unsigned flags, JSErrorCallback callback,
                       void *userRef, const unsigned errorNumber,
                       js::ErrorArgumentsType argumentsType, va_list ap);

extern bool
js_ReportErrorNumberUCArray(JSContext *cx, unsigned flags, JSErrorCallback callback,
                            void *userRef, const unsigned errorNumber,
                            const jschar **args);
#endif

extern bool
js_ExpandErrorArguments(js::ExclusiveContext *cx, JSErrorCallback callback,
                        void *userRef, const unsigned errorNumber,
                        char **message, JSErrorReport *reportp,
                        js::ErrorArgumentsType argumentsType, va_list ap);

namespace js {


extern void
ReportUsageError(JSContext *cx, HandleObject callee, const char *msg);







extern bool
PrintError(JSContext *cx, FILE *file, const char *message, JSErrorReport *report,
           bool reportWarnings);
} 





extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);




extern bool
js_ReportIsNullOrUndefined(JSContext *cx, int spindex, js::HandleValue v,
                           js::HandleString fallback);

extern void
js_ReportMissingArg(JSContext *cx, js::HandleValue v, unsigned arg);






extern bool
js_ReportValueErrorFlags(JSContext *cx, unsigned flags, const unsigned errorNumber,
                         int spindex, js::HandleValue v, js::HandleString fallback,
                         const char *arg1, const char *arg2);

#define js_ReportValueError(cx,errorNumber,spindex,v,fallback)                \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, nullptr, nullptr))

#define js_ReportValueError2(cx,errorNumber,spindex,v,fallback,arg1)          \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, nullptr))

#define js_ReportValueError3(cx,errorNumber,spindex,v,fallback,arg1,arg2)     \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, arg2))

extern const JSErrorFormatString js_ErrorFormatString[JSErr_Limit];

char *
js_strdup(js::ExclusiveContext *cx, const char *s);

#ifdef JS_THREADSAFE
# define JS_ASSERT_REQUEST_DEPTH(cx)  JS_ASSERT((cx)->runtime()->requestDepth >= 1)
#else
# define JS_ASSERT_REQUEST_DEPTH(cx)  ((void) 0)
#endif





extern bool
js_InvokeOperationCallback(JSContext *cx);

extern bool
js_HandleExecutionInterrupt(JSContext *cx);






static MOZ_ALWAYS_INLINE bool
JS_CHECK_OPERATION_LIMIT(JSContext *cx)
{
    JS_ASSERT_REQUEST_DEPTH(cx);
    return !cx->runtime()->interrupt || js_InvokeOperationCallback(cx);
}

namespace js {



class AutoStringVector : public AutoVectorRooter<JSString *>
{
  public:
    explicit AutoStringVector(JSContext *cx
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSString *>(cx, STRINGVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoShapeVector : public AutoVectorRooter<Shape *>
{
  public:
    explicit AutoShapeVector(JSContext *cx
                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Shape *>(cx, SHAPEVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueArray : public AutoGCRooter
{
    Value *start_;
    unsigned length_;
    SkipRoot skip;

  public:
    AutoValueArray(JSContext *cx, Value *start, unsigned length
                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, VALARRAY), start_(start), length_(length), skip(cx, start, length)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    Value *start() { return start_; }
    unsigned length() const { return length_; }

    MutableHandleValue handleAt(unsigned i)
    {
        JS_ASSERT(i < length_);
        return MutableHandleValue::fromMarkedLocation(&start_[i]);
    }
    HandleValue handleAt(unsigned i) const
    {
        JS_ASSERT(i < length_);
        return HandleValue::fromMarkedLocation(&start_[i]);
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectObjectHashMap : public AutoHashMapRooter<JSObject *, JSObject *>
{
  public:
    explicit AutoObjectObjectHashMap(JSContext *cx
                                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashMapRooter<JSObject *, JSObject *>(cx, OBJOBJHASHMAP)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectUnsigned32HashMap : public AutoHashMapRooter<JSObject *, uint32_t>
{
  public:
    explicit AutoObjectUnsigned32HashMap(JSContext *cx
                                         MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashMapRooter<JSObject *, uint32_t>(cx, OBJU32HASHMAP)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectHashSet : public AutoHashSetRooter<JSObject *>
{
  public:
    explicit AutoObjectHashSet(JSContext *cx
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashSetRooter<JSObject *>(cx, OBJHASHSET)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoAssertNoException
{
#ifdef DEBUG
    JSContext *cx;
    bool hadException;
#endif

  public:
    AutoAssertNoException(JSContext *cx)
#ifdef DEBUG
      : cx(cx),
        hadException(cx->isExceptionPending())
#endif
    {
    }

    ~AutoAssertNoException()
    {
        JS_ASSERT_IF(!hadException, !cx->isExceptionPending());
    }
};




class ContextAllocPolicy
{
    ThreadSafeContext *const cx_;

  public:
    ContextAllocPolicy(ThreadSafeContext *cx) : cx_(cx) {}
    ThreadSafeContext *context() const { return cx_; }
    void *malloc_(size_t bytes) { return cx_->malloc_(bytes); }
    void *calloc_(size_t bytes) { return cx_->calloc_(bytes); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) { return cx_->realloc_(p, oldBytes, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const { js_ReportAllocationOverflow(cx_); }
};


bool intrinsic_ToObject(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_IsCallable(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ThrowError(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_NewDenseArray(JSContext *cx, unsigned argc, Value *vp);

bool intrinsic_UnsafePutElements(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_UnsafeSetReservedSlot(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_UnsafeGetReservedSlot(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_HaveSameClass(JSContext *cx, unsigned argc, Value *vp);

bool intrinsic_ShouldForceSequential(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_NewParallelArray(JSContext *cx, unsigned argc, Value *vp);

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif
