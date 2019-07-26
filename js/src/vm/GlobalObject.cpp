






#include "GlobalObject.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfriendapi.h"
#include "jsmath.h"
#include "json.h"
#include "jsweakmap.h"

#include "builtin/Eval.h"
#include "builtin/Intl.h"
#include "builtin/MapObject.h"
#include "builtin/Object.h"
#include "builtin/RegExp.h"
#include "frontend/BytecodeEmitter.h"

#include "jsobjinlines.h"

#include "vm/GlobalObject-inl.h"
#include "vm/RegExpObject-inl.h"
#include "vm/RegExpStatics-inl.h"

#ifdef JS_METHODJIT
#include "methodjit/Retcon.h"
#endif

using namespace js;

JSObject *
js_InitObjectClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    return obj->asGlobal().getOrCreateObjectPrototype(cx);
}

JSObject *
js_InitFunctionClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    return obj->asGlobal().getOrCreateFunctionPrototype(cx);
}

static JSBool
ThrowTypeError(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                 JSMSG_THROW_TYPE_ERROR);
    return false;
}

static bool
TestProtoGetterThis(const Value &v)
{
    return !v.isNullOrUndefined();
}

static bool
ProtoGetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(TestProtoGetterThis(args.thisv()));

    const Value &thisv = args.thisv();
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

static JSBool
ProtoGetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, TestProtoGetterThis, ProtoGetterImpl, args);
}

namespace js {
size_t sSetProtoCalled = 0;
} 

static bool
TestProtoSetterThis(const Value &v)
{
    if (v.isNullOrUndefined())
        return false;

    
    if (!v.isObject())
        return true;

    
    return !v.toObject().isProxy();
}

static bool
ProtoSetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(TestProtoSetterThis(args.thisv()));

    const Value &thisv = args.thisv();
    if (thisv.isPrimitive()) {
        JS_ASSERT(!thisv.isNullOrUndefined());

        
        args.rval().setUndefined();
        return true;
    }

    if (!cx->runningWithTrustedPrincipals())
        ++sSetProtoCalled;

    Rooted<JSObject*> obj(cx, &args.thisv().toObject());

    
    if (!obj->isExtensible()) {
        obj->reportNotExtensible(cx);
        return false;
    }

    





    if (obj->isProxy() || obj->isArrayBuffer()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Object", "__proto__ setter",
                             obj->isProxy() ? "Proxy" : "ArrayBuffer");
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

static JSBool
ProtoSetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, TestProtoSetterThis, ProtoSetterImpl, args);
}

