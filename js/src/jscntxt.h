







#ifndef jscntxt_h
#define jscntxt_h

#include "mozilla/MemoryReporting.h"

#include "js/Vector.h"
#include "vm/Runtime.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif

struct DtoaState;

extern void
js_ReportOutOfMemory(js::ThreadSafeContext *cx);

extern void
js_ReportAllocationOverflow(js::ThreadSafeContext *cx);

extern void
js_ReportOverRecursed(js::ThreadSafeContext *cx);

namespace js {

namespace jit {
class IonContext;
class CompileCompartment;
}

struct CallsiteCloneKey {
    
    JSFunction *original;

    
    JSScript *script;

    
    uint32_t offset;

    CallsiteCloneKey(JSFunction *f, JSScript *s, uint32_t o) : original(f), script(s), offset(o) {}

    bool operator==(const CallsiteCloneKey& other) {
        return original == other.original && script == other.script && offset == other.offset;
    }

    bool operator!=(const CallsiteCloneKey& other) {
        return !(*this == other);
    }

    typedef CallsiteCloneKey Lookup;

    static inline uint32_t hash(CallsiteCloneKey key) {
        return uint32_t(size_t(key.script->offsetToPC(key.offset)) ^ size_t(key.original));
    }

    static inline bool match(const CallsiteCloneKey &a, const CallsiteCloneKey &b) {
        return a.script == b.script && a.offset == b.offset && a.original == b.original;
    }

    static void rekey(CallsiteCloneKey &k, const CallsiteCloneKey &newKey) {
        k.original = newKey.original;
        k.script = newKey.script;
        k.offset = newKey.offset;
    }
};

typedef HashMap<CallsiteCloneKey,
                ReadBarrieredFunction,
                CallsiteCloneKey,
                SystemAllocPolicy> CallsiteCloneTable;

JSFunction *
ExistingCloneFunctionAtCallsite(const CallsiteCloneTable &table, JSFunction *fun,
                                JSScript *script, jsbytecode *pc);

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
class ForkJoinContext;
class RegExpStatics;

namespace frontend { struct CompileError; }






























struct ThreadSafeContext : ContextFriendFields,
                           public MallocProvider<ThreadSafeContext>
{
    friend struct StackBaseShape;
    friend class Activation;
    friend UnownedBaseShape *BaseShape::lookupUnowned(ThreadSafeContext *cx,
                                                      const StackBaseShape &base);
    friend Shape *JSObject::lookupChildProperty(ThreadSafeContext *cx,
                                                JS::HandleObject obj, js::HandleShape parent,
                                                js::StackShape &child);

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

    bool isForkJoinContext() const;
    ForkJoinContext *asForkJoinContext();

    










  protected:
    Allocator *allocator_;

  public:
    static size_t offsetOfAllocator() { return offsetof(ThreadSafeContext, allocator_); }

    inline Allocator *allocator() const;

    
    inline AllowGC allowGC() const { return isJSContext() ? CanGC : NoGC; }

    template <typename T>
    bool isInsideCurrentZone(T thing) const {
        return thing->zoneFromAnyThread() == zone_;
    }

    template <typename T>
    inline bool isInsideCurrentCompartment(T thing) const {
        return thing->compartment() == compartment_;
    }

    template <typename T>
    inline bool isThreadLocal(T thing) const;

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtime_->onOutOfMemory(p, nbytes, maybeJSContext());
    }

    
    void recoverFromOutOfMemory();

    inline void updateMallocCounter(size_t nbytes) {
        
        runtime_->updateMallocCounter(zone_, nbytes);
    }

    void reportAllocationOverflow() {
        js_ReportAllocationOverflow(this);
    }

    
    JSAtomState &names() { return *runtime_->commonNames; }
    StaticStrings &staticStrings() { return *runtime_->staticStrings; }
    AtomSet &permanentAtoms() { return *runtime_->permanentAtoms; }
    const JS::AsmJSCacheOps &asmJSCacheOps() { return runtime_->asmJSCacheOps; }
    PropertyName *emptyString() { return runtime_->emptyString; }
    FreeOp *defaultFreeOp() { return runtime_->defaultFreeOp(); }
    void *runtimeAddressForJit() { return runtime_; }
    void *runtimeAddressOfInterrupt() { return &runtime_->interrupt; }
    void *stackLimitAddress(StackKind kind) { return &runtime_->mainThread.nativeStackLimit[kind]; }
    void *stackLimitAddressForJitCode(StackKind kind);
    size_t gcSystemPageSize() { return gc::SystemPageSize(); }
    bool signalHandlersInstalled() const { return runtime_->signalHandlersInstalled(); }
    bool canUseSignalHandlers() const { return runtime_->canUseSignalHandlers(); }
    bool jitSupportsFloatingPoint() const { return runtime_->jitSupportsFloatingPoint; }
    bool jitSupportsSimd() const { return runtime_->jitSupportsSimd; }

    
    DtoaState *dtoaState() {
        return perThreadData->dtoaState;
    }
};

