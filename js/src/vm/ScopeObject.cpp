





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
using namespace js::gc;
using namespace js::types;

using mozilla::PodZero;

typedef Rooted<ArgumentsObject *> RootedArgumentsObject;
typedef MutableHandle<ArgumentsObject *> MutableHandleArgumentsObject;



static JSObject *
InnermostStaticScope(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script->containsPC(pc));
    JS_ASSERT(JOF_OPTYPE(*pc) == JOF_SCOPECOORD);

    NestedScopeObject *scope = script->getStaticScope(pc);
    if (scope)
        return scope;
    return script->functionNonDelazifying();
}

Shape *
js::ScopeCoordinateToStaticScopeShape(JSScript *script, jsbytecode *pc)
{
    StaticScopeIter<NoGC> ssi(InnermostStaticScope(script, pc));
    uint32_t hops = ScopeCoordinate(pc).hops();
    while (true) {
        JS_ASSERT(!ssi.done());
        if (ssi.hasDynamicScopeObject()) {
            if (!hops)
                break;
            hops--;
        }
        ssi++;
    }
    return ssi.scopeShape();
}

static const uint32_t SCOPE_COORDINATE_NAME_THRESHOLD = 20;

void
ScopeCoordinateNameCache::purge()
{
    shape = nullptr;
    if (map.initialized())
        map.finish();
}

PropertyName *
js::ScopeCoordinateName(ScopeCoordinateNameCache &cache, JSScript *script, jsbytecode *pc)
{
    Shape *shape = ScopeCoordinateToStaticScopeShape(script, pc);
    if (shape != cache.shape && shape->slot() >= SCOPE_COORDINATE_NAME_THRESHOLD) {
        cache.purge();
        if (cache.map.init(shape->slot())) {
            cache.shape = shape;
            Shape::Range<NoGC> r(shape);
            while (!r.empty()) {
                if (!cache.map.putNew(r.front().slot(), r.front().propid())) {
                    cache.purge();
                    break;
                }
                r.popFront();
            }
        }
    }

    jsid id;
    ScopeCoordinate sc(pc);
    if (shape == cache.shape) {
        ScopeCoordinateNameCache::Map::Ptr p = cache.map.lookup(sc.slot());
        id = p->value();
    } else {
        Shape::Range<NoGC> r(shape);
        while (r.front().slot() != sc.slot())
            r.popFront();
        id = r.front().propidRaw();
    }

    
    if (!JSID_IS_ATOM(id))
        return script->runtimeFromAnyThread()->commonNames->empty;
    return JSID_TO_ATOM(id)->asPropertyName();
}

JSScript *
js::ScopeCoordinateFunctionScript(JSScript *script, jsbytecode *pc)
{
    StaticScopeIter<NoGC> ssi(InnermostStaticScope(script, pc));
    uint32_t hops = ScopeCoordinate(pc).hops();
    while (true) {
        if (ssi.hasDynamicScopeObject()) {
            if (!hops)
                break;
            hops--;
        }
        ssi++;
    }
    if (ssi.type() != StaticScopeIter<NoGC>::FUNCTION)
        return nullptr;
    return ssi.funScript();
}



void
ScopeObject::setEnclosingScope(HandleObject obj)
{
    JS_ASSERT_IF(obj->is<CallObject>() || obj->is<DeclEnvObject>() || obj->is<BlockObject>(),
                 obj->isDelegate());
    setFixedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*obj));
}

CallObject *
CallObject::create(JSContext *cx, HandleShape shape, HandleTypeObject type)
{
    MOZ_ASSERT(!type->singleton(),
               "passed a singleton type to create() (use createSingleton() "
               "instead)");
    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    MOZ_ASSERT(CanBeFinalizedInBackground(kind, &CallObject::class_));
    kind = gc::GetBackgroundAllocKind(kind);

    JSObject *obj = JSObject::create(cx, kind, gc::DefaultHeap, shape, type);
    if (!obj)
        return nullptr;

    return &obj->as<CallObject>();
}

CallObject *
CallObject::createSingleton(JSContext *cx, HandleShape shape)
{
    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    MOZ_ASSERT(CanBeFinalizedInBackground(kind, &CallObject::class_));
    kind = gc::GetBackgroundAllocKind(kind);

    RootedTypeObject type(cx, cx->getSingletonType(&class_, TaggedProto(nullptr)));
    if (!type)
        return nullptr;
    RootedObject obj(cx, JSObject::create(cx, kind, gc::TenuredHeap, shape, type));
    if (!obj)
        return nullptr;

    MOZ_ASSERT(obj->hasSingletonType(),
               "type created inline above must be a singleton");

    return &obj->as<CallObject>();
}






