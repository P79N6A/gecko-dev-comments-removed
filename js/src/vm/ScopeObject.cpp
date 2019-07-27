





#include "vm/ScopeObject-inl.h"

#include "mozilla/PodOperations.h"
#include "mozilla/SizePrintfMacros.h"

#include "jscompartment.h"
#include "jsiter.h"

#include "vm/ArgumentsObject.h"
#include "vm/GlobalObject.h"
#include "vm/ProxyObject.h"
#include "vm/Shape.h"
#include "vm/WeakMapObject.h"
#include "vm/Xdr.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::PodZero;

typedef Rooted<ArgumentsObject*> RootedArgumentsObject;
typedef MutableHandle<ArgumentsObject*> MutableHandleArgumentsObject;



Shape*
js::ScopeCoordinateToStaticScopeShape(JSScript* script, jsbytecode* pc)
{
    MOZ_ASSERT(JOF_OPTYPE(JSOp(*pc)) == JOF_SCOPECOORD);
    StaticScopeIter<NoGC> ssi(script->innermostStaticScopeInScript(pc));
    uint32_t hops = ScopeCoordinate(pc).hops();
    while (true) {
        MOZ_ASSERT(!ssi.done());
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

PropertyName*
js::ScopeCoordinateName(ScopeCoordinateNameCache& cache, JSScript* script, jsbytecode* pc)
{
    Shape* shape = ScopeCoordinateToStaticScopeShape(script, pc);
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

JSScript*
js::ScopeCoordinateFunctionScript(JSScript* script, jsbytecode* pc)
{
    MOZ_ASSERT(JOF_OPTYPE(JSOp(*pc)) == JOF_SCOPECOORD);
    StaticScopeIter<NoGC> ssi(script->innermostStaticScopeInScript(pc));
    uint32_t hops = ScopeCoordinate(pc).hops();
    while (true) {
        if (ssi.hasDynamicScopeObject()) {
            if (!hops)
                break;
            hops--;
        }
        ssi++;
    }
    if (ssi.type() != StaticScopeIter<NoGC>::Function)
        return nullptr;
    return ssi.funScript();
}



void
ScopeObject::setEnclosingScope(HandleObject obj)
{
    MOZ_ASSERT_IF(obj->is<CallObject>() || obj->is<DeclEnvObject>() || obj->is<BlockObject>(),
                  obj->isDelegate());
    setFixedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*obj));
}

CallObject*
CallObject::create(JSContext* cx, HandleShape shape, HandleObjectGroup group, uint32_t lexicalBegin)
{
    MOZ_ASSERT(!group->singleton(),
               "passed a singleton group to create() (use createSingleton() "
               "instead)");
    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    MOZ_ASSERT(CanBeFinalizedInBackground(kind, &CallObject::class_));
    kind = gc::GetBackgroundAllocKind(kind);

    JSObject* obj = JSObject::create(cx, kind, gc::DefaultHeap, shape, group);
    if (!obj)
        return nullptr;

    obj->as<CallObject>().initRemainingSlotsToUninitializedLexicals(lexicalBegin);
    return &obj->as<CallObject>();
}

CallObject*
CallObject::createSingleton(JSContext* cx, HandleShape shape, uint32_t lexicalBegin)
{
    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    MOZ_ASSERT(CanBeFinalizedInBackground(kind, &CallObject::class_));
    kind = gc::GetBackgroundAllocKind(kind);

    RootedObjectGroup group(cx, ObjectGroup::lazySingletonGroup(cx, &class_, TaggedProto(nullptr)));
    if (!group)
        return nullptr;
    RootedObject obj(cx, JSObject::create(cx, kind, gc::TenuredHeap, shape, group));
    if (!obj)
        return nullptr;

    MOZ_ASSERT(obj->isSingleton(),
               "group created inline above must be a singleton");

    obj->as<CallObject>().initRemainingSlotsToUninitializedLexicals(lexicalBegin);
    return &obj->as<CallObject>();
}






CallObject*
CallObject::createTemplateObject(JSContext* cx, HandleScript script, gc::InitialHeap heap)
{
    RootedShape shape(cx, script->bindings.callObjShape());
    MOZ_ASSERT(shape->getObjectClass() == &class_);

    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &class_, TaggedProto(nullptr)));
    if (!group)
        return nullptr;

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    MOZ_ASSERT(CanBeFinalizedInBackground(kind, &class_));
    kind = gc::GetBackgroundAllocKind(kind);

    JSObject* obj = JSObject::create(cx, kind, heap, shape, group);
    if (!obj)
        return nullptr;

    
    
    obj->as<CallObject>().initAliasedLexicalsToThrowOnTouch(script);

    return &obj->as<CallObject>();
}







CallObject*
CallObject::create(JSContext* cx, HandleScript script, HandleObject enclosing, HandleFunction callee)
{
    gc::InitialHeap heap = script->treatAsRunOnce() ? gc::TenuredHeap : gc::DefaultHeap;
    CallObject* callobj = CallObject::createTemplateObject(cx, script, heap);
    if (!callobj)
        return nullptr;

    callobj->as<ScopeObject>().setEnclosingScope(enclosing);
    callobj->initFixedSlot(CALLEE_SLOT, ObjectOrNullValue(callee));

    if (script->treatAsRunOnce()) {
        Rooted<CallObject*> ncallobj(cx, callobj);
        if (!JSObject::setSingleton(cx, ncallobj))
            return nullptr;
        return ncallobj;
    }

    return callobj;
}

CallObject*
CallObject::createForFunction(JSContext* cx, HandleObject enclosing, HandleFunction callee)
{
    RootedObject scopeChain(cx, enclosing);
    MOZ_ASSERT(scopeChain);

    



    if (callee->isNamedLambda()) {
        scopeChain = DeclEnvObject::create(cx, scopeChain, callee);
        if (!scopeChain)
            return nullptr;
    }

    RootedScript script(cx, callee->nonLazyScript());
    return create(cx, script, scopeChain, callee);
}

CallObject*
CallObject::createForFunction(JSContext* cx, AbstractFramePtr frame)
{
    MOZ_ASSERT(frame.isNonEvalFunctionFrame());
    assertSameCompartment(cx, frame);

    RootedObject scopeChain(cx, frame.scopeChain());
    RootedFunction callee(cx, frame.callee());

    CallObject* callobj = createForFunction(cx, scopeChain, callee);
    if (!callobj)
        return nullptr;

    
    for (AliasedFormalIter i(frame.script()); i; i++) {
        callobj->setAliasedVar(cx, i, i->name(),
                               frame.unaliasedFormal(i.frameIndex(), DONT_CHECK_ALIASING));
    }

    return callobj;
}

CallObject*
CallObject::createForStrictEval(JSContext* cx, AbstractFramePtr frame)
{
    MOZ_ASSERT(frame.isStrictEvalFrame());
    MOZ_ASSERT_IF(frame.isInterpreterFrame(), cx->interpreterFrame() == frame.asInterpreterFrame());
    MOZ_ASSERT_IF(frame.isInterpreterFrame(), cx->interpreterRegs().pc == frame.script()->code());

    RootedFunction callee(cx);
    RootedScript script(cx, frame.script());
    RootedObject scopeChain(cx, frame.scopeChain());
    return create(cx, script, scopeChain, callee);
}

CallObject*
CallObject::createHollowForDebug(JSContext* cx, HandleFunction callee)
{
    MOZ_ASSERT(!callee->isHeavyweight());

    
    
    
    
    Rooted<GlobalObject*> global(cx, &callee->global());
    Rooted<CallObject*> callobj(cx, createForFunction(cx, global, callee));
    if (!callobj)
        return nullptr;

    RootedValue optimizedOut(cx, MagicValue(JS_OPTIMIZED_OUT));
    RootedId id(cx);
    RootedScript script(cx, callee->nonLazyScript());
    for (BindingIter bi(script); !bi.done(); bi++) {
        id = NameToId(bi->name());
        if (!SetProperty(cx, callobj, id, optimizedOut))
            return nullptr;
    }

    return callobj;
}

const Class CallObject::class_ = {
    "Call",
    JSCLASS_IS_ANONYMOUS | JSCLASS_HAS_RESERVED_SLOTS(CallObject::RESERVED_SLOTS)
};

const Class DeclEnvObject::class_ = {
    js_Object_str,
    JSCLASS_HAS_RESERVED_SLOTS(DeclEnvObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object)
};






