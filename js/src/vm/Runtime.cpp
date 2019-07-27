





#include "vm/Runtime-inl.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/Atomics.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/ThreadLocal.h"

#include <locale.h>
#include <string.h>

#ifdef JS_CAN_CHECK_THREADSAFE_ACCESSES
# include <sys/mman.h>
#endif

#include "jsatom.h"
#include "jsdtoa.h"
#include "jsgc.h"
#include "jsmath.h"
#include "jsnativestack.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jswatchpoint.h"
#include "jswin.h"
#include "jswrapper.h"

#include "asmjs/AsmJSSignalHandlers.h"
#include "jit/arm/Simulator-arm.h"
#include "jit/arm64/vixl/Simulator-vixl.h"
#include "jit/JitCompartment.h"
#include "jit/mips/Simulator-mips.h"
#include "jit/PcScriptCache.h"
#include "js/MemoryMetrics.h"
#include "js/SliceBudget.h"
#include "vm/Debugger.h"

#include "jscntxtinlines.h"
#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

using mozilla::Atomic;
using mozilla::DebugOnly;
using mozilla::NegativeInfinity;
using mozilla::PodZero;
using mozilla::PodArrayZero;
using mozilla::PositiveInfinity;
using mozilla::ThreadLocal;
using JS::GenericNaN;
using JS::DoubleNaNValue;

 ThreadLocal<PerThreadData*> js::TlsPerThreadData;
 Atomic<size_t> JSRuntime::liveRuntimesCount;

namespace js {
    bool gCanUseExtraThreads = true;
};

void
js::DisableExtraThreads()
{
    gCanUseExtraThreads = false;
}

const JSSecurityCallbacks js::NullSecurityCallbacks = { };

PerThreadData::PerThreadData(JSRuntime* runtime)
  : PerThreadDataFriendFields(),
    runtime_(runtime),
#ifdef JS_TRACE_LOGGING
    traceLogger(nullptr),
#endif
    autoFlushICache_(nullptr),
    dtoaState(nullptr),
    suppressGC(0),
#ifdef DEBUG
    ionCompiling(false),
    ionCompilingSafeForMinorGC(false),
    gcSweeping(false),
#endif
    activeCompilations(0)
{}

PerThreadData::~PerThreadData()
{
    if (dtoaState)
        DestroyDtoaState(dtoaState);
}

bool
PerThreadData::init()
{
    dtoaState = NewDtoaState();
    if (!dtoaState)
        return false;

    return true;
}

static const JSWrapObjectCallbacks DefaultWrapObjectCallbacks = {
    TransparentObjectWrapper,
    nullptr
};

static size_t
ReturnZeroSize(const void* p)
{
    return 0;
}