CallObject *
CallObject::createTemplateObject(JSContext *cx, HandleScript script, gc::InitialHeap heap)
{
    RootedShape shape(cx, script->bindings.callObjShape());
    JS_ASSERT(shape->getObjectClass() == &class_);

    RootedTypeObject type(cx, cx->getNewType(&class_, TaggedProto(nullptr)));
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
    gc::InitialHeap heap = script->treatAsRunOnce() ? gc::TenuredHeap : gc::DefaultHeap;
    CallObject *callobj = CallObject::createTemplateObject(cx, script, heap);
    if (!callobj)
        return nullptr;

    callobj->as<ScopeObject>().setEnclosingScope(enclosing);
    callobj->initFixedSlot(CALLEE_SLOT, ObjectOrNullValue(callee));

    if (script->treatAsRunOnce()) {
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
    JS_ASSERT_IF(frame.isInterpreterFrame(), cx->interpreterFrame() == frame.asInterpreterFrame());
    JS_ASSERT_IF(frame.isInterpreterFrame(), cx->interpreterRegs().pc == frame.script()->code());

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

    RootedTypeObject type(cx, cx->getNewType(&class_, TaggedProto(nullptr)));
    if (!type)
        return nullptr;

    RootedShape emptyDeclEnvShape(cx);
    emptyDeclEnvShape = EmptyShape::getInitialShape(cx, &class_, TaggedProto(nullptr),
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
    if (!JSObject::putProperty<SequentialExecution>(cx, obj, id, clasp->getProperty,
                                                    clasp->setProperty, lambdaSlot(), attrs, 0)) {
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

template<XDRMode mode>
bool
js::XDRStaticWithObject(XDRState<mode> *xdr, HandleObject enclosingScope, StaticWithObject **objp)
{
    if (mode == XDR_DECODE) {
        JSContext *cx = xdr->cx();
        Rooted<StaticWithObject*> obj(cx, StaticWithObject::create(cx));
        if (!obj)
            return false;
        obj->initEnclosingNestedScope(enclosingScope);
        *objp = obj;
    }
    
    
    

    return true;
}

template bool
js::XDRStaticWithObject(XDRState<XDR_ENCODE> *, HandleObject, StaticWithObject **);

template bool
js::XDRStaticWithObject(XDRState<XDR_DECODE> *, HandleObject, StaticWithObject **);

StaticWithObject *
StaticWithObject::create(ExclusiveContext *cx)
{
    RootedTypeObject type(cx, cx->getNewType(&class_, TaggedProto(nullptr)));
    if (!type)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(nullptr),
                                                      nullptr, nullptr, FINALIZE_KIND));
    if (!shape)
        return nullptr;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, gc::TenuredHeap, shape, type));
    if (!obj)
        return nullptr;

    return &obj->as<StaticWithObject>();
}

static JSObject *
CloneStaticWithObject(JSContext *cx, HandleObject enclosingScope, Handle<StaticWithObject*> srcWith)
{
    Rooted<StaticWithObject*> clone(cx, StaticWithObject::create(cx));
    if (!clone)
        return nullptr;

    clone->initEnclosingNestedScope(enclosingScope);

    return clone;
}

DynamicWithObject *
DynamicWithObject::create(JSContext *cx, HandleObject object, HandleObject enclosing,
                          HandleObject staticWith)
{
    JS_ASSERT(staticWith->is<StaticWithObject>());
    RootedTypeObject type(cx, cx->getNewType(&class_, TaggedProto(staticWith.get())));
    if (!type)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(staticWith),
                                                      &enclosing->global(), nullptr,
                                                      FINALIZE_KIND));
    if (!shape)
        return nullptr;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, gc::DefaultHeap, shape, type));
    if (!obj)
        return nullptr;

    JSObject *thisp = JSObject::thisObject(cx, object);
    if (!thisp)
        return nullptr;

    obj->as<ScopeObject>().setEnclosingScope(enclosing);
    obj->setFixedSlot(OBJECT_SLOT, ObjectValue(*object));
    obj->setFixedSlot(THIS_SLOT, ObjectValue(*thisp));

    return &obj->as<DynamicWithObject>();
}

static bool
with_LookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
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
with_GetGeneric(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
                MutableHandleValue vp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::getGeneric(cx, actual, actual, id, vp);
}

static bool
with_GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandlePropertyName name,
                 MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
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
with_SetGeneric(JSContext *cx, HandleObject obj, HandleId id,
                MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::setGeneric(cx, actual, actual, id, vp, strict);
}

static bool
with_SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                 MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::setProperty(cx, actual, actual, name, vp, strict);
}

static bool
with_SetElement(JSContext *cx, HandleObject obj, uint32_t index,
                MutableHandleValue vp, bool strict)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::setElement(cx, actual, actual, index, vp, strict);
}

static bool
with_GetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::getGenericAttributes(cx, actual, id, attrsp);
}

static bool
with_SetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::setGenericAttributes(cx, actual, id, attrsp);
}

static bool
with_DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return JSObject::deleteGeneric(cx, actual, id, succeeded);
}

static JSObject *
with_ThisObject(JSContext *cx, HandleObject obj)
{
    return &obj->as<DynamicWithObject>().withThis();
}

