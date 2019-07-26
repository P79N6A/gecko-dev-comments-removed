





#include "jit/ParallelFunctions.h"

#include "jit/IonSpewer.h"
#include "vm/ArrayObject.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace jit;

using parallel::Spew;
using parallel::SpewOps;
using parallel::SpewBailouts;
using parallel::SpewBailoutIR;


ForkJoinSlice *
jit::ForkJoinSlicePar()
{
    return ForkJoinSlice::Current();
}




JSObject *
jit::NewGCThingPar(gc::AllocKind allocKind)
{
    ForkJoinSlice *slice = ForkJoinSlice::Current();
    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);
    return gc::NewGCThing<JSObject, NoGC>(slice, allocKind, thingSize, gc::DefaultHeap);
}



bool
jit::IsThreadLocalObject(ForkJoinSlice *slice, JSObject *object)
{
    JS_ASSERT(ForkJoinSlice::Current() == slice);
    return !IsInsideNursery(slice->runtime(), object) &&
           slice->allocator()->arenas.containsArena(slice->runtime(), object->arenaHeader());
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
jit::TraceLIR(uint32_t bblock, uint32_t lir, uint32_t execModeInt,
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
jit::CheckOverRecursedPar(ForkJoinSlice *slice)
{
    JS_ASSERT(ForkJoinSlice::Current() == slice);
    int stackDummy_;

    
    
    
    
    
    
    
    

    uintptr_t realStackLimit;
    if (slice->isMainThread())
        realStackLimit = GetNativeStackLimit(slice);
    else
        realStackLimit = slice->perThreadData->ionStackLimit;

    if (!JS_CHECK_STACK_SIZE(realStackLimit, &stackDummy_)) {
        slice->bailoutRecord->setCause(ParallelBailoutOverRecursed,
                                       NULL, NULL, NULL);
        return false;
    }

    return CheckInterruptPar(slice);
}

bool
jit::CheckInterruptPar(ForkJoinSlice *slice)
{
    JS_ASSERT(ForkJoinSlice::Current() == slice);
    bool result = slice->check();
    if (!result) {
        
        
        
        
        
        return false;
    }
    return true;
}

JSObject *
jit::PushPar(PushParArgs *args)
{
    
    
    
    ForkJoinSlice *slice = js::ForkJoinSlice::Current();
    JSObject::EnsureDenseResult res =
        args->object->parExtendDenseElements(slice, &args->value, 1);
    if (res != JSObject::ED_OK)
        return NULL;
    return args->object;
}

JSObject *
jit::ExtendArrayPar(ForkJoinSlice *slice, JSObject *array, uint32_t length)
{
    JSObject::EnsureDenseResult res =
        array->parExtendDenseElements(slice, NULL, length);
    if (res != JSObject::ED_OK)
        return NULL;
    return array;
}

ParallelResult
jit::ConcatStringsPar(ForkJoinSlice *slice, HandleString left, HandleString right,
                      MutableHandleString out)
{
    JSString *str = ConcatStrings<NoGC>(slice, left, right);
    if (!str)
        return TP_RETRY_SEQUENTIALLY;
    out.set(str);
    return TP_SUCCESS;
}

ParallelResult
jit::IntToStringPar(ForkJoinSlice *slice, int i, MutableHandleString out)
{
    JSFlatString *str = Int32ToString<NoGC>(slice, i);
    if (!str)
        return TP_RETRY_SEQUENTIALLY;
    out.set(str);
    return TP_SUCCESS;
}

ParallelResult
jit::DoubleToStringPar(ForkJoinSlice *slice, double d, MutableHandleString out)
{
    JSString *str = NumberToString<NoGC>(slice, d);
    if (!str)
        return TP_RETRY_SEQUENTIALLY;
    out.set(str);
    return TP_SUCCESS;
}

ParallelResult
jit::StringToNumberPar(ForkJoinSlice *slice, JSString *str, double *out)
{
    return StringToNumber(slice, str, out) ? TP_SUCCESS : TP_FATAL;
}

#define PAR_RELATIONAL_OP(OP, EXPECTED)                                         \
do {                                                                            \
    /* Optimize for two int-tagged operands (typical loop control). */          \
    if (lhs.isInt32() && rhs.isInt32()) {                                       \
        *res = (lhs.toInt32() OP rhs.toInt32()) == EXPECTED;                    \
    } else if (lhs.isNumber() && rhs.isNumber()) {                              \
        double l = lhs.toNumber(), r = rhs.toNumber();                          \
        *res = (l OP r) == EXPECTED;                                            \
    } else if (lhs.isBoolean() && rhs.isBoolean()) {                            \
        bool l = lhs.toBoolean();                                               \
        bool r = rhs.toBoolean();                                               \
        *res = (l OP r) == EXPECTED;                                            \
    } else if (lhs.isBoolean() && rhs.isNumber()) {                             \
        bool l = lhs.toBoolean();                                               \
        double r = rhs.toNumber();                                              \
        *res = (l OP r) == EXPECTED;                                            \
    } else if (lhs.isNumber() && rhs.isBoolean()) {                             \
        double l = lhs.toNumber();                                              \
        bool r = rhs.toBoolean();                                               \
        *res = (l OP r) == EXPECTED;                                            \
    } else {                                                                    \
        int32_t vsZero;                                                         \
        ParallelResult ret = CompareMaybeStringsPar(slice, lhs, rhs, &vsZero);  \
        if (ret != TP_SUCCESS)                                                  \
            return ret;                                                         \
        *res = (vsZero OP 0) == EXPECTED;                                       \
    }                                                                           \
    return TP_SUCCESS;                                                          \
} while(0)

static ParallelResult
CompareStringsPar(ForkJoinSlice *slice, JSString *left, JSString *right, int32_t *res)
{
    ScopedThreadSafeStringInspector leftInspector(left);
    ScopedThreadSafeStringInspector rightInspector(right);
    if (!leftInspector.ensureChars(slice) || !rightInspector.ensureChars(slice))
        return TP_FATAL;

    if (!CompareChars(leftInspector.chars(), left->length(),
                      rightInspector.chars(), right->length(),
                      res))
        return TP_FATAL;

    return TP_SUCCESS;
}

static ParallelResult
CompareMaybeStringsPar(ForkJoinSlice *slice, HandleValue v1, HandleValue v2, int32_t *res)
{
    if (!v1.isString())
        return TP_RETRY_SEQUENTIALLY;
    if (!v2.isString())
        return TP_RETRY_SEQUENTIALLY;
    return CompareStringsPar(slice, v1.toString(), v2.toString(), res);
}

template<bool Equal>
ParallelResult
LooselyEqualImplPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(==, Equal);
}

ParallelResult
js::jit::LooselyEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return LooselyEqualImplPar<true>(slice, lhs, rhs, res);
}

