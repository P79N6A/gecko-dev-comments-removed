








#include <stdlib.h>
#include <string.h>

#include "mozilla/DebugOnly.h"

#include "jstypes.h"
#include "jsclist.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"

#include "js/HashTable.h"
#include "js/MemoryMetrics.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;

bool
ShapeTable::init(JSRuntime *rt, RawShape lastProp)
{
    






    uint32_t sizeLog2 = JS_CEILING_LOG2W(2 * entryCount);
    if (sizeLog2 < MIN_SIZE_LOG2)
        sizeLog2 = MIN_SIZE_LOG2;

    



    entries = (Shape **) rt->calloc_(sizeOfEntries(JS_BIT(sizeLog2)));
    if (!entries)
        return false;

    hashShift = HASH_BITS - sizeLog2;
    for (Shape::Range<NoGC> r(lastProp); !r.empty(); r.popFront()) {
        Shape &shape = r.front();
        Shape **spp = search(shape.propid(), true);

        



        if (!SHAPE_FETCH(spp))
            SHAPE_STORE_PRESERVING_COLLISION(spp, &shape);
    }
    return true;
}

bool
Shape::makeOwnBaseShape(JSContext *cx)
{
    JS_ASSERT(!base()->isOwned());
    assertSameCompartmentDebugOnly(cx, compartment());

    RawBaseShape nbase = js_NewGCBaseShape<NoGC>(cx);
    if (!nbase)
        return false;

    new (nbase) BaseShape(StackBaseShape(this));
    nbase->setOwned(base()->toUnowned());

    this->base_ = nbase;

    return true;
}

void
Shape::handoffTableTo(RawShape shape)
{
    JS_ASSERT(inDictionary() && shape->inDictionary());

    if (this == shape)
        return;

    JS_ASSERT(base()->isOwned() && !shape->base()->isOwned());

    RawBaseShape nbase = base();

    JS_ASSERT_IF(shape->hasSlot(), nbase->slotSpan() > shape->slot());

    this->base_ = nbase->baseUnowned();
    nbase->adoptUnowned(shape->base()->toUnowned());

    shape->base_ = nbase;
}

 bool
Shape::hashify(JSContext *cx, Shape *shape)
{
    JS_ASSERT(!shape->hasTable());

    if (!shape->ensureOwnBaseShape(cx))
        return false;

    JSRuntime *rt = cx->runtime;
    ShapeTable *table = rt->new_<ShapeTable>(shape->entryCount());
    if (!table)
        return false;

    if (!table->init(rt, shape)) {
        js_free(table);
        return false;
    }

    shape->base()->setTable(table);
    return true;
}





#define HASH1(hash0,shift)      ((hash0) >> (shift))
#define HASH2(hash0,log2,shift) ((((hash0) << (log2)) >> (shift)) | 1)

Shape **
ShapeTable::search(jsid id, bool adding)
{
    js::HashNumber hash0, hash1, hash2;
    int sizeLog2;
    Shape *stored, *shape, **spp, **firstRemoved;
    uint32_t sizeMask;

    JS_ASSERT(entries);
    JS_ASSERT(!JSID_IS_EMPTY(id));

    
    hash0 = HashId(id);
    hash1 = HASH1(hash0, hashShift);
    spp = entries + hash1;

    
    stored = *spp;
    if (SHAPE_IS_FREE(stored))
        return spp;

    
    shape = SHAPE_CLEAR_COLLISION(stored);
    if (shape && shape->propid() == id)
        return spp;

    
    sizeLog2 = HASH_BITS - hashShift;
    hash2 = HASH2(hash0, sizeLog2, hashShift);
    sizeMask = JS_BITMASK(sizeLog2);

#ifdef DEBUG
    uintptr_t collision_flag = SHAPE_COLLISION;
#endif

    
    if (SHAPE_IS_REMOVED(stored)) {
        firstRemoved = spp;
    } else {
        firstRemoved = NULL;
        if (adding && !SHAPE_HAD_COLLISION(stored))
            SHAPE_FLAG_COLLISION(spp, shape);
#ifdef DEBUG
        collision_flag &= uintptr_t(*spp) & SHAPE_COLLISION;
#endif
    }

    for (;;) {
        hash1 -= hash2;
        hash1 &= sizeMask;
        spp = entries + hash1;

        stored = *spp;
        if (SHAPE_IS_FREE(stored))
            return (adding && firstRemoved) ? firstRemoved : spp;

        shape = SHAPE_CLEAR_COLLISION(stored);
        if (shape && shape->propid() == id) {
            JS_ASSERT(collision_flag);
            return spp;
        }

        if (SHAPE_IS_REMOVED(stored)) {
            if (!firstRemoved)
                firstRemoved = spp;
        } else {
            if (adding && !SHAPE_HAD_COLLISION(stored))
                SHAPE_FLAG_COLLISION(spp, shape);
#ifdef DEBUG
            collision_flag &= uintptr_t(*spp) & SHAPE_COLLISION;
#endif
        }
    }

    
    return NULL;
}

