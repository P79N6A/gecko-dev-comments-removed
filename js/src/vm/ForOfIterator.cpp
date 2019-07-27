





#include "jsapi.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsobj.h"

#include "vm/Interpreter.h"
#include "vm/PIC.h"

#include "jsobjinlines.h"

using namespace js;
using JS::ForOfIterator;

bool
ForOfIterator::init(HandleValue iterable, NonIterableBehavior nonIterableBehavior)
{
    JSContext *cx = cx_;
    RootedObject iterableObj(cx, ToObject(cx, iterable));
    if (!iterableObj)
        return false;

    MOZ_ASSERT(index == NOT_ARRAY);

    
    if (iterableObj->is<ArrayObject>()) {
        ForOfPIC::Chain *stubChain = ForOfPIC::getOrCreate(cx);
        if (!stubChain)
            return false;

        bool optimized;
        if (!stubChain->tryOptimizeArray(cx, iterableObj.as<ArrayObject>(), &optimized))
            return false;

        if (optimized) {
            
            index = 0;
            iterator = iterableObj;
            return true;
        }
    }

    MOZ_ASSERT(index == NOT_ARRAY);

    
    InvokeArgs args(cx);
    if (!args.init(0))
        return false;
    args.setThis(ObjectValue(*iterableObj));

    RootedValue callee(cx);
    RootedId iteratorId(cx, SYMBOL_TO_JSID(cx->wellKnownSymbols().iterator));
    if (!GetProperty(cx, iterableObj, iterableObj, iteratorId, &callee))
        return false;

    
    
    
    if (nonIterableBehavior == AllowNonIterable && callee.isUndefined())
        return true;

    
    
    
    
    if (!callee.isObject() || !callee.toObject().isCallable()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, iterable, NullPtr());
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_ITERABLE, bytes);
        js_free(bytes);
        return false;
    }

    args.setCallee(callee);
    if (!Invoke(cx, args))
        return false;

    iterator = ToObject(cx, args.rval());
    if (!iterator)
        return false;

    return true;
}

bool
ForOfIterator::initWithIterator(HandleValue aIterator)
{
    JSContext *cx = cx_;
    RootedObject iteratorObj(cx, ToObject(cx, aIterator));
    return iterator = iteratorObj;
}

inline bool
ForOfIterator::nextFromOptimizedArray(MutableHandleValue vp, bool *done)
{
    MOZ_ASSERT(index != NOT_ARRAY);

    if (!CheckForInterrupt(cx_))
        return false;

    ArrayObject *arr = &iterator->as<ArrayObject>();

    if (index >= arr->length()) {
        vp.setUndefined();
        *done = true;
        return true;
    }
    *done = false;

    
    if (index < arr->getDenseInitializedLength()) {
        vp.set(arr->getDenseElement(index));
        if (!vp.isMagic(JS_ELEMENTS_HOLE)) {
            ++index;
            return true;
        }
    }

    return GetElement(cx_, iterator, iterator, index++, vp);
}

bool
ForOfIterator::next(MutableHandleValue vp, bool *done)
{
    MOZ_ASSERT(iterator);

    if (index != NOT_ARRAY) {
        ForOfPIC::Chain *stubChain = ForOfPIC::getOrCreate(cx_);
        if (!stubChain)
            return false;

        if (stubChain->isArrayNextStillSane())
            return nextFromOptimizedArray(vp, done);

        
        
        if (!materializeArrayIterator())
            return false;
    }

    RootedValue method(cx_);
    if (!GetProperty(cx_, iterator, iterator, cx_->names().next, &method))
        return false;

    InvokeArgs args(cx_);
    if (!args.init(1))
        return false;
    args.setCallee(method);
    args.setThis(ObjectValue(*iterator));
    args[0].setUndefined();
    if (!Invoke(cx_, args))
        return false;

    RootedObject resultObj(cx_, ToObject(cx_, args.rval()));
    if (!resultObj)
        return false;
    RootedValue doneVal(cx_);
    if (!GetProperty(cx_, resultObj, resultObj, cx_->names().done, &doneVal))
        return false;
    *done = ToBoolean(doneVal);
    if (*done) {
        vp.setUndefined();
        return true;
    }
    return GetProperty(cx_, resultObj, resultObj, cx_->names().value, vp);
}

bool
ForOfIterator::materializeArrayIterator()
{
    MOZ_ASSERT(index != NOT_ARRAY);

    const char *nameString = "ArrayValuesAt";

    RootedAtom name(cx_, Atomize(cx_, nameString, strlen(nameString)));
    if (!name)
        return false;

    RootedValue val(cx_);
    if (!cx_->global()->getSelfHostedFunction(cx_, name, name, 1, &val))
        return false;

    InvokeArgs args(cx_);
    if (!args.init(1))
        return false;
    args.setCallee(val);
    args.setThis(ObjectValue(*iterator));
    args[0].set(Int32Value(index));
    if (!Invoke(cx_, args))
        return false;

    index = NOT_ARRAY;
    
    iterator = &args.rval().toObject();
    return true;
}
