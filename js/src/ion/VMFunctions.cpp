






#include "Ion.h"
#include "IonCompartment.h"
#include "jsinterp.h"
#include "ion/IonFrames.h"
#include "ion/IonFrames-inl.h" 

#include "vm/StringObject-inl.h"

#include "jsboolinlines.h"
#include "jsinterpinlines.h"

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

static inline bool
ShouldMonitorReturnType(JSFunction *fun)
{
    return fun->isInterpreted() &&
           (!fun->nonLazyScript()->hasAnalysis() ||
            !fun->nonLazyScript()->analysis()->ranInference());
}

bool
InvokeFunction(JSContext *cx, HandleFunction fun, uint32_t argc, Value *argv, Value *rval)
{
    AssertCanGC();

    
    
    if (fun->isInterpreted()) {
        if (fun->isInterpretedLazy() && !JSFunction::getOrCreateScript(cx, fun))
            return false;
        if (!fun->nonLazyScript()->canIonCompile()) {
            UnrootedScript script = GetTopIonJSScript(cx);
            if (script->hasIonScript() &&
                ++script->ion->slowCallCount >= js_IonOptions.slowCallLimit)
            {
                AutoFlushCache afc("InvokeFunction");

                
                
                ForbidCompilation(cx, script);
            }
        }

        
        
        
        fun->nonLazyScript()->incUseCount(js_IonOptions.slowCallIncUseCount);
    }

    
    
    
    
    
    bool needsMonitor = ShouldMonitorReturnType(fun);

    
    Value thisv = argv[0];
    Value *argvWithoutThis = argv + 1;

    
    bool ok = Invoke(cx, thisv, ObjectValue(*fun), argc, argvWithoutThis, rval);
    if (ok && needsMonitor)
        types::TypeScript::Monitor(cx, *rval);

    return ok;
}

JSObject *
NewGCThing(JSContext *cx, gc::AllocKind allocKind, size_t thingSize)
{
    return gc::NewGCThing<JSObject>(cx, allocKind, thingSize);
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
LooselyEqual(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res)
{
    bool equal;
    if (!js::LooselyEqual(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool LooselyEqual<true>(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res);
template bool LooselyEqual<false>(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res);

template<bool Equal>
bool
StrictlyEqual(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res)
{
    bool equal;
    if (!js::StrictlyEqual(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool StrictlyEqual<true>(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res);
template bool StrictlyEqual<false>(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res);

bool
LessThan(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res)
{
    bool cond;
    if (!LessThanOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
LessThanOrEqual(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res)
{
    bool cond;
    if (!LessThanOrEqualOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
GreaterThan(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res)
{
    bool cond;
    if (!GreaterThanOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
GreaterThanOrEqual(JSContext *cx, HandleValue lhs, HandleValue rhs, JSBool *res)
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
ObjectEmulatesUndefined(RawObject obj)
{
    AutoAssertNoGC nogc;
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

JSObject*
NewInitArray(JSContext *cx, uint32_t count, types::TypeObject *typeArg)
{
    RootedTypeObject type(cx, typeArg);
    RootedObject obj(cx, NewDenseAllocatedArray(cx, count));
    if (!obj)
        return NULL;

    if (!type) {
        if (!JSObject::setSingletonType(cx, obj))
            return NULL;

        types::TypeScript::Monitor(cx, ObjectValue(*obj));
    } else {
        obj->setType(type);
    }

    return obj;
}

JSObject*
NewInitObject(JSContext *cx, HandleObject templateObject)
{
    RootedObject obj(cx, CopyInitializerObject(cx, templateObject));

    if (!obj)
        return NULL;

    if (templateObject->hasSingletonType()) {
        if (!JSObject::setSingletonType(cx, obj))
            return NULL;

        types::TypeScript::Monitor(cx, ObjectValue(*obj));
    } else {
        obj->setType(templateObject->type());
    }

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
    JS_ASSERT(index >= 0 &&
              static_cast<uint32_t>(index) < str->length());

    const jschar *chars = str->getChars(cx);
    if (!chars)
        return false;

    *code = chars[index];
    return true;
}

JSFlatString *
StringFromCharCode(JSContext *cx, int32_t code)
{
    jschar c = jschar(code);

    if (StaticStrings::hasUnit(c))
        return cx->runtime->staticStrings.getUnit(c);

    return js_NewStringCopyN(cx, &c, 1);

}

bool
SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, HandleValue value,
            bool strict, bool isSetName)
{
    RootedValue v(cx, value);
    RootedId id(cx, NameToId(name));

    if (JS_LIKELY(!obj->getOps()->setProperty)) {
        unsigned defineHow = isSetName ? DNP_UNQUALIFIED : 0;
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
NewCallObject(JSContext *cx, HandleShape shape, HandleTypeObject type, HeapSlot *slots)
{
    return CallObject::create(cx, shape, type, slots);
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
    RootedValue dummy(cx); 
    RootedId id(cx);
    if (!FetchElementId(cx, obj, key, &id, &dummy))
        return false;

    RootedObject obj2(cx);
    RootedShape prop(cx);
    if (!JSObject::lookupGeneric(cx, obj, id, &obj2, &prop))
        return false;

    *out = !!prop;
    return true;
}

bool
GetIntrinsicValue(JSContext *cx, HandlePropertyName name, MutableHandleValue rval)
{
    return cx->global()->getIntrinsicValue(cx, name, rval);
}

bool
CreateThis(JSContext *cx, HandleObject callee, MutableHandleValue rval)
{
    rval.set(MagicValue(JS_IS_CONSTRUCTING));

    if (callee->isFunction()) {
        JSFunction *fun = callee->toFunction();
        if (fun->isInterpreted())
            rval.set(ObjectValue(*js_CreateThisForFunction(cx, callee, false)));
    }

    return true;
}

} 
} 