ParallelResult
js::jit::LooselyUnequalPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return LooselyEqualImplPar<false>(slice, lhs, rhs, res);
}

template<bool Equal>
ParallelResult
StrictlyEqualImplPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    if (lhs.isNumber()) {
        if (rhs.isNumber()) {
            *res = (lhs.toNumber() == rhs.toNumber()) == Equal;
            return TP_SUCCESS;
        }
    } else if (lhs.isBoolean()) {
        if (rhs.isBoolean()) {
            *res = (lhs.toBoolean() == rhs.toBoolean()) == Equal;
            return TP_SUCCESS;
        }
    } else if (lhs.isNull()) {
        if (rhs.isNull()) {
            *res = Equal;
            return TP_SUCCESS;
        }
    } else if (lhs.isUndefined()) {
        if (rhs.isUndefined()) {
            *res = Equal;
            return TP_SUCCESS;
        }
    } else if (lhs.isObject()) {
        if (rhs.isObject()) {
            *res = (lhs.toObjectOrNull() == rhs.toObjectOrNull()) == Equal;
            return TP_SUCCESS;
        }
    } else if (lhs.isString()) {
        if (rhs.isString())
            return LooselyEqualImplPar<Equal>(slice, lhs, rhs, res);
    }

    *res = false;
    return TP_SUCCESS;
}

ParallelResult
js::jit::StrictlyEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return StrictlyEqualImplPar<true>(slice, lhs, rhs, res);
}

ParallelResult
js::jit::StrictlyUnequalPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return StrictlyEqualImplPar<false>(slice, lhs, rhs, res);
}

ParallelResult
js::jit::LessThanPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(<, true);
}

ParallelResult
js::jit::LessThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(<=, true);
}

ParallelResult
js::jit::GreaterThanPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(>, true);
}

ParallelResult
js::jit::GreaterThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(>=, true);
}

template<bool Equal>
ParallelResult
StringsEqualImplPar(ForkJoinSlice *slice, HandleString lhs, HandleString rhs, bool *res)
{
    int32_t vsZero;
    ParallelResult ret = CompareStringsPar(slice, lhs, rhs, &vsZero);
    if (ret != TP_SUCCESS)
        return ret;
    *res = (vsZero == 0) == Equal;
    return TP_SUCCESS;
}

ParallelResult
js::jit::StringsEqualPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *res)
{
    return StringsEqualImplPar<true>(slice, v1, v2, res);
}

ParallelResult
js::jit::StringsUnequalPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *res)
{
    return StringsEqualImplPar<false>(slice, v1, v2, res);
}

ParallelResult
jit::BitNotPar(ForkJoinSlice *slice, HandleValue in, int32_t *out)
{
    if (in.isObject())
        return TP_RETRY_SEQUENTIALLY;
    int i;
    if (!NonObjectToInt32(slice, in, &i))
        return TP_FATAL;
    *out = ~i;
    return TP_SUCCESS;
}

#define BIT_OP(OP)                                                      \
    JS_BEGIN_MACRO                                                      \
    int32_t left, right;                                                \
    if (lhs.isObject() || rhs.isObject())                               \
        return TP_RETRY_SEQUENTIALLY;                                   \
    if (!NonObjectToInt32(slice, lhs, &left) ||                         \
        !NonObjectToInt32(slice, rhs, &right))                          \
    {                                                                   \
        return TP_FATAL;                                                \
    }                                                                   \
    *out = (OP);                                                        \
    return TP_SUCCESS;                                                  \
    JS_END_MACRO

