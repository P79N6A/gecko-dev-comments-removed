






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



StaticBlockObject *
js::ScopeCoordinateBlockChain(JSScript *script, jsbytecode *pc)
{
    ScopeCoordinate sc(pc);

    uint32_t blockIndex = GET_UINT32_INDEX(pc + 2 * sizeof(uint16_t));
    if (blockIndex == UINT32_MAX)
        return NULL;

    StaticBlockObject *block = &script->getObject(blockIndex)->asStaticBlock();
    unsigned i = 0;
    while (true) {
        while (block && !block->needsClone())
            block = block->enclosingBlock();
        if (i++ == sc.hops)
            break;
        block = block->enclosingBlock();
    }
    return block;
}

PropertyName *
js::ScopeCoordinateName(JSScript *script, jsbytecode *pc)
{
    StaticBlockObject *maybeBlock = ScopeCoordinateBlockChain(script, pc);
    ScopeCoordinate sc(pc);
    uint32_t targetSlot = ScopeObject::CALL_BLOCK_RESERVED_SLOTS + sc.slot;
    Shape *shape = maybeBlock ? maybeBlock->lastProperty() : script->bindings.lastShape();
    Shape::Range r = shape->all();
    while (r.front().slot() != targetSlot)
        r.popFront();
    return JSID_TO_ATOM(r.front().propid())->asPropertyName();
}









CallObject *
CallObject::create(JSContext *cx, JSScript *script, HandleObject enclosing, HandleFunction callee)
{
    RootedShape shape(cx);
    shape = script->bindings.callObjectShape(cx);
    if (shape == NULL)
        return NULL;

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
#ifdef JS_THREADSAFE
    JS_ASSERT(CanBeFinalizedInBackground(kind, &CallClass));
    kind = gc::GetBackgroundAllocKind(kind);
#endif

    RootedTypeObject type(cx);
    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, shape, &slots))
        return NULL;

    RootedObject obj(cx, JSObject::create(cx, kind, shape, type, slots));
    if (!obj)
        return NULL;

    




    if (&enclosing->global() != obj->getParent()) {
        JS_ASSERT(obj->getParent() == NULL);
        if (!JSObject::setParent(cx, obj, RootedObject(cx, &enclosing->global())))
            return NULL;
    }

    if (!obj->asScope().setEnclosingScope(cx, enclosing))
        return NULL;

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

    RootedObject scopeChain(cx, fp->scopeChain());

    



    if (js_IsNamedLambda(fp->fun())) {
        scopeChain = DeclEnvObject::create(cx, fp);
        if (!scopeChain)
            return NULL;
    }

    JSScript *script = fp->script();
    CallObject *callobj = create(cx, script, scopeChain, RootedFunction(cx, &fp->callee()));
    if (!callobj)
        return NULL;

    
    if (script->bindingsAccessedDynamically) {
        Value *formals = fp->formals();
        for (unsigned slot = 0, n = fp->fun()->nargs; slot < n; ++slot)
            callobj->setArg(slot, formals[slot]);
    } else if (unsigned n = script->numClosedArgs()) {
        Value *formals = fp->formals();
        for (unsigned i = 0; i < n; ++i) {
            uint32_t slot = script->getClosedArg(i);
            callobj->setArg(slot, formals[slot]);
        }
    }

    return callobj;
}

void
CallObject::copyUnaliasedValues(StackFrame *fp)
{
    JS_ASSERT(fp->script() == getCalleeFunction()->script());
    JSScript *script = fp->script();

    
    if (script->bindingsAccessedDynamically)
        return;

    
    for (unsigned i = 0; i < script->bindings.numArgs(); ++i) {
        if (!script->formalLivesInCallObject(i)) {
            if (script->argsObjAliasesFormals())
                setArg(i, fp->argsObj().arg(i), DONT_CHECK_ALIASING);
            else
                setArg(i, fp->unaliasedFormal(i), DONT_CHECK_ALIASING);
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

    return create(cx, fp->script(), fp->scopeChain(), RootedFunction(cx));
}

JSBool
CallObject::setArgOp(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16_t) JSID_TO_INT(id) == JSID_TO_INT(id));
    unsigned i = (uint16_t) JSID_TO_INT(id);

    JSScript *script = callobj.getCalleeFunction()->script();
    JS_ASSERT(script->formalLivesInCallObject(i));

    callobj.setArg(i, *vp);

    if (!script->ensureHasTypes(cx))
        return false;

    TypeScript::SetArgument(cx, script, i, *vp);
    return true;
}

