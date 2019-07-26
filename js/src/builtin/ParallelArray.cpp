






#include "jsapi.h"
#include "jsobj.h"
#include "jsarray.h"

#include "builtin/ParallelArray.h"

#include "vm/ForkJoin.h"
#include "vm/GlobalObject.h"
#include "vm/String.h"
#include "vm/ThreadPool.h"

#include "jsinterpinlines.h"
#include "jsobjinlines.h"

using namespace js;





FixedHeapPtr<PropertyName> ParallelArrayObject::ctorNames[NumCtors];

JSFunctionSpec ParallelArrayObject::methods[] = {
    { "map",       JSOP_NULLWRAPPER, 2, 0, "ParallelArrayMap"       },
    { "reduce",    JSOP_NULLWRAPPER, 2, 0, "ParallelArrayReduce"    },
    { "scan",      JSOP_NULLWRAPPER, 2, 0, "ParallelArrayScan"      },
    { "scatter",   JSOP_NULLWRAPPER, 5, 0, "ParallelArrayScatter"   },
    { "filter",    JSOP_NULLWRAPPER, 2, 0, "ParallelArrayFilter"    },
    { "partition", JSOP_NULLWRAPPER, 1, 0, "ParallelArrayPartition" },
    { "flatten",   JSOP_NULLWRAPPER, 0, 0, "ParallelArrayFlatten" },

    
    
    
    
    
    
    
    

    { "toString", JSOP_NULLWRAPPER, 0, 0, "ParallelArrayToString" },
    JS_FS_END
};

Class ParallelArrayObject::protoClass = {
    "ParallelArray",
    JSCLASS_HAS_CACHED_PROTO(JSProto_ParallelArray),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

Class ParallelArrayObject::class_ = {
    "ParallelArray",
    JSCLASS_HAS_CACHED_PROTO(JSProto_ParallelArray),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

 bool
ParallelArrayObject::initProps(JSContext *cx, HandleObject obj)
{
    RootedValue undef(cx, UndefinedValue());
    RootedValue zero(cx, Int32Value(0));

    if (!JSObject::defineProperty(cx, obj, cx->names().buffer, undef))
        return false;
    if (!JSObject::defineProperty(cx, obj, cx->names().offset, zero))
        return false;
    if (!JSObject::defineProperty(cx, obj, cx->names().shape, undef))
        return false;
    if (!JSObject::defineProperty(cx, obj, cx->names().get, undef))
        return false;

    return true;
}

 JSBool
ParallelArrayObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    RootedFunction ctor(cx, getConstructor(cx, argc));
    if (!ctor)
        return false;
    CallArgs args = CallArgsFromVp(argc, vp);
    return constructHelper(cx, &ctor, args);
}


 JSFunction *
ParallelArrayObject::getConstructor(JSContext *cx, unsigned argc)
{
    RootedPropertyName ctorName(cx, ctorNames[js::Min(argc, NumCtors - 1)]);
    RootedValue ctorValue(cx);
    if (!cx->global()->getIntrinsicValue(cx, ctorName, &ctorValue))
        return NULL;
    JS_ASSERT(ctorValue.isObject() && ctorValue.toObject().isFunction());
    return ctorValue.toObject().toFunction();
}

 JSObject *
ParallelArrayObject::newInstance(JSContext *cx)
{
    gc::AllocKind kind = gc::GetGCObjectKind(NumFixedSlots);
    RootedObject result(cx, NewBuiltinClassInstance(cx, &class_, kind));
    if (!result)
        return NULL;

    
    if (!initProps(cx, result))
        return NULL;

    return result;
}

 JSBool
ParallelArrayObject::constructHelper(JSContext *cx, MutableHandleFunction ctor, CallArgs &args0)
{
    RootedObject result(cx, newInstance(cx));
    if (!result)
        return false;

    if (cx->typeInferenceEnabled()) {
        jsbytecode *pc;
        RootedScript script(cx, cx->stack.currentScript(&pc));
        if (script) {
            if (ctor->nonLazyScript()->shouldCloneAtCallsite) {
                ctor.set(CloneFunctionAtCallsite(cx, ctor, script, pc));
                if (!ctor)
                    return false;
            }

            
            
            
            
            
            
            types::TypeObject *paTypeObject =
                types::TypeScript::InitObject(cx, script, pc, JSProto_ParallelArray);
            if (!paTypeObject)
                return false;
            if (paTypeObject->getPropertyCount() == 0) {
                if (!paTypeObject->addDefiniteProperties(cx, result))
                    return false;

                
                
                JS_ASSERT(paTypeObject->getPropertyCount() == NumFixedSlots);
            }
            result->setType(paTypeObject);
        }
    }

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, args0.length(), &args))
        return false;

    args.setCallee(ObjectValue(*ctor));
    args.setThis(ObjectValue(*result));

    for (uint32_t i = 0; i < args0.length(); i++)
        args[i] = args0[i];

    if (!Invoke(cx, args))
        return false;

    args0.rval().setObject(*result);
    return true;
}

JSObject *
ParallelArrayObject::initClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    
    {
        const char *ctorStrs[NumCtors] = { "ParallelArrayConstructEmpty",
                                           "ParallelArrayConstructFromArray",
                                           "ParallelArrayConstructFromFunction",
                                           "ParallelArrayConstructFromFunctionMode" };
        for (uint32_t i = 0; i < NumCtors; i++) {
            JSAtom *atom = Atomize(cx, ctorStrs[i], strlen(ctorStrs[i]), InternAtom);
            if (!atom)
                return NULL;
            ctorNames[i].init(atom->asPropertyName());
        }
    }

    Rooted<GlobalObject *> global(cx, &obj->asGlobal());

    RootedObject proto(cx, global->createBlankPrototype(cx, &protoClass));
    if (!proto)
        return NULL;

    JSProtoKey key = JSProto_ParallelArray;
    RootedFunction ctor(cx, global->createConstructor(cx, construct,
                                                      cx->names().ParallelArray, 0));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto, NULL, methods) ||
        !DefineConstructorAndPrototype(cx, global, key, ctor, proto))
    {
        return NULL;
    }

    
    {
        const char lengthStr[] = "ParallelArrayLength";
        JSAtom *atom = Atomize(cx, lengthStr, strlen(lengthStr));
        if (!atom)
            return NULL;
        Rooted<PropertyName *> lengthProp(cx, atom->asPropertyName());
        RootedValue lengthValue(cx);
        if (!cx->global()->getIntrinsicValue(cx, lengthProp, &lengthValue))
            return NULL;
        RootedObject lengthGetter(cx, &lengthValue.toObject());
        if (!lengthGetter)
            return NULL;

        RootedId lengthId(cx, AtomToId(cx->names().length));
        unsigned flags = JSPROP_PERMANENT | JSPROP_SHARED | JSPROP_GETTER;
        RootedValue value(cx, UndefinedValue());
        if (!DefineNativeProperty(cx, proto, lengthId, value,
                                  JS_DATA_TO_FUNC_PTR(PropertyOp, lengthGetter.get()), NULL,
                                  flags, 0, 0))
        {
            return NULL;
        }
    }

    return proto;
}

bool
ParallelArrayObject::is(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&class_);
}

JSObject *
js_InitParallelArrayClass(JSContext *cx, js::HandleObject obj)
{
    return ParallelArrayObject::initClass(cx, obj);
}