JSRuntime::JSRuntime(JSRuntime* parentRuntime)
  : mainThread(this),
    jitTop(nullptr),
    jitJSContext(nullptr),
    jitActivation(nullptr),
    jitStackLimit_(0xbad),
    activation_(nullptr),
    profilingActivation_(nullptr),
    profilerSampleBufferGen_(0),
    profilerSampleBufferLapCount_(1),
    asmJSActivationStack_(nullptr),
    asyncStackForNewActivations(nullptr),
    asyncCauseForNewActivations(nullptr),
    entryMonitor(nullptr),
    parentRuntime(parentRuntime),
    interrupt_(false),
    telemetryCallback(nullptr),
    handlingSignal(false),
    interruptCallback(nullptr),
    exclusiveAccessLock(nullptr),
    exclusiveAccessOwner(nullptr),
    mainThreadHasExclusiveAccess(false),
    numExclusiveThreads(0),
    numCompartments(0),
    localeCallbacks(nullptr),
    defaultLocale(nullptr),
    defaultVersion_(JSVERSION_DEFAULT),
    ownerThread_(nullptr),
    ownerThreadNative_(0),
    tempLifoAlloc(TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    jitRuntime_(nullptr),
    selfHostingGlobal_(nullptr),
    nativeStackBase(GetNativeStackBase()),
    cxCallback(nullptr),
    destroyCompartmentCallback(nullptr),
    destroyZoneCallback(nullptr),
    sweepZoneCallback(nullptr),
    compartmentNameCallback(nullptr),
    activityCallback(nullptr),
    activityCallbackArg(nullptr),
    requestDepth(0),
#ifdef DEBUG
    checkRequestDepth(0),
    activeContext(nullptr),
#endif
    gc(thisFromCtor()),
    gcInitialized(false),
#ifdef JS_SIMULATOR
    simulator_(nullptr),
#endif
    scriptAndCountsVector(nullptr),
    NaNValue(DoubleNaNValue()),
    negativeInfinityValue(DoubleValue(NegativeInfinity<double>())),
    positiveInfinityValue(DoubleValue(PositiveInfinity<double>())),
    emptyString(nullptr),
    spsProfiler(thisFromCtor()),
    profilingScripts(false),
    suppressProfilerSampling(false),
    hadOutOfMemory(false),
    handlingInitFailure(false),
    haveCreatedContext(false),
    allowRelazificationForTesting(false),
    data(nullptr),
    signalHandlersInstalled_(false),
    canUseSignalHandlers_(false),
    defaultFreeOp_(thisFromCtor()),
    debuggerMutations(0),
    securityCallbacks(const_cast<JSSecurityCallbacks*>(&NullSecurityCallbacks)),
    DOMcallbacks(nullptr),
    destroyPrincipals(nullptr),
    structuredCloneCallbacks(nullptr),
    errorReporter(nullptr),
    linkedAsmJSModules(nullptr),
    propertyRemovals(0),
#if !EXPOSE_INTL_API
    thousandsSeparator(0),
    decimalSeparator(0),
    numGrouping(0),
#endif
    mathCache_(nullptr),
    activeCompilations_(0),
    keepAtoms_(0),
    trustedPrincipals_(nullptr),
    beingDestroyed_(false),
    atoms_(nullptr),
    atomsCompartment_(nullptr),
    staticStrings(nullptr),
    commonNames(nullptr),
    permanentAtoms(nullptr),
    wellKnownSymbols(nullptr),
    wrapObjectCallbacks(&DefaultWrapObjectCallbacks),
    preserveWrapperCallback(nullptr),
    jitSupportsFloatingPoint(false),
    jitSupportsSimd(false),
    ionPcScriptCache(nullptr),
    scriptEnvironmentPreparer(nullptr),
    ctypesActivityCallback(nullptr),
    offthreadIonCompilationEnabled_(true),
    parallelParsingEnabled_(true),
    autoWritableJitCodeActive_(false),
#ifdef DEBUG
    enteredPolicy(nullptr),
#endif
    largeAllocationFailureCallback(nullptr),
    oomCallback(nullptr),
    debuggerMallocSizeOf(ReturnZeroSize),
    lastAnimationTime(0)
{
    setGCStoreBufferPtr(&gc.storeBuffer);

    liveRuntimesCount++;

    
    JS_INIT_CLIST(&onNewGlobalObjectWatchers);

    PodArrayZero(nativeStackQuota);
    PodZero(&asmJSCacheOps);
}

static bool
SignalBasedTriggersDisabled()
{
  
  
  return !!getenv("JS_DISABLE_SLOW_SCRIPT_SIGNALS") || !!getenv("JS_NO_SIGNALS");
}

bool
JSRuntime::init(uint32_t maxbytes, uint32_t maxNurseryBytes)
{
    ownerThread_ = PR_GetCurrentThread();

    
    
#ifdef XP_WIN
    size_t openFlags = THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME |
                       THREAD_QUERY_INFORMATION;
    HANDLE self = OpenThread(openFlags, false, GetCurrentThreadId());
    if (!self)
        return false;
    static_assert(sizeof(HANDLE) <= sizeof(ownerThreadNative_), "need bigger field");
    ownerThreadNative_ = (size_t)self;
#else
    static_assert(sizeof(pthread_t) <= sizeof(ownerThreadNative_), "need bigger field");
    ownerThreadNative_ = (size_t)pthread_self();
#endif

    exclusiveAccessLock = PR_NewLock();
    if (!exclusiveAccessLock)
        return false;

    if (!mainThread.init())
        return false;

    if (!regexpStack.init())
        return false;

    js::TlsPerThreadData.set(&mainThread);

    if (CanUseExtraThreads())
        EnsureHelperThreadsInitialized();

    if (!gc.init(maxbytes, maxNurseryBytes))
        return false;

    const char* size = getenv("JSGC_MARK_STACK_LIMIT");
    if (size)
        gc.setMarkStackLimit(atoi(size));

    ScopedJSDeletePtr<Zone> atomsZone(new_<Zone>(this));
    if (!atomsZone || !atomsZone->init(true))
        return false;

    JS::CompartmentOptions options;
    ScopedJSDeletePtr<JSCompartment> atomsCompartment(new_<JSCompartment>(atomsZone.get(), options));
    if (!atomsCompartment || !atomsCompartment->init(nullptr))
        return false;

    gc.zones.append(atomsZone.get());
    atomsZone->compartments.append(atomsCompartment.get());

    atomsCompartment->setIsSystem(true);

    atomsZone.forget();
    this->atomsCompartment_ = atomsCompartment.forget();

    if (!symbolRegistry_.init())
        return false;

    if (!scriptDataTable_.init())
        return false;

    if (!evalCache.init())
        return false;

    if (!compressedSourceSet.init())
        return false;

    
    gcInitialized = true;

    if (!InitRuntimeNumberState(this))
        return false;

    dateTimeInfo.updateTimeZoneAdjustment();

#ifdef JS_SIMULATOR
    simulator_ = js::jit::Simulator::Create();
    if (!simulator_)
        return false;
#endif

    jitSupportsFloatingPoint = js::jit::JitSupportsFloatingPoint();
    jitSupportsSimd = js::jit::JitSupportsSimd();

    signalHandlersInstalled_ = EnsureSignalHandlersInstalled(this);
    canUseSignalHandlers_ = signalHandlersInstalled_ && !SignalBasedTriggersDisabled();

    if (!spsProfiler.init())
        return false;

    if (!fx.initInstance())
        return false;

    return true;
}

JSRuntime::~JSRuntime()
{
    MOZ_ASSERT(!isHeapBusy());

    fx.destroyInstance();

    if (gcInitialized) {
        
        sourceHook = nullptr;

        





        for (CompartmentsIter comp(this, SkipAtoms); !comp.done(); comp.next())
            CancelOffThreadIonCompile(comp, nullptr);
        CancelOffThreadParses(this);

        
        for (CompartmentsIter comp(this, SkipAtoms); !comp.done(); comp.next()) {
            if (WatchpointMap* wpmap = comp->watchpointMap)
                wpmap->clear();
        }

        
        finishAtoms();

        
        gc.finishRoots();

        



        beingDestroyed_ = true;

        
        profilingScripts = false;

        
        profilerSampleBufferGen_ = UINT32_MAX;

        JS::PrepareForFullGC(this);
        gc.gc(GC_NORMAL, JS::gcreason::DESTROY_RUNTIME);
    }

    



    finishSelfHosting();

    MOZ_ASSERT(!exclusiveAccessOwner);
    if (exclusiveAccessLock)
        PR_DestroyLock(exclusiveAccessLock);

    
    MOZ_ASSERT(!numExclusiveThreads);
    mainThreadHasExclusiveAccess = true;

    



    FreeScriptData(this);

#ifdef DEBUG
    
    if (hasContexts()) {
        unsigned cxcount = 0;
        for (ContextIter acx(this); !acx.done(); acx.next()) {
            fprintf(stderr,
"JS API usage error: found live context at %p\n",
                    (void*) acx.get());
            cxcount++;
        }
        fprintf(stderr,
"JS API usage error: %u context%s left in runtime upon JS_DestroyRuntime.\n",
                cxcount, (cxcount == 1) ? "" : "s");
    }
#endif

#if !EXPOSE_INTL_API
    FinishRuntimeNumberState(this);
#endif

    gc.finish();
    atomsCompartment_ = nullptr;

    js_free(defaultLocale);
    js_delete(mathCache_);
    js_delete(jitRuntime_);

    js_delete(ionPcScriptCache);

    gc.storeBuffer.disable();
    gc.nursery.disable();

#ifdef JS_SIMULATOR
    js::jit::Simulator::Destroy(simulator_);
#endif

    DebugOnly<size_t> oldCount = liveRuntimesCount--;
    MOZ_ASSERT(oldCount > 0);

    js::TlsPerThreadData.set(nullptr);

#ifdef XP_WIN
    if (ownerThreadNative_)
        CloseHandle((HANDLE)ownerThreadNative_);
#endif
}

void
JSRuntime::addTelemetry(int id, uint32_t sample, const char* key)
{
    if (telemetryCallback)
        (*telemetryCallback)(id, sample, key);
}

void
JSRuntime::setTelemetryCallback(JSRuntime* rt, JSAccumulateTelemetryDataCallback callback)
{
    rt->telemetryCallback = callback;
}

void
NewObjectCache::clearNurseryObjects(JSRuntime* rt)
{
    for (unsigned i = 0; i < mozilla::ArrayLength(entries); ++i) {
        Entry& e = entries[i];
        NativeObject* obj = reinterpret_cast<NativeObject*>(&e.templateObject);
        if (IsInsideNursery(e.key) ||
            rt->gc.nursery.isInside(obj->slots_) ||
            rt->gc.nursery.isInside(obj->elements_))
        {
            PodZero(&e);
        }
    }
}

void
JSRuntime::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::RuntimeSizes* rtSizes)
{
    
    AutoLockForExclusiveAccess lock(this);

    rtSizes->object += mallocSizeOf(this);

    rtSizes->atomsTable += atoms().sizeOfIncludingThis(mallocSizeOf);

    if (!parentRuntime) {
        rtSizes->atomsTable += mallocSizeOf(staticStrings);
        rtSizes->atomsTable += mallocSizeOf(commonNames);
        rtSizes->atomsTable += permanentAtoms->sizeOfIncludingThis(mallocSizeOf);
    }

    for (ContextIter acx(this); !acx.done(); acx.next())
        rtSizes->contexts += acx->sizeOfIncludingThis(mallocSizeOf);

    rtSizes->dtoa += mallocSizeOf(mainThread.dtoaState);

    rtSizes->temporary += tempLifoAlloc.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->interpreterStack += interpreterStack_.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->mathCache += mathCache_ ? mathCache_->sizeOfIncludingThis(mallocSizeOf) : 0;

    rtSizes->uncompressedSourceCache += uncompressedSourceCache.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->compressedSourceSet += compressedSourceSet.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->scriptData += scriptDataTable().sizeOfExcludingThis(mallocSizeOf);
    for (ScriptDataTable::Range r = scriptDataTable().all(); !r.empty(); r.popFront())
        rtSizes->scriptData += mallocSizeOf(r.front());

    if (jitRuntime_)
        jitRuntime_->execAlloc().addSizeOfCode(&rtSizes->code);

    rtSizes->gc.marker += gc.marker.sizeOfExcludingThis(mallocSizeOf);
    rtSizes->gc.nurseryCommitted += gc.nursery.sizeOfHeapCommitted();
    rtSizes->gc.nurseryDecommitted += gc.nursery.sizeOfHeapDecommitted();
    rtSizes->gc.nurseryMallocedBuffers += gc.nursery.sizeOfMallocedBuffers(mallocSizeOf);
    gc.storeBuffer.addSizeOfExcludingThis(mallocSizeOf, &rtSizes->gc);
}

