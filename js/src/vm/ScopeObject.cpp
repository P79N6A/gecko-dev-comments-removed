





#include "vm/ScopeObject-inl.h"

#include "mozilla/PodOperations.h"

#include "jscompartment.h"
#include "jsiter.h"

#include "vm/ArgumentsObject.h"
#include "vm/GlobalObject.h"
#include "vm/ProxyObject.h"
#include "vm/Shape.h"
#include "vm/Xdr.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::types;

using mozilla::PodZero;

typedef Rooted<ArgumentsObject *> RootedArgumentsObject;



StaticScopeIter::StaticScopeIter(ExclusiveContext *cx, JSObject *objArg)
  : obj(cx, objArg), onNamedLambda(false)
{
    JS_ASSERT_IF(obj, obj->is<StaticBlockObject>() || obj->is<JSFunction>());
}

bool
StaticScopeIter::done() const
{
    return !obj;
}

void
StaticScopeIter::operator++(int)
{
    if (obj->is<StaticBlockObject>()) {
        obj = obj->as<StaticBlockObject>().enclosingStaticScope();
    } else if (onNamedLambda || !obj->as<JSFunction>().isNamedLambda()) {
        onNamedLambda = false;
        obj = obj->as<JSFunction>().nonLazyScript()->enclosingStaticScope();
    } else {
        onNamedLambda = true;
    }
    JS_ASSERT_IF(obj, obj->is<StaticBlockObject>() || obj->is<JSFunction>());
    JS_ASSERT_IF(onNamedLambda, obj->is<JSFunction>());
}

bool
StaticScopeIter::hasDynamicScopeObject() const
{
    return obj->is<StaticBlockObject>()
           ? obj->as<StaticBlockObject>().needsClone()
           : obj->as<JSFunction>().isHeavyweight();
}

Shape *
StaticScopeIter::scopeShape() const
{
    JS_ASSERT(hasDynamicScopeObject());
    JS_ASSERT(type() != NAMED_LAMBDA);
    return type() == BLOCK
           ? block().lastProperty()
           : funScript()->bindings.callObjShape();
}

StaticScopeIter::Type
StaticScopeIter::type() const
{
    if (onNamedLambda)
        return NAMED_LAMBDA;
    return obj->is<StaticBlockObject>() ? BLOCK : FUNCTION;
}

StaticBlockObject &
StaticScopeIter::block() const
{
    JS_ASSERT(type() == BLOCK);
    return obj->as<StaticBlockObject>();
}

JSScript *
StaticScopeIter::funScript() const
{
    JS_ASSERT(type() == FUNCTION);
    return obj->as<JSFunction>().nonLazyScript();
}



static JSObject *
InnermostStaticScope(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->code && pc < script->code + script->length);
    JS_ASSERT(JOF_OPTYPE(*pc) == JOF_SCOPECOORD);

    uint32_t blockIndex = GET_UINT32_INDEX(pc + 2 * sizeof(uint16_t));

    if (blockIndex == UINT32_MAX)
        return script->function();
    return &script->getObject(blockIndex)->as<StaticBlockObject>();
}

Shape *
js::ScopeCoordinateToStaticScopeShape(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    StaticScopeIter ssi(cx, InnermostStaticScope(script, pc));
    ScopeCoordinate sc(pc);
    while (true) {
        if (ssi.hasDynamicScopeObject()) {
            if (!sc.hops)
                break;
            sc.hops--;
        }
        ssi++;
    }
    return ssi.scopeShape();
}

PropertyName *
js::ScopeCoordinateName(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    Shape::Range<NoGC> r(ScopeCoordinateToStaticScopeShape(cx, script, pc));
    ScopeCoordinate sc(pc);
    while (r.front().slot() != sc.slot)
        r.popFront();
    jsid id = r.front().propid();

    
    if (!JSID_IS_ATOM(id))
        return cx->runtime()->atomState.empty;
    return JSID_TO_ATOM(id)->asPropertyName();
}

JSScript *
js::ScopeCoordinateFunctionScript(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    StaticScopeIter ssi(cx, InnermostStaticScope(script, pc));
    ScopeCoordinate sc(pc);
    while (true) {
        if (ssi.hasDynamicScopeObject()) {
            if (!sc.hops)
                break;
            sc.hops--;
        }
        ssi++;
    }
    if (ssi.type() != StaticScopeIter::FUNCTION)
        return nullptr;
    return ssi.funScript();
}



inline void
ScopeObject::setEnclosingScope(HandleObject obj)
{
    JS_ASSERT_IF(obj->is<CallObject>() || obj->is<DeclEnvObject>() || obj->is<BlockObject>(),
                 obj->isDelegate());
    setFixedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*obj));
}





CallObject *
CallObject::create(JSContext *cx, HandleScript script, HandleShape shape, HandleTypeObject type, HeapSlot *slots)
{
    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    JS_ASSERT(CanBeFinalizedInBackground(kind, &CallObject::class_));
    kind = gc::GetBackgroundAllocKind(kind);

    gc::InitialHeap heap = script->treatAsRunOnce ? gc::TenuredHeap : gc::DefaultHeap;
    JSObject *obj = JSObject::create(cx, kind, heap, shape, type, slots);
    if (!obj)
        return nullptr;

    if (script->treatAsRunOnce) {
        RootedObject nobj(cx, obj);
        if (!JSObject::setSingletonType(cx, nobj))
            return nullptr;
        return &nobj->as<CallObject>();
    }

    return &obj->as<CallObject>();
}






CallObject *
CallObject::createTemplateObject(JSContext *cx, HandleScript script, gc::InitialHeap heap)
{
    RootedShape shape(cx, script->bindings.callObjShape());
    JS_ASSERT(shape->getObjectClass() == &class_);

    RootedTypeObject type(cx, cx->getNewType(&class_, nullptr));
    if (!type)
        return nullptr;

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    JS_ASSERT(CanBeFinalizedInBackground(kind, &class_));
    kind = gc::GetBackgroundAllocKind(kind);

    JSObject *obj = JSObject::create(cx, kind, heap, shape, type);
    if (!obj)
        return nullptr;

    return &obj->as<CallObject>();
}







CallObject *
CallObject::create(JSContext *cx, HandleScript script, HandleObject enclosing, HandleFunction callee)
{
    gc::InitialHeap heap = script->treatAsRunOnce ? gc::TenuredHeap : gc::DefaultHeap;
    CallObject *callobj = CallObject::createTemplateObject(cx, script, heap);
    if (!callobj)
        return nullptr;

    callobj->as<ScopeObject>().setEnclosingScope(enclosing);
    callobj->initFixedSlot(CALLEE_SLOT, ObjectOrNullValue(callee));

    if (script->treatAsRunOnce) {
        Rooted<CallObject*> ncallobj(cx, callobj);
        if (!JSObject::setSingletonType(cx, ncallobj))
            return nullptr;
        return ncallobj;
    }

    return callobj;
}

