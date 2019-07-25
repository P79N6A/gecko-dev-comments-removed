








































#include "jscompartment.h"
#include "jsiter.h"
#include "jsscope.h"

#include "GlobalObject.h"
#include "ScopeObject.h"
#include "Xdr.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "ScopeObject-inl.h"

using namespace js;
using namespace js::types;

void
js_PutCallObject(StackFrame *fp)
{
    CallObject &callobj = fp->callObj().asCall();
    JS_ASSERT(callobj.maybeStackFrame() == fp);
    JS_ASSERT_IF(fp->isEvalFrame(), fp->isStrictEvalFrame());
    JS_ASSERT(fp->isEvalFrame() == callobj.isForEval());

    JSScript *script = fp->script();
    Bindings &bindings = script->bindings;

    if (callobj.isForEval()) {
        JS_ASSERT(script->strictModeCode);
        JS_ASSERT(bindings.numArgs() == 0);

        
        callobj.copyValues(0, NULL, bindings.numVars(), fp->slots());
    } else {
        JSFunction *fun = fp->fun();
        JS_ASSERT(script == callobj.getCalleeFunction()->script());
        JS_ASSERT(script == fun->script());

        unsigned n = bindings.count();
        if (n > 0) {
            uint32_t nvars = bindings.numVars();
            uint32_t nargs = bindings.numArgs();
            JS_ASSERT(fun->nargs == nargs);
            JS_ASSERT(nvars + nargs == n);

            JSScript *script = fun->script();
            if (script->bindingsAccessedDynamically
#ifdef JS_METHODJIT
                || script->debugMode
#endif
                ) {
                callobj.copyValues(nargs, fp->formalArgs(), nvars, fp->slots());
            } else {
                






                uint32_t nclosed = script->numClosedArgs();
                for (uint32_t i = 0; i < nclosed; i++) {
                    uint32_t e = script->getClosedArg(i);
#ifdef JS_GC_ZEAL
                    callobj.setArg(e, fp->formalArg(e));
#else
                    callobj.initArgUnchecked(e, fp->formalArg(e));
#endif
                }

                nclosed = script->numClosedVars();
                for (uint32_t i = 0; i < nclosed; i++) {
                    uint32_t e = script->getClosedVar(i);
#ifdef JS_GC_ZEAL
                    callobj.setVar(e, fp->slots()[e]);
#else
                    callobj.initVarUnchecked(e, fp->slots()[e]);
#endif
                }
            }

            



            types::TypeScriptNesting *nesting = script->nesting();
            if (nesting && script->isOuterFunction) {
                nesting->argArray = callobj.argArray();
                nesting->varArray = callobj.varArray();
            }
        }

        
        if (js_IsNamedLambda(fun)) {
            JSObject &env = callobj.enclosingScope();
            JS_ASSERT(env.asDeclEnv().maybeStackFrame() == fp);
            env.setPrivate(NULL);
        }
    }

    callobj.setStackFrame(NULL);
}







CallObject *
CallObject::create(JSContext *cx, JSScript *script, HandleObject enclosing, HandleObject callee)
{
    RootedVarShape shape(cx);
    shape = script->bindings.callObjectShape(cx);
    if (shape == NULL)
        return NULL;

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots() + 1);

    RootedVarTypeObject type(cx);
    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, shape, &slots))
        return NULL;

    RootedVarObject obj(cx, JSObject::create(cx, kind, shape, type, slots));
    if (!obj)
        return NULL;

    




    if (&enclosing->global() != obj->getParent()) {
        JS_ASSERT(obj->getParent() == NULL);
        if (!JSObject::setParent(cx, obj, RootedVarObject(cx, &enclosing->global())))
            return NULL;
    }

#ifdef DEBUG
    JS_ASSERT(!obj->inDictionaryMode());
    for (Shape::Range r = obj->lastProperty(); !r.empty(); r.popFront()) {
        const Shape &s = r.front();
        if (s.hasSlot()) {
            JS_ASSERT(s.slot() + 1 == obj->slotSpan());
            break;
        }
    }
#endif

    if (!obj->asScope().setEnclosingScope(cx, enclosing))
        return NULL;

    JS_ASSERT_IF(callee, callee->isFunction());
    obj->initFixedSlot(CALLEE_SLOT, ObjectOrNullValue(callee));

    



    if (obj->lastProperty()->extensibleParents()) {
        if (!obj->generateOwnShape(cx))
            return NULL;
    }

    return &obj->asCall();
}