static bool
InvokeInterruptCallback(JSContext* cx)
{
    MOZ_ASSERT(cx->runtime()->requestDepth >= 1);

    cx->runtime()->gc.gcIfRequested();

    
    
    jit::AttachFinishedCompilations(cx);

    
    
    
    JSInterruptCallback cb = cx->runtime()->interruptCallback;
    if (!cb)
        return true;

    if (cb(cx)) {
        
        
        if (cx->compartment()->isDebuggee()) {
            ScriptFrameIter iter(cx);
            if (iter.script()->stepModeEnabled()) {
                RootedValue rval(cx);
                switch (Debugger::onSingleStep(cx, &rval)) {
                  case JSTRAP_ERROR:
                    return false;
                  case JSTRAP_CONTINUE:
                    return true;
                  case JSTRAP_RETURN:
                    
                    Debugger::propagateForcedReturn(cx, iter.abstractFramePtr(), rval);
                    return false;
                  case JSTRAP_THROW:
                    cx->setPendingException(rval);
                    return false;
                  default:;
                }
            }
        }

        return true;
    }

    
    
    JSString* stack = ComputeStackString(cx);
    JSFlatString* flat = stack ? stack->ensureFlat(cx) : nullptr;

    const char16_t* chars;
    AutoStableStringChars stableChars(cx);
    if (flat && stableChars.initTwoByte(cx, flat))
        chars = stableChars.twoByteRange().start().get();
    else
        chars = MOZ_UTF16("(stack not available)");
    JS_ReportErrorFlagsAndNumberUC(cx, JSREPORT_WARNING, GetErrorMessage, nullptr,
                                   JSMSG_TERMINATED, chars);

    return false;
}