CallObject *
CallObject::createForFunction(JSContext *cx, HandleObject enclosing, HandleFunction callee)
{
    RootedObject scopeChain(cx, enclosing);
    JS_ASSERT(scopeChain);

    



    if (callee->isNamedLambda()) {
        scopeChain = DeclEnvObject::create(cx, scopeChain, callee);
        if (!scopeChain)
            return nullptr;
    }

    RootedScript script(cx, callee->nonLazyScript());
    return create(cx, script, scopeChain, callee);
}

CallObject *
CallObject::createForFunction(JSContext *cx, AbstractFramePtr frame)
{
    JS_ASSERT(frame.isNonEvalFunctionFrame());
    assertSameCompartment(cx, frame);

    RootedObject scopeChain(cx, frame.scopeChain());
    RootedFunction callee(cx, frame.callee());

    CallObject *callobj = createForFunction(cx, scopeChain, callee);
    if (!callobj)
        return nullptr;

    
    for (AliasedFormalIter i(frame.script()); i; i++) {
        callobj->setAliasedVar(cx, i, i->name(),
                               frame.unaliasedFormal(i.frameIndex(), DONT_CHECK_ALIASING));
    }

    return callobj;
}

CallObject *
CallObject::createForStrictEval(JSContext *cx, AbstractFramePtr frame)
{
    JS_ASSERT(frame.isStrictEvalFrame());
    JS_ASSERT_IF(frame.isStackFrame(), cx->interpreterFrame() == frame.asStackFrame());
    JS_ASSERT_IF(frame.isStackFrame(), cx->interpreterRegs().pc == frame.script()->code);

    RootedFunction callee(cx);
    RootedScript script(cx, frame.script());
    RootedObject scopeChain(cx, frame.scopeChain());
    return create(cx, script, scopeChain, callee);
}

const Class CallObject::class_ = {
    "Call",
    JSCLASS_IS_ANONYMOUS | JSCLASS_HAS_RESERVED_SLOTS(CallObject::RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    nullptr                  
};

const Class DeclEnvObject::class_ = {
    js_Object_str,
    JSCLASS_HAS_RESERVED_SLOTS(DeclEnvObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};






DeclEnvObject *
DeclEnvObject::createTemplateObject(JSContext *cx, HandleFunction fun, gc::InitialHeap heap)
{
    JS_ASSERT(IsNurseryAllocable(FINALIZE_KIND));

    RootedTypeObject type(cx, cx->getNewType(&class_, nullptr));
    if (!type)
        return nullptr;

    RootedShape emptyDeclEnvShape(cx);
    emptyDeclEnvShape = EmptyShape::getInitialShape(cx, &class_, nullptr,
                                                    cx->global(), nullptr, FINALIZE_KIND,
                                                    BaseShape::DELEGATE);
    if (!emptyDeclEnvShape)
        return nullptr;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, heap, emptyDeclEnvShape, type));
    if (!obj)
        return nullptr;

    
    Rooted<jsid> id(cx, AtomToId(fun->atom()));
    const Class *clasp = obj->getClass();
    unsigned attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY;
    if (!JSObject::putProperty(cx, obj, id, clasp->getProperty, clasp->setProperty,
                               lambdaSlot(), attrs, 0, 0))
    {
        return nullptr;
    }

    JS_ASSERT(!obj->hasDynamicSlots());
    return &obj->as<DeclEnvObject>();
}

DeclEnvObject *
DeclEnvObject::create(JSContext *cx, HandleObject enclosing, HandleFunction callee)
{
    RootedObject obj(cx, createTemplateObject(cx, callee, gc::DefaultHeap));
    if (!obj)
        return nullptr;

    obj->as<ScopeObject>().setEnclosingScope(enclosing);
    obj->setFixedSlot(lambdaSlot(), ObjectValue(*callee));
    return &obj->as<DeclEnvObject>();
}

WithObject *
WithObject::create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth)
{
    RootedTypeObject type(cx, cx->getNewType(&class_, proto.get()));
    if (!type)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(proto),
                                                      &enclosing->global(), nullptr,
                                                      FINALIZE_KIND));
    if (!shape)
        return nullptr;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, gc::DefaultHeap, shape, type));
    if (!obj)
        return nullptr;

    obj->as<ScopeObject>().setEnclosingScope(enclosing);
    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(depth));

    JSObject *thisp = JSObject::thisObject(cx, proto);
    if (!thisp)
        return nullptr;

    obj->setFixedSlot(THIS_SLOT, ObjectValue(*thisp));

    return &obj->as<WithObject>();
}

static bool
with_LookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::lookupGeneric(cx, actual, id, objp, propp);
}

static bool
with_LookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                    MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static bool
with_LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static bool
with_LookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static bool
with_GetGeneric(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
                MutableHandleValue vp)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::getGeneric(cx, actual, actual, id, vp);
}

static bool
with_GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandlePropertyName name,
                 MutableHandleValue vp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static bool
with_GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
                MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static bool
with_GetSpecial(JSContext *cx, HandleObject obj, HandleObject receiver, HandleSpecialId sid,
                MutableHandleValue vp)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static bool
with_SetGeneric(JSContext *cx, HandleObject obj, HandleId id,
                MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::setGeneric(cx, actual, actual, id, vp, strict);
}

static bool
with_SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                 MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::setProperty(cx, actual, actual, name, vp, strict);
}

static bool
with_SetElement(JSContext *cx, HandleObject obj, uint32_t index,
                MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::setElement(cx, actual, actual, index, vp, strict);
}

static bool
with_SetSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::setSpecial(cx, actual, actual, sid, vp, strict);
}

static bool
with_GetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::getGenericAttributes(cx, actual, id, attrsp);
}

static bool
with_SetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::setGenericAttributes(cx, actual, id, attrsp);
}

static bool
with_DeleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                    bool *succeeded)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::deleteProperty(cx, actual, name, succeeded);
}

static bool
with_DeleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                   bool *succeeded)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::deleteElement(cx, actual, index, succeeded);
}

static bool
with_DeleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                   bool *succeeded)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::deleteSpecial(cx, actual, sid, succeeded);
}

static bool
with_Enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
               MutableHandleValue statep, MutableHandleId idp)
{
    RootedObject actual(cx, &obj->as<WithObject>().object());
    return JSObject::enumerate(cx, actual, enum_op, statep, idp);
}

static JSObject *
with_ThisObject(JSContext *cx, HandleObject obj)
{
    return &obj->as<WithObject>().withThis();
}

