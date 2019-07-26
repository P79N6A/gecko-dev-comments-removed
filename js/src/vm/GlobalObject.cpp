





#include "vm/GlobalObject.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfriendapi.h"
#include "jsmath.h"
#include "json.h"
#include "jsprototypes.h"
#include "jsweakmap.h"
#include "jsworkers.h"

#include "builtin/Eval.h"
#if EXPOSE_INTL_API
# include "builtin/Intl.h"
#endif
#include "builtin/MapObject.h"
#include "builtin/Object.h"
#include "builtin/RegExp.h"
#include "builtin/SIMD.h"
#include "builtin/TypedObject.h"
#include "vm/PIC.h"
#include "vm/RegExpStatics.h"
#include "vm/StopIterationObject.h"
#include "vm/WeakMapObject.h"

#include "jscompartmentinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/ObjectImpl-inl.h"

using namespace js;

struct ProtoTableEntry {
    const Class *clasp;
    ClassInitializerOp init;
};

#define DECLARE_PROTOTYPE_CLASS_INIT(name,code,init,clasp) \
    extern JSObject *init(JSContext *cx, Handle<JSObject*> obj);
JS_FOR_EACH_PROTOTYPE(DECLARE_PROTOTYPE_CLASS_INIT)
#undef DECLARE_PROTOTYPE_CLASS_INIT

JSObject *
js_InitViaClassSpec(JSContext *cx, Handle<JSObject*> obj)
{
    MOZ_ASSUME_UNREACHABLE();
}

static const ProtoTableEntry protoTable[JSProto_LIMIT] = {
#define INIT_FUNC(name,code,init,clasp) { clasp, init },
#define INIT_FUNC_DUMMY(name,code,init,clasp) { nullptr, nullptr },
    JS_FOR_PROTOTYPES(INIT_FUNC, INIT_FUNC_DUMMY)
#undef INIT_FUNC_DUMMY
#undef INIT_FUNC
};

const js::Class *
js::ProtoKeyToClass(JSProtoKey key)
{
    MOZ_ASSERT(key < JSProto_LIMIT);
    return protoTable[key].clasp;
}




TypedObjectModuleObject&
js::GlobalObject::getTypedObjectModule() const {
    Value v = getConstructor(JSProto_TypedObject);
    
    JS_ASSERT(v.isObject());
    return v.toObject().as<TypedObjectModuleObject>();
}

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
TestProtoThis(HandleValue v)
{
    return !v.isNullOrUndefined();
}

static bool
ProtoGetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(TestProtoThis(args.thisv()));

    HandleValue thisv = args.thisv();
    if (thisv.isPrimitive() && !BoxNonStrictThis(cx, args))
        return false;

    RootedObject obj(cx, &args.thisv().toObject());
    RootedObject proto(cx);
    if (!JSObject::getProto(cx, obj, &proto))
        return false;
    args.rval().setObjectOrNull(proto);
    return true;
}

static bool
ProtoGetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, TestProtoThis, ProtoGetterImpl, args);
}

namespace js {
size_t sSetProtoCalled = 0;
} 

static bool
ProtoSetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(TestProtoThis(args.thisv()));

    HandleValue thisv = args.thisv();
    if (thisv.isPrimitive()) {
        JS_ASSERT(!thisv.isNullOrUndefined());

        
        args.rval().setUndefined();
        return true;
    }

    if (!cx->runningWithTrustedPrincipals())
        ++sSetProtoCalled;

    Rooted<JSObject*> obj(cx, &args.thisv().toObject());

    
    if (args.length() == 0 || !args[0].isObjectOrNull()) {
        args.rval().setUndefined();
        return true;
    }

    Rooted<JSObject*> newProto(cx, args[0].toObjectOrNull());

    bool success;
    if (!JSObject::setProto(cx, obj, newProto, &success))
        return false;

    if (!success) {
        js_ReportValueError(cx, JSMSG_SETPROTOTYPEOF_FAIL, JSDVG_IGNORE_STACK, thisv, js::NullPtr());
        return false;
    }

    args.rval().setUndefined();
    return true;
}

static bool
ProtoSetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    
    
    RootedObject callee(cx, &args.callee());
    if (!GlobalObject::warnOnceAboutPrototypeMutation(cx, callee))
        return false;

    return CallNonGenericMethod(cx, TestProtoThis, ProtoSetterImpl, args);
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
            cx->new_<ScriptSource>();
        if (!ss) {
            js_free(source);
            return nullptr;
        }
        ScriptSourceHolder ssHolder(ss);
        ss->setSource(source, sourceLen);
        CompileOptions options(cx);
        options.setNoScriptRval(true)
               .setVersion(JSVERSION_DEFAULT);
        RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss, options));
        if (!sourceObject)
            return nullptr;

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
    if (!JSObject::defineProperty(cx, objectProto,
                                  cx->names().proto, UndefinedHandleValue,
                                  JS_DATA_TO_FUNC_PTR(PropertyOp, getter.get()),
                                  JS_DATA_TO_FUNC_PTR(StrictPropertyOp, setter.get()),
                                  JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED))
    {
        return nullptr;
    }