const Class StaticWithObject::class_ = {
    "WithTemplate",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(StaticWithObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

const Class DynamicWithObject::class_ = {
    "With",
    JSCLASS_HAS_RESERVED_SLOTS(DynamicWithObject::RESERVED_SLOTS) |
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
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    {
        with_LookupGeneric,
        with_LookupProperty,
        with_LookupElement,
        nullptr,             
        nullptr,             
        nullptr,             
        with_GetGeneric,
        with_GetProperty,
        with_GetElement,
        with_SetGeneric,
        with_SetProperty,
        with_SetElement,
        with_GetGenericAttributes,
        with_SetGenericAttributes,
        with_DeleteGeneric,
        nullptr, nullptr,    
        nullptr,             
        nullptr,             
        with_ThisObject,
    }
};



ClonedBlockObject *
ClonedBlockObject::create(JSContext *cx, Handle<StaticBlockObject *> block, AbstractFramePtr frame)
{
    assertSameCompartment(cx, frame);
    JS_ASSERT(block->getClass() == &BlockObject::class_);

    RootedTypeObject type(cx, cx->getNewType(&BlockObject::class_, TaggedProto(block.get())));
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
    JS_ASSERT(obj->slotSpan() >= block->numVariables() + RESERVED_SLOTS);

    obj->setReservedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*frame.scopeChain()));

    



    unsigned nvars = block->numVariables();
    for (unsigned i = 0; i < nvars; ++i) {
        if (block->isAliased(i)) {
            Value &val = frame.unaliasedLocal(block->blockIndexToLocalIndex(i));
            obj->as<ClonedBlockObject>().setVar(i, val);
        }
    }

    JS_ASSERT(obj->isDelegate());

    return &obj->as<ClonedBlockObject>();
}

void
ClonedBlockObject::copyUnaliasedValues(AbstractFramePtr frame)
{
    StaticBlockObject &block = staticBlock();
    for (unsigned i = 0; i < numVariables(); ++i) {
        if (!block.isAliased(i)) {
            Value &val = frame.unaliasedLocal(block.blockIndexToLocalIndex(i));
            setVar(i, val, DONT_CHECK_ALIASING);
        }
    }
}