bool
ShapeTable::change(int log2Delta, JSContext *cx)
{
    JS_ASSERT(entries);

    


    int oldlog2 = HASH_BITS - hashShift;
    int newlog2 = oldlog2 + log2Delta;
    uint32_t oldsize = JS_BIT(oldlog2);
    uint32_t newsize = JS_BIT(newlog2);
    Shape **newTable = (Shape **) cx->calloc_(sizeOfEntries(newsize));
    if (!newTable)
        return false;

    
    hashShift = HASH_BITS - newlog2;
    removedCount = 0;
    Shape **oldTable = entries;
    entries = newTable;

    
    for (Shape **oldspp = oldTable; oldsize != 0; oldspp++) {
        Shape *shape = SHAPE_FETCH(oldspp);
        if (shape) {
            Shape **spp = search(shape->propid(), true);
            JS_ASSERT(SHAPE_IS_FREE(*spp));
            *spp = shape;
        }
        oldsize--;
    }

    
    js_free(oldTable);
    return true;
}

bool
ShapeTable::grow(JSContext *cx)
{
    JS_ASSERT(needsToGrow());

    uint32_t size = capacity();
    int delta = removedCount < size >> 2;

    if (!change(delta, cx) && entryCount + removedCount == size - 1) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

RawShape
Shape::getChildBinding(JSContext *cx, const StackShape &child)
{
    JS_ASSERT(!inDictionary());

    
    uint32_t slots = child.slotSpan();
    gc::AllocKind kind = gc::GetGCObjectKind(slots);
    uint32_t nfixed = gc::GetGCKindSlots(kind);

    return cx->propertyTree().getChild(cx, this, nfixed, child);
}

 RawShape
Shape::replaceLastProperty(JSContext *cx, const StackBaseShape &base,
                           TaggedProto proto, HandleShape shape)
{
    JS_ASSERT(!shape->inDictionary());

    if (!shape->parent) {
        
        AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
        return EmptyShape::getInitialShape(cx, base.clasp, proto,
                                           base.parent, kind,
                                           base.flags & BaseShape::OBJECT_FLAG_MASK);
    }

    StackShape child(shape);
    StackShape::AutoRooter childRoot(cx, &child);
    {
        RawUnownedBaseShape nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;

        child.base = nbase;
    }

    return cx->propertyTree().getChild(cx, shape->parent, shape->numFixedSlots(), child);
}






 RawShape
JSObject::getChildProperty(JSContext *cx, HandleObject obj, HandleShape parent, StackShape &child)
{
    




    if (!child.hasSlot()) {
        child.setSlot(parent->maybeSlot());
    } else {
        if (child.hasMissingSlot()) {
            uint32_t slot;
            if (!allocSlot(cx, obj, &slot))
                return NULL;
            child.setSlot(slot);
        } else {
            
            JS_ASSERT(obj->inDictionaryMode() ||
                      parent->hasMissingSlot() ||
                      child.slot() == parent->maybeSlot() + 1);
        }
    }

    RootedShape shape(cx);

    if (obj->inDictionaryMode()) {
        JS_ASSERT(parent == obj->lastProperty());
        StackShape::AutoRooter childRoot(cx, &child);
        shape = js_NewGCShape(cx);
        if (!shape)
            return NULL;
        if (child.hasSlot() && child.slot() >= obj->lastProperty()->base()->slotSpan()) {
            if (!JSObject::setSlotSpan(cx, obj, child.slot() + 1))
                return NULL;
        }
        shape->initDictionaryShape(child, obj->numFixedSlots(), &obj->shape_);
    } else {
        shape = cx->propertyTree().getChild(cx, parent, obj->numFixedSlots(), child);
        if (!shape)
            return NULL;
        
        
        if (!JSObject::setLastProperty(cx, obj, shape))
            return NULL;
    }

    return shape;
}

bool
JSObject::toDictionaryMode(JSContext *cx)
{
    JS_ASSERT(!inDictionaryMode());

    
    JS_ASSERT(compartment() == cx->compartment);

    uint32_t span = slotSpan();

    RootedObject self(cx, this);

    





    RootedShape root(cx);
    RootedShape dictionaryShape(cx);

    RootedShape shape(cx, lastProperty());
    while (shape) {
        JS_ASSERT(!shape->inDictionary());

        RawShape dprop = js_NewGCShape(cx);
        if (!dprop) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        HeapPtrShape *listp = dictionaryShape
                              ? &dictionaryShape->parent
                              : (HeapPtrShape *) root.address();

        StackShape child(shape);
        dprop->initDictionaryShape(child, self->numFixedSlots(), listp);

        JS_ASSERT(!dprop->hasTable());
        dictionaryShape = dprop;
        shape = shape->previous();
    }

    if (!Shape::hashify(cx, root)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    JS_ASSERT((Shape **) root->listp == root.address());
    root->listp = &self->shape_;
    self->shape_ = root;

    JS_ASSERT(self->inDictionaryMode());
    root->base()->setSlotSpan(span);

    return true;
}





static inline bool
NormalizeGetterAndSetter(JSObject *obj,
                         jsid id, unsigned attrs, unsigned flags,
                         PropertyOp &getter,
                         StrictPropertyOp &setter)
{
    if (setter == JS_StrictPropertyStub) {
        JS_ASSERT(!(attrs & JSPROP_SETTER));
        setter = NULL;
    }
    if (getter == JS_PropertyStub) {
        JS_ASSERT(!(attrs & JSPROP_GETTER));
        getter = NULL;
    }

    return true;
}

 RawShape
JSObject::addProperty(JSContext *cx, HandleObject obj, HandleId id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32_t slot, unsigned attrs,
                      unsigned flags, int shortid, bool allowDictionary)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    if (!obj->isExtensible()) {
        obj->reportNotExtensible(cx);
        return NULL;
    }

    NormalizeGetterAndSetter(obj, id, attrs, flags, getter, setter);

    Shape **spp = NULL;
    if (obj->inDictionaryMode())
        spp = obj->lastProperty()->table().search(id, true);

    return addPropertyInternal(cx, obj, id, getter, setter, slot, attrs, flags, shortid,
                               spp, allowDictionary);
}

 RawShape
JSObject::addPropertyInternal(JSContext *cx, HandleObject obj, HandleId id,
                              PropertyOp getter, StrictPropertyOp setter,
                              uint32_t slot, unsigned attrs,
                              unsigned flags, int shortid, Shape **spp,
                              bool allowDictionary)
{
    JS_ASSERT_IF(!allowDictionary, !obj->inDictionaryMode());

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    ShapeTable *table = NULL;
    if (!obj->inDictionaryMode()) {
        bool stableSlot =
            (slot == SHAPE_INVALID_SLOT) ||
            obj->lastProperty()->hasMissingSlot() ||
            (slot == obj->lastProperty()->maybeSlot() + 1);
        JS_ASSERT_IF(!allowDictionary, stableSlot);
        if (allowDictionary &&
            (!stableSlot || obj->lastProperty()->entryCount() >= PropertyTree::MAX_HEIGHT)) {
            if (!obj->toDictionaryMode(cx))
                return NULL;
            table = &obj->lastProperty()->table();
            spp = table->search(id, true);
        }
    } else {
        table = &obj->lastProperty()->table();
        if (table->needsToGrow()) {
            if (!table->grow(cx))
                return NULL;
            spp = table->search(id, true);
            JS_ASSERT(!SHAPE_FETCH(spp));
        }
    }

    JS_ASSERT(!!table == !!spp);

    
    RootedShape shape(cx);
    {
        RootedShape last(cx, obj->lastProperty());

        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);

        Rooted<UnownedBaseShape*> nbase(cx);
        if (last->base()->matchesGetterSetter(getter, setter) && !indexed) {
            nbase = last->base()->unowned();
        } else {
            StackBaseShape base(last->base());
            base.updateGetterSetter(attrs, getter, setter);
            if (indexed)
                base.flags |= BaseShape::INDEXED;
            nbase = BaseShape::getUnowned(cx, base);
            if (!nbase)
                return NULL;
        }

        StackShape child(nbase, id, slot, obj->numFixedSlots(), attrs, flags, shortid);
        shape = getChildProperty(cx, obj, last, child);
    }

    if (shape) {
        JS_ASSERT(shape == obj->lastProperty());

        if (table) {
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, static_cast<RawShape>(shape));
            ++table->entryCount;

            
            JS_ASSERT(&shape->parent->table() == table);
            shape->parent->handoffTableTo(shape);
        }

        obj->checkShapeConsistency();
        return shape;
    }

    obj->checkShapeConsistency();
    return NULL;
}






