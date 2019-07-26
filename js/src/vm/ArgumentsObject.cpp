






#include "jsgc.h"
#include "jsinfer.h"
#include "jsinterp.h"

#include "vm/GlobalObject.h"
#include "vm/Stack.h"
#include "vm/Xdr.h"

#include "jsobjinlines.h"

#include "gc/Barrier-inl.h"
#include "vm/Stack-inl.h"
#include "vm/ArgumentsObject-inl.h"

using namespace js;
using namespace js::gc;

static void
CopyStackFrameArguments(const AbstractFramePtr frame, HeapValue *dst)
{
    JS_ASSERT_IF(frame.isStackFrame(), !frame.asStackFrame()->runningInIon());

    unsigned numActuals = frame.numActualArgs();
    unsigned numFormals = frame.callee().nargs;

    
    Value *src = frame.formals();
    Value *end = src + numFormals;
    while (src != end)
        (dst++)->init(*src++);

    
    if (numFormals < numActuals) {
        src = frame.actuals() + numFormals;
        end = src + (numActuals - numFormals);
        while (src != end)
            (dst++)->init(*src++);
    }
}

 void
ArgumentsObject::MaybeForwardToCallObject(AbstractFramePtr frame, JSObject *obj, ArgumentsData *data)
{
    RawScript script = frame.script();
    if (frame.fun()->isHeavyweight() && script->argsObjAliasesFormals()) {
        obj->initFixedSlot(MAYBE_CALL_SLOT, ObjectValue(frame.callObj()));
        for (AliasedFormalIter fi(script); fi; fi++)
            data->args[fi.frameIndex()] = MagicValue(JS_FORWARD_TO_CALL_OBJECT);
    }
}

struct CopyFrameArgs
{
    AbstractFramePtr frame_;

    CopyFrameArgs(AbstractFramePtr frame)
      : frame_(frame)
    { }

    void copyArgs(JSContext *, HeapValue *dst) const {
        CopyStackFrameArguments(frame_, dst);
    }

    



    void maybeForwardToCallObject(JSObject *obj, ArgumentsData *data) {
        ArgumentsObject::MaybeForwardToCallObject(frame_, obj, data);
    }
};

struct CopyStackIterArgs
{
    StackIter &iter_;

    CopyStackIterArgs(StackIter &iter)
      : iter_(iter)
    { }

    void copyArgs(JSContext *cx, HeapValue *dstBase) const {
        if (!iter_.isIon()) {
            CopyStackFrameArguments(iter_.abstractFramePtr(), dstBase);
            return;
        }

        
        iter_.ionForEachCanonicalActualArg(cx, CopyToHeap(dstBase));

        
        unsigned numActuals = iter_.numActualArgs();
        unsigned numFormals = iter_.callee()->nargs;
        if (numActuals < numFormals) {
            HeapValue *dst = dstBase + numActuals, *dstEnd = dstBase + numFormals;
            while (dst != dstEnd)
                (dst++)->init(UndefinedValue());
        }
    }

    



    void maybeForwardToCallObject(JSObject *obj, ArgumentsData *data) {
        if (!iter_.isIon())
            ArgumentsObject::MaybeForwardToCallObject(iter_.abstractFramePtr(), obj, data);
    }
};

template <typename CopyArgs>
 ArgumentsObject *