CallObject *
CallObject::createForFunction(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(fp->isNonEvalFunctionFrame());
    JS_ASSERT(!fp->hasCallObj());

    RootedVarObject scopeChain(cx, fp->scopeChain());
    JS_ASSERT_IF(scopeChain->isWith() || scopeChain->isBlock() || scopeChain->isCall(),
                 scopeChain->getPrivate() != fp);

    



    RootedVarAtom lambdaName(cx, CallObjectLambdaName(fp->fun()));
    if (lambdaName) {
        scopeChain = DeclEnvObject::create(cx, fp);
        if (!scopeChain)
            return NULL;

        if (!DefineNativeProperty(cx, scopeChain, ATOM_TO_JSID(lambdaName),
                                  ObjectValue(fp->callee()), NULL, NULL,
                                  JSPROP_PERMANENT | JSPROP_READONLY, 0, 0)) {
            return NULL;
        }
    }

    CallObject *callobj = create(cx, fp->script(), scopeChain, RootedVarObject(cx, &fp->callee()));
    if (!callobj)
        return NULL;

    callobj->setStackFrame(fp);
    fp->setScopeChainWithOwnCallObj(*callobj);
    return callobj;
}

CallObject *
CallObject::createForStrictEval(JSContext *cx, StackFrame *fp)
{
    CallObject *callobj = create(cx, fp->script(), fp->scopeChain(), RootedVarObject(cx));
    if (!callobj)
        return NULL;

    callobj->setStackFrame(fp);
    fp->setScopeChainWithOwnCallObj(*callobj);
    return callobj;
}

JSBool
CallObject::getArgOp(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16_t) JSID_TO_INT(id) == JSID_TO_INT(id));
    unsigned i = (uint16_t) JSID_TO_INT(id);

    DebugOnly<JSScript *> script = callobj.getCalleeFunction()->script();
    JS_ASSERT_IF(!callobj.compartment()->debugMode(), script->argLivesInCallObject(i));

    if (StackFrame *fp = callobj.maybeStackFrame())
        *vp = fp->formalArg(i);
    else
        *vp = callobj.arg(i);
    return true;
}

JSBool
CallObject::setArgOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16_t) JSID_TO_INT(id) == JSID_TO_INT(id));
    unsigned i = (uint16_t) JSID_TO_INT(id);

    JSScript *script = callobj.getCalleeFunction()->script();
    JS_ASSERT_IF(!callobj.compartment()->debugMode(), script->argLivesInCallObject(i));

    if (StackFrame *fp = callobj.maybeStackFrame())
        fp->formalArg(i) = *vp;
    else
        callobj.setArg(i, *vp);

    if (!script->ensureHasTypes(cx))
        return false;

    TypeScript::SetArgument(cx, script, i, *vp);

    return true;
}

JSBool
CallObject::getVarOp(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16_t) JSID_TO_INT(id) == JSID_TO_INT(id));
    unsigned i = (uint16_t) JSID_TO_INT(id);

    DebugOnly<JSScript *> script = callobj.getCalleeFunction()->script();
    JS_ASSERT_IF(!callobj.compartment()->debugMode(), script->varIsAliased(i));

    if (StackFrame *fp = callobj.maybeStackFrame())
        *vp = fp->varSlot(i);
    else
        *vp = callobj.var(i);

    
    if (vp->isMagic(JS_OPTIMIZED_ARGUMENTS))
        *vp = UndefinedValue();

    return true;
}

JSBool
CallObject::setVarOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16_t) JSID_TO_INT(id) == JSID_TO_INT(id));
    unsigned i = (uint16_t) JSID_TO_INT(id);

    JSScript *script = callobj.getCalleeFunction()->script();
    JS_ASSERT_IF(!callobj.compartment()->debugMode(), script->varIsAliased(i));

    if (StackFrame *fp = callobj.maybeStackFrame())
        fp->varSlot(i) = *vp;
    else
        callobj.setVar(i, *vp);

    if (!script->ensureHasTypes(cx))
        return false;

    TypeScript::SetLocal(cx, script, i, *vp);
    return true;
}

bool
CallObject::containsVarOrArg(PropertyName *name, Value *vp, JSContext *cx)
{
    jsid id = ATOM_TO_JSID(name);
    const Shape *shape = nativeLookup(cx, id);
    if (!shape)
        return false;

    PropertyOp op = shape->getterOp();
    if (op != getVarOp && op != getArgOp)
        return false;

    JS_ALWAYS_TRUE(op(cx, this, INT_TO_JSID(shape->shortid()), vp));
    return true;
}