const Class WithObject::class_ = {
    "With",
    JSCLASS_HAS_RESERVED_SLOTS(WithObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    JS_NULL_CLASS_EXT,
    {
        with_LookupGeneric,
        with_LookupProperty,
        with_LookupElement,
        with_LookupSpecial,
        nullptr,             
        nullptr,             
        nullptr,             
        nullptr,             
        with_GetGeneric,
        with_GetProperty,
        with_GetElement,
        nullptr,             
        with_GetSpecial,
        with_SetGeneric,
        with_SetProperty,
        with_SetElement,
        with_SetSpecial,
        with_GetGenericAttributes,
        with_SetGenericAttributes,
        with_DeleteProperty,
        with_DeleteElement,
        with_DeleteSpecial,
        with_Enumerate,
        with_ThisObject,
    }
};



ClonedBlockObject *
ClonedBlockObject::create(JSContext *cx, Handle<StaticBlockObject *> block, AbstractFramePtr frame)
{
    assertSameCompartment(cx, frame);
    JS_ASSERT(block->getClass() == &BlockObject::class_);

    RootedTypeObject type(cx, cx->getNewType(&BlockObject::class_, block.get()));
    if (!type)
        return nullptr;

    RootedShape shape(cx, block->lastProperty());

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, gc::TenuredHeap, shape, type));
    if (!obj)
        return nullptr;

    
    if (&frame.scopeChain()->global() != obj->getParent()) {
        JS_ASSERT(obj->getParent() == nullptr);
        Rooted<GlobalObject*> global(cx, &frame.scopeChain()->global());
        if (!JSObject::setParent(cx, obj, global))
            return nullptr;
    }

    JS_ASSERT(!obj->inDictionaryMode());
    JS_ASSERT(obj->slotSpan() >= block->slotCount() + RESERVED_SLOTS);

    obj->setReservedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*frame.scopeChain()));
    obj->setReservedSlot(DEPTH_SLOT, PrivateUint32Value(block->stackDepth()));

    



    unsigned nslots = block->slotCount();
    unsigned base = frame.script()->nfixed + block->stackDepth();
    for (unsigned i = 0; i < nslots; ++i) {
        if (block->isAliased(i))
            obj->as<ClonedBlockObject>().setVar(i, frame.unaliasedLocal(base + i));
    }

    JS_ASSERT(obj->isDelegate());

    return &obj->as<ClonedBlockObject>();
}

void
ClonedBlockObject::copyUnaliasedValues(AbstractFramePtr frame)
{
    StaticBlockObject &block = staticBlock();
    unsigned base = frame.script()->nfixed + block.stackDepth();
    for (unsigned i = 0; i < slotCount(); ++i) {
        if (!block.isAliased(i))
            setVar(i, frame.unaliasedLocal(base + i), DONT_CHECK_ALIASING);
    }
}

StaticBlockObject *
StaticBlockObject::create(ExclusiveContext *cx)
{
    RootedTypeObject type(cx, cx->getNewType(&BlockObject::class_, nullptr));
    if (!type)
        return nullptr;

    RootedShape emptyBlockShape(cx);
    emptyBlockShape = EmptyShape::getInitialShape(cx, &BlockObject::class_, nullptr, nullptr,
                                                  nullptr, FINALIZE_KIND, BaseShape::DELEGATE);
    if (!emptyBlockShape)
        return nullptr;

    JSObject *obj = JSObject::create(cx, FINALIZE_KIND, gc::TenuredHeap, emptyBlockShape, type);
    if (!obj)
        return nullptr;

    return &obj->as<StaticBlockObject>();
}

 Shape *
StaticBlockObject::addVar(ExclusiveContext *cx, Handle<StaticBlockObject*> block, HandleId id,
                          int index, bool *redeclared)
{
    JS_ASSERT(JSID_IS_ATOM(id) || (JSID_IS_INT(id) && JSID_TO_INT(id) == index));

    *redeclared = false;

    
    Shape **spp;
    if (Shape::search(cx, block->lastProperty(), id, &spp, true)) {
        *redeclared = true;
        return nullptr;
    }

    



    uint32_t slot = JSSLOT_FREE(&BlockObject::class_) + index;
    return JSObject::addPropertyInternal(cx, block, id,
                                          nullptr,  nullptr,
                                         slot, JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                         Shape::HAS_SHORTID, index, spp,
                                          false);
}