ArgumentsObject::create(JSContext *cx, HandleScript script, HandleFunction callee, unsigned numActuals,
                        CopyArgs &copy)
{
    AssertCanGC();

    RootedObject proto(cx, callee->global().getOrCreateObjectPrototype(cx));
    if (!proto)
        return NULL;

    bool strict = callee->strict();
    Class *clasp = strict ? &StrictArgumentsObjectClass : &NormalArgumentsObjectClass;

    RootedTypeObject type(cx, proto->getNewType(cx, clasp));
    if (!type)
        return NULL;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, TaggedProto(proto),
                                                      proto->getParent(), FINALIZE_KIND,
                                                      BaseShape::INDEXED));
    if (!shape)
        return NULL;

    unsigned numFormals = callee->nargs;
    unsigned numDeletedWords = NumWordsForBitArrayOfLength(numActuals);
    unsigned numArgs = Max(numActuals, numFormals);
    unsigned numBytes = offsetof(ArgumentsData, args) +
                        numDeletedWords * sizeof(size_t) +
                        numArgs * sizeof(Value);

    ArgumentsData *data = (ArgumentsData *)cx->malloc_(numBytes);
    if (!data)
        return NULL;

    data->numArgs = numArgs;
    data->callee.init(ObjectValue(*callee.get()));
    data->script = script;

    
    HeapValue *dst = data->args, *dstEnd = data->args + numArgs;
    copy.copyArgs(cx, dst);

    data->deletedBits = reinterpret_cast<size_t *>(dstEnd);
    ClearAllBitArrayElements(data->deletedBits, numDeletedWords);

    RawObject obj = JSObject::create(cx, FINALIZE_KIND, GetInitialHeap(GenericObject, clasp),
                                     shape, type, NULL);
    if (!obj) {
        js_free(data);
        return NULL;
    }

    obj->initFixedSlot(INITIAL_LENGTH_SLOT, Int32Value(numActuals << PACKED_BITS_COUNT));
    obj->initFixedSlot(DATA_SLOT, PrivateValue(data));

    copy.maybeForwardToCallObject(obj, data);

    ArgumentsObject &argsobj = obj->asArguments();
    JS_ASSERT(argsobj.initialLength() == numActuals);
    JS_ASSERT(!argsobj.hasOverriddenLength());
    return &argsobj;
}

ArgumentsObject *
ArgumentsObject::createExpected(JSContext *cx, AbstractFramePtr frame)
{
    JS_ASSERT(frame.script()->needsArgsObj());
    RootedScript script(cx, frame.script());
    RootedFunction callee(cx, &frame.callee());
    CopyFrameArgs copy(frame);
    ArgumentsObject *argsobj = create(cx, script, callee, frame.numActualArgs(), copy);
    if (!argsobj)
        return NULL;

    frame.initArgsObj(*argsobj);
    return argsobj;
}

ArgumentsObject *
ArgumentsObject::createUnexpected(JSContext *cx, StackIter &iter)
{
    RootedScript script(cx, iter.script());
    RootedFunction callee(cx, iter.callee());
    CopyStackIterArgs copy(iter);
    return create(cx, script, callee, iter.numActualArgs(), copy);
}

ArgumentsObject *
ArgumentsObject::createUnexpected(JSContext *cx, AbstractFramePtr frame)
{
    RootedScript script(cx, frame.script());
    RootedFunction callee(cx, &frame.callee());
    CopyFrameArgs copy(frame);
    return create(cx, script, callee, frame.numActualArgs(), copy);
}

static JSBool
args_delProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    ArgumentsObject &argsobj = obj->asArguments();
    if (JSID_IS_INT(id)) {
        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg))
            argsobj.markElementDeleted(arg);
    } else if (JSID_IS_ATOM(id, cx->names().length)) {
        argsobj.markLengthOverridden();
    } else if (JSID_IS_ATOM(id, cx->names().callee)) {
        argsobj.asNormalArguments().clearCallee();
    }
    return true;
}

static JSBool
ArgGetter(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    if (!obj->isNormalArguments())
        return true;

    NormalArgumentsObject &argsobj = obj->asNormalArguments();
    if (JSID_IS_INT(id)) {
        



        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg))
            vp.set(argsobj.element(arg));
    } else if (JSID_IS_ATOM(id, cx->names().length)) {
        if (!argsobj.hasOverriddenLength())
            vp.setInt32(argsobj.initialLength());
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->names().callee));
        if (!argsobj.callee().isMagic(JS_OVERWRITTEN_CALLEE))
            vp.set(argsobj.callee());
    }
    return true;
}

