






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



StaticScopeIter::StaticScopeIter(JSObject *obj)
  : obj(obj), onNamedLambda(false)
{
    JS_ASSERT_IF(obj, obj->isStaticBlock() || obj->isFunction());
}

bool
StaticScopeIter::done() const
{
    return obj == NULL;
}

void
StaticScopeIter::operator++(int)
{
    if (obj->isStaticBlock()) {
        obj = obj->asStaticBlock().enclosingStaticScope();
    } else if (onNamedLambda || !obj->toFunction()->isNamedLambda()) {
        onNamedLambda = false;
        obj = obj->toFunction()->script()->enclosingStaticScope();
    } else {
        onNamedLambda = true;
    }
    JS_ASSERT_IF(obj, obj->isStaticBlock() || obj->isFunction());
    JS_ASSERT_IF(onNamedLambda, obj->isFunction());
}

bool
StaticScopeIter::hasDynamicScopeObject() const
{
    return obj->isStaticBlock()
           ? obj->asStaticBlock().needsClone()
           : obj->toFunction()->isHeavyweight();
}

Shape *
StaticScopeIter::scopeShape() const
{
    JS_ASSERT(hasDynamicScopeObject());
    JS_ASSERT(type() != NAMED_LAMBDA);
    return type() == BLOCK ? block().lastProperty() : funScript()->bindings.lastBinding;
}

StaticScopeIter::Type
StaticScopeIter::type() const
{
    if (onNamedLambda)
        return NAMED_LAMBDA;
    return obj->isStaticBlock() ? BLOCK : FUNCTION;
}

StaticBlockObject &
StaticScopeIter::block() const
{
    JS_ASSERT(type() == BLOCK);
    return obj->asStaticBlock();
}

JSScript *
StaticScopeIter::funScript() const
{
    JS_ASSERT(type() == FUNCTION);
    return obj->toFunction()->script();
}



StaticScopeIter
js::ScopeCoordinateToStaticScope(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->code && pc < script->code + script->length);
    JS_ASSERT(JOF_OPTYPE(*pc) == JOF_SCOPECOORD);

    uint32_t blockIndex = GET_UINT32_INDEX(pc + 2 * sizeof(uint16_t));
    JSObject *innermostStaticScope;
    if (blockIndex == UINT32_MAX)
        innermostStaticScope = script->function();
    else
        innermostStaticScope = &script->getObject(blockIndex)->asStaticBlock();

    StaticScopeIter ssi(innermostStaticScope);
    ScopeCoordinate sc(pc);
    while (true) {
        if (ssi.hasDynamicScopeObject()) {
            if (!sc.hops)
                break;
            sc.hops--;
        }
        ssi++;
    }
    return ssi;
}

PropertyName *
js::ScopeCoordinateName(JSRuntime *rt, JSScript *script, jsbytecode *pc)
{
    Shape::Range r = ScopeCoordinateToStaticScope(script, pc).scopeShape()->all();
    ScopeCoordinate sc(pc);
    while (r.front().slot() != sc.slot)
        r.popFront();
    jsid id = r.front().propid();

    
    if (!JSID_IS_ATOM(id))
        return rt->atomState.emptyAtom;
    return JSID_TO_ATOM(id)->asPropertyName();
}







CallObject *
CallObject::create(JSContext *cx, HandleShape shape, HandleTypeObject type, HeapSlot *slots,
                   HandleObject global)
{
    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    JS_ASSERT(CanBeFinalizedInBackground(kind, &CallClass));
    kind = gc::GetBackgroundAllocKind(kind);

    RootedObject obj(cx, JSObject::create(cx, kind, shape, type, slots));
    if (!obj)
        return NULL;

    



    if (obj->lastProperty()->extensibleParents()) {
        if (!obj->generateOwnShape(cx))
            return NULL;
    }

    JS_ASSERT(obj->isDelegate());

    return &obj->asCall();
}







CallObject *
CallObject::create(JSContext *cx, JSScript *script, HandleObject enclosing, HandleFunction callee)
{
    RootedShape shape(cx, script->bindings.callObjectShape(cx));
    if (!shape)
        return NULL;

    RootedTypeObject type(cx, cx->compartment->getEmptyType(cx));
    if (!type)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, shape, &slots))
        return NULL;

    RootedObject global(cx, &enclosing->global());
    RootedObject obj(cx, CallObject::create(cx, shape, type, slots, global));
    if (!obj)
        return NULL;

    JS_ASSERT(enclosing->global() == obj->global());

    obj->asScope().setEnclosingScope(enclosing);
    obj->initFixedSlot(CALLEE_SLOT, ObjectOrNullValue(callee));

    return &obj->asCall();
}

CallObject *
CallObject::createForFunction(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(fp->isNonEvalFunctionFrame());

    RootedObject scopeChain(cx, fp->scopeChain());

    



    if (fp->fun()->isNamedLambda()) {
        scopeChain = DeclEnvObject::create(cx, fp);
        if (!scopeChain)
            return NULL;
    }

    RootedScript script(cx, fp->script());
    Rooted<JSFunction*> callee(cx, &fp->callee());
    CallObject *callobj = create(cx, script, scopeChain, callee);
    if (!callobj)
        return NULL;

    
    if (script->bindingsAccessedDynamically) {
        Value *formals = fp->formals();
        for (unsigned slot = 0, n = fp->fun()->nargs; slot < n; ++slot)
            callobj->setFormal(slot, formals[slot]);
    } else if (unsigned n = script->numClosedArgs()) {
        Value *formals = fp->formals();
        for (unsigned i = 0; i < n; ++i) {
            uint32_t slot = script->getClosedArg(i);
            callobj->setFormal(slot, formals[slot]);
        }
    }

    return callobj;
}

