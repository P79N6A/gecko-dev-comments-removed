





#ifndef jit_CompileWrappers_h
#define jit_CompileWrappers_h

#ifdef JS_ION

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

    
    const void *addressOfIonTop();

    
    const void *addressOfIonStackLimit();

    
    const void *addressOfJSContext();

    
    const void *addressOfActivation();

    
    const void *addressOfLastCachedNativeIterator();

#ifdef JS_GC_ZEAL
    const void *addressOfGCZeal();
#endif

    const void *addressOfInterrupt();

#ifdef JS_THREADSAFE
    const void *addressOfInterruptPar();
#endif

    const JitRuntime *jitRuntime();

    
    SPSProfiler &spsProfiler();

    bool signalHandlersInstalled();
    bool jitSupportsFloatingPoint();
    bool hadOutOfMemory();

    const JSAtomState &names();
    const StaticStrings &staticStrings();
    const Value &NaNValue();
    const Value &positiveInfinityValue();

#ifdef DEBUG
    bool isInsideNursery(gc::Cell *cell);
#endif

    
    const DOMCallbacks *DOMcallbacks();

    const MathCache *maybeGetMathCache();

#ifdef JSGC_GENERATIONAL
    const Nursery &gcNursery();
#endif
};

class CompileZone
{
    Zone *zone();

  public:
    static CompileZone *get(Zone *zone);

    const void *addressOfNeedsBarrier();

    
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

    const CallsiteCloneTable &callsiteClones();

    const JitCompartment *jitCompartment();

    bool hasObjectMetadataCallback();

    
    void setSingletonsAsValues();
};

class JitCompileOptions
{
  public:
    JitCompileOptions();
    JitCompileOptions(JSContext *cx);

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

#endif 