JSObject *
GlobalObject::initFunctionAndObjectClasses(JSContext *cx)
{
    Rooted<GlobalObject*> self(cx, this);

    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    JS_ASSERT(isNative());

    cx->setDefaultCompartmentObjectIfUnset(self);

    RootedObject objectProto(cx);

    



    objectProto = NewObjectWithGivenProto(cx, &ObjectClass, NULL, self, SingletonObject);
    if (!objectProto)
        return NULL;

    




    if (!setNewTypeUnknown(cx, &ObjectClass, objectProto))
        return NULL;

    
    RootedFunction functionProto(cx);
    {
        RawObject functionProto_ = NewObjectWithGivenProto(cx, &FunctionClass, objectProto, self,
                                                           SingletonObject);
        if (!functionProto_)
            return NULL;
        functionProto = functionProto_->toFunction();

        



        {
            RawObject proto = NewFunction(cx, functionProto, NULL, 0, JSFunction::INTERPRETED,
                                          self, NullPtr());
            if (!proto)
                return NULL;
            JS_ASSERT(proto == functionProto);
            functionProto->setIsFunctionPrototype();
        }

        const char *rawSource = "() {\n}";
        size_t sourceLen = strlen(rawSource);
        jschar *source = InflateString(cx, rawSource, &sourceLen);
        if (!source)
            return NULL;
        ScriptSource *ss = cx->new_<ScriptSource>();
        if (!ss) {
            js_free(source);
            return NULL;
        }
        ScriptSourceHolder ssh(ss);
        ss->setSource(source, sourceLen);

        CompileOptions options(cx);
        options.setNoScriptRval(true)
               .setVersion(JSVERSION_DEFAULT);
        RootedScript script(cx, JSScript::Create(cx,
                                                  NullPtr(),
                                                  false,
                                                 options,
                                                  0,
                                                 ss,
                                                 0,
                                                 ss->length()));
        if (!script || !JSScript::fullyInitTrivial(cx, script))
            return NULL;

        functionProto->initScript(script);
        types::TypeObject* protoType = functionProto->getType(cx);
        if (!protoType)
            return NULL;
        protoType->interpretedFunction = functionProto;
        script->setFunction(functionProto);

        




        if (!setNewTypeUnknown(cx, &FunctionClass, functionProto))
            return NULL;
    }

    
    RootedFunction objectCtor(cx);
    {
        RootedObject ctor(cx, NewObjectWithGivenProto(cx, &FunctionClass, functionProto, self,
                                                      SingletonObject));
        if (!ctor)
            return NULL;
        RootedAtom objectAtom(cx, cx->names().Object);
        objectCtor = NewFunction(cx, ctor, obj_construct, 1, JSFunction::NATIVE_CTOR, self,
                                 objectAtom);
        if (!objectCtor)
            return NULL;
    }

    



    self->setObjectClassDetails(objectCtor, objectProto);

    
    RootedFunction functionCtor(cx);
    {
        
        RootedObject ctor(cx, NewObjectWithGivenProto(cx, &FunctionClass, functionProto, self,
                                                      SingletonObject));
        if (!ctor)
            return NULL;
        RootedAtom functionAtom(cx, cx->names().Function);
        functionCtor = NewFunction(cx, ctor, Function, 1, JSFunction::NATIVE_CTOR, self,
                                   functionAtom);
        if (!functionCtor)
            return NULL;
        JS_ASSERT(ctor == functionCtor);
    }

    



    self->setFunctionClassDetails(functionCtor, functionProto);

    



    if (!LinkConstructorAndPrototype(cx, objectCtor, objectProto) ||
        !DefinePropertiesAndBrand(cx, objectProto, NULL, object_methods))
    {
        return NULL;
    }

    





    RootedFunction getter(cx, NewFunction(cx, NullPtr(), ProtoGetter, 0, JSFunction::NATIVE_FUN,
                                          self, NullPtr()));
    if (!getter)
        return NULL;
#if JS_HAS_OBJ_PROTO_PROP
    RootedFunction setter(cx, NewFunction(cx, NullPtr(), ProtoSetter, 0, JSFunction::NATIVE_FUN,
                                          self, NullPtr()));
    if (!setter)
        return NULL;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!JSObject::defineProperty(cx, objectProto,
                                  cx->names().proto, undefinedValue,
                                  JS_DATA_TO_FUNC_PTR(PropertyOp, getter.get()),
                                  JS_DATA_TO_FUNC_PTR(StrictPropertyOp, setter.get()),
                                  JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED))
    {
        return NULL;
    }