void
JSRuntime::resetJitStackLimit()
{
    
    
    
#ifdef JS_SIMULATOR
    jitStackLimit_ = jit::Simulator::StackLimit();
#else
    jitStackLimit_ = mainThread.nativeStackLimit[StackForUntrustedScript];
#endif
}

void
JSRuntime::initJitStackLimit()
{
    resetJitStackLimit();
}

void
JSRuntime::requestInterrupt(InterruptMode mode)
{
    interrupt_ = true;
    jitStackLimit_ = UINTPTR_MAX;

    if (mode == JSRuntime::RequestInterruptUrgent) {
        
        
        
        
        
        fx.lock();
        if (fx.isWaiting())
            fx.wake(FutexRuntime::WakeForJSInterrupt);
        fx.unlock();
        InterruptRunningJitCode(this);
    }
}

bool
JSRuntime::handleInterrupt(JSContext* cx)
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(cx->runtime()));
    if (interrupt_ || jitStackLimit_ == UINTPTR_MAX) {
        interrupt_ = false;
        resetJitStackLimit();
        return InvokeInterruptCallback(cx);
    }
    return true;
}

MathCache*
JSRuntime::createMathCache(JSContext* cx)
{
    MOZ_ASSERT(!mathCache_);
    MOZ_ASSERT(cx->runtime() == this);

    MathCache* newMathCache = js_new<MathCache>();
    if (!newMathCache) {
        ReportOutOfMemory(cx);
        return nullptr;
    }

    mathCache_ = newMathCache;
    return mathCache_;
}