JSBool
CallObject::setVarOp(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp)
{
    CallObject &callobj = obj->asCall();

    JS_ASSERT((int16_t) JSID_TO_INT(id) == JSID_TO_INT(id));
    unsigned i = (uint16_t) JSID_TO_INT(id);

    JSScript *script = callobj.getCalleeFunction()->script();
    JS_ASSERT(script->varIsAliased(i));

    callobj.setVar(i, *vp);

    if (!script->ensureHasTypes(cx))
        return false;

    TypeScript::SetLocal(cx, script, i, *vp);
    return true;
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
    RootedTypeObject type(cx);
    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    RootedShape emptyDeclEnvShape(cx);
    emptyDeclEnvShape = EmptyShape::getInitialShape(cx, &DeclEnvClass, NULL,
                                                    &fp->global(), FINALIZE_KIND);
    if (!emptyDeclEnvShape)
        return NULL;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, emptyDeclEnvShape, type, NULL));
    if (!obj)
        return NULL;

    if (!obj->asScope().setEnclosingScope(cx, fp->scopeChain()))
        return NULL;


    if (!DefineNativeProperty(cx, obj, RootedId(cx, AtomToId(fp->fun()->atom)),
                              ObjectValue(fp->callee()), NULL, NULL,
                              JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY,
                              0, 0)) {
        return NULL;
    }

    return &obj->asDeclEnv();
}

WithObject *
WithObject::create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth)
{
    RootedTypeObject type(cx);
    type = proto->getNewType(cx);
    if (!type)
        return NULL;

    RootedShape emptyWithShape(cx);
    emptyWithShape = EmptyShape::getInitialShape(cx, &WithClass, proto,
                                                 &enclosing->global(), FINALIZE_KIND);
    if (!emptyWithShape)
        return NULL;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, emptyWithShape, type, NULL));
    if (!obj)
        return NULL;

    if (!obj->asScope().setEnclosingScope(cx, enclosing))
        return NULL;

    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(depth));

    JSObject *thisp = proto->thisObject(cx);
    if (!thisp)
        return NULL;

    obj->setFixedSlot(THIS_SLOT, ObjectValue(*thisp));

    return &obj->asWith();
}

static JSBool
with_LookupGeneric(JSContext *cx, HandleObject obj, HandleId id, JSObject **objp, JSProperty **propp)
{
    
    unsigned flags = cx->resolveFlags;
    if (flags == RESOLVE_INFER)
        flags = js_InferFlags(cx, flags);
    flags |= JSRESOLVE_WITH;
    JSAutoResolveFlags rf(cx, flags);
    return obj->asWith().object().lookupGeneric(cx, id, objp, propp);
}

static JSBool
with_LookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, JSObject **objp, JSProperty **propp)
{
    return with_LookupGeneric(cx, obj, RootedId(cx, NameToId(name)), objp, propp);
}

static JSBool
with_LookupElement(JSContext *cx, HandleObject obj, uint32_t index, JSObject **objp,
                   JSProperty **propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, id.address()))
        return false;
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static JSBool
with_LookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, JSObject **objp, JSProperty **propp)
{
    return with_LookupGeneric(cx, obj, RootedId(cx, SPECIALID_TO_JSID(sid)), objp, propp);
}

static JSBool
with_GetGeneric(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, Value *vp)
{
    return obj->asWith().object().getGeneric(cx, id, vp);
}

static JSBool
with_GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandlePropertyName name, Value *vp)
{
    return with_GetGeneric(cx, obj, receiver, RootedId(cx, NameToId(name)), vp);
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
    return with_GetGeneric(cx, obj, receiver, RootedId(cx, SPECIALID_TO_JSID(sid)), vp);
}

