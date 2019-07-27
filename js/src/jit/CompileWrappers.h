





#ifndef jit_CompileWrappers_h
#define jit_CompileWrappers_h

#include "jscntxt.h"

namespace js {
namespace jit {

class JitRuntime;







class CompileRuntime
{
    JSRuntime *runtime();

  public:
    static CompileRuntime *get(JSRuntime *rt);

    bool onMainThread();

    js::PerThreadData *mainThread();

    
    const void *addressOfJitTop();

    
    const void *addressOfJitActivation();

    
    const void *addressOfProfilingActivation();

    
    const void *addressOfJitStackLimit();

    
    const void *addressOfJSContext();

    
    const void *addressOfActivation();

    
    const void *addressOfLastCachedNativeIterator();

#ifdef JS_GC_ZEAL
    const void *addressOfGCZeal();
#endif

    const void *addressOfInterruptUint32();

    const JitRuntime *jitRuntime();

    
    SPSProfiler &spsProfiler();

    bool canUseSignalHandlers();
    bool jitSupportsFloatingPoint();
    bool hadOutOfMemory();
    bool profilingScripts();

    const JSAtomState &names();
    const PropertyName *emptyString();
    const StaticStrings &staticStrings();
    const Value &NaNValue();
    const Value &positiveInfinityValue();
    const WellKnownSymbols &wellKnownSymbols();

#ifdef DEBUG
    bool isInsideNursery(gc::Cell *cell);
#endif

    
    const DOMCallbacks *DOMcallbacks();

    const MathCache *maybeGetMathCache();

    const Nursery &gcNursery();
};

class CompileZone
{
    Zone *zone();

  public:
    static CompileZone *get(Zone *zone);

    const void *addressOfNeedsIncrementalBarrier();

    
    const void *addressOfFreeListFirst(gc::AllocKind allocKind);
    const void *addressOfFreeListLast(gc::AllocKind allocKind);
};

class CompileCompartment
{
    JSCompartment *compartment();

  public:
    static CompileCompartment *get(JSCompartment *comp);

    CompileZone *zone();
    CompileRuntime *runtime();

    const void *addressOfEnumerators();

    const JitCompartment *jitCompartment();

    bool hasObjectMetadataCallback();

    
    void setSingletonsAsValues();
};

class JitCompileOptions
{
  public:
    JitCompileOptions();
    explicit JitCompileOptions(JSContext *cx);

    bool cloneSingletons() const {
        return cloneSingletons_;
    }

    bool spsSlowAssertionsEnabled() const {
        return spsSlowAssertionsEnabled_;
    }

  private:
    bool cloneSingletons_;
    bool spsSlowAssertionsEnabled_;
};

} 
} 

#endif 
