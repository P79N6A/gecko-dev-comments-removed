





#include "jit/ParallelFunctions.h"

#include "builtin/TypedObject.h"
#include "jit/arm/Simulator-arm.h"
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
    return ForkJoinSlice::current();
}




JSObject *
jit::NewGCThingPar(ForkJoinSlice *slice, gc::AllocKind allocKind)
{
    JS_ASSERT(ForkJoinSlice::current() == slice);
    uint32_t thingSize = (uint32_t)gc::Arena::thingSize(allocKind);
    return gc::NewGCThing<JSObject, NoGC>(slice, allocKind, thingSize, gc::DefaultHeap);
}

bool
jit::ParallelWriteGuard(ForkJoinSlice *slice, JSObject *object)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(ForkJoinSlice::current() == slice);

    if (IsTypedDatum(*object)) {
        TypedDatum &datum = AsTypedDatum(*object);

        
        
        
        
        if (IsInTargetRegion(slice, &datum))
            return true;

        
        TypedDatum *owner = datum.owner();
        return owner && slice->isThreadLocal(owner);
    }

    
    return slice->isThreadLocal(object);
}









bool
jit::IsInTargetRegion(ForkJoinSlice *slice, TypedDatum *datum)
{
    JS_ASSERT(IsTypedDatum(*datum)); 
    uint8_t *typedMem = datum->typedMem();
    return (typedMem >= slice->targetRegionStart &&
            typedMem <  slice->targetRegionEnd);
}

#ifdef DEBUG
static void
printTrace(const char *prefix, struct IonLIRTraceData *cached)
{
    fprintf(stderr, "%s / Block %3u / LIR %3u / Mode %u / LIR %s\n",
            prefix,
            cached->blockIndex, cached->lirIndex, cached->execModeInt, cached->lirOpName);
}

static struct IonLIRTraceData seqTraceData;
#endif

void
jit::TraceLIR(IonLIRTraceData *current)
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
    if (current->execModeInt == 0)
        cached = &seqTraceData;
    else
        cached = &ForkJoinSlice::current()->traceData;

    if (current->blockIndex == 0xDEADBEEF) {
        if (current->execModeInt == 0)
            printTrace("BAILOUT", cached);
        else
            SpewBailoutIR(cached);
    }

    memcpy(cached, current, sizeof(IonLIRTraceData));

    if (traceMode == All)
        printTrace("Exec", cached);
#endif
}

bool
jit::CheckOverRecursedPar(ForkJoinSlice *slice)
{
    JS_ASSERT(ForkJoinSlice::current() == slice);
    int stackDummy_;

    
    
    
    
    
    
    
    

#ifdef JS_ARM_SIMULATOR
    if (Simulator::Current()->overRecursed()) {
        slice->bailoutRecord->setCause(ParallelBailoutOverRecursed);
        return false;
    }
#endif

    uintptr_t realStackLimit;
    if (slice->isMainThread())
        realStackLimit = GetNativeStackLimit(slice);
    else
        realStackLimit = slice->perThreadData->ionStackLimit;

    if (!JS_CHECK_STACK_SIZE(realStackLimit, &stackDummy_)) {
        slice->bailoutRecord->setCause(ParallelBailoutOverRecursed);
        return false;
    }

    return CheckInterruptPar(slice);
}

bool
jit::CheckInterruptPar(ForkJoinSlice *slice)
{
    JS_ASSERT(ForkJoinSlice::current() == slice);
    bool result = slice->check();
    if (!result) {
        
        
        
        
        
        return false;
    }
    return true;
}

JSObject *
jit::ExtendArrayPar(ForkJoinSlice *slice, JSObject *array, uint32_t length)
{
    JSObject::EnsureDenseResult res =
        array->ensureDenseElementsPreservePackedFlag(slice, 0, length);
    if (res != JSObject::ED_OK)
        return nullptr;
    return array;
}

bool
jit::SetPropertyPar(ForkJoinSlice *slice, HandleObject obj, HandlePropertyName name,
                    HandleValue value, bool strict, jsbytecode *pc)
{
    JS_ASSERT(slice->isThreadLocal(obj));

    if (*pc == JSOP_SETALIASEDVAR) {
        
        Shape *shape = obj->nativeLookupPure(name);
        JS_ASSERT(shape && shape->hasSlot());
        return obj->nativeSetSlotIfHasType(shape, value);
    }

    
    if (obj->getOps()->setProperty)
        return TP_RETRY_SEQUENTIALLY;

    RootedValue v(slice, value);
    RootedId id(slice, NameToId(name));
    return baseops::SetPropertyHelper<ParallelExecution>(slice, obj, obj, id, 0, &v, strict);
}

