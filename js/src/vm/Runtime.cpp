





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

#include "asmjs/AsmJSSignalHandlers.h"
#include "jit/arm/Simulator-arm.h"
#include "jit/JitCompartment.h"
#include "jit/mips/Simulator-mips.h"
#include "jit/PcScriptCache.h"
#include "js/MemoryMetrics.h"
#include "js/SliceBudget.h"

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

PerThreadData::PerThreadData(JSRuntime *runtime)
  : PerThreadDataFriendFields(),
    runtime_(runtime),
    jitTop(nullptr),
    jitJSContext(nullptr),
    jitStackLimit(0),
#ifdef JS_TRACE_LOGGING
    traceLogger(nullptr),
#endif
    activation_(nullptr),
    profilingActivation_(nullptr),
    asmJSActivationStack_(nullptr),
    autoFlushICache_(nullptr),
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    simulator_(nullptr),
    simulatorStackLimit_(0),
#endif
    dtoaState(nullptr),
    suppressGC(0),
#ifdef DEBUG
    ionCompiling(false),
#endif
    activeCompilations(0)
{}

PerThreadData::~PerThreadData()
{
    if (dtoaState)
        js_DestroyDtoaState(dtoaState);

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js_delete(simulator_);
#endif
}

bool
PerThreadData::init()
{
    dtoaState = js_NewDtoaState();
    if (!dtoaState)
        return false;

    if (!regexpStack.init())
        return false;

    return true;
}

static const JSWrapObjectCallbacks DefaultWrapObjectCallbacks = {
    TransparentObjectWrapper,
    nullptr
};