static void
call_trace(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->isCall());

    
#if JS_HAS_GENERATORS
    StackFrame *fp = (StackFrame *) obj->getPrivate();
    if (fp && fp->isFloatingGenerator())
        MarkObject(trc, &js_FloatingFrameToGenerator(fp)->obj, "generator object");
#endif
}

JS_PUBLIC_DATA(Class) js::CallClass = {
    "Call",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_IS_ANONYMOUS |
    JSCLASS_HAS_RESERVED_SLOTS(CallObject::RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    call_trace
};

Class js::DeclEnvClass = {
    js_Object_str,
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(DeclEnvObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

DeclEnvObject *
DeclEnvObject::create(JSContext *cx, StackFrame *fp)
{
    RootedVarTypeObject type(cx);
    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    RootedVarShape emptyDeclEnvShape(cx);
    emptyDeclEnvShape = EmptyShape::getInitialShape(cx, &DeclEnvClass, NULL,
                                                    &fp->global(), FINALIZE_KIND);
    if (!emptyDeclEnvShape)
        return NULL;

    RootedVarObject obj(cx, JSObject::create(cx, FINALIZE_KIND, emptyDeclEnvShape, type, NULL));
    if (!obj)
        return NULL;

    obj->setPrivate(fp);
    if (!obj->asScope().setEnclosingScope(cx, fp->scopeChain()))
        return NULL;

    return &obj->asDeclEnv();
}

WithObject *
WithObject::create(JSContext *cx, StackFrame *fp, HandleObject proto, HandleObject enclosing,
                   uint32_t depth)
{
    RootedVarTypeObject type(cx);
    type = proto->getNewType(cx);
    if (!type)
        return NULL;

    RootedVarShape emptyWithShape(cx);
    emptyWithShape = EmptyShape::getInitialShape(cx, &WithClass, proto,
                                                 &enclosing->global(), FINALIZE_KIND);
    if (!emptyWithShape)
        return NULL;

    RootedVarObject obj(cx, JSObject::create(cx, FINALIZE_KIND, emptyWithShape, type, NULL));
    if (!obj)
        return NULL;

    if (!obj->asScope().setEnclosingScope(cx, enclosing))
        return NULL;

    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(depth));
    obj->setPrivate(js_FloatingFrameIfGenerator(cx, fp));

    JSObject *thisp = proto->thisObject(cx);
    if (!thisp)
        return NULL;

    obj->setFixedSlot(THIS_SLOT, ObjectValue(*thisp));

    return &obj->asWith();
}

static JSBool
with_LookupGeneric(JSContext *cx, JSObject *obj, jsid id, JSObject **objp, JSProperty **propp)
{
    
    unsigned flags = cx->resolveFlags;
    if (flags == RESOLVE_INFER)
        flags = js_InferFlags(cx, flags);
    flags |= JSRESOLVE_WITH;
    JSAutoResolveFlags rf(cx, flags);
    return obj->asWith().object().lookupGeneric(cx, id, objp, propp);
}

static JSBool
with_LookupProperty(JSContext *cx, JSObject *obj, PropertyName *name, JSObject **objp, JSProperty **propp)
{
    return with_LookupGeneric(cx, obj, ATOM_TO_JSID(name), objp, propp);
}

static JSBool
with_LookupElement(JSContext *cx, JSObject *obj, uint32_t index, JSObject **objp,
                   JSProperty **propp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static JSBool
with_LookupSpecial(JSContext *cx, JSObject *obj, SpecialId sid, JSObject **objp, JSProperty **propp)
{
    return with_LookupGeneric(cx, obj, SPECIALID_TO_JSID(sid), objp, propp);
}

static JSBool
with_GetGeneric(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    return obj->asWith().object().getGeneric(cx, id, vp);
}

static JSBool
with_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, PropertyName *name, Value *vp)
{
    return with_GetGeneric(cx, obj, receiver, ATOM_TO_JSID(name), vp);
}

static JSBool
with_GetElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t index, Value *vp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static JSBool
with_GetSpecial(JSContext *cx, JSObject *obj, JSObject *receiver, SpecialId sid, Value *vp)
{
    return with_GetGeneric(cx, obj, receiver, SPECIALID_TO_JSID(sid), vp);
}

static JSBool
with_SetGeneric(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    return obj->asWith().object().setGeneric(cx, id, vp, strict);
}

static JSBool
with_SetProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp, JSBool strict)
{
    return obj->asWith().object().setProperty(cx, name, vp, strict);
}

static JSBool
with_SetElement(JSContext *cx, JSObject *obj, uint32_t index, Value *vp, JSBool strict)
{
    return obj->asWith().object().setElement(cx, index, vp, strict);
}

static JSBool
with_SetSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *vp, JSBool strict)
{
    return obj->asWith().object().setSpecial(cx, sid, vp, strict);
}

