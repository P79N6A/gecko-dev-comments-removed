





#include "Ion.h"
#include "IonCompartment.h"
#include "ion/BaselineFrame-inl.h"
#include "ion/BaselineIC.h"
#include "ion/IonFrames.h"

#include "vm/Debugger.h"
#include "vm/Interpreter.h"
#include "vm/StringObject-inl.h"

#include "builtin/ParallelArray.h"

#include "frontend/BytecodeCompiler.h"

#include "jsboolinlines.h"

#include "ion/IonFrames-inl.h" 

#include "vm/Interpreter-inl.h"
#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {



 VMFunction *VMFunction::functions;

void
VMFunction::addToFunctions()
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        functions = NULL;
    }
    this->next = functions;
    functions = this;
}

bool
InvokeFunction(JSContext *cx, HandleFunction fun0, uint32_t argc, Value *argv, Value *rval)
{
    RootedFunction fun(cx, fun0);
    if (fun->isInterpreted()) {
        if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
            return false;

        
        if (fun->nonLazyScript()->shouldCloneAtCallsite) {
            RootedScript script(cx);
            jsbytecode *pc;
            types::TypeScript::GetPcScript(cx, script.address(), &pc);
            fun = CloneFunctionAtCallsite(cx, fun0, script, pc);
            if (!fun)
                return false;
        }
    }

    
    Value thisv = argv[0];
    Value *argvWithoutThis = argv + 1;

    
    
    
    if (thisv.isMagic(JS_IS_CONSTRUCTING))
        return InvokeConstructor(cx, ObjectValue(*fun), argc, argvWithoutThis, rval);
    return Invoke(cx, thisv, ObjectValue(*fun), argc, argvWithoutThis, rval);
}

JSObject *
NewGCThing(JSContext *cx, gc::AllocKind allocKind, size_t thingSize)
{
    return gc::NewGCThing<JSObject, CanGC>(cx, allocKind, thingSize, gc::DefaultHeap);
}

bool
CheckOverRecursed(JSContext *cx)
{
    
    
    
    
    
    
    
    
    
    
    
    
    JS_CHECK_RECURSION(cx, return false);

    if (cx->runtime->interrupt)
        return InterruptCheck(cx);

    return true;
}

bool
DefVarOrConst(JSContext *cx, HandlePropertyName dn, unsigned attrs, HandleObject scopeChain)
{
    
    RootedObject obj(cx, scopeChain);
    while (!obj->isVarObj())
        obj = obj->enclosingScope();

    return DefVarOrConstOperation(cx, obj, dn, attrs);
}

bool
SetConst(JSContext *cx, HandlePropertyName name, HandleObject scopeChain, HandleValue rval)
{
    
    RootedObject obj(cx, scopeChain);
    while (!obj->isVarObj())
        obj = obj->enclosingScope();

    return SetConstOperation(cx, obj, name, rval);
}

bool
InitProp(JSContext *cx, HandleObject obj, HandlePropertyName name, HandleValue value)
{
    
    RootedValue rval(cx, value);
    RootedId id(cx, NameToId(name));

    if (name == cx->names().proto)
        return baseops::SetPropertyHelper(cx, obj, obj, id, 0, &rval, false);
    return DefineNativeProperty(cx, obj, id, rval, NULL, NULL, JSPROP_ENUMERATE, 0, 0, 0);
}