void
CallObject::copyUnaliasedValues(StackFrame *fp)
{
    JS_ASSERT(fp->script() == callee().script());
    JSScript *script = fp->script();

    
    if (script->bindingsAccessedDynamically)
        return;

    
    for (unsigned i = 0; i < script->bindings.numArgs(); ++i) {
        if (!script->formalLivesInCallObject(i)) {
            if (script->argsObjAliasesFormals() && fp->hasArgsObj())
                setFormal(i, fp->argsObj().arg(i), DONT_CHECK_ALIASING);
            else
                setFormal(i, fp->unaliasedFormal(i, DONT_CHECK_ALIASING), DONT_CHECK_ALIASING);
        }
    }

    
    for (unsigned i = 0; i < script->bindings.numVars(); ++i) {
        if (!script->varIsAliased(i))
            setVar(i, fp->unaliasedLocal(i), DONT_CHECK_ALIASING);
    }
}

CallObject *
CallObject::createForStrictEval(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(fp->isStrictEvalFrame());
    JS_ASSERT(cx->fp() == fp);
    JS_ASSERT(cx->regs().pc == fp->script()->code);

    Rooted<JSFunction*> callee(cx, NULL);
    return create(cx, fp->script(), fp->scopeChain(), callee);
}

JS_PUBLIC_DATA(Class) js::CallClass = {
    "Call",
    JSCLASS_IS_ANONYMOUS | JSCLASS_HAS_RESERVED_SLOTS(CallObject::RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    NULL                     
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
    RootedTypeObject type(cx, cx->compartment->getEmptyType(cx));
    if (!type)
        return NULL;

    RootedShape emptyDeclEnvShape(cx);
    emptyDeclEnvShape = EmptyShape::getInitialShape(cx, &DeclEnvClass, NULL,
                                                    &fp->global(), FINALIZE_KIND,
                                                    BaseShape::DELEGATE);
    if (!emptyDeclEnvShape)
        return NULL;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, emptyDeclEnvShape, type, NULL));
    if (!obj)
        return NULL;

    obj->asScope().setEnclosingScope(fp->scopeChain());

    Rooted<jsid> id(cx, AtomToId(fp->fun()->atom));
    if (!DefineNativeProperty(cx, obj, id, ObjectValue(fp->callee()), NULL, NULL,
                              JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY,
                              0, 0)) {
        return NULL;
    }

    return &obj->asDeclEnv();
}

WithObject *
WithObject::create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth)
{
    RootedTypeObject type(cx, proto->getNewType(cx));
    if (!type)
        return NULL;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &WithClass, proto,
                                                      &enclosing->global(), FINALIZE_KIND));
    if (!shape)
        return NULL;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, shape, type, NULL));
    if (!obj)
        return NULL;

    obj->asScope().setEnclosingScope(enclosing);
    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(depth));

    JSObject *thisp = proto->thisObject(cx);
    if (!thisp)
        return NULL;

    obj->setFixedSlot(THIS_SLOT, ObjectValue(*thisp));

    return &obj->asWith();
}

static JSBool
with_LookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    return obj->asWith().object().lookupGeneric(cx, id, objp, propp);
}

static JSBool
with_LookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                    MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static JSBool
with_LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, id.address()))
        return false;
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static JSBool
with_LookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static JSBool
with_GetGeneric(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, Value *vp)
{
    return obj->asWith().object().getGeneric(cx, id, vp);
}

static JSBool
with_GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandlePropertyName name, Value *vp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static JSBool
with_GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index, Value *vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, id.address()))
        return false;
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static JSBool
with_GetSpecial(JSContext *cx, HandleObject obj, HandleObject receiver, HandleSpecialId sid, Value *vp)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static JSBool
with_SetGeneric(JSContext *cx, HandleObject obj, HandleId id, Value *vp, JSBool strict)
{
    Rooted<JSObject*> actual(cx, &obj->asWith().object());
    return actual->setGeneric(cx, actual, id, vp, strict);
}

static JSBool
with_SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, Value *vp, JSBool strict)
{
    Rooted<JSObject*> actual(cx, &obj->asWith().object());
    return actual->setProperty(cx, actual, name, vp, strict);
}

static JSBool
with_SetElement(JSContext *cx, HandleObject obj, uint32_t index, Value *vp, JSBool strict)
{
    Rooted<JSObject*> actual(cx, &obj->asWith().object());
    return actual->setElement(cx, actual, index, vp, strict);
}

static JSBool
with_SetSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, Value *vp, JSBool strict)
{
    Rooted<JSObject*> actual(cx, &obj->asWith().object());
    return actual->setSpecial(cx, actual, sid, vp, strict);
}

static JSBool
with_GetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    return obj->asWith().object().getGenericAttributes(cx, id, attrsp);
}

static JSBool
with_GetPropertyAttributes(JSContext *cx, HandleObject obj, HandlePropertyName name, unsigned *attrsp)
{
    return obj->asWith().object().getPropertyAttributes(cx, name, attrsp);
}

static JSBool
with_GetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp)
{
    return obj->asWith().object().getElementAttributes(cx, index, attrsp);
}

static JSBool
with_GetSpecialAttributes(JSContext *cx, HandleObject obj, HandleSpecialId sid, unsigned *attrsp)
{
    return obj->asWith().object().getSpecialAttributes(cx, sid, attrsp);
}

static JSBool
with_SetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    return obj->asWith().object().setGenericAttributes(cx, id, attrsp);
}

static JSBool
with_SetPropertyAttributes(JSContext *cx, HandleObject obj, HandlePropertyName name, unsigned *attrsp)
{
    return obj->asWith().object().setPropertyAttributes(cx, name, attrsp);
}

static JSBool
with_SetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp)
{
    return obj->asWith().object().setElementAttributes(cx, index, attrsp);
}