StaticBlockObject *
StaticBlockObject::create(ExclusiveContext *cx)
{
    RootedTypeObject type(cx, cx->getNewType(&BlockObject::class_, TaggedProto(nullptr)));
    if (!type)
        return nullptr;

    RootedShape emptyBlockShape(cx);
    emptyBlockShape = EmptyShape::getInitialShape(cx, &BlockObject::class_, TaggedProto(nullptr), nullptr,
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
                          unsigned index, bool *redeclared)
{
    JS_ASSERT(JSID_IS_ATOM(id));
    JS_ASSERT(index < LOCAL_INDEX_LIMIT);

    *redeclared = false;

    
    Shape **spp;
    if (Shape::search(cx, block->lastProperty(), id, &spp, true)) {
        *redeclared = true;
        return nullptr;
    }

    



    uint32_t slot = JSSLOT_FREE(&BlockObject::class_) + index;
    return JSObject::addPropertyInternal<SequentialExecution>(cx, block, id,
                                                               nullptr,
                                                               nullptr,
                                                              slot,
                                                              JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                                               0,
                                                              spp,
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
    uint32_t count = 0, offset = 0;

    if (mode == XDR_ENCODE) {
        obj = *objp;
        count = obj->numVariables();
        offset = obj->localOffset();
    }

    if (mode == XDR_DECODE) {
        obj = StaticBlockObject::create(cx);
        if (!obj)
            return false;
        obj->initEnclosingNestedScope(enclosingScope);
        *objp = obj;
    }

    if (!xdr->codeUint32(&count))
        return false;
    if (!xdr->codeUint32(&offset))
        return false;

    




    if (mode == XDR_DECODE) {
        obj->setLocalOffset(offset);

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

        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront())
            shapes[obj->shapeToIndex(r.front())].set(&r.front());

        RootedShape shape(cx);
        RootedId propid(cx);
        RootedAtom atom(cx);
        for (unsigned i = 0; i < count; i++) {
            shape = shapes[i];
            JS_ASSERT(shape->hasDefaultGetter());
            JS_ASSERT(obj->shapeToIndex(*shape) == i);

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

static JSObject *
CloneStaticBlockObject(JSContext *cx, HandleObject enclosingScope, Handle<StaticBlockObject*> srcBlock)
{
    

    Rooted<StaticBlockObject*> clone(cx, StaticBlockObject::create(cx));
    if (!clone)
        return nullptr;

    clone->initEnclosingNestedScope(enclosingScope);
    clone->setLocalOffset(srcBlock->localOffset());

    
    AutoShapeVector shapes(cx);
    if (!shapes.growBy(srcBlock->numVariables()))
        return nullptr;

    for (Shape::Range<NoGC> r(srcBlock->lastProperty()); !r.empty(); r.popFront())
        shapes[srcBlock->shapeToIndex(r.front())].set(&r.front());

    for (Shape **p = shapes.begin(); p != shapes.end(); ++p) {
        RootedId id(cx, (*p)->propid());
        unsigned i = srcBlock->shapeToIndex(**p);

        bool redeclared;
        if (!StaticBlockObject::addVar(cx, clone, id, i, &redeclared)) {
            JS_ASSERT(!redeclared);
            return nullptr;
        }

        clone->setAliased(i, srcBlock->isAliased(i));
    }

    return clone;
}

JSObject *
js::CloneNestedScopeObject(JSContext *cx, HandleObject enclosingScope, Handle<NestedScopeObject*> srcBlock)
{
    if (srcBlock->is<StaticBlockObject>()) {
        Rooted<StaticBlockObject *> blockObj(cx, &srcBlock->as<StaticBlockObject>());
        return CloneStaticBlockObject(cx, enclosingScope, blockObj);
    } else {
        Rooted<StaticWithObject *> withObj(cx, &srcBlock->as<StaticWithObject>());
        return CloneStaticWithObject(cx, enclosingScope, withObj);
    }
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
    staticScope_(cx, si.staticScope_),
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
    staticScope_(cx, nullptr),
    type_(Type(-1))
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(AbstractFramePtr frame, jsbytecode *pc, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx),
    frame_(frame),
    cur_(cx, frame.scopeChain()),
    staticScope_(cx, frame.script()->getStaticScope(pc))
{
    assertSameCompartment(cx, frame);
    settle();
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(const ScopeIterVal &val, JSContext *cx
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx),
    frame_(val.frame_),
    cur_(cx, val.cur_),
    staticScope_(cx, val.staticScope_),
    type_(val.type_),
    hasScopeObject_(val.hasScopeObject_)
{
    assertSameCompartment(cx, val.frame_);
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
        JS_ASSERT(staticScope_ && staticScope_->is<StaticBlockObject>());
        staticScope_ = staticScope_->as<StaticBlockObject>().enclosingNestedScope();
        if (hasScopeObject_)
            cur_ = &cur_->as<ClonedBlockObject>().enclosingScope();
        settle();
        break;
      case With:
        JS_ASSERT(staticScope_ && staticScope_->is<StaticWithObject>());
        JS_ASSERT(hasScopeObject_);
        staticScope_ = staticScope_->as<StaticWithObject>().enclosingNestedScope();
        cur_ = &cur_->as<DynamicWithObject>().enclosingScope();
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
        if (staticScope_) {
            
            
            JS_ASSERT(staticScope_->is<StaticBlockObject>());
            type_ = Block;
            hasScopeObject_ = staticScope_->as<StaticBlockObject>().needsClone();
        } else {
            type_ = Call;
            hasScopeObject_ = false;
        }
    } else if (frame_.isNonStrictDirectEvalFrame() && cur_ == frame_.evalPrevScopeChain(cx)) {
        if (staticScope_) {
            JS_ASSERT(staticScope_->is<StaticBlockObject>());
            JS_ASSERT(!staticScope_->as<StaticBlockObject>().needsClone());
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
    } else if (staticScope_) {
        if (staticScope_->is<StaticWithObject>()) {
            JS_ASSERT(cur_);
            JS_ASSERT(cur_->as<DynamicWithObject>().staticScope() == staticScope_);
            type_ = With;
            hasScopeObject_ = true;
        } else {
            type_ = Block;
            hasScopeObject_ = staticScope_->as<StaticBlockObject>().needsClone();
            JS_ASSERT_IF(hasScopeObject_,
                         cur_->as<ClonedBlockObject>().staticBlock() == *staticScope_);
        }
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
    
    return size_t(si.frame_.raw()) ^ size_t(si.cur_) ^ size_t(si.staticScope_) ^ si.type_;
}

 bool
ScopeIterKey::match(ScopeIterKey si1, ScopeIterKey si2)
{
    
    return si1.frame_ == si2.frame_ &&
           (!si1.frame_ ||
            (si1.cur_   == si2.cur_   &&
             si1.staticScope_ == si2.staticScope_ &&
             si1.type_  == si2.type_));
}

void
ScopeIterVal::sweep()
{
    
    MOZ_ALWAYS_FALSE(IsObjectAboutToBeFinalized(cur_.unsafeGet()));
    if (staticScope_)
        MOZ_ALWAYS_FALSE(IsObjectAboutToBeFinalized(staticScope_.unsafeGet()));
}






void ScopeIterVal::staticAsserts() {
    static_assert(sizeof(ScopeIterVal) == sizeof(ScopeIterKey),
                  "ScopeIterVal must be same size of ScopeIterKey");
    static_assert(offsetof(ScopeIterVal, cur_) == offsetof(ScopeIterKey, cur_),
                  "ScopeIterVal.cur_ must alias ScopeIterKey.cur_");
    static_assert(offsetof(ScopeIterVal, staticScope_) == offsetof(ScopeIterKey, staticScope_),
                  "ScopeIterVal.staticScope_ must alias ScopeIterKey.staticScope_");
}



namespace {



















class DebugScopeProxy : public BaseProxyHandler
{
    enum Action { SET, GET };

    enum AccessResult {
        ACCESS_UNALIASED,
        ACCESS_GENERIC,
        ACCESS_LOST
    };

    























    bool handleUnaliasedAccess(JSContext *cx, Handle<DebugScopeObject*> debugScope,
                               Handle<ScopeObject*> scope, jsid id, Action action,
                               MutableHandleValue vp, AccessResult *accessResult) const
    {
        JS_ASSERT(&debugScope->scope() == scope);
        *accessResult = ACCESS_GENERIC;
        ScopeIterVal *maybeLiveScope = DebugScopes::hasLiveScope(*scope);

        
        if (scope->is<CallObject>() && !scope->as<CallObject>().isForEval()) {
            CallObject &callobj = scope->as<CallObject>();
            RootedScript script(cx, callobj.callee().nonLazyScript());
            if (!script->ensureHasTypes(cx) || !script->ensureHasAnalyzedArgsUsage(cx))
                return false;

            Bindings &bindings = script->bindings;
            BindingIter bi(script);
            while (bi && NameToId(bi->name()) != id)
                bi++;
            if (!bi)
                return true;

            if (bi->kind() == Binding::VARIABLE || bi->kind() == Binding::CONSTANT) {
                uint32_t i = bi.frameIndex();
                if (script->varIsAliased(i))
                    return true;

                if (maybeLiveScope) {
                    AbstractFramePtr frame = maybeLiveScope->frame();
                    if (action == GET)
                        vp.set(frame.unaliasedVar(i));
                    else
                        frame.unaliasedVar(i) = vp;
                } else if (JSObject *snapshot = debugScope->maybeSnapshot()) {
                    if (action == GET)
                        vp.set(snapshot->getDenseElement(bindings.numArgs() + i));
                    else
                        snapshot->setDenseElement(bindings.numArgs() + i, vp);
                } else {
                    
                    if (action == GET) {
                        *accessResult = ACCESS_LOST;
                        return true;
                    }
                }
            } else {
                JS_ASSERT(bi->kind() == Binding::ARGUMENT);
                unsigned i = bi.frameIndex();
                if (script->formalIsAliased(i))
                    return true;

                if (maybeLiveScope) {
                    AbstractFramePtr frame = maybeLiveScope->frame();
                    if (script->argsObjAliasesFormals() && frame.hasArgsObj()) {
                        if (action == GET)
                            vp.set(frame.argsObj().arg(i));
                        else
                            frame.argsObj().setArg(i, vp);
                    } else {
                        if (action == GET)
                            vp.set(frame.unaliasedFormal(i, DONT_CHECK_ALIASING));
                        else
                            frame.unaliasedFormal(i, DONT_CHECK_ALIASING) = vp;
                    }
                } else if (JSObject *snapshot = debugScope->maybeSnapshot()) {
                    if (action == GET)
                        vp.set(snapshot->getDenseElement(i));
                    else
                        snapshot->setDenseElement(i, vp);
                } else {
                    
                    if (action == GET) {
                        *accessResult = ACCESS_LOST;
                        return true;
                    }
                }

                if (action == SET)
                    TypeScript::SetArgument(cx, script, i, vp);
            }

            *accessResult = ACCESS_UNALIASED;
            return true;
        }

        
        if (scope->is<ClonedBlockObject>()) {
            Rooted<ClonedBlockObject *> block(cx, &scope->as<ClonedBlockObject>());
            Shape *shape = block->lastProperty()->search(cx, id);
            if (!shape)
                return true;

            unsigned i = block->staticBlock().shapeToIndex(*shape);
            if (block->staticBlock().isAliased(i))
                return true;

            if (maybeLiveScope) {
                AbstractFramePtr frame = maybeLiveScope->frame();
                uint32_t local = block->staticBlock().blockIndexToLocalIndex(i);
                JS_ASSERT(local < frame.script()->nfixed());
                if (action == GET)
                    vp.set(frame.unaliasedLocal(local));
                else
                    frame.unaliasedLocal(local) = vp;
            } else {
                if (action == GET)
                    vp.set(block->var(i, DONT_CHECK_ALIASING));
                else
                    block->setVar(i, vp, DONT_CHECK_ALIASING);
            }

            *accessResult = ACCESS_UNALIASED;
            return true;
        }

        
        JS_ASSERT(scope->is<DeclEnvObject>() || scope->is<DynamicWithObject>() ||
                  scope->as<CallObject>().isForEval());
        return true;
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

    





    static bool isMissingArguments(JSContext *cx, jsid id, ScopeObject &scope)
    {
        return isArguments(cx, id) && isFunctionScope(scope) &&
               !scope.as<CallObject>().callee().nonLazyScript()->needsArgsObj();
    }

    



    static bool createMissingArguments(JSContext *cx, jsid id, ScopeObject &scope,
                                       MutableHandleArgumentsObject argsObj)
    {
        MOZ_ASSERT(isMissingArguments(cx, id, scope));
        argsObj.set(nullptr);

        ScopeIterVal *maybeScope = DebugScopes::hasLiveScope(scope);
        if (!maybeScope)
            return true;

        argsObj.set(ArgumentsObject::createUnexpected(cx, maybeScope->frame()));
        return !!argsObj;
    }

  public:
    static const char family;
    static const DebugScopeProxy singleton;

    MOZ_CONSTEXPR DebugScopeProxy() : BaseProxyHandler(&family) {}

    bool isExtensible(JSContext *cx, HandleObject proxy, bool *extensible) const MOZ_OVERRIDE
    {
        
        
        *extensible = true;
        return true;
    }

    bool preventExtensions(JSContext *cx, HandleObject proxy) const MOZ_OVERRIDE
    {
        
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CHANGE_EXTENSIBILITY);
        return false;
    }

    bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                               MutableHandle<PropertyDescriptor> desc) const MOZ_OVERRIDE
    {
        return getOwnPropertyDescriptor(cx, proxy, id, desc);
    }

    bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                  MutableHandle<PropertyDescriptor> desc) const MOZ_OVERRIDE
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &debugScope->scope());

        if (isMissingArguments(cx, id, *scope)) {
            RootedArgumentsObject argsObj(cx);
            if (!createMissingArguments(cx, id, *scope, &argsObj))
                return false;

            if (!argsObj) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_LIVE,
                                     "Debugger scope");
                return false;
            }

            desc.object().set(debugScope);
            desc.setAttributes(JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT);
            desc.value().setObject(*argsObj);
            desc.setGetter(nullptr);
            desc.setSetter(nullptr);
            return true;
        }

        RootedValue v(cx);
        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, GET, &v, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            desc.object().set(debugScope);
            desc.setAttributes(JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT);
            desc.value().set(v);
            desc.setGetter(nullptr);
            desc.setSetter(nullptr);
            return true;
          case ACCESS_GENERIC:
            return JS_GetOwnPropertyDescriptorById(cx, scope, id, desc);
          case ACCESS_LOST:
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_OPTIMIZED_OUT);
            return false;
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool get(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
             MutableHandleValue vp) const MOZ_OVERRIDE
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        if (isMissingArguments(cx, id, *scope)) {
            RootedArgumentsObject argsObj(cx);
            if (!createMissingArguments(cx, id, *scope, &argsObj))
                return false;

            if (!argsObj) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_LIVE,
                                     "Debugger scope");
                return false;
            }

            vp.setObject(*argsObj);
            return true;
        }

        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, GET, vp, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            return true;
          case ACCESS_GENERIC:
            return JSObject::getGeneric(cx, scope, scope, id, vp);
          case ACCESS_LOST:
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_OPTIMIZED_OUT);
            return false;
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    



    bool getMaybeSentinelValue(JSContext *cx, Handle<DebugScopeObject *> debugScope, HandleId id,
                               MutableHandleValue vp) const
    {
        Rooted<ScopeObject*> scope(cx, &debugScope->scope());

        if (isMissingArguments(cx, id, *scope)) {
            RootedArgumentsObject argsObj(cx);
            if (!createMissingArguments(cx, id, *scope, &argsObj))
                return false;
            vp.set(argsObj ? ObjectValue(*argsObj) : MagicValue(JS_OPTIMIZED_ARGUMENTS));
            return true;
        }

        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, GET, vp, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            return true;
          case ACCESS_GENERIC:
            return JSObject::getGeneric(cx, scope, scope, id, vp);
          case ACCESS_LOST:
            vp.setMagic(JS_OPTIMIZED_OUT);
            return true;
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool set(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id, bool strict,
             MutableHandleValue vp) const MOZ_OVERRIDE
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, SET, vp, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            return true;
          case ACCESS_GENERIC:
            return JSObject::setGeneric(cx, scope, scope, id, vp, strict);
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                        MutableHandle<PropertyDescriptor> desc) const MOZ_OVERRIDE
    {
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        bool found;
        if (!has(cx, proxy, id, &found))
            return false;
        if (found)
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);

        return JS_DefinePropertyById(cx, scope, id, desc.value(), desc.attributes(), desc.getter(), desc.setter());
    }

    bool getScopePropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props,
                               unsigned flags) const
    {
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        if (isMissingArgumentsBinding(*scope)) {
            if (!props.append(NameToId(cx->names().arguments)))
                return false;
        }

        
        
        
        
        
        
        Rooted<JSObject*> target(cx, (scope->is<DynamicWithObject>()
                                      ? &scope->as<DynamicWithObject>().object() : scope));
        if (!GetPropertyNames(cx, target, flags, &props))
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

    bool getOwnPropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props) const MOZ_OVERRIDE
    {
        return getScopePropertyNames(cx, proxy, props, JSITER_OWNONLY);
    }

    bool enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props) const MOZ_OVERRIDE
    {
        return getScopePropertyNames(cx, proxy, props, 0);
    }

    bool has(JSContext *cx, HandleObject proxy, HandleId id_, bool *bp) const MOZ_OVERRIDE
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

    bool delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp) const MOZ_OVERRIDE
    {
        RootedValue idval(cx, IdToValue(id));
        return js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_CANT_DELETE,
                                        JSDVG_IGNORE_STACK, idval, NullPtr(), nullptr, nullptr);
    }
};

} 

