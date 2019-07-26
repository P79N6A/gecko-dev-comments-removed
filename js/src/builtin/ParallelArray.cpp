





#include "builtin/ParallelArray.h"

#include "jsapi.h"
#include "jsobj.h"

#include "vm/GlobalObject.h"
#include "vm/String.h"

#include "jsobjinlines.h"

using namespace js;





FixedHeapPtr<PropertyName> ParallelArrayObject::ctorNames[NumCtors];

const JSFunctionSpec ParallelArrayObject::methods[] = {
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

const Class ParallelArrayObject::protoClass = {
    "ParallelArray",
    JSCLASS_HAS_CACHED_PROTO(JSProto_ParallelArray),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

const Class ParallelArrayObject::class_ = {
    "ParallelArray",
    JSCLASS_HAS_CACHED_PROTO(JSProto_ParallelArray),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
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

 bool
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
        return nullptr;
    JS_ASSERT(ctorValue.isObject() && ctorValue.toObject().is<JSFunction>());
    return &ctorValue.toObject().as<JSFunction>();
}

 JSObject *
ParallelArrayObject::newInstance(JSContext *cx, NewObjectKind newKind )
{
    gc::AllocKind kind = gc::GetGCObjectKind(NumFixedSlots);
    RootedObject result(cx, NewBuiltinClassInstance(cx, &class_, kind, newKind));
    if (!result)
        return nullptr;

    
    if (!initProps(cx, result))
        return nullptr;

    return result;
}

 bool
ParallelArrayObject::constructHelper(JSContext *cx, MutableHandleFunction ctor, CallArgs &args0)
{
    RootedObject result(cx, newInstance(cx, TenuredObject));
    if (!result)
        return false;

    if (cx->typeInferenceEnabled()) {
        jsbytecode *pc;
        RootedScript script(cx, cx->currentScript(&pc));
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
            if (paTypeObject->getPropertyCount() == 0 && !paTypeObject->unknownProperties()) {
                if (!paTypeObject->addDefiniteProperties(cx, result))
                    return false;

                
                
                JS_ASSERT(paTypeObject->getPropertyCount() == NumFixedSlots);
            }
            result->setType(paTypeObject);
        }
    }

    InvokeArgs args(cx);
    if (!args.init(args0.length()))
        return false;

    args.setCallee(ObjectValue(*ctor));
    args.setThis(ObjectValue(*result));

    for (uint32_t i = 0; i < args0.length(); i++)
        args[i].set(args0[i]);

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
        static const char *const ctorStrs[NumCtors] = {
            "ParallelArrayConstructEmpty",
            "ParallelArrayConstructFromArray",
            "ParallelArrayConstructFromFunction",
            "ParallelArrayConstructFromFunctionMode"
        };
        for (uint32_t i = 0; i < NumCtors; i++) {
            JSAtom *atom = Atomize(cx, ctorStrs[i], strlen(ctorStrs[i]), InternAtom);
            if (!atom)
                return nullptr;
            ctorNames[i].init(atom->asPropertyName());
        }
    }

    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());

    RootedObject proto(cx, global->createBlankPrototype(cx, &protoClass));
    if (!proto)
        return nullptr;

    JSProtoKey key = JSProto_ParallelArray;
    RootedFunction ctor(cx, global->createConstructor(cx, construct,
                                                      cx->names().ParallelArray, 0));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto, nullptr, methods) ||
        !DefineConstructorAndPrototype(cx, global, key, ctor, proto))
    {
        return nullptr;
    }

    
    {
        const char lengthStr[] = "ParallelArrayLength";
        JSAtom *atom = Atomize(cx, lengthStr, strlen(lengthStr));
        if (!atom)
            return nullptr;
        Rooted<PropertyName *> lengthProp(cx, atom->asPropertyName());
        RootedValue lengthValue(cx);
        if (!cx->global()->getIntrinsicValue(cx, lengthProp, &lengthValue))
            return nullptr;
        RootedObject lengthGetter(cx, &lengthValue.toObject());
        if (!lengthGetter)
            return nullptr;

        RootedId lengthId(cx, AtomToId(cx->names().length));
        unsigned flags = JSPROP_PERMANENT | JSPROP_SHARED | JSPROP_GETTER;
        RootedValue value(cx, UndefinedValue());
        if (!DefineNativeProperty(cx, proto, lengthId, value,
                                  JS_DATA_TO_FUNC_PTR(PropertyOp, lengthGetter.get()), nullptr,
                                  flags, 0, 0))
        {
            return nullptr;
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
