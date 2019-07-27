





#include "jit/VMFunctions.h"

#include "builtin/TypedObject.h"
#include "frontend/BytecodeCompiler.h"
#include "jit/arm/Simulator-arm.h"
#include "jit/BaselineIC.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/mips/Simulator-mips.h"
#include "vm/ArrayObject.h"
#include "vm/Debugger.h"
#include "vm/Interpreter.h"
#include "vm/TraceLogging.h"

#include "jit/BaselineFrame-inl.h"
#include "jit/JitFrames-inl.h"
#include "vm/Debugger-inl.h"
#include "vm/Interpreter-inl.h"
#include "vm/NativeObject-inl.h"
#include "vm/StringObject-inl.h"
#include "vm/TypeInference-inl.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {



 VMFunction* VMFunction::functions;

AutoDetectInvalidation::AutoDetectInvalidation(JSContext* cx, MutableHandleValue rval)
  : cx_(cx),
    ionScript_(GetTopJitJSScript(cx)->ionScript()),
    rval_(rval),
    disabled_(false)
{ }

void
VMFunction::addToFunctions()
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        functions = nullptr;
    }
    this->next = functions;
    functions = this;
}

bool
InvokeFunction(JSContext* cx, HandleObject obj, uint32_t argc, Value* argv, Value* rval)
{
    AutoArrayRooter argvRoot(cx, argc + 1, argv);

    
    Value thisv = argv[0];
    Value* argvWithoutThis = argv + 1;

    
    
    
    RootedValue rv(cx);
    if (thisv.isMagic(JS_IS_CONSTRUCTING)) {
        if (!InvokeConstructor(cx, ObjectValue(*obj), argc, argvWithoutThis, &rv))
            return false;
    } else {
        if (!Invoke(cx, thisv, ObjectValue(*obj), argc, argvWithoutThis, &rv))
            return false;
    }

    if (obj->is<JSFunction>()) {
        jsbytecode* pc;
        RootedScript script(cx, cx->currentScript(&pc));
        TypeScript::Monitor(cx, script, pc, rv.get());
    }

    *rval = rv;
    return true;
}

bool
CheckOverRecursed(JSContext* cx)
{
    
    
    
    
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    JS_CHECK_SIMULATOR_RECURSION_WITH_EXTRA(cx, 0, return false);
#else
    JS_CHECK_RECURSION(cx, return false);
#endif
    gc::MaybeVerifyBarriers(cx);
    return cx->runtime()->handleInterrupt(cx);
}










bool
CheckOverRecursedWithExtra(JSContext* cx, BaselineFrame* frame,
                           uint32_t extra, uint32_t earlyCheck)
{
    MOZ_ASSERT_IF(earlyCheck, !frame->overRecursed());

    
    
    
    uint8_t spDummy;
    uint8_t* checkSp = (&spDummy) - extra;
    if (earlyCheck) {
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
        (void)checkSp;
        JS_CHECK_SIMULATOR_RECURSION_WITH_EXTRA(cx, extra, frame->setOverRecursed());
#else
        JS_CHECK_RECURSION_WITH_SP(cx, checkSp, frame->setOverRecursed());
#endif
        return true;
    }

    
    
    if (frame->overRecursed())
        return false;

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    JS_CHECK_SIMULATOR_RECURSION_WITH_EXTRA(cx, extra, return false);
#else
    JS_CHECK_RECURSION_WITH_SP(cx, checkSp, return false);
#endif

    gc::MaybeVerifyBarriers(cx);
    return cx->runtime()->handleInterrupt(cx);
}

bool
DefVarOrConst(JSContext* cx, HandlePropertyName dn, unsigned attrs, HandleObject scopeChain)
{
    
    RootedObject obj(cx, scopeChain);
    while (!obj->isQualifiedVarObj())
        obj = obj->enclosingScope();

    return DefVarOrConstOperation(cx, obj, dn, attrs);
}

bool
SetConst(JSContext* cx, HandlePropertyName name, HandleObject scopeChain, HandleValue rval)
{
    
    RootedObject obj(cx, scopeChain);
    while (!obj->isQualifiedVarObj())
        obj = obj->enclosingScope();

    return SetConstOperation(cx, obj, name, rval);
}

bool
MutatePrototype(JSContext* cx, HandlePlainObject obj, HandleValue value)
{
    if (!value.isObjectOrNull())
        return true;

    RootedObject newProto(cx, value.toObjectOrNull());
    return SetPrototype(cx, obj, newProto);
}