#endif 
    self->setProtoGetter(getter);


    if (!DefinePropertiesAndBrand(cx, objectCtor, NULL, object_static_methods) ||
        !LinkConstructorAndPrototype(cx, functionCtor, functionProto) ||
        !DefinePropertiesAndBrand(cx, functionProto, NULL, function_methods) ||
        !DefinePropertiesAndBrand(cx, functionCtor, NULL, NULL))
    {
        return NULL;
    }

    
    if (!self->addDataProperty(cx, cx->names().Object, JSProto_Object + JSProto_LIMIT * 2, 0))
        return NULL;
    if (!self->addDataProperty(cx, cx->names().Function, JSProto_Function + JSProto_LIMIT * 2, 0))
        return NULL;

    

    
    RootedId evalId(cx, NameToId(cx->names().eval));
    RawObject evalobj = DefineFunction(cx, self, evalId, IndirectEval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return NULL;
    self->setOriginalEval(evalobj);

    
    RootedFunction throwTypeError(cx, NewFunction(cx, NullPtr(), ThrowTypeError, 0,
                                                  JSFunction::NATIVE_FUN, self, NullPtr()));
    if (!throwTypeError)
        return NULL;
    if (!throwTypeError->preventExtensions(cx))
        return NULL;
    self->setThrowTypeError(throwTypeError);

    RootedObject intrinsicsHolder(cx);
    if (cx->runtime->isSelfHostingGlobal(self)) {
        intrinsicsHolder = self;
    } else {
        intrinsicsHolder = NewObjectWithClassProto(cx, &ObjectClass, NULL, self);
        if (!intrinsicsHolder)
            return NULL;
    }
    self->setIntrinsicsHolder(intrinsicsHolder);
    
    RootedValue global(cx, ObjectValue(*self));
    if (!JSObject::defineProperty(cx, intrinsicsHolder, cx->names().global,
                                  global, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return NULL;
    }

    







    Rooted<TaggedProto> tagged(cx, TaggedProto(objectProto));
    if (self->shouldSplicePrototype(cx) && !self->splicePrototype(cx, self->getClass(), tagged))
        return NULL;

    



    RootedScript functionProtoScript(cx, functionProto->nonLazyScript());
    CallNewScriptHook(cx, functionProtoScript, functionProto);
    return functionProto;
}

GlobalObject *
GlobalObject::create(JSContext *cx, Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    JSObject *obj = NewObjectWithGivenProto(cx, clasp, NULL, NULL, SingletonObject);
    if (!obj)
        return NULL;

    Rooted<GlobalObject *> global(cx, &obj->asGlobal());

    cx->compartment->initGlobal(*global);

    if (!global->setVarObj(cx))
        return NULL;
    if (!global->setDelegate(cx))
        return NULL;

    
    JSObject *res = RegExpStatics::create(cx, global);
    if (!res)
        return NULL;

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
#if ENABLE_INTL_API
           js_InitIntlClass(cx, global) &&
#endif
           true;
}

 bool
GlobalObject::isRuntimeCodeGenEnabled(JSContext *cx, Handle<GlobalObject*> global)
{
    HeapSlot &v = global->getSlotRef(RUNTIME_CODEGEN_ENABLED);
    if (v.isUndefined()) {
        



        JSCSPEvalChecker allows = cx->runtime->securityCallbacks->contentSecurityPolicyAllows;
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
CreateBlankProto(JSContext *cx, Class *clasp, JSObject &proto, GlobalObject &global)
{
    JS_ASSERT(clasp != &ObjectClass);
    JS_ASSERT(clasp != &FunctionClass);

    RootedObject blankProto(cx, NewObjectWithGivenProto(cx, clasp, &proto, &global, SingletonObject));
    if (!blankProto)
        return NULL;

    return blankProto;
}

JSObject *
GlobalObject::createBlankPrototype(JSContext *cx, Class *clasp)
{
    Rooted<GlobalObject*> self(cx, this);
    JSObject *objectProto = getOrCreateObjectPrototype(cx);
    if (!objectProto)
        return NULL;

    return CreateBlankProto(cx, clasp, *objectProto, *self.get());
}

JSObject *
GlobalObject::createBlankPrototypeInheriting(JSContext *cx, Class *clasp, JSObject &proto)
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

    if (ps && !JS_DefineProperties(cx, obj, const_cast<JSPropertySpec*>(ps)))
        return false;
    if (fs && !JS_DefineFunctions(cx, obj, const_cast<JSFunctionSpec*>(fs)))
        return false;
    return true;
}

static void
GlobalDebuggees_finalize(FreeOp *fop, RawObject obj)
{
    fop->delete_((GlobalObject::DebuggerVector *) obj->getPrivate());
}

static Class
GlobalDebuggees_class = {
    "GlobalDebuggee", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, GlobalDebuggees_finalize
};

GlobalObject::DebuggerVector *
GlobalObject::getDebuggers()
{
    Value debuggers = getReservedSlot(DEBUGGERS);
    if (debuggers.isUndefined())
        return NULL;
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

    JSObject *obj = NewObjectWithGivenProto(cx, &GlobalDebuggees_class, NULL, global);
    if (!obj)
        return NULL;
    debuggers = cx->new_<DebuggerVector>();
    if (!debuggers)
        return NULL;
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
        global->compartment()->removeDebuggee(cx->runtime->defaultFreeOp(), global);
        return false;
    }
    return true;
}