static JSBool
with_SetSpecialAttributes(JSContext *cx, HandleObject obj, HandleSpecialId sid, unsigned *attrsp)
{
    return obj->asWith().object().setSpecialAttributes(cx, sid, attrsp);
}

static JSBool
with_DeleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, Value *rval, JSBool strict)
{
    return obj->asWith().object().deleteProperty(cx, name, rval, strict);
}

static JSBool
with_DeleteElement(JSContext *cx, HandleObject obj, uint32_t index, Value *rval, JSBool strict)
{
    return obj->asWith().object().deleteElement(cx, index, rval, strict);
}

static JSBool
with_DeleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, Value *rval, JSBool strict)
{
    return obj->asWith().object().deleteSpecial(cx, sid, rval, strict);
}

static JSBool
with_Enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
               Value *statep, jsid *idp)
{
    return obj->asWith().object().enumerate(cx, enum_op, statep, idp);
}

static JSType
with_TypeOf(JSContext *cx, HandleObject obj)
{
    return JSTYPE_OBJECT;
}

static JSObject *
with_ThisObject(JSContext *cx, HandleObject obj)
{
    return &obj->asWith().withThis();
}

Class js::WithClass = {
    "With",
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
ClonedBlockObject::create(JSContext *cx, Handle<StaticBlockObject *> block, StackFrame *fp)
{
    RootedTypeObject type(cx, block->getNewType(cx));
    if (!type)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, block->lastProperty(), &slots))
        return NULL;

    RootedShape shape(cx, block->lastProperty());

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, shape, type, slots));
    if (!obj)
        return NULL;

    
    if (&fp->global() != obj->getParent()) {
        JS_ASSERT(obj->getParent() == NULL);
        Rooted<GlobalObject*> global(cx, &fp->global());
        if (!JSObject::setParent(cx, obj, global))
            return NULL;
    }

    JS_ASSERT(!obj->inDictionaryMode());
    JS_ASSERT(obj->slotSpan() >= block->slotCount() + RESERVED_SLOTS);

    obj->setReservedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*fp->scopeChain()));
    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(block->stackDepth()));

    if (obj->lastProperty()->extensibleParents() && !obj->generateOwnShape(cx))
        return NULL;

    



    Value *src = fp->base() + block->stackDepth();
    unsigned nslots = block->slotCount();
    for (unsigned i = 0; i < nslots; ++i, ++src) {
        if (block->isAliased(i))
            obj->asClonedBlock().setVar(i, *src);
    }

    JS_ASSERT(obj->isDelegate());

    return &obj->asClonedBlock();
}

void
ClonedBlockObject::copyUnaliasedValues(StackFrame *fp)
{
    StaticBlockObject &block = staticBlock();
    unsigned base = fp->script()->nfixed + block.stackDepth();
    for (unsigned i = 0; i < slotCount(); ++i) {
        if (!block.isAliased(i))
            setVar(i, fp->unaliasedLocal(base + i), DONT_CHECK_ALIASING);
    }
}

StaticBlockObject *
StaticBlockObject::create(JSContext *cx)
{
    RootedTypeObject type(cx, cx->compartment->getEmptyType(cx));
    if (!type)
        return NULL;

    RootedShape emptyBlockShape(cx);
    emptyBlockShape = EmptyShape::getInitialShape(cx, &BlockClass, NULL, NULL, FINALIZE_KIND,
                                                  BaseShape::DELEGATE);
    if (!emptyBlockShape)
        return NULL;

    JSObject *obj = JSObject::create(cx, FINALIZE_KIND, emptyBlockShape, type, NULL);
    if (!obj)
        return NULL;

    return &obj->asStaticBlock();
}

 Shape *
StaticBlockObject::addVar(JSContext *cx, Handle<StaticBlockObject*> block, HandleId id,
                          int index, bool *redeclared)
{
    JS_ASSERT(JSID_IS_ATOM(id) || (JSID_IS_INT(id) && JSID_TO_INT(id) == index));

    *redeclared = false;

    
    Shape **spp;
    if (Shape::search(cx, block->lastProperty(), id, &spp, true)) {
        *redeclared = true;
        return NULL;
    }

    



    uint32_t slot = JSSLOT_FREE(&BlockClass) + index;
    return block->addPropertyInternal(cx, id,  NULL,  NULL,
                                      slot, JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                      Shape::HAS_SHORTID, index, spp,
                                       false);
}