#endif 

    if (!DefinePropertiesAndBrand(cx, objectCtor, nullptr, object_static_methods) ||
        !LinkConstructorAndPrototype(cx, functionCtor, functionProto) ||
        !DefinePropertiesAndBrand(cx, functionProto, nullptr, function_methods) ||
        !DefinePropertiesAndBrand(cx, functionCtor, nullptr, nullptr))
    {
        return nullptr;
    }

    
    if (!self->addDataProperty(cx, cx->names().Object, constructorPropertySlot(JSProto_Object), 0))
        return nullptr;
    if (!self->addDataProperty(cx, cx->names().Function, constructorPropertySlot(JSProto_Function), 0))
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
        RootedObject proto(cx, self->getOrCreateObjectPrototype(cx));
        if (!proto)
            return nullptr;
        intrinsicsHolder = NewObjectWithGivenProto(cx, &JSObject::class_, proto, self,
                                                   TenuredObject);
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

 bool
GlobalObject::ensureConstructor(JSContext *cx, Handle<GlobalObject*> global, JSProtoKey key)
{
    if (global->isStandardClassResolved(key))
        return true;
    return resolveConstructor(cx, global, key);
}

 bool
GlobalObject::resolveConstructor(JSContext *cx, Handle<GlobalObject*> global, JSProtoKey key)
{
    MOZ_ASSERT(!global->isStandardClassResolved(key));

    
    
    
    
    ClassInitializerOp init = protoTable[key].init;
    if (init == js_InitViaClassSpec)
        init = nullptr;

    const Class *clasp = ProtoKeyToClass(key);

    
    
    
    
    
    
    bool haveSpec = clasp && clasp->spec.defined();
    if (!init && !haveSpec)
        return true;

    
    if (init) {
        MOZ_ASSERT(!haveSpec);
        return init(cx, global);
    }

    
    
    

    
    RootedObject ctor(cx, clasp->spec.createConstructor(cx, key));
    if (!ctor)
        return false;

    
    if (const JSFunctionSpec *funs = clasp->spec.constructorFunctions) {
        if (!JS_DefineFunctions(cx, ctor, funs))
            return false;
    }

    
    
    RootedObject proto(cx);
    if (clasp->spec.createPrototype) {
        proto = clasp->spec.createPrototype(cx, key);
        if (!proto)
            return false;
    }
    if (const JSFunctionSpec *funs = clasp->spec.prototypeFunctions) {
        if (!JS_DefineFunctions(cx, proto, funs))
            return false;
    }

    
    if (proto && !LinkConstructorAndPrototype(cx, ctor, proto))
        return false;

    
    if (clasp->spec.finishInit && !clasp->spec.finishInit(cx, ctor, proto))
        return false;

    
    return initBuiltinConstructor(cx, global, key, ctor, proto);
}

 bool
GlobalObject::initBuiltinConstructor(JSContext *cx, Handle<GlobalObject*> global,
                                     JSProtoKey key, HandleObject ctor, HandleObject proto)
{
    JS_ASSERT(!global->nativeEmpty()); 
    JS_ASSERT(key != JSProto_Null);
    JS_ASSERT(ctor);
    JS_ASSERT(proto);

    RootedId id(cx, NameToId(ClassName(key, cx)));
    JS_ASSERT(!global->nativeLookup(cx, id));

    if (!global->addDataProperty(cx, id, constructorPropertySlot(key), 0))
        return false;

    global->setConstructor(key, ObjectValue(*ctor));
    global->setPrototype(key, ObjectValue(*proto));
    global->setConstructorPropertySlot(key, ObjectValue(*ctor));

    types::AddTypePropertyId(cx, global, id, ObjectValue(*ctor));
    return true;
}

GlobalObject *
GlobalObject::create(JSContext *cx, const Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);
    JS_ASSERT(clasp->trace == JS_GlobalObjectTraceHook);

    JSObject *obj = NewObjectWithGivenProto(cx, clasp, nullptr, nullptr, SingletonObject);
    if (!obj)
        return nullptr;

    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());

    cx->compartment()->initGlobal(*global);

    if (!global->setVarObj(cx))
        return nullptr;
    if (!global->setDelegate(cx))
        return nullptr;

    return global;
}

 bool
GlobalObject::getOrCreateEval(JSContext *cx, Handle<GlobalObject*> global,
                              MutableHandleObject eval)
{
    if (!global->getOrCreateObjectPrototype(cx))
        return false;
    eval.set(&global->getSlot(EVAL).toObject());
    return true;
}