static JSBool
with_GetGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp)
{
    return obj->asWith().object().getGenericAttributes(cx, id, attrsp);
}

static JSBool
with_GetPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp)
{
    return obj->asWith().object().getPropertyAttributes(cx, name, attrsp);
}

static JSBool
with_GetElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp)
{
    return obj->asWith().object().getElementAttributes(cx, index, attrsp);
}

static JSBool
with_GetSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp)
{
    return obj->asWith().object().getSpecialAttributes(cx, sid, attrsp);
}

static JSBool
with_SetGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp)
{
    return obj->asWith().object().setGenericAttributes(cx, id, attrsp);
}

static JSBool
with_SetPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp)
{
    return obj->asWith().object().setPropertyAttributes(cx, name, attrsp);
}

static JSBool
with_SetElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp)
{
    return obj->asWith().object().setElementAttributes(cx, index, attrsp);
}

static JSBool
with_SetSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp)
{
    return obj->asWith().object().setSpecialAttributes(cx, sid, attrsp);
}

static JSBool
with_DeleteProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *rval, JSBool strict)
{
    return obj->asWith().object().deleteProperty(cx, name, rval, strict);
}

static JSBool
with_DeleteElement(JSContext *cx, JSObject *obj, uint32_t index, Value *rval, JSBool strict)
{
    return obj->asWith().object().deleteElement(cx, index, rval, strict);
}

static JSBool
with_DeleteSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *rval, JSBool strict)
{
    return obj->asWith().object().deleteSpecial(cx, sid, rval, strict);
}

static JSBool
with_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
               Value *statep, jsid *idp)
{
    return obj->asWith().object().enumerate(cx, enum_op, statep, idp);
}

static JSType
with_TypeOf(JSContext *cx, JSObject *obj)
{
    return JSTYPE_OBJECT;
}

static JSObject *
with_ThisObject(JSContext *cx, JSObject *obj)
{
    return &obj->asWith().withThis();
}

Class js::WithClass = {
    "With",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(WithObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    JS_NULL_CLASS_EXT,
    {
        with_LookupGeneric,
        with_LookupProperty,
        with_LookupElement,
        with_LookupSpecial,
        NULL,             
        NULL,             
        NULL,             
        NULL,             
        with_GetGeneric,
        with_GetProperty,
        with_GetElement,
        NULL,             
        with_GetSpecial,
        with_SetGeneric,
        with_SetProperty,
        with_SetElement,
        with_SetSpecial,
        with_GetGenericAttributes,
        with_GetPropertyAttributes,
        with_GetElementAttributes,
        with_GetSpecialAttributes,
        with_SetGenericAttributes,
        with_SetPropertyAttributes,
        with_SetElementAttributes,
        with_SetSpecialAttributes,
        with_DeleteProperty,
        with_DeleteElement,
        with_DeleteSpecial,
        with_Enumerate,
        with_TypeOf,
        with_ThisObject,
        NULL,             
    }
};

ClonedBlockObject *
ClonedBlockObject::create(JSContext *cx, Handle<StaticBlockObject*> block, StackFrame *fp)
{
    RootedVarTypeObject type(cx);
    type = block->getNewType(cx);
    if (!type)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, block->lastProperty(), &slots))
        return NULL;

    RootedVarShape shape(cx);
    shape = block->lastProperty();

    RootedVarObject obj(cx, JSObject::create(cx, FINALIZE_KIND, shape, type, slots));
    if (!obj)
        return NULL;

    
    if (&fp->global() != obj->getParent()) {
        JS_ASSERT(obj->getParent() == NULL);
        if (!JSObject::setParent(cx, obj, RootedVarObject(cx, &fp->global())))
            return NULL;
    }

    JS_ASSERT(!obj->inDictionaryMode());
    JS_ASSERT(obj->slotSpan() >= block->slotCount() + RESERVED_SLOTS);

    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(block->stackDepth()));
    obj->setPrivate(js_FloatingFrameIfGenerator(cx, fp));

    if (obj->lastProperty()->extensibleParents() && !obj->generateOwnShape(cx))
        return NULL;

    return &obj->asClonedBlock();
}