bool
JSRuntime::setDefaultLocale(const char* locale)
{
    if (!locale)
        return false;
    resetDefaultLocale();
    defaultLocale = JS_strdup(this, locale);
    return defaultLocale != nullptr;
}

void
JSRuntime::resetDefaultLocale()
{
    js_free(defaultLocale);
    defaultLocale = nullptr;
}

const char*
JSRuntime::getDefaultLocale()
{
    if (defaultLocale)
        return defaultLocale;

    char* locale;
    char* lang;
    char* p;
#ifdef HAVE_SETLOCALE
    locale = setlocale(LC_ALL, nullptr);
#else
    locale = getenv("LANG");
#endif
    
    if (!locale || !strcmp(locale, "C"))
        locale = const_cast<char*>("und");
    lang = JS_strdup(this, locale);
    if (!lang)
        return nullptr;
    if ((p = strchr(lang, '.')))
        *p = '\0';
    while ((p = strchr(lang, '_')))
        *p = '-';

    defaultLocale = lang;
    return defaultLocale;
}

void
JSRuntime::triggerActivityCallback(bool active)
{
    if (!activityCallback)
        return;

    






    AutoSuppressGC suppress(this);

    activityCallback(activityCallbackArg, active);
}

void
JSRuntime::updateMallocCounter(size_t nbytes)
{
    updateMallocCounter(nullptr, nbytes);
}