bool
jit::SetElementPar(ForkJoinSlice *slice, HandleObject obj, HandleValue index, HandleValue value,
                   bool strict)
{
    RootedId id(slice);
    if (!ValueToIdPure(index, id.address()))
        return false;

    
    
    
    
    
    RootedValue v(slice, value);
    return baseops::SetPropertyHelper<ParallelExecution>(slice, obj, obj, id, 0, &v, strict);
}

JSString *
jit::ConcatStringsPar(ForkJoinSlice *slice, HandleString left, HandleString right)
{
    return ConcatStrings<NoGC>(slice, left, right);
}

JSFlatString *
jit::IntToStringPar(ForkJoinSlice *slice, int i)
{
    return Int32ToString<NoGC>(slice, i);
}

JSString *
jit::DoubleToStringPar(ForkJoinSlice *slice, double d)
{
    return NumberToString<NoGC>(slice, d);
}

bool
jit::StringToNumberPar(ForkJoinSlice *slice, JSString *str, double *out)
{
    return StringToNumber(slice, str, out);
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
        if (!CompareMaybeStringsPar(slice, lhs, rhs, &vsZero))                  \
            return false;                                                       \
        *res = (vsZero OP 0) == EXPECTED;                                       \
    }                                                                           \
    return true;                                                                \
} while(0)

static bool
CompareStringsPar(ForkJoinSlice *slice, JSString *left, JSString *right, int32_t *res)
{
    ScopedThreadSafeStringInspector leftInspector(left);
    ScopedThreadSafeStringInspector rightInspector(right);
    if (!leftInspector.ensureChars(slice) || !rightInspector.ensureChars(slice))
        return false;

    *res = CompareChars(leftInspector.chars(), left->length(),
                        rightInspector.chars(), right->length());
    return true;
}

static bool
CompareMaybeStringsPar(ForkJoinSlice *slice, HandleValue v1, HandleValue v2, int32_t *res)
{
    if (!v1.isString())
        return false;
    if (!v2.isString())
        return false;
    return CompareStringsPar(slice, v1.toString(), v2.toString(), res);
}

template<bool Equal>
bool
LooselyEqualImplPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(==, Equal);
}

bool
js::jit::LooselyEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return LooselyEqualImplPar<true>(slice, lhs, rhs, res);
}

bool
js::jit::LooselyUnequalPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return LooselyEqualImplPar<false>(slice, lhs, rhs, res);
}

template<bool Equal>
bool
StrictlyEqualImplPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    if (lhs.isNumber()) {
        if (rhs.isNumber()) {
            *res = (lhs.toNumber() == rhs.toNumber()) == Equal;
            return true;
        }
    } else if (lhs.isBoolean()) {
        if (rhs.isBoolean()) {
            *res = (lhs.toBoolean() == rhs.toBoolean()) == Equal;
            return true;
        }
    } else if (lhs.isNull()) {
        if (rhs.isNull()) {
            *res = Equal;
            return true;
        }
    } else if (lhs.isUndefined()) {
        if (rhs.isUndefined()) {
            *res = Equal;
            return true;
        }
    } else if (lhs.isObject()) {
        if (rhs.isObject()) {
            *res = (lhs.toObjectOrNull() == rhs.toObjectOrNull()) == Equal;
            return true;
        }
    } else if (lhs.isString()) {
        if (rhs.isString())
            return LooselyEqualImplPar<Equal>(slice, lhs, rhs, res);
    }

    *res = false;
    return true;
}

bool
js::jit::StrictlyEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return StrictlyEqualImplPar<true>(slice, lhs, rhs, res);
}

bool
js::jit::StrictlyUnequalPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return StrictlyEqualImplPar<false>(slice, lhs, rhs, res);
}

bool
js::jit::LessThanPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(<, true);
}

bool
js::jit::LessThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(<=, true);
}

bool
js::jit::GreaterThanPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(>, true);
}

bool
js::jit::GreaterThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(>=, true);
}

template<bool Equal>
bool
StringsEqualImplPar(ForkJoinSlice *slice, HandleString lhs, HandleString rhs, bool *res)
{
    int32_t vsZero;
    bool ret = CompareStringsPar(slice, lhs, rhs, &vsZero);
    if (ret != true)
        return ret;
    *res = (vsZero == 0) == Equal;
    return true;
}