DeclEnvObject*
DeclEnvObject::createTemplateObject(JSContext* cx, HandleFunction fun, gc::InitialHeap heap)
{
    MOZ_ASSERT(IsNurseryAllocable(FINALIZE_KIND));

    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &class_, TaggedProto(nullptr)));
    if (!group)
        return nullptr;

    RootedShape emptyDeclEnvShape(cx);
    emptyDeclEnvShape = EmptyShape::getInitialShape(cx, &class_, TaggedProto(nullptr),
                                                    FINALIZE_KIND, BaseShape::DELEGATE);
    if (!emptyDeclEnvShape)
        return nullptr;

    RootedNativeObject obj(cx, MaybeNativeObject(JSObject::create(cx, FINALIZE_KIND, heap,
                                                                  emptyDeclEnvShape, group)));
    if (!obj)
        return nullptr;

    
    Rooted<jsid> id(cx, AtomToId(fun->atom()));
    const Class* clasp = obj->getClass();
    unsigned attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY;

    JSGetterOp getter = clasp->getProperty;
    JSSetterOp setter = clasp->setProperty;
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    if (!NativeObject::putProperty(cx, obj, id, getter, setter, lambdaSlot(), attrs, 0))
        return nullptr;

    MOZ_ASSERT(!obj->hasDynamicSlots());
    return &obj->as<DeclEnvObject>();
}

DeclEnvObject*
DeclEnvObject::create(JSContext* cx, HandleObject enclosing, HandleFunction callee)
{
    Rooted<DeclEnvObject*> obj(cx, createTemplateObject(cx, callee, gc::DefaultHeap));
    if (!obj)
        return nullptr;

    obj->setEnclosingScope(enclosing);
    obj->setFixedSlot(lambdaSlot(), ObjectValue(*callee));
    return obj;
}

template<XDRMode mode>
bool
js::XDRStaticWithObject(XDRState<mode>* xdr, HandleObject enclosingScope,
                        MutableHandle<StaticWithObject*> objp)
{
    if (mode == XDR_DECODE) {
        JSContext* cx = xdr->cx();
        Rooted<StaticWithObject*> obj(cx, StaticWithObject::create(cx));
        if (!obj)
            return false;
        obj->initEnclosingNestedScope(enclosingScope);
        objp.set(obj);
    }
    
    
    

    return true;
}

template bool
js::XDRStaticWithObject(XDRState<XDR_ENCODE>*, HandleObject, MutableHandle<StaticWithObject*>);

template bool
js::XDRStaticWithObject(XDRState<XDR_DECODE>*, HandleObject, MutableHandle<StaticWithObject*>);

StaticWithObject*
StaticWithObject::create(ExclusiveContext* cx)
{
    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &class_, TaggedProto(nullptr)));
    if (!group)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(nullptr),
                                                      FINALIZE_KIND));
    if (!shape)
        return nullptr;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, gc::TenuredHeap, shape, group));
    if (!obj)
        return nullptr;

    return &obj->as<StaticWithObject>();
}

static JSObject*
CloneStaticWithObject(JSContext* cx, HandleObject enclosingScope, Handle<StaticWithObject*> srcWith)
{
    Rooted<StaticWithObject*> clone(cx, StaticWithObject::create(cx));
    if (!clone)
        return nullptr;

    clone->initEnclosingNestedScope(enclosingScope);

    return clone;
}

DynamicWithObject*
DynamicWithObject::create(JSContext* cx, HandleObject object, HandleObject enclosing,
                          HandleObject staticWith, WithKind kind)
{
    MOZ_ASSERT(staticWith->is<StaticWithObject>());
    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &class_,
                                                             TaggedProto(staticWith.get())));
    if (!group)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(staticWith),
                                                      FINALIZE_KIND));
    if (!shape)
        return nullptr;

    RootedNativeObject obj(cx, MaybeNativeObject(JSObject::create(cx, FINALIZE_KIND,
                                                                  gc::DefaultHeap, shape, group)));
    if (!obj)
        return nullptr;

    JSObject* thisp = GetThisObject(cx, object);
    if (!thisp)
        return nullptr;

    obj->as<ScopeObject>().setEnclosingScope(enclosing);
    obj->setFixedSlot(OBJECT_SLOT, ObjectValue(*object));
    obj->setFixedSlot(THIS_SLOT, ObjectValue(*thisp));
    obj->setFixedSlot(KIND_SLOT, Int32Value(kind));

    return &obj->as<DynamicWithObject>();
}

static bool
with_LookupProperty(JSContext* cx, HandleObject obj, HandleId id,
                    MutableHandleObject objp, MutableHandleShape propp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return LookupProperty(cx, actual, id, objp, propp);
}

static bool
with_DefineProperty(JSContext* cx, HandleObject obj, HandleId id, Handle<PropertyDescriptor> desc,
                    ObjectOpResult& result)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return DefineProperty(cx, actual, id, desc, result);
}

static bool
with_HasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return HasProperty(cx, actual, id, foundp);
}

static bool
with_GetProperty(JSContext* cx, HandleObject obj, HandleObject receiver, HandleId id,
                 MutableHandleValue vp)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return GetProperty(cx, actual, actual, id, vp);
}

static bool
with_SetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                 HandleValue receiver, ObjectOpResult& result)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    RootedValue actualReceiver(cx, receiver);
    if (receiver.isObject() && &receiver.toObject() == obj)
        actualReceiver.setObject(*actual);
    return SetProperty(cx, actual, id, v, actualReceiver, result);
}

static bool
with_GetOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                              MutableHandle<JSPropertyDescriptor> desc)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return GetOwnPropertyDescriptor(cx, actual, id, desc);
}

static bool
with_DeleteProperty(JSContext* cx, HandleObject obj, HandleId id, ObjectOpResult& result)
{
    RootedObject actual(cx, &obj->as<DynamicWithObject>().object());
    return DeleteProperty(cx, actual, id, result);
}

static JSObject*
with_ThisObject(JSContext* cx, HandleObject obj)
{
    return &obj->as<DynamicWithObject>().withThis();
}

const Class StaticWithObject::class_ = {
    "WithTemplate",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(StaticWithObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS
};

const Class DynamicWithObject::class_ = {
    "With",
    JSCLASS_HAS_RESERVED_SLOTS(DynamicWithObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    {
        with_LookupProperty,
        with_DefineProperty,
        with_HasProperty,
        with_GetProperty,
        with_SetProperty,
        with_GetOwnPropertyDescriptor,
        with_DeleteProperty,
        nullptr, nullptr,    
        nullptr,             
        nullptr,             
        with_ThisObject,
    }
};

 StaticEvalObject*
StaticEvalObject::create(JSContext* cx, HandleObject enclosing)
{
    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &class_, TaggedProto(nullptr)));
    if (!group)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(nullptr),
                                                      FINALIZE_KIND, BaseShape::DELEGATE));
    if (!shape)
        return nullptr;

    RootedNativeObject obj(cx, MaybeNativeObject(JSObject::create(cx, FINALIZE_KIND,
                                                                  gc::TenuredHeap, shape, group)));
    if (!obj)
        return nullptr;

    obj->setReservedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(enclosing));
    obj->setReservedSlot(STRICT_SLOT, BooleanValue(false));
    return &obj->as<StaticEvalObject>();
}

const Class StaticEvalObject::class_ = {
    "StaticEval",
    JSCLASS_HAS_RESERVED_SLOTS(StaticEvalObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS
};



 ClonedBlockObject*
ClonedBlockObject::create(JSContext* cx, Handle<StaticBlockObject*> block, HandleObject enclosing)
{
    MOZ_ASSERT(block->getClass() == &BlockObject::class_);

    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &BlockObject::class_,
                                                             TaggedProto(block.get())));
    if (!group)
        return nullptr;

    RootedShape shape(cx, block->lastProperty());

    RootedNativeObject obj(cx, MaybeNativeObject(JSObject::create(cx, FINALIZE_KIND,
                                                                  gc::TenuredHeap, shape, group)));
    if (!obj)
        return nullptr;

    MOZ_ASSERT(!obj->inDictionaryMode());
    MOZ_ASSERT(obj->slotSpan() >= block->numVariables() + RESERVED_SLOTS);

    obj->setReservedSlot(SCOPE_CHAIN_SLOT, ObjectValue(*enclosing));

    MOZ_ASSERT(obj->isDelegate());

    return &obj->as<ClonedBlockObject>();
}

 ClonedBlockObject*
