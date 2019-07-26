





#include "vm/GlobalObject.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfriendapi.h"
#include "jsmath.h"
#include "json.h"
#include "jsweakmap.h"

#include "builtin/Eval.h"
#if EXPOSE_INTL_API
# include "builtin/Intl.h"
#endif
#include "builtin/MapObject.h"
#include "builtin/Object.h"
#include "builtin/RegExp.h"
#include "vm/RegExpStatics.h"

#include "jscompartmentinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/ObjectImpl-inl.h"

using namespace js;

JSObject *
js_InitObjectClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    return obj->as<GlobalObject>().getOrCreateObjectPrototype(cx);
}

JSObject *
js_InitFunctionClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    return obj->as<GlobalObject>().getOrCreateFunctionPrototype(cx);
}

static bool
ThrowTypeError(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, nullptr,
                                 JSMSG_THROW_TYPE_ERROR);
    return false;
}

static bool
TestProtoGetterThis(HandleValue v)
{
    return !v.isNullOrUndefined();
}

static bool
ProtoGetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(TestProtoGetterThis(args.thisv()));

    HandleValue thisv = args.thisv();
    if (thisv.isPrimitive() && !BoxNonStrictThis(cx, args))
        return false;

    unsigned dummy;
    RootedObject obj(cx, &args.thisv().toObject());
    RootedId nid(cx, NameToId(cx->names().proto));
    RootedValue v(cx);
    if (!CheckAccess(cx, obj, nid, JSACC_PROTO, &v, &dummy))
        return false;

    args.rval().set(v);
    return true;
}

static bool
ProtoGetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, TestProtoGetterThis, ProtoGetterImpl, args);
}

namespace js {
size_t sSetProtoCalled = 0;
} 

static bool
TestProtoSetterThis(HandleValue v)
{
    if (v.isNullOrUndefined())
        return false;

    
    if (!v.isObject())
        return true;

    
    return !v.toObject().is<ProxyObject>();
}

static bool
ProtoSetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(TestProtoSetterThis(args.thisv()));

    HandleValue thisv = args.thisv();
    if (thisv.isPrimitive()) {
        JS_ASSERT(!thisv.isNullOrUndefined());

        
        args.rval().setUndefined();
        return true;
    }

    if (!cx->runningWithTrustedPrincipals())
        ++sSetProtoCalled;

    Rooted<JSObject*> obj(cx, &args.thisv().toObject());

    
    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return false;
    if (!extensible) {
        obj->reportNotExtensible(cx);
        return false;
    }

    





    if (obj->is<ProxyObject>() || obj->is<ArrayBufferObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Object", "__proto__ setter",
                             obj->is<ProxyObject>() ? "Proxy" : "ArrayBuffer");
        return false;
    }

    
    if (args.length() == 0 || !args[0].isObjectOrNull()) {
        args.rval().setUndefined();
        return true;
    }

    Rooted<JSObject*> newProto(cx, args[0].toObjectOrNull());

    unsigned dummy;
    RootedId nid(cx, NameToId(cx->names().proto));
    RootedValue v(cx);
    if (!CheckAccess(cx, obj, nid, JSAccessMode(JSACC_PROTO | JSACC_WRITE), &v, &dummy))
        return false;

    if (!SetClassAndProto(cx, obj, obj->getClass(), newProto, true))
        return false;

    args.rval().setUndefined();
    return true;
}

static bool
ProtoSetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, TestProtoSetterThis, ProtoSetterImpl, args);
}

