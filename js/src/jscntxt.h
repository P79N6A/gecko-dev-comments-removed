







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

namespace js {

namespace jit {
class JitContext;
class DebugModeOSRVolatileJitFrameIterator;
}

typedef HashSet<JSObject*> ObjectSet;
typedef HashSet<Shape*> ShapeSet;


class AutoCycleDetector
{
    JSContext* cx;
    RootedObject obj;
    bool cyclic;
    uint32_t hashsetGenerationAtInit;
    ObjectSet::AddPtr hashsetAddPointer;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoCycleDetector(JSContext* cx, HandleObject objArg
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
TraceCycleDetectionSet(JSTracer* trc, ObjectSet& set);

struct AutoResolving;

namespace frontend { struct CompileError; }





















struct HelperThread;

class ExclusiveContext : public ContextFriendFields,
                         public MallocProvider<ExclusiveContext>
{
    friend class gc::ArenaLists;
    friend class AutoCompartment;
    friend class AutoLockForExclusiveAccess;
    friend struct StackBaseShape;
    friend void JSScript::initCompartment(ExclusiveContext* cx);
    friend class jit::JitContext;
    friend class Activation;

    
    HelperThread* helperThread_;

  public:
    enum ContextKind {
        Context_JS,
        Context_Exclusive
    };

  private:
    ContextKind contextKind_;

  public:
    PerThreadData* perThreadData;

    ExclusiveContext(JSRuntime* rt, PerThreadData* pt, ContextKind kind);

    bool isJSContext() const {
        return contextKind_ == Context_JS;
    }

    JSContext* maybeJSContext() const {
        if (isJSContext())
            return (JSContext*) this;
        return nullptr;
    }

    JSContext* asJSContext() const {
        
        
        
        
        MOZ_ASSERT(isJSContext());
        return maybeJSContext();
    }

    
    
    
    
    
    
    
    bool shouldBeJSContext() const {
        MOZ_ASSERT(isJSContext());
        return isJSContext();
    }

  protected:
    js::gc::ArenaLists* arenas_;

  public:
    inline js::gc::ArenaLists* arenas() const { return arenas_; }

    template <typename T>
    bool isInsideCurrentZone(T thing) const {
        return thing->zoneFromAnyThread() == zone_;
    }

    template <typename T>
    inline bool isInsideCurrentCompartment(T thing) const {
        return thing->compartment() == compartment_;
    }

    void* onOutOfMemory(js::AllocFunction allocFunc, size_t nbytes, void* reallocPtr = nullptr) {
        return runtime_->onOutOfMemory(allocFunc, nbytes, reallocPtr, maybeJSContext());
    }

    
    void recoverFromOutOfMemory();

    inline void updateMallocCounter(size_t nbytes) {
        
        runtime_->updateMallocCounter(zone_, nbytes);
    }

    void reportAllocationOverflow() {
        js::ReportAllocationOverflow(this);
    }

    
    JSAtomState& names() { return *runtime_->commonNames; }
    StaticStrings& staticStrings() { return *runtime_->staticStrings; }
    bool isPermanentAtomsInitialized() { return !!runtime_->permanentAtoms; }
    FrozenAtomSet& permanentAtoms() { return *runtime_->permanentAtoms; }
    WellKnownSymbols& wellKnownSymbols() { return *runtime_->wellKnownSymbols; }
    const JS::AsmJSCacheOps& asmJSCacheOps() { return runtime_->asmJSCacheOps; }
    PropertyName* emptyString() { return runtime_->emptyString; }
    FreeOp* defaultFreeOp() { return runtime_->defaultFreeOp(); }
    void* runtimeAddressForJit() { return runtime_; }
    void* runtimeAddressOfInterruptUint32() { return runtime_->addressOfInterruptUint32(); }
    void* stackLimitAddress(StackKind kind) { return &runtime_->mainThread.nativeStackLimit[kind]; }
    void* stackLimitAddressForJitCode(StackKind kind);
    uintptr_t stackLimit(StackKind kind) { return runtime_->mainThread.nativeStackLimit[kind]; }
    size_t gcSystemPageSize() { return gc::SystemPageSize(); }
    bool canUseSignalHandlers() const { return runtime_->canUseSignalHandlers(); }
    bool jitSupportsFloatingPoint() const { return runtime_->jitSupportsFloatingPoint; }
    bool jitSupportsSimd() const { return runtime_->jitSupportsSimd; }

    
    DtoaState* dtoaState() {
        return perThreadData->dtoaState;
    }

    














  protected:
    unsigned            enterCompartmentDepth_;
    inline void setCompartment(JSCompartment* comp);
  public:
    bool hasEnteredCompartment() const {
        return enterCompartmentDepth_ > 0;
    }
#ifdef DEBUG
    unsigned getEnterCompartmentDepth() const {
        return enterCompartmentDepth_;
    }
#endif

    inline void enterCompartment(JSCompartment* c);
    inline void enterNullCompartment();
    inline void leaveCompartment(JSCompartment* oldCompartment);

    void setHelperThread(HelperThread* helperThread);
    HelperThread* helperThread() const { return helperThread_; }

    
    
    JSCompartment* compartment() const {
        MOZ_ASSERT_IF(runtime_->isAtomsCompartment(compartment_),
                      runtime_->currentThreadHasExclusiveAccess());
        return compartment_;
    }
    JS::Zone* zone() const {
        MOZ_ASSERT_IF(!compartment(), !zone_);
        MOZ_ASSERT_IF(compartment(), js::GetCompartmentZone(compartment()) == zone_);
        return zone_;
    }

    
    inline js::LifoAlloc& typeLifoAlloc();

    
    
    inline js::Handle<js::GlobalObject*> global() const;

    
    frontend::ParseMapPool& parseMapPool() {
        return runtime_->parseMapPool();
    }
    AtomSet& atoms() {
        return runtime_->atoms();
    }
    JSCompartment* atomsCompartment() {
        return runtime_->atomsCompartment();
    }
    SymbolRegistry& symbolRegistry() {
        return runtime_->symbolRegistry();
    }
    ScriptDataTable& scriptDataTable() {
        return runtime_->scriptDataTable();
    }

    
    frontend::CompileError& addPendingCompileError();
    void addPendingOverRecursed();
};

} 

struct JSContext : public js::ExclusiveContext,
                   public mozilla::LinkedListElement<JSContext>
{
    explicit JSContext(JSRuntime* rt);
    ~JSContext();

    JSRuntime* runtime() const { return runtime_; }
    js::PerThreadData& mainThread() const { return runtime()->mainThread; }

    static size_t offsetOfRuntime() {
        return offsetof(JSContext, runtime_);
    }

    friend class js::ExclusiveContext;
    friend class JS::AutoSaveExceptionState;
    friend class js::jit::DebugModeOSRVolatileJitFrameIterator;
    friend void js::ReportOverRecursed(JSContext*);

  private:
    
    bool                throwing;            
    js::Value           unwrappedException_; 

    
    JS::ContextOptions  options_;

    
    
    bool                overRecursed_;

    
    
    bool                propagatingForcedReturn_;

    
    
    js::jit::DebugModeOSRVolatileJitFrameIterator* liveVolatileJitFrameIterators_;

  public:
    int32_t             reportGranularity;  

    js::AutoResolving*  resolvingList;

    
    bool                generatingError;

    
  private:
    struct SavedFrameChain {
        SavedFrameChain(JSCompartment* comp, unsigned count)
          : compartment(comp), enterCompartmentCount(count) {}
        JSCompartment* compartment;
        unsigned enterCompartmentCount;
    };
    typedef js::Vector<SavedFrameChain, 1, js::SystemAllocPolicy> SaveStack;
    SaveStack           savedFrameChains_;
  public:
    bool saveFrameChain();
    void restoreFrameChain();

  public:
    
    js::ObjectSet       cycleDetectorSet;

    
    void*               data;
    void*               data2;

  public:

    







    JSVersion findVersion() const;

    const JS::ContextOptions& options() const {
        return options_;
    }

    JS::ContextOptions& options() {
        return options_;
    }

    js::LifoAlloc& tempLifoAlloc() { return runtime()->tempLifoAlloc; }

    unsigned            outstandingRequests;



    bool jitIsBroken;

    void updateJITEnabled();

    
    bool currentlyRunning() const;

    bool currentlyRunningInInterpreter() const {
        return runtime_->activation()->isInterpreter();
    }
    bool currentlyRunningInJit() const {
        return runtime_->activation()->isJit();
    }
    js::InterpreterFrame* interpreterFrame() const {
        return runtime_->activation()->asInterpreter()->current();
    }
    js::InterpreterRegs& interpreterRegs() const {
        return runtime_->activation()->asInterpreter()->regs();
    }

    





    enum MaybeAllowCrossCompartment {
        DONT_ALLOW_CROSS_COMPARTMENT = false,
        ALLOW_CROSS_COMPARTMENT = true
    };
    inline JSScript* currentScript(jsbytecode** pc = nullptr,
                                   MaybeAllowCrossCompartment = DONT_ALLOW_CROSS_COMPARTMENT) const;

    
    inline js::Nursery& nursery() {
        return runtime_->gc.nursery;
    }

    void minorGC(JS::gcreason::Reason reason) {
        runtime_->gc.minorGC(this, reason);
    }

  public:
    bool isExceptionPending() {
        return throwing;
    }

    MOZ_WARN_UNUSED_RESULT
    bool getPendingException(JS::MutableHandleValue rval);

    bool isThrowingOutOfMemory();
    bool isClosingGenerator();

    void setPendingException(js::Value v);

    void clearPendingException() {
        throwing = false;
        overRecursed_ = false;
        unwrappedException_.setUndefined();
    }

    bool isThrowingOverRecursed() const { return throwing && overRecursed_; }
    bool isPropagatingForcedReturn() const { return propagatingForcedReturn_; }
    void setPropagatingForcedReturn() { propagatingForcedReturn_ = true; }
    void clearPropagatingForcedReturn() { propagatingForcedReturn_ = false; }

    



    inline bool runningWithTrustedPrincipals() const;

    JS_FRIEND_API(size_t) sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    void mark(JSTracer* trc);

  private:
    





    JS_FRIEND_API(void) checkMallocGCPressure(void* p);
}; 

namespace js {

struct AutoResolving {
  public:
    enum Kind {
        LOOKUP,
        WATCH
    };

