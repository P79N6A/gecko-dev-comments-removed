





#include "vm/GlobalObject.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfriendapi.h"
#include "jsmath.h"
#include "json.h"
#include "jsprototypes.h"
#include "jsweakmap.h"

#include "builtin/Eval.h"
#if EXPOSE_INTL_API
# include "builtin/Intl.h"
#endif
#include "builtin/MapObject.h"
#include "builtin/Object.h"
#include "builtin/RegExp.h"
#include "builtin/SIMD.h"
#include "builtin/SymbolObject.h"
#include "builtin/TypedObject.h"
#include "builtin/WeakSetObject.h"
#include "vm/HelperThreads.h"
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
    MOZ_CRASH("js_InitViaClassSpec() should not be called.");
}

static const ProtoTableEntry protoTable[JSProto_LIMIT] = {
#define INIT_FUNC(name,code,init,clasp) { clasp, init },
#define INIT_FUNC_DUMMY(name,code,init,clasp) { nullptr, nullptr },
    JS_FOR_PROTOTYPES(INIT_FUNC, INIT_FUNC_DUMMY)
#undef INIT_FUNC_DUMMY
#undef INIT_FUNC
};

JS_FRIEND_API(const js::Class *)
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

    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    if (key == JSProto_Function && global->getPrototype(JSProto_Object).isUndefined())
        return resolveConstructor(cx, global, JSProto_Object);

    
    
    
    RootedObject proto(cx);
    if (clasp->spec.createPrototype) {
        proto = clasp->spec.createPrototype(cx, key);
        if (!proto)
            return false;

        global->setPrototype(key, ObjectValue(*proto));
    }

    
    RootedObject ctor(cx, clasp->spec.createConstructor(cx, key));
    if (!ctor)
        return false;

    RootedId id(cx, NameToId(ClassName(key, cx)));
    if (clasp->spec.shouldDefineConstructor()) {
        if (!global->addDataProperty(cx, id, constructorPropertySlot(key), 0))
            return false;
    }

    global->setConstructor(key, ObjectValue(*ctor));
    global->setConstructorPropertySlot(key, ObjectValue(*ctor));

    
    
    if (!StandardClassIsDependent(key)) {
        if (const JSFunctionSpec *funs = clasp->spec.prototypeFunctions) {
            if (!JS_DefineFunctions(cx, proto, funs))
                return false;
        }
        if (const JSPropertySpec *props = clasp->spec.prototypeProperties) {
            if (!JS_DefineProperties(cx, proto, props))
                return false;
        }
        if (const JSFunctionSpec *funs = clasp->spec.constructorFunctions) {
            if (!JS_DefineFunctions(cx, ctor, funs))
                return false;
        }
    }

    
    if (proto && !LinkConstructorAndPrototype(cx, ctor, proto))
        return false;

    
    if (clasp->spec.finishInit && !clasp->spec.finishInit(cx, ctor, proto))
        return false;

    if (clasp->spec.shouldDefineConstructor()) {
        
        
        types::AddTypePropertyId(cx, global, id, ObjectValue(*ctor));
    }

    return true;
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

    if (!global->setQualifiedVarObj(cx))
        return nullptr;
    if (!global->setUnqualifiedVarObj(cx))
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
js::DefinePropertiesAndFunctions(JSContext *cx, HandleObject obj,
                                 const JSPropertySpec *ps, const JSFunctionSpec *fs)
{
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