JSObject *
GlobalObject::initFunctionAndObjectClasses(JSContext *cx)
{
    Rooted<GlobalObject*> self(cx, this);

    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    JS_ASSERT(isNative());

    cx->setDefaultCompartmentObjectIfUnset(self);

    RootedObject objectProto(cx);

    



    objectProto = NewObjectWithGivenProto(cx, &JSObject::class_, nullptr, self, SingletonObject);
    if (!objectProto)
        return nullptr;

    




    if (!setNewTypeUnknown(cx, &JSObject::class_, objectProto))
        return nullptr;

    
    RootedFunction functionProto(cx);
    {
        JSObject *functionProto_ = NewObjectWithGivenProto(cx, &JSFunction::class_,
                                                           objectProto, self, SingletonObject);
        if (!functionProto_)
            return nullptr;
        functionProto = &functionProto_->as<JSFunction>();

        



        {
            JSObject *proto = NewFunction(cx, functionProto, nullptr, 0, JSFunction::INTERPRETED,
                                          self, NullPtr());
            if (!proto)
                return nullptr;
            JS_ASSERT(proto == functionProto);
            functionProto->setIsFunctionPrototype();
        }

        const char *rawSource = "() {\n}";
        size_t sourceLen = strlen(rawSource);
        jschar *source = InflateString(cx, rawSource, &sourceLen);
        if (!source)
            return nullptr;
        ScriptSource *ss =
            cx->new_<ScriptSource>( (JSPrincipals*)nullptr);
        if (!ss) {
            js_free(source);
            return nullptr;
        }
        RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss));
        if (!sourceObject)
            return nullptr;
        ss->setSource(source, sourceLen);

        CompileOptions options(cx);
        options.setNoScriptRval(true)
               .setVersion(JSVERSION_DEFAULT);

        RootedScript script(cx, JSScript::Create(cx,
                                                  NullPtr(),
                                                  false,
                                                 options,
                                                  0,
                                                 sourceObject,
                                                 0,
                                                 ss->length()));
        if (!script || !JSScript::fullyInitTrivial(cx, script))
            return nullptr;

        functionProto->initScript(script);
        types::TypeObject* protoType = functionProto->getType(cx);
        if (!protoType)
            return nullptr;
        protoType->interpretedFunction = functionProto;
        script->setFunction(functionProto);

        




        if (!setNewTypeUnknown(cx, &JSFunction::class_, functionProto))
            return nullptr;
    }

    
    RootedFunction objectCtor(cx);
    {
        RootedObject ctor(cx, NewObjectWithGivenProto(cx, &JSFunction::class_, functionProto,
                                                      self, SingletonObject));
        if (!ctor)
            return nullptr;
        RootedAtom objectAtom(cx, cx->names().Object);
        objectCtor = NewFunction(cx, ctor, obj_construct, 1, JSFunction::NATIVE_CTOR, self,
                                 objectAtom);
        if (!objectCtor)
            return nullptr;
    }

    



    self->setObjectClassDetails(objectCtor, objectProto);

    
    RootedFunction functionCtor(cx);
    {
        
        RootedObject ctor(cx, NewObjectWithGivenProto(cx, &JSFunction::class_, functionProto,
                                                      self, SingletonObject));
        if (!ctor)
            return nullptr;
        RootedAtom functionAtom(cx, cx->names().Function);
        functionCtor = NewFunction(cx, ctor, Function, 1, JSFunction::NATIVE_CTOR, self,
                                   functionAtom);
        if (!functionCtor)
            return nullptr;
        JS_ASSERT(ctor == functionCtor);
    }

    



    self->setFunctionClassDetails(functionCtor, functionProto);

    



    if (!LinkConstructorAndPrototype(cx, objectCtor, objectProto) ||
        !DefinePropertiesAndBrand(cx, objectProto, nullptr, object_methods))
    {
        return nullptr;
    }

    





    RootedFunction getter(cx, NewFunction(cx, NullPtr(), ProtoGetter, 0, JSFunction::NATIVE_FUN,
                                          self, NullPtr()));
    if (!getter)
        return nullptr;
#if JS_HAS_OBJ_PROTO_PROP
    RootedFunction setter(cx, NewFunction(cx, NullPtr(), ProtoSetter, 0, JSFunction::NATIVE_FUN,
                                          self, NullPtr()));
    if (!setter)
        return nullptr;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!JSObject::defineProperty(cx, objectProto,
                                  cx->names().proto, undefinedValue,
                                  JS_DATA_TO_FUNC_PTR(PropertyOp, getter.get()),
                                  JS_DATA_TO_FUNC_PTR(StrictPropertyOp, setter.get()),
                                  JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED))
    {
        return nullptr;
    }