inline bool
CheckCanChangeAttrs(JSContext *cx, JSObject *obj, Shape *shape, unsigned *attrsp)
{
    if (shape->configurable())
        return true;

    
    *attrsp |= JSPROP_PERMANENT;

    
    if (shape->isDataDescriptor() && shape->hasSlot() &&
        (*attrsp & (JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED))) {
        obj->reportNotConfigurable(cx, shape->propid());
        return false;
    }

    return true;
}

 RawShape
JSObject::putProperty(JSContext *cx, HandleObject obj, HandleId id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32_t slot, unsigned attrs,
                      unsigned flags, int shortid)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    NormalizeGetterAndSetter(obj, id, attrs, flags, getter, setter);

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    
    Shape **spp;
    RootedShape shape(cx, Shape::search(cx, obj->lastProperty(), id, &spp, true));
    if (!shape) {
        



        if (!obj->isExtensible()) {
            obj->reportNotExtensible(cx);
            return NULL;
        }

        return addPropertyInternal(cx, obj, id, getter, setter, slot, attrs, flags, shortid, spp, true);
    }

    
    JS_ASSERT_IF(spp, !SHAPE_IS_REMOVED(*spp));

    if (!CheckCanChangeAttrs(cx, obj, shape, &attrs))
        return NULL;

    




    bool hadSlot = shape->hasSlot();
    uint32_t oldSlot = shape->maybeSlot();
    if (!(attrs & JSPROP_SHARED) && slot == SHAPE_INVALID_SLOT && hadSlot)
        slot = oldSlot;

    Rooted<UnownedBaseShape*> nbase(cx);
    {
        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);
        StackBaseShape base(obj->lastProperty()->base());
        base.updateGetterSetter(attrs, getter, setter);
        if (indexed)
            base.flags |= BaseShape::INDEXED;
        nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;
    }

    



    if (shape->matchesParamsAfterId(nbase, slot, attrs, flags, shortid))
        return shape;

    




    if (shape != obj->lastProperty() && !obj->inDictionaryMode()) {
        if (!obj->toDictionaryMode(cx))
            return NULL;
        spp = obj->lastProperty()->table().search(shape->propid(), false);
        shape = SHAPE_FETCH(spp);
    }

    JS_ASSERT_IF(shape->hasSlot() && !(attrs & JSPROP_SHARED), shape->slot() == slot);

    if (obj->inDictionaryMode()) {
        





        bool updateLast = (shape == obj->lastProperty());
        shape = obj->replaceWithNewEquivalentShape(cx, shape);
        if (!shape)
            return NULL;
        if (!updateLast && !obj->generateOwnShape(cx))
            return NULL;

        
        if (slot == SHAPE_INVALID_SLOT && !(attrs & JSPROP_SHARED)) {
            if (!allocSlot(cx, obj, &slot))
                return NULL;
        }

        if (updateLast)
            shape->base()->adoptUnowned(nbase);
        else
            shape->base_ = nbase;

        shape->setSlot(slot);
        shape->attrs = uint8_t(attrs);
        shape->flags = flags | Shape::IN_DICTIONARY;
        shape->shortid_ = int16_t(shortid);
    } else {
        



        StackBaseShape base(obj->lastProperty()->base());
        base.updateGetterSetter(attrs, getter, setter);

        RawUnownedBaseShape nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;

        JS_ASSERT(shape == obj->lastProperty());

        
        StackShape child(nbase, id, slot, obj->numFixedSlots(), attrs, flags, shortid);
        RootedShape parent(cx, shape->parent);
        RawShape newShape = JSObject::getChildProperty(cx, obj, parent, child);

        if (!newShape) {
            obj->checkShapeConsistency();
            return NULL;
        }

        shape = newShape;
    }

    





    if (hadSlot && !shape->hasSlot()) {
        if (oldSlot < obj->slotSpan())
            obj->freeSlot(oldSlot);
        ++cx->runtime->propertyRemovals;
    }

    obj->checkShapeConsistency();

    return shape;
}

 RawShape