void
JSRuntime::updateMallocCounter(JS::Zone* zone, size_t nbytes)
{
    gc.updateMallocCounter(zone, nbytes);
}

JS_FRIEND_API(void*)
JSRuntime::onOutOfMemory(AllocFunction allocFunc, size_t nbytes, void* reallocPtr,
                         JSContext* maybecx)
{
    MOZ_ASSERT_IF(allocFunc != AllocFunction::Realloc, !reallocPtr);

    if (isHeapBusy())
        return nullptr;

    if (!oom::IsSimulatedOOMAllocation()) {
        



        gc.onOutOfMallocMemory();
        void* p;
        switch (allocFunc) {
          case AllocFunction::Malloc:
            p = js_malloc(nbytes);
            break;
          case AllocFunction::Calloc:
            p = js_calloc(nbytes);
            break;
          case AllocFunction::Realloc:
            p = js_realloc(reallocPtr, nbytes);
            break;
          default:
            MOZ_CRASH();
        }
        if (p)
            return p;
    }

    if (maybecx)
        ReportOutOfMemory(maybecx);
    return nullptr;
}

void*
JSRuntime::onOutOfMemoryCanGC(AllocFunction allocFunc, size_t bytes, void* reallocPtr)
{
    if (largeAllocationFailureCallback && bytes >= LARGE_ALLOCATION)
        largeAllocationFailureCallback(largeAllocationFailureCallbackData);
    return onOutOfMemory(allocFunc, bytes, reallocPtr);
}

bool
JSRuntime::activeGCInAtomsZone()
{
    Zone* zone = atomsCompartment_->zone();
    return zone->needsIncrementalBarrier() || zone->isGCScheduled() || zone->wasGCStarted();
}

void
JSRuntime::setUsedByExclusiveThread(Zone* zone)
{
    MOZ_ASSERT(!zone->usedByExclusiveThread);
    zone->usedByExclusiveThread = true;
    numExclusiveThreads++;
}

void
JSRuntime::clearUsedByExclusiveThread(Zone* zone)
{
    MOZ_ASSERT(zone->usedByExclusiveThread);
    zone->usedByExclusiveThread = false;
    numExclusiveThreads--;
    if (gc.fullGCForAtomsRequested() && !keepAtoms())
        gc.triggerFullGCForAtoms();
}

bool
js::CurrentThreadCanAccessRuntime(JSRuntime* rt)
{
    return rt->ownerThread_ == PR_GetCurrentThread();
}

bool
js::CurrentThreadCanAccessZone(Zone* zone)
{
    if (CurrentThreadCanAccessRuntime(zone->runtime_))
        return true;

    
    
    
    return zone->usedByExclusiveThread;
}

#ifdef DEBUG