const Class BlockObject::class_ = {
    "Block",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(BlockObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

template<XDRMode mode>
bool
js::XDRStaticBlockObject(XDRState<mode> *xdr, HandleObject enclosingScope,
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
            RootedAtom atom(cx);
            if (!XDRAtom(xdr, &atom))
                return false;

            
            RootedId id(cx, atom != cx->runtime()->emptyString
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

        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
            Shape *shape = &r.front();
            shapes[shape->shortid()] = shape;
        }

        



        RootedShape shape(cx);
        RootedId propid(cx);
        RootedAtom atom(cx);
        for (unsigned i = 0; i < count; i++) {
            shape = shapes[i];
            JS_ASSERT(shape->hasDefaultGetter());
            JS_ASSERT(unsigned(shape->shortid()) == i);

            propid = shape->propid();
            JS_ASSERT(JSID_IS_ATOM(propid) || JSID_IS_INT(propid));

            
            atom = JSID_IS_ATOM(propid)
                   ? JSID_TO_ATOM(propid)
                   : cx->runtime()->emptyString;
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
js::XDRStaticBlockObject(XDRState<XDR_ENCODE> *, HandleObject, StaticBlockObject **);

template bool
js::XDRStaticBlockObject(XDRState<XDR_DECODE> *, HandleObject, StaticBlockObject **);

JSObject *
js::CloneStaticBlockObject(JSContext *cx, HandleObject enclosingScope, Handle<StaticBlockObject*> srcBlock)
{
    

    Rooted<StaticBlockObject*> clone(cx, StaticBlockObject::create(cx));
    if (!clone)
        return nullptr;

    clone->initEnclosingStaticScope(enclosingScope);
    clone->setStackDepth(srcBlock->stackDepth());

    
    AutoShapeVector shapes(cx);
    if (!shapes.growBy(srcBlock->slotCount()))
        return nullptr;
    for (Shape::Range<NoGC> r(srcBlock->lastProperty()); !r.empty(); r.popFront())
        shapes[r.front().shortid()] = &r.front();

    for (Shape **p = shapes.begin(); p != shapes.end(); ++p) {
        RootedId id(cx, (*p)->propid());
        unsigned i = (*p)->shortid();

        bool redeclared;
        if (!StaticBlockObject::addVar(cx, clone, id, i, &redeclared)) {
            JS_ASSERT(!redeclared);
            return nullptr;
        }

        clone->setAliased(i, srcBlock->isAliased(i));
    }

    return clone;
}





static inline JSAtom *
CallObjectLambdaName(JSFunction &fun)
{
    return fun.isNamedLambda() ? fun.atom() : nullptr;
}

ScopeIter::ScopeIter(const ScopeIter &si, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx),
    frame_(si.frame_),
    cur_(cx, si.cur_),
    block_(cx, si.block_),
    type_(si.type_),
    hasScopeObject_(si.hasScopeObject_)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(JSObject &enclosingScope, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx),
    frame_(NullFramePtr()),
    cur_(cx, &enclosingScope),
    block_(cx, nullptr),
    type_(Type(-1))
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(AbstractFramePtr frame, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx),
    frame_(frame),
    cur_(cx, frame.scopeChain()),
    block_(cx, frame.maybeBlockChain())
{
    assertSameCompartment(cx, frame);
    settle();
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(const ScopeIter &si, AbstractFramePtr frame, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(si.cx),
    frame_(frame),
    cur_(cx, si.cur_),
    block_(cx, si.block_),
    type_(si.type_),
    hasScopeObject_(si.hasScopeObject_)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(AbstractFramePtr frame, ScopeObject &scope, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx),
    frame_(frame),
    cur_(cx, &scope),
    block_(cx)
{
    














    if (cur_->is<NestedScopeObject>()) {
        block_ = frame.maybeBlockChain();
        while (block_) {
            if (block_->stackDepth() <= cur_->as<NestedScopeObject>().stackDepth())
                break;
            block_ = block_->enclosingBlock();
        }
        JS_ASSERT_IF(cur_->is<ClonedBlockObject>(),
                     cur_->as<ClonedBlockObject>().staticBlock() == *block_);
    } else {
        block_ = nullptr;
    }
    settle();
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeObject &
ScopeIter::scope() const
{
    JS_ASSERT(hasScopeObject());
    return cur_->as<ScopeObject>();
}

ScopeIter &
ScopeIter::operator++()
{
    JS_ASSERT(!done());
    switch (type_) {
      case Call:
        if (hasScopeObject_) {
            cur_ = &cur_->as<CallObject>().enclosingScope();
            if (CallObjectLambdaName(*frame_.fun()))
                cur_ = &cur_->as<DeclEnvObject>().enclosingScope();
        }
        frame_ = NullFramePtr();
        break;
      case Block:
        block_ = block_->enclosingBlock();
        if (hasScopeObject_)
            cur_ = &cur_->as<ClonedBlockObject>().enclosingScope();
        settle();
        break;
      case With:
        JS_ASSERT(hasScopeObject_);
        cur_ = &cur_->as<WithObject>().enclosingScope();
        settle();
        break;
      case StrictEvalScope:
        if (hasScopeObject_)
            cur_ = &cur_->as<CallObject>().enclosingScope();
        frame_ = NullFramePtr();
        break;
    }
    return *this;
}

void
ScopeIter::settle()
{
    






















    if (frame_.isNonEvalFunctionFrame() && !frame_.fun()->isHeavyweight()) {
        if (block_) {
            type_ = Block;
            hasScopeObject_ = block_->needsClone();
        } else {
            type_ = Call;
            hasScopeObject_ = false;
        }
    } else if (frame_.isNonStrictDirectEvalFrame() && cur_ == frame_.evalPrevScopeChain(cx)) {
        if (block_) {
            JS_ASSERT(!block_->needsClone());
            type_ = Block;
            hasScopeObject_ = false;
        } else {
            frame_ = NullFramePtr();
        }
    } else if (frame_.isNonEvalFunctionFrame() && !frame_.hasCallObj()) {
        JS_ASSERT(cur_ == frame_.fun()->environment());
        frame_ = NullFramePtr();
    } else if (frame_.isStrictEvalFrame() && !frame_.hasCallObj()) {
        JS_ASSERT(cur_ == frame_.evalPrevScopeChain(cx));
        frame_ = NullFramePtr();
    } else if (cur_->is<WithObject>()) {
        JS_ASSERT_IF(frame_.isFunctionFrame(), frame_.fun()->isHeavyweight());
        JS_ASSERT_IF(block_, block_->needsClone());
        JS_ASSERT_IF(block_, block_->stackDepth() < cur_->as<WithObject>().stackDepth());
        type_ = With;
        hasScopeObject_ = true;
    } else if (block_) {
        type_ = Block;
        hasScopeObject_ = block_->needsClone();
        JS_ASSERT_IF(hasScopeObject_, cur_->as<ClonedBlockObject>().staticBlock() == *block_);
    } else if (cur_->is<CallObject>()) {
        CallObject &callobj = cur_->as<CallObject>();
        type_ = callobj.isForEval() ? StrictEvalScope : Call;
        hasScopeObject_ = true;
        JS_ASSERT_IF(type_ == Call, callobj.callee().nonLazyScript() == frame_.script());
    } else {
        JS_ASSERT(!cur_->is<ScopeObject>());
        JS_ASSERT(frame_.isGlobalFrame() || frame_.isDebuggerFrame());
        frame_ = NullFramePtr();
    }
}

 HashNumber
ScopeIterKey::hash(ScopeIterKey si)
{
    
    return size_t(si.frame_.raw()) ^ size_t(si.cur_) ^ size_t(si.block_) ^ si.type_;
}

 bool
ScopeIterKey::match(ScopeIterKey si1, ScopeIterKey si2)
{
    
    return si1.frame_ == si2.frame_ &&
           (!si1.frame_ ||
            (si1.cur_   == si2.cur_   &&
             si1.block_ == si2.block_ &&
             si1.type_  == si2.type_));
}



namespace {



















class DebugScopeProxy : public BaseProxyHandler
{
    enum Action { SET, GET };

    






















    bool handleUnaliasedAccess(JSContext *cx, Handle<DebugScopeObject*> debugScope, Handle<ScopeObject*> scope,
                               jsid id, Action action, MutableHandleValue vp)
    {
        JS_ASSERT(&debugScope->scope() == scope);
        AbstractFramePtr maybeframe = DebugScopes::hasLiveFrame(*scope);

        
        if (scope->is<CallObject>() && !scope->as<CallObject>().isForEval()) {
            CallObject &callobj = scope->as<CallObject>();
            RootedScript script(cx, callobj.callee().nonLazyScript());
            if (!script->ensureHasTypes(cx))
                return false;

            Bindings &bindings = script->bindings;
            BindingIter bi(script);
            while (bi && NameToId(bi->name()) != id)
                bi++;
            if (!bi)
                return false;

            if (bi->kind() == VARIABLE || bi->kind() == CONSTANT) {
                unsigned i = bi.frameIndex();
                if (script->varIsAliased(i))
                    return false;

                if (maybeframe) {
                    if (action == GET)
                        vp.set(maybeframe.unaliasedVar(i));
                    else
                        maybeframe.unaliasedVar(i) = vp;
                } else if (JSObject *snapshot = debugScope->maybeSnapshot()) {
                    if (action == GET)
                        vp.set(snapshot->getDenseElement(bindings.numArgs() + i));
                    else
                        snapshot->setDenseElement(bindings.numArgs() + i, vp);
                } else {
                    
                    if (action == GET)
                        vp.set(UndefinedValue());
                }
            } else {
                JS_ASSERT(bi->kind() == ARGUMENT);
                unsigned i = bi.frameIndex();
                if (script->formalIsAliased(i))
                    return false;

                if (maybeframe) {
                    if (script->argsObjAliasesFormals() && maybeframe.hasArgsObj()) {
                        if (action == GET)
                            vp.set(maybeframe.argsObj().arg(i));
                        else
                            maybeframe.argsObj().setArg(i, vp);
                    } else {
                        if (action == GET)
                            vp.set(maybeframe.unaliasedFormal(i, DONT_CHECK_ALIASING));
                        else
                            maybeframe.unaliasedFormal(i, DONT_CHECK_ALIASING) = vp;
                    }
                } else if (JSObject *snapshot = debugScope->maybeSnapshot()) {
                    if (action == GET)
                        vp.set(snapshot->getDenseElement(i));
                    else
                        snapshot->setDenseElement(i, vp);
                } else {
                    
                    if (action == GET)
                        vp.set(UndefinedValue());
                }

                if (action == SET)
                    TypeScript::SetArgument(cx, script, i, vp);
            }

            return true;
        }

        
        if (scope->is<ClonedBlockObject>()) {
            Rooted<ClonedBlockObject *> block(cx, &scope->as<ClonedBlockObject>());
            Shape *shape = block->lastProperty()->search(cx, id);
            if (!shape)
                return false;

            unsigned i = shape->shortid();
            if (block->staticBlock().isAliased(i))
                return false;

            if (maybeframe) {
                JSScript *script = maybeframe.script();
                unsigned local = block->slotToLocalIndex(script->bindings, shape->slot());
                if (action == GET)
                    vp.set(maybeframe.unaliasedLocal(local));
                else
                    maybeframe.unaliasedLocal(local) = vp;
                JS_ASSERT(analyze::LocalSlot(script, local) >= analyze::TotalSlots(script));
            } else {
                if (action == GET)
                    vp.set(block->var(i, DONT_CHECK_ALIASING));
                else
                    block->setVar(i, vp, DONT_CHECK_ALIASING);
            }

            return true;
        }

        
        JS_ASSERT(scope->is<DeclEnvObject>() || scope->is<WithObject>() ||
                  scope->as<CallObject>().isForEval());
        return false;
    }

    static bool isArguments(JSContext *cx, jsid id)
    {
        return id == NameToId(cx->names().arguments);
    }

    static bool isFunctionScope(ScopeObject &scope)
    {
        return scope.is<CallObject>() && !scope.as<CallObject>().isForEval();
    }

    





    static bool isMissingArgumentsBinding(ScopeObject &scope)
    {
        return isFunctionScope(scope) &&
               !scope.as<CallObject>().callee().nonLazyScript()->argumentsHasVarBinding();
    }

    





    static bool checkForMissingArguments(JSContext *cx, jsid id, ScopeObject &scope,
                                         ArgumentsObject **maybeArgsObj)
    {
        *maybeArgsObj = nullptr;

        if (!isArguments(cx, id) || !isFunctionScope(scope))
            return true;

        if (scope.as<CallObject>().callee().nonLazyScript()->needsArgsObj())
            return true;

        AbstractFramePtr maybeframe = DebugScopes::hasLiveFrame(scope);
        if (!maybeframe) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_LIVE,
                                 "Debugger scope");
            return false;
        }

        *maybeArgsObj = ArgumentsObject::createUnexpected(cx, maybeframe);
        return true;
    }

  public:
    static int family;
    static DebugScopeProxy singleton;

    DebugScopeProxy() : BaseProxyHandler(&family) {}

    bool isExtensible(JSContext *cx, HandleObject proxy, bool *extensible) MOZ_OVERRIDE
    {
        
        
        *extensible = true;
        return true;
    }

    bool preventExtensions(JSContext *cx, HandleObject proxy) MOZ_OVERRIDE
    {
        
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CHANGE_EXTENSIBILITY);
        return false;
    }

    bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                               MutableHandle<PropertyDescriptor> desc,
                               unsigned flags) MOZ_OVERRIDE
    {
        return getOwnPropertyDescriptor(cx, proxy, id, desc, flags);
    }

    bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                  MutableHandle<PropertyDescriptor> desc,
                                  unsigned flags) MOZ_OVERRIDE
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &debugScope->scope());

        RootedArgumentsObject maybeArgsObj(cx);
        if (!checkForMissingArguments(cx, id, *scope, maybeArgsObj.address()))
            return false;

        if (maybeArgsObj) {
            desc.object().set(debugScope);
            desc.setAttributes(JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT);
            desc.value().setObject(*maybeArgsObj);
            desc.setShortId(0);
            desc.setGetter(nullptr);
            desc.setSetter(nullptr);
            return true;
        }

        RootedValue v(cx);
        if (handleUnaliasedAccess(cx, debugScope, scope, id, GET, &v)) {
            desc.object().set(debugScope);
            desc.setAttributes(JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT);
            desc.value().set(v);
            desc.setShortId(0);
            desc.setGetter(nullptr);
            desc.setSetter(nullptr);
            return true;
        }

        return JS_GetOwnPropertyDescriptorById(cx, scope, id, flags, desc);
    }

    bool get(JSContext *cx, HandleObject proxy, HandleObject receiver,  HandleId id,
             MutableHandleValue vp) MOZ_OVERRIDE
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        RootedArgumentsObject maybeArgsObj(cx);
        if (!checkForMissingArguments(cx, id, *scope, maybeArgsObj.address()))
            return false;

        if (maybeArgsObj) {
            vp.set(ObjectValue(*maybeArgsObj));
            return true;
        }

        if (handleUnaliasedAccess(cx, debugScope, scope, id, GET, vp))
            return true;

        return JSObject::getGeneric(cx, scope, scope, id, vp);
    }

    bool set(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id, bool strict,
             MutableHandleValue vp) MOZ_OVERRIDE
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());
        if (handleUnaliasedAccess(cx, debugScope, scope, id, SET, vp))
            return true;
        return JSObject::setGeneric(cx, scope, scope, id, vp, strict);
    }

    bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                        MutableHandle<PropertyDescriptor> desc) MOZ_OVERRIDE
    {
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        bool found;
        if (!has(cx, proxy, id, &found))
            return false;
        if (found)
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);

        return JS_DefinePropertyById(cx, scope, id, desc.value(), desc.getter(), desc.setter(),
                                     desc.attributes());
    }

    bool getScopePropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props,
                               unsigned flags)
    {
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        if (isMissingArgumentsBinding(*scope)) {
            if (!props.append(NameToId(cx->names().arguments)))
                return false;
        }

        if (!GetPropertyNames(cx, scope, flags, &props))
            return false;

        



        if (scope->is<CallObject>() && !scope->as<CallObject>().isForEval()) {
            RootedScript script(cx, scope->as<CallObject>().callee().nonLazyScript());
            for (BindingIter bi(script); bi; bi++) {
                if (!bi->aliased() && !props.append(NameToId(bi->name())))
                    return false;
            }
        }

        return true;
    }

    bool getOwnPropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props) MOZ_OVERRIDE
    {
        return getScopePropertyNames(cx, proxy, props, JSITER_OWNONLY);
    }

    bool enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props) MOZ_OVERRIDE
    {
        return getScopePropertyNames(cx, proxy, props, 0);
    }

    bool has(JSContext *cx, HandleObject proxy, HandleId id_, bool *bp) MOZ_OVERRIDE
    {
        RootedId id(cx, id_);
        ScopeObject &scopeObj = proxy->as<DebugScopeObject>().scope();

        if (isArguments(cx, id) && isFunctionScope(scopeObj)) {
            *bp = true;
            return true;
        }

        bool found;
        RootedObject scope(cx, &scopeObj);
        if (!JS_HasPropertyById(cx, scope, id, &found))
            return false;

        



        if (!found && scope->is<CallObject>() && !scope->as<CallObject>().isForEval()) {
            RootedScript script(cx, scope->as<CallObject>().callee().nonLazyScript());
            for (BindingIter bi(script); bi; bi++) {
                if (!bi->aliased() && NameToId(bi->name()) == id) {
                    found = true;
                    break;
                }
            }
        }

        *bp = found;
        return true;
    }

    bool delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp) MOZ_OVERRIDE
    {
        RootedValue idval(cx, IdToValue(id));
        return js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_CANT_DELETE,
                                        JSDVG_IGNORE_STACK, idval, NullPtr(), nullptr, nullptr);
    }
};

} 