JSObject::changeProperty(JSContext *cx, HandleObject obj, HandleShape shape, unsigned attrs,
                         unsigned mask, PropertyOp getter, StrictPropertyOp setter)
{
    JS_ASSERT(obj->nativeContains(cx, shape));

    attrs |= shape->attrs & mask;

    
    JS_ASSERT(!((attrs ^ shape->attrs) & JSPROP_SHARED) ||
              !(attrs & JSPROP_SHARED));

    types::MarkTypePropertyConfigured(cx, obj, shape->propid());
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER))
        types::AddTypePropertyId(cx, obj, shape->propid(), types::Type::UnknownType());

    if (getter == JS_PropertyStub)
        getter = NULL;
    if (setter == JS_StrictPropertyStub)
        setter = NULL;

    if (!CheckCanChangeAttrs(cx, obj, shape, &attrs))
        return NULL;

    if (shape->attrs == attrs && shape->getter() == getter && shape->setter() == setter)
        return shape;

    





    RootedId propid(cx, shape->propid());
    RawShape newShape = putProperty(cx, obj, propid, getter, setter, shape->maybeSlot(),
                                    attrs, shape->flags, shape->maybeShortid());

    obj->checkShapeConsistency();
    return newShape;
}

bool
JSObject::removeProperty(JSContext *cx, jsid id_)
{
    RootedId id(cx, id_);
    RootedObject self(cx, this);

    Shape **spp;
    RootedShape shape(cx, Shape::search(cx, lastProperty(), id, &spp));
    if (!shape)
        return true;

    



    if (!self->inDictionaryMode() && (shape != self->lastProperty() || !self->canRemoveLastProperty())) {
        if (!self->toDictionaryMode(cx))
            return false;
        spp = self->lastProperty()->table().search(shape->propid(), false);
        shape = SHAPE_FETCH(spp);
    }

    






    RootedShape spare(cx);
    if (self->inDictionaryMode()) {
        spare = js_NewGCShape(cx);
        if (!spare)
            return false;
        new (spare) Shape(shape->base()->unowned(), 0);
        if (shape == self->lastProperty()) {
            





            RootedShape previous(cx, self->lastProperty()->parent);
            StackBaseShape base(self->lastProperty()->base());
            base.updateGetterSetter(previous->attrs, previous->getter(), previous->setter());
            RawBaseShape nbase = BaseShape::getUnowned(cx, base);
            if (!nbase)
                return false;
            previous->base_ = nbase;
        }
    }

    
    if (shape->hasSlot()) {
        self->freeSlot(shape->slot());
        ++cx->runtime->propertyRemovals;
    }

    




    if (self->inDictionaryMode()) {
        ShapeTable &table = self->lastProperty()->table();

        if (SHAPE_HAD_COLLISION(*spp)) {
            *spp = SHAPE_REMOVED;
            ++table.removedCount;
            --table.entryCount;
        } else {
            *spp = NULL;
            --table.entryCount;

#ifdef DEBUG
            




            RawShape aprop = self->lastProperty();
            for (int n = 50; --n >= 0 && aprop->parent; aprop = aprop->parent)
                JS_ASSERT_IF(aprop != shape, self->nativeContains(cx, aprop));
#endif
        }

        {
            
            RawShape oldLastProp = self->lastProperty();
            shape->removeFromDictionary(self);

            
            oldLastProp->handoffTableTo(self->lastProperty());
        }

        
        JS_ALWAYS_TRUE(self->generateOwnShape(cx, spare));

        
        uint32_t size = table.capacity();
        if (size > ShapeTable::MIN_SIZE && table.entryCount <= size >> 2)
            (void) table.change(-1, cx);
    } else {
        





        JS_ASSERT(shape == self->lastProperty());
        self->removeLastProperty(cx);
    }

    self->checkShapeConsistency();
    return true;
}

 void