const char DebugScopeProxy::family = 0;
const DebugScopeProxy DebugScopeProxy::singleton;

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
DebugScopeObject::getMaybeSentinelValue(JSContext *cx, HandleId id, MutableHandleValue vp)
{
    Rooted<DebugScopeObject *> self(cx, this);
    return DebugScopeProxy::singleton.getMaybeSentinelValue(cx, self, id, vp);
}

bool
js_IsDebugScopeSlow(ProxyObject *proxy)
{
    JS_ASSERT(proxy->hasClass(&ProxyObject::class_));
    return proxy->handler() == &DebugScopeProxy::singleton;
}



 MOZ_ALWAYS_INLINE void
DebugScopes::proxiedScopesPostWriteBarrier(JSRuntime *rt, ObjectWeakMap *map,
                                           const PreBarrieredObject &key)
{
#ifdef JSGC_GENERATIONAL
    








    ObjectWeakMap::Base *baseHashMap = static_cast<ObjectWeakMap::Base *>(map);

    typedef HashMap<JSObject *, JSObject *> UnbarrieredMap;
    UnbarrieredMap *unbarrieredMap = reinterpret_cast<UnbarrieredMap *>(baseHashMap);

    typedef gc::HashKeyRef<UnbarrieredMap, JSObject *> Ref;
    if (key && IsInsideNursery(key))
        rt->gc.storeBuffer.putGeneric(Ref(unbarrieredMap, key.get()));
#endif
}