bool
js::jit::StringsEqualPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *res)
{
    return StringsEqualImplPar<true>(slice, v1, v2, res);
}

bool
js::jit::StringsUnequalPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *res)
{
    return StringsEqualImplPar<false>(slice, v1, v2, res);
}

bool
jit::BitNotPar(ForkJoinSlice *slice, HandleValue in, int32_t *out)
{
    if (in.isObject())
        return false;
    int i;
    if (!NonObjectToInt32(slice, in, &i))
        return false;
    *out = ~i;
    return true;
}

#define BIT_OP(OP)                                                      \
    JS_BEGIN_MACRO                                                      \
    int32_t left, right;                                                \
    if (lhs.isObject() || rhs.isObject())                               \
        return TP_RETRY_SEQUENTIALLY;                                   \
    if (!NonObjectToInt32(slice, lhs, &left) ||                         \
        !NonObjectToInt32(slice, rhs, &right))                          \
    {                                                                   \
        return false;                                                   \
    }                                                                   \
    *out = (OP);                                                        \
    return true;                                                        \
    JS_END_MACRO

bool
jit::BitXorPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left ^ right);
}

bool
jit::BitOrPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left | right);
}

bool
jit::BitAndPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left & right);
}

bool
jit::BitLshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left << (right & 31));
}

bool
jit::BitRshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left >> (right & 31));
}

#undef BIT_OP

bool
jit::UrshValuesPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs,
                   Value *out)
{
    uint32_t left;
    int32_t right;
    if (lhs.isObject() || rhs.isObject())
        return false;
    if (!NonObjectToUint32(slice, lhs, &left) || !NonObjectToInt32(slice, rhs, &right))
        return false;
    left >>= right & 31;
    out->setNumber(uint32_t(left));
    return true;
}

void
jit::AbortPar(ParallelBailoutCause cause, JSScript *outermostScript, JSScript *currentScript,
              jsbytecode *bytecode)
{
    
    Spew(SpewBailouts,
         "Parallel abort with cause %d in %p:%s:%d "
         "(%p:%s:%d at line %d)",
         cause,
         outermostScript, outermostScript->filename(), outermostScript->lineno(),
         currentScript, currentScript->filename(), currentScript->lineno(),
         (currentScript ? PCToLineNumber(currentScript, bytecode) : 0));

    JS_ASSERT(InParallelSection());
    JS_ASSERT(outermostScript != nullptr);
    JS_ASSERT(currentScript != nullptr);
    JS_ASSERT(outermostScript->hasParallelIonScript());

    ForkJoinSlice *slice = ForkJoinSlice::current();

    JS_ASSERT(slice->bailoutRecord->depth == 0);
    slice->bailoutRecord->setCause(cause, outermostScript, currentScript, bytecode);
}

void
jit::PropagateAbortPar(JSScript *outermostScript, JSScript *currentScript)
{
    Spew(SpewBailouts,
         "Propagate parallel abort via %p:%s:%d (%p:%s:%d)",
         outermostScript, outermostScript->filename(), outermostScript->lineno(),
         currentScript, currentScript->filename(), currentScript->lineno());

    JS_ASSERT(InParallelSection());
    JS_ASSERT(outermostScript->hasParallelIonScript());

    outermostScript->parallelIonScript()->setHasUncompiledCallTarget();

    ForkJoinSlice *slice = ForkJoinSlice::current();
    if (currentScript)
        slice->bailoutRecord->addTrace(currentScript, nullptr);
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
             script, script->filename(), script->lineno());
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
                 depth, script, script->filename(), script->lineno());
        } else {
            Spew(SpewBailouts, "Call to bound function (excessive depth: %d)", depth);
        }
    } else {
        JS_ASSERT(func->isNative());
        Spew(SpewBailouts, "Call to native function");
    }
#endif
}

JSObject *
jit::InitRestParameterPar(ForkJoinSlice *slice, uint32_t length, Value *rest,
                          HandleObject templateObj, HandleObject res)
{
    
    
    
    JS_ASSERT(res);
    JS_ASSERT(res->is<ArrayObject>());
    JS_ASSERT(!res->getDenseInitializedLength());
    JS_ASSERT(res->type() == templateObj->type());

    if (length > 0) {
        JSObject::EnsureDenseResult edr =
            res->ensureDenseElementsPreservePackedFlag(slice, 0, length);
        if (edr != JSObject::ED_OK)
            return nullptr;
        res->initDenseElementsUnbarriered(0, rest, length);
        res->as<ArrayObject>().setLengthInt32(length);
    }

    return res;
}