bool
GlobalObject::valueIsEval(Value val)
{
    Value eval = getSlot(EVAL);
    return eval.isObject() && eval == val;
}

 bool
GlobalObject::initStandardClasses(JSContext *cx, Handle<GlobalObject*> global)
{
    
    if (!JSObject::defineProperty(cx, global, cx->names().undefined, UndefinedHandleValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return false;
    }

    for (size_t k = 0; k < JSProto_LIMIT; ++k) {
        if (!ensureConstructor(cx, global, static_cast<JSProtoKey>(k)))
            return false;
    }
    return true;
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

 bool
GlobalObject::warnOnceAbout(JSContext *cx, HandleObject obj, uint32_t slot, unsigned errorNumber)
{
    Rooted<GlobalObject*> global(cx, &obj->global());
    HeapSlot &v = global->getSlotRef(slot);
    if (v.isUndefined()) {
        if (!JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING, js_GetErrorMessage, nullptr,
                                          errorNumber))
        {
            return false;
        }
        v.init(global, HeapSlot::Slot, slot, BooleanValue(true));
    }
    return true;
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
    if (!blankProto || !blankProto->setDelegate(cx))
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

    return JSObject::defineProperty(cx, ctor, cx->names().prototype,
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
        (void) global->compartment()->removeDebuggee(cx, global);
        return false;
    }
    return true;
}

 JSObject *
GlobalObject::getOrCreateForOfPICObject(JSContext *cx, Handle<GlobalObject *> global)
{
    assertSameCompartment(cx, global);
    JSObject *forOfPIC = global->getForOfPICObject();
    if (forOfPIC)
        return forOfPIC;

    forOfPIC = ForOfPIC::createForOfPICObject(cx, global);
    if (!forOfPIC)
        return nullptr;
    global->setReservedSlot(FOR_OF_PIC_CHAIN, ObjectValue(*forOfPIC));
    return forOfPIC;
}

bool
GlobalObject::hasRegExpStatics() const
{
    return !getSlot(REGEXP_STATICS).isUndefined();
}

RegExpStatics *
GlobalObject::getRegExpStatics(ExclusiveContext *cx) const
{
    MOZ_ASSERT(cx);
    Rooted<GlobalObject*> self(cx, const_cast<GlobalObject*>(this));

    JSObject *resObj = nullptr;
    const Value &val = this->getSlot(REGEXP_STATICS);
    if (!val.isObject()) {
        MOZ_ASSERT(val.isUndefined());
        resObj = RegExpStatics::create(cx, self);
        if (!resObj)
            return nullptr;

        self->initSlot(REGEXP_STATICS, ObjectValue(*resObj));
    } else {
        resObj = &val.toObject();
    }
    return static_cast<RegExpStatics*>(resObj->getPrivate( 1));
}

RegExpStatics *
GlobalObject::getAlreadyCreatedRegExpStatics() const
{
    const Value &val = this->getSlot(REGEXP_STATICS);
    MOZ_ASSERT(val.isObject());
    return static_cast<RegExpStatics*>(val.toObject().getPrivate( 1));
}

bool
GlobalObject::getSelfHostedFunction(JSContext *cx, HandleAtom selfHostedName, HandleAtom name,
                                    unsigned nargs, MutableHandleValue funVal)
{
    RootedId shId(cx, AtomToId(selfHostedName));
    RootedObject holder(cx, cx->global()->intrinsicsHolder());

    if (cx->global()->maybeGetIntrinsicValue(shId, funVal.address()))
        return true;

    JSFunction *fun = NewFunction(cx, NullPtr(), nullptr, nargs, JSFunction::INTERPRETED_LAZY,
                                  holder, name, JSFunction::ExtendedFinalizeKind, SingletonObject);
    if (!fun)
        return false;
    fun->setIsSelfHostedBuiltin();
    fun->setExtendedSlot(0, StringValue(selfHostedName));
    funVal.setObject(*fun);

    return cx->global()->addIntrinsicValue(cx, shId, funVal);
}

bool
GlobalObject::addIntrinsicValue(JSContext *cx, HandleId id, HandleValue value)
{
    RootedObject holder(cx, intrinsicsHolder());

    uint32_t slot = holder->slotSpan();
    RootedShape last(cx, holder->lastProperty());
    Rooted<UnownedBaseShape*> base(cx, last->base()->unowned());

    StackShape child(base, id, slot, 0, 0);
    RootedShape shape(cx, cx->compartment()->propertyTree.getChild(cx, last, child));
    if (!shape)
        return false;

    if (!JSObject::setLastProperty(cx, holder, shape))
        return false;

    holder->setSlot(shape->slot(), value);
    return true;
}