#ifdef JSGC_GENERATIONAL
class DebugScopes::MissingScopesRef : public gc::BufferableRef
{
    MissingScopeMap *map;
    ScopeIterKey key;

  public:
    MissingScopesRef(MissingScopeMap *m, const ScopeIterKey &k) : map(m), key(k) {}

    void mark(JSTracer *trc) {
        ScopeIterKey prior = key;
        MissingScopeMap::Ptr p = map->lookup(key);
        if (!p)
            return;
        trc->setTracingLocation(&const_cast<ScopeIterKey &>(p->key()).enclosingScope());
        Mark(trc, &key.enclosingScope(), "MissingScopesRef");
        map->rekeyIfMoved(prior, key);
    }
};
#endif

 MOZ_ALWAYS_INLINE void
DebugScopes::missingScopesPostWriteBarrier(JSRuntime *rt, MissingScopeMap *map,
                                           const ScopeIterKey &key)
{
#ifdef JSGC_GENERATIONAL
    if (key.enclosingScope() && IsInsideNursery(key.enclosingScope()))
        rt->gc.storeBuffer.putGeneric(MissingScopesRef(map, key));
#endif
}

 MOZ_ALWAYS_INLINE void
DebugScopes::liveScopesPostWriteBarrier(JSRuntime *rt, LiveScopeMap *map, ScopeObject *key)
{
#ifdef JSGC_GENERATIONAL
    
    
    typedef HashMap<ScopeObject *,
                    ScopeIterKey,
                    DefaultHasher<ScopeObject *>,
                    RuntimeAllocPolicy> UnbarrieredLiveScopeMap;
    typedef gc::HashKeyRef<UnbarrieredLiveScopeMap, ScopeObject *> Ref;
    if (key && IsInsideNursery(key))
        rt->gc.storeBuffer.putGeneric(Ref(reinterpret_cast<UnbarrieredLiveScopeMap *>(map), key));
#endif
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
        DebugScopeObject **debugScope = e.front().value().unsafeGet();
        if (IsObjectAboutToBeFinalized(debugScope)) {
            
















            liveScopes.remove(&(*debugScope)->scope());
            e.removeFront();
        } else {
            ScopeIterKey key = e.front().key();
            bool needsUpdate = false;
            if (IsForwarded(key.cur())) {
                key.updateCur(js::gc::Forwarded(key.cur()));
                needsUpdate = true;
            }
            if (key.staticScope() && IsForwarded(key.staticScope())) {
                key.updateStaticScope(Forwarded(key.staticScope()));
                needsUpdate = true;
            }
            if (needsUpdate)
                e.rekeyFront(key);
        }
    }

    for (LiveScopeMap::Enum e(liveScopes); !e.empty(); e.popFront()) {
        ScopeObject *scope = e.front().key();

        e.front().value().sweep();

        



        if (IsObjectAboutToBeFinalized(&scope))
            e.removeFront();
        else if (scope != e.front().key())
            e.rekeyFront(scope);
    }
}