void
ClonedBlockObject::put(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    JS_ASSERT(maybeStackFrame() == js_FloatingFrameIfGenerator(cx, fp));

    uint32_t count = slotCount();
    uint32_t depth = stackDepth();

    
    JS_ASSERT(depth <= uint32_t(cx->regs().sp - fp->base()));
    JS_ASSERT(count <= uint32_t(cx->regs().sp - fp->base() - depth));

    
    JS_ASSERT(count >= 1);

    copySlotRange(RESERVED_SLOTS, fp->base() + depth, count);

    
    setPrivate(NULL);
    fp->setScopeChainNoCallObj(enclosingScope());
}

static JSBool
block_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    




    ClonedBlockObject &block = obj->asClonedBlock();
    unsigned index = (unsigned) JSID_TO_INT(id);

    JS_ASSERT_IF(!block.compartment()->debugMode(), block.staticBlock().isAliased(index));

    if (StackFrame *fp = block.maybeStackFrame()) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->numFixed() + block.stackDepth();
        JS_ASSERT(index < fp->numSlots());
        *vp = fp->slots()[index];
        return true;
    }

    
    JS_ASSERT(block.closedSlot(index) == *vp);
    return true;
}

static JSBool
block_setProperty(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    ClonedBlockObject &block = obj->asClonedBlock();
    unsigned index = (unsigned) JSID_TO_INT(id);

    JS_ASSERT_IF(!block.compartment()->debugMode(), block.staticBlock().isAliased(index));

    if (StackFrame *fp = block.maybeStackFrame()) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->numFixed() + block.stackDepth();
        JS_ASSERT(index < fp->numSlots());
        fp->slots()[index] = *vp;
        return true;
    }

    



    return true;
}

bool
ClonedBlockObject::containsVar(PropertyName *name, Value *vp, JSContext *cx)
{
    jsid id = ATOM_TO_JSID(name);
    const Shape *shape = nativeLookup(cx, id);
    if (!shape)
        return false;

    JS_ASSERT(shape->getterOp() == block_getProperty);
    JS_ALWAYS_TRUE(block_getProperty(cx, this, INT_TO_JSID(shape->shortid()), vp));
    return true;
}

StaticBlockObject *
StaticBlockObject::create(JSContext *cx)
{
    RootedVarTypeObject type(cx);
    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    RootedVarShape emptyBlockShape(cx);
    emptyBlockShape = EmptyShape::getInitialShape(cx, &BlockClass, NULL, NULL, FINALIZE_KIND);
    if (!emptyBlockShape)
        return NULL;

    JSObject *obj = JSObject::create(cx, FINALIZE_KIND, emptyBlockShape, type, NULL);
    if (!obj)
        return NULL;

    return &obj->asStaticBlock();
}

const Shape *
StaticBlockObject::addVar(JSContext *cx, jsid id, int index, bool *redeclared)
{
    JS_ASSERT(JSID_IS_ATOM(id) || (JSID_IS_INT(id) && JSID_TO_INT(id) == index));

    *redeclared = false;

    
    Shape **spp;
    if (Shape::search(cx, lastProperty(), id, &spp, true)) {
        *redeclared = true;
        return NULL;
    }

    



    uint32_t slot = JSSLOT_FREE(&BlockClass) + index;
    return addPropertyInternal(cx, id, block_getProperty, block_setProperty,
                               slot, JSPROP_ENUMERATE | JSPROP_PERMANENT,
                               Shape::HAS_SHORTID, index, spp,
                                false);
}