JSObject::clear(JSContext *cx, HandleObject obj)
{
    RootedShape shape(cx, obj->lastProperty());
    JS_ASSERT(obj->inDictionaryMode() == shape->inDictionary());

    while (shape->parent) {
        shape = shape->parent;
        JS_ASSERT(obj->inDictionaryMode() == shape->inDictionary());
    }
    JS_ASSERT(shape->isEmptyShape());

    if (obj->inDictionaryMode())
        shape->listp = &obj->shape_;

    JS_ALWAYS_TRUE(JSObject::setLastProperty(cx, obj, shape));

    ++cx->runtime->propertyRemovals;
    obj->checkShapeConsistency();
}

void
JSObject::rollbackProperties(JSContext *cx, uint32_t slotSpan)
{
    




    JS_ASSERT(!inDictionaryMode() && slotSpan <= this->slotSpan());
    while (this->slotSpan() != slotSpan) {
        JS_ASSERT(lastProperty()->hasSlot() && getSlot(lastProperty()->slot()).isUndefined());
        removeLastProperty(cx);
    }
}

Shape *
JSObject::replaceWithNewEquivalentShape(JSContext *cx, Shape *oldShape, Shape *newShape)
{
    JS_ASSERT(cx->compartment == oldShape->compartment());
    JS_ASSERT_IF(oldShape != lastProperty(),
                 inDictionaryMode() &&
                 nativeLookup(cx, oldShape->propidRef()) == oldShape);

    JSObject *self = this;

    if (!inDictionaryMode()) {
        RootedObject selfRoot(cx, self);
        RootedShape newRoot(cx, newShape);
        if (!toDictionaryMode(cx))
            return NULL;
        oldShape = selfRoot->lastProperty();
        self = selfRoot;
        newShape = newRoot;
    }

    if (!newShape) {
        RootedObject selfRoot(cx, self);
        RootedShape oldRoot(cx, oldShape);
        newShape = js_NewGCShape(cx);
        if (!newShape)
            return NULL;
        new (newShape) Shape(oldRoot->base()->unowned(), 0);
        self = selfRoot;
        oldShape = oldRoot;
    }

    ShapeTable &table = self->lastProperty()->table();
    Shape **spp = oldShape->isEmptyShape()
                  ? NULL
                  : table.search(oldShape->propidRef(), false);

    



    StackShape nshape(oldShape);
    newShape->initDictionaryShape(nshape, self->numFixedSlots(), oldShape->listp);

    JS_ASSERT(newShape->parent == oldShape);
    oldShape->removeFromDictionary(self);

    if (newShape == self->lastProperty())
        oldShape->handoffTableTo(newShape);

    if (spp)
        SHAPE_STORE_PRESERVING_COLLISION(spp, newShape);
    return newShape;
}