Class js::BlockClass = {
    "Block",
    JSCLASS_IMPLEMENTS_BARRIERS |
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

template<XDRMode mode>
bool
js::XDRStaticBlockObject(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript script,
                         StaticBlockObject **objp)
{
    

    JSContext *cx = xdr->cx();

    Rooted<StaticBlockObject*> obj(cx);
    uint32_t count = 0;
    uint32_t depthAndCount = 0;

    if (mode == XDR_ENCODE) {
        obj = *objp;
        uint32_t depth = obj->stackDepth();
        JS_ASSERT(depth <= UINT16_MAX);
        count = obj->slotCount();
        JS_ASSERT(count <= UINT16_MAX);
        depthAndCount = (depth << 16) | uint16_t(count);
    }

    if (mode == XDR_DECODE) {
        obj = StaticBlockObject::create(cx);
        if (!obj)
            return false;
        obj->initEnclosingStaticScope(enclosingScope);
        *objp = obj;
    }

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

            
            RootedId id(cx, atom != cx->runtime->emptyString
                            ? AtomToId(atom)
                            : INT_TO_JSID(i));

            bool redeclared;
            if (!StaticBlockObject::addVar(cx, obj, id, i, &redeclared)) {
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
            Shape *shape = &r.front();
            shapes[shape->shortid()] = shape;
        }

        



        for (unsigned i = 0; i < count; i++) {
            Shape *shape = shapes[i];
            JS_ASSERT(shape->hasDefaultGetter());
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
js::XDRStaticBlockObject(XDRState<XDR_ENCODE> *, HandleObject, HandleScript, StaticBlockObject **);

template bool
js::XDRStaticBlockObject(XDRState<XDR_DECODE> *, HandleObject, HandleScript, StaticBlockObject **);

JSObject *
js::CloneStaticBlockObject(JSContext *cx, HandleObject enclosingScope, Handle<StaticBlockObject*> srcBlock)
{
    

    Rooted<StaticBlockObject*> clone(cx, StaticBlockObject::create(cx));
    if (!clone)
        return NULL;

    clone->initEnclosingStaticScope(enclosingScope);
    clone->setStackDepth(srcBlock->stackDepth());

    
    AutoShapeVector shapes(cx);
    if (!shapes.growBy(srcBlock->slotCount()))
        return NULL;
    for (Shape::Range r = srcBlock->lastProperty()->all(); !r.empty(); r.popFront())
        shapes[r.front().shortid()] = &r.front();

    for (Shape **p = shapes.begin(); p != shapes.end(); ++p) {
        RootedId id(cx, (*p)->propid());
        unsigned i = (*p)->shortid();

        bool redeclared;
        if (!StaticBlockObject::addVar(cx, clone, id, i, &redeclared)) {
            JS_ASSERT(!redeclared);
            return NULL;
        }

        clone->setAliased(i, srcBlock->isAliased(i));
    }

    return clone;
}



ScopeIter::ScopeIter(JSContext *cx
                     JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : fp_(NULL),
    cur_(cx, reinterpret_cast<JSObject *>(-1)),
    block_(cx, reinterpret_cast<StaticBlockObject *>(-1)),
    type_(Type(-1))
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(const ScopeIter &si, JSContext *cx
                     JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : fp_(si.fp_),
    cur_(cx, si.cur_),
    block_(cx, si.block_),
    type_(si.type_),
    hasScopeObject_(si.hasScopeObject_)
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(JSObject &enclosingScope, JSContext *cx
                     JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : fp_(NULL),
    cur_(cx, &enclosingScope),
    block_(cx, reinterpret_cast<StaticBlockObject *>(-1)),
    type_(Type(-1))
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(StackFrame *fp, JSContext *cx
                     JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : fp_(fp),
    cur_(cx, fp->scopeChain()),
    block_(cx, fp->maybeBlockChain())
{
    settle();
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(const ScopeIter &si, StackFrame *fp, JSContext *cx
                     JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : fp_(fp),
    cur_(cx, si.cur_),
    block_(cx, si.block_),
    type_(si.type_),
    hasScopeObject_(si.hasScopeObject_)
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(StackFrame *fp, ScopeObject &scope, JSContext *cx
                     JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : fp_(fp),
    cur_(cx, &scope),
    block_(cx)
{
    














    if (cur_->isNestedScope()) {
        block_ = fp->maybeBlockChain();
        while (block_) {
            if (block_->stackDepth() <= cur_->asNestedScope().stackDepth())
                break;
            block_ = block_->enclosingBlock();
        }
        JS_ASSERT_IF(cur_->isClonedBlock(), cur_->asClonedBlock().staticBlock() == *block_);
    } else {
        block_ = NULL;
    }
    settle();
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeObject &
ScopeIter::scope() const
{
    JS_ASSERT(hasScopeObject());
    return cur_->asScope();
}

ScopeIter &
ScopeIter::operator++()
{
    JS_ASSERT(!done());
    switch (type_) {
      case Call:
        if (hasScopeObject_) {
            cur_ = &cur_->asCall().enclosingScope();
            if (CallObjectLambdaName(*fp_->fun()))
                cur_ = &cur_->asDeclEnv().enclosingScope();
        }
        fp_ = NULL;
        break;
      case Block:
        block_ = block_->enclosingBlock();
        if (hasScopeObject_)
            cur_ = &cur_->asClonedBlock().enclosingScope();
        settle();
        break;
      case With:
        JS_ASSERT(hasScopeObject_);
        cur_ = &cur_->asWith().enclosingScope();
        settle();
        break;
      case StrictEvalScope:
        if (hasScopeObject_)
            cur_ = &cur_->asCall().enclosingScope();
        fp_ = NULL;
        break;
    }
    return *this;
}

void
ScopeIter::settle()
{
    






















    if (fp_->isNonEvalFunctionFrame() && !fp_->fun()->isHeavyweight()) {
        if (block_) {
            type_ = Block;
            hasScopeObject_ = block_->needsClone();
        } else {
            type_ = Call;
            hasScopeObject_ = false;
        }
    } else if (fp_->isNonStrictDirectEvalFrame() && cur_ == fp_->prev()->scopeChain()) {
        if (block_) {
            JS_ASSERT(!block_->needsClone());
            type_ = Block;
            hasScopeObject_ = false;
        } else {
            fp_ = NULL;
        }
    } else if (fp_->isNonEvalFunctionFrame() && !fp_->hasCallObj()) {
        JS_ASSERT(cur_ == fp_->fun()->environment());
        fp_ = NULL;
    } else if (fp_->isStrictEvalFrame() && !fp_->hasCallObj()) {
        JS_ASSERT(cur_ == fp_->prev()->scopeChain());
        fp_ = NULL;
    } else if (cur_->isWith()) {
        JS_ASSERT_IF(fp_->isFunctionFrame(), fp_->fun()->isHeavyweight());
        JS_ASSERT_IF(block_, block_->needsClone());
        JS_ASSERT_IF(block_, block_->stackDepth() < cur_->asWith().stackDepth());
        type_ = With;
        hasScopeObject_ = true;
    } else if (block_) {
        type_ = Block;
        hasScopeObject_ = block_->needsClone();
        JS_ASSERT_IF(hasScopeObject_, cur_->asClonedBlock().staticBlock() == *block_);
    } else if (cur_->isCall()) {
        CallObject &callobj = cur_->asCall();
        type_ = callobj.isForEval() ? StrictEvalScope : Call;
        hasScopeObject_ = true;
        JS_ASSERT_IF(type_ == Call, callobj.callee().script() == fp_->script());
    } else {
        JS_ASSERT(!cur_->isScope());
        JS_ASSERT(fp_->isGlobalFrame() || fp_->isDebuggerFrame());
        fp_ = NULL;
    }
}

 HashNumber
ScopeIterKey::hash(ScopeIterKey si)
{
    
    return size_t(si.fp_) ^ size_t(si.cur_) ^ size_t(si.block_) ^ si.type_;
}

 bool
ScopeIterKey::match(ScopeIterKey si1, ScopeIterKey si2)
{
    
    return si1.fp_ == si2.fp_ &&
           (!si1.fp_ ||
            (si1.cur_   == si2.cur_   &&
             si1.block_ == si2.block_ &&
             si1.type_  == si2.type_));
}



namespace js {


















class DebugScopeProxy : public BaseProxyHandler
{
    enum Action { SET, GET };

    









    bool handleUnaliasedAccess(JSContext *cx, Handle<ScopeObject*> scope, jsid id, Action action, Value *vp)
    {
        Shape *shape = scope->lastProperty()->search(cx, id);
        if (!shape)
            return false;

        StackFrame *maybefp = cx->runtime->debugScopes->hasLiveFrame(*scope);

        if (scope->isCall() && !scope->asCall().isForEval()) {
            CallObject &callobj = scope->asCall();
            JSScript *script = callobj.callee().script();
            if (!script->ensureHasTypes(cx))
                return false;

            Bindings &bindings = script->bindings;
            unsigned i = shape->slot() - CallObject::RESERVED_SLOTS;
            bool isArg = i < bindings.numArgs();
            bool isVar = !isArg && (i - bindings.numArgs()) < bindings.numVars();

            if (isVar) {
                unsigned i = shape->shortid();
                if (script->varIsAliased(i))
                    return false;

                if (maybefp) {
                    if (action == GET)
                        *vp = maybefp->unaliasedVar(i);
                    else
                        maybefp->unaliasedVar(i) = *vp;
                } else {
                    if (action == GET)
                        *vp = callobj.var(i, DONT_CHECK_ALIASING);
                    else
                        callobj.setVar(i, *vp, DONT_CHECK_ALIASING);
                }

                if (action == SET)
                    TypeScript::SetLocal(cx, script, i, *vp);

                return true;
            }

            if (isArg) {
                unsigned i = shape->shortid();
                if (script->formalLivesInCallObject(i))
                    return false;

                if (maybefp) {
                    if (script->argsObjAliasesFormals() && maybefp->hasArgsObj()) {
                        if (action == GET)
                            *vp = maybefp->argsObj().arg(i);
                        else
                            maybefp->argsObj().setArg(i, *vp);
                    } else {
                        if (action == GET)
                            *vp = maybefp->unaliasedFormal(i, DONT_CHECK_ALIASING);
                        else
                            maybefp->unaliasedFormal(i, DONT_CHECK_ALIASING) = *vp;
                    }
                } else {
                    if (action == GET)
                        *vp = callobj.formal(i, DONT_CHECK_ALIASING);
                    else
                        callobj.setFormal(i, *vp, DONT_CHECK_ALIASING);
                }

                if (action == SET)
                    TypeScript::SetArgument(cx, script, i, *vp);

                return true;
            }

            return false;
        }

        if (scope->isClonedBlock()) {
            ClonedBlockObject &block = scope->asClonedBlock();
            unsigned i = shape->shortid();
            if (block.staticBlock().isAliased(i))
                return false;

            if (maybefp) {
                JSScript *script = maybefp->script();
                unsigned local = block.slotToLocalIndex(script->bindings, shape->slot());
                if (action == GET)
                    *vp = maybefp->unaliasedLocal(local);
                else
                    maybefp->unaliasedLocal(local) = *vp;
                JS_ASSERT(analyze::LocalSlot(script, local) >= analyze::TotalSlots(script));
            } else {
                if (action == GET)
                    *vp = block.var(i, DONT_CHECK_ALIASING);
                else
                    block.setVar(i, *vp, DONT_CHECK_ALIASING);
            }

            return true;
        }

        JS_ASSERT(scope->isDeclEnv() || scope->isWith() || scope->asCall().isForEval());
        return false;
    }

    static bool isArguments(JSContext *cx, jsid id)
    {
        return id == NameToId(cx->runtime->atomState.argumentsAtom);
    }

    static bool isFunctionScope(ScopeObject &scope)
    {
        return scope.isCall() && !scope.asCall().isForEval();
    }

    





    static bool isMissingArgumentsBinding(ScopeObject &scope)
    {
        return isFunctionScope(scope) &&
               !scope.asCall().callee().script()->argumentsHasVarBinding();
    }

    





    static bool checkForMissingArguments(JSContext *cx, jsid id, ScopeObject &scope,
                                         ArgumentsObject **maybeArgsObj)
    {
        *maybeArgsObj = NULL;

        if (!isArguments(cx, id) || !isFunctionScope(scope))
            return true;

        JSScript *script = scope.asCall().callee().script();
        if (script->needsArgsObj())
            return true;

        StackFrame *maybefp = cx->runtime->debugScopes->hasLiveFrame(scope);
        if (!maybefp) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_LIVE,
                                 "Debugger scope");
            return false;
        }

        *maybeArgsObj = ArgumentsObject::createUnexpected(cx, maybefp);
        return true;
    }

  public:
    static int family;
    static DebugScopeProxy singleton;

    DebugScopeProxy() : BaseProxyHandler(&family) {}

    bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                               PropertyDescriptor *desc) MOZ_OVERRIDE
    {
        return getOwnPropertyDescriptor(cx, proxy, id, set, desc);
    }

    bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id_, bool set,
                                  PropertyDescriptor *desc) MOZ_OVERRIDE
    {
        Rooted<ScopeObject*> scope(cx, &proxy->asDebugScope().scope());
        RootedId id(cx, id_);

        ArgumentsObject *maybeArgsObj;
        if (!checkForMissingArguments(cx, id, *scope, &maybeArgsObj))
            return false;

        if (maybeArgsObj) {
            PodZero(desc);
            desc->obj = proxy;
            desc->attrs = JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT;
            desc->value = ObjectValue(*maybeArgsObj);
            return true;
        }

        Value v;
        if (handleUnaliasedAccess(cx, scope, id, GET, &v)) {
            PodZero(desc);
            desc->obj = proxy;
            desc->attrs = JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT;
            desc->value = v;
            return true;
        }

        return JS_GetPropertyDescriptorById(cx, scope, id, JSRESOLVE_QUALIFIED, desc);
    }

    bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id_, Value *vp) MOZ_OVERRIDE
    {
        Rooted<ScopeObject*> scope(cx, &proxy->asDebugScope().scope());
        RootedId id(cx, id_);

        ArgumentsObject *maybeArgsObj;
        if (!checkForMissingArguments(cx, id, *scope, &maybeArgsObj))
            return false;

        if (maybeArgsObj) {
            *vp = ObjectValue(*maybeArgsObj);
            return true;
        }

        if (handleUnaliasedAccess(cx, scope, id, GET, vp))
            return true;

        return scope->getGeneric(cx, scope, id, vp);
    }

    bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id_, bool strict,
                     Value *vp) MOZ_OVERRIDE
    {
        Rooted<ScopeObject*> scope(cx, &proxy->asDebugScope().scope());
        RootedId id(cx, id_);

        if (handleUnaliasedAccess(cx, scope, id, SET, vp))
            return true;

        return scope->setGeneric(cx, scope, id, vp, strict);
    }

    bool defineProperty(JSContext *cx, JSObject *proxy, jsid id_, PropertyDescriptor *desc) MOZ_OVERRIDE
    {
        Rooted<ScopeObject*> scope(cx, &proxy->asDebugScope().scope());
        RootedId id(cx, id_);

        bool found;
        if (!has(cx, proxy, id, &found))
            return false;
        if (found)
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);

        return JS_DefinePropertyById(cx, scope, id, desc->value, desc->getter, desc->setter,
                                     desc->attrs);
    }

    bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, AutoIdVector &props) MOZ_OVERRIDE
    {
        ScopeObject &scope = proxy->asDebugScope().scope();

        if (isMissingArgumentsBinding(scope) &&
            !props.append(NameToId(cx->runtime->atomState.argumentsAtom)))
        {
            return false;
        }

        return GetPropertyNames(cx, &scope, JSITER_OWNONLY, &props);
    }

    bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp) MOZ_OVERRIDE
    {
        return js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_CANT_DELETE,
                                        JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                        NULL, NULL);
    }

    bool enumerate(JSContext *cx, JSObject *proxy, AutoIdVector &props) MOZ_OVERRIDE
    {
        ScopeObject &scope = proxy->asDebugScope().scope();

        if (isMissingArgumentsBinding(scope) &&
            !props.append(NameToId(cx->runtime->atomState.argumentsAtom)))
        {
            return false;
        }

        return GetPropertyNames(cx, &scope, 0, &props);
    }

    bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp) MOZ_OVERRIDE
    {
        ScopeObject &scope = proxy->asDebugScope().scope();

        if (isArguments(cx, id) && isFunctionScope(scope)) {
            *bp = true;
            return true;
        }

        JSBool found;
        if (!JS_HasPropertyById(cx, &scope, id, &found))
            return false;

        *bp = found;
        return true;
    }
};

}  

