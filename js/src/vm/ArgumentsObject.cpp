






#include "jsgc.h"
#include "jsinfer.h"
#include "jsinterp.h"

#include "vm/GlobalObject.h"
#include "vm/MethodGuard.h"
#include "vm/Stack.h"
#include "vm/Xdr.h"

#include "jsobjinlines.h"

#include "gc/Barrier-inl.h"
#include "vm/ArgumentsObject-inl.h"

using namespace js;
using namespace js::gc;

struct PutArg
{
    PutArg(JSCompartment *comp, ArgumentsObject &argsobj)
      : compartment(comp), argsobj(argsobj), dst(argsobj.data()->slots) {}
    JSCompartment *compartment;
    ArgumentsObject &argsobj;
    HeapValue *dst;
    bool operator()(unsigned i, Value *src) {
        JS_ASSERT(dst->isUndefined());
        if (!argsobj.isElementDeleted(i))
            dst->set(compartment, *src);
        ++dst;
        return true;
    }
};

void
js_PutArgsObject(StackFrame *fp)
{
    ArgumentsObject &argsobj = fp->argsObj();
    if (argsobj.isNormalArguments()) {
        JS_ASSERT(argsobj.maybeStackFrame() == fp);
        JSCompartment *comp = fp->compartment();
        fp->forEachCanonicalActualArg(PutArg(comp, argsobj));
        argsobj.setStackFrame(NULL);
    } else {
        JS_ASSERT(!argsobj.maybeStackFrame());
    }
}

ArgumentsObject *
ArgumentsObject::create(JSContext *cx, uint32_t argc, HandleObject callee)
{
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);
    JS_ASSERT(!callee->toFunction()->hasRest());

    RootedObject proto(cx, callee->global().getOrCreateObjectPrototype(cx));
    if (!proto)
        return NULL;

    RootedTypeObject type(cx);

    type = proto->getNewType(cx);
    if (!type)
        return NULL;

    bool strict = callee->toFunction()->inStrictMode();
    Class *clasp = strict ? &StrictArgumentsObjectClass : &NormalArgumentsObjectClass;

    RootedShape emptyArgumentsShape(cx);
    emptyArgumentsShape =
        EmptyShape::getInitialShape(cx, clasp, proto,
                                    proto->getParent(), FINALIZE_KIND,
                                    BaseShape::INDEXED);
    if (!emptyArgumentsShape)
        return NULL;

    unsigned numDeletedWords = NumWordsForBitArrayOfLength(argc);
    unsigned numBytes = offsetof(ArgumentsData, slots) +
                        numDeletedWords * sizeof(size_t) +
                        argc * sizeof(Value);

    ArgumentsData *data = (ArgumentsData *)cx->malloc_(numBytes);
    if (!data)
        return NULL;

    data->callee.init(ObjectValue(*callee));
    for (HeapValue *vp = data->slots; vp != data->slots + argc; vp++)
        vp->init(UndefinedValue());
    data->deletedBits = (size_t *)(data->slots + argc);
    ClearAllBitArrayElements(data->deletedBits, numDeletedWords);

    
    JSObject *obj = JSObject::create(cx, FINALIZE_KIND, emptyArgumentsShape, type, NULL);
    if (!obj)
        return NULL;

    ArgumentsObject &argsobj = obj->asArguments();

    JS_ASSERT(UINT32_MAX > (uint64_t(argc) << PACKED_BITS_COUNT));
    argsobj.initInitialLength(argc);
    argsobj.initData(data);
    argsobj.setStackFrame(NULL);

    JS_ASSERT(argsobj.numFixedSlots() >= NormalArgumentsObject::RESERVED_SLOTS);
    JS_ASSERT(argsobj.numFixedSlots() >= StrictArgumentsObject::RESERVED_SLOTS);

    return &argsobj;
}

ArgumentsObject *
ArgumentsObject::create(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(fp->script()->needsArgsObj());

    ArgumentsObject *argsobj = ArgumentsObject::create(cx, fp->numActualArgs(),
                                                       RootedObject(cx, &fp->callee()));
    if (!argsobj)
        return NULL;

    




    if (argsobj->isStrictArguments())
        fp->forEachCanonicalActualArg(PutArg(cx->compartment, *argsobj));
    else
        argsobj->setStackFrame(fp);

    fp->initArgsObj(*argsobj);
    return argsobj;
}

ArgumentsObject *
ArgumentsObject::createUnexpected(JSContext *cx, StackFrame *fp)
{
    ArgumentsObject *argsobj = create(cx, fp->numActualArgs(), RootedObject(cx, &fp->callee()));
    if (!argsobj)
        return NULL;

    fp->forEachCanonicalActualArg(PutArg(cx->compartment, *argsobj));
    return argsobj;
}

static JSBool
args_delProperty(JSContext *cx, HandleObject obj, HandleId id, Value *vp)
{
    ArgumentsObject &argsobj = obj->asArguments();
    if (JSID_IS_INT(id)) {
        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg)) {
            argsobj.setElement(arg, UndefinedValue());
            argsobj.markElementDeleted(arg);
        }
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        argsobj.markLengthOverridden();
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom)) {
        argsobj.asNormalArguments().clearCallee();
    }
    return true;
}