Class js::BlockClass = {
    "Block",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(BlockObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#define NO_PARENT_INDEX UINT32_MAX






static uint32_t
FindObjectIndex(JSScript *script, StaticBlockObject *maybeBlock)
{
    if (!maybeBlock || !JSScript::isValidOffset(script->objectsOffset))
        return NO_PARENT_INDEX;

    ObjectArray *objects = script->objects();
    HeapPtrObject *vector = objects->vector;
    unsigned length = objects->length;
    for (unsigned i = 0; i < length; ++i) {
        if (vector[i] == maybeBlock)
            return i;
    }

    return NO_PARENT_INDEX;
}

template<XDRMode mode>
bool
js::XDRStaticBlockObject(XDRState<mode> *xdr, JSScript *script, StaticBlockObject **objp)
{
    

    JSContext *cx = xdr->cx();

    StaticBlockObject *obj = NULL;
    uint32_t parentId = 0;
    uint32_t count = 0;
    uint32_t depthAndCount = 0;
    if (mode == XDR_ENCODE) {
        obj = *objp;
        parentId = FindObjectIndex(script, obj->enclosingBlock());
        uint32_t depth = obj->stackDepth();
        JS_ASSERT(depth <= UINT16_MAX);
        count = obj->slotCount();
        JS_ASSERT(count <= UINT16_MAX);
        depthAndCount = (depth << 16) | uint16_t(count);
    }

    
    if (!xdr->codeUint32(&parentId))
        return false;

    if (mode == XDR_DECODE) {
        obj = StaticBlockObject::create(cx);
        if (!obj)
            return false;
        *objp = obj;

        obj->setEnclosingBlock(parentId == NO_PARENT_INDEX
                               ? NULL
                               : &script->getObject(parentId)->asStaticBlock());
    }

    AutoObjectRooter tvr(cx, obj);

    if (!xdr->codeUint32(&depthAndCount))
        return false;

    if (mode == XDR_DECODE) {
        uint32_t depth = uint16_t(depthAndCount >> 16);
        count = uint16_t(depthAndCount);
        obj->setStackDepth(depth);

        



        for (unsigned i = 0; i < count; i++) {
            JSAtom *atom;
            if (!XDRAtom(xdr, &atom))
                return false;

            
            jsid id = atom != cx->runtime->emptyString
                      ? ATOM_TO_JSID(atom)
                      : INT_TO_JSID(i);

            bool redeclared;
            if (!obj->addVar(cx, id, i, &redeclared)) {
                JS_ASSERT(!redeclared);
                return false;
            }

            uint32_t aliased;
            if (!xdr->codeUint32(&aliased))
                return false;

            JS_ASSERT(aliased == 0 || aliased == 1);
            obj->setAliased(i, !!aliased);
        }
    } else {
        AutoShapeVector shapes(cx);
        if (!shapes.growBy(count))
            return false;

        for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
            const Shape *shape = &r.front();
            shapes[shape->shortid()] = shape;
        }

        



        for (unsigned i = 0; i < count; i++) {
            const Shape *shape = shapes[i];
            JS_ASSERT(shape->getter() == block_getProperty);
            JS_ASSERT(unsigned(shape->shortid()) == i);

            jsid propid = shape->propid();
            JS_ASSERT(JSID_IS_ATOM(propid) || JSID_IS_INT(propid));

            
            JSAtom *atom = JSID_IS_ATOM(propid)
                           ? JSID_TO_ATOM(propid)
                           : cx->runtime->emptyString;

            if (!XDRAtom(xdr, &atom))
                return false;

            uint32_t aliased = obj->isAliased(i);
            if (!xdr->codeUint32(&aliased))
                return false;
        }
    }
    return true;
}

template bool
js::XDRStaticBlockObject(XDRState<XDR_ENCODE> *xdr, JSScript *script, StaticBlockObject **objp);

template bool
js::XDRStaticBlockObject(XDRState<XDR_DECODE> *xdr, JSScript *script, StaticBlockObject **objp);

JSObject *
js::CloneStaticBlockObject(JSContext *cx, StaticBlockObject &srcBlock,
                           const AutoObjectVector &objects, JSScript *src)
{
    

    StaticBlockObject *clone = StaticBlockObject::create(cx);
    if (!clone)
        return NULL;

    uint32_t parentId = FindObjectIndex(src, srcBlock.enclosingBlock());
    clone->setEnclosingBlock(parentId == NO_PARENT_INDEX
                             ? NULL
                             : &objects[parentId]->asStaticBlock());

    clone->setStackDepth(srcBlock.stackDepth());

    
    AutoShapeVector shapes(cx);
    if (!shapes.growBy(srcBlock.slotCount()))
        return NULL;
    for (Shape::Range r = srcBlock.lastProperty()->all(); !r.empty(); r.popFront())
        shapes[r.front().shortid()] = &r.front();

    for (const Shape **p = shapes.begin(); p != shapes.end(); ++p) {
        jsid id = (*p)->propid();
        unsigned i = (*p)->shortid();

        bool redeclared;
        if (!clone->addVar(cx, id, i, &redeclared)) {
            JS_ASSERT(!redeclared);
            return NULL;
        }

        clone->setAliased(i, srcBlock.isAliased(i));
    }

    return clone;
}