    AutoResolving(JSContext* cx, HandleObject obj, HandleId id, Kind kind = LOOKUP
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : context(cx), object(obj), id(id), kind(kind), link(cx->resolvingList)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        MOZ_ASSERT(obj);
        cx->resolvingList = this;
    }

    ~AutoResolving() {
        MOZ_ASSERT(context->resolvingList == this);
        context->resolvingList = link;
    }

    bool alreadyStarted() const {
        return link && alreadyStartedSlow();
    }

  private:
    bool alreadyStartedSlow() const;

    JSContext*          const context;
    HandleObject        object;
    HandleId            id;
    Kind                const kind;
    AutoResolving*      const link;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};




class ContextIter {
    JSContext* iter;

public:
    explicit ContextIter(JSRuntime* rt) {
        iter = rt->contextList.getFirst();
    }

    bool done() const {
        return !iter;
    }

    void next() {
        MOZ_ASSERT(!done());
        iter = iter->getNext();
    }

    JSContext* get() const {
        MOZ_ASSERT(!done());
        return iter;
    }

    operator JSContext*() const {
        return get();
    }

    JSContext* operator ->() const {
        return get();
    }
};





extern JSContext*
NewContext(JSRuntime* rt, size_t stackChunkSize);

enum DestroyContextMode {
    DCM_NO_GC,
    DCM_FORCE_GC,
    DCM_NEW_FAILED
};

extern void
DestroyContext(JSContext* cx, DestroyContextMode mode);

enum ErrorArgumentsType {
    ArgumentsAreUnicode,
    ArgumentsAreASCII
};







JSFunction*
SelfHostedFunction(JSContext* cx, HandlePropertyName propName);

#ifdef va_start
extern bool
ReportErrorVA(JSContext* cx, unsigned flags, const char* format, va_list ap);

extern bool
ReportErrorNumberVA(JSContext* cx, unsigned flags, JSErrorCallback callback,
                    void* userRef, const unsigned errorNumber,
                    ErrorArgumentsType argumentsType, va_list ap);

extern bool
ReportErrorNumberUCArray(JSContext* cx, unsigned flags, JSErrorCallback callback,
                         void* userRef, const unsigned errorNumber,
                         const char16_t** args);
#endif

extern bool
ExpandErrorArgumentsVA(ExclusiveContext* cx, JSErrorCallback callback,
                       void* userRef, const unsigned errorNumber,
                       char** message, JSErrorReport* reportp,
                       ErrorArgumentsType argumentsType, va_list ap);


extern void
ReportUsageError(JSContext* cx, HandleObject callee, const char* msg);







extern bool
PrintError(JSContext* cx, FILE* file, const char* message, JSErrorReport* report,
           bool reportWarnings);




void
CallErrorReporter(JSContext* cx, const char* message, JSErrorReport* report);

extern bool
ReportIsNotDefined(JSContext* cx, HandlePropertyName name);

extern bool
ReportIsNotDefined(JSContext* cx, HandleId id);




extern bool
ReportIsNullOrUndefined(JSContext* cx, int spindex, HandleValue v, HandleString fallback);

extern void
ReportMissingArg(JSContext* cx, js::HandleValue v, unsigned arg);






extern bool
ReportValueErrorFlags(JSContext* cx, unsigned flags, const unsigned errorNumber,
                      int spindex, HandleValue v, HandleString fallback,
                      const char* arg1, const char* arg2);

#define ReportValueError(cx,errorNumber,spindex,v,fallback)                   \
    ((void)ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,             \
                                    spindex, v, fallback, nullptr, nullptr))

#define ReportValueError2(cx,errorNumber,spindex,v,fallback,arg1)             \
    ((void)ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,             \
                                    spindex, v, fallback, arg1, nullptr))

#define ReportValueError3(cx,errorNumber,spindex,v,fallback,arg1,arg2)        \
    ((void)ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,             \
                                    spindex, v, fallback, arg1, arg2))

} 