JSRuntime::JSRuntime(JSRuntime *parentRuntime)
  : JS::shadow::Runtime(
#ifdef JSGC_GENERATIONAL
        &gc.storeBuffer
#endif
    ),
    mainThread(this),
    parentRuntime(parentRuntime),
    interrupt(false),
    interruptPar(false),
    handlingSignal(false),
    interruptCallback(nullptr),
    interruptLock(nullptr),
    interruptLockOwner(nullptr),
    exclusiveAccessLock(nullptr),
    exclusiveAccessOwner(nullptr),
    mainThreadHasExclusiveAccess(false),
    numExclusiveThreads(0),
    numCompartments(0),
    localeCallbacks(nullptr),
    defaultLocale(nullptr),
    defaultVersion_(JSVERSION_DEFAULT),
    ownerThread_(nullptr),
    tempLifoAlloc(TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    freeLifoAlloc(TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    execAlloc_(nullptr),
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
    requestDepth(0),
#ifdef DEBUG
    checkRequestDepth(0),
    activeContext(nullptr),
#endif
    gc(thisFromCtor()),
    gcInitialized(false),
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    simulatorRuntime_(nullptr),
#endif
    scriptAndCountsVector(nullptr),
    NaNValue(DoubleNaNValue()),
    negativeInfinityValue(DoubleValue(NegativeInfinity<double>())),
    positiveInfinityValue(DoubleValue(PositiveInfinity<double>())),
    emptyString(nullptr),
#ifdef NIGHTLY_BUILD
    assertOnScriptEntryHook_(nullptr),
#endif
    debugMode(false),
    spsProfiler(thisFromCtor()),
    profilingScripts(false),
    suppressProfilerSampling(false),
    hadOutOfMemory(false),
    haveCreatedContext(false),
    data(nullptr),
    signalHandlersInstalled_(false),
    canUseSignalHandlers_(false),
    defaultFreeOp_(thisFromCtor()),
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
    wellKnownSymbols(nullptr),
    wrapObjectCallbacks(&DefaultWrapObjectCallbacks),
    preserveWrapperCallback(nullptr),
    jitSupportsFloatingPoint(false),
    jitSupportsSimd(false),
    ionPcScriptCache(nullptr),
    threadPool(this),
    defaultJSContextCallback(nullptr),
    ctypesActivityCallback(nullptr),
    forkJoinWarmup(0),
    offthreadIonCompilationEnabled_(true),
    parallelParsingEnabled_(true),
#ifdef DEBUG
    enteredPolicy(nullptr),
#endif
    largeAllocationFailureCallback(nullptr),
    oomCallback(nullptr)
{
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

    interruptLock = PR_NewLock();
    if (!interruptLock)
        return false;

    exclusiveAccessLock = PR_NewLock();
    if (!exclusiveAccessLock)
        return false;

    if (!mainThread.init())
        return false;

    js::TlsPerThreadData.set(&mainThread);

    if (!threadPool.init())
        return false;

    if (!gc.init(maxbytes, maxNurseryBytes))
        return false;

    const char *size = getenv("JSGC_MARK_STACK_LIMIT");
    if (size)
        SetMarkStackLimit(this, atoi(size));

    ScopedJSDeletePtr<Zone> atomsZone(new_<Zone>(this));
    if (!atomsZone || !atomsZone->init(true))
        return false;

    JS::CompartmentOptions options;
    ScopedJSDeletePtr<JSCompartment> atomsCompartment(new_<JSCompartment>(atomsZone.get(), options));
    if (!atomsCompartment || !atomsCompartment->init(nullptr))
        return false;

    gc.zones.append(atomsZone.get());
    atomsZone->compartments.append(atomsCompartment.get());

    atomsCompartment->isSystem = true;

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

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    simulatorRuntime_ = js::jit::CreateSimulatorRuntime();
    if (!simulatorRuntime_)
        return false;
#endif

    nativeStackBase = GetNativeStackBase();

    jitSupportsFloatingPoint = js::jit::JitSupportsFloatingPoint();
    jitSupportsSimd = js::jit::JitSupportsSimd();

    signalHandlersInstalled_ = EnsureAsmJSSignalHandlersInstalled(this);
    canUseSignalHandlers_ = signalHandlersInstalled_ && !SignalBasedTriggersDisabled();

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
            if (WatchpointMap *wpmap = comp->watchpointMap)
                wpmap->clear();
        }

        
        finishAtoms();

        



        beingDestroyed_ = true;

        
        profilingScripts = false;

        JS::PrepareForFullGC(this);
        gc.gc(GC_NORMAL, JS::gcreason::DESTROY_RUNTIME);
    }

    



    finishSelfHosting();

    JS_ASSERT(!exclusiveAccessOwner);
    if (exclusiveAccessLock)
        PR_DestroyLock(exclusiveAccessLock);

    
    JS_ASSERT(!numExclusiveThreads);
    mainThreadHasExclusiveAccess = true;

    JS_ASSERT(!interruptLockOwner);
    if (interruptLock)
        PR_DestroyLock(interruptLock);

    



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

    gc.finish();
    atomsCompartment_ = nullptr;

    js_free(defaultLocale);
    js_delete(mathCache_);
    js_delete(jitRuntime_);
    js_delete(execAlloc_);  

    js_delete(ionPcScriptCache);

#ifdef JSGC_GENERATIONAL
    gc.storeBuffer.disable();
    gc.nursery.disable();
#endif

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    js::jit::DestroySimulatorRuntime(simulatorRuntime_);
#endif

    DebugOnly<size_t> oldCount = liveRuntimesCount--;
    JS_ASSERT(oldCount > 0);

    js::TlsPerThreadData.set(nullptr);
}

void
NewObjectCache::clearNurseryObjects(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    for (unsigned i = 0; i < mozilla::ArrayLength(entries); ++i) {
        Entry &e = entries[i];
        JSObject *obj = reinterpret_cast<JSObject *>(&e.templateObject);
        if (IsInsideNursery(e.key) ||
            rt->gc.nursery.isInside(obj->slots) ||
            rt->gc.nursery.isInside(obj->elements))
        {
            PodZero(&e);
        }
    }
#endif
}

