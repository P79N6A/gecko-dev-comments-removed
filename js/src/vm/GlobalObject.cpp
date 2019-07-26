






#include "GlobalObject.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfriendapi.h"
#include "jsmath.h"
#include "json.h"
#include "jsweakmap.h"

#include "builtin/Eval.h"
#include "builtin/MapObject.h"
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
js_InitObjectClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    return obj->asGlobal().getOrCreateObjectPrototype(cx);
}

JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj)
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

namespace js {

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
    Rooted<JSObject*> obj(cx, &args.thisv().toObject());
    Rooted<jsid> nid(cx, NameToId(cx->runtime->atomState.protoAtom));
    Rooted<Value> v(cx);
    if (!CheckAccess(cx, obj, nid, JSACC_PROTO, v.address(), &dummy))
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

size_t sSetProtoCalled = 0;

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
    Rooted<jsid> nid(cx, NameToId(cx->runtime->atomState.protoAtom));
    Rooted<Value> v(cx);
    if (!CheckAccess(cx, obj, nid, JSAccessMode(JSACC_PROTO | JSACC_WRITE), v.address(), &dummy))
        return false;

    if (!SetProto(cx, obj, newProto, true))
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

JSFunctionSpec intrinsic_functions[] = {
    JS_FN("ThrowTypeError",      ThrowTypeError,     0,0),
    JS_FS_END
};
JSObject *
GlobalObject::initFunctionAndObjectClasses(JSContext *cx)
{
    Rooted<GlobalObject*> self(cx, this);

    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    JS_ASSERT(isNative());

    




    

    
    if (!cx->globalObject)
        JS_SetGlobalObject(cx, self);

    RootedObject objectProto(cx);

    



    objectProto = NewObjectWithGivenProto(cx, &ObjectClass, NULL, self);
    if (!objectProto || !objectProto->setSingletonType(cx))
        return NULL;

    




    if (!objectProto->setNewTypeUnknown(cx))
        return NULL;

    
    RootedFunction functionProto(cx);
    {
        JSObject *functionProto_ = NewObjectWithGivenProto(cx, &FunctionClass, objectProto, self);
        if (!functionProto_)
            return NULL;
        functionProto = functionProto_->toFunction();

        



        JSObject *proto = js_NewFunction(cx, functionProto,
                                         NULL, 0, JSFUN_INTERPRETED, self, NULL);
        if (!proto)
            return NULL;
        JS_ASSERT(proto == functionProto);
        functionProto->flags |= JSFUN_PROTOTYPE;

        const char *rawSource = "() {\n}";
        size_t sourceLen = strlen(rawSource);
        jschar *source = InflateString(cx, rawSource, &sourceLen);
        if (!source)
            return NULL;
        ScriptSource *ss = cx->new_<ScriptSource>();
        if (!ss) {
            cx->free_(source);
            return NULL;
        }
        ss->setSource(source, sourceLen);

        CompileOptions options(cx);
        options.setNoScriptRval(true)
               .setVersion(JSVERSION_DEFAULT);
        Rooted<JSScript*> script(cx, JSScript::Create(cx,
                                                       NullPtr(),
                                                       false,
                                                      options,
                                                       0,
                                                      ss,
                                                      0,
                                                      ss->length()));
        ss->attachToRuntime(cx->runtime);
        if (!script || !JSScript::fullyInitTrivial(cx, script))
            return NULL;

        functionProto->initScript(script);
        functionProto->getType(cx)->interpretedFunction = functionProto;
        script->setFunction(functionProto);

        if (!functionProto->setSingletonType(cx))
            return NULL;

        




        if (!functionProto->setNewTypeUnknown(cx))
            return NULL;
    }

    
    RootedFunction objectCtor(cx);
    {
        JSObject *ctor = NewObjectWithGivenProto(cx, &FunctionClass, functionProto, self);
        if (!ctor)
            return NULL;
        objectCtor = js_NewFunction(cx, ctor, js_Object, 1, JSFUN_CONSTRUCTOR, self,
                                    CLASS_NAME(cx, Object));
        if (!objectCtor)
            return NULL;
    }

    



    self->setObjectClassDetails(objectCtor, objectProto);

    
    RootedFunction functionCtor(cx);
    {
        
        RootedObject ctor(cx, NewObjectWithGivenProto(cx, &FunctionClass, functionProto, self));
        if (!ctor)
            return NULL;
        functionCtor = js_NewFunction(cx, ctor, Function, 1, JSFUN_CONSTRUCTOR, self,
                                      CLASS_NAME(cx, Function));
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

    





    Rooted<JSFunction*> getter(cx, js_NewFunction(cx, NULL, ProtoGetter, 0, 0, self, NULL));
    if (!getter)
        return NULL;
#if JS_HAS_OBJ_PROTO_PROP
    Rooted<JSFunction*> setter(cx, js_NewFunction(cx, NULL, ProtoSetter, 0, 0, self, NULL));
    if (!setter)
        return NULL;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!objectProto->defineProperty(cx, cx->runtime->atomState.protoAtom, undefinedValue,
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

    
    jsid objectId = NameToId(CLASS_NAME(cx, Object));
    if (!self->addDataProperty(cx, objectId, JSProto_Object + JSProto_LIMIT * 2, 0))
        return NULL;
    jsid functionId = NameToId(CLASS_NAME(cx, Function));
    if (!self->addDataProperty(cx, functionId, JSProto_Function + JSProto_LIMIT * 2, 0))
        return NULL;

    

    
    RootedId id(cx, NameToId(cx->runtime->atomState.evalAtom));
    JSObject *evalobj = js_DefineFunction(cx, self, id, IndirectEval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return NULL;
    self->setOriginalEval(evalobj);

    
    RootedFunction throwTypeError(cx, js_NewFunction(cx, NULL, ThrowTypeError, 0, 0, self, NULL));
    if (!throwTypeError)
        return NULL;
    if (!throwTypeError->preventExtensions(cx))
        return NULL;
    self->setThrowTypeError(throwTypeError);

    RootedObject intrinsicsHolder(cx, JS_NewObject(cx, NULL, NULL, self));
    if (!intrinsicsHolder)
        return NULL;
    self->setIntrinsicsHolder(intrinsicsHolder);
    if (!JS_DefineFunctions(cx, intrinsicsHolder, intrinsic_functions))
        return NULL;

    







    if (self->shouldSplicePrototype(cx) && !self->splicePrototype(cx, objectProto))
        return NULL;

    



    js_CallNewScriptHook(cx, functionProto->script(), functionProto);
    return functionProto;
}

GlobalObject *
GlobalObject::create(JSContext *cx, Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    JSObject *obj = NewObjectWithGivenProto(cx, clasp, NULL, NULL);
    if (!obj)
        return NULL;

    Rooted<GlobalObject *> global(cx, &obj->asGlobal());

    cx->compartment->initGlobal(*global);

    if (!global->setSingletonType(cx) || !global->setVarObj(cx))
        return NULL;
    if (!obj->setDelegate(cx))
        return NULL;

    
    JSObject *res = RegExpStatics::create(cx, global);
    if (!res)
        return NULL;
    global->initSlot(REGEXP_STATICS, ObjectValue(*res));
    global->initFlags(0);

    return global;
}

 bool
GlobalObject::initStandardClasses(JSContext *cx, Handle<GlobalObject*> global)
{
    JSAtomState &state = cx->runtime->atomState;

    
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!global->defineProperty(cx, state.typeAtoms[JSTYPE_VOID], undefinedValue,
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
#if JS_HAS_XML_SUPPORT
           (!VersionHasAllowXML(cx->findVersion()) || js_InitXMLClasses(cx, global)) &&
#endif
           js_InitIteratorClasses(cx, global) &&
           js_InitDateClass(cx, global) &&
           js_InitWeakMapClass(cx, global) &&
           js_InitProxyClass(cx, global) &&
           js_InitMapClass(cx, global) &&
           GlobalObject::initMapIteratorProto(cx, global) &&
           js_InitSetClass(cx, global) &&
           GlobalObject::initSetIteratorProto(cx, global);
}

void
GlobalObject::clear(JSContext *cx)
{
    for (int key = JSProto_Null; key < JSProto_LIMIT * 3; key++)
        setSlot(key, UndefinedValue());

    
    getRegExpStatics()->clear();

    
    setSlot(RUNTIME_CODEGEN_ENABLED, UndefinedValue());

    




    setSlot(BOOLEAN_VALUEOF, UndefinedValue());
    setSlot(EVAL, UndefinedValue());
    setSlot(CREATE_DATAVIEW_FOR_THIS, UndefinedValue());
    setSlot(THROWTYPEERROR, UndefinedValue());
    setSlot(INTRINSICS, UndefinedValue());
    setSlot(PROTO_GETTER, UndefinedValue());

    



    int32_t flags = getSlot(FLAGS).toInt32();
    flags |= FLAGS_CLEARED;
    setSlot(FLAGS, Int32Value(flags));

    



    cx->runtime->newObjectCache.purge();

#ifdef JS_METHODJIT
    




    for (gc::CellIter i(cx->compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compileAndGo && script->hasMJITInfo() && script->hasClearedGlobal()) {
            mjit::Recompiler::clearStackReferences(cx->runtime->defaultFreeOp(), script);
            mjit::ReleaseScriptCode(cx->runtime->defaultFreeOp(), script);
        }
    }
#endif
}

bool
GlobalObject::isRuntimeCodeGenEnabled(JSContext *cx)
{
    HeapSlot &v = getSlotRef(RUNTIME_CODEGEN_ENABLED);
    if (v.isUndefined()) {
        



        JSCSPEvalChecker allows = cx->runtime->securityCallbacks->contentSecurityPolicyAllows;
        v.set(this, RUNTIME_CODEGEN_ENABLED, BooleanValue(!allows || allows(cx)));
    }
    return !v.isFalse();
}

JSFunction *
GlobalObject::createConstructor(JSContext *cx, Native ctor, JSAtom *name, unsigned length,
                                gc::AllocKind kind)
{
    RootedObject self(cx, this);
    return js_NewFunction(cx, NULL, ctor, length, JSFUN_CONSTRUCTOR, self, name, kind);
}

static JSObject *
CreateBlankProto(JSContext *cx, Class *clasp, JSObject &proto, GlobalObject &global)
{
    JS_ASSERT(clasp != &ObjectClass);
    JS_ASSERT(clasp != &FunctionClass);

    RootedObject blankProto(cx, NewObjectWithGivenProto(cx, clasp, &proto, &global));
    if (!blankProto || !blankProto->setSingletonType(cx))
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
LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor_, JSObject *proto_)
{
    RootedObject ctor(cx, ctor_), proto(cx, proto_);

    RootedValue protoVal(cx, ObjectValue(*proto));
    RootedValue ctorVal(cx, ObjectValue(*ctor));

    return ctor->defineProperty(cx, cx->runtime->atomState.classPrototypeAtom,
                                protoVal, JS_PropertyStub, JS_StrictPropertyStub,
                                JSPROP_PERMANENT | JSPROP_READONLY) &&
           proto->defineProperty(cx, cx->runtime->atomState.constructorAtom,
                                 ctorVal, JS_PropertyStub, JS_StrictPropertyStub, 0);
}

bool
DefinePropertiesAndBrand(JSContext *cx, JSObject *obj_,
                         const JSPropertySpec *ps, const JSFunctionSpec *fs)
{
    RootedObject obj(cx, obj_);

    if (ps && !JS_DefineProperties(cx, obj, const_cast<JSPropertySpec*>(ps)))
        return false;
    if (fs && !JS_DefineFunctions(cx, obj, const_cast<JSFunctionSpec*>(fs)))
        return false;
    return true;
}

void
GlobalDebuggees_finalize(FreeOp *fop, JSObject *obj)
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

} 