int DebugScopeProxy::family = 0;
DebugScopeProxy DebugScopeProxy::singleton;

 DebugScopeObject *
DebugScopeObject::create(JSContext *cx, ScopeObject &scope, HandleObject enclosing)
{
    JS_ASSERT(scope.compartment() == cx->compartment());
    RootedValue priv(cx, ObjectValue(scope));
    JSObject *obj = NewProxyObject(cx, &DebugScopeProxy::singleton, priv,
                                   nullptr , &scope.global());
    if (!obj)
        return nullptr;

    JS_ASSERT(!enclosing->is<ScopeObject>());

    DebugScopeObject *debugScope = &obj->as<DebugScopeObject>();
    debugScope->setExtra(ENCLOSING_EXTRA, ObjectValue(*enclosing));
    debugScope->setExtra(SNAPSHOT_EXTRA, NullValue());

    return debugScope;
}

ScopeObject &
DebugScopeObject::scope() const
{
    return target()->as<ScopeObject>();
}

JSObject &
DebugScopeObject::enclosingScope() const
{
    return extra(ENCLOSING_EXTRA).toObject();
}

JSObject *
DebugScopeObject::maybeSnapshot() const
{
    JS_ASSERT(!scope().as<CallObject>().isForEval());
    return extra(SNAPSHOT_EXTRA).toObjectOrNull();
}