bool
InitProp(JSContext* cx, HandleObject obj, HandlePropertyName name, HandleValue value,
         jsbytecode* pc)
{
    RootedId id(cx, NameToId(name));
    return InitPropertyOperation(cx, JSOp(*pc), obj, id, value);
}

template<bool Equal>
bool
LooselyEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res)
{
    if (!js::LooselyEqual(cx, lhs, rhs, res))
        return false;
    if (!Equal)
        *res = !*res;
    return true;
}

template bool LooselyEqual<true>(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);
template bool LooselyEqual<false>(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);

template<bool Equal>
bool
StrictlyEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res)
{
    if (!js::StrictlyEqual(cx, lhs, rhs, res))
        return false;
    if (!Equal)
        *res = !*res;
    return true;
}

template bool StrictlyEqual<true>(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);
template bool StrictlyEqual<false>(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res);

bool
LessThan(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res)
{
    return LessThanOperation(cx, lhs, rhs, res);
}

bool
LessThanOrEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res)
{
    return LessThanOrEqualOperation(cx, lhs, rhs, res);
}

bool
GreaterThan(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res)
{
    return GreaterThanOperation(cx, lhs, rhs, res);
}

bool
GreaterThanOrEqual(JSContext* cx, MutableHandleValue lhs, MutableHandleValue rhs, bool* res)
{
    return GreaterThanOrEqualOperation(cx, lhs, rhs, res);
}

template<bool Equal>
bool
StringsEqual(JSContext* cx, HandleString lhs, HandleString rhs, bool* res)
{
    if (!js::EqualStrings(cx, lhs, rhs, res))
        return false;
    if (!Equal)
        *res = !*res;
    return true;
}

template bool StringsEqual<true>(JSContext* cx, HandleString lhs, HandleString rhs, bool* res);
template bool StringsEqual<false>(JSContext* cx, HandleString lhs, HandleString rhs, bool* res);

bool
ArraySpliceDense(JSContext* cx, HandleObject obj, uint32_t start, uint32_t deleteCount)
{
    JS::AutoValueArray<4> argv(cx);
    argv[0].setUndefined();
    argv[1].setObject(*obj);
    argv[2].set(Int32Value(start));
    argv[3].set(Int32Value(deleteCount));

    return js::array_splice_impl(cx, 2, argv.begin(), false);
}

bool
ArrayPopDense(JSContext* cx, HandleObject obj, MutableHandleValue rval)
{
    MOZ_ASSERT(obj->is<ArrayObject>());

    AutoDetectInvalidation adi(cx, rval);

    JS::AutoValueArray<2> argv(cx);
    argv[0].setUndefined();
    argv[1].setObject(*obj);
    if (!js::array_pop(cx, 0, argv.begin()))
        return false;

    
    
    rval.set(argv[0]);
    if (rval.isUndefined())
        TypeScript::Monitor(cx, rval);
    return true;
}

bool
ArrayPushDense(JSContext* cx, HandleArrayObject obj, HandleValue v, uint32_t* length)
{
    if (MOZ_LIKELY(obj->lengthIsWritable())) {
        uint32_t idx = obj->length();
        NativeObject::EnsureDenseResult result = obj->ensureDenseElements(cx, idx, 1);
        if (result == NativeObject::ED_FAILED)
            return false;

        if (result == NativeObject::ED_OK) {
            obj->setDenseElement(idx, v);
            MOZ_ASSERT(idx < INT32_MAX);
            *length = idx + 1;
            obj->setLengthInt32(*length);
            return true;
        }
    }

    JS::AutoValueArray<3> argv(cx);
    argv[0].setUndefined();
    argv[1].setObject(*obj);
    argv[2].set(v);
    if (!js::array_push(cx, 1, argv.begin()))
        return false;

    *length = argv[0].toInt32();
    return true;
}

bool
ArrayShiftDense(JSContext* cx, HandleObject obj, MutableHandleValue rval)
{
    MOZ_ASSERT(obj->is<ArrayObject>());

    AutoDetectInvalidation adi(cx, rval);

    JS::AutoValueArray<2> argv(cx);
    argv[0].setUndefined();
    argv[1].setObject(*obj);
    if (!js::array_shift(cx, 0, argv.begin()))
        return false;

    
    
    rval.set(argv[0]);
    if (rval.isUndefined())
        TypeScript::Monitor(cx, rval);
    return true;
}

