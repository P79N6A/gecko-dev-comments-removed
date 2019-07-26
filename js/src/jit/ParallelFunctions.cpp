





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


ForkJoinContext *
jit::ForkJoinContextPar()
{
    return ForkJoinContext::current();
}




JSObject *
jit::NewGCThingPar(ForkJoinContext *cx, gc::AllocKind allocKind)
{
    JS_ASSERT(ForkJoinContext::current() == cx);
    return js::NewGCObject<NoGC>(cx, allocKind, 0, gc::TenuredHeap);
}

bool
jit::ParallelWriteGuard(ForkJoinContext *cx, JSObject *object)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(ForkJoinContext::current() == cx);

    if (object->is<TypedDatum>()) {
        TypedDatum &datum = object->as<TypedDatum>();

        
        
        
        
        if (IsInTargetRegion(cx, &datum))
            return true;

        
        TypedDatum &owner = datum.owner();
        return cx->isThreadLocal(&owner);
    }

    
    return cx->isThreadLocal(object);
}









bool
jit::IsInTargetRegion(ForkJoinContext *cx, TypedDatum *datum)
{
    JS_ASSERT(datum->is<TypedDatum>()); 
    uint8_t *typedMem = datum->typedMem();
    return (typedMem >= cx->targetRegionStart &&
            typedMem <  cx->targetRegionEnd);
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
        cached = &ForkJoinContext::current()->traceData;

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
jit::CheckOverRecursedPar(ForkJoinContext *cx)
{
    JS_ASSERT(ForkJoinContext::current() == cx);
    int stackDummy_;

    
    
    
    
    
    
    
    

#ifdef JS_ARM_SIMULATOR
    if (Simulator::Current()->overRecursed()) {
        cx->bailoutRecord->setCause(ParallelBailoutOverRecursed);
        return false;
    }
#endif

    uintptr_t realStackLimit;
    if (cx->isMainThread())
        realStackLimit = GetNativeStackLimit(cx);
    else
        realStackLimit = cx->perThreadData->ionStackLimit;

    if (!JS_CHECK_STACK_SIZE(realStackLimit, &stackDummy_)) {
        cx->bailoutRecord->setCause(ParallelBailoutOverRecursed);
        return false;
    }

    return InterruptCheckPar(cx);
}

bool
jit::InterruptCheckPar(ForkJoinContext *cx)
{
    JS_ASSERT(ForkJoinContext::current() == cx);
    bool result = cx->check();
    if (!result) {
        
        
        
        
        
        return false;
    }
    return true;
}

JSObject *
jit::ExtendArrayPar(ForkJoinContext *cx, JSObject *array, uint32_t length)
{
    JSObject::EnsureDenseResult res =
        array->ensureDenseElementsPreservePackedFlag(cx, 0, length);
    if (res != JSObject::ED_OK)
        return nullptr;
    return array;
}

bool
jit::SetPropertyPar(ForkJoinContext *cx, HandleObject obj, HandlePropertyName name,
                    HandleValue value, bool strict, jsbytecode *pc)
{
    JS_ASSERT(cx->isThreadLocal(obj));

    if (*pc == JSOP_SETALIASEDVAR) {
        
        Shape *shape = obj->nativeLookupPure(name);
        JS_ASSERT(shape && shape->hasSlot());
        return obj->nativeSetSlotIfHasType(shape, value);
    }

    
    if (obj->getOps()->setProperty)
        return TP_RETRY_SEQUENTIALLY;

    RootedValue v(cx, value);
    RootedId id(cx, NameToId(name));
    return baseops::SetPropertyHelper<ParallelExecution>(cx, obj, obj, id, 0, &v, strict);
}

bool
jit::SetElementPar(ForkJoinContext *cx, HandleObject obj, HandleValue index, HandleValue value,
                   bool strict)
{
    RootedId id(cx);
    if (!ValueToIdPure(index, id.address()))
        return false;

    
    
    
    
    
    RootedValue v(cx, value);
    return baseops::SetPropertyHelper<ParallelExecution>(cx, obj, obj, id, 0, &v, strict);
}

JSString *
jit::ConcatStringsPar(ForkJoinContext *cx, HandleString left, HandleString right)
{
    return ConcatStrings<NoGC>(cx, left, right);
}

JSFlatString *
jit::IntToStringPar(ForkJoinContext *cx, int i)
{
    return Int32ToString<NoGC>(cx, i);
}

JSString *
jit::DoubleToStringPar(ForkJoinContext *cx, double d)
{
    return NumberToString<NoGC>(cx, d);
}

JSString *
jit::PrimitiveToStringPar(ForkJoinContext *cx, HandleValue input)
{
    
    JS_ASSERT(input.isDouble() || input.isInt32());

    if (input.isInt32())
        return Int32ToString<NoGC>(cx, input.toInt32());

    return NumberToString<NoGC>(cx, input.toDouble());
}

bool
jit::StringToNumberPar(ForkJoinContext *cx, JSString *str, double *out)
{
    return StringToNumber(cx, str, out);
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
        if (!CompareMaybeStringsPar(cx, lhs, rhs, &vsZero))                  \
            return false;                                                       \
        *res = (vsZero OP 0) == EXPECTED;                                       \
    }                                                                           \
    return true;                                                                \
} while(0)