void
DebugScopeObject::initSnapshot(JSObject &o)
{
    JS_ASSERT(maybeSnapshot() == nullptr);
    setExtra(SNAPSHOT_EXTRA, ObjectValue(o));
}

bool
DebugScopeObject::isForDeclarative() const
{
    ScopeObject &s = scope();
    return s.is<CallObject>() || s.is<BlockObject>() || s.is<DeclEnvObject>();
}

bool
js_IsDebugScopeSlow(ProxyObject *proxy)
{
    JS_ASSERT(proxy->hasClass(&ProxyObject::uncallableClass_));
    return proxy->handler() == &DebugScopeProxy::singleton;
}



DebugScopes::DebugScopes(JSContext *cx)
 : proxiedScopes(cx),
   missingScopes(cx->runtime()),
   liveScopes(cx->runtime())
{}

DebugScopes::~DebugScopes()
{
    JS_ASSERT(missingScopes.empty());
    WeakMapBase::removeWeakMapFromList(&proxiedScopes);
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
        if (IsObjectAboutToBeFinalized(e.front().value.unsafeGet()))
            e.removeFront();
    }

    for (LiveScopeMap::Enum e(liveScopes); !e.empty(); e.popFront()) {
        ScopeObject *scope = e.front().key;
        AbstractFramePtr frame = e.front().value;

        



        if (IsObjectAboutToBeFinalized(&scope)) {
            e.removeFront();
            continue;
        }

        




        if (JSGenerator *gen = frame.maybeSuspendedGenerator(rt)) {
            JS_ASSERT(gen->state == JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
            if (IsObjectAboutToBeFinalized(&gen->obj)) {
                e.removeFront();
                continue;
            }
        }
    }
}








static bool
CanUseDebugScopeMaps(JSContext *cx)
{
    return cx->compartment()->debugMode();
}

DebugScopes *
DebugScopes::ensureCompartmentData(JSContext *cx)
{
    JSCompartment *c = cx->compartment();
    if (c->debugScopes)
        return c->debugScopes;

    c->debugScopes = cx->runtime()->new_<DebugScopes>(cx);
    if (c->debugScopes && c->debugScopes->init())
        return c->debugScopes;

    js_ReportOutOfMemory(cx);
    return nullptr;
}

DebugScopeObject *
DebugScopes::hasDebugScope(JSContext *cx, ScopeObject &scope)
{
    DebugScopes *scopes = scope.compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (ObjectWeakMap::Ptr p = scopes->proxiedScopes.lookup(&scope)) {
        JS_ASSERT(CanUseDebugScopeMaps(cx));
        return &p->value->as<DebugScopeObject>();
    }

    return nullptr;
}

bool
DebugScopes::addDebugScope(JSContext *cx, ScopeObject &scope, DebugScopeObject &debugScope)
{
    JS_ASSERT(cx->compartment() == scope.compartment());
    JS_ASSERT(cx->compartment() == debugScope.compartment());

    if (!CanUseDebugScopeMaps(cx))
        return true;

    DebugScopes *scopes = ensureCompartmentData(cx);
    if (!scopes)
        return false;

    JS_ASSERT(!scopes->proxiedScopes.has(&scope));
    if (!scopes->proxiedScopes.put(&scope, &debugScope)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    HashTableWriteBarrierPost(cx->runtime(), &scopes->proxiedScopes, &scope);
    return true;
}

DebugScopeObject *
DebugScopes::hasDebugScope(JSContext *cx, const ScopeIter &si)
{
    JS_ASSERT(!si.hasScopeObject());

    DebugScopes *scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(si)) {
        JS_ASSERT(CanUseDebugScopeMaps(cx));
        return p->value;
    }
    return nullptr;
}

bool
DebugScopes::addDebugScope(JSContext *cx, const ScopeIter &si, DebugScopeObject &debugScope)
{
    JS_ASSERT(!si.hasScopeObject());
    JS_ASSERT(cx->compartment() == debugScope.compartment());

    if (!CanUseDebugScopeMaps(cx))
        return true;

    DebugScopes *scopes = ensureCompartmentData(cx);
    if (!scopes)
        return false;

    JS_ASSERT(!scopes->missingScopes.has(si));
    if (!scopes->missingScopes.put(si, &debugScope)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    JS_ASSERT(!scopes->liveScopes.has(&debugScope.scope()));
    if (!scopes->liveScopes.put(&debugScope.scope(), si.frame())) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    HashTableWriteBarrierPost(cx->runtime(), &scopes->liveScopes, &debugScope.scope());

    return true;
}

void
DebugScopes::onPopCall(AbstractFramePtr frame, JSContext *cx)
{
    JS_ASSERT(!frame.isYielding());
    assertSameCompartment(cx, frame);

    DebugScopes *scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    Rooted<DebugScopeObject*> debugScope(cx, nullptr);

    if (frame.fun()->isHeavyweight()) {
        



        if (!frame.hasCallObj())
            return;

        CallObject &callobj = frame.scopeChain()->as<CallObject>();
        scopes->liveScopes.remove(&callobj);
        if (ObjectWeakMap::Ptr p = scopes->proxiedScopes.lookup(&callobj))
            debugScope = &p->value->as<DebugScopeObject>();
    } else {
        ScopeIter si(frame, cx);
        if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(si)) {
            debugScope = p->value;
            scopes->liveScopes.remove(&debugScope->scope().as<CallObject>());
            scopes->missingScopes.remove(p);
        }
    }

    









    if (debugScope) {
        




        AutoValueVector vec(cx);
        if (!frame.copyRawFrameSlots(&vec) || vec.length() == 0)
            return;

        



        RootedScript script(cx, frame.script());
        if (script->analyzedArgsUsage() && script->needsArgsObj() && frame.hasArgsObj()) {
            for (unsigned i = 0; i < frame.numFormalArgs(); ++i) {
                if (script->formalLivesInArgumentsObject(i))
                    vec[i] = frame.argsObj().arg(i);
            }
        }

        



        RootedObject snapshot(cx, NewDenseCopiedArray(cx, vec.length(), vec.begin()));
        if (!snapshot) {
            cx->clearPendingException();
            return;
        }

        debugScope->initSnapshot(*snapshot);
    }
}