JSObject*
ArrayConcatDense(JSContext* cx, HandleObject obj1, HandleObject obj2, HandleObject objRes)
{
    Rooted<ArrayObject*> arr1(cx, &obj1->as<ArrayObject>());
    Rooted<ArrayObject*> arr2(cx, &obj2->as<ArrayObject>());
    Rooted<ArrayObject*> arrRes(cx, objRes ? &objRes->as<ArrayObject>() : nullptr);

    if (arrRes) {
        
        if (!js::array_concat_dense(cx, arr1, arr2, arrRes))
            return nullptr;
        return arrRes;
    }

    JS::AutoValueArray<3> argv(cx);
    argv[0].setUndefined();
    argv[1].setObject(*arr1);
    argv[2].setObject(*arr2);
    if (!js::array_concat(cx, 1, argv.begin()))
        return nullptr;
    return &argv[0].toObject();
}

JSString*
ArrayJoin(JSContext* cx, HandleObject array, HandleString sep)
{
    
    

    
    RootedObject obj(cx, array);
    if (!obj)
        return nullptr;

    AutoCycleDetector detector(cx, obj);
    if (!detector.init())
        return nullptr;

    if (detector.foundCycle())
        return nullptr;

    
    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return nullptr;

    
    RootedLinearString sepstr(cx);
    if (sep) {
        sepstr = sep->ensureLinear(cx);
        if (!sepstr)
            return nullptr;
    } else {
        sepstr = cx->names().comma;
    }

    
    return js::ArrayJoin<false>(cx, obj, sepstr, length);
}


bool
CharCodeAt(JSContext* cx, HandleString str, int32_t index, uint32_t* code)
{
    char16_t c;
    if (!str->getChar(cx, index, &c))
        return false;
    *code = c;
    return true;
}

JSFlatString*
StringFromCharCode(JSContext* cx, int32_t code)
{
    char16_t c = char16_t(code);

    if (StaticStrings::hasUnit(c))
        return cx->staticStrings().getUnit(c);

    return NewStringCopyN<CanGC>(cx, &c, 1);
}

bool
SetProperty(JSContext* cx, HandleObject obj, HandlePropertyName name, HandleValue value,
            bool strict, jsbytecode* pc)
{
    RootedId id(cx, NameToId(name));

    JSOp op = JSOp(*pc);

    if (op == JSOP_SETALIASEDVAR) {
        
        
        Shape* shape = obj->as<NativeObject>().lookup(cx, name);
        MOZ_ASSERT(shape && shape->hasSlot());
        obj->as<NativeObject>().setSlotWithType(cx, shape, value);
        return true;
    }

    RootedValue receiver(cx, ObjectValue(*obj));
    ObjectOpResult result;
    if (MOZ_LIKELY(!obj->getOps()->setProperty)) {
        if (!NativeSetProperty(
                cx, obj.as<NativeObject>(), id, value, receiver,
                (op == JSOP_SETNAME || op == JSOP_STRICTSETNAME ||
                 op == JSOP_SETGNAME || op == JSOP_STRICTSETGNAME)
                ? Unqualified
                : Qualified,
                result))
        {
            return false;
        }
    } else {
        if (!SetProperty(cx, obj, id, value, receiver, result))
            return false;
    }
    return result.checkStrictErrorOrWarning(cx, obj, id, strict);
}

bool
InterruptCheck(JSContext* cx)
{
    gc::MaybeVerifyBarriers(cx);

    {
        JitRuntime* jrt = cx->runtime()->jitRuntime();
        JitRuntime::AutoMutateBackedges amb(jrt);
        jrt->patchIonBackedges(cx->runtime(), JitRuntime::BackedgeLoopHeader);
    }

    return CheckForInterrupt(cx);
}

void*
MallocWrapper(JSRuntime* rt, size_t nbytes)
{
    return rt->pod_malloc<uint8_t>(nbytes);
}

JSObject*
NewCallObject(JSContext* cx, HandleShape shape, HandleObjectGroup group, uint32_t lexicalBegin)
{
    JSObject* obj = CallObject::create(cx, shape, group, lexicalBegin);
    if (!obj)
        return nullptr;

    
    
    
    if (!IsInsideNursery(obj))
        cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(obj);

    return obj;
}

JSObject*
NewSingletonCallObject(JSContext* cx, HandleShape shape, uint32_t lexicalBegin)
{
    JSObject* obj = CallObject::createSingleton(cx, shape, lexicalBegin);
    if (!obj)
        return nullptr;

    
    
    
    MOZ_ASSERT(!IsInsideNursery(obj),
               "singletons are created in the tenured heap");
    cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(obj);

    return obj;
}

JSObject*
NewStringObject(JSContext* cx, HandleString str)
{
    return StringObject::create(cx, str);
}