static JSBool
ArgSetter(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, MutableHandleValue vp)
{
    if (!obj->isNormalArguments())
        return true;

    unsigned attrs;
    if (!baseops::GetAttributes(cx, obj, id, &attrs))
        return false;
    JS_ASSERT(!(attrs & JSPROP_READONLY));
    attrs &= (JSPROP_ENUMERATE | JSPROP_PERMANENT); 

    NormalArgumentsObject &argsobj = obj->asNormalArguments();
    RootedScript script(cx, argsobj.containingScript());

    if (JSID_IS_INT(id)) {
        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg)) {
            argsobj.setElement(arg, vp);
            if (arg < script->function()->nargs) {
                if (!script->ensureHasTypes(cx))
                    return false;
                types::TypeScript::SetArgument(cx, script, arg, vp);
            }
            return true;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->names().length) || JSID_IS_ATOM(id, cx->names().callee));
    }

    







    RootedValue value(cx);
    return baseops::DeleteGeneric(cx, obj, id, &value, false) &&
           baseops::DefineGeneric(cx, obj, id, vp, NULL, NULL, attrs);
}

static JSBool
args_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
             MutableHandleObject objp)
{
    objp.set(NULL);

    Rooted<NormalArgumentsObject*> argsobj(cx, &obj->asNormalArguments());

    unsigned attrs = JSPROP_SHARED | JSPROP_SHADOWABLE;
    if (JSID_IS_INT(id)) {
        uint32_t arg = uint32_t(JSID_TO_INT(id));
        if (arg >= argsobj->initialLength() || argsobj->isElementDeleted(arg))
            return true;

        attrs |= JSPROP_ENUMERATE;
    } else if (JSID_IS_ATOM(id, cx->names().length)) {
        if (argsobj->hasOverriddenLength())
            return true;
    } else {
        if (!JSID_IS_ATOM(id, cx->names().callee))
            return true;

        if (argsobj->callee().isMagic(JS_OVERWRITTEN_CALLEE))
            return true;
    }

    RootedValue undef(cx, UndefinedValue());
    if (!baseops::DefineGeneric(cx, argsobj, id, undef, ArgGetter, ArgSetter, attrs))
        return JS_FALSE;

    objp.set(argsobj);
    return true;
}

static JSBool
args_enumerate(JSContext *cx, HandleObject obj)
{
    Rooted<NormalArgumentsObject*> argsobj(cx, &obj->asNormalArguments());
    RootedId id(cx);

    



    int argc = int(argsobj->initialLength());
    for (int i = -2; i != argc; i++) {
        id = (i == -2)
             ? NameToId(cx->names().length)
             : (i == -1)
             ? NameToId(cx->names().callee)
             : INT_TO_JSID(i);

        RootedObject pobj(cx);
        RootedShape prop(cx);
        if (!baseops::LookupProperty<CanGC>(cx, argsobj, id, &pobj, &prop))
            return false;
    }
    return true;
}

static JSBool
StrictArgGetter(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    if (!obj->isStrictArguments())
        return true;

    StrictArgumentsObject &argsobj = obj->asStrictArguments();

    if (JSID_IS_INT(id)) {
        



        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj.initialLength() && !argsobj.isElementDeleted(arg))
            vp.set(argsobj.element(arg));
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->names().length));
        if (!argsobj.hasOverriddenLength())
            vp.setInt32(argsobj.initialLength());
    }
    return true;
}

static JSBool
StrictArgSetter(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, MutableHandleValue vp)
{
    if (!obj->isStrictArguments())
        return true;

    unsigned attrs;
    if (!baseops::GetAttributes(cx, obj, id, &attrs))
        return false;
    JS_ASSERT(!(attrs & JSPROP_READONLY));
    attrs &= (JSPROP_ENUMERATE | JSPROP_PERMANENT); 

    Rooted<StrictArgumentsObject*> argsobj(cx, &obj->asStrictArguments());

    if (JSID_IS_INT(id)) {
        unsigned arg = unsigned(JSID_TO_INT(id));
        if (arg < argsobj->initialLength()) {
            argsobj->setElement(arg, vp);
            return true;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->names().length));
    }

    





    RootedValue value(cx);
    return baseops::DeleteGeneric(cx, argsobj, id, &value, strict) &&
           baseops::DefineGeneric(cx, argsobj, id, vp, NULL, NULL, attrs);
}