static bool
CompareStringsPar(ForkJoinContext *cx, JSString *left, JSString *right, int32_t *res)
{
    ScopedThreadSafeStringInspector leftInspector(left);
    ScopedThreadSafeStringInspector rightInspector(right);
    if (!leftInspector.ensureChars(cx) || !rightInspector.ensureChars(cx))
        return false;

    *res = CompareChars(leftInspector.chars(), left->length(),
                        rightInspector.chars(), right->length());
    return true;
}

static bool
CompareMaybeStringsPar(ForkJoinContext *cx, HandleValue v1, HandleValue v2, int32_t *res)
{
    if (!v1.isString())
        return false;
    if (!v2.isString())
        return false;
    return CompareStringsPar(cx, v1.toString(), v2.toString(), res);
}

template<bool Equal>
bool
LooselyEqualImplPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(==, Equal);
}

bool
js::jit::LooselyEqualPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return LooselyEqualImplPar<true>(cx, lhs, rhs, res);
}

bool
js::jit::LooselyUnequalPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return LooselyEqualImplPar<false>(cx, lhs, rhs, res);
}

template<bool Equal>
bool
StrictlyEqualImplPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
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
            return LooselyEqualImplPar<Equal>(cx, lhs, rhs, res);
    }

    *res = false;
    return true;
}

bool
js::jit::StrictlyEqualPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return StrictlyEqualImplPar<true>(cx, lhs, rhs, res);
}

bool
js::jit::StrictlyUnequalPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    return StrictlyEqualImplPar<false>(cx, lhs, rhs, res);
}

bool
js::jit::LessThanPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(<, true);
}

bool
js::jit::LessThanOrEqualPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(<=, true);
}

bool
js::jit::GreaterThanPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(>, true);
}

bool
js::jit::GreaterThanOrEqualPar(ForkJoinContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, bool *res)
{
    PAR_RELATIONAL_OP(>=, true);
}

template<bool Equal>
bool
StringsEqualImplPar(ForkJoinContext *cx, HandleString lhs, HandleString rhs, bool *res)
{
    int32_t vsZero;
    bool ret = CompareStringsPar(cx, lhs, rhs, &vsZero);
    if (ret != true)
        return ret;
    *res = (vsZero == 0) == Equal;
    return true;
}

bool
js::jit::StringsEqualPar(ForkJoinContext *cx, HandleString v1, HandleString v2, bool *res)
{
    return StringsEqualImplPar<true>(cx, v1, v2, res);
}

bool
js::jit::StringsUnequalPar(ForkJoinContext *cx, HandleString v1, HandleString v2, bool *res)
{
    return StringsEqualImplPar<false>(cx, v1, v2, res);
}

bool
jit::BitNotPar(ForkJoinContext *cx, HandleValue in, int32_t *out)
{
    if (in.isObject())
        return false;
    int i;
    if (!NonObjectToInt32(cx, in, &i))
        return false;
    *out = ~i;
    return true;
}

#define BIT_OP(OP)                                                      \
    JS_BEGIN_MACRO                                                      \
    int32_t left, right;                                                \
    if (lhs.isObject() || rhs.isObject())                               \
        return TP_RETRY_SEQUENTIALLY;                                   \
    if (!NonObjectToInt32(cx, lhs, &left) ||                         \
        !NonObjectToInt32(cx, rhs, &right))                          \
    {                                                                   \
        return false;                                                   \
    }                                                                   \
    *out = (OP);                                                        \
    return true;                                                        \
    JS_END_MACRO

bool
jit::BitXorPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left ^ right);
}

bool
jit::BitOrPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left | right);
}

bool
jit::BitAndPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left & right);
}

bool
jit::BitLshPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left << (right & 31));
}

bool
jit::BitRshPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out)
{
    BIT_OP(left >> (right & 31));
}

#undef BIT_OP

bool
jit::UrshValuesPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs,
                   Value *out)
{
    uint32_t left;
    int32_t right;
    if (lhs.isObject() || rhs.isObject())
        return false;
    if (!NonObjectToUint32(cx, lhs, &left) || !NonObjectToInt32(cx, rhs, &right))
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

    ForkJoinContext *cx = ForkJoinContext::current();

    JS_ASSERT(cx->bailoutRecord->depth == 0);
    cx->bailoutRecord->setCause(cause, outermostScript, currentScript, bytecode);
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

    ForkJoinContext *cx = ForkJoinContext::current();
    if (currentScript)
        cx->bailoutRecord->addTrace(currentScript, nullptr);
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
jit::InitRestParameterPar(ForkJoinContext *cx, uint32_t length, Value *rest,
                          HandleObject templateObj, HandleObject res)
{
    
    
    
    JS_ASSERT(res);
    JS_ASSERT(res->is<ArrayObject>());
    JS_ASSERT(!res->getDenseInitializedLength());
    JS_ASSERT(res->type() == templateObj->type());

    if (length > 0) {
        JSObject::EnsureDenseResult edr =
            res->ensureDenseElementsPreservePackedFlag(cx, 0, length);
        if (edr != JSObject::ED_OK)
            return nullptr;
        res->initDenseElementsUnbarriered(0, rest, length);
        res->as<ArrayObject>().setLengthInt32(length);
    }

    return res;
}