bool
OperatorIn(JSContext* cx, HandleValue key, HandleObject obj, bool* out)
{
    RootedId id(cx);
    return ValueToId<CanGC>(cx, key, &id) &&
           HasProperty(cx, obj, id, out);
}

bool
OperatorInI(JSContext* cx, uint32_t index, HandleObject obj, bool* out)
{
    RootedValue key(cx, Int32Value(index));
    return OperatorIn(cx, key, obj, out);
}

bool
GetIntrinsicValue(JSContext* cx, HandlePropertyName name, MutableHandleValue rval)
{
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), name, rval))
        return false;

    
    
    
    
    
    TypeScript::Monitor(cx, rval);

    return true;
}

bool
CreateThis(JSContext* cx, HandleObject callee, MutableHandleValue rval)
{
    rval.set(MagicValue(JS_IS_CONSTRUCTING));

    if (callee->is<JSFunction>()) {
        JSFunction* fun = &callee->as<JSFunction>();
        if (fun->isInterpretedConstructor()) {
            JSScript* script = fun->getOrCreateScript(cx);
            if (!script || !script->ensureHasTypes(cx))
                return false;
            JSObject* thisObj = CreateThisForFunction(cx, callee, GenericObject);
            if (!thisObj)
                return false;
            rval.set(ObjectValue(*thisObj));
        }
    }

    return true;
}

void
GetDynamicName(JSContext* cx, JSObject* scopeChain, JSString* str, Value* vp)
{
    
    
    

    JSAtom* atom;
    if (str->isAtom()) {
        atom = &str->asAtom();
    } else {
        atom = AtomizeString(cx, str);
        if (!atom) {
            vp->setUndefined();
            return;
        }
    }

    if (!frontend::IsIdentifier(atom) || frontend::IsKeyword(atom)) {
        vp->setUndefined();
        return;
    }

    Shape* shape = nullptr;
    JSObject* scope = nullptr;
    JSObject* pobj = nullptr;
    if (LookupNameNoGC(cx, atom->asPropertyName(), scopeChain, &scope, &pobj, &shape)) {
        if (FetchNameNoGC(pobj, shape, MutableHandleValue::fromMarkedLocation(vp)))
            return;
    }

    vp->setUndefined();
}

bool
FilterArgumentsOrEval(JSContext* cx, JSString* str)
{
    
    
    
    
    JS::AutoCheckCannotGC nogc;
    JSLinearString* linear = str->ensureLinear(cx);
    if (!linear)
        return false;

    static const char16_t arguments[] = {'a', 'r', 'g', 'u', 'm', 'e', 'n', 't', 's'};
    static const char16_t eval[] = {'e', 'v', 'a', 'l'};

    return !StringHasPattern(linear, arguments, mozilla::ArrayLength(arguments)) &&
        !StringHasPattern(linear, eval, mozilla::ArrayLength(eval));
}

void
PostWriteBarrier(JSRuntime* rt, JSObject* obj)
{
    MOZ_ASSERT(!IsInsideNursery(obj));
    rt->gc.storeBuffer.putWholeCellFromMainThread(obj);
}

void
PostGlobalWriteBarrier(JSRuntime* rt, JSObject* obj)
{
    MOZ_ASSERT(obj->is<GlobalObject>());
    if (!obj->compartment()->globalWriteBarriered) {
        PostWriteBarrier(rt, obj);
        obj->compartment()->globalWriteBarriered = true;
    }
}

uint32_t
GetIndexFromString(JSString* str)
{
    
    
    

    if (!str->isAtom())
        return UINT32_MAX;

    uint32_t index;
    JSAtom* atom = &str->asAtom();
    if (!atom->isIndex(&index))
        return UINT32_MAX;

    return index;
}

bool
DebugPrologue(JSContext* cx, BaselineFrame* frame, jsbytecode* pc, bool* mustReturn)
{
    *mustReturn = false;

    switch (Debugger::onEnterFrame(cx, frame)) {
      case JSTRAP_CONTINUE:
        return true;

      case JSTRAP_RETURN:
        
        
        MOZ_ASSERT(frame->hasReturnValue());
        *mustReturn = true;
        return jit::DebugEpilogue(cx, frame, pc, true);

      case JSTRAP_THROW:
      case JSTRAP_ERROR:
        return false;

      default:
        MOZ_CRASH("bad Debugger::onEnterFrame status");
    }
}