static JSBool
ArgGetter(JSContext *cx, HandleObject obj, HandleId id, Value *vp)
{
    if (!obj->isNormalArguments())
        return true;

    NormalArgumentsObject &argsobj = obj->asNormalArguments();
    if (JSID_IS_INT(id)) {
        



        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg)) {
            if (StackFrame *fp = argsobj.maybeStackFrame()) {
                JS_ASSERT_IF(arg < fp->numFormalArgs(), fp->script()->formalIsAliased(arg));
                *vp = fp->canonicalActualArg(arg);
            } else {
                *vp = argsobj.element(arg);
            }
        }
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (!argsobj.hasOverriddenLength())
            vp->setInt32(argsobj.initialLength());
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom));
        const Value &v = argsobj.callee();
        if (!v.isMagic(JS_OVERWRITTEN_CALLEE))
            *vp = v;
    }
    return true;
}

static JSBool
ArgSetter(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp)
{
    if (!obj->isNormalArguments())
        return true;

    NormalArgumentsObject &argsobj = obj->asNormalArguments();

    if (JSID_IS_INT(id)) {
        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength()) {
            if (StackFrame *fp = argsobj.maybeStackFrame()) {
                JSScript *script = fp->functionScript();
                JS_ASSERT(script->needsArgsObj());
                if (arg < fp->numFormalArgs()) {
                    JS_ASSERT(fp->script()->formalIsAliased(arg));
                    types::TypeScript::SetArgument(cx, script, arg, *vp);
                }
                fp->canonicalActualArg(arg) = *vp;
                return true;
            }
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom) ||
                  JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom));
    }

    







    RootedValue value(cx);
    return baseops::DeleteGeneric(cx, obj, id, value.address(), false) &&
           baseops::DefineProperty(cx, obj, id, vp, NULL, NULL, JSPROP_ENUMERATE);
}

static JSBool
args_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
             JSObject **objp)
{
    *objp = NULL;

    Rooted<NormalArgumentsObject*> argsobj(cx, &obj->asNormalArguments());

    unsigned attrs = JSPROP_SHARED | JSPROP_SHADOWABLE;
    if (JSID_IS_INT(id)) {
        uint32_t arg = uint32_t(JSID_TO_INT(id));
        if (arg >= argsobj->initialLength() || argsobj->isElementDeleted(arg))
            return true;

        attrs |= JSPROP_ENUMERATE;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (argsobj->hasOverriddenLength())
            return true;
    } else {
        if (!JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom))
            return true;

        if (argsobj->callee().isMagic(JS_OVERWRITTEN_CALLEE))
            return true;
    }

    Value undef = UndefinedValue();
    if (!baseops::DefineProperty(cx, argsobj, id, &undef, ArgGetter, ArgSetter, attrs))
        return JS_FALSE;

    *objp = argsobj;
    return true;
}

bool
NormalArgumentsObject::optimizedGetElem(JSContext *cx, StackFrame *fp, const Value &elem, Value *vp)
{
    JS_ASSERT(!fp->hasArgsObj());

    
    if (elem.isInt32()) {
        int32_t i = elem.toInt32();
        if (i >= 0 && uint32_t(i) < fp->numActualArgs()) {
            *vp = fp->canonicalActualArg(i);
            return true;
        }
    }

    

    jsid id;
    if (!ValueToId(cx, elem, &id))
        return false;

    if (JSID_IS_INT(id)) {
        int32_t i = JSID_TO_INT(id);
        if (i >= 0 && uint32_t(i) < fp->numActualArgs()) {
            *vp = fp->canonicalActualArg(i);
            return true;
        }
    }

    if (id == NameToId(cx->runtime->atomState.lengthAtom)) {
        *vp = Int32Value(fp->numActualArgs());
        return true;
    }

    if (id == NameToId(cx->runtime->atomState.calleeAtom)) {
        *vp = ObjectValue(fp->callee());
        return true;
    }

    JSObject *proto = fp->global().getOrCreateObjectPrototype(cx);
    if (!proto)
        return false;

    return proto->getGeneric(cx, RootedId(cx, id), vp);
}

static JSBool
args_enumerate(JSContext *cx, HandleObject obj)
{
    Rooted<NormalArgumentsObject*> argsobj(cx, &obj->asNormalArguments());
    RootedId id(cx);

    



    int argc = int(argsobj->initialLength());
    for (int i = -2; i != argc; i++) {
        id = (i == -2)
             ? NameToId(cx->runtime->atomState.lengthAtom)
             : (i == -1)
             ? NameToId(cx->runtime->atomState.calleeAtom)
             : INT_TO_JSID(i);

        JSObject *pobj;
        JSProperty *prop;
        if (!baseops::LookupProperty(cx, argsobj, id, &pobj, &prop))
            return false;
    }
    return true;
}