#endif 
    self->setProtoGetter(getter);


    if (!DefinePropertiesAndBrand(cx, objectCtor, nullptr, object_static_methods) ||
        !LinkConstructorAndPrototype(cx, functionCtor, functionProto) ||
        !DefinePropertiesAndBrand(cx, functionProto, nullptr, function_methods) ||
        !DefinePropertiesAndBrand(cx, functionCtor, nullptr, nullptr))
    {
        return nullptr;
    }

    
    if (!self->addDataProperty(cx, cx->names().Object, JSProto_Object + JSProto_LIMIT * 2, 0))
        return nullptr;
    if (!self->addDataProperty(cx, cx->names().Function, JSProto_Function + JSProto_LIMIT * 2, 0))
        return nullptr;

    

    
    RootedId evalId(cx, NameToId(cx->names().eval));
    JSObject *evalobj = DefineFunction(cx, self, evalId, IndirectEval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return nullptr;
    self->setOriginalEval(evalobj);

    
    RootedFunction throwTypeError(cx, NewFunction(cx, NullPtr(), ThrowTypeError, 0,
                                                  JSFunction::NATIVE_FUN, self, NullPtr()));
    if (!throwTypeError)
        return nullptr;
    if (!JSObject::preventExtensions(cx, throwTypeError))
        return nullptr;
    self->setThrowTypeError(throwTypeError);

    RootedObject intrinsicsHolder(cx);
    if (cx->runtime()->isSelfHostingGlobal(self)) {
        intrinsicsHolder = self;
    } else {
        intrinsicsHolder = NewObjectWithClassProto(cx, &JSObject::class_, nullptr, self, TenuredObject);
        if (!intrinsicsHolder)
            return nullptr;
    }
    self->setIntrinsicsHolder(intrinsicsHolder);
    
    RootedValue global(cx, ObjectValue(*self));
    if (!JSObject::defineProperty(cx, intrinsicsHolder, cx->names().global,
                                  global, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return nullptr;
    }

    







    Rooted<TaggedProto> tagged(cx, TaggedProto(objectProto));
    if (self->shouldSplicePrototype(cx) && !self->splicePrototype(cx, self->getClass(), tagged))
        return nullptr;

    



    RootedScript functionProtoScript(cx, functionProto->nonLazyScript());
    CallNewScriptHook(cx, functionProtoScript, functionProto);
    return functionProto;
}

GlobalObject *
GlobalObject::create(JSContext *cx, const Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    JSObject *obj = NewObjectWithGivenProto(cx, clasp, nullptr, nullptr, SingletonObject);
    if (!obj)
        return nullptr;

    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());

    cx->compartment()->initGlobal(*global);

    if (!global->setVarObj(cx))
        return nullptr;
    if (!global->setDelegate(cx))
        return nullptr;

    
    JSObject *res = RegExpStatics::create(cx, global);
    if (!res)
        return nullptr;

    global->initSlot(REGEXP_STATICS, ObjectValue(*res));
    return global;
}

 bool
GlobalObject::initStandardClasses(JSContext *cx, Handle<GlobalObject*> global)
{
    
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!JSObject::defineProperty(cx, global, cx->names().undefined, undefinedValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return false;
    }

    if (!global->initFunctionAndObjectClasses(cx))
        return false;

    
    return js_InitArrayClass(cx, global) &&
           js_InitBooleanClass(cx, global) &&
           js_InitExceptionClasses(cx, global) &&
           js_InitMathClass(cx, global) &&
           js_InitNumberClass(cx, global) &&
           js_InitJSONClass(cx, global) &&
           js_InitRegExpClass(cx, global) &&
           js_InitStringClass(cx, global) &&
           js_InitTypedArrayClasses(cx, global) &&
           js_InitIteratorClasses(cx, global) &&
           js_InitDateClass(cx, global) &&
           js_InitWeakMapClass(cx, global) &&
           js_InitProxyClass(cx, global) &&
           js_InitMapClass(cx, global) &&
           GlobalObject::initMapIteratorProto(cx, global) &&
           js_InitSetClass(cx, global) &&
           GlobalObject::initSetIteratorProto(cx, global) &&
#if EXPOSE_INTL_API
           js_InitIntlClass(cx, global) &&
#endif
           true;
}

 bool
GlobalObject::isRuntimeCodeGenEnabled(JSContext *cx, Handle<GlobalObject*> global)
{
    HeapSlot &v = global->getSlotRef(RUNTIME_CODEGEN_ENABLED);
    if (v.isUndefined()) {
        



        JSCSPEvalChecker allows = cx->runtime()->securityCallbacks->contentSecurityPolicyAllows;
        Value boolValue = BooleanValue(!allows || allows(cx));
        v.set(global, HeapSlot::Slot, RUNTIME_CODEGEN_ENABLED, boolValue);
    }
    return !v.isFalse();
}

JSFunction *
GlobalObject::createConstructor(JSContext *cx, Native ctor, JSAtom *nameArg, unsigned length,
                                gc::AllocKind kind)
{
    RootedAtom name(cx, nameArg);
    RootedObject self(cx, this);
    return NewFunction(cx, NullPtr(), ctor, length, JSFunction::NATIVE_CTOR, self, name, kind);
}

static JSObject *
CreateBlankProto(JSContext *cx, const Class *clasp, JSObject &proto, GlobalObject &global)
{
    JS_ASSERT(clasp != &JSObject::class_);
    JS_ASSERT(clasp != &JSFunction::class_);

    RootedObject blankProto(cx, NewObjectWithGivenProto(cx, clasp, &proto, &global, SingletonObject));
    if (!blankProto)
        return nullptr;

    return blankProto;
}

