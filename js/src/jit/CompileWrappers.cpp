





#include "jit/Ion.h"

using namespace js;
using namespace js::jit;

JSRuntime *
CompileRuntime::runtime()
{
    return reinterpret_cast<JSRuntime *>(this);
}

 CompileRuntime *
CompileRuntime::get(JSRuntime *rt)
{
    return reinterpret_cast<CompileRuntime *>(rt);
}

bool
CompileRuntime::onMainThread()
{
    return js::CurrentThreadCanAccessRuntime(runtime());
}

js::PerThreadData *
CompileRuntime::mainThread()
{
    JS_ASSERT(onMainThread());
    return &runtime()->mainThread;
}

const void *
CompileRuntime::addressOfJitTop()
{
    return &runtime()->mainThread.jitTop;
}

const void *
CompileRuntime::addressOfJitStackLimit()
{
    return &runtime()->mainThread.jitStackLimit;
}

const void *
CompileRuntime::addressOfJSContext()
{
    return &runtime()->mainThread.jitJSContext;
}

const void *
CompileRuntime::addressOfActivation()
{
    return runtime()->mainThread.addressOfActivation();
}

const void *
CompileRuntime::addressOfLastCachedNativeIterator()
{
    return &runtime()->nativeIterCache.last;
}

#ifdef JS_GC_ZEAL
const void *
CompileRuntime::addressOfGCZeal()
{
    return runtime()->gc.addressOfZealMode();
}
#endif

const void *
CompileRuntime::addressOfInterrupt()
{
    return &runtime()->interrupt;
}

const void *
CompileRuntime::addressOfInterruptPar()
{
    return &runtime()->interruptPar;
}

const void *
CompileRuntime::addressOfThreadPool()
{
    return &runtime()->threadPool;
}

const JitRuntime *
CompileRuntime::jitRuntime()
{
    return runtime()->jitRuntime();
}

SPSProfiler &
CompileRuntime::spsProfiler()
{
    return runtime()->spsProfiler;
}

bool
CompileRuntime::canUseSignalHandlers()
{
    return runtime()->canUseSignalHandlers();
}

bool
CompileRuntime::jitSupportsFloatingPoint()
{
    return runtime()->jitSupportsFloatingPoint;
}

bool
CompileRuntime::hadOutOfMemory()
{
    return runtime()->hadOutOfMemory;
}

bool
CompileRuntime::profilingScripts()
{
    return runtime()->profilingScripts;
}

const JSAtomState &
CompileRuntime::names()
{
    return *runtime()->commonNames;
}

const PropertyName *
CompileRuntime::emptyString()
{
    return runtime()->emptyString;
}

const StaticStrings &
CompileRuntime::staticStrings()
{
    return *runtime()->staticStrings;
}

const Value &
CompileRuntime::NaNValue()
{
    return runtime()->NaNValue;
}

const Value &
CompileRuntime::positiveInfinityValue()
{
    return runtime()->positiveInfinityValue;
}

#ifdef DEBUG
bool
CompileRuntime::isInsideNursery(gc::Cell *cell)
{
    return UninlinedIsInsideNursery(cell);
}
#endif

const DOMCallbacks *
CompileRuntime::DOMcallbacks()
{
    return GetDOMCallbacks(runtime());
}

const MathCache *
CompileRuntime::maybeGetMathCache()
{
    return runtime()->maybeGetMathCache();
}

#ifdef JSGC_GENERATIONAL
const Nursery &
CompileRuntime::gcNursery()
{
    return runtime()->gc.nursery;
}
#endif

Zone *
CompileZone::zone()
{
    return reinterpret_cast<Zone *>(this);
}

 CompileZone *
CompileZone::get(Zone *zone)
{
    return reinterpret_cast<CompileZone *>(zone);
}

const void *
CompileZone::addressOfNeedsIncrementalBarrier()
{
    return zone()->addressOfNeedsIncrementalBarrier();
}

const void *
CompileZone::addressOfFreeListFirst(gc::AllocKind allocKind)
{
    return zone()->allocator.arenas.getFreeList(allocKind)->addressOfFirst();
}

const void *
CompileZone::addressOfFreeListLast(gc::AllocKind allocKind)
{
    return zone()->allocator.arenas.getFreeList(allocKind)->addressOfLast();
}

JSCompartment *
CompileCompartment::compartment()
{
    return reinterpret_cast<JSCompartment *>(this);
}

 CompileCompartment *
CompileCompartment::get(JSCompartment *comp)
{
    return reinterpret_cast<CompileCompartment *>(comp);
}

CompileZone *
CompileCompartment::zone()
{
    return CompileZone::get(compartment()->zone());
}

CompileRuntime *
CompileCompartment::runtime()
{
    return CompileRuntime::get(compartment()->runtimeFromAnyThread());
}

const void *
CompileCompartment::addressOfEnumerators()
{
    return &compartment()->enumerators;
}

const CallsiteCloneTable &
CompileCompartment::callsiteClones()
{
    return compartment()->callsiteClones;
}

const JitCompartment *
CompileCompartment::jitCompartment()
{
    return compartment()->jitCompartment();
}

bool
CompileCompartment::hasObjectMetadataCallback()
{
    return compartment()->hasObjectMetadataCallback();
}








void
CompileCompartment::setSingletonsAsValues()
{
    return JS::CompartmentOptionsRef(compartment()).setSingletonsAsValues();
}

JitCompileOptions::JitCompileOptions()
  : cloneSingletons_(false),
    spsSlowAssertionsEnabled_(false)
{
}

JitCompileOptions::JitCompileOptions(JSContext *cx)
{
    JS::CompartmentOptions &options = cx->compartment()->options();
    cloneSingletons_ = options.cloneSingletons();
    spsSlowAssertionsEnabled_ = cx->runtime()->spsProfiler.enabled() &&
                                cx->runtime()->spsProfiler.slowAssertionsEnabled();
}