bool
JSObject::shadowingShapeChange(JSContext *cx, const Shape &shape)
{
    return generateOwnShape(cx);
}

 bool
JSObject::clearParent(JSContext *cx, HandleObject obj)
{
    return setParent(cx, obj, NullPtr());
}

 bool
JSObject::setParent(JSContext *cx, HandleObject obj, HandleObject parent)
{
    if (parent && !parent->setDelegate(cx))
        return false;

    if (obj->inDictionaryMode()) {
        StackBaseShape base(obj->lastProperty());
        base.parent = parent;
        RawUnownedBaseShape nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return false;

        obj->lastProperty()->base()->adoptUnowned(nbase);
        return true;
    }

    RawShape newShape = Shape::setObjectParent(cx, parent, obj->getTaggedProto(), obj->shape_);
    if (!newShape)
        return false;

    obj->shape_ = newShape;
    return true;
}

 RawShape
Shape::setObjectParent(JSContext *cx, JSObject *parent, TaggedProto proto, Shape *last)
{
    if (last->getObjectParent() == parent)
        return last;

    StackBaseShape base(last);
    base.parent = parent;

    RootedShape lastRoot(cx, last);
    return replaceLastProperty(cx, base, proto, lastRoot);
}

bool
JSObject::preventExtensions(JSContext *cx)
{
    JS_ASSERT(isExtensible());

    RootedObject self(cx, this);

    



    AutoIdVector props(cx);
    if (!js::GetPropertyNames(cx, self, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    





    if (self->isNative() && !JSObject::sparsifyDenseElements(cx, self))
        return false;

    return self->setFlag(cx, BaseShape::NOT_EXTENSIBLE, GENERATE_SHAPE);
}

bool
JSObject::setFlag(JSContext *cx,  uint32_t flag_, GenerateShape generateShape)
{
    BaseShape::Flag flag = (BaseShape::Flag) flag_;

    if (lastProperty()->getObjectFlags() & flag)
        return true;

    RootedObject self(cx, this);

    if (inDictionaryMode()) {
        if (generateShape == GENERATE_SHAPE && !generateOwnShape(cx))
            return false;
        StackBaseShape base(self->lastProperty());
        base.flags |= flag;
        RawUnownedBaseShape nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return false;

        self->lastProperty()->base()->adoptUnowned(nbase);
        return true;
    }

    RawShape newShape = Shape::setObjectFlag(cx, flag, getTaggedProto(), lastProperty());
    if (!newShape)
        return false;

    self->shape_ = newShape;
    return true;
}

bool
JSObject::clearFlag(JSContext *cx,  uint32_t flag)
{
    JS_ASSERT(inDictionaryMode());
    JS_ASSERT(lastProperty()->getObjectFlags() & flag);

    RootedObject self(cx, this);

    StackBaseShape base(self->lastProperty());
    base.flags &= ~flag;
    RawUnownedBaseShape nbase = BaseShape::getUnowned(cx, base);
    if (!nbase)
        return false;

    self->lastProperty()->base()->adoptUnowned(nbase);
    return true;
}

 RawShape
Shape::setObjectFlag(JSContext *cx, BaseShape::Flag flag, TaggedProto proto, Shape *last)
{
    if (last->getObjectFlags() & flag)
        return last;

    StackBaseShape base(last);
    base.flags |= flag;

    RootedShape lastRoot(cx, last);
    return replaceLastProperty(cx, base, proto, lastRoot);
}

 inline HashNumber
StackBaseShape::hash(const StackBaseShape *base)
{
    HashNumber hash = base->flags;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(base->clasp) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(base->parent) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ uintptr_t(base->rawGetter);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ uintptr_t(base->rawSetter);
    return hash;
}

 inline bool
StackBaseShape::match(RawUnownedBaseShape key, const StackBaseShape *lookup)
{
    return key->flags == lookup->flags
        && key->clasp == lookup->clasp
        && key->parent == lookup->parent
        && key->rawGetter == lookup->rawGetter
        && key->rawSetter == lookup->rawSetter;
}

 UnownedBaseShape*
BaseShape::getUnowned(JSContext *cx, const StackBaseShape &base)
{
    BaseShapeSet &table = cx->compartment->baseShapes;

    if (!table.initialized() && !table.init())
        return NULL;

    BaseShapeSet::AddPtr p = table.lookupForAdd(&base);

    if (p)
        return *p;

    StackBaseShape::AutoRooter root(cx, &base);

    RawBaseShape nbase_ = js_NewGCBaseShape<CanGC>(cx);
    if (!nbase_)
        return NULL;

    new (nbase_) BaseShape(base);

    RawUnownedBaseShape nbase = static_cast<RawUnownedBaseShape>(nbase_);

    if (!table.relookupOrAdd(p, &base, nbase))
        return NULL;

    return nbase;
}

void
JSCompartment::sweepBaseShapeTable()
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES_BASE_SHAPE);

    if (baseShapes.initialized()) {
        for (BaseShapeSet::Enum e(baseShapes); !e.empty(); e.popFront()) {
            RawUnownedBaseShape base = e.front();
            if (IsBaseShapeAboutToBeFinalized(&base))
                e.removeFront();
        }
    }
}