void
DebugScopes::onPopBlock(JSContext *cx, AbstractFramePtr frame)
{
    assertSameCompartment(cx, frame);

    DebugScopes *scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    StaticBlockObject &staticBlock = *frame.maybeBlockChain();
    if (staticBlock.needsClone()) {
        ClonedBlockObject &clone = frame.scopeChain()->as<ClonedBlockObject>();
        clone.copyUnaliasedValues(frame);
        scopes->liveScopes.remove(&clone);
    } else {
        ScopeIter si(frame, cx);
        if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(si)) {
            ClonedBlockObject &clone = p->value->scope().as<ClonedBlockObject>();
            clone.copyUnaliasedValues(frame);
            scopes->liveScopes.remove(&clone);
            scopes->missingScopes.remove(p);
        }
    }
}

void
DebugScopes::onPopWith(AbstractFramePtr frame)
{
    DebugScopes *scopes = frame.compartment()->debugScopes;
    if (scopes)
        scopes->liveScopes.remove(&frame.scopeChain()->as<WithObject>());
}

void
DebugScopes::onPopStrictEvalScope(AbstractFramePtr frame)
{
    DebugScopes *scopes = frame.compartment()->debugScopes;
    if (!scopes)
        return;

    



    if (frame.hasCallObj())
        scopes->liveScopes.remove(&frame.scopeChain()->as<CallObject>());
}

void
DebugScopes::onGeneratorFrameChange(AbstractFramePtr from, AbstractFramePtr to, JSContext *cx)
{
    for (ScopeIter toIter(to, cx); !toIter.done(); ++toIter) {
        DebugScopes *scopes = ensureCompartmentData(cx);
        if (!scopes)
            return;

        if (toIter.hasScopeObject()) {
            






            JS_ASSERT(toIter.scope().compartment() == cx->compartment());
            LiveScopeMap::AddPtr livePtr = scopes->liveScopes.lookupForAdd(&toIter.scope());
            if (livePtr) {
                livePtr->value = to;
            } else {
                scopes->liveScopes.add(livePtr, &toIter.scope(), to);  
                HashTableWriteBarrierPost(cx->runtime(), &scopes->liveScopes, &toIter.scope());
            }
        } else {
            ScopeIter si(toIter, from, cx);
            JS_ASSERT(si.frame().scopeChain()->compartment() == cx->compartment());
            if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(si)) {
                DebugScopeObject &debugScope = *p->value;
                scopes->liveScopes.lookup(&debugScope.scope())->value = to;
                scopes->missingScopes.remove(p);
                scopes->missingScopes.put(toIter, &debugScope);  
            }
        }
    }
}

void
DebugScopes::onCompartmentLeaveDebugMode(JSCompartment *c)
{
    DebugScopes *scopes = c->debugScopes;
    if (scopes) {
        scopes->proxiedScopes.clear();
        scopes->missingScopes.clear();
        scopes->liveScopes.clear();
    }
}

bool
DebugScopes::updateLiveScopes(JSContext *cx)
{
    JS_CHECK_RECURSION(cx, return false);

    










    for (AllFramesIter i(cx); !i.done(); ++i) {
        



        if (i.isIon())
            continue;

        AbstractFramePtr frame = i.abstractFramePtr();
        if (frame.scopeChain()->compartment() != cx->compartment())
            continue;

        for (ScopeIter si(frame, cx); !si.done(); ++si) {
            if (si.hasScopeObject()) {
                JS_ASSERT(si.scope().compartment() == cx->compartment());
                DebugScopes *scopes = ensureCompartmentData(cx);
                if (!scopes)
                    return false;
                if (!scopes->liveScopes.put(&si.scope(), frame))
                    return false;
                HashTableWriteBarrierPost(cx->runtime(), &scopes->liveScopes, &si.scope());
            }
        }

        if (frame.prevUpToDate())
            return true;
        JS_ASSERT(frame.scopeChain()->compartment()->debugMode());
        frame.setPrevUpToDate();
    }

    return true;
}

AbstractFramePtr
DebugScopes::hasLiveFrame(ScopeObject &scope)
{
    DebugScopes *scopes = scope.compartment()->debugScopes;
    if (!scopes)
        return NullFramePtr();

    if (LiveScopeMap::Ptr p = scopes->liveScopes.lookup(&scope)) {
        AbstractFramePtr frame = p->value;

        










        if (JSGenerator *gen = frame.maybeSuspendedGenerator(scope.compartment()->runtimeFromMainThread()))
            JSObject::readBarrier(gen->obj);

        return frame;
    }
    return NullFramePtr();
}



static JSObject *
GetDebugScope(JSContext *cx, const ScopeIter &si);

static DebugScopeObject *
GetDebugScopeForScope(JSContext *cx, Handle<ScopeObject*> scope, const ScopeIter &enclosing)
{
    if (DebugScopeObject *debugScope = DebugScopes::hasDebugScope(cx, *scope))
        return debugScope;

    RootedObject enclosingDebug(cx, GetDebugScope(cx, enclosing));
    if (!enclosingDebug)
        return nullptr;

    JSObject &maybeDecl = scope->enclosingScope();
    if (maybeDecl.is<DeclEnvObject>()) {
        JS_ASSERT(CallObjectLambdaName(scope->as<CallObject>().callee()));
        enclosingDebug = DebugScopeObject::create(cx, maybeDecl.as<DeclEnvObject>(), enclosingDebug);
        if (!enclosingDebug)
            return nullptr;
    }

    DebugScopeObject *debugScope = DebugScopeObject::create(cx, *scope, enclosingDebug);
    if (!debugScope)
        return nullptr;

    if (!DebugScopes::addDebugScope(cx, *scope, *debugScope))
        return nullptr;

    return debugScope;
}