int DebugScopeProxy::family = 0;
DebugScopeProxy DebugScopeProxy::singleton;

 DebugScopeObject *
DebugScopeObject::create(JSContext *cx, ScopeObject &scope, HandleObject enclosing)
{
    JSObject *obj = NewProxyObject(cx, &DebugScopeProxy::singleton, ObjectValue(scope),
                                   NULL , &scope.global(),
                                   NULL , NULL );
    if (!obj)
        return NULL;

    JS_ASSERT(!enclosing->isScope());
    SetProxyExtra(obj, ENCLOSING_EXTRA, ObjectValue(*enclosing));

    return &obj->asDebugScope();
}

ScopeObject &
DebugScopeObject::scope() const
{
    return GetProxyTargetObject(this)->asScope();
}

JSObject &
DebugScopeObject::enclosingScope() const
{
    return GetProxyExtra(this, ENCLOSING_EXTRA).toObject();
}

bool
DebugScopeObject::isForDeclarative() const
{
    ScopeObject &s = scope();
    return s.isCall() || s.isBlock() || s.isDeclEnv();
}

bool
js_IsDebugScopeSlow(const JSObject *obj)
{
    return obj->getClass() == &ObjectProxyClass &&
           GetProxyHandler(obj) == &DebugScopeProxy::singleton;
}