#ifdef JSGC_HASH_TABLE_CHECKS
void
DebugScopes::checkHashTablesAfterMovingGC(JSRuntime *runtime)
{
    




    for (ObjectWeakMap::Range r = proxiedScopes.all(); !r.empty(); r.popFront()) {
        CheckGCThingAfterMovingGC(r.front().key().get());
        CheckGCThingAfterMovingGC(r.front().value().get());
    }
    for (MissingScopeMap::Range r = missingScopes.all(); !r.empty(); r.popFront()) {
        CheckGCThingAfterMovingGC(r.front().key().cur());
        CheckGCThingAfterMovingGC(r.front().key().staticScope());
        CheckGCThingAfterMovingGC(r.front().value().get());
    }
    for (LiveScopeMap::Range r = liveScopes.all(); !r.empty(); r.popFront()) {
        CheckGCThingAfterMovingGC(r.front().key());
        CheckGCThingAfterMovingGC(r.front().value().cur_.get());
        CheckGCThingAfterMovingGC(r.front().value().staticScope_.get());
    }
}
#endif








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
        return &p->value()->as<DebugScopeObject>();
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

    proxiedScopesPostWriteBarrier(cx->runtime(), &scopes->proxiedScopes, &scope);
    return true;
}

DebugScopeObject *
DebugScopes::hasDebugScope(JSContext *cx, const ScopeIter &si)
{
    JS_ASSERT(!si.hasScopeObject());

    DebugScopes *scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(ScopeIterKey(si))) {
        JS_ASSERT(CanUseDebugScopeMaps(cx));
        return p->value();
    }
    return nullptr;
}