void
BaseShape::finalize(FreeOp *fop)
{
    if (table_) {
        fop->delete_(table_);
        table_ = NULL;
    }
}

inline
InitialShapeEntry::InitialShapeEntry() : shape(NULL), proto(NULL)
{
}

inline
InitialShapeEntry::InitialShapeEntry(const ReadBarriered<Shape> &shape, TaggedProto proto)
  : shape(shape), proto(proto)
{
}

inline InitialShapeEntry::Lookup
InitialShapeEntry::getLookup() const
{
    return Lookup(shape->getObjectClass(), proto, shape->getObjectParent(),
                  shape->numFixedSlots(), shape->getObjectFlags());
}

 inline HashNumber
InitialShapeEntry::hash(const Lookup &lookup)
{
    HashNumber hash = uintptr_t(lookup.clasp) >> 3;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(lookup.proto.toWord()) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(lookup.parent) >> 3);
    return hash + lookup.nfixed;
}

 inline bool
InitialShapeEntry::match(const InitialShapeEntry &key, const Lookup &lookup)
{
    const Shape *shape = *key.shape.unsafeGet();
    return lookup.clasp == shape->getObjectClass()
        && lookup.proto.toWord() == key.proto.toWord()
        && lookup.parent == shape->getObjectParent()
        && lookup.nfixed == shape->numFixedSlots()
        && lookup.baseFlags == shape->getObjectFlags();
}

 Shape *