template<bool Equal>
bool
LooselyEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res)
{
    bool equal;
    if (!js::LooselyEqual(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool LooselyEqual<true>(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);
template bool LooselyEqual<false>(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);

template<bool Equal>
bool
StrictlyEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res)
{
    bool equal;
    if (!js::StrictlyEqual(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool StrictlyEqual<true>(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);
template bool StrictlyEqual<false>(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res);

bool
LessThan(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res)
{
    bool cond;
    if (!LessThanOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
LessThanOrEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res)
{
    bool cond;
    if (!LessThanOrEqualOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
GreaterThan(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res)
{
    bool cond;
    if (!GreaterThanOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
GreaterThanOrEqual(JSContext *cx, MutableHandleValue lhs, MutableHandleValue rhs, JSBool *res)
{
    bool cond;
    if (!GreaterThanOrEqualOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

template<bool Equal>
bool
StringsEqual(JSContext *cx, HandleString lhs, HandleString rhs, JSBool *res)
{
    bool equal;
    if (!js::EqualStrings(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool StringsEqual<true>(JSContext *cx, HandleString lhs, HandleString rhs, JSBool *res);
template bool StringsEqual<false>(JSContext *cx, HandleString lhs, HandleString rhs, JSBool *res);

JSBool
ObjectEmulatesUndefined(JSObject *obj)
{
    return EmulatesUndefined(obj);
}

bool
IteratorMore(JSContext *cx, HandleObject obj, JSBool *res)
{
    RootedValue tmp(cx);
    if (!js_IteratorMore(cx, obj, &tmp))
        return false;

    *res = tmp.toBoolean();
    return true;
}

JSObject *
NewInitParallelArray(JSContext *cx, HandleObject templateObject)
{
    JS_ASSERT(templateObject->getClass() == &ParallelArrayObject::class_);
    JS_ASSERT(!templateObject->hasSingletonType());

    RootedObject obj(cx, ParallelArrayObject::newInstance(cx, TenuredObject));
    if (!obj)
        return NULL;

    obj->setType(templateObject->type());

    return obj;
}

JSObject*
NewInitArray(JSContext *cx, uint32_t count, types::TypeObject *typeArg)
{
    RootedTypeObject type(cx, typeArg);
    NewObjectKind newKind = !type ? SingletonObject : GenericObject;
    RootedObject obj(cx, NewDenseAllocatedArray(cx, count, NULL, newKind));
    if (!obj)
        return NULL;

    if (!type)
        types::TypeScript::Monitor(cx, ObjectValue(*obj));
    else
        obj->setType(type);

    return obj;
}

JSObject*
NewInitObject(JSContext *cx, HandleObject templateObject)
{
    NewObjectKind newKind = templateObject->hasSingletonType() ? SingletonObject : GenericObject;
    RootedObject obj(cx, CopyInitializerObject(cx, templateObject, newKind));

    if (!obj)
        return NULL;

    if (templateObject->hasSingletonType())
        types::TypeScript::Monitor(cx, ObjectValue(*obj));
    else
        obj->setType(templateObject->type());

    return obj;
}

bool
ArrayPopDense(JSContext *cx, HandleObject obj, MutableHandleValue rval)
{
    JS_ASSERT(obj->isArray());

    AutoDetectInvalidation adi(cx, rval.address());

    Value argv[] = { UndefinedValue(), ObjectValue(*obj) };
    AutoValueArray ava(cx, argv, 2);
    if (!js::array_pop(cx, 0, argv))
        return false;

    
    
    rval.set(argv[0]);
    if (rval.isUndefined())
        types::TypeScript::Monitor(cx, rval);
    return true;
}

bool
ArrayPushDense(JSContext *cx, HandleObject obj, HandleValue v, uint32_t *length)
{
    JS_ASSERT(obj->isArray());

    Value argv[] = { UndefinedValue(), ObjectValue(*obj), v };
    AutoValueArray ava(cx, argv, 3);
    if (!js::array_push(cx, 1, argv))
        return false;

    *length = argv[0].toInt32();
    return true;
}

bool
ArrayShiftDense(JSContext *cx, HandleObject obj, MutableHandleValue rval)
{
    JS_ASSERT(obj->isArray());

    AutoDetectInvalidation adi(cx, rval.address());

    Value argv[] = { UndefinedValue(), ObjectValue(*obj) };
    AutoValueArray ava(cx, argv, 2);
    if (!js::array_shift(cx, 0, argv))
        return false;

    
    
    rval.set(argv[0]);
    if (rval.isUndefined())
        types::TypeScript::Monitor(cx, rval);
    return true;
}

JSObject *
ArrayConcatDense(JSContext *cx, HandleObject obj1, HandleObject obj2, HandleObject res)
{
    JS_ASSERT(obj1->isArray());
    JS_ASSERT(obj2->isArray());
    JS_ASSERT_IF(res, res->isArray());

    if (res) {
        
        if (!js::array_concat_dense(cx, obj1, obj2, res))
            return NULL;
        return res;
    }

    Value argv[] = { UndefinedValue(), ObjectValue(*obj1), ObjectValue(*obj2) };
    AutoValueArray ava(cx, argv, 3);
    if (!js::array_concat(cx, 1, argv))
        return NULL;
    return &argv[0].toObject();
}

bool
CharCodeAt(JSContext *cx, HandleString str, int32_t index, uint32_t *code)
{
    jschar c;
    if (!str->getChar(cx, index, &c))
        return false;
    *code = c;
    return true;
}

JSFlatString *
StringFromCharCode(JSContext *cx, int32_t code)
{
    jschar c = jschar(code);

    if (StaticStrings::hasUnit(c))
        return cx->runtime->staticStrings.getUnit(c);

    return js_NewStringCopyN<CanGC>(cx, &c, 1);
}

bool
SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, HandleValue value,
            bool strict, int jsop)
{
    RootedValue v(cx, value);
    RootedId id(cx, NameToId(name));

    if (jsop == JSOP_SETALIASEDVAR) {
        
        
        Shape *shape = obj->nativeLookup(cx, name);
        JS_ASSERT(shape && shape->hasSlot());
        JSObject::nativeSetSlotWithType(cx, obj, shape, value);
        return true;
    }

    if (JS_LIKELY(!obj->getOps()->setProperty)) {
        unsigned defineHow = (jsop == JSOP_SETNAME || jsop == JSOP_SETGNAME) ? DNP_UNQUALIFIED : 0;
        return baseops::SetPropertyHelper(cx, obj, obj, id, defineHow, &v, strict);
    }

    return JSObject::setGeneric(cx, obj, obj, id, &v, strict);
}

bool
InterruptCheck(JSContext *cx)
{
    gc::MaybeVerifyBarriers(cx);

    return !!js_HandleExecutionInterrupt(cx);
}

HeapSlot *
NewSlots(JSRuntime *rt, unsigned nslots)
{
    JS_STATIC_ASSERT(sizeof(Value) == sizeof(HeapSlot));

    Value *slots = reinterpret_cast<Value *>(rt->malloc_(nslots * sizeof(Value)));
    if (!slots)
        return NULL;

    for (unsigned i = 0; i < nslots; i++)
        slots[i] = UndefinedValue();

    return reinterpret_cast<HeapSlot *>(slots);
}

JSObject *
NewCallObject(JSContext *cx, HandleScript script,
              HandleShape shape, HandleTypeObject type, HeapSlot *slots)
{
    return CallObject::create(cx, script, shape, type, slots);
}

JSObject *
NewStringObject(JSContext *cx, HandleString str)
{
    return StringObject::create(cx, str);
}

bool
SPSEnter(JSContext *cx, HandleScript script)
{
    return cx->runtime->spsProfiler.enter(cx, script, script->function());
}

bool
SPSExit(JSContext *cx, HandleScript script)
{
    cx->runtime->spsProfiler.exit(cx, script, script->function());
    return true;
}

bool
OperatorIn(JSContext *cx, HandleValue key, HandleObject obj, JSBool *out)
{
    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, key, &id))
        return false;

    RootedObject obj2(cx);
    RootedShape prop(cx);
    if (!JSObject::lookupGeneric(cx, obj, id, &obj2, &prop))
        return false;

    *out = !!prop;
    return true;
}

bool
OperatorInI(JSContext *cx, uint32_t index, HandleObject obj, JSBool *out)
{
    RootedValue key(cx, Int32Value(index));
    return OperatorIn(cx, key, obj, out);
}

bool
GetIntrinsicValue(JSContext *cx, HandlePropertyName name, MutableHandleValue rval)
{
    if (!cx->global()->getIntrinsicValue(cx, name, rval))
        return false;

    
    
    
    
    
    types::TypeScript::Monitor(cx, rval);

    return true;
}

bool
CreateThis(JSContext *cx, HandleObject callee, MutableHandleValue rval)
{
    rval.set(MagicValue(JS_IS_CONSTRUCTING));

    if (callee->isFunction()) {
        JSFunction *fun = callee->toFunction();
        if (fun->isInterpreted()) {
            JSScript *script = fun->getOrCreateScript(cx);
            if (!script || !script->ensureHasTypes(cx))
                return false;
            JSObject *thisObj = CreateThisForFunction(cx, callee, false);
            if (!thisObj)
                return false;
            rval.set(ObjectValue(*thisObj));
        }
    }

    return true;
}

void
GetDynamicName(JSContext *cx, JSObject *scopeChain, JSString *str, Value *vp)
{
    
    
    

    JSAtom *atom;
    if (str->isAtom()) {
        atom = &str->asAtom();
    } else {
        atom = AtomizeString<NoGC>(cx, str);
        if (!atom) {
            vp->setUndefined();
            return;
        }
    }

    if (!frontend::IsIdentifier(atom) || frontend::IsKeyword(atom)) {
        vp->setUndefined();
        return;
    }

    Shape *shape = NULL;
    JSObject *scope = NULL, *pobj = NULL;
    if (LookupNameNoGC(cx, atom->asPropertyName(), scopeChain, &scope, &pobj, &shape)) {
        if (FetchNameNoGC(pobj, shape, MutableHandleValue::fromMarkedLocation(vp)))
            return;
    }

    vp->setUndefined();
}

JSBool
FilterArguments(JSContext *cx, JSString *str)
{
    
    
    
    
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return false;

    static jschar arguments[] = {'a', 'r', 'g', 'u', 'm', 'e', 'n', 't', 's'};
    return !StringHasPattern(chars, str->length(), arguments, mozilla::ArrayLength(arguments));
}

#ifdef JSGC_GENERATIONAL
void
PostWriteBarrier(JSRuntime *rt, JSObject *obj)
{
    JS_ASSERT(!IsInsideNursery(rt, obj));
    rt->gcStoreBuffer.putWholeObject(obj);
}
#endif

uint32_t
GetIndexFromString(JSString *str)
{
    
    
    

    if (!str->isAtom())
        return UINT32_MAX;

    uint32_t index;
    JSAtom *atom = &str->asAtom();
    if (!atom->isIndex(&index))
        return UINT32_MAX;

    return index;
}

bool
DebugPrologue(JSContext *cx, BaselineFrame *frame, JSBool *mustReturn)
{
    *mustReturn = false;

    JSTrapStatus status = ScriptDebugPrologue(cx, frame);
    switch (status) {
      case JSTRAP_CONTINUE:
        return true;

      case JSTRAP_RETURN:
        
        
        JS_ASSERT(frame->hasReturnValue());
        *mustReturn = true;
        return ion::DebugEpilogue(cx, frame, true);

      case JSTRAP_THROW:
      case JSTRAP_ERROR:
        return false;

      default:
        JS_NOT_REACHED("Invalid trap status");
    }
}

bool
DebugEpilogue(JSContext *cx, BaselineFrame *frame, JSBool ok)
{
    
    UnwindScope(cx, frame, 0);

    
    
    
    ok = ScriptDebugEpilogue(cx, frame, ok);

    if (frame->isNonEvalFunctionFrame()) {
        JS_ASSERT_IF(ok, frame->hasReturnValue());
        DebugScopes::onPopCall(frame, cx);
    } else if (frame->isStrictEvalFrame()) {
        JS_ASSERT_IF(frame->hasCallObj(), frame->scopeChain()->asCall().isForEval());
        DebugScopes::onPopStrictEvalScope(frame);
    }

    
    if (frame->hasPushedSPSFrame()) {
        cx->runtime->spsProfiler.exit(cx, frame->script(), frame->maybeFun());
        
        
        
        frame->unsetPushedSPSFrame();
    }

    if (!ok) {
        
        

        IonJSFrameLayout *prefix = frame->framePrefix();
        EnsureExitFrame(prefix);
        cx->mainThread().ionTop = (uint8_t *)prefix;
    }

    return ok;
}

bool
StrictEvalPrologue(JSContext *cx, BaselineFrame *frame)
{
    return frame->strictEvalPrologue(cx);
}

bool
HeavyweightFunPrologue(JSContext *cx, BaselineFrame *frame)
{
    return frame->heavyweightFunPrologue(cx);
}

bool
NewArgumentsObject(JSContext *cx, BaselineFrame *frame, MutableHandleValue res)
{
    ArgumentsObject *obj = ArgumentsObject::createExpected(cx, frame);
    if (!obj)
        return false;
    res.setObject(*obj);
    return true;
}

JSObject *
InitRestParameter(JSContext *cx, uint32_t length, Value *rest, HandleObject templateObj,
                  HandleObject res)
{
    if (res) {
        JS_ASSERT(res->isArray());
        JS_ASSERT(!res->getDenseInitializedLength());
        JS_ASSERT(res->type() == templateObj->type());

        
        
        if (length > 0) {
            if (!res->ensureElements(cx, length))
                return NULL;
            res->setDenseInitializedLength(length);
            res->initDenseElements(0, rest, length);
            res->setArrayLengthInt32(length);

            
            
            for (unsigned i = 0; i < length; i++)
                types::AddTypePropertyId(cx, res, JSID_VOID, rest[i]);
        }
        return res;
    }

    JSObject *obj = NewDenseCopiedArray(cx, length, rest, NULL);
    if (!obj)
        return NULL;
    obj->setType(templateObj->type());

    for (unsigned i = 0; i < length; i++)
        types::AddTypePropertyId(cx, obj, JSID_VOID, rest[i]);

    return obj;
}

bool
HandleDebugTrap(JSContext *cx, BaselineFrame *frame, uint8_t *retAddr, JSBool *mustReturn)
{
    *mustReturn = false;

    RootedScript script(cx, frame->script());
    jsbytecode *pc = script->baselineScript()->icEntryFromReturnAddress(retAddr).pc(script);

    JS_ASSERT(cx->compartment->debugMode());
    JS_ASSERT(script->stepModeEnabled() || script->hasBreakpointsAt(pc));

    RootedValue rval(cx);
    JSTrapStatus status = JSTRAP_CONTINUE;
    JSInterruptHook hook = cx->runtime->debugHooks.interruptHook;

    if (hook || script->stepModeEnabled()) {
        if (hook)
            status = hook(cx, script, pc, rval.address(), cx->runtime->debugHooks.interruptHookData);
        if (status == JSTRAP_CONTINUE && script->stepModeEnabled())
            status = Debugger::onSingleStep(cx, &rval);
    }

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
        return ion::DebugEpilogue(cx, frame, true);

      case JSTRAP_THROW:
        cx->setPendingException(rval);
        return false;

      default:
        JS_NOT_REACHED("Invalid trap status");
    }

    return true;
}

bool
OnDebuggerStatement(JSContext *cx, BaselineFrame *frame, jsbytecode *pc, JSBool *mustReturn)
{
    *mustReturn = false;

    RootedScript script(cx, frame->script());
    JSTrapStatus status = JSTRAP_CONTINUE;
    RootedValue rval(cx);

    if (JSDebuggerHandler handler = cx->runtime->debugHooks.debuggerHandler)
        status = handler(cx, script, pc, rval.address(), cx->runtime->debugHooks.debuggerHandlerData);

    if (status == JSTRAP_CONTINUE)
        status = Debugger::onDebuggerStatement(cx, &rval);

    switch (status) {
      case JSTRAP_ERROR:
        return false;

      case JSTRAP_CONTINUE:
        return true;

      case JSTRAP_RETURN:
        frame->setReturnValue(rval);
        *mustReturn = true;
        return ion::DebugEpilogue(cx, frame, true);

      case JSTRAP_THROW:
        cx->setPendingException(rval);
        return false;

      default:
        JS_NOT_REACHED("Invalid trap status");
    }
}

bool
EnterBlock(JSContext *cx, BaselineFrame *frame, Handle<StaticBlockObject *> block)
{
    return frame->pushBlock(cx, block);
}

bool
LeaveBlock(JSContext *cx, BaselineFrame *frame)
{
    frame->popBlock(cx);
    return true;
}

bool
InitBaselineFrameForOsr(BaselineFrame *frame, StackFrame *interpFrame, uint32_t numStackValues)
{
    return frame->initForOsr(interpFrame, numStackValues);
}

} 
} 