ParallelResult
jit::BitXorPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left ^ right);
}

ParallelResult
jit::BitOrPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left | right);
}

ParallelResult
jit::BitAndPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left & right);
}

ParallelResult
jit::BitLshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left << (right & 31));
}

ParallelResult
jit::BitRshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left >> (right & 31));
}

#undef BIT_OP

ParallelResult
jit::UrshValuesPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs,
                   Value *out)
{
    uint32_t left;
    int32_t right;
    if (lhs.isObject() || rhs.isObject())
        return TP_RETRY_SEQUENTIALLY;
    if (!NonObjectToUint32(slice, lhs, &left) || !NonObjectToInt32(slice, rhs, &right))
        return TP_FATAL;
    left >>= right & 31;
    out->setNumber(uint32_t(left));
    return TP_SUCCESS;
}

void
jit::AbortPar(ParallelBailoutCause cause, JSScript *outermostScript, JSScript *currentScript,
              jsbytecode *bytecode)
{
    
    Spew(SpewBailouts,
         "Parallel abort with cause %d in %p:%s:%d "
         "(%p:%s:%d at line %d)",
         cause,
         outermostScript, outermostScript->filename(), outermostScript->lineno,
         currentScript, currentScript->filename(), currentScript->lineno,
         (currentScript ? PCToLineNumber(currentScript, bytecode) : 0));

    JS_ASSERT(InParallelSection());
    JS_ASSERT(outermostScript != NULL);
    JS_ASSERT(currentScript != NULL);
    JS_ASSERT(outermostScript->hasParallelIonScript());

    ForkJoinSlice *slice = ForkJoinSlice::Current();

    JS_ASSERT(slice->bailoutRecord->depth == 0);
    slice->bailoutRecord->setCause(cause, outermostScript,
                                   currentScript, bytecode);
}

void
jit::PropagateAbortPar(JSScript *outermostScript, JSScript *currentScript)
{
    Spew(SpewBailouts,
         "Propagate parallel abort via %p:%s:%d (%p:%s:%d)",
         outermostScript, outermostScript->filename(), outermostScript->lineno,
         currentScript, currentScript->filename(), currentScript->lineno);

    JS_ASSERT(InParallelSection());
    JS_ASSERT(outermostScript->hasParallelIonScript());

    outermostScript->parallelIonScript()->setHasUncompiledCallTarget();

    ForkJoinSlice *slice = ForkJoinSlice::Current();
    if (currentScript)
        slice->bailoutRecord->addTrace(currentScript, NULL);
}

void
jit::CallToUncompiledScriptPar(JSObject *obj)
{
    JS_ASSERT(InParallelSection());

#ifdef DEBUG
    static const int max_bound_function_unrolling = 5;

    if (!obj->is<JSFunction>()) {
        Spew(SpewBailouts, "Call to non-function");
        return;
    }

    JSFunction *func = &obj->as<JSFunction>();
    if (func->hasScript()) {
        JSScript *script = func->nonLazyScript();
        Spew(SpewBailouts, "Call to uncompiled script: %p:%s:%d",
             script, script->filename(), script->lineno);
    } else if (func->isInterpretedLazy()) {
        Spew(SpewBailouts, "Call to uncompiled lazy script");
    } else if (func->isBoundFunction()) {
        int depth = 0;
        JSFunction *target = &func->getBoundFunctionTarget()->as<JSFunction>();
        while (depth < max_bound_function_unrolling) {
            if (target->hasScript())
                break;
            if (target->isBoundFunction())
                target = &target->getBoundFunctionTarget()->as<JSFunction>();
            depth--;
        }
        if (target->hasScript()) {
            JSScript *script = target->nonLazyScript();
            Spew(SpewBailouts, "Call to bound function leading (depth: %d) to script: %p:%s:%d",
                 depth, script, script->filename(), script->lineno);
        } else {
            Spew(SpewBailouts, "Call to bound function (excessive depth: %d)", depth);
        }
    } else {
        JS_ASSERT(func->isNative());
        Spew(SpewBailouts, "Call to native function");
    }
#endif
}

ParallelResult
jit::InitRestParameterPar(ForkJoinSlice *slice, uint32_t length, Value *rest,
                          HandleObject templateObj, HandleObject res,
                          MutableHandleObject out)
{
    
    
    
    JS_ASSERT(res);
    JS_ASSERT(res->is<ArrayObject>());
    JS_ASSERT(!res->getDenseInitializedLength());
    JS_ASSERT(res->type() == templateObj->type());

    if (length) {
        JSObject::EnsureDenseResult edr = res->parExtendDenseElements(slice, rest, length);
        if (edr != JSObject::ED_OK)
            return TP_FATAL;
    }

    out.set(res);
    return TP_SUCCESS;
}