bool
DebugEpilogueOnBaselineReturn(JSContext* cx, BaselineFrame* frame, jsbytecode* pc)
{
    if (!DebugEpilogue(cx, frame, pc, true)) {
        
        
        TraceLoggerThread* logger = TraceLoggerForMainThread(cx->runtime());
        TraceLogStopEvent(logger, TraceLogger_Baseline);
        TraceLogStopEvent(logger, TraceLogger_Scripts);
        return false;
    }

    return true;
}

bool
DebugEpilogue(JSContext* cx, BaselineFrame* frame, jsbytecode* pc, bool ok)
{
    
    
    
    ok = Debugger::onLeaveFrame(cx, frame, ok);

    
    
    ScopeIter si(cx, frame, pc);
    UnwindAllScopesInFrame(cx, si);
    JSScript* script = frame->script();
    frame->setOverridePc(script->lastPC());

    if (frame->isNonEvalFunctionFrame()) {
        MOZ_ASSERT_IF(ok, frame->hasReturnValue());
        DebugScopes::onPopCall(frame, cx);
    } else if (frame->isStrictEvalFrame()) {
        MOZ_ASSERT_IF(frame->hasCallObj(), frame->scopeChain()->as<CallObject>().isForEval());
        DebugScopes::onPopStrictEvalScope(frame);
    }

    if (!ok) {
        
        

        JitFrameLayout* prefix = frame->framePrefix();
        EnsureExitFrame(prefix);
        cx->runtime()->jitTop = (uint8_t*)prefix;
        return false;
    }

    
    
    
    frame->clearOverridePc();
    return true;
}

void
FrameIsDebuggeeCheck(BaselineFrame* frame)
{
    if (frame->script()->isDebuggee())
        frame->setIsDebuggee();
}

JSObject*
CreateGenerator(JSContext* cx, BaselineFrame* frame)
{
    return GeneratorObject::create(cx, frame);
}

bool
NormalSuspend(JSContext* cx, HandleObject obj, BaselineFrame* frame, jsbytecode* pc,
              uint32_t stackDepth)
{
    MOZ_ASSERT(*pc == JSOP_YIELD);

    
    MOZ_ASSERT(stackDepth >= 1);

    
    
    
    AutoValueVector exprStack(cx);
    if (!exprStack.reserve(stackDepth - 1))
        return false;

    size_t firstSlot = frame->numValueSlots() - stackDepth;
    for (size_t i = 0; i < stackDepth - 1; i++)
        exprStack.infallibleAppend(*frame->valueSlot(firstSlot + i));

    MOZ_ASSERT(exprStack.length() == stackDepth - 1);

    return GeneratorObject::normalSuspend(cx, obj, frame, pc, exprStack.begin(), stackDepth - 1);
}

bool
FinalSuspend(JSContext* cx, HandleObject obj, BaselineFrame* frame, jsbytecode* pc)
{
    MOZ_ASSERT(*pc == JSOP_FINALYIELDRVAL);

    if (!GeneratorObject::finalSuspend(cx, obj)) {
        
        return DebugEpilogue(cx, frame, pc,  false);
    }

    return true;
}

bool
InterpretResume(JSContext* cx, HandleObject obj, HandleValue val, HandlePropertyName kind,
                MutableHandleValue rval)
{
    MOZ_ASSERT(obj->is<GeneratorObject>());

    RootedValue selfHostedFun(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), cx->names().InterpretGeneratorResume,
                                         &selfHostedFun))
    {
        return false;
    }

    MOZ_ASSERT(selfHostedFun.toObject().is<JSFunction>());

    InvokeArgs args(cx);
    if (!args.init(3))
        return false;

    args.setCallee(selfHostedFun);
    args.setThis(UndefinedValue());

    args[0].setObject(*obj);
    args[1].set(val);
    args[2].setString(kind);

    if (!Invoke(cx, args))
        return false;

    rval.set(args.rval());
    return true;
}

bool
DebugAfterYield(JSContext* cx, BaselineFrame* frame)
{
    
    
    if (frame->script()->isDebuggee())
        frame->setIsDebuggee();
    return true;
}

bool
GeneratorThrowOrClose(JSContext* cx, BaselineFrame* frame, Handle<GeneratorObject*> genObj,
                      HandleValue arg, uint32_t resumeKind)
{
    
    
    
    JSScript* script = frame->script();
    uint32_t offset = script->yieldOffsets()[genObj->yieldIndex()];
    frame->setOverridePc(script->offsetToPC(offset));

    MOZ_ALWAYS_TRUE(DebugAfterYield(cx, frame));
    MOZ_ALWAYS_FALSE(js::GeneratorThrowOrClose(cx, frame, genObj, arg, resumeKind));
    return false;
}

