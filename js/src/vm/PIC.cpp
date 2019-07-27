





#include "vm/PIC.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsobj.h"
#include "gc/Marking.h"

#include "vm/GlobalObject.h"
#include "vm/SelfHosting.h"

#include "jsobjinlines.h"
#include "vm/NativeObject-inl.h"

using namespace js;
using namespace js::gc;

bool
js::ForOfPIC::Chain::initialize(JSContext* cx)
{
    MOZ_ASSERT(!initialized_);

    
    RootedNativeObject arrayProto(cx, GlobalObject::getOrCreateArrayPrototype(cx, cx->global()));
    if (!arrayProto)
        return false;

    
    RootedNativeObject arrayIteratorProto(cx,
        GlobalObject::getOrCreateArrayIteratorPrototype(cx, cx->global()));
    if (!arrayIteratorProto)
        return false;

    
    
    initialized_ = true;
    arrayProto_ = arrayProto;
    arrayIteratorProto_ = arrayIteratorProto;

    
    
    disabled_ = true;

    
    Shape* iterShape = arrayProto->lookup(cx, SYMBOL_TO_JSID(cx->wellKnownSymbols().iterator));
    if (!iterShape || !iterShape->hasSlot() || !iterShape->hasDefaultGetter())
        return true;

    
    Value iterator = arrayProto->getSlot(iterShape->slot());
    JSFunction* iterFun;
    if (!IsFunctionObject(iterator, &iterFun))
        return true;
    if (!IsSelfHostedFunctionWithName(iterFun, cx->names().ArrayValues))
        return true;

    
    Shape* nextShape = arrayIteratorProto->lookup(cx, cx->names().next);
    if (!nextShape || !nextShape->hasSlot())
        return true;

    
    Value next = arrayIteratorProto->getSlot(nextShape->slot());
    JSFunction* nextFun;
    if (!IsFunctionObject(next, &nextFun))
        return true;
    if (!IsSelfHostedFunctionWithName(nextFun, cx->names().ArrayIteratorNext))
        return true;

    disabled_ = false;
    arrayProtoShape_ = arrayProto->lastProperty();
    arrayProtoIteratorSlot_ = iterShape->slot();
    canonicalIteratorFunc_ = iterator;
    arrayIteratorProtoShape_ = arrayIteratorProto->lastProperty();
    arrayIteratorProtoNextSlot_ = nextShape->slot();
    canonicalNextFunc_ = next;
    return true;
}

js::ForOfPIC::Stub*
js::ForOfPIC::Chain::isArrayOptimized(ArrayObject* obj)
{
    Stub* stub = getMatchingStub(obj);
    if (!stub)
        return nullptr;

    
    if (!isOptimizableArray(obj))
        return nullptr;

    
    if (!isArrayStateStillSane())
        return nullptr;

    return stub;
}

bool
js::ForOfPIC::Chain::tryOptimizeArray(JSContext* cx, HandleArrayObject array, bool* optimized)
{
    MOZ_ASSERT(optimized);

    *optimized = false;

    if (!initialized_) {
        
        if (!initialize(cx))
            return false;

    } else if (!disabled_ && !isArrayStateStillSane()) {
        
        reset(cx);

        if (!initialize(cx))
            return false;
    }
    MOZ_ASSERT(initialized_);

    
    if (disabled_)
        return true;

    
    MOZ_ASSERT(isArrayStateStillSane());

    
    ForOfPIC::Stub* stub = isArrayOptimized(&array->as<ArrayObject>());
    if (stub) {
        *optimized = true;
        return true;
    }

    
    
    
    if (numStubs() >= MAX_STUBS)
        eraseChain();

    
    if (!isOptimizableArray(array))
        return true;

    
    if (array->lookup(cx, SYMBOL_TO_JSID(cx->wellKnownSymbols().iterator)))
        return true;

    
    RootedShape shape(cx, array->lastProperty());
    stub = cx->new_<Stub>(shape);
    if (!stub)
        return false;

    
    addStub(stub);

    *optimized = true;
    return true;
}

js::ForOfPIC::Stub*
js::ForOfPIC::Chain::getMatchingStub(JSObject* obj)
{
    
    if (!initialized_ || disabled_)
        return nullptr;

    
    for (Stub* stub = stubs(); stub != nullptr; stub = stub->next()) {
        if (stub->shape() == obj->maybeShape())
            return stub;
    }

    return nullptr;
}