ClonedBlockObject::create(JSContext* cx, Handle<StaticBlockObject*> block, AbstractFramePtr frame)
{
    assertSameCompartment(cx, frame);
    RootedObject enclosing(cx, frame.scopeChain());
    return create(cx, block, enclosing);
}

 ClonedBlockObject*
ClonedBlockObject::createHollowForDebug(JSContext* cx, Handle<StaticBlockObject*> block)
{
    MOZ_ASSERT(!block->needsClone());

    
    
    
    
    Rooted<GlobalObject*> global(cx, &block->global());
    Rooted<ClonedBlockObject*> obj(cx, create(cx, block, global));
    if (!obj)
        return nullptr;

    for (unsigned i = 0; i < block->numVariables(); i++)
        obj->setVar(i, MagicValue(JS_OPTIMIZED_OUT), DONT_CHECK_ALIASING);

    return obj;
}

void
ClonedBlockObject::copyUnaliasedValues(AbstractFramePtr frame)
{
    StaticBlockObject& block = staticBlock();
    for (unsigned i = 0; i < numVariables(); ++i) {
        if (!block.isAliased(i)) {
            Value& val = frame.unaliasedLocal(block.blockIndexToLocalIndex(i));
            setVar(i, val, DONT_CHECK_ALIASING);
        }
    }
}

 ClonedBlockObject*
ClonedBlockObject::clone(JSContext* cx, Handle<ClonedBlockObject*> clonedBlock)
{
    Rooted<StaticBlockObject*> staticBlock(cx, &clonedBlock->staticBlock());
    RootedObject enclosing(cx, &clonedBlock->enclosingScope());

    Rooted<ClonedBlockObject*> copy(cx, create(cx, staticBlock, enclosing));
    if (!copy)
        return nullptr;

    for (uint32_t i = 0, count = staticBlock->numVariables(); i < count; i++)
        copy->setVar(i, clonedBlock->var(i, DONT_CHECK_ALIASING), DONT_CHECK_ALIASING);

    return copy;
}

StaticBlockObject*
StaticBlockObject::create(ExclusiveContext* cx)
{
    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &BlockObject::class_,
                                                             TaggedProto(nullptr)));
    if (!group)
        return nullptr;

    RootedShape emptyBlockShape(cx);
    emptyBlockShape = EmptyShape::getInitialShape(cx, &BlockObject::class_, TaggedProto(nullptr),
                                                  FINALIZE_KIND, BaseShape::DELEGATE);
    if (!emptyBlockShape)
        return nullptr;

    JSObject* obj = JSObject::create(cx, FINALIZE_KIND, gc::TenuredHeap, emptyBlockShape, group);
    if (!obj)
        return nullptr;

    return &obj->as<StaticBlockObject>();
}

 Shape*
StaticBlockObject::addVar(ExclusiveContext* cx, Handle<StaticBlockObject*> block, HandleId id,
                          bool constant, unsigned index, bool* redeclared)
{
    MOZ_ASSERT(JSID_IS_ATOM(id));
    MOZ_ASSERT(index < LOCAL_INDEX_LIMIT);

    *redeclared = false;

    
    ShapeTable::Entry* entry;
    if (Shape::search(cx, block->lastProperty(), id, &entry, true)) {
        *redeclared = true;
        return nullptr;
    }

    



    uint32_t slot = JSSLOT_FREE(&BlockObject::class_) + index;
    uint32_t readonly = constant ? JSPROP_READONLY : 0;
    uint32_t propFlags = readonly | JSPROP_ENUMERATE | JSPROP_PERMANENT;
    return NativeObject::addPropertyInternal(cx, block, id,
                                              nullptr,
                                              nullptr,
                                             slot,
                                             propFlags,
                                              0,
                                             entry,
                                              false);
}

const Class BlockObject::class_ = {
    "Block",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(BlockObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS
};

template<XDRMode mode>
bool
js::XDRStaticBlockObject(XDRState<mode>* xdr, HandleObject enclosingScope,
                         MutableHandle<StaticBlockObject*> objp)
{
    

    JSContext* cx = xdr->cx();

    Rooted<StaticBlockObject*> obj(cx);
    uint32_t count = 0, offset = 0;

    if (mode == XDR_ENCODE) {
        obj = objp;
        count = obj->numVariables();
        offset = obj->localOffset();
    }

    if (mode == XDR_DECODE) {
        obj = StaticBlockObject::create(cx);
        if (!obj)
            return false;
        obj->initEnclosingNestedScope(enclosingScope);
        objp.set(obj);
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

            uint32_t propFlags;
            if (!xdr->codeUint32(&propFlags))
                return false;

            bool readonly = !!(propFlags & 1);

            bool redeclared;
            if (!StaticBlockObject::addVar(cx, obj, id, readonly, i, &redeclared)) {
                MOZ_ASSERT(!redeclared);
                return false;
            }

            bool aliased = !!(propFlags >> 1);
            obj->setAliased(i, aliased);
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
            MOZ_ASSERT(shape->hasDefaultGetter());
            MOZ_ASSERT(obj->shapeToIndex(*shape) == i);

            propid = shape->propid();
            MOZ_ASSERT(JSID_IS_ATOM(propid) || JSID_IS_INT(propid));

            atom = JSID_IS_ATOM(propid)
                   ? JSID_TO_ATOM(propid)
                   : cx->runtime()->emptyString;
            if (!XDRAtom(xdr, &atom))
                return false;

            bool aliased = obj->isAliased(i);
            bool readonly = !shape->writable();
            uint32_t propFlags = (aliased << 1) | readonly;
            if (!xdr->codeUint32(&propFlags))
                return false;
        }
    }
    return true;
}

template bool
js::XDRStaticBlockObject(XDRState<XDR_ENCODE>*, HandleObject, MutableHandle<StaticBlockObject*>);

template bool
js::XDRStaticBlockObject(XDRState<XDR_DECODE>*, HandleObject, MutableHandle<StaticBlockObject*>);

static JSObject*
CloneStaticBlockObject(JSContext* cx, HandleObject enclosingScope, Handle<StaticBlockObject*> srcBlock)
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

    for (Shape** p = shapes.begin(); p != shapes.end(); ++p) {
        RootedId id(cx, (*p)->propid());
        unsigned i = srcBlock->shapeToIndex(**p);

        bool redeclared;
        if (!StaticBlockObject::addVar(cx, clone, id, !(*p)->writable(), i, &redeclared)) {
            MOZ_ASSERT(!redeclared);
            return nullptr;
        }

        clone->setAliased(i, srcBlock->isAliased(i));
    }

    return clone;
}

JSObject*
js::CloneNestedScopeObject(JSContext* cx, HandleObject enclosingScope, Handle<NestedScopeObject*> srcBlock)
{
    if (srcBlock->is<StaticBlockObject>()) {
        Rooted<StaticBlockObject*> blockObj(cx, &srcBlock->as<StaticBlockObject>());
        return CloneStaticBlockObject(cx, enclosingScope, blockObj);
    } else {
        Rooted<StaticWithObject*> withObj(cx, &srcBlock->as<StaticWithObject>());
        return CloneStaticWithObject(cx, enclosingScope, withObj);
    }
}

 UninitializedLexicalObject*
UninitializedLexicalObject::create(JSContext* cx, HandleObject enclosing)
{
    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &class_, TaggedProto(nullptr)));
    if (!group)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &class_, TaggedProto(nullptr),
                                                      FINALIZE_KIND));
    if (!shape)
        return nullptr;

    RootedObject obj(cx, JSObject::create(cx, FINALIZE_KIND, gc::DefaultHeap, shape, group));
    if (!obj)
        return nullptr;

    obj->as<ScopeObject>().setEnclosingScope(enclosing);

    return &obj->as<UninitializedLexicalObject>();
}

static void
ReportUninitializedLexicalId(JSContext* cx, HandleId id)
{
    if (JSID_IS_ATOM(id)) {
        RootedPropertyName name(cx, JSID_TO_ATOM(id)->asPropertyName());
        ReportUninitializedLexical(cx, name);
        return;
    }
    MOZ_CRASH("UninitializedLexicalObject should only be used with property names");
}

static bool
uninitialized_LookupProperty(JSContext* cx, HandleObject obj, HandleId id,
                             MutableHandleObject objp, MutableHandleShape propp)
{
    ReportUninitializedLexicalId(cx, id);
    return false;
}