bool
DebugScopes::addDebugScope(JSContext *cx, const ScopeIter &si, DebugScopeObject &debugScope)
{
    JS_ASSERT(!si.hasScopeObject());
    JS_ASSERT(cx->compartment() == debugScope.compartment());
    JS_ASSERT_IF(si.frame().isFunctionFrame(), !si.frame().callee()->isGenerator());

    if (!CanUseDebugScopeMaps(cx))
        return true;

    DebugScopes *scopes = ensureCompartmentData(cx);
    if (!scopes)
        return false;

    JS_ASSERT(!scopes->missingScopes.has(ScopeIterKey(si)));
    if (!scopes->missingScopes.put(ScopeIterKey(si), ReadBarriered<DebugScopeObject*>(&debugScope))) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    missingScopesPostWriteBarrier(cx->runtime(), &scopes->missingScopes, ScopeIterKey(si));

    JS_ASSERT(!scopes->liveScopes.has(&debugScope.scope()));
    if (!scopes->liveScopes.put(&debugScope.scope(), ScopeIterVal(si))) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    liveScopesPostWriteBarrier(cx->runtime(), &scopes->liveScopes, &debugScope.scope());

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
            debugScope = &p->value()->as<DebugScopeObject>();
    } else {
        ScopeIter si(frame, frame.script()->main(), cx);
        if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(ScopeIterKey(si))) {
            debugScope = p->value();
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
                    vec[i].set(frame.argsObj().arg(i));
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
DebugScopes::onPopBlock(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc)
{
    assertSameCompartment(cx, frame);

    DebugScopes *scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    ScopeIter si(frame, pc, cx);
    onPopBlock(cx, si);
}

void
DebugScopes::onPopBlock(JSContext *cx, const ScopeIter &si)
{
    DebugScopes *scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    JS_ASSERT(si.type() == ScopeIter::Block);

    if (si.staticBlock().needsClone()) {
        ClonedBlockObject &clone = si.scope().as<ClonedBlockObject>();
        clone.copyUnaliasedValues(si.frame());
        scopes->liveScopes.remove(&clone);
    } else {
        if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(ScopeIterKey(si))) {
            ClonedBlockObject &clone = p->value()->scope().as<ClonedBlockObject>();
            clone.copyUnaliasedValues(si.frame());
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
        scopes->liveScopes.remove(&frame.scopeChain()->as<DynamicWithObject>());
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
        if (!i.hasUsableAbstractFramePtr())
            continue;

        AbstractFramePtr frame = i.abstractFramePtr();
        if (frame.scopeChain()->compartment() != cx->compartment())
            continue;

        if (frame.isFunctionFrame() && frame.callee()->isGenerator())
            continue;

        for (ScopeIter si(frame, i.pc(), cx); !si.done(); ++si) {
            if (si.hasScopeObject()) {
                JS_ASSERT(si.scope().compartment() == cx->compartment());
                DebugScopes *scopes = ensureCompartmentData(cx);
                if (!scopes)
                    return false;
                if (!scopes->liveScopes.put(&si.scope(), ScopeIterVal(si)))
                    return false;
                liveScopesPostWriteBarrier(cx->runtime(), &scopes->liveScopes, &si.scope());
            }
        }

        if (frame.prevUpToDate())
            return true;
        JS_ASSERT(frame.scopeChain()->compartment()->debugMode());
        frame.setPrevUpToDate();
    }

    return true;
}

ScopeIterVal*
DebugScopes::hasLiveScope(ScopeObject &scope)
{
    DebugScopes *scopes = scope.compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (LiveScopeMap::Ptr p = scopes->liveScopes.lookup(&scope))
        return &p->value();

    return nullptr;
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
        
        JS_ASSERT(!si.frame().callee()->isGenerator());
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
        
        JS_ASSERT_IF(si.frame().isFunctionFrame(), !si.frame().callee()->isGenerator());
        Rooted<StaticBlockObject *> staticBlock(cx, &si.staticBlock());
        ClonedBlockObject *block = ClonedBlockObject::create(cx, staticBlock, si.frame());
        if (!block)
            return nullptr;

        debugScope = DebugScopeObject::create(cx, *block, enclosingDebug);
        break;
      }
      case ScopeIter::With:
      case ScopeIter::StrictEvalScope:
        MOZ_CRASH("should already have a scope");
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
    if (ScopeIterVal *maybeLiveScope = DebugScopes::hasLiveScope(*scope)) {
        ScopeIter si(*maybeLiveScope, cx);
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
js::GetDebugScopeForFrame(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc)
{
    assertSameCompartment(cx, frame);
    if (CanUseDebugScopeMaps(cx) && !DebugScopes::updateLiveScopes(cx))
        return nullptr;
    ScopeIter si(frame, pc, cx);
    return GetDebugScope(cx, si);
}

#ifdef DEBUG

typedef HashSet<PropertyName *> PropertyNameSet;

static bool
RemoveReferencedNames(JSContext *cx, HandleScript script, PropertyNameSet &remainingNames)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    for (jsbytecode *pc = script->code(); pc != script->codeEnd(); pc += GetBytecodeLength(pc)) {
        PropertyName *name;

        switch (JSOp(*pc)) {
          case JSOP_NAME:
          case JSOP_SETNAME:
            name = script->getName(pc);
            break;

          case JSOP_GETALIASEDVAR:
          case JSOP_SETALIASEDVAR:
            name = ScopeCoordinateName(cx->runtime()->scopeCoordinateNameCache, script, pc);
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

        if (JSAtom *name = script->functionNonDelazifying()->displayAtom()) {
            buf.putString(name);
            buf.printf(" ");
        }

        buf.printf("(%s:%d) has variables entrained by ", script->filename(), script->lineno());

        if (JSAtom *name = innerScript->functionNonDelazifying()->displayAtom()) {
            buf.putString(name);
            buf.printf(" ");
        }

        buf.printf("(%s:%d) ::", innerScript->filename(), innerScript->lineno());

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
                RootedScript innerInnerScript(cx, fun->getOrCreateScript(cx));
                if (!innerInnerScript ||
                    !AnalyzeEntrainedVariablesInScript(cx, script, innerInnerScript))
                {
                    return false;
                }
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

            if (script->functionDelazifying() && script->functionDelazifying()->isHeavyweight()) {
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