bool
StrictEvalPrologue(JSContext* cx, BaselineFrame* frame)
{
    return frame->strictEvalPrologue(cx);
}

bool
HeavyweightFunPrologue(JSContext* cx, BaselineFrame* frame)
{
    return frame->heavyweightFunPrologue(cx);
}

bool
NewArgumentsObject(JSContext* cx, BaselineFrame* frame, MutableHandleValue res)
{
    ArgumentsObject* obj = ArgumentsObject::createExpected(cx, frame);
    if (!obj)
        return false;
    res.setObject(*obj);
    return true;
}

JSObject*
InitRestParameter(JSContext* cx, uint32_t length, Value* rest, HandleObject templateObj,
                  HandleObject objRes)
{
    if (objRes) {
        Rooted<ArrayObject*> arrRes(cx, &objRes->as<ArrayObject>());

        MOZ_ASSERT(!arrRes->getDenseInitializedLength());
        MOZ_ASSERT(arrRes->group() == templateObj->group());

        
        
        if (length > 0) {
            if (!arrRes->ensureElements(cx, length))
                return nullptr;
            arrRes->setDenseInitializedLength(length);
            arrRes->initDenseElements(0, rest, length);
            arrRes->setLengthInt32(length);
        }
        return arrRes;
    }

    NewObjectKind newKind = templateObj->group()->shouldPreTenure()
                            ? TenuredObject
                            : GenericObject;
    ArrayObject* arrRes = NewDenseCopiedArray(cx, length, rest, NullPtr(), newKind);
    if (arrRes)
        arrRes->setGroup(templateObj->group());
    return arrRes;
}

bool
HandleDebugTrap(JSContext* cx, BaselineFrame* frame, uint8_t* retAddr, bool* mustReturn)
{
    *mustReturn = false;

    RootedScript script(cx, frame->script());
    jsbytecode* pc = script->baselineScript()->icEntryFromReturnAddress(retAddr).pc(script);

    MOZ_ASSERT(frame->isDebuggee());
    MOZ_ASSERT(script->stepModeEnabled() || script->hasBreakpointsAt(pc));

    RootedValue rval(cx);
    JSTrapStatus status = JSTRAP_CONTINUE;

    if (script->stepModeEnabled())
        status = Debugger::onSingleStep(cx, &rval);

    if (status == JSTRAP_CONTINUE && script->hasBreakpointsAt(pc))
        status = Debugger::onTrap(cx, &rval);

    switch (status) {
      case JSTRAP_CONTINUE:
        break;

      case JSTRAP_ERROR:
        return false;

      case JSTRAP_RETURN:
        *mustReturn = true;
        frame->setReturnValue(rval);
        return jit::DebugEpilogue(cx, frame, pc, true);

      case JSTRAP_THROW:
        cx->setPendingException(rval);
        return false;

      default:
        MOZ_CRASH("Invalid trap status");
    }

    return true;
}

bool
OnDebuggerStatement(JSContext* cx, BaselineFrame* frame, jsbytecode* pc, bool* mustReturn)
{
    *mustReturn = false;

    switch (Debugger::onDebuggerStatement(cx, frame)) {
      case JSTRAP_ERROR:
        return false;

      case JSTRAP_CONTINUE:
        return true;

      case JSTRAP_RETURN:
        *mustReturn = true;
        return jit::DebugEpilogue(cx, frame, pc, true);

      case JSTRAP_THROW:
        return false;

      default:
        MOZ_CRASH("Invalid trap status");
    }
}

bool
GlobalHasLiveOnDebuggerStatement(JSContext* cx)
{
    return cx->compartment()->isDebuggee() &&
           Debugger::hasLiveHook(cx->global(), Debugger::OnDebuggerStatement);
}

bool
PushBlockScope(JSContext* cx, BaselineFrame* frame, Handle<StaticBlockObject*> block)
{
    return frame->pushBlock(cx, block);
}

bool
PopBlockScope(JSContext* cx, BaselineFrame* frame)
{
    frame->popBlock(cx);
    return true;
}

bool
DebugLeaveThenPopBlockScope(JSContext* cx, BaselineFrame* frame, jsbytecode* pc)
{
    MOZ_ALWAYS_TRUE(DebugLeaveBlock(cx, frame, pc));
    frame->popBlock(cx);
    return true;
}

bool
FreshenBlockScope(JSContext* cx, BaselineFrame* frame)
{
    return frame->freshenBlock(cx);
}