DebugScopes::DebugScopes(JSRuntime *rt)
 : rt(rt),
   proxiedScopes(rt),
   missingScopes(rt),
   liveScopes(rt)
{}

DebugScopes::~DebugScopes()
{
    JS_ASSERT(missingScopes.empty());
}

bool
DebugScopes::init()
{
    if (!liveScopes.init() ||
        !proxiedScopes.init() ||
        !missingScopes.init())
    {
        return false;
    }
    return true;
}

void
DebugScopes::mark(JSTracer *trc)
{
    proxiedScopes.trace(trc);
}

void
DebugScopes::sweep()
{
    




    for (MissingScopeMap::Enum e(missingScopes); !e.empty(); e.popFront()) {
        if (!IsObjectMarked(e.front().value.unsafeGet()))
            e.removeFront();
    }

    for (LiveScopeMap::Enum e(liveScopes); !e.empty(); e.popFront()) {
        ScopeObject *scope = e.front().key;
        StackFrame *fp = e.front().value;

        



        if (!IsObjectMarked(&scope)) {
            e.removeFront();
            continue;
        }

        




        if (JSGenerator *gen = fp->maybeSuspendedGenerator(rt)) {
            JS_ASSERT(gen->state == JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
            if (!IsObjectMarked(&gen->obj)) {
                e.removeFront();
                continue;
            }
        }
    }
}








static bool
CanUseDebugScopeMaps(JSContext *cx)
{
    return cx->compartment->debugMode();
}

DebugScopeObject *
DebugScopes::hasDebugScope(JSContext *cx, ScopeObject &scope) const
{
    if (ObjectWeakMap::Ptr p = proxiedScopes.lookup(&scope)) {
        JS_ASSERT(CanUseDebugScopeMaps(cx));
        return &p->value->asDebugScope();
    }
    return NULL;
}

bool
DebugScopes::addDebugScope(JSContext *cx, ScopeObject &scope, DebugScopeObject &debugScope)
{
    if (!CanUseDebugScopeMaps(cx))
        return true;

    JS_ASSERT(!proxiedScopes.has(&scope));
    if (!proxiedScopes.put(&scope, &debugScope)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

DebugScopeObject *
DebugScopes::hasDebugScope(JSContext *cx, const ScopeIter &si) const
{
    JS_ASSERT(!si.hasScopeObject());
    if (MissingScopeMap::Ptr p = missingScopes.lookup(si)) {
        JS_ASSERT(CanUseDebugScopeMaps(cx));
        return p->value;
    }
    return NULL;
}

bool
DebugScopes::addDebugScope(JSContext *cx, const ScopeIter &si, DebugScopeObject &debugScope)
{
    JS_ASSERT(!si.hasScopeObject());
    if (!CanUseDebugScopeMaps(cx))
        return true;

    JS_ASSERT(!missingScopes.has(si));
    if (!missingScopes.put(si, &debugScope)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    JS_ASSERT(!liveScopes.has(&debugScope.scope()));
    if (!liveScopes.put(&debugScope.scope(), si.fp())) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
DebugScopes::onPopCall(StackFrame *fp, JSContext *cx)
{
    JS_ASSERT(!fp->isYielding());
    if (fp->fun()->isHeavyweight()) {
        



        if (fp->hasCallObj()) {
            CallObject &callobj = fp->scopeChain()->asCall();
            callobj.copyUnaliasedValues(fp);
            liveScopes.remove(&callobj);
        }
    } else {
        ScopeIter si(fp, cx);
        if (MissingScopeMap::Ptr p = missingScopes.lookup(si)) {
            CallObject &callobj = p->value->scope().asCall();
            callobj.copyUnaliasedValues(fp);
            liveScopes.remove(&callobj);
            missingScopes.remove(p);
        }
    }
}

void
DebugScopes::onPopBlock(JSContext *cx, StackFrame *fp)
{
    StaticBlockObject &staticBlock = *fp->maybeBlockChain();
    if (staticBlock.needsClone()) {
        ClonedBlockObject &clone = fp->scopeChain()->asClonedBlock();
        clone.copyUnaliasedValues(fp);
        liveScopes.remove(&clone);
    } else {
        ScopeIter si(fp, cx);
        if (MissingScopeMap::Ptr p = missingScopes.lookup(si)) {
            ClonedBlockObject &clone = p->value->scope().asClonedBlock();
            clone.copyUnaliasedValues(fp);
            liveScopes.remove(&clone);
            missingScopes.remove(p);
        }
    }
}

void
DebugScopes::onPopWith(StackFrame *fp)
{
    liveScopes.remove(&fp->scopeChain()->asWith());
}

void
DebugScopes::onPopStrictEvalScope(StackFrame *fp)
{
    



    if (fp->hasCallObj())
        liveScopes.remove(&fp->scopeChain()->asCall());
}

void
DebugScopes::onGeneratorFrameChange(StackFrame *from, StackFrame *to, JSContext *cx)
{
    for (ScopeIter toIter(to, cx); !toIter.done(); ++toIter) {
        if (toIter.hasScopeObject()) {
            






            LiveScopeMap::AddPtr livePtr = liveScopes.lookupForAdd(&toIter.scope());
            if (livePtr)
                livePtr->value = to;
            else
                liveScopes.add(livePtr, &toIter.scope(), to);
        } else {
            ScopeIter si(toIter, from, cx);
            if (MissingScopeMap::Ptr p = missingScopes.lookup(si)) {
                DebugScopeObject &debugScope = *p->value;
                liveScopes.lookup(&debugScope.scope())->value = to;
                missingScopes.remove(p);
                missingScopes.put(toIter, &debugScope);
            }
        }
    }
}

void
DebugScopes::onCompartmentLeaveDebugMode(JSCompartment *c)
{
    for (MissingScopeMap::Enum e(missingScopes); !e.empty(); e.popFront()) {
        if (e.front().key.fp()->compartment() == c)
            e.removeFront();
    }
    for (LiveScopeMap::Enum e(liveScopes); !e.empty(); e.popFront()) {
        if (e.front().key->compartment() == c)
            e.removeFront();
    }
}

bool
DebugScopes::updateLiveScopes(JSContext *cx)
{
    JS_CHECK_RECURSION(cx, return false);

    










    for (AllFramesIter i(cx->runtime->stackSpace); !i.done(); ++i) {
        StackFrame *fp = i.fp();
        if (fp->isDummyFrame() || fp->scopeChain()->compartment() != cx->compartment)
            continue;

        for (ScopeIter si(fp, cx); !si.done(); ++si) {
            if (si.hasScopeObject() && !liveScopes.put(&si.scope(), fp))
                return false;
        }

        if (fp->prevUpToDate())
            return true;
        JS_ASSERT(fp->compartment()->debugMode());
        fp->setPrevUpToDate();
    }

    return true;
}

StackFrame *
DebugScopes::hasLiveFrame(ScopeObject &scope)
{
    if (LiveScopeMap::Ptr p = liveScopes.lookup(&scope)) {
        StackFrame *fp = p->value;

        










        if (JSGenerator *gen = fp->maybeSuspendedGenerator(rt))
            JSObject::readBarrier(gen->obj);

        return fp;
    }
    return NULL;
}



static JSObject *
GetDebugScope(JSContext *cx, const ScopeIter &si);

static DebugScopeObject *
GetDebugScopeForScope(JSContext *cx, Handle<ScopeObject*> scope, const ScopeIter &enclosing)
{
    DebugScopes &debugScopes = *cx->runtime->debugScopes;
    if (DebugScopeObject *debugScope = debugScopes.hasDebugScope(cx, *scope))
        return debugScope;

    RootedObject enclosingDebug(cx, GetDebugScope(cx, enclosing));
    if (!enclosingDebug)
        return NULL;

    JSObject &maybeDecl = scope->enclosingScope();
    if (maybeDecl.isDeclEnv()) {
        JS_ASSERT(CallObjectLambdaName(scope->asCall().callee()));
        enclosingDebug = DebugScopeObject::create(cx, maybeDecl.asDeclEnv(), enclosingDebug);
        if (!enclosingDebug)
            return NULL;
    }

    DebugScopeObject *debugScope = DebugScopeObject::create(cx, *scope, enclosingDebug);
    if (!debugScope)
        return NULL;

    if (!debugScopes.addDebugScope(cx, *scope, *debugScope))
        return NULL;

    return debugScope;
}

static DebugScopeObject *
GetDebugScopeForMissing(JSContext *cx, const ScopeIter &si)
{
    DebugScopes &debugScopes = *cx->runtime->debugScopes;
    if (DebugScopeObject *debugScope = debugScopes.hasDebugScope(cx, si))
        return debugScope;

    ScopeIter copy(si, cx);
    RootedObject enclosingDebug(cx, GetDebugScope(cx, ++copy));
    if (!enclosingDebug)
        return NULL;

    





    DebugScopeObject *debugScope = NULL;
    switch (si.type()) {
      case ScopeIter::Call: {
        Rooted<CallObject*> callobj(cx, CallObject::createForFunction(cx, si.fp()));
        if (!callobj)
            return NULL;

        if (callobj->enclosingScope().isDeclEnv()) {
            JS_ASSERT(CallObjectLambdaName(callobj->callee()));
            DeclEnvObject &declenv = callobj->enclosingScope().asDeclEnv();
            enclosingDebug = DebugScopeObject::create(cx, declenv, enclosingDebug);
            if (!enclosingDebug)
                return NULL;
        }

        debugScope = DebugScopeObject::create(cx, *callobj, enclosingDebug);
        break;
      }
      case ScopeIter::Block: {
        Rooted<StaticBlockObject *> staticBlock(cx, &si.staticBlock());
        ClonedBlockObject *block = ClonedBlockObject::create(cx, staticBlock, si.fp());
        if (!block)
            return NULL;

        debugScope = DebugScopeObject::create(cx, *block, enclosingDebug);
        break;
      }
      case ScopeIter::With:
      case ScopeIter::StrictEvalScope:
        JS_NOT_REACHED("should already have a scope");
    }
    if (!debugScope)
        return NULL;

    if (!debugScopes.addDebugScope(cx, si, *debugScope))
        return NULL;

    return debugScope;
}

static JSObject *
GetDebugScope(JSContext *cx, JSObject &obj)
{
    





    if (!obj.isScope()) {
#ifdef DEBUG
        JSObject *o = &obj;
        while ((o = o->enclosingScope()))
            JS_ASSERT(!o->isScope());
#endif
        return &obj;
    }

    Rooted<ScopeObject*> scope(cx, &obj.asScope());
    if (StackFrame *fp = cx->runtime->debugScopes->hasLiveFrame(*scope)) {
        ScopeIter si(fp, *scope, cx);
        return GetDebugScope(cx, si);
    }
    ScopeIter si(scope->enclosingScope(), cx);
    return GetDebugScopeForScope(cx, scope, si);
}

static JSObject *
GetDebugScope(JSContext *cx, const ScopeIter &si)
{
    JS_CHECK_RECURSION(cx, return NULL);

    if (si.done())
        return GetDebugScope(cx, si.enclosingScope());

    if (!si.hasScopeObject())
        return GetDebugScopeForMissing(cx, si);

    Rooted<ScopeObject*> scope(cx, &si.scope());

    ScopeIter copy(si, cx);
    return GetDebugScopeForScope(cx, scope, ++copy);
}

JSObject *
js::GetDebugScopeForFunction(JSContext *cx, JSFunction *fun)
{
    assertSameCompartment(cx, fun);
    JS_ASSERT(cx->compartment->debugMode());
    if (!cx->runtime->debugScopes->updateLiveScopes(cx))
        return NULL;
    return GetDebugScope(cx, *fun->environment());
}

JSObject *
js::GetDebugScopeForFrame(JSContext *cx, StackFrame *fp)
{
    assertSameCompartment(cx, fp);
    if (CanUseDebugScopeMaps(cx) && !cx->runtime->debugScopes->updateLiveScopes(cx))
        return NULL;
    ScopeIter si(fp, cx);
    return GetDebugScope(cx, si);
}
