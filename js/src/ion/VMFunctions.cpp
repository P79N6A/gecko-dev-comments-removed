







































#include "Ion.h"
#include "IonCompartment.h"
#include "jsinterp.h"
#include "ion/IonFrames.h"

#include "jsinterpinlines.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {

bool
InvokeFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval)
{
    Value fval = ObjectValue(*fun);

    
    Value thisv = argv[0];
    Value *argvWithoutThis = argv + 1;

    
    bool ok = Invoke(cx, thisv, fval, argc, argvWithoutThis, rval);
    return ok;
}

bool
InvokeConstructorFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval)
{
    Value fval = ObjectValue(*fun);

    
    Value *argvWithoutThis = argv + 1;

    bool ok = InvokeConstructor(cx, fval, argc, argvWithoutThis, rval);
    return ok;
}

bool
ReportOverRecursed(JSContext *cx)
{
    js_ReportOverRecursed(cx);

    
    return false;
}

bool
DefVarOrConst(JSContext *cx, PropertyName *dn, unsigned attrs, JSObject *scopeChain)
{
    
    JSObject *obj = scopeChain;
    while (!obj->isVarObj())
        obj = obj->enclosingScope();

    return DefVarOrConstOperation(cx, *obj, dn, attrs);
}

template<bool Equal>
bool
LooselyEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res)
{
    bool equal;
    if (!js::LooselyEqual(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool LooselyEqual<true>(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);
template bool LooselyEqual<false>(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);

template<bool Equal>
bool
StrictlyEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res)
{
    bool equal;
    if (!js::StrictlyEqual(cx, lhs, rhs, &equal))
        return false;
    *res = (equal == Equal);
    return true;
}

template bool StrictlyEqual<true>(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);
template bool StrictlyEqual<false>(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res);

bool
LessThan(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res)
{
    bool cond;
    if (!LessThanOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
LessThanOrEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res)
{
    bool cond;
    if (!LessThanOrEqualOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
GreaterThan(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res)
{
    bool cond;
    if (!GreaterThanOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
GreaterThanOrEqual(JSContext *cx, const Value &lhs, const Value &rhs, JSBool *res)
{
    bool cond;
    if (!GreaterThanOrEqualOperation(cx, lhs, rhs, &cond))
        return false;
    *res = cond;
    return true;
}

bool
ValueToBooleanComplement(JSContext *cx, const Value &input, JSBool *output)
{
    *output = !js_ValueToBoolean(input);
    return true;
}

bool
IteratorMore(JSContext *cx, JSObject *obj, JSBool *res)
{
    Value tmp;
    if (!js_IteratorMore(cx, obj, &tmp))
        return false;

    *res = tmp.toBoolean();
    return true;
}

bool
CloseIteratorFromIon(JSContext *cx, JSObject *obj)
{
    bool result = CloseIterator(cx, obj);

    
    
    IonActivation *ion = cx->runtime->ionActivation;
    if (obj == ion->savedEnumerators())
        ion->updateSavedEnumerators(cx->enumerators);

    return result;
}

JSObject*
NewInitArray(JSContext *cx, uint32_t count, types::TypeObject *type)
{
    JSObject *obj = NewDenseAllocatedArray(cx, count);
    if (!obj)
        return NULL;

    if (!type) {
        if (!obj->setSingletonType(cx))
            return NULL;

        types::TypeScript::Monitor(cx, ObjectValue(*obj));
    } else {
        obj->setType(type);
    }

    return obj;
}

JSObject*
NewInitObject(JSContext *cx, JSObject *baseObj, types::TypeObject *type)
{
    JSObject *obj = CopyInitializerObject(cx, baseObj);
    if (!obj)
        return NULL;

    if (!type) {
        if (!obj->setSingletonType(cx))
            return NULL;

        types::TypeScript::Monitor(cx, ObjectValue(*obj));
    } else {
        obj->setType(type);
    }

    return obj;
}

} 
} 