static JSBool
StrictArgGetter(JSContext *cx, HandleObject obj, HandleId id, Value *vp)
{
    if (!obj->isStrictArguments())
        return true;

    StrictArgumentsObject &argsobj = obj->asStrictArguments();

    if (JSID_IS_INT(id)) {
        



        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg))
            *vp = argsobj.element(arg);
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom));
        if (!argsobj.hasOverriddenLength())
            vp->setInt32(argsobj.initialLength());
    }
    return true;
}

static JSBool
StrictArgSetter(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp)
{
    if (!obj->isStrictArguments())
        return true;

    Rooted<StrictArgumentsObject*> argsobj(cx, &obj->asStrictArguments());

    if (JSID_IS_INT(id)) {
        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj->initialLength()) {
            argsobj->setElement(arg, *vp);
            return true;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom));
    }

    





    RootedValue value(cx);
    return baseops::DeleteGeneric(cx, argsobj, id, value.address(), strict) &&
           baseops::SetPropertyHelper(cx, argsobj, id, 0, vp, strict);
}

static JSBool
strictargs_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags, JSObject **objp)
{
    *objp = NULL;

    Rooted<StrictArgumentsObject*> argsobj(cx, &obj->asStrictArguments());

    unsigned attrs = JSPROP_SHARED | JSPROP_SHADOWABLE;
    PropertyOp getter = StrictArgGetter;
    StrictPropertyOp setter = StrictArgSetter;

    if (JSID_IS_INT(id)) {
        uint32_t arg = uint32_t(JSID_TO_INT(id));
        if (arg >= argsobj->initialLength() || argsobj->isElementDeleted(arg))
            return true;

        attrs |= JSPROP_ENUMERATE;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (argsobj->hasOverriddenLength())
            return true;
    } else {
        if (!JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom) &&
            !JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom)) {
            return true;
        }

        attrs = JSPROP_PERMANENT | JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
        getter = CastAsPropertyOp(argsobj->global().getThrowTypeError());
        setter = CastAsStrictPropertyOp(argsobj->global().getThrowTypeError());
    }

    Value undef = UndefinedValue();
    if (!baseops::DefineProperty(cx, argsobj, id, &undef, getter, setter, attrs))
        return false;

    *objp = argsobj;
    return true;
}

static JSBool
strictargs_enumerate(JSContext *cx, HandleObject obj)
{
    Rooted<StrictArgumentsObject*> argsobj(cx, &obj->asStrictArguments());

    



    JSObject *pobj;
    JSProperty *prop;
    RootedId id(cx);

    
    id = NameToId(cx->runtime->atomState.lengthAtom);
    if (!baseops::LookupProperty(cx, argsobj, id, &pobj, &prop))
        return false;

    
    id = NameToId(cx->runtime->atomState.calleeAtom);
    if (!baseops::LookupProperty(cx, argsobj, id, &pobj, &prop))
        return false;

    
    id = NameToId(cx->runtime->atomState.callerAtom);
    if (!baseops::LookupProperty(cx, argsobj, id, &pobj, &prop))
        return false;

    for (uint32_t i = 0, argc = argsobj->initialLength(); i < argc; i++) {
        id = INT_TO_JSID(i);
        if (!baseops::LookupProperty(cx, argsobj, id, &pobj, &prop))
            return false;
    }

    return true;
}

static void
args_finalize(FreeOp *fop, JSObject *obj)
{
    fop->free_(reinterpret_cast<void *>(obj->asArguments().data()));
}

static void
args_trace(JSTracer *trc, JSObject *obj)
{
    ArgumentsObject &argsobj = obj->asArguments();
    ArgumentsData *data = argsobj.data();
    MarkValue(trc, &data->callee, js_callee_str);
    MarkValueRange(trc, argsobj.initialLength(), data->slots, js_arguments_str);

    








#if JS_HAS_GENERATORS
    StackFrame *fp = argsobj.maybeStackFrame();
    if (fp && fp->isFloatingGenerator())
        MarkObject(trc, &js_FloatingFrameToGenerator(fp)->obj, "generator object");
#endif
}







Class js::NormalArgumentsObjectClass = {
    "Arguments",
    JSCLASS_NEW_RESOLVE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(NormalArgumentsObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object) |
    JSCLASS_FOR_OF_ITERATION,
    JS_PropertyStub,         
    args_delProperty,
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    args_enumerate,
    reinterpret_cast<JSResolveOp>(args_resolve),
    JS_ConvertStub,
    args_finalize,           
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    args_trace,
    {
        NULL,       
        NULL,       
        NULL,       
        JS_ElementIteratorStub,
        NULL,       
        false,      
    }
};






Class js::StrictArgumentsObjectClass = {
    "Arguments",
    JSCLASS_NEW_RESOLVE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(StrictArgumentsObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object) |
    JSCLASS_FOR_OF_ITERATION,
    JS_PropertyStub,         
    args_delProperty,
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    strictargs_enumerate,
    reinterpret_cast<JSResolveOp>(strictargs_resolve),
    JS_ConvertStub,
    args_finalize,           
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    args_trace,
    {
        NULL,       
        NULL,       
        NULL,       
        JS_ElementIteratorStub,
        NULL,       
        false,      
    }
};