static JSBool
strictargs_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                   MutableHandleObject objp)
{
    objp.set(NULL);

    Rooted<StrictArgumentsObject*> argsobj(cx, &obj->asStrictArguments());

    unsigned attrs = JSPROP_SHARED | JSPROP_SHADOWABLE;
    PropertyOp getter = StrictArgGetter;
    StrictPropertyOp setter = StrictArgSetter;

    if (JSID_IS_INT(id)) {
        uint32_t arg = uint32_t(JSID_TO_INT(id));
        if (arg >= argsobj->initialLength() || argsobj->isElementDeleted(arg))
            return true;

        attrs |= JSPROP_ENUMERATE;
    } else if (JSID_IS_ATOM(id, cx->names().length)) {
        if (argsobj->hasOverriddenLength())
            return true;
    } else {
        if (!JSID_IS_ATOM(id, cx->names().callee) && !JSID_IS_ATOM(id, cx->names().caller))
            return true;

        attrs = JSPROP_PERMANENT | JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
        getter = CastAsPropertyOp(argsobj->global().getThrowTypeError());
        setter = CastAsStrictPropertyOp(argsobj->global().getThrowTypeError());
    }

    RootedValue undef(cx, UndefinedValue());
    if (!baseops::DefineGeneric(cx, argsobj, id, undef, getter, setter, attrs))
        return false;

    objp.set(argsobj);
    return true;
}

static JSBool
strictargs_enumerate(JSContext *cx, HandleObject obj)
{
    Rooted<StrictArgumentsObject*> argsobj(cx, &obj->asStrictArguments());

    



    RootedObject pobj(cx);
    RootedShape prop(cx);
    RootedId id(cx);

    
    id = NameToId(cx->names().length);
    if (!baseops::LookupProperty<CanGC>(cx, argsobj, id, &pobj, &prop))
        return false;

    
    id = NameToId(cx->names().callee);
    if (!baseops::LookupProperty<CanGC>(cx, argsobj, id, &pobj, &prop))
        return false;

    
    id = NameToId(cx->names().caller);
    if (!baseops::LookupProperty<CanGC>(cx, argsobj, id, &pobj, &prop))
        return false;

    for (uint32_t i = 0, argc = argsobj->initialLength(); i < argc; i++) {
        id = INT_TO_JSID(i);
        if (!baseops::LookupProperty<CanGC>(cx, argsobj, id, &pobj, &prop))
            return false;
    }

    return true;
}

void
ArgumentsObject::finalize(FreeOp *fop, RawObject obj)
{
    fop->free_(reinterpret_cast<void *>(obj->asArguments().data()));
}

void
ArgumentsObject::trace(JSTracer *trc, RawObject obj)
{
    ArgumentsObject &argsobj = obj->asArguments();
    ArgumentsData *data = argsobj.data();
    MarkValue(trc, &data->callee, js_callee_str);
    MarkValueRange(trc, data->numArgs, data->args, js_arguments_str);
    MarkScriptUnbarriered(trc, &data->script, "script");
}







Class js::NormalArgumentsObjectClass = {
    "Arguments",
    JSCLASS_NEW_RESOLVE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(NormalArgumentsObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object) | JSCLASS_BACKGROUND_FINALIZE,
    JS_PropertyStub,         
    args_delProperty,
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    args_enumerate,
    reinterpret_cast<JSResolveOp>(args_resolve),
    JS_ConvertStub,
    ArgumentsObject::finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    ArgumentsObject::trace,
    {
        NULL,       
        NULL,       
        NULL,       
        false,      
    }
};






Class js::StrictArgumentsObjectClass = {
    "Arguments",
    JSCLASS_NEW_RESOLVE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(StrictArgumentsObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object) | JSCLASS_BACKGROUND_FINALIZE,
    JS_PropertyStub,         
    args_delProperty,
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    strictargs_enumerate,
    reinterpret_cast<JSResolveOp>(strictargs_resolve),
    JS_ConvertStub,
    ArgumentsObject::finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    ArgumentsObject::trace,
    {
        NULL,       
        NULL,       
        NULL,       
        false,      
    }
};