static JSBool
with_SetGeneric(JSContext *cx, HandleObject obj, HandleId id, Value *vp, JSBool strict)
{
    return obj->asWith().object().setGeneric(cx, id, vp, strict);
}

static JSBool
with_SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, Value *vp, JSBool strict)
{
    return obj->asWith().object().setProperty(cx, name, vp, strict);
}

static JSBool
with_SetElement(JSContext *cx, HandleObject obj, uint32_t index, Value *vp, JSBool strict)
{
    return obj->asWith().object().setElement(cx, index, vp, strict);
}

static JSBool
with_SetSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, Value *vp, JSBool strict)
{
    return obj->asWith().object().setSpecial(cx, sid, vp, strict);
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
    RootedTypeObject type(cx);
    type = block->getNewType(cx);
    if (!type)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, block->lastProperty(), &slots))
        return NULL;

    RootedShape shape(cx);
    shape = block->lastProperty();

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, shape, type, slots));
    if (!obj)
        return NULL;

    
    if (&fp->global() != obj->getParent()) {
        JS_ASSERT(obj->getParent() == NULL);
        if (!JSObject::setParent(cx, obj, RootedObject(cx, &fp->global())))
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

    return &obj->asClonedBlock();
}

void
ClonedBlockObject::copyUnaliasedValues(StackFrame *fp)
{
    StaticBlockObject &block = staticBlock();
    unsigned base = block.slotToFrameLocal(fp->script(), 0);
    for (unsigned i = 0; i < slotCount(); ++i) {
        if (!block.isAliased(i))
            setVar(i, fp->unaliasedLocal(base + i), DONT_CHECK_ALIASING);
    }
}

