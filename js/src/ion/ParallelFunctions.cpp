






#include "jsinterp.h"
#include "ParallelFunctions.h"
#include "IonSpewer.h"

#include "jsinterpinlines.h"
#include "jscompartmentinlines.h"

#include "vm/ParallelDo.h"

using namespace js;
using namespace ion;

using parallel::Spew;
using parallel::SpewBailouts;
using parallel::SpewBailoutIR;


ForkJoinSlice *
ion::ParForkJoinSlice()
{
    return ForkJoinSlice::Current();
}




JSObject *
ion::ParNewGCThing(gc::AllocKind allocKind)
{
    ForkJoinSlice *slice = ForkJoinSlice::Current();
    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);
    void *t = slice->allocator->parallelNewGCThing(allocKind, thingSize);
    return static_cast<JSObject *>(t);
}



bool
ion::ParWriteGuard(ForkJoinSlice *slice, JSObject *object)
{
    JS_ASSERT(ForkJoinSlice::Current() == slice);
    return slice->allocator->arenas.containsArena(slice->runtime(),
                                                  object->arenaHeader());
}

#ifdef DEBUG
static void
printTrace(const char *prefix, struct IonLIRTraceData *cached)
{
    fprintf(stderr, "%s / Block %3u / LIR %3u / Mode %u / LIR %s\n",
            prefix,
            cached->bblock, cached->lir, cached->execModeInt, cached->lirOpName);
}

struct IonLIRTraceData seqTraceData;
#endif

void
ion::TraceLIR(uint32_t bblock, uint32_t lir, uint32_t execModeInt,
              const char *lirOpName, const char *mirOpName,
              JSScript *script, jsbytecode *pc)
{
#ifdef DEBUG
    static enum { NotSet, All, Bailouts } traceMode;

    
    
    
    
    
    
    
    
    

    if (traceMode == NotSet) {
        
        const char *env = getenv("IONFLAGS");
        if (strstr(env, "trace-all"))
            traceMode = All;
        else
            traceMode = Bailouts;
    }

    IonLIRTraceData *cached;
    if (execModeInt == 0)
        cached = &seqTraceData;
    else
        cached = &ForkJoinSlice::Current()->traceData;

    if (bblock == 0xDEADBEEF) {
        if (execModeInt == 0)
            printTrace("BAILOUT", cached);
        else
            SpewBailoutIR(cached->bblock, cached->lir,
                          cached->lirOpName, cached->mirOpName,
                          cached->script, cached->pc);
    }

    cached->bblock = bblock;
    cached->lir = lir;
    cached->execModeInt = execModeInt;
    cached->lirOpName = lirOpName;
    cached->mirOpName = mirOpName;
    cached->script = script;
    cached->pc = pc;

    if (traceMode == All)
        printTrace("Exec", cached);
#endif
}

bool
ion::ParCheckOverRecursed(ForkJoinSlice *slice)
{
    JS_ASSERT(ForkJoinSlice::Current() == slice);

    
    
    
    
    if (slice->isMainThread()) {
        int stackDummy_;
        if (!JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(slice->runtime()), &stackDummy_))
            return false;
        return ParCheckInterrupt(slice);
    } else {
        
        
        
        
        
        return false;
    }
}

bool
ion::ParCheckInterrupt(ForkJoinSlice *slice)
{
    JS_ASSERT(ForkJoinSlice::Current() == slice);
    bool result = slice->check();
    if (!result)
        return false;
    return true;
}

void
ion::ParDumpValue(Value *v)
{
#ifdef DEBUG
    js_DumpValue(*v);
#endif
}

JSObject*
ion::ParPush(ParPushArgs *args)
{
    
    
    
    ForkJoinSlice *slice = js::ForkJoinSlice::Current();
    JSObject::EnsureDenseResult res =
        args->object->parExtendDenseElements(slice->allocator,
                                             &args->value, 1);
    if (res != JSObject::ED_OK)
        return NULL;
    return args->object;
}

JSObject *
ion::ParExtendArray(ForkJoinSlice *slice, JSObject *array, uint32_t length)
{
    JSObject::EnsureDenseResult res =
        array->parExtendDenseElements(slice->allocator, NULL, length);
    if (res != JSObject::ED_OK)
        return NULL;
    return array;
}

ParCompareResult
ion::ParCompareStrings(JSString *str1, JSString *str2)
{
    
    if (!str1->isLinear())
        return ParCompareUnknown;
    if (!str2->isLinear())
        return ParCompareUnknown;

    JSLinearString &linearStr1 = str1->asLinear();
    JSLinearString &linearStr2 = str2->asLinear();
    if (EqualStrings(&linearStr1, &linearStr2))
        return ParCompareEq;
    return ParCompareNe;
}

void
ion::ParallelAbort(JSScript *script)
{
    JS_ASSERT(InParallelSection());

    ForkJoinSlice *slice = ForkJoinSlice::Current();

    Spew(SpewBailouts, "Parallel abort in %p:%s:%d (hasParallelIonScript:%d)",
         script, script->filename(), script->lineno,
         script->hasParallelIonScript());

    
    JS_ASSERT(script->hasParallelIonScript());

    if (!slice->abortedScript)
        slice->abortedScript = script;
}

void
ion::ParCallToUncompiledScript(JSFunction *func)
{
    JS_ASSERT(InParallelSection());

#ifdef DEBUG
    RawScript script = func->nonLazyScript();
    Spew(SpewBailouts, "Call to uncompiled script: %p:%s:%d", script, script->filename(), script->lineno);
#endif
}