static DebugScopeObject *
GetDebugScopeForMissing(JSContext *cx, const ScopeIter &si)
{
    if (DebugScopeObject *debugScope = DebugScopes::hasDebugScope(cx, si))
        return debugScope;

    ScopeIter copy(si, cx);
    RootedObject enclosingDebug(cx, GetDebugScope(cx, ++copy));
    if (!enclosingDebug)
        return nullptr;

    










    DebugScopeObject *debugScope = nullptr;
    switch (si.type()) {
      case ScopeIter::Call: {
        Rooted<CallObject*> callobj(cx, CallObject::createForFunction(cx, si.frame()));
        if (!callobj)
            return nullptr;

        if (callobj->enclosingScope().is<DeclEnvObject>()) {
            JS_ASSERT(CallObjectLambdaName(callobj->callee()));
            DeclEnvObject &declenv = callobj->enclosingScope().as<DeclEnvObject>();
            enclosingDebug = DebugScopeObject::create(cx, declenv, enclosingDebug);
            if (!enclosingDebug)
                return nullptr;
        }

        debugScope = DebugScopeObject::create(cx, *callobj, enclosingDebug);
        break;
      }
      case ScopeIter::Block: {
        Rooted<StaticBlockObject *> staticBlock(cx, &si.staticBlock());
        ClonedBlockObject *block = ClonedBlockObject::create(cx, staticBlock, si.frame());
        if (!block)
            return nullptr;

        debugScope = DebugScopeObject::create(cx, *block, enclosingDebug);
        break;
      }
      case ScopeIter::With:
      case ScopeIter::StrictEvalScope:
        MOZ_ASSUME_UNREACHABLE("should already have a scope");
    }
    if (!debugScope)
        return nullptr;

    if (!DebugScopes::addDebugScope(cx, si, *debugScope))
        return nullptr;

    return debugScope;
}

static JSObject *
GetDebugScope(JSContext *cx, JSObject &obj)
{
    





    if (!obj.is<ScopeObject>()) {
#ifdef DEBUG
        JSObject *o = &obj;
        while ((o = o->enclosingScope()))
            JS_ASSERT(!o->is<ScopeObject>());
#endif
        return &obj;
    }

    Rooted<ScopeObject*> scope(cx, &obj.as<ScopeObject>());
    if (AbstractFramePtr frame = DebugScopes::hasLiveFrame(*scope)) {
        ScopeIter si(frame, *scope, cx);
        return GetDebugScope(cx, si);
    }
    ScopeIter si(scope->enclosingScope(), cx);
    return GetDebugScopeForScope(cx, scope, si);
}

static JSObject *
GetDebugScope(JSContext *cx, const ScopeIter &si)
{
    JS_CHECK_RECURSION(cx, return nullptr);

    if (si.done())
        return GetDebugScope(cx, si.enclosingScope());

    if (!si.hasScopeObject())
        return GetDebugScopeForMissing(cx, si);

    Rooted<ScopeObject*> scope(cx, &si.scope());

    ScopeIter copy(si, cx);
    return GetDebugScopeForScope(cx, scope, ++copy);
}

JSObject *
js::GetDebugScopeForFunction(JSContext *cx, HandleFunction fun)
{
    assertSameCompartment(cx, fun);
    JS_ASSERT(cx->compartment()->debugMode());
    if (!DebugScopes::updateLiveScopes(cx))
        return nullptr;
    return GetDebugScope(cx, *fun->environment());
}

JSObject *
js::GetDebugScopeForFrame(JSContext *cx, AbstractFramePtr frame)
{
    assertSameCompartment(cx, frame);
    if (CanUseDebugScopeMaps(cx) && !DebugScopes::updateLiveScopes(cx))
        return nullptr;
    ScopeIter si(frame, cx);
    return GetDebugScope(cx, si);
}

#ifdef DEBUG

typedef HashSet<PropertyName *> PropertyNameSet;

static bool
RemoveReferencedNames(JSContext *cx, HandleScript script, PropertyNameSet &remainingNames)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    for (jsbytecode *pc = script->code;
         pc != script->code + script->length;
         pc += GetBytecodeLength(pc))
    {
        PropertyName *name;

        switch (JSOp(*pc)) {
          case JSOP_NAME:
          case JSOP_CALLNAME:
          case JSOP_SETNAME:
            name = script->getName(pc);
            break;

          case JSOP_GETALIASEDVAR:
          case JSOP_CALLALIASEDVAR:
          case JSOP_SETALIASEDVAR:
            name = ScopeCoordinateName(cx, script, pc);
            break;

          default:
            name = nullptr;
            break;
        }

        if (name)
            remainingNames.remove(name);
    }

    if (script->hasObjects()) {
        ObjectArray *objects = script->objects();
        for (size_t i = 0; i < objects->length; i++) {
            JSObject *obj = objects->vector[i];
            if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted()) {
                JSFunction *fun = &obj->as<JSFunction>();
                RootedScript innerScript(cx, fun->getOrCreateScript(cx));
                if (!innerScript)
                    return false;

                if (!RemoveReferencedNames(cx, innerScript, remainingNames))
                    return false;
            }
        }
    }

    return true;
}

static bool
AnalyzeEntrainedVariablesInScript(JSContext *cx, HandleScript script, HandleScript innerScript)
{
    PropertyNameSet remainingNames(cx);
    if (!remainingNames.init())
        return false;

    for (BindingIter bi(script); bi; bi++) {
        if (bi->aliased()) {
            PropertyNameSet::AddPtr p = remainingNames.lookupForAdd(bi->name());
            if (!p && !remainingNames.add(p, bi->name()))
                return false;
        }
    }

    if (!RemoveReferencedNames(cx, innerScript, remainingNames))
        return false;

    if (!remainingNames.empty()) {
        Sprinter buf(cx);
        if (!buf.init())
            return false;

        buf.printf("Script ");

        if (JSAtom *name = script->function()->displayAtom()) {
            buf.putString(name);
            buf.printf(" ");
        }

        buf.printf("(%s:%d) has variables entrained by ", script->filename(), script->lineno);

        if (JSAtom *name = innerScript->function()->displayAtom()) {
            buf.putString(name);
            buf.printf(" ");
        }

        buf.printf("(%s:%d) ::", innerScript->filename(), innerScript->lineno);

        for (PropertyNameSet::Range r = remainingNames.all(); !r.empty(); r.popFront()) {
            buf.printf(" ");
            buf.putString(r.front());
        }

        printf("%s\n", buf.string());
    }

    if (innerScript->hasObjects()) {
        ObjectArray *objects = innerScript->objects();
        for (size_t i = 0; i < objects->length; i++) {
            JSObject *obj = objects->vector[i];
            if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted()) {
                JSFunction *fun = &obj->as<JSFunction>();
                RootedScript innerInnerScript(cx, fun->nonLazyScript());
                if (!AnalyzeEntrainedVariablesInScript(cx, script, innerInnerScript))
                    return false;
            }
        }
    }

    return true;
}












bool
js::AnalyzeEntrainedVariables(JSContext *cx, HandleScript script)
{
    if (!script->hasObjects())
        return true;

    ObjectArray *objects = script->objects();
    for (size_t i = 0; i < objects->length; i++) {
        JSObject *obj = objects->vector[i];
        if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted()) {
            JSFunction *fun = &obj->as<JSFunction>();
            RootedScript innerScript(cx, fun->getOrCreateScript(cx));
            if (!innerScript)
                return false;

            if (script->function() && script->function()->isHeavyweight()) {
                if (!AnalyzeEntrainedVariablesInScript(cx, script, innerScript))
                    return false;
            }

            if (!AnalyzeEntrainedVariables(cx, innerScript))
                return false;
        }
    }

    return true;
}

#endif