void
JSRuntime::resetJitStackLimit()
{
    AutoLockForInterrupt lock(this);
    mainThread.setJitStackLimit(mainThread.nativeStackLimit[js::StackForUntrustedScript]);

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
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

    rtSizes->interpreterStack += interpreterStack_.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->mathCache += mathCache_ ? mathCache_->sizeOfIncludingThis(mallocSizeOf) : 0;

    rtSizes->uncompressedSourceCache += uncompressedSourceCache.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->compressedSourceSet += compressedSourceSet.sizeOfExcludingThis(mallocSizeOf);

    rtSizes->scriptData += scriptDataTable().sizeOfExcludingThis(mallocSizeOf);
    for (ScriptDataTable::Range r = scriptDataTable().all(); !r.empty(); r.popFront())
        rtSizes->scriptData += mallocSizeOf(r.front());

    if (execAlloc_)
        execAlloc_->addSizeOfCode(&rtSizes->code);
    {
        AutoLockForInterrupt lock(this);
        if (jitRuntime()) {
            if (jit::ExecutableAllocator *ionAlloc = jitRuntime()->ionAlloc(this))
                ionAlloc->addSizeOfCode(&rtSizes->code);
        }
    }

    rtSizes->gc.marker += gc.marker.sizeOfExcludingThis(mallocSizeOf);
#ifdef JSGC_GENERATIONAL
    rtSizes->gc.nurseryCommitted += gc.nursery.sizeOfHeapCommitted();
    rtSizes->gc.nurseryDecommitted += gc.nursery.sizeOfHeapDecommitted();
    rtSizes->gc.nurseryHugeSlots += gc.nursery.sizeOfHugeSlots(mallocSizeOf);
    gc.storeBuffer.addSizeOfExcludingThis(mallocSizeOf, &rtSizes->gc);
#endif
}

void
JSRuntime::requestInterrupt(InterruptMode mode)
{
    AutoLockForInterrupt lock(this);

    





    mainThread.setJitStackLimit(-1);

    interrupt = true;

    RequestInterruptForForkJoin(this, mode);

    



    if (canUseSignalHandlers()) {
        RequestInterruptForAsmJSCode(this, mode);
        jit::RequestInterruptForIonCode(this, mode);
    }
}

jit::ExecutableAllocator *
JSRuntime::createExecutableAllocator(JSContext *cx)
{
    JS_ASSERT(!execAlloc_);
    JS_ASSERT(cx->runtime() == this);

    execAlloc_ = js_new<jit::ExecutableAllocator>();
    if (!execAlloc_)
        js_ReportOutOfMemory(cx);
    return execAlloc_;
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
JSRuntime::updateMallocCounter(size_t nbytes)
{
    updateMallocCounter(nullptr, nbytes);
}

void
JSRuntime::updateMallocCounter(JS::Zone *zone, size_t nbytes)
{
    gc.updateMallocCounter(zone, nbytes);
}

JS_FRIEND_API(void)
JSRuntime::onTooMuchMalloc()
{
    gc.onTooMuchMalloc();
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
    gc.waitBackgroundSweepOrAllocEnd();
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

void *
JSRuntime::onOutOfMemoryCanGC(void *p, size_t bytes)
{
    if (!largeAllocationFailureCallback || bytes < LARGE_ALLOCATION)
        return nullptr;
    largeAllocationFailureCallback(largeAllocationFailureCallbackData);
    return onOutOfMemory(p, bytes);
}

bool
JSRuntime::activeGCInAtomsZone()
{
    Zone *zone = atomsCompartment_->zone();
    return zone->needsIncrementalBarrier() || zone->isGCScheduled() || zone->wasGCStarted();
}

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

#ifdef DEBUG

void
JSRuntime::assertCanLock(RuntimeLock which)
{
    
    
    
    switch (which) {
      case ExclusiveAccessLock:
        JS_ASSERT(exclusiveAccessOwner != PR_GetCurrentThread());
      case HelperThreadStateLock:
        JS_ASSERT(!HelperThreadState().isLocked());
      case InterruptLock:
        JS_ASSERT(!currentThreadOwnsInterruptLock());
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
    PerThreadData *pt = TlsPerThreadData.get();
    if (pt && pt->runtime_)
        pt->runtime_->assertCanLock(which);
}

#endif 