struct HelperThread;

class ExclusiveContext : public ThreadSafeContext
{
    friend class gc::ArenaLists;
    friend class AutoCompartment;
    friend class AutoLockForExclusiveAccess;
    friend struct StackBaseShape;
    friend void JSScript::initCompartment(ExclusiveContext *cx);
    friend class jit::IonContext;

    
    HelperThread *helperThread_;

  public:

    ExclusiveContext(JSRuntime *rt, PerThreadData *pt, ContextKind kind)
      : ThreadSafeContext(rt, pt, kind),
        helperThread_(nullptr),
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
    inline void enterNullCompartment();
    inline void leaveCompartment(JSCompartment *oldCompartment);

    void setHelperThread(HelperThread *helperThread);
    HelperThread *helperThread() const { return helperThread_; }

    
    
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

    
    types::TypeObject *getNewType(const Class *clasp, TaggedProto proto, JSFunction *fun = nullptr);
    types::TypeObject *getSingletonType(const Class *clasp, TaggedProto proto);
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
    SymbolRegistry &symbolRegistry() {
        return runtime_->symbolRegistry();
    }
    ScriptDataTable &scriptDataTable() {
        return runtime_->scriptDataTable();
    }

    
    frontend::CompileError &addPendingCompileError();
    void addPendingOverRecursed();
};

} 

struct JSContext : public js::ExclusiveContext,
                   public mozilla::LinkedListElement<JSContext>
{
    explicit JSContext(JSRuntime *rt);
    ~JSContext();

    JSRuntime *runtime() const { return runtime_; }
    js::PerThreadData &mainThread() const { return runtime()->mainThread; }

    static size_t offsetOfRuntime() {
        return offsetof(JSContext, runtime_);
    }

    friend class js::ExclusiveContext;
    friend class JS::AutoSaveExceptionState;

  private:
    
    bool                throwing;            
    js::Value           unwrappedException_; 

    
    JS::ContextOptions  options_;

    
    
    bool                propagatingForcedReturn_;

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

  public:
    
    js::ObjectSet       cycleDetectorSet;

    
    void                *data;
    void                *data2;

  public:

    







    JSVersion findVersion() const;

    const JS::ContextOptions &options() const {
        return options_;
    }

    JS::ContextOptions &options() {
        return options_;
    }

    js::LifoAlloc &tempLifoAlloc() { return runtime()->tempLifoAlloc; }

    unsigned            outstandingRequests;



    
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
    js::InterpreterFrame *interpreterFrame() const {
        return mainThread().activation()->asInterpreter()->current();
    }
    js::InterpreterRegs &interpreterRegs() const {
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

    
#ifdef JSGC_GENERATIONAL
    inline js::Nursery &nursery() {
        return runtime_->gc.nursery;
    }
#endif

    void minorGC(JS::gcreason::Reason reason) {
        runtime_->gc.minorGC(this, reason);
    }

    void gcIfNeeded() {
        runtime_->gc.gcIfNeeded(this);
    }

  private:
    
    JSGenerator *innermostGenerator_;
  public:
    JSGenerator *innermostGenerator() const { return innermostGenerator_; }
    void enterGenerator(JSGenerator *gen);
    void leaveGenerator(JSGenerator *gen);

    bool isExceptionPending() {
        return throwing;
    }

    MOZ_WARN_UNUSED_RESULT
    bool getPendingException(JS::MutableHandleValue rval);

    bool isThrowingOutOfMemory();

    void setPendingException(js::Value v);

    void clearPendingException() {
        throwing = false;
        unwrappedException_.setUndefined();
    }

    bool isPropagatingForcedReturn() const { return propagatingForcedReturn_; }
    void setPropagatingForcedReturn() { propagatingForcedReturn_ = true; }
    void clearPropagatingForcedReturn() { propagatingForcedReturn_ = false; }

    



    inline bool runningWithTrustedPrincipals() const;

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








JSFunction *
SelfHostedFunction(JSContext *cx, HandlePropertyName propName);

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
                            const char16_t **args);
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




void
CallErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);

} 

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

