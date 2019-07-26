





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
#include "jswrapper.h"

#if defined(JS_ION)
# include "assembler/assembler/MacroAssembler.h"
#endif
#include "jit/arm/Simulator-arm.h"
#include "jit/AsmJSSignalHandlers.h"
#include "jit/JitCompartment.h"
#include "jit/PcScriptCache.h"
#include "js/MemoryMetrics.h"
#include "js/SliceBudget.h"
#include "yarr/BumpPointerAllocator.h"

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

#ifdef JS_THREADSAFE
 Atomic<size_t> JSRuntime::liveRuntimesCount;
#else
 size_t JSRuntime::liveRuntimesCount;
#endif

const JSSecurityCallbacks js::NullSecurityCallbacks = { };

PerThreadData::PerThreadData(JSRuntime *runtime)
  : PerThreadDataFriendFields(),
    runtime_(runtime),
    ionTop(nullptr),
    jitJSContext(nullptr),
    jitStackLimit(0),
    activation_(nullptr),
    asmJSActivationStack_(nullptr),
#ifdef JS_ARM_SIMULATOR
    simulator_(nullptr),
    simulatorStackLimit_(0),
#endif
    dtoaState(nullptr),
    suppressGC(0),
    activeCompilations(0)
{}

PerThreadData::~PerThreadData()
{
    if (dtoaState)
        js_DestroyDtoaState(dtoaState);

#ifdef JS_ARM_SIMULATOR
    js_delete(simulator_);
#endif
}

bool
PerThreadData::init()
{
    dtoaState = js_NewDtoaState();
    if (!dtoaState)
        return false;

    return true;
}

static const JSWrapObjectCallbacks DefaultWrapObjectCallbacks = {
    TransparentObjectWrapper,
    nullptr,
    nullptr
};

JSRuntime::JSRuntime(JSRuntime *parentRuntime, JSUseHelperThreads useHelperThreads)
  : JS::shadow::Runtime(
#ifdef JSGC_GENERATIONAL
        &gcStoreBuffer
#endif
    ),
    mainThread(this),
    parentRuntime(parentRuntime),
    interrupt(false),
#if defined(JS_THREADSAFE) && defined(JS_ION)
    interruptPar(false),
#endif
    handlingSignal(false),
    interruptCallback(nullptr),
#ifdef JS_THREADSAFE
    interruptLock(nullptr),
    interruptLockOwner(nullptr),
    exclusiveAccessLock(nullptr),
    exclusiveAccessOwner(nullptr),
    mainThreadHasExclusiveAccess(false),
    numExclusiveThreads(0),
#else
    interruptLockTaken(false),
#endif
    systemZone(nullptr),
    numCompartments(0),
    localeCallbacks(nullptr),
    defaultLocale(nullptr),
    defaultVersion_(JSVERSION_DEFAULT),
#ifdef JS_THREADSAFE
    ownerThread_(nullptr),
