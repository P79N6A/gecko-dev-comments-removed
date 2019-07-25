







































#include "GlobalObject.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsmath.h"
#include "json.h"
#include "jsweakmap.h"

#include "builtin/MapObject.h"
#include "builtin/RegExp.h"
#include "frontend/BytecodeEmitter.h"
#include "vm/GlobalObject-inl.h"

#include "jsobjinlines.h"
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

JSObject *
GlobalObject::initFunctionAndObjectClasses(JSContext *cx)
{
    RootedVar<GlobalObject*> self(cx, this);

    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    JS_ASSERT(isNative());

    




    

    
    if (!cx->globalObject)
        JS_SetGlobalObject(cx, self);

    RootedVarObject objectProto(cx);

    



    objectProto = NewObjectWithGivenProto(cx, &ObjectClass, NULL, self);
    if (!objectProto || !objectProto->setSingletonType(cx))
        return NULL;

    




    if (!objectProto->setNewTypeUnknown(cx))
        return NULL;

    
    RootedVarFunction functionProto(cx);
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

        JSScript *script =
            JSScript::NewScript(cx, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, JSVERSION_DEFAULT);
        if (!script)
            return NULL;
        script->noScriptRval = true;
        script->code[0] = JSOP_STOP;
        script->code[1] = SRC_NULL;
        functionProto->initScript(script);
        functionProto->getType(cx)->interpretedFunction = functionProto;
        script->setFunction(functionProto);

        if (!functionProto->setSingletonType(cx))
            return NULL;

        




        if (!functionProto->setNewTypeUnknown(cx))
            return NULL;
    }

    
    RootedVarFunction objectCtor(cx);
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

    
    RootedVarFunction functionCtor(cx);
    {
        JSObject *ctor =
            NewObjectWithGivenProto(cx, &FunctionClass, functionProto, self);
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
        !DefinePropertiesAndBrand(cx, objectProto, object_props, object_methods) ||
        !DefinePropertiesAndBrand(cx, objectCtor, NULL, object_static_methods) ||
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

    

    
    jsid id = NameToId(cx->runtime->atomState.evalAtom);
    JSObject *evalobj = js_DefineFunction(cx, self, id, eval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return NULL;
    self->setOriginalEval(evalobj);

    
    RootedVarFunction throwTypeError(cx);
    throwTypeError = js_NewFunction(cx, NULL, ThrowTypeError, 0, 0, self, NULL);
    if (!throwTypeError)
        return NULL;
    if (!throwTypeError->preventExtensions(cx))
        return NULL;
    self->setThrowTypeError(throwTypeError);

    







    if (self->shouldSplicePrototype(cx) && !self->splicePrototype(cx, objectProto))
        return NULL;

    



    js_CallNewScriptHook(cx, functionProto->script(), functionProto);
    return functionProto;
}

GlobalObject *
GlobalObject::create(JSContext *cx, Class *clasp)
{
    JS_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    RootedVar<GlobalObject*> obj(cx);

    JSObject *obj_ = NewObjectWithGivenProto(cx, clasp, NULL, NULL);
    if (!obj_)
        return NULL;
    obj = &obj_->asGlobal();

    if (!obj->setSingletonType(cx) || !obj->setVarObj(cx))
        return NULL;

    
    JSObject *res = RegExpStatics::create(cx, obj);
    if (!res)
        return NULL;
    obj->initSlot(REGEXP_STATICS, ObjectValue(*res));
    obj->initFlags(0);

    return obj;
}

 bool
GlobalObject::initStandardClasses(JSContext *cx, Handle<GlobalObject*> global)
{
    JSAtomState &state = cx->runtime->atomState;

    
    if (!global->defineProperty(cx, state.typeAtoms[JSTYPE_VOID], UndefinedValue(),
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
           js_InitXMLClasses(cx, global) &&
#endif
#if JS_HAS_GENERATORS
           js_InitIteratorClasses(cx, global) &&
#endif
           js_InitDateClass(cx, global) &&
           js_InitWeakMapClass(cx, global) &&
           js_InitProxyClass(cx, global) &&
           js_InitMapClass(cx, global) &&
           js_InitSetClass(cx, global);
}

void
GlobalObject::clear(JSContext *cx)
{
    for (int key = JSProto_Null; key < JSProto_LIMIT * 3; key++)
        setSlot(key, UndefinedValue());

    
    getRegExpStatics()->clear();

    
    setSlot(RUNTIME_CODEGEN_ENABLED, UndefinedValue());

    




    setSlot(EVAL, UndefinedValue());
    setSlot(THROWTYPEERROR, UndefinedValue());

    



    int32_t flags = getSlot(FLAGS).toInt32();
    flags |= FLAGS_CLEARED;
    setSlot(FLAGS, Int32Value(flags));

    



    cx->runtime->newObjectCache.purge();

#ifdef JS_METHODJIT
    




    for (gc::CellIter i(cx->compartment, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compileAndGo && script->hasJITCode() && script->hasClearedGlobal()) {
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
    RootedVarObject self(cx, this);
    return js_NewFunction(cx, NULL, ctor, length, JSFUN_CONSTRUCTOR, self, name, kind);
}

static JSObject *
CreateBlankProto(JSContext *cx, Class *clasp, JSObject &proto, GlobalObject &global)
{
    JS_ASSERT(clasp != &ObjectClass);
    JS_ASSERT(clasp != &FunctionClass);

    JSObject *blankProto = NewObjectWithGivenProto(cx, clasp, &proto, &global);
    if (!blankProto || !blankProto->setSingletonType(cx))
        return NULL;

    return blankProto;
}

JSObject *
GlobalObject::createBlankPrototype(JSContext *cx, Class *clasp)
{
    JSObject *objectProto = getOrCreateObjectPrototype(cx);
    if (!objectProto)
        return NULL;

    return CreateBlankProto(cx, clasp, *objectProto, *this);
}

JSObject *
GlobalObject::createBlankPrototypeInheriting(JSContext *cx, Class *clasp, JSObject &proto)
{
    return CreateBlankProto(cx, clasp, proto, *this);
}

bool
LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto)
{
    RootObject ctorRoot(cx, &ctor);
    RootObject protoRoot(cx, &proto);

    return ctor->defineProperty(cx, cx->runtime->atomState.classPrototypeAtom,
                                ObjectValue(*proto), JS_PropertyStub, JS_StrictPropertyStub,
                                JSPROP_PERMANENT | JSPROP_READONLY) &&
           proto->defineProperty(cx, cx->runtime->atomState.constructorAtom,
                                 ObjectValue(*ctor), JS_PropertyStub, JS_StrictPropertyStub, 0);
}

bool
DefinePropertiesAndBrand(JSContext *cx, JSObject *obj, JSPropertySpec *ps, JSFunctionSpec *fs)
{
    RootObject root(cx, &obj);

    if ((ps && !JS_DefineProperties(cx, obj, ps)) || (fs && !JS_DefineFunctions(cx, obj, fs)))
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