void
JSRuntime::assertCanLock(RuntimeLock which)
{
    
    
    
    switch (which) {
      case ExclusiveAccessLock:
        MOZ_ASSERT(exclusiveAccessOwner != PR_GetCurrentThread());
      case HelperThreadStateLock:
        MOZ_ASSERT(!HelperThreadState().isLocked());
      case GCLock:
        gc.assertCanLock();
        break;
      default:
        MOZ_CRASH();
    }
}

void
js::AssertCurrentThreadCanLock(RuntimeLock which)
{
    PerThreadData* pt = TlsPerThreadData.get();
    if (pt && pt->runtime_)
        pt->runtime_->assertCanLock(which);
}

#endif 

JS_FRIEND_API(void)
JS::UpdateJSRuntimeProfilerSampleBufferGen(JSRuntime* runtime, uint32_t generation,
                                           uint32_t lapCount)
{
    runtime->setProfilerSampleBufferGen(generation);
    runtime->updateProfilerSampleBufferLapCount(lapCount);
}

JS_FRIEND_API(bool)
JS::IsProfilingEnabledForRuntime(JSRuntime* runtime)
{
    MOZ_ASSERT(runtime);
    return runtime->spsProfiler.enabled();
}

void
js::ResetStopwatches(JSRuntime* rt)
{
    MOZ_ASSERT(rt);
    rt->stopwatch.reset();
}

bool
js::SetStopwatchIsMonitoringJank(JSRuntime* rt, bool value)
{
    return rt->stopwatch.setIsMonitoringJank(value);
}
bool
js::GetStopwatchIsMonitoringJank(JSRuntime* rt)
{
    return rt->stopwatch.isMonitoringJank();
}

bool
js::SetStopwatchIsMonitoringCPOW(JSRuntime* rt, bool value)
{
    return rt->stopwatch.setIsMonitoringCPOW(value);
}
bool
js::GetStopwatchIsMonitoringCPOW(JSRuntime* rt)
{
    return rt->stopwatch.isMonitoringCPOW();
}

js::PerformanceGroupHolder::~PerformanceGroupHolder()
{
    unlink();
}

void*
js::PerformanceGroupHolder::getHashKey(JSContext* cx)
{
    if (runtime_->stopwatch.currentPerfGroupCallback) {
        return (*runtime_->stopwatch.currentPerfGroupCallback)(cx);
    }

    
    return nullptr;
}

void
js::PerformanceGroupHolder::unlink()
{
    if (!group_) {
        
        return;
    }

    js::PerformanceGroup* group = group_;
    group_ = nullptr;

    if (group->decRefCount() > 0) {
        
        return;
    }


    JSRuntime::Stopwatch::Groups::Ptr ptr =
        runtime_->stopwatch.groups_.lookup(group->key_);
    MOZ_ASSERT(ptr);
    runtime_->stopwatch.groups_.remove(ptr);
    js_delete(group);
}

PerformanceGroup*
js::PerformanceGroupHolder::getGroup(JSContext* cx)
{
    if (group_)
        return group_;

    void* key = getHashKey(cx);
    JSRuntime::Stopwatch::Groups::AddPtr ptr =
        runtime_->stopwatch.groups_.lookupForAdd(key);
    if (ptr) {
        group_ = ptr->value();
        MOZ_ASSERT(group_);
    } else {
        group_ = runtime_->new_<PerformanceGroup>(cx, key);
        runtime_->stopwatch.groups_.add(ptr, key, group_);
    }

    group_->incRefCount();

    return group_;
}

PerformanceData*
js::GetPerformanceData(JSRuntime* rt)
{
    return &rt->stopwatch.performance;
}

js::PerformanceGroup::PerformanceGroup(JSContext* cx, void* key)
  : uid(cx->runtime()->stopwatch.uniqueId())
  , stopwatch_(nullptr)
  , iteration_(0)
  , key_(key)
  , refCount_(0)
{
}

void
JS_SetCurrentPerfGroupCallback(JSRuntime *rt, JSCurrentPerfGroupCallback cb)
{
    rt->stopwatch.currentPerfGroupCallback = cb;
}