EmptyShape::getInitialShape(JSContext *cx, Class *clasp, TaggedProto proto, JSObject *parent,
                            size_t nfixed, uint32_t objectFlags)
{
    JS_ASSERT_IF(proto.isObject(), cx->compartment == proto.toObject()->compartment());
    JS_ASSERT_IF(parent, cx->compartment == parent->compartment());

    InitialShapeSet &table = cx->compartment->initialShapes;

    if (!table.initialized() && !table.init())
        return NULL;

    InitialShapeEntry::Lookup lookup(clasp, proto, parent, nfixed, objectFlags);

    InitialShapeSet::AddPtr p = table.lookupForAdd(lookup);

    if (p)
        return p->shape;

    Rooted<TaggedProto> protoRoot(cx, lookup.proto);
    RootedObject parentRoot(cx, lookup.parent);

    StackBaseShape base(cx->compartment, clasp, parent, objectFlags);
    Rooted<UnownedBaseShape*> nbase(cx, BaseShape::getUnowned(cx, base));
    if (!nbase)
        return NULL;

    RawShape shape = cx->propertyTree().newShape(cx);
    if (!shape)
        return NULL;
    new (shape) EmptyShape(nbase, nfixed);

    lookup.proto = protoRoot;
    lookup.parent = parentRoot;

    if (!table.relookupOrAdd(p, lookup, InitialShapeEntry(shape, lookup.proto)))
        return NULL;

    return shape;
}

 Shape *
EmptyShape::getInitialShape(JSContext *cx, Class *clasp, TaggedProto proto, JSObject *parent,
                            AllocKind kind, uint32_t objectFlags)
{
    return getInitialShape(cx, clasp, proto, parent, GetGCKindSlots(kind, clasp), objectFlags);
}

void
NewObjectCache::invalidateEntriesForShape(JSContext *cx, HandleShape shape, HandleObject proto)
{
    Class *clasp = shape->getObjectClass();

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    if (CanBeFinalizedInBackground(kind, clasp))
        kind = GetBackgroundAllocKind(kind);

    Rooted<GlobalObject *> global(cx, &shape->getObjectParent()->global());
    Rooted<types::TypeObject *> type(cx, proto->getNewType(cx, clasp));

    EntryIndex entry;
    if (lookupGlobal(clasp, global, kind, &entry))
        PodZero(&entries[entry]);
    if (!proto->isGlobal() && lookupProto(clasp, proto, kind, &entry))
        PodZero(&entries[entry]);
    if (lookupType(clasp, type, kind, &entry))
        PodZero(&entries[entry]);
}

 void
EmptyShape::insertInitialShape(JSContext *cx, HandleShape shape, HandleObject proto)
{
    InitialShapeEntry::Lookup lookup(shape->getObjectClass(), TaggedProto(proto),
                                     shape->getObjectParent(), shape->numFixedSlots(),
                                     shape->getObjectFlags());

    InitialShapeSet::Ptr p = cx->compartment->initialShapes.lookup(lookup);
    JS_ASSERT(p);

    InitialShapeEntry &entry = const_cast<InitialShapeEntry &>(*p);
    JS_ASSERT(entry.shape->isEmptyShape());

    
#ifdef DEBUG
    RawShape nshape = shape;
    while (!nshape->isEmptyShape())
        nshape = nshape->previous();
    JS_ASSERT(nshape == entry.shape);
#endif

    entry.shape = shape.get();

    







    cx->runtime->newObjectCache.invalidateEntriesForShape(cx, shape, proto);
}

void
JSCompartment::sweepInitialShapeTable()
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES_INITIAL_SHAPE);

    if (initialShapes.initialized()) {
        for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
            const InitialShapeEntry &entry = e.front();
            RawShape shape = entry.shape;
            JSObject *proto = entry.proto.raw();
            if (IsShapeAboutToBeFinalized(&shape) || (entry.proto.isObject() && IsObjectAboutToBeFinalized(&proto))) {
                e.removeFront();
            } else {
#ifdef DEBUG
                DebugOnly<JSObject *> parent = shape->getObjectParent();
                JS_ASSERT(!parent || !IsObjectAboutToBeFinalized(&parent));
                JS_ASSERT(parent == shape->getObjectParent());
#endif
                if (shape != entry.shape || proto != entry.proto.raw()) {
                    InitialShapeEntry newKey(shape, proto);
                    e.rekeyFront(newKey.getLookup(), newKey);
                }
            }
        }
    }
}