bool
DebugLeaveThenFreshenBlockScope(JSContext* cx, BaselineFrame* frame, jsbytecode* pc)
{
    MOZ_ALWAYS_TRUE(DebugLeaveBlock(cx, frame, pc));
    return frame->freshenBlock(cx);
}

bool
DebugLeaveBlock(JSContext* cx, BaselineFrame* frame, jsbytecode* pc)
{
    MOZ_ASSERT(frame->script()->baselineScript()->hasDebugInstrumentation());
    if (cx->compartment()->isDebuggee())
        DebugScopes::onPopBlock(cx, frame, pc);
    return true;
}

bool
EnterWith(JSContext* cx, BaselineFrame* frame, HandleValue val, Handle<StaticWithObject*> templ)
{
    return EnterWithOperation(cx, frame, val, templ);
}

bool
LeaveWith(JSContext* cx, BaselineFrame* frame)
{
    frame->popWith(cx);
    return true;
}

bool
InitBaselineFrameForOsr(BaselineFrame* frame, InterpreterFrame* interpFrame,
                        uint32_t numStackValues)
{
    return frame->initForOsr(interpFrame, numStackValues);
}

JSObject*
CreateDerivedTypedObj(JSContext* cx, HandleObject descr,
                      HandleObject owner, int32_t offset)
{
    MOZ_ASSERT(descr->is<TypeDescr>());
    MOZ_ASSERT(owner->is<TypedObject>());
    Rooted<TypeDescr*> descr1(cx, &descr->as<TypeDescr>());
    Rooted<TypedObject*> owner1(cx, &owner->as<TypedObject>());
    return OutlineTypedObject::createDerived(cx, descr1, owner1, offset);
}

JSString*
RegExpReplace(JSContext* cx, HandleString string, HandleObject regexp, HandleString repl)
{
    MOZ_ASSERT(string);
    MOZ_ASSERT(repl);

    RootedValue rval(cx);
    if (!str_replace_regexp_raw(cx, string, regexp, repl, &rval))
        return nullptr;

    return rval.toString();
}

JSString*
StringReplace(JSContext* cx, HandleString string, HandleString pattern, HandleString repl)
{
    MOZ_ASSERT(string);
    MOZ_ASSERT(pattern);
    MOZ_ASSERT(repl);

    RootedValue rval(cx);
    if (!str_replace_string_raw(cx, string, pattern, repl, &rval))
        return nullptr;

    return rval.toString();
}

bool
RecompileImpl(JSContext* cx, bool force)
{
    MOZ_ASSERT(cx->currentlyRunningInJit());
    JitActivationIterator activations(cx->runtime());
    JitFrameIterator iter(activations);

    MOZ_ASSERT(iter.type() == JitFrame_Exit);
    ++iter;

    bool isConstructing = iter.isConstructing();
    RootedScript script(cx, iter.script());
    MOZ_ASSERT(script->hasIonScript());

    if (!IsIonEnabled(cx))
        return true;

    MethodStatus status = Recompile(cx, script, nullptr, nullptr, isConstructing, force);
    if (status == Method_Error)
        return false;

    return true;
}

bool
ForcedRecompile(JSContext* cx)
{
    return RecompileImpl(cx,  true);
}

bool
Recompile(JSContext* cx)
{
    return RecompileImpl(cx,  false);
}

bool
SetDenseOrUnboxedArrayElement(JSContext* cx, HandleObject obj, int32_t index,
                              HandleValue value, bool strict)
{
    
    
    

    NativeObject::EnsureDenseResult result = NativeObject::ED_SPARSE;
    do {
        if (index < 0 || obj->is<UnboxedArrayObject>())
            break;
        bool isArray = obj->is<ArrayObject>();
        if (isArray && !obj->as<ArrayObject>().lengthIsWritable())
            break;
        uint32_t idx = uint32_t(index);
        result = obj->as<NativeObject>().ensureDenseElements(cx, idx, 1);
        if (result != NativeObject::ED_OK)
            break;
        if (isArray) {
            ArrayObject& arr = obj->as<ArrayObject>();
            if (idx >= arr.length())
                arr.setLengthInt32(idx + 1);
        }
        obj->as<NativeObject>().setDenseElement(idx, value);
        return true;
    } while (false);

    if (result == NativeObject::ED_FAILED)
        return false;
    MOZ_ASSERT(result == NativeObject::ED_SPARSE);

    RootedValue indexVal(cx, Int32Value(index));
    return SetObjectElement(cx, obj, indexVal, value, strict);
}

void
AutoDetectInvalidation::setReturnOverride()
{
    cx_->runtime()->jitRuntime()->setIonReturnOverride(rval_.get());
}