JSObject *
GlobalObject::createBlankPrototype(JSContext *cx, const Class *clasp)
{
    Rooted<GlobalObject*> self(cx, this);
    JSObject *objectProto = getOrCreateObjectPrototype(cx);
    if (!objectProto)
        return nullptr;

    return CreateBlankProto(cx, clasp, *objectProto, *self.get());
}

JSObject *
GlobalObject::createBlankPrototypeInheriting(JSContext *cx, const Class *clasp, JSObject &proto)
{
    return CreateBlankProto(cx, clasp, proto, *this);
}

bool
js::LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor_, JSObject *proto_)
{
    RootedObject ctor(cx, ctor_), proto(cx, proto_);

    RootedValue protoVal(cx, ObjectValue(*proto));
    RootedValue ctorVal(cx, ObjectValue(*ctor));

    return JSObject::defineProperty(cx, ctor, cx->names().classPrototype,
                                    protoVal, JS_PropertyStub, JS_StrictPropertyStub,
                                    JSPROP_PERMANENT | JSPROP_READONLY) &&
           JSObject::defineProperty(cx, proto, cx->names().constructor,
                                    ctorVal, JS_PropertyStub, JS_StrictPropertyStub, 0);
}

bool
js::DefinePropertiesAndBrand(JSContext *cx, JSObject *obj_,
                             const JSPropertySpec *ps, const JSFunctionSpec *fs)
{
    RootedObject obj(cx, obj_);

    if (ps && !JS_DefineProperties(cx, obj, ps))
        return false;
    if (fs && !JS_DefineFunctions(cx, obj, fs))
        return false;
    return true;
}

static void
GlobalDebuggees_finalize(FreeOp *fop, JSObject *obj)
{
    fop->delete_((GlobalObject::DebuggerVector *) obj->getPrivate());
}

static const Class
GlobalDebuggees_class = {
    "GlobalDebuggee", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, GlobalDebuggees_finalize
};

GlobalObject::DebuggerVector *
GlobalObject::getDebuggers()
{
    Value debuggers = getReservedSlot(DEBUGGERS);
    if (debuggers.isUndefined())
        return nullptr;
    JS_ASSERT(debuggers.toObject().getClass() == &GlobalDebuggees_class);
    return (DebuggerVector *) debuggers.toObject().getPrivate();
}

 GlobalObject::DebuggerVector *
GlobalObject::getOrCreateDebuggers(JSContext *cx, Handle<GlobalObject*> global)
{
    assertSameCompartment(cx, global);
    DebuggerVector *debuggers = global->getDebuggers();
    if (debuggers)
        return debuggers;

    JSObject *obj = NewObjectWithGivenProto(cx, &GlobalDebuggees_class, nullptr, global);
    if (!obj)
        return nullptr;
    debuggers = cx->new_<DebuggerVector>();
    if (!debuggers)
        return nullptr;
    obj->setPrivate(debuggers);
    global->setReservedSlot(DEBUGGERS, ObjectValue(*obj));
    return debuggers;
}

 bool
GlobalObject::addDebugger(JSContext *cx, Handle<GlobalObject*> global, Debugger *dbg)
{
    DebuggerVector *debuggers = getOrCreateDebuggers(cx, global);
    if (!debuggers)
        return false;
#ifdef DEBUG
    for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++)
        JS_ASSERT(*p != dbg);
#endif
    if (debuggers->empty() && !global->compartment()->addDebuggee(cx, global))
        return false;
    if (!debuggers->append(dbg)) {
        global->compartment()->removeDebuggee(cx->runtime()->defaultFreeOp(), global);
        return false;
    }
    return true;
}

bool
GlobalObject::getSelfHostedFunction(JSContext *cx, HandleAtom selfHostedName, HandleAtom name,
                                    unsigned nargs, MutableHandleValue funVal)
{
    RootedId shId(cx, AtomToId(selfHostedName));
    RootedObject holder(cx, cx->global()->intrinsicsHolder());

    if (HasDataProperty(cx, holder, shId, funVal.address()))
        return true;

    if (!cx->runtime()->maybeWrappedSelfHostedFunction(cx, shId, funVal))
        return false;
    if (!funVal.isUndefined())
        return true;

    JSFunction *fun = NewFunction(cx, NullPtr(), nullptr, nargs, JSFunction::INTERPRETED_LAZY,
                                  holder, name, JSFunction::ExtendedFinalizeKind, SingletonObject);
    if (!fun)
        return false;
    fun->setIsSelfHostedBuiltin();
    fun->setExtendedSlot(0, StringValue(selfHostedName));
    funVal.setObject(*fun);

    return JSObject::defineGeneric(cx, holder, shId, funVal, nullptr, nullptr, 0);
}