extern const JSErrorFormatString js_ErrorFormatString[JSErr_Limit];

namespace js {

MOZ_ALWAYS_INLINE bool
CheckForInterrupt(JSContext* cx)
{
    
    
    JSRuntime* rt = cx->runtime();
    if (MOZ_UNLIKELY(rt->hasPendingInterrupt()))
        return rt->handleInterrupt(cx);
    return true;
}



typedef JS::AutoVectorRooter<JSString*> AutoStringVector;
typedef JS::AutoVectorRooter<PropertyName*> AutoPropertyNameVector;
typedef JS::AutoVectorRooter<Shape*> AutoShapeVector;

class AutoObjectObjectHashMap : public AutoHashMapRooter<JSObject*, JSObject*>
{
  public:
    explicit AutoObjectObjectHashMap(JSContext* cx
                                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashMapRooter<JSObject*, JSObject*>(cx, OBJOBJHASHMAP)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectUnsigned32HashMap : public AutoHashMapRooter<JSObject*, uint32_t>
{
  public:
    explicit AutoObjectUnsigned32HashMap(JSContext* cx
                                         MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashMapRooter<JSObject*, uint32_t>(cx, OBJU32HASHMAP)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectHashSet : public AutoHashSetRooter<JSObject*>
{
  public:
    explicit AutoObjectHashSet(JSContext* cx
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoHashSetRooter<JSObject*>(cx, OBJHASHSET)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


class AutoArrayRooter : private JS::AutoGCRooter
{
  public:
    AutoArrayRooter(JSContext* cx, size_t len, Value* vec
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : JS::AutoGCRooter(cx, len), array(vec)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        MOZ_ASSERT(tag_ >= 0);
    }

    void changeLength(size_t newLength) {
        tag_ = ptrdiff_t(newLength);
        MOZ_ASSERT(tag_ >= 0);
    }

    void changeArray(Value* newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    Value* start() {
        return array;
    }

    size_t length() {
        MOZ_ASSERT(tag_ >= 0);
        return size_t(tag_);
    }

    MutableHandleValue handleAt(size_t i) {
        MOZ_ASSERT(i < size_t(tag_));
        return MutableHandleValue::fromMarkedLocation(&array[i]);
    }
    HandleValue handleAt(size_t i) const {
        MOZ_ASSERT(i < size_t(tag_));
        return HandleValue::fromMarkedLocation(&array[i]);
    }
    MutableHandleValue operator[](size_t i) {
        MOZ_ASSERT(i < size_t(tag_));
        return MutableHandleValue::fromMarkedLocation(&array[i]);
    }
    HandleValue operator[](size_t i) const {
        MOZ_ASSERT(i < size_t(tag_));
        return HandleValue::fromMarkedLocation(&array[i]);
    }

    friend void JS::AutoGCRooter::trace(JSTracer* trc);

  private:
    Value* array;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoAssertNoException
{
#ifdef DEBUG
    JSContext* cx;
    bool hadException;
#endif

  public:
    explicit AutoAssertNoException(JSContext* cx)
#ifdef DEBUG
      : cx(cx),
        hadException(cx->isExceptionPending())
#endif
    {
    }

    ~AutoAssertNoException()
    {
        MOZ_ASSERT_IF(!hadException, !cx->isExceptionPending());
    }
};


bool intrinsic_ToObject(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsObject(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_ToInteger(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_ToString(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsCallable(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_ThrowRangeError(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_ThrowTypeError(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_NewDenseArray(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsConstructing(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_SubstringKernel(JSContext* cx, unsigned argc, Value* vp);

bool intrinsic_UnsafePutElements(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_DefineDataProperty(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_UnsafeSetReservedSlot(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_UnsafeGetReservedSlot(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_UnsafeGetObjectFromReservedSlot(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_UnsafeGetInt32FromReservedSlot(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_UnsafeGetStringFromReservedSlot(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_UnsafeGetBooleanFromReservedSlot(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsPackedArray(JSContext* cx, unsigned argc, Value* vp);

bool intrinsic_IsSuspendedStarGenerator(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsArrayIterator(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsStringIterator(JSContext* cx, unsigned argc, Value* vp);

bool intrinsic_IsArrayBuffer(JSContext* cx, unsigned argc, Value* vp);

bool intrinsic_IsTypedArray(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_IsPossiblyWrappedTypedArray(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_TypedArrayBuffer(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_TypedArrayByteOffset(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_TypedArrayElementShift(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_TypedArrayLength(JSContext* cx, unsigned argc, Value* vp);

bool intrinsic_MoveTypedArrayElements(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_SetFromTypedArrayApproach(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_SetDisjointTypedElements(JSContext* cx, unsigned argc, Value* vp);
bool intrinsic_SetOverlappingTypedElements(JSContext* cx, unsigned argc, Value* vp);

class AutoLockForExclusiveAccess
{
    JSRuntime* runtime;

    void init(JSRuntime* rt) {
        runtime = rt;
        if (runtime->numExclusiveThreads) {
            runtime->assertCanLock(ExclusiveAccessLock);
            PR_Lock(runtime->exclusiveAccessLock);
#ifdef DEBUG
            runtime->exclusiveAccessOwner = PR_GetCurrentThread();
#endif
        } else {
            MOZ_ASSERT(!runtime->mainThreadHasExclusiveAccess);
            runtime->mainThreadHasExclusiveAccess = true;
        }
    }

  public:
    explicit AutoLockForExclusiveAccess(ExclusiveContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx->runtime_);
    }
    explicit AutoLockForExclusiveAccess(JSRuntime* rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }
    ~AutoLockForExclusiveAccess() {
        if (runtime->numExclusiveThreads) {
            MOZ_ASSERT(runtime->exclusiveAccessOwner == PR_GetCurrentThread());
            runtime->exclusiveAccessOwner = nullptr;
            PR_Unlock(runtime->exclusiveAccessLock);
        } else {
            MOZ_ASSERT(runtime->mainThreadHasExclusiveAccess);
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