#endif
    tempLifoAlloc(TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    freeLifoAlloc(TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    execAlloc_(nullptr),
    bumpAlloc_(nullptr),
    jitRuntime_(nullptr),
    selfHostingGlobal_(nullptr),
    nativeStackBase(0),
    cxCallback(nullptr),
    destroyCompartmentCallback(nullptr),
    destroyZoneCallback(nullptr),
    sweepZoneCallback(nullptr),
    compartmentNameCallback(nullptr),
    activityCallback(nullptr),
    activityCallbackArg(nullptr),
#ifdef JS_THREADSAFE
    requestDepth(0),
# ifdef DEBUG
    checkRequestDepth(0),
# endif
#endif
#ifdef DEBUG
    activeContext(nullptr),
#endif
    gcInitialized(false),
    gcSystemAvailableChunkListHead(nullptr),
    gcUserAvailableChunkListHead(nullptr),
    gcBytes(0),
    gcMaxBytes(0),
    gcMaxMallocBytes(0),
    gcNumArenasFreeCommitted(0),
    gcMarker(this),
    gcVerifyPreData(nullptr),
    gcVerifyPostData(nullptr),
    gcChunkAllocationSinceLastGC(false),
    gcNextFullGCTime(0),
    gcLastGCTime(0),
    gcJitReleaseTime(0),
    gcAllocationThreshold(30 * 1024 * 1024),
    gcHighFrequencyGC(false),
    gcHighFrequencyTimeThreshold(1000),
    gcHighFrequencyLowLimitBytes(100 * 1024 * 1024),
    gcHighFrequencyHighLimitBytes(500 * 1024 * 1024),
    gcHighFrequencyHeapGrowthMax(3.0),
    gcHighFrequencyHeapGrowthMin(1.5),
    gcLowFrequencyHeapGrowth(1.5),
    gcDynamicHeapGrowth(false),
    gcDynamicMarkSlice(false),
    gcDecommitThreshold(32 * 1024 * 1024),
    gcShouldCleanUpEverything(false),
    gcGrayBitsValid(false),
    gcIsNeeded(0),
    gcStats(thisFromCtor()),
    gcNumber(0),
    gcStartNumber(0),
    gcIsFull(false),
    gcTriggerReason(JS::gcreason::NO_REASON),
    gcStrictCompartmentChecking(false),
#ifdef DEBUG
    gcDisableStrictProxyCheckingCount(0),
#endif
    gcIncrementalState(gc::NO_INCREMENTAL),
    gcLastMarkSlice(false),
    gcSweepOnBackgroundThread(false),
    gcFoundBlackGrayEdges(false),
    gcSweepingZones(nullptr),
    gcZoneGroupIndex(0),
    gcZoneGroups(nullptr),
    gcCurrentZoneGroup(nullptr),
    gcSweepPhase(0),
    gcSweepZone(nullptr),
    gcSweepKindIndex(0),
    gcAbortSweepAfterCurrentGroup(false),
    gcArenasAllocatedDuringSweep(nullptr),
#ifdef DEBUG
    gcMarkingValidator(nullptr),
#endif
    gcInterFrameGC(0),
    gcSliceBudget(SliceBudget::Unlimited),
    gcIncrementalEnabled(true),
    gcGenerationalDisabled(0),
    gcManipulatingDeadZones(false),
    gcObjectsMarkedInDeadZones(0),
    gcPoke(false),
    heapState(Idle),
#ifdef JSGC_GENERATIONAL
    gcNursery(thisFromCtor()),
    gcStoreBuffer(thisFromCtor(), gcNursery),
#endif
#ifdef JS_GC_ZEAL
    gcZeal_(0),
    gcZealFrequency(0),
    gcNextScheduled(0),
    gcDeterministicOnly(false),
    gcIncrementalLimit(0),
#endif
    gcValidate(true),
    gcFullCompartmentChecks(false),
    gcCallback(nullptr),
    gcSliceCallback(nullptr),
    gcFinalizeCallback(nullptr),
    gcMallocBytes(0),
    gcMallocGCTriggered(false),
#ifdef JS_ARM_SIMULATOR
    simulatorRuntime_(nullptr),
#endif
    scriptAndCountsVector(nullptr),
    NaNValue(DoubleNaNValue()),
    negativeInfinityValue(DoubleValue(NegativeInfinity<double>())),
    positiveInfinityValue(DoubleValue(PositiveInfinity<double>())),
    emptyString(nullptr),
    debugMode(false),
    spsProfiler(thisFromCtor()),
    profilingScripts(false),
    alwaysPreserveCode(false),
    hadOutOfMemory(false),
    haveCreatedContext(false),
    data(nullptr),
    gcLock(nullptr),
    gcLockOwner(nullptr),
    gcHelperThread(thisFromCtor()),
    signalHandlersInstalled_(false),
    defaultFreeOp_(thisFromCtor(), false),
    debuggerMutations(0),
    securityCallbacks(const_cast<JSSecurityCallbacks *>(&NullSecurityCallbacks)),
    DOMcallbacks(nullptr),
    destroyPrincipals(nullptr),
    structuredCloneCallbacks(nullptr),
    telemetryCallback(nullptr),
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
    wrapObjectCallbacks(&DefaultWrapObjectCallbacks),
    preserveWrapperCallback(nullptr),
#ifdef DEBUG
    noGCOrAllocationCheck(0),
#endif
    jitSupportsFloatingPoint(false),
    ionPcScriptCache(nullptr),
    threadPool(this),
    defaultJSContextCallback(nullptr),
    ctypesActivityCallback(nullptr),
    forkJoinWarmup(0),
    ionReturnOverride_(MagicValue(JS_ARG_POISON)),
    useHelperThreads_(useHelperThreads),
    parallelIonCompilationEnabled_(true),
    parallelParsingEnabled_(true),
    isWorkerRuntime_(false),
#ifdef DEBUG
    enteredPolicy(nullptr),
#endif
    largeAllocationFailureCallback(nullptr),
    oomCallback(nullptr)
{
    liveRuntimesCount++;

    setGCMode(JSGC_MODE_GLOBAL);

    
    JS_INIT_CLIST(&onNewGlobalObjectWatchers);

    PodZero(&debugHooks);
    PodArrayZero(nativeStackQuota);
    PodZero(&asmJSCacheOps);

#if JS_STACK_GROWTH_DIRECTION > 0
    nativeStackLimit = UINTPTR_MAX;
#endif
}

static bool
JitSupportsFloatingPoint()
{
#if defined(JS_ION)
    if (!JSC::MacroAssembler::supportsFloatingPoint())
        return false;

#if defined(JS_ION) && WTF_ARM_ARCH_VERSION == 6
    if (!js::jit::hasVFP())
        return false;
#endif

    return true;
#else
    return false;
#endif
}

bool
JSRuntime::init(uint32_t maxbytes)
{
#ifdef JS_THREADSAFE
    ownerThread_ = PR_GetCurrentThread();

    interruptLock = PR_NewLock();
    if (!interruptLock)
        return false;

    gcLock = PR_NewLock();
    if (!gcLock)
        return false;

    exclusiveAccessLock = PR_NewLock();
    if (!exclusiveAccessLock)
        return false;
#endif

    if (!mainThread.init())
        return false;

    js::TlsPerThreadData.set(&mainThread);

    if (!threadPool.init())
        return false;

    if (!js_InitGC(this, maxbytes))
        return false;

    if (!gcMarker.init(gcMode()))
        return false;

    const char *size = getenv("JSGC_MARK_STACK_LIMIT");
    if (size)
        SetMarkStackLimit(this, atoi(size));

    ScopedJSDeletePtr<Zone> atomsZone(new_<Zone>(this));
    if (!atomsZone)
        return false;

    JS::CompartmentOptions options;
    ScopedJSDeletePtr<JSCompartment> atomsCompartment(new_<JSCompartment>(atomsZone.get(), options));
    if (!atomsCompartment || !atomsCompartment->init(nullptr))
        return false;

    zones.append(atomsZone.get());
    atomsZone->compartments.append(atomsCompartment.get());

    atomsCompartment->isSystem = true;
    atomsZone->isSystem = true;
    atomsZone->setGCLastBytes(8192, GC_NORMAL);

    atomsZone.forget();
    this->atomsCompartment_ = atomsCompartment.forget();

    if (!scriptDataTable_.init())
        return false;

    if (!evalCache.init())
        return false;

    
    gcInitialized = true;

    if (!InitRuntimeNumberState(this))
        return false;

    dateTimeInfo.updateTimeZoneAdjustment();

#ifdef JS_ARM_SIMULATOR
    simulatorRuntime_ = js::jit::CreateSimulatorRuntime();
    if (!simulatorRuntime_)
        return false;
#endif

    nativeStackBase = GetNativeStackBase();

    jitSupportsFloatingPoint = JitSupportsFloatingPoint();

#ifdef JS_ION
    signalHandlersInstalled_ = EnsureAsmJSSignalHandlersInstalled(this);
#endif

    if (!spsProfiler.init())
        return false;

    return true;
}

JSRuntime::~JSRuntime()
{
    JS_ASSERT(!isHeapBusy());

    if (gcInitialized) {
        
        sourceHook = nullptr;

        





        for (CompartmentsIter comp(this, SkipAtoms); !comp.done(); comp.next())
            CancelOffThreadIonCompile(comp, nullptr);
        CancelOffThreadParses(this);

        
        for (CompartmentsIter comp(this, SkipAtoms); !comp.done(); comp.next()) {
            comp->clearTraps(defaultFreeOp());
            if (WatchpointMap *wpmap = comp->watchpointMap)
                wpmap->clear();
        }

        
        finishAtoms();

        



        beingDestroyed_ = true;

        
        profilingScripts = false;

        JS::PrepareForFullGC(this);
        GC(this, GC_NORMAL, JS::gcreason::DESTROY_RUNTIME);
    }

    



    finishSelfHosting();

#ifdef JS_THREADSAFE
    JS_ASSERT(!exclusiveAccessOwner);
    if (exclusiveAccessLock)
        PR_DestroyLock(exclusiveAccessLock);

    
    JS_ASSERT(!numExclusiveThreads);
    mainThreadHasExclusiveAccess = true;

    JS_ASSERT(!interruptLockOwner);
    if (interruptLock)
        PR_DestroyLock(interruptLock);
#endif

    



    FreeScriptData(this);

#ifdef DEBUG
    
    if (hasContexts()) {
        unsigned cxcount = 0;
        for (ContextIter acx(this); !acx.done(); acx.next()) {
            fprintf(stderr,
"JS API usage error: found live context at %p\n",
                    (void *) acx.get());
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

    js_FinishGC(this);
    atomsCompartment_ = nullptr;

#ifdef JS_THREADSAFE
    if (gcLock)
        PR_DestroyLock(gcLock);
#endif

    js_free(defaultLocale);
    js_delete(bumpAlloc_);
    js_delete(mathCache_);
#ifdef JS_ION
    js_delete(jitRuntime_);
#endif
    js_delete(execAlloc_);  

    js_delete(ionPcScriptCache);

#ifdef JSGC_GENERATIONAL
    gcStoreBuffer.disable();
    gcNursery.disable();
#endif

#ifdef JS_ARM_SIMULATOR
    js::jit::DestroySimulatorRuntime(simulatorRuntime_);
#endif

    DebugOnly<size_t> oldCount = liveRuntimesCount--;
    JS_ASSERT(oldCount > 0);

#ifdef JS_THREADSAFE
    js::TlsPerThreadData.set(nullptr);
#endif
}

void
NewObjectCache::clearNurseryObjects(JSRuntime *rt)
{
    for (unsigned i = 0; i < mozilla::ArrayLength(entries); ++i) {
        Entry &e = entries[i];
        JSObject *obj = reinterpret_cast<JSObject *>(&e.templateObject);
        if (IsInsideNursery(rt, e.key) ||
            IsInsideNursery(rt, obj->slots) ||
            IsInsideNursery(rt, obj->elements))
        {
            PodZero(&e);
        }
    }
}

void
JSRuntime::resetJitStackLimit()
{
    AutoLockForInterrupt lock(this);
    mainThread.setJitStackLimit(mainThread.nativeStackLimit[js::StackForUntrustedScript]);

#ifdef JS_ARM_SIMULATOR
    mainThread.setJitStackLimit(js::jit::Simulator::StackLimit());
#endif
 }

void
JSRuntime::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::RuntimeSizes *rtSizes)
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

    rtSizes->regexpData += bumpAlloc_ ? bumpAlloc_->sizeOfNonHeapData() : 0;

    rtSizes->interpreterStack += interpreterStack_.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->mathCache += mathCache_ ? mathCache_->sizeOfIncludingThis(mallocSizeOf) : 0;

    rtSizes->sourceDataCache += sourceDataCache.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->scriptData += scriptDataTable().sizeOfExcludingThis(mallocSizeOf);
    for (ScriptDataTable::Range r = scriptDataTable().all(); !r.empty(); r.popFront())
        rtSizes->scriptData += mallocSizeOf(r.front());

    if (execAlloc_)
        execAlloc_->addSizeOfCode(&rtSizes->code);
#ifdef JS_ION
    {
        AutoLockForInterrupt lock(this);
        if (jitRuntime()) {
            if (JSC::ExecutableAllocator *ionAlloc = jitRuntime()->ionAlloc(this))
                ionAlloc->addSizeOfCode(&rtSizes->code);
        }
    }
#endif

    rtSizes->gc.marker += gcMarker.sizeOfExcludingThis(mallocSizeOf);
#ifdef JSGC_GENERATIONAL
    rtSizes->gc.nurseryCommitted += gcNursery.sizeOfHeapCommitted();
    rtSizes->gc.nurseryDecommitted += gcNursery.sizeOfHeapDecommitted();
    gcStoreBuffer.addSizeOfExcludingThis(mallocSizeOf, &rtSizes->gc);
#endif
}

static bool
SignalBasedTriggersDisabled()
{
  
  
  return !!getenv("JS_DISABLE_SLOW_SCRIPT_SIGNALS");
}

void
JSRuntime::requestInterrupt(InterruptMode mode)
{
    AutoLockForInterrupt lock(this);

    





    mainThread.setJitStackLimit(-1);

    interrupt = true;

#ifdef JS_ION
#ifdef JS_THREADSAFE
    RequestInterruptForForkJoin(this, mode);
#endif

    



    if (!SignalBasedTriggersDisabled()) {
        RequestInterruptForAsmJSCode(this);
        jit::RequestInterruptForIonCode(this, mode);
    }
#endif
}

JSC::ExecutableAllocator *
JSRuntime::createExecutableAllocator(JSContext *cx)
{
    JS_ASSERT(!execAlloc_);
    JS_ASSERT(cx->runtime() == this);

    execAlloc_ = js_new<JSC::ExecutableAllocator>();
    if (!execAlloc_)
        js_ReportOutOfMemory(cx);
    return execAlloc_;
}

WTF::BumpPointerAllocator *
JSRuntime::createBumpPointerAllocator(JSContext *cx)
{
    JS_ASSERT(!bumpAlloc_);
    JS_ASSERT(cx->runtime() == this);

    bumpAlloc_ = js_new<WTF::BumpPointerAllocator>();
    if (!bumpAlloc_)
        js_ReportOutOfMemory(cx);
    return bumpAlloc_;
}

MathCache *
JSRuntime::createMathCache(JSContext *cx)
{
    JS_ASSERT(!mathCache_);
    JS_ASSERT(cx->runtime() == this);

    MathCache *newMathCache = js_new<MathCache>();
    if (!newMathCache) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    mathCache_ = newMathCache;
    return mathCache_;
}

bool
JSRuntime::setDefaultLocale(const char *locale)
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

const char *
JSRuntime::getDefaultLocale()
{
    if (defaultLocale)
        return defaultLocale;

    char *locale, *lang, *p;
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
JSRuntime::setGCMaxMallocBytes(size_t value)
{
    



    gcMaxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
    resetGCMallocBytes();
    for (ZonesIter zone(this, WithAtoms); !zone.done(); zone.next())
        zone->setGCMaxMallocBytes(value);
}

void
JSRuntime::updateMallocCounter(size_t nbytes)
{
    updateMallocCounter(nullptr, nbytes);
}

void
JSRuntime::updateMallocCounter(JS::Zone *zone, size_t nbytes)
{
    
    gcMallocBytes -= ptrdiff_t(nbytes);
    if (MOZ_UNLIKELY(gcMallocBytes <= 0))
        onTooMuchMalloc();
    else if (zone)
        zone->updateMallocCounter(nbytes);
}

JS_FRIEND_API(void)
JSRuntime::onTooMuchMalloc()
{
    if (!CurrentThreadCanAccessRuntime(this))
        return;

    if (!gcMallocGCTriggered)
        gcMallocGCTriggered = TriggerGC(this, JS::gcreason::TOO_MUCH_MALLOC);
}

JS_FRIEND_API(void *)
JSRuntime::onOutOfMemory(void *p, size_t nbytes)
{
    return onOutOfMemory(p, nbytes, nullptr);
}

JS_FRIEND_API(void *)
JSRuntime::onOutOfMemory(void *p, size_t nbytes, JSContext *cx)
{
    if (isHeapBusy())
        return nullptr;

    



    JS::ShrinkGCBuffers(this);
    gcHelperThread.waitBackgroundSweepOrAllocEnd();
    if (!p)
        p = js_malloc(nbytes);
    else if (p == reinterpret_cast<void *>(1))
        p = js_calloc(nbytes);
    else
      p = js_realloc(p, nbytes);
    if (p)
        return p;
    if (cx)
        js_ReportOutOfMemory(cx);
    return nullptr;
}

bool
JSRuntime::activeGCInAtomsZone()
{
    Zone *zone = atomsCompartment_->zone();
    return zone->needsBarrier() || zone->isGCScheduled() || zone->wasGCStarted();
}

#ifdef JS_THREADSAFE

void
JSRuntime::setUsedByExclusiveThread(Zone *zone)
{
    JS_ASSERT(!zone->usedByExclusiveThread);
    zone->usedByExclusiveThread = true;
    numExclusiveThreads++;
}

void
JSRuntime::clearUsedByExclusiveThread(Zone *zone)
{
    JS_ASSERT(zone->usedByExclusiveThread);
    zone->usedByExclusiveThread = false;
    numExclusiveThreads--;
}

bool
js::CurrentThreadCanAccessRuntime(JSRuntime *rt)
{
    return rt->ownerThread_ == PR_GetCurrentThread() && !InParallelSection();
}

bool
js::CurrentThreadCanAccessZone(Zone *zone)
{
    if (CurrentThreadCanAccessRuntime(zone->runtime_))
        return true;
    if (InParallelSection()) {
        DebugOnly<PerThreadData *> pt = js::TlsPerThreadData.get();
        JS_ASSERT(pt && pt->associatedWith(zone->runtime_));
        return true;
    }

    
    
    
    return zone->usedByExclusiveThread;
}

#else 

bool
js::CurrentThreadCanAccessRuntime(JSRuntime *rt)
{
    return true;
}

bool
js::CurrentThreadCanAccessZone(Zone *zone)
{
    return true;
}

#endif 

#ifdef DEBUG

void
JSRuntime::assertCanLock(RuntimeLock which)
{
#ifdef JS_THREADSAFE
    
    
    
    switch (which) {
      case ExclusiveAccessLock:
        JS_ASSERT(exclusiveAccessOwner != PR_GetCurrentThread());
      case WorkerThreadStateLock:
        JS_ASSERT(!WorkerThreadState().isLocked());
      case InterruptLock:
        JS_ASSERT(!currentThreadOwnsInterruptLock());
      case GCLock:
        JS_ASSERT(gcLockOwner != PR_GetCurrentThread());
        break;
      default:
        MOZ_CRASH();
    }
#endif 
}

void
js::AssertCurrentThreadCanLock(RuntimeLock which)
{
#ifdef JS_THREADSAFE
    PerThreadData *pt = TlsPerThreadData.get();
    if (pt && pt->runtime_)
        pt->runtime_->assertCanLock(which);
#endif
}

#endif 