StaticBlockObject *
StaticBlockObject::create(JSContext *cx)
{
    RootedTypeObject type(cx);
    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    RootedShape emptyBlockShape(cx);
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
    return addPropertyInternal(cx, id,  NULL,  NULL,
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

#define NO_PARENT_INDEX UINT32_MAX






static uint32_t
FindObjectIndex(JSScript *script, StaticBlockObject *maybeBlock)
{
    if (!maybeBlock || !script->hasObjects())
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
                      ? AtomToId(atom)
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



ScopeIter::ScopeIter()
 : fp_(NULL),
   cur_(reinterpret_cast<JSObject *>(-1)),
   block_(reinterpret_cast<StaticBlockObject *>(-1)),
   type_(Type(-1))
{}

ScopeIter::ScopeIter(JSObject &enclosingScope)
  : fp_(NULL),
    cur_(&enclosingScope),
    block_(reinterpret_cast<StaticBlockObject *>(-1)),
    type_(Type(-1))
{}

ScopeIter::ScopeIter(StackFrame *fp)
  : fp_(fp),
    cur_(fp->scopeChain()),
    block_(fp->maybeBlockChain())
{
    settle();
}

ScopeIter::ScopeIter(ScopeIter si, StackFrame *fp)
  : fp_(fp),
    cur_(si.cur_),
    block_(si.block_),
    type_(si.type_),
    hasScopeObject_(si.hasScopeObject_)
{}

ScopeIter::ScopeIter(StackFrame *fp, ScopeObject &scope)
  : fp_(fp),
    cur_(&scope)
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
}

ScopeObject &
ScopeIter::scope() const
{
    JS_ASSERT(hasScopeObject());
    return cur_->asScope();
}

ScopeIter
ScopeIter::enclosing() const
{
    JS_ASSERT(!done());
    ScopeIter si = *this;
    switch (type_) {
      case Call:
        if (hasScopeObject_) {
            si.cur_ = &cur_->asCall().enclosingScope();
            if (CallObjectLambdaName(si.fp_->fun()))
                si.cur_ = &si.cur_->asDeclEnv().enclosingScope();
        }
        si.fp_ = NULL;
        break;
      case Block:
        si.block_ = block_->enclosingBlock();
        if (hasScopeObject_)
            si.cur_ = &cur_->asClonedBlock().enclosingScope();
        si.settle();
        break;
      case With:
        JS_ASSERT(hasScopeObject_);
        si.cur_ = &cur_->asWith().enclosingScope();
        si.settle();
        break;
      case StrictEvalScope:
        if (hasScopeObject_)
            si.cur_ = &cur_->asCall().enclosingScope();
        si.fp_ = NULL;
        break;
    }
    return si;
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
    } else if (fp_->isNonEvalFunctionFrame() && fp_->beforeHeavyweightPrologue()) {
        JS_ASSERT(cur_ == fp_->fun()->environment());
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
        JS_ASSERT_IF(type_ == Call, callobj.getCalleeFunction()->script() == fp_->script());
    } else {
        JS_ASSERT(!cur_->isScope());
        JS_ASSERT(fp_->isGlobalFrame() || fp_->isDebuggerFrame());
        fp_ = NULL;
    }
}

 HashNumber
ScopeIter::hash(ScopeIter si)
{
    
    return size_t(si.fp_) ^ size_t(si.cur_) ^ size_t(si.block_) ^ si.type_;
}

 bool
ScopeIter::match(ScopeIter si1, ScopeIter si2)
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

    









    bool handleUnaliasedAccess(JSContext *cx, ScopeObject &scope, jsid id, Action action, Value *vp)
    {
        Shape *shape = scope.lastProperty()->search(cx, id);
        if (!shape)
            return false;

        StackFrame *maybefp = cx->runtime->debugScopes->hasLiveFrame(scope);

        if (scope.isCall() && !scope.asCall().isForEval()) {
            CallObject &callobj = scope.asCall();
            JSScript *script = callobj.getCalleeFunction()->script();
            if (!script->ensureHasTypes(cx))
                return false;

            if (shape->setterOp() == CallObject::setVarOp) {
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

            if (shape->setterOp() == CallObject::setArgOp) {
                unsigned i = shape->shortid();
                if (script->formalLivesInCallObject(i))
                    return false;
                
                if (maybefp) {
                    if (script->argsObjAliasesFormals()) {
                        if (action == GET)
                            *vp = maybefp->argsObj().arg(i);
                        else
                            maybefp->argsObj().setArg(i, *vp);
                    } else {
                        if (action == GET)
                            *vp = maybefp->unaliasedFormal(i);
                        else
                            maybefp->unaliasedFormal(i) = *vp;
                    }
                } else {
                    if (action == GET)
                        *vp = callobj.arg(i, DONT_CHECK_ALIASING);
                    else
                        callobj.setArg(i, *vp, DONT_CHECK_ALIASING);
                }

                if (action == SET)
                    TypeScript::SetArgument(cx, script, i, *vp);

                return true;
            }

            return false;
        }

        if (scope.isClonedBlock()) {
            ClonedBlockObject &block = scope.asClonedBlock();
            unsigned i = shape->shortid();
            if (block.staticBlock().isAliased(i))
                return false;

            if (maybefp) {
                JSScript *script = maybefp->script();
                    unsigned local = block.slotToFrameLocal(maybefp->script(), i);
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

        JS_ASSERT(scope.isDeclEnv() || scope.isWith() || scope.asCall().isForEval());
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
               !scope.asCall().getCalleeFunction()->script()->argumentsHasLocalBinding();
    }

    





    static bool checkForMissingArguments(JSContext *cx, jsid id, ScopeObject &scope,
                                         ArgumentsObject **maybeArgsObj)
    {
        *maybeArgsObj = NULL;

        if (!isArguments(cx, id) || !isFunctionScope(scope))
            return true;

        JSScript *script = scope.asCall().getCalleeFunction()->script();
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

    bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                  PropertyDescriptor *desc) MOZ_OVERRIDE
    {
        ScopeObject &scope = proxy->asDebugScope().scope();

        ArgumentsObject *maybeArgsObj;
        if (!checkForMissingArguments(cx, id, scope, &maybeArgsObj))
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

        return JS_GetPropertyDescriptorById(cx, &scope, id, JSRESOLVE_QUALIFIED, desc);
    }

    bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp) MOZ_OVERRIDE
    {
        ScopeObject &scope = proxy->asDebugScope().scope();

        ArgumentsObject *maybeArgsObj;
        if (!checkForMissingArguments(cx, id, scope, &maybeArgsObj))
            return false;

        if (maybeArgsObj) {
            *vp = ObjectValue(*maybeArgsObj);
            return true;
        }

        if (handleUnaliasedAccess(cx, scope, id, GET, vp))
            return true;
            
        return scope.getGeneric(cx, RootedObject(cx, &scope), RootedId(cx, id), vp);
    }

    bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
                     Value *vp) MOZ_OVERRIDE
    {
        ScopeObject &scope = proxy->asDebugScope().scope();

        if (handleUnaliasedAccess(cx, scope, id, SET, vp))
            return true;

        return scope.setGeneric(cx, RootedId(cx, id), vp, strict);
    }

    bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, PropertyDescriptor *desc) MOZ_OVERRIDE
    {
        bool found;
        if (!has(cx, proxy, id, &found))
            return false;
        if (found)
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);

        ScopeObject &scope = proxy->asDebugScope().scope();
        return JS_DefinePropertyById(cx, &scope, id, desc->value, desc->getter, desc->setter,
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
DebugScopeObject::create(JSContext *cx, ScopeObject &scope, JSObject &enclosing)
{
    JSObject *obj = NewProxyObject(cx, &DebugScopeProxy::singleton, ObjectValue(scope),
                                   NULL , &scope.global(),
                                   NULL , NULL );
    if (!obj)
        return NULL;

    JS_ASSERT(!enclosing.isScope());
    SetProxyExtra(obj, ENCLOSING_EXTRA, ObjectValue(enclosing));

    return &obj->asDebugScope();
}

ScopeObject &
DebugScopeObject::scope() const
{
    return Wrapper::wrappedObject(this)->asScope();
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
 : proxiedScopes(rt),
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
DebugScopes::sweep(JSRuntime *rt)
{
    




    for (MissingScopeMap::Enum e(missingScopes); !e.empty(); e.popFront()) {
        if (!IsObjectMarked(&e.front().value))
            e.removeFront();
    }

    for (LiveScopeMap::Enum e(liveScopes); !e.empty(); e.popFront()) {
        ScopeObject *scope = e.front().key;
        StackFrame *fp = e.front().value;

        



        if (JS_IsAboutToBeFinalized(scope)) {
            e.removeFront();
            continue;
        }

        






        if (JSGenerator *gen = fp->maybeSuspendedGenerator(rt)) {
            JS_ASSERT(gen->state == JSGEN_OPEN);
            if (!IsMarked(&gen->obj)) {
                if (scope->isCall())
                    scope->asCall().copyUnaliasedValues(fp);
                else if (scope->isBlock())
                    scope->asClonedBlock().copyUnaliasedValues(fp);
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
DebugScopes::hasDebugScope(JSContext *cx, ScopeIter si) const
{
    JS_ASSERT(!si.hasScopeObject());
    if (MissingScopeMap::Ptr p = missingScopes.lookup(si)) {
        JS_ASSERT(CanUseDebugScopeMaps(cx));
        return p->value;
    }
    return NULL;
}

bool
DebugScopes::addDebugScope(JSContext *cx, ScopeIter si, DebugScopeObject &debugScope)
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
DebugScopes::onPopCall(StackFrame *fp)
{
    if (fp->isGeneratorFrame())
        return;

    if (fp->fun()->isHeavyweight()) {
        CallObject &callobj = fp->scopeChain()->asCall();
        callobj.copyUnaliasedValues(fp);
        liveScopes.remove(&callobj);
    } else {
        if (MissingScopeMap::Ptr p = missingScopes.lookup(ScopeIter(fp))) {
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
        if (MissingScopeMap::Ptr p = missingScopes.lookup(ScopeIter(fp))) {
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
    liveScopes.remove(&fp->scopeChain()->asCall());
}

void
DebugScopes::onGeneratorFrameChange(StackFrame *from, StackFrame *to)
{
    for (ScopeIter toIter(to); !toIter.done(); toIter = toIter.enclosing()) {
        if (toIter.hasScopeObject()) {
            






            LiveScopeMap::AddPtr livePtr = liveScopes.lookupForAdd(&toIter.scope());
            if (livePtr)
                livePtr->value = to;
            else
                liveScopes.add(livePtr, &toIter.scope(), to);
        } else {
            if (MissingScopeMap::Ptr p = missingScopes.lookup(ScopeIter(toIter, from))) {
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

        for (ScopeIter si(fp); !si.done(); si = si.enclosing()) {
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
    if (LiveScopeMap::Ptr p = liveScopes.lookup(&scope))
        return p->value;
    return NULL;
}



static JSObject *
GetDebugScope(JSContext *cx, ScopeIter si);

static DebugScopeObject *
GetDebugScopeForScope(JSContext *cx, ScopeObject &scope, ScopeIter enclosing)
{
    DebugScopes &debugScopes = *cx->runtime->debugScopes;
    if (DebugScopeObject *debugScope = debugScopes.hasDebugScope(cx, scope))
        return debugScope;

    JSObject *enclosingDebug = GetDebugScope(cx, enclosing);
    if (!enclosingDebug)
        return NULL;

    JSObject &maybeDecl = scope.enclosingScope();
    if (maybeDecl.isDeclEnv()) {
        JS_ASSERT(CallObjectLambdaName(scope.asCall().getCalleeFunction()));
        enclosingDebug = DebugScopeObject::create(cx, maybeDecl.asDeclEnv(), *enclosingDebug);
        if (!enclosingDebug)
            return NULL;
    }

    DebugScopeObject *debugScope = DebugScopeObject::create(cx, scope, *enclosingDebug);
    if (!debugScope)
        return NULL;

    if (!debugScopes.addDebugScope(cx, scope, *debugScope))
        return NULL;

    return debugScope;
}

static DebugScopeObject *
GetDebugScopeForMissing(JSContext *cx, ScopeIter si)
{
    DebugScopes &debugScopes = *cx->runtime->debugScopes;
    if (DebugScopeObject *debugScope = debugScopes.hasDebugScope(cx, si))
        return debugScope;

    JSObject *enclosingDebug = GetDebugScope(cx, si.enclosing());
    if (!enclosingDebug)
        return NULL;

    





    DebugScopeObject *debugScope = NULL;
    switch (si.type()) {
      case ScopeIter::Call: {
        CallObject *callobj = CallObject::createForFunction(cx, si.fp());
        if (!callobj)
            return NULL;

        if (callobj->enclosingScope().isDeclEnv()) {
            JS_ASSERT(CallObjectLambdaName(callobj->getCalleeFunction()));
            DeclEnvObject &declenv = callobj->enclosingScope().asDeclEnv();
            enclosingDebug = DebugScopeObject::create(cx, declenv, *enclosingDebug);
            if (!enclosingDebug)
                return NULL;
        }

        debugScope = DebugScopeObject::create(cx, *callobj, *enclosingDebug);
        break;
      }
      case ScopeIter::Block: {
        Rooted<StaticBlockObject *> staticBlock(cx, &si.staticBlock());
        ClonedBlockObject *block = ClonedBlockObject::create(cx, staticBlock, si.fp());
        if (!block)
            return NULL;

        debugScope = DebugScopeObject::create(cx, *block, *enclosingDebug);
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

    ScopeObject &scope = obj.asScope();
    if (StackFrame *fp = cx->runtime->debugScopes->hasLiveFrame(scope))
        return GetDebugScope(cx, ScopeIter(fp, scope));
    return GetDebugScopeForScope(cx, scope, ScopeIter(scope.enclosingScope()));
}

static JSObject *
GetDebugScope(JSContext *cx, ScopeIter si)
{
    JS_CHECK_RECURSION(cx, return NULL);

    if (si.done())
        return GetDebugScope(cx, si.enclosingScope());

    if (!si.hasScopeObject())
        return GetDebugScopeForMissing(cx, si);

    return GetDebugScopeForScope(cx, si.scope(), si.enclosing());
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
    return GetDebugScope(cx, ScopeIter(fp));
}