void
AssertValidObjectPtr(JSContext* cx, JSObject* obj)
{
#ifdef DEBUG
    
    
    MOZ_ASSERT(obj->compartment() == cx->compartment());
    MOZ_ASSERT(obj->runtimeFromMainThread() == cx->runtime());

    MOZ_ASSERT_IF(!obj->hasLazyGroup() && obj->maybeShape(),
                  obj->group()->clasp() == obj->maybeShape()->getObjectClass());

    if (obj->isTenured()) {
        MOZ_ASSERT(obj->isAligned());
        gc::AllocKind kind = obj->asTenured().getAllocKind();
        MOZ_ASSERT(gc::IsObjectAllocKind(kind));
        MOZ_ASSERT(obj->asTenured().zone() == cx->zone());
    }
#endif
}

void
AssertValidObjectOrNullPtr(JSContext* cx, JSObject* obj)
{
    if (obj)
        AssertValidObjectPtr(cx, obj);
}

void
AssertValidStringPtr(JSContext* cx, JSString* str)
{
#ifdef DEBUG
    
    if (str->runtimeFromAnyThread() != cx->runtime()) {
        MOZ_ASSERT(str->isPermanentAtom());
        return;
    }

    if (str->isAtom())
        MOZ_ASSERT(cx->runtime()->isAtomsZone(str->zone()));
    else
        MOZ_ASSERT(str->zone() == cx->zone());

    MOZ_ASSERT(str->runtimeFromMainThread() == cx->runtime());
    MOZ_ASSERT(str->isAligned());
    MOZ_ASSERT(str->length() <= JSString::MAX_LENGTH);

    gc::AllocKind kind = str->getAllocKind();
    if (str->isFatInline())
        MOZ_ASSERT(kind == gc::AllocKind::FAT_INLINE_STRING);
    else if (str->isExternal())
        MOZ_ASSERT(kind == gc::AllocKind::EXTERNAL_STRING);
    else if (str->isAtom() || str->isFlat())
        MOZ_ASSERT(kind == gc::AllocKind::STRING || kind == gc::AllocKind::FAT_INLINE_STRING);
    else
        MOZ_ASSERT(kind == gc::AllocKind::STRING);
#endif
}

void
AssertValidSymbolPtr(JSContext* cx, JS::Symbol* sym)
{
    
    if (sym->runtimeFromAnyThread() != cx->runtime())
        return;

    MOZ_ASSERT(cx->runtime()->isAtomsZone(sym->zone()));

    MOZ_ASSERT(sym->runtimeFromMainThread() == cx->runtime());
    MOZ_ASSERT(sym->isAligned());
    if (JSString* desc = sym->description()) {
        MOZ_ASSERT(desc->isAtom());
        AssertValidStringPtr(cx, desc);
    }

    MOZ_ASSERT(sym->getAllocKind() == gc::AllocKind::SYMBOL);
}

void
AssertValidValue(JSContext* cx, Value* v)
{
    if (v->isObject())
        AssertValidObjectPtr(cx, &v->toObject());
    else if (v->isString())
        AssertValidStringPtr(cx, v->toString());
    else if (v->isSymbol())
        AssertValidSymbolPtr(cx, v->toSymbol());
}

bool
ObjectIsCallable(JSObject* obj)
{
    return obj->isCallable();
}

void
MarkValueFromIon(JSRuntime* rt, Value* vp)
{
    TraceManuallyBarrieredEdge(&rt->gc.marker, vp, "write barrier");
}

void
MarkStringFromIon(JSRuntime* rt, JSString** stringp)
{
    if (*stringp)
        TraceManuallyBarrieredEdge(&rt->gc.marker, stringp, "write barrier");
}

void
MarkObjectFromIon(JSRuntime* rt, JSObject** objp)
{
    if (*objp)
        TraceManuallyBarrieredEdge(&rt->gc.marker, objp, "write barrier");
}

void
MarkShapeFromIon(JSRuntime* rt, Shape** shapep)
{
    TraceManuallyBarrieredEdge(&rt->gc.marker, shapep, "write barrier");
}

void
MarkObjectGroupFromIon(JSRuntime* rt, ObjectGroup** groupp)
{
    TraceManuallyBarrieredEdge(&rt->gc.marker, groupp, "write barrier");
}

bool
ThrowUninitializedLexical(JSContext* cx)
{
    ScriptFrameIter iter(cx);
    RootedScript script(cx, iter.script());
    ReportUninitializedLexical(cx, script, iter.pc());
    return false;
}

} 
} 