bool
js::ForOfPIC::Chain::isOptimizableArray(JSObject* obj)
{
    MOZ_ASSERT(obj->is<ArrayObject>());

    
    if (!obj->getTaggedProto().isObject())
        return false;
    if (obj->getTaggedProto().toObject() != arrayProto_)
        return false;

    return true;
}

bool
js::ForOfPIC::Chain::isArrayStateStillSane()
{
    
    if (arrayProto_->lastProperty() != arrayProtoShape_)
        return false;

    
    
    if (arrayProto_->getSlot(arrayProtoIteratorSlot_) != canonicalIteratorFunc_)
        return false;

    
    return isArrayNextStillSane();
}

void
js::ForOfPIC::Chain::reset(JSContext* cx)
{
    
    MOZ_ASSERT(!disabled_);

    
    eraseChain();

    arrayProto_ = nullptr;
    arrayIteratorProto_ = nullptr;

    arrayProtoShape_ = nullptr;
    arrayProtoIteratorSlot_ = -1;
    canonicalIteratorFunc_ = UndefinedValue();

    arrayIteratorProtoShape_ = nullptr;
    arrayIteratorProtoNextSlot_ = -1;
    canonicalNextFunc_ = UndefinedValue();

    initialized_ = false;
}

void
js::ForOfPIC::Chain::eraseChain()
{
    
    MOZ_ASSERT(!disabled_);

    
    Stub* stub = stubs_;
    while (stub) {
        Stub* next = stub->next();
        js_delete(stub);
        stub = next;
    }
    stubs_ = nullptr;
}



void
js::ForOfPIC::Chain::mark(JSTracer* trc)
{
    if (!initialized_ || disabled_)
        return;

    TraceEdge(trc, &arrayProto_, "ForOfPIC Array.prototype.");
    TraceEdge(trc, &arrayIteratorProto_, "ForOfPIC ArrayIterator.prototype.");

    TraceEdge(trc, &arrayProtoShape_, "ForOfPIC Array.prototype shape.");
    TraceEdge(trc, &arrayIteratorProtoShape_, "ForOfPIC ArrayIterator.prototype shape.");

    TraceEdge(trc, &canonicalIteratorFunc_, "ForOfPIC ArrayValues builtin.");
    TraceEdge(trc, &canonicalNextFunc_, "ForOfPIC ArrayIterator.prototype.next builtin.");

    
    while (stubs_)
        removeStub(stubs_, nullptr);
}

void
js::ForOfPIC::Chain::sweep(FreeOp* fop)
{
    
    while (stubs_) {
        Stub* next = stubs_->next();
        fop->delete_(stubs_);
        stubs_ = next;
    }
    fop->delete_(this);
}

static void
ForOfPIC_finalize(FreeOp* fop, JSObject* obj)
{
    if (ForOfPIC::Chain* chain = ForOfPIC::fromJSObject(&obj->as<NativeObject>()))
        chain->sweep(fop);
}

static void
ForOfPIC_traceObject(JSTracer* trc, JSObject* obj)
{
    if (ForOfPIC::Chain* chain = ForOfPIC::fromJSObject(&obj->as<NativeObject>()))
        chain->mark(trc);
}

const Class ForOfPIC::jsclass = {
    "ForOfPIC", JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, ForOfPIC_finalize,
    nullptr,              
    nullptr,              
    nullptr,              
    ForOfPIC_traceObject
};

 NativeObject*
js::ForOfPIC::createForOfPICObject(JSContext* cx, Handle<GlobalObject*> global)
{
    assertSameCompartment(cx, global);
    NativeObject* obj = NewNativeObjectWithGivenProto(cx, &ForOfPIC::jsclass, NullPtr());
    if (!obj)
        return nullptr;
    ForOfPIC::Chain* chain = cx->new_<ForOfPIC::Chain>();
    if (!chain)
        return nullptr;
    obj->setPrivate(chain);
    return obj;
}

 js::ForOfPIC::Chain*
js::ForOfPIC::create(JSContext* cx)
{
    MOZ_ASSERT(!cx->global()->getForOfPICObject());
    Rooted<GlobalObject*> global(cx, cx->global());
    NativeObject* obj = GlobalObject::getOrCreateForOfPICObject(cx, global);
    if (!obj)
        return nullptr;
    return fromJSObject(obj);
}