namespace js {





bool
InvokeInterruptCallback(JSContext *cx);

bool
HandleExecutionInterrupt(JSContext *cx);













MOZ_ALWAYS_INLINE bool
CheckForInterrupt(JSContext *cx)
{
    MOZ_ASSERT(cx->runtime()->requestDepth >= 1);
    return !cx->runtime()->interrupt || InvokeInterruptCallback(cx);
}



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

class AutoPropertyNameVector : public AutoVectorRooter<PropertyName *>
{
  public:
    explicit AutoPropertyNameVector(JSContext *cx
                                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<PropertyName *>(cx, STRINGVECTOR)
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


class AutoArrayRooter : private JS::AutoGCRooter
{
  public:
    AutoArrayRooter(JSContext *cx, size_t len, Value *vec
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : JS::AutoGCRooter(cx, len), array(vec)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag_ >= 0);
    }

    void changeLength(size_t newLength) {
        tag_ = ptrdiff_t(newLength);
        JS_ASSERT(tag_ >= 0);
    }

    void changeArray(Value *newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    Value *start() {
        return array;
    }

    size_t length() {
        JS_ASSERT(tag_ >= 0);
        return size_t(tag_);
    }

    MutableHandleValue handleAt(size_t i) {
        JS_ASSERT(i < size_t(tag_));
        return MutableHandleValue::fromMarkedLocation(&array[i]);
    }
    HandleValue handleAt(size_t i) const {
        JS_ASSERT(i < size_t(tag_));
        return HandleValue::fromMarkedLocation(&array[i]);
    }
    MutableHandleValue operator[](size_t i) {
        JS_ASSERT(i < size_t(tag_));
        return MutableHandleValue::fromMarkedLocation(&array[i]);
    }
    HandleValue operator[](size_t i) const {
        JS_ASSERT(i < size_t(tag_));
        return HandleValue::fromMarkedLocation(&array[i]);
    }

    friend void JS::AutoGCRooter::trace(JSTracer *trc);

  private:
    Value *array;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoAssertNoException
{
#ifdef DEBUG
    JSContext *cx;
    bool hadException;
#endif

  public:
    explicit AutoAssertNoException(JSContext *cx)
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


bool intrinsic_ToObject(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_IsObject(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ToInteger(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ToString(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_IsCallable(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ThrowError(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_NewDenseArray(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_IsConstructing(JSContext *cx, unsigned argc, Value *vp);

bool intrinsic_UnsafePutElements(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_DefineDataProperty(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_UnsafeSetReservedSlot(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_UnsafeGetReservedSlot(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_HaveSameClass(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_IsPackedArray(JSContext *cx, unsigned argc, Value *vp);

bool intrinsic_ShouldForceSequential(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_NewParallelArray(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ForkJoinGetSlice(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_InParallelSection(JSContext *cx, unsigned argc, Value *vp);

bool intrinsic_ObjectIsTypedObject(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ObjectIsTransparentTypedObject(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ObjectIsOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_ObjectIsTypeDescr(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_TypeDescrIsSimpleType(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_TypeDescrIsArrayType(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_TypeDescrIsUnsizedArrayType(JSContext *cx, unsigned argc, Value *vp);
bool intrinsic_TypeDescrIsSizedArrayType(JSContext *cx, unsigned argc, Value *vp);

class AutoLockForExclusiveAccess
{
    JSRuntime *runtime;

    void init(JSRuntime *rt) {
        runtime = rt;
        if (runtime->numExclusiveThreads) {
            runtime->assertCanLock(ExclusiveAccessLock);
            PR_Lock(runtime->exclusiveAccessLock);
#ifdef DEBUG
            runtime->exclusiveAccessOwner = PR_GetCurrentThread();
#endif
        } else {
            JS_ASSERT(!runtime->mainThreadHasExclusiveAccess);
            runtime->mainThreadHasExclusiveAccess = true;
        }
    }

  public:
    explicit AutoLockForExclusiveAccess(ExclusiveContext *cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx->runtime_);
    }
    explicit AutoLockForExclusiveAccess(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }
    ~AutoLockForExclusiveAccess() {
        if (runtime->numExclusiveThreads) {
            JS_ASSERT(runtime->exclusiveAccessOwner == PR_GetCurrentThread());
            runtime->exclusiveAccessOwner = nullptr;
            PR_Unlock(runtime->exclusiveAccessLock);
        } else {
            JS_ASSERT(runtime->mainThreadHasExclusiveAccess);
            runtime->mainThreadHasExclusiveAccess = false;
        }
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