static bool
uninitialized_HasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp)
{
    ReportUninitializedLexicalId(cx, id);
    return false;
}

static bool
uninitialized_GetProperty(JSContext* cx, HandleObject obj, HandleObject receiver, HandleId id,
                          MutableHandleValue vp)
{
    ReportUninitializedLexicalId(cx, id);
    return false;
}

static bool
uninitialized_SetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                          HandleValue receiver, ObjectOpResult& result)
{
    ReportUninitializedLexicalId(cx, id);
    return false;
}

static bool
uninitialized_GetOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                                       MutableHandle<JSPropertyDescriptor> desc)
{
    ReportUninitializedLexicalId(cx, id);
    return false;
}

static bool
uninitialized_DeleteProperty(JSContext* cx, HandleObject obj, HandleId id, ObjectOpResult& result)
{
    ReportUninitializedLexicalId(cx, id);
    return false;
}

const Class UninitializedLexicalObject::class_ = {
    "UninitializedLexical",
    JSCLASS_HAS_RESERVED_SLOTS(UninitializedLexicalObject::RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    {
        uninitialized_LookupProperty,
        nullptr,             
        uninitialized_HasProperty,
        uninitialized_GetProperty,
        uninitialized_SetProperty,
        uninitialized_GetOwnPropertyDescriptor,
        uninitialized_DeleteProperty,
        nullptr, nullptr,    
        nullptr,             
        nullptr,             
        nullptr,             
    }
};





static inline JSAtom*
CallObjectLambdaName(JSFunction& fun)
{
    return fun.isNamedLambda() ? fun.atom() : nullptr;
}

ScopeIter::ScopeIter(JSContext* cx, const ScopeIter& si
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : ssi_(cx, si.ssi_),
    scope_(cx, si.scope_),
    frame_(si.frame_)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(JSContext* cx, JSObject* scope, JSObject* staticScope
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : ssi_(cx, staticScope),
    scope_(cx, scope),
    frame_(NullFramePtr())
{
    settle();
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

ScopeIter::ScopeIter(JSContext* cx, AbstractFramePtr frame, jsbytecode* pc
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : ssi_(cx, frame.script()->innermostStaticScope(pc)),
    scope_(cx, frame.scopeChain()),
    frame_(frame)
{
    assertSameCompartment(cx, frame);
    settle();
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

void
ScopeIter::incrementStaticScopeIter()
{
    ssi_++;

    
    
    
    if (!ssi_.done() && ssi_.type() == StaticScopeIter<CanGC>::NamedLambda)
        ssi_++;
}

void
ScopeIter::settle()
{
    
    
    if (frame_ && frame_.isNonEvalFunctionFrame() &&
        frame_.fun()->isHeavyweight() && !frame_.hasCallObj())
    {
        MOZ_ASSERT(ssi_.type() == StaticScopeIter<CanGC>::Function);
        incrementStaticScopeIter();
    }

    
    
    if (frame_ && (ssi_.done() || maybeStaticScope() == frame_.script()->enclosingStaticScope()))
        frame_ = NullFramePtr();

#ifdef DEBUG
    if (!ssi_.done() && hasScopeObject()) {
        switch (ssi_.type()) {
          case StaticScopeIter<CanGC>::Function:
            MOZ_ASSERT(scope_->as<CallObject>().callee().nonLazyScript() == ssi_.funScript());
            break;
          case StaticScopeIter<CanGC>::Block:
            MOZ_ASSERT(scope_->as<ClonedBlockObject>().staticBlock() == staticBlock());
            break;
          case StaticScopeIter<CanGC>::With:
            MOZ_ASSERT(scope_->as<DynamicWithObject>().staticScope() == &staticWith());
            break;
          case StaticScopeIter<CanGC>::Eval:
            MOZ_ASSERT(scope_->as<CallObject>().isForEval());
            break;
          case StaticScopeIter<CanGC>::NamedLambda:
            MOZ_CRASH("named lambda static scopes should have been skipped");
        }
    }
#endif
}

ScopeIter&
ScopeIter::operator++()
{
    if (hasScopeObject()) {
        scope_ = &scope_->as<ScopeObject>().enclosingScope();
        if (scope_->is<DeclEnvObject>())
            scope_ = &scope_->as<DeclEnvObject>().enclosingScope();
    }

    incrementStaticScopeIter();
    settle();

    return *this;
}

ScopeIter::Type
ScopeIter::type() const
{
    MOZ_ASSERT(!done());

    switch (ssi_.type()) {
      case StaticScopeIter<CanGC>::Function:
        return Call;
      case StaticScopeIter<CanGC>::Block:
        return Block;
      case StaticScopeIter<CanGC>::With:
        return With;
      case StaticScopeIter<CanGC>::Eval:
        return Eval;
      case StaticScopeIter<CanGC>::NamedLambda:
        MOZ_CRASH("named lambda static scopes should have been skipped");
      default:
        MOZ_CRASH("bad SSI type");
    }
}

ScopeObject&
ScopeIter::scope() const
{
    MOZ_ASSERT(hasScopeObject());
    return scope_->as<ScopeObject>();
}

JSObject*
ScopeIter::maybeStaticScope() const
{
    if (ssi_.done())
        return nullptr;

    switch (ssi_.type()) {
      case StaticScopeIter<CanGC>::Function:
        return &fun();
      case StaticScopeIter<CanGC>::Block:
        return &staticBlock();
      case StaticScopeIter<CanGC>::With:
        return &staticWith();
      case StaticScopeIter<CanGC>::Eval:
        return &staticEval();
      case StaticScopeIter<CanGC>::NamedLambda:
        MOZ_CRASH("named lambda static scopes should have been skipped");
      default:
        MOZ_CRASH("bad SSI type");
    }
}

 HashNumber
MissingScopeKey::hash(MissingScopeKey sk)
{
    return size_t(sk.frame_.raw()) ^ size_t(sk.staticScope_);
}

 bool
MissingScopeKey::match(MissingScopeKey sk1, MissingScopeKey sk2)
{
    return sk1.frame_ == sk2.frame_ && sk1.staticScope_ == sk2.staticScope_;
}

void
LiveScopeVal::sweep()
{
    if (staticScope_)
        MOZ_ALWAYS_FALSE(IsObjectAboutToBeFinalized(staticScope_.unsafeGet()));
}






void
LiveScopeVal::staticAsserts()
{
    static_assert(sizeof(LiveScopeVal) == sizeof(MissingScopeKey),
                  "LiveScopeVal must be same size of MissingScopeKey");
    static_assert(offsetof(LiveScopeVal, staticScope_) == offsetof(MissingScopeKey, staticScope_),
                  "LiveScopeVal.staticScope_ must alias MissingScopeKey.staticScope_");
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

    























    bool handleUnaliasedAccess(JSContext* cx, Handle<DebugScopeObject*> debugScope,
                               Handle<ScopeObject*> scope, HandleId id, Action action,
                               MutableHandleValue vp, AccessResult* accessResult) const
    {
        MOZ_ASSERT(&debugScope->scope() == scope);
        MOZ_ASSERT_IF(action == SET, !debugScope->isOptimizedOut());
        *accessResult = ACCESS_GENERIC;
        LiveScopeVal* maybeLiveScope = DebugScopes::hasLiveScope(*scope);

        
        if (scope->is<CallObject>() && !scope->as<CallObject>().isForEval()) {
            CallObject& callobj = scope->as<CallObject>();
            RootedScript script(cx, callobj.callee().getOrCreateScript(cx));
            if (!script->ensureHasTypes(cx) || !script->ensureHasAnalyzedArgsUsage(cx))
                return false;

            Bindings& bindings = script->bindings;
            BindingIter bi(script);
            while (bi && NameToId(bi->name()) != id)
                bi++;
            if (!bi)
                return true;

            if (bi->kind() == Binding::VARIABLE || bi->kind() == Binding::CONSTANT) {
                if (script->bindingIsAliased(bi))
                    return true;

                uint32_t i = bi.frameIndex();
                if (maybeLiveScope) {
                    AbstractFramePtr frame = maybeLiveScope->frame();
                    if (action == GET)
                        vp.set(frame.unaliasedLocal(i));
                    else
                        frame.unaliasedLocal(i) = vp;
                } else if (NativeObject* snapshot = debugScope->maybeSnapshot()) {
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
                MOZ_ASSERT(bi->kind() == Binding::ARGUMENT);
                unsigned i = bi.argIndex();
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
                } else if (NativeObject* snapshot = debugScope->maybeSnapshot()) {
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
            Rooted<ClonedBlockObject*> block(cx, &scope->as<ClonedBlockObject>());
            Shape* shape = block->lastProperty()->search(cx, id);
            if (!shape)
                return true;

            unsigned i = block->staticBlock().shapeToIndex(*shape);
            if (block->staticBlock().isAliased(i))
                return true;

            if (maybeLiveScope) {
                AbstractFramePtr frame = maybeLiveScope->frame();
                uint32_t local = block->staticBlock().blockIndexToLocalIndex(i);
                MOZ_ASSERT(local < frame.script()->nfixed());
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

        
        MOZ_ASSERT(scope->is<DeclEnvObject>() || scope->is<DynamicWithObject>() ||
                   scope->as<CallObject>().isForEval());
        return true;
    }

    static bool isArguments(JSContext* cx, jsid id)
    {
        return id == NameToId(cx->names().arguments);
    }

    static bool isFunctionScope(ScopeObject& scope)
    {
        return scope.is<CallObject>() && !scope.as<CallObject>().isForEval();
    }

    





    static bool isMissingArgumentsBinding(ScopeObject& scope)
    {
        return isFunctionScope(scope) &&
               !scope.as<CallObject>().callee().nonLazyScript()->argumentsHasVarBinding();
    }

    





    static bool isMissingArguments(JSContext* cx, jsid id, ScopeObject& scope)
    {
        return isArguments(cx, id) && isFunctionScope(scope) &&
               !scope.as<CallObject>().callee().nonLazyScript()->needsArgsObj();
    }

    












    static bool isMagicMissingArgumentsValue(JSContext* cx, ScopeObject& scope, HandleValue v)
    {
        bool isMagic = v.isMagic() && v.whyMagic() == JS_OPTIMIZED_ARGUMENTS;
        MOZ_ASSERT_IF(isMagic,
                      isFunctionScope(scope) &&
                      scope.as<CallObject>().callee().nonLazyScript()->argumentsHasVarBinding());
        return isMagic;
    }

    



    static bool createMissingArguments(JSContext* cx, ScopeObject& scope,
                                       MutableHandleArgumentsObject argsObj)
    {
        argsObj.set(nullptr);

        LiveScopeVal* maybeScope = DebugScopes::hasLiveScope(scope);
        if (!maybeScope)
            return true;

        argsObj.set(ArgumentsObject::createUnexpected(cx, maybeScope->frame()));
        return !!argsObj;
    }

  public:
    static const char family;
    static const DebugScopeProxy singleton;

    MOZ_CONSTEXPR DebugScopeProxy() : BaseProxyHandler(&family) {}

    bool preventExtensions(JSContext* cx, HandleObject proxy,
                           ObjectOpResult& result) const override
    {
        
        
        return result.fail(JSMSG_CANT_CHANGE_EXTENSIBILITY);
    }

    bool isExtensible(JSContext* cx, HandleObject proxy, bool* extensible) const override
    {
        
        *extensible = true;
        return true;
    }

    bool getPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                               MutableHandle<PropertyDescriptor> desc) const override
    {
        return getOwnPropertyDescriptor(cx, proxy, id, desc);
    }

    bool getMissingArgumentsPropertyDescriptor(JSContext* cx,
                                               Handle<DebugScopeObject*> debugScope,
                                               ScopeObject& scope,
                                               MutableHandle<PropertyDescriptor> desc) const
    {
        RootedArgumentsObject argsObj(cx);
        if (!createMissingArguments(cx, scope, &argsObj))
            return false;

        if (!argsObj) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_LIVE,
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

    bool getOwnPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                  MutableHandle<PropertyDescriptor> desc) const override
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &debugScope->scope());

        if (isMissingArguments(cx, id, *scope))
            return getMissingArgumentsPropertyDescriptor(cx, debugScope, *scope, desc);

        RootedValue v(cx);
        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, GET, &v, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            if (isMagicMissingArgumentsValue(cx, *scope, v))
                return getMissingArgumentsPropertyDescriptor(cx, debugScope, *scope, desc);
            desc.object().set(debugScope);
            desc.setAttributes(JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT);
            desc.value().set(v);
            desc.setGetter(nullptr);
            desc.setSetter(nullptr);
            return true;
          case ACCESS_GENERIC:
            return JS_GetOwnPropertyDescriptorById(cx, scope, id, desc);
          case ACCESS_LOST:
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEBUG_OPTIMIZED_OUT);
            return false;
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool getMissingArguments(JSContext* cx, ScopeObject& scope, MutableHandleValue vp) const
    {
        RootedArgumentsObject argsObj(cx);
        if (!createMissingArguments(cx, scope, &argsObj))
            return false;

        if (!argsObj) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_LIVE,
                                 "Debugger scope");
            return false;
        }

        vp.setObject(*argsObj);
        return true;
    }

    bool get(JSContext* cx, HandleObject proxy, HandleObject receiver, HandleId id,
             MutableHandleValue vp) const override
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        if (isMissingArguments(cx, id, *scope))
            return getMissingArguments(cx, *scope, vp);

        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, GET, vp, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            if (isMagicMissingArgumentsValue(cx, *scope, vp))
                return getMissingArguments(cx, *scope, vp);
            return true;
          case ACCESS_GENERIC:
            return GetProperty(cx, scope, scope, id, vp);
          case ACCESS_LOST:
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEBUG_OPTIMIZED_OUT);
            return false;
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool getMissingArgumentsMaybeSentinelValue(JSContext* cx, ScopeObject& scope,
                                               MutableHandleValue vp) const
    {
        RootedArgumentsObject argsObj(cx);
        if (!createMissingArguments(cx, scope, &argsObj))
            return false;
        vp.set(argsObj ? ObjectValue(*argsObj) : MagicValue(JS_OPTIMIZED_ARGUMENTS));
        return true;
    }

    



    bool getMaybeSentinelValue(JSContext* cx, Handle<DebugScopeObject*> debugScope, HandleId id,
                               MutableHandleValue vp) const
    {
        Rooted<ScopeObject*> scope(cx, &debugScope->scope());

        if (isMissingArguments(cx, id, *scope))
            return getMissingArgumentsMaybeSentinelValue(cx, *scope, vp);

        AccessResult access;
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, GET, vp, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            if (isMagicMissingArgumentsValue(cx, *scope, vp))
                return getMissingArgumentsMaybeSentinelValue(cx, *scope, vp);
            return true;
          case ACCESS_GENERIC:
            return GetProperty(cx, scope, scope, id, vp);
          case ACCESS_LOST:
            vp.setMagic(JS_OPTIMIZED_OUT);
            return true;
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool set(JSContext* cx, HandleObject proxy, HandleId id, HandleValue v, HandleValue receiver,
             ObjectOpResult& result) const override
    {
        Rooted<DebugScopeObject*> debugScope(cx, &proxy->as<DebugScopeObject>());
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        if (debugScope->isOptimizedOut())
            return Throw(cx, id, JSMSG_DEBUG_CANT_SET_OPT_ENV);

        AccessResult access;
        RootedValue valCopy(cx, v);
        if (!handleUnaliasedAccess(cx, debugScope, scope, id, SET, &valCopy, &access))
            return false;

        switch (access) {
          case ACCESS_UNALIASED:
            return result.succeed();
          case ACCESS_GENERIC:
            {
                RootedValue scopeVal(cx, ObjectValue(*scope));
                return SetProperty(cx, scope, id, v, scopeVal, result);
            }
          default:
            MOZ_CRASH("bad AccessResult");
        }
    }

    bool defineProperty(JSContext* cx, HandleObject proxy, HandleId id,
                        Handle<PropertyDescriptor> desc,
                        ObjectOpResult& result) const override
    {
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        bool found;
        if (!has(cx, proxy, id, &found))
            return false;
        if (found)
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);

        return JS_DefinePropertyById(cx, scope, id, desc, result);
    }

    bool ownPropertyKeys(JSContext* cx, HandleObject proxy, AutoIdVector& props) const override
    {
        Rooted<ScopeObject*> scope(cx, &proxy->as<DebugScopeObject>().scope());

        if (isMissingArgumentsBinding(*scope)) {
            if (!props.append(NameToId(cx->names().arguments)))
                return false;
        }

        
        
        
        
        
        
        Rooted<JSObject*> target(cx, (scope->is<DynamicWithObject>()
                                      ? &scope->as<DynamicWithObject>().object() : scope));
        if (!GetPropertyKeys(cx, target, JSITER_OWNONLY, &props))
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

    bool enumerate(JSContext* cx, HandleObject proxy, MutableHandleObject objp) const override
    {
        return BaseProxyHandler::enumerate(cx, proxy, objp);
    }

    bool has(JSContext* cx, HandleObject proxy, HandleId id_, bool* bp) const override
    {
        RootedId id(cx, id_);
        ScopeObject& scopeObj = proxy->as<DebugScopeObject>().scope();

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

    bool delete_(JSContext* cx, HandleObject proxy, HandleId id,
                 ObjectOpResult& result) const override
    {
        return result.fail(JSMSG_CANT_DELETE);
    }
};

} 

const char DebugScopeProxy::family = 0;
const DebugScopeProxy DebugScopeProxy::singleton;

 DebugScopeObject*
DebugScopeObject::create(JSContext* cx, ScopeObject& scope, HandleObject enclosing)
{
    MOZ_ASSERT(scope.compartment() == cx->compartment());
    MOZ_ASSERT(!IsSyntacticScope(enclosing));

    RootedValue priv(cx, ObjectValue(scope));
    JSObject* obj = NewProxyObject(cx, &DebugScopeProxy::singleton, priv,
                                   nullptr );
    if (!obj)
        return nullptr;

    DebugScopeObject* debugScope = &obj->as<DebugScopeObject>();
    debugScope->setExtra(ENCLOSING_EXTRA, ObjectValue(*enclosing));
    debugScope->setExtra(SNAPSHOT_EXTRA, NullValue());

    return debugScope;
}

ScopeObject&
DebugScopeObject::scope() const
{
    return target()->as<ScopeObject>();
}

JSObject&
DebugScopeObject::enclosingScope() const
{
    return extra(ENCLOSING_EXTRA).toObject();
}

ArrayObject*
DebugScopeObject::maybeSnapshot() const
{
    MOZ_ASSERT(!scope().as<CallObject>().isForEval());
    JSObject* obj = extra(SNAPSHOT_EXTRA).toObjectOrNull();
    return obj ? &obj->as<ArrayObject>() : nullptr;
}

void
DebugScopeObject::initSnapshot(ArrayObject& o)
{
    MOZ_ASSERT(maybeSnapshot() == nullptr);
    setExtra(SNAPSHOT_EXTRA, ObjectValue(o));
}

bool
DebugScopeObject::isForDeclarative() const
{
    ScopeObject& s = scope();
    return s.is<CallObject>() || s.is<BlockObject>() || s.is<DeclEnvObject>();
}

bool
DebugScopeObject::getMaybeSentinelValue(JSContext* cx, HandleId id, MutableHandleValue vp)
{
    Rooted<DebugScopeObject*> self(cx, this);
    return DebugScopeProxy::singleton.getMaybeSentinelValue(cx, self, id, vp);
}

bool
DebugScopeObject::isOptimizedOut() const
{
    ScopeObject& s = scope();

    if (DebugScopes::hasLiveScope(s))
        return false;

    if (s.is<ClonedBlockObject>())
        return !s.as<ClonedBlockObject>().staticBlock().needsClone();

    if (s.is<CallObject>()) {
        return !s.as<CallObject>().isForEval() &&
               !s.as<CallObject>().callee().isHeavyweight() &&
               !maybeSnapshot();
    }

    return false;
}

bool
js::IsDebugScopeSlow(ProxyObject* proxy)
{
    MOZ_ASSERT(proxy->hasClass(&ProxyObject::class_));
    return proxy->handler() == &DebugScopeProxy::singleton;
}



 MOZ_ALWAYS_INLINE void
DebugScopes::liveScopesPostWriteBarrier(JSRuntime* rt, LiveScopeMap* map, ScopeObject* key)
{
    
    
    typedef HashMap<ScopeObject*,
                    MissingScopeKey,
                    DefaultHasher<ScopeObject*>,
                    RuntimeAllocPolicy> UnbarrieredLiveScopeMap;
    typedef gc::HashKeyRef<UnbarrieredLiveScopeMap, ScopeObject*> Ref;
    if (key && IsInsideNursery(key))
        rt->gc.storeBuffer.putGeneric(Ref(reinterpret_cast<UnbarrieredLiveScopeMap*>(map), key));
}

DebugScopes::DebugScopes(JSContext* cx)
 : proxiedScopes(cx),
   missingScopes(cx->runtime()),
   liveScopes(cx->runtime())
{}

DebugScopes::~DebugScopes()
{
    MOZ_ASSERT(missingScopes.empty());
}

bool
DebugScopes::init()
{
    return liveScopes.init() && missingScopes.init();
}

void
DebugScopes::mark(JSTracer* trc)
{
    proxiedScopes.trace(trc);
}

void
DebugScopes::sweep(JSRuntime* rt)
{
    



    for (MissingScopeMap::Enum e(missingScopes); !e.empty(); e.popFront()) {
        DebugScopeObject** debugScope = e.front().value().unsafeGet();
        if (IsObjectAboutToBeFinalized(debugScope)) {
            
















            liveScopes.remove(&(*debugScope)->scope());
            e.removeFront();
        } else {
            MissingScopeKey key = e.front().key();
            if (IsForwarded(key.staticScope())) {
                key.updateStaticScope(Forwarded(key.staticScope()));
                e.rekeyFront(key);
            }
        }
    }

    for (LiveScopeMap::Enum e(liveScopes); !e.empty(); e.popFront()) {
        ScopeObject* scope = e.front().key();

        e.front().value().sweep();

        



        if (IsObjectAboutToBeFinalized(&scope))
            e.removeFront();
        else if (scope != e.front().key())
            e.rekeyFront(scope);
    }
}

#ifdef JSGC_HASH_TABLE_CHECKS
void
DebugScopes::checkHashTablesAfterMovingGC(JSRuntime* runtime)
{
    




    proxiedScopes.checkAfterMovingGC();
    for (MissingScopeMap::Range r = missingScopes.all(); !r.empty(); r.popFront()) {
        CheckGCThingAfterMovingGC(r.front().key().staticScope());
        CheckGCThingAfterMovingGC(r.front().value().get());
    }
    for (LiveScopeMap::Range r = liveScopes.all(); !r.empty(); r.popFront()) {
        CheckGCThingAfterMovingGC(r.front().key());
        CheckGCThingAfterMovingGC(r.front().value().staticScope_.get());
    }
}
#endif








static bool
CanUseDebugScopeMaps(JSContext* cx)
{
    return cx->compartment()->isDebuggee();
}

DebugScopes*
DebugScopes::ensureCompartmentData(JSContext* cx)
{
    JSCompartment* c = cx->compartment();
    if (c->debugScopes)
        return c->debugScopes;

    c->debugScopes = cx->runtime()->new_<DebugScopes>(cx);
    if (c->debugScopes && c->debugScopes->init())
        return c->debugScopes;

    if (c->debugScopes)
        js_delete<DebugScopes>(c->debugScopes);
    c->debugScopes = nullptr;
    ReportOutOfMemory(cx);
    return nullptr;
}

DebugScopeObject*
DebugScopes::hasDebugScope(JSContext* cx, ScopeObject& scope)
{
    DebugScopes* scopes = scope.compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (JSObject* obj = scopes->proxiedScopes.lookup(&scope)) {
        MOZ_ASSERT(CanUseDebugScopeMaps(cx));
        return &obj->as<DebugScopeObject>();
    }

    return nullptr;
}

bool
DebugScopes::addDebugScope(JSContext* cx, ScopeObject& scope, DebugScopeObject& debugScope)
{
    MOZ_ASSERT(cx->compartment() == scope.compartment());
    MOZ_ASSERT(cx->compartment() == debugScope.compartment());

    if (!CanUseDebugScopeMaps(cx))
        return true;

    DebugScopes* scopes = ensureCompartmentData(cx);
    if (!scopes)
        return false;

    return scopes->proxiedScopes.add(cx, &scope, &debugScope);
}

DebugScopeObject*
DebugScopes::hasDebugScope(JSContext* cx, const ScopeIter& si)
{
    MOZ_ASSERT(!si.hasScopeObject());

    DebugScopes* scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(MissingScopeKey(si))) {
        MOZ_ASSERT(CanUseDebugScopeMaps(cx));
        return p->value();
    }
    return nullptr;
}

bool
DebugScopes::addDebugScope(JSContext* cx, const ScopeIter& si, DebugScopeObject& debugScope)
{
    MOZ_ASSERT(!si.hasScopeObject());
    MOZ_ASSERT(cx->compartment() == debugScope.compartment());
    MOZ_ASSERT_IF(si.withinInitialFrame() && si.initialFrame().isFunctionFrame(),
                  !si.initialFrame().callee()->isGenerator());
    
    MOZ_ASSERT_IF(si.type() == ScopeIter::Call, !si.fun().isGenerator());

    if (!CanUseDebugScopeMaps(cx))
        return true;

    DebugScopes* scopes = ensureCompartmentData(cx);
    if (!scopes)
        return false;

    MissingScopeKey key(si);
    MOZ_ASSERT(!scopes->missingScopes.has(key));
    if (!scopes->missingScopes.put(key, ReadBarriered<DebugScopeObject*>(&debugScope))) {
        ReportOutOfMemory(cx);
        return false;
    }

    
    
    if (si.withinInitialFrame()) {
        MOZ_ASSERT(!scopes->liveScopes.has(&debugScope.scope()));
        if (!scopes->liveScopes.put(&debugScope.scope(), LiveScopeVal(si))) {
            ReportOutOfMemory(cx);
            return false;
        }
        liveScopesPostWriteBarrier(cx->runtime(), &scopes->liveScopes, &debugScope.scope());
    }

    return true;
}

void
DebugScopes::onPopCall(AbstractFramePtr frame, JSContext* cx)
{
    assertSameCompartment(cx, frame);

    DebugScopes* scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    Rooted<DebugScopeObject*> debugScope(cx, nullptr);

    if (frame.fun()->isHeavyweight()) {
        



        if (!frame.hasCallObj())
            return;

        if (frame.fun()->isGenerator())
            return;

        CallObject& callobj = frame.scopeChain()->as<CallObject>();
        scopes->liveScopes.remove(&callobj);
        if (JSObject* obj = scopes->proxiedScopes.lookup(&callobj))
            debugScope = &obj->as<DebugScopeObject>();
    } else {
        ScopeIter si(cx, frame, frame.script()->main());
        if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(MissingScopeKey(si))) {
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

        



        RootedArrayObject snapshot(cx, NewDenseCopiedArray(cx, vec.length(), vec.begin()));
        if (!snapshot) {
            cx->clearPendingException();
            return;
        }

        debugScope->initSnapshot(*snapshot);
    }
}

void
DebugScopes::onPopBlock(JSContext* cx, AbstractFramePtr frame, jsbytecode* pc)
{
    assertSameCompartment(cx, frame);

    DebugScopes* scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    ScopeIter si(cx, frame, pc);
    onPopBlock(cx, si);
}

void
DebugScopes::onPopBlock(JSContext* cx, const ScopeIter& si)
{
    DebugScopes* scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    MOZ_ASSERT(si.withinInitialFrame());
    MOZ_ASSERT(si.type() == ScopeIter::Block);

    if (si.staticBlock().needsClone()) {
        ClonedBlockObject& clone = si.scope().as<ClonedBlockObject>();
        clone.copyUnaliasedValues(si.initialFrame());
        scopes->liveScopes.remove(&clone);
    } else {
        if (MissingScopeMap::Ptr p = scopes->missingScopes.lookup(MissingScopeKey(si))) {
            ClonedBlockObject& clone = p->value()->scope().as<ClonedBlockObject>();
            clone.copyUnaliasedValues(si.initialFrame());
            scopes->liveScopes.remove(&clone);
            scopes->missingScopes.remove(p);
        }
    }
}

void
DebugScopes::onPopWith(AbstractFramePtr frame)
{
    DebugScopes* scopes = frame.compartment()->debugScopes;
    if (scopes)
        scopes->liveScopes.remove(&frame.scopeChain()->as<DynamicWithObject>());
}

void
DebugScopes::onPopStrictEvalScope(AbstractFramePtr frame)
{
    DebugScopes* scopes = frame.compartment()->debugScopes;
    if (!scopes)
        return;

    



    if (frame.hasCallObj())
        scopes->liveScopes.remove(&frame.scopeChain()->as<CallObject>());
}

void
DebugScopes::onCompartmentUnsetIsDebuggee(JSCompartment* c)
{
    DebugScopes* scopes = c->debugScopes;
    if (scopes) {
        scopes->proxiedScopes.clear();
        scopes->missingScopes.clear();
        scopes->liveScopes.clear();
    }
}

bool
DebugScopes::updateLiveScopes(JSContext* cx)
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

        if (!frame.isDebuggee())
            continue;

        for (ScopeIter si(cx, frame, i.pc()); si.withinInitialFrame(); ++si) {
            if (si.hasScopeObject()) {
                MOZ_ASSERT(si.scope().compartment() == cx->compartment());
                DebugScopes* scopes = ensureCompartmentData(cx);
                if (!scopes)
                    return false;
                if (!scopes->liveScopes.put(&si.scope(), LiveScopeVal(si)))
                    return false;
                liveScopesPostWriteBarrier(cx->runtime(), &scopes->liveScopes, &si.scope());
            }
        }

        if (frame.prevUpToDate())
            return true;
        MOZ_ASSERT(frame.scopeChain()->compartment()->isDebuggee());
        frame.setPrevUpToDate();
    }

    return true;
}

LiveScopeVal*
DebugScopes::hasLiveScope(ScopeObject& scope)
{
    DebugScopes* scopes = scope.compartment()->debugScopes;
    if (!scopes)
        return nullptr;

    if (LiveScopeMap::Ptr p = scopes->liveScopes.lookup(&scope))
        return &p->value();

    return nullptr;
}

 void
DebugScopes::unsetPrevUpToDateUntil(JSContext* cx, AbstractFramePtr until)
{
    
    
    
    
    
    
    
    for (AllFramesIter i(cx); !i.done(); ++i) {
        if (!i.hasUsableAbstractFramePtr())
            continue;

        AbstractFramePtr frame = i.abstractFramePtr();
        if (frame == until)
            return;

        if (frame.scopeChain()->compartment() != cx->compartment())
            continue;

        frame.unsetPrevUpToDate();
    }
}

 void
DebugScopes::forwardLiveFrame(JSContext* cx, AbstractFramePtr from, AbstractFramePtr to)
{
    DebugScopes* scopes = cx->compartment()->debugScopes;
    if (!scopes)
        return;

    for (MissingScopeMap::Enum e(scopes->missingScopes); !e.empty(); e.popFront()) {
        MissingScopeKey key = e.front().key();
        if (key.frame() == from) {
            key.updateFrame(to);
            e.rekeyFront(key);
        }
    }

    for (LiveScopeMap::Enum e(scopes->liveScopes); !e.empty(); e.popFront()) {
        LiveScopeVal& val = e.front().value();
        if (val.frame() == from)
            val.updateFrame(to);
    }
}



static JSObject*
GetDebugScope(JSContext* cx, const ScopeIter& si);

static DebugScopeObject*
GetDebugScopeForScope(JSContext* cx, const ScopeIter& si)
{
    Rooted<ScopeObject*> scope(cx, &si.scope());
    if (DebugScopeObject* debugScope = DebugScopes::hasDebugScope(cx, *scope))
        return debugScope;

    ScopeIter copy(cx, si);
    RootedObject enclosingDebug(cx, GetDebugScope(cx, ++copy));
    if (!enclosingDebug)
        return nullptr;

    JSObject& maybeDecl = scope->enclosingScope();
    if (maybeDecl.is<DeclEnvObject>()) {
        MOZ_ASSERT(CallObjectLambdaName(scope->as<CallObject>().callee()));
        enclosingDebug = DebugScopeObject::create(cx, maybeDecl.as<DeclEnvObject>(), enclosingDebug);
        if (!enclosingDebug)
            return nullptr;
    }

    DebugScopeObject* debugScope = DebugScopeObject::create(cx, *scope, enclosingDebug);
    if (!debugScope)
        return nullptr;

    if (!DebugScopes::addDebugScope(cx, *scope, *debugScope))
        return nullptr;

    return debugScope;
}

static DebugScopeObject*
GetDebugScopeForMissing(JSContext* cx, const ScopeIter& si)
{
    MOZ_ASSERT(!si.hasScopeObject() && si.canHaveScopeObject());

    if (DebugScopeObject* debugScope = DebugScopes::hasDebugScope(cx, si))
        return debugScope;

    ScopeIter copy(cx, si);
    RootedObject enclosingDebug(cx, GetDebugScope(cx, ++copy));
    if (!enclosingDebug)
        return nullptr;

    










    DebugScopeObject* debugScope = nullptr;
    switch (si.type()) {
      case ScopeIter::Call: {
        RootedFunction callee(cx, &si.fun());
        
        MOZ_ASSERT(!callee->isGenerator());

        Rooted<CallObject*> callobj(cx);
        if (si.withinInitialFrame())
            callobj = CallObject::createForFunction(cx, si.initialFrame());
        else
            callobj = CallObject::createHollowForDebug(cx, callee);
        if (!callobj)
            return nullptr;

        if (callobj->enclosingScope().is<DeclEnvObject>()) {
            MOZ_ASSERT(CallObjectLambdaName(callobj->callee()));
            DeclEnvObject& declenv = callobj->enclosingScope().as<DeclEnvObject>();
            enclosingDebug = DebugScopeObject::create(cx, declenv, enclosingDebug);
            if (!enclosingDebug)
                return nullptr;
        }

        debugScope = DebugScopeObject::create(cx, *callobj, enclosingDebug);
        break;
      }
      case ScopeIter::Block: {
        
        MOZ_ASSERT_IF(si.withinInitialFrame() && si.initialFrame().isFunctionFrame(),
                      !si.initialFrame().callee()->isGenerator());

        Rooted<StaticBlockObject*> staticBlock(cx, &si.staticBlock());
        ClonedBlockObject* block;
        if (si.withinInitialFrame())
            block = ClonedBlockObject::create(cx, staticBlock, si.initialFrame());
        else
            block = ClonedBlockObject::createHollowForDebug(cx, staticBlock);
        if (!block)
            return nullptr;

        debugScope = DebugScopeObject::create(cx, *block, enclosingDebug);
        break;
      }
      case ScopeIter::With:
      case ScopeIter::Eval:
        MOZ_CRASH("should already have a scope");
    }
    if (!debugScope)
        return nullptr;

    if (!DebugScopes::addDebugScope(cx, si, *debugScope))
        return nullptr;

    return debugScope;
}

static JSObject*
GetDebugScopeForNonScopeObject(const ScopeIter& si)
{
    JSObject& enclosing = si.enclosingScope();
    MOZ_ASSERT(!IsSyntacticScope(&enclosing));
#ifdef DEBUG
    JSObject* o = &enclosing;
    while ((o = o->enclosingScope()))
        MOZ_ASSERT(!IsSyntacticScope(o));
#endif
    return &enclosing;
}

static JSObject*
GetDebugScope(JSContext* cx, const ScopeIter& si)
{
    JS_CHECK_RECURSION(cx, return nullptr);

    if (si.done())
        return GetDebugScopeForNonScopeObject(si);

    if (si.hasScopeObject())
        return GetDebugScopeForScope(cx, si);

    if (si.canHaveScopeObject())
        return GetDebugScopeForMissing(cx, si);

    ScopeIter copy(cx, si);
    return GetDebugScope(cx, ++copy);
}

JSObject*
js::GetDebugScopeForFunction(JSContext* cx, HandleFunction fun)
{
    assertSameCompartment(cx, fun);
    MOZ_ASSERT(CanUseDebugScopeMaps(cx));
    if (!DebugScopes::updateLiveScopes(cx))
        return nullptr;
    JSScript* script = fun->getOrCreateScript(cx);
    if (!script)
        return nullptr;
    ScopeIter si(cx, fun->environment(), script->enclosingStaticScope());
    return GetDebugScope(cx, si);
}

JSObject*
js::GetDebugScopeForFrame(JSContext* cx, AbstractFramePtr frame, jsbytecode* pc)
{
    assertSameCompartment(cx, frame);
    if (CanUseDebugScopeMaps(cx) && !DebugScopes::updateLiveScopes(cx))
        return nullptr;
    ScopeIter si(cx, frame, pc);
    return GetDebugScope(cx, si);
}


JS_FRIEND_API(JSObject*)
js::GetObjectEnvironmentObjectForFunction(JSFunction* fun)
{
    if (!fun->isInterpreted())
        return &fun->global();

    JSObject* env = fun->environment();
    if (!env || !env->is<DynamicWithObject>())
        return &fun->global();

    return &env->as<DynamicWithObject>().object();
}

bool
js::CreateScopeObjectsForScopeChain(JSContext* cx, AutoObjectVector& scopeChain,
                                    HandleObject dynamicTerminatingScope,
                                    MutableHandleObject dynamicScopeObj,
                                    MutableHandleObject staticScopeObj)
{
#ifdef DEBUG
    for (size_t i = 0; i < scopeChain.length(); ++i) {
        assertSameCompartment(cx, scopeChain[i]);
        MOZ_ASSERT(!scopeChain[i]->is<GlobalObject>());
    }
#endif

    
    
    Rooted<StaticWithObject*> staticWith(cx);
    RootedObject staticEnclosingScope(cx);
    Rooted<DynamicWithObject*> dynamicWith(cx);
    RootedObject dynamicEnclosingScope(cx, dynamicTerminatingScope);
    for (size_t i = scopeChain.length(); i > 0; ) {
        staticWith = StaticWithObject::create(cx);
        if (!staticWith)
            return false;
        staticWith->initEnclosingNestedScope(staticEnclosingScope);
        staticEnclosingScope = staticWith;

        dynamicWith = DynamicWithObject::create(cx, scopeChain[--i], dynamicEnclosingScope,
                                                staticWith, DynamicWithObject::NonSyntacticWith);
        if (!dynamicWith)
            return false;
        dynamicEnclosingScope = dynamicWith;
    }

    dynamicScopeObj.set(dynamicEnclosingScope);
    staticScopeObj.set(staticEnclosingScope);
    return true;
}

#ifdef DEBUG

typedef HashSet<PropertyName*> PropertyNameSet;

static bool
RemoveReferencedNames(JSContext* cx, HandleScript script, PropertyNameSet& remainingNames)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    for (jsbytecode* pc = script->code(); pc != script->codeEnd(); pc += GetBytecodeLength(pc)) {
        PropertyName* name;

        switch (JSOp(*pc)) {
          case JSOP_GETNAME:
          case JSOP_SETNAME:
          case JSOP_STRICTSETNAME:
            name = script->getName(pc);
            break;

          case JSOP_GETGNAME:
          case JSOP_SETGNAME:
          case JSOP_STRICTSETGNAME:
            if (script->hasPollutedGlobalScope())
                name = script->getName(pc);
            else
                name = nullptr;
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
        ObjectArray* objects = script->objects();
        for (size_t i = 0; i < objects->length; i++) {
            JSObject* obj = objects->vector[i];
            if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted()) {
                JSFunction* fun = &obj->as<JSFunction>();
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
AnalyzeEntrainedVariablesInScript(JSContext* cx, HandleScript script, HandleScript innerScript)
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

        if (JSAtom* name = script->functionNonDelazifying()->displayAtom()) {
            buf.putString(name);
            buf.printf(" ");
        }

        buf.printf("(%s:%" PRIuSIZE ") has variables entrained by ", script->filename(), script->lineno());

        if (JSAtom* name = innerScript->functionNonDelazifying()->displayAtom()) {
            buf.putString(name);
            buf.printf(" ");
        }

        buf.printf("(%s:%" PRIuSIZE ") ::", innerScript->filename(), innerScript->lineno());

        for (PropertyNameSet::Range r = remainingNames.all(); !r.empty(); r.popFront()) {
            buf.printf(" ");
            buf.putString(r.front());
        }

        printf("%s\n", buf.string());
    }

    if (innerScript->hasObjects()) {
        ObjectArray* objects = innerScript->objects();
        for (size_t i = 0; i < objects->length; i++) {
            JSObject* obj = objects->vector[i];
            if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted()) {
                JSFunction* fun = &obj->as<JSFunction>();
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
js::AnalyzeEntrainedVariables(JSContext* cx, HandleScript script)
{
    if (!script->hasObjects())
        return true;

    ObjectArray* objects = script->objects();
    for (size_t i = 0; i < objects->length; i++) {
        JSObject* obj = objects->vector[i];
        if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpreted()) {
            JSFunction* fun = &obj->as<JSFunction>();
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
