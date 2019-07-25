










































#include <new>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsclist.h"
#include "jsdhash.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsstr.h"

#include "js/MemoryMetrics.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;
using namespace js::gc;

bool
PropertyTable::init(JSRuntime *rt, Shape *lastProp)
{
    






    uint32_t sizeLog2 = JS_CEILING_LOG2W(2 * entryCount);
    if (sizeLog2 < MIN_SIZE_LOG2)
        sizeLog2 = MIN_SIZE_LOG2;

    



    entries = (Shape **) rt->calloc_(sizeOfEntries(JS_BIT(sizeLog2)));
    if (!entries)
        return false;

    hashShift = JS_DHASH_BITS - sizeLog2;
    for (Shape::Range r = lastProp->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
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
    assertSameCompartment(cx, compartment());

    RootedVarShape self(cx, this);

    BaseShape *nbase = js_NewGCBaseShape(cx);
    if (!nbase)
        return false;

    new (nbase) BaseShape(*self->base());
    nbase->setOwned(self->base()->toUnowned());

    self->base_ = nbase;

    return true;
}

void
Shape::handoffTableTo(Shape *shape)
{
    JS_ASSERT(inDictionary() && shape->inDictionary());

    if (this == shape)
        return;

    JS_ASSERT(base()->isOwned() && !shape->base()->isOwned());

    BaseShape *nbase = base();

    JS_ASSERT_IF(shape->hasSlot(), nbase->slotSpan() > shape->slot());

    this->base_ = nbase->baseUnowned();
    nbase->adoptUnowned(shape->base()->toUnowned());

    shape->base_ = nbase;
}

bool
Shape::hashify(JSContext *cx)
{
    JS_ASSERT(!hasTable());

    RootedVarShape self(cx, this);

    if (!ensureOwnBaseShape(cx))
        return false;

    JSRuntime *rt = cx->runtime;
    PropertyTable *table = rt->new_<PropertyTable>(self->entryCount());
    if (!table)
        return false;

    if (!table->init(rt, self)) {
        rt->free_(table);
        return false;
    }

    self->base()->setTable(table);
    return true;
}





#define HASH1(hash0,shift)      ((hash0) >> (shift))
#define HASH2(hash0,log2,shift) ((((hash0) << (log2)) >> (shift)) | 1)

Shape **
PropertyTable::search(jsid id, bool adding)
{
    JSHashNumber hash0, hash1, hash2;
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

    
    sizeLog2 = JS_DHASH_BITS - hashShift;
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
PropertyTable::change(int log2Delta, JSContext *cx)
{
    JS_ASSERT(entries);

    


    int oldlog2 = JS_DHASH_BITS - hashShift;
    int newlog2 = oldlog2 + log2Delta;
    uint32_t oldsize = JS_BIT(oldlog2);
    uint32_t newsize = JS_BIT(newlog2);
    Shape **newTable = (Shape **) cx->calloc_(sizeOfEntries(newsize));
    if (!newTable)
        return false;

    
    hashShift = JS_DHASH_BITS - newlog2;
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

    
    cx->free_(oldTable);
    return true;
}

bool
PropertyTable::grow(JSContext *cx)
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

Shape *
Shape::getChildBinding(JSContext *cx, const StackShape &child)
{
    JS_ASSERT(!inDictionary());

    Shape *shape = JS_PROPERTY_TREE(cx).getChild(cx, this, numFixedSlots(), child);
    if (shape) {
        JS_ASSERT(shape->parent == this);

        






        uint32_t slots = child.slotSpan() + 1;  
        gc::AllocKind kind = gc::GetGCObjectKind(slots);

        





        uint32_t nfixed = gc::GetGCKindSlots(kind);
        if (nfixed < slots) {
            nfixed = CallObject::RESERVED_SLOTS + 1;
            JS_ASSERT(gc::GetGCKindSlots(gc::GetGCObjectKind(nfixed)) == CallObject::RESERVED_SLOTS + 1);
        }

        shape->setNumFixedSlots(nfixed - 1);
    }
    return shape;
}

 Shape *
Shape::replaceLastProperty(JSContext *cx, const StackBaseShape &base, JSObject *proto, Shape *shape)
{
    JS_ASSERT(!shape->inDictionary());

    if (!shape->parent) {
        
        AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
        return EmptyShape::getInitialShape(cx, base.clasp, proto,
                                           base.parent, kind,
                                           base.flags & BaseShape::OBJECT_FLAG_MASK);
    }

    RootShape root(cx, &shape);

    UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
    if (!nbase)
        return NULL;

    StackShape child(shape);
    child.base = nbase;

    return JS_PROPERTY_TREE(cx).getChild(cx, shape->parent, shape->numFixedSlots(), child);
}






Shape *
JSObject::getChildProperty(JSContext *cx, Shape *parent, StackShape &child)
{
    




    if (!child.hasSlot()) {
        child.setSlot(parent->maybeSlot());
    } else {
        if (child.hasMissingSlot()) {
            uint32_t slot;
            if (!allocSlot(cx, &slot))
                return NULL;
            child.setSlot(slot);
        } else {
            
            JS_ASSERT(inDictionaryMode() ||
                      parent->hasMissingSlot() ||
                      child.slot() == parent->maybeSlot() + 1);
        }
    }

    Shape *shape;

    RootedVarObject self(cx, this);

    if (inDictionaryMode()) {
        JS_ASSERT(parent == lastProperty());
        RootStackShape childRoot(cx, &child);
        shape = js_NewGCShape(cx);
        if (!shape)
            return NULL;
        if (child.hasSlot() && child.slot() >= self->lastProperty()->base()->slotSpan()) {
            if (!self->setSlotSpan(cx, child.slot() + 1))
                return NULL;
        }
        shape->initDictionaryShape(child, self->numFixedSlots(), &self->shape_);
    } else {
        shape = JS_PROPERTY_TREE(cx).getChild(cx, parent, self->numFixedSlots(), child);
        if (!shape)
            return NULL;
        
        
        if (!self->setLastProperty(cx, shape))
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

    RootedVarObject self(cx, this);

    





    RootedVarShape root(cx);
    RootedVarShape dictionaryShape(cx);

    RootedVarShape shape(cx);
    shape = lastProperty();

    while (shape) {
        JS_ASSERT(!shape->inDictionary());

        Shape *dprop = js_NewGCShape(cx);
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

    if (!root->hashify(cx)) {
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
NormalizeGetterAndSetter(JSContext *cx, JSObject *obj,
                         jsid id, unsigned attrs, unsigned flags,
                         PropertyOp &getter,
                         StrictPropertyOp &setter)
{
    if (setter == JS_StrictPropertyStub) {
        JS_ASSERT(!(attrs & JSPROP_SETTER));
        setter = NULL;
    }
    if (flags & Shape::METHOD) {
        JS_ASSERT_IF(getter, getter == JS_PropertyStub);
        JS_ASSERT(!setter);
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        getter = NULL;
    } else {
        if (getter == JS_PropertyStub) {
            JS_ASSERT(!(attrs & JSPROP_GETTER));
            getter = NULL;
        }
    }

    return true;
}

Shape *
JSObject::addProperty(JSContext *cx, jsid id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32_t slot, unsigned attrs,
                      unsigned flags, int shortid, bool allowDictionary)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    if (!isExtensible()) {
        reportNotExtensible(cx);
        return NULL;
    }

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    Shape **spp = NULL;
    if (inDictionaryMode())
        spp = lastProperty()->table().search(id, true);

    return addPropertyInternal(cx, id, getter, setter, slot, attrs, flags, shortid,
                               spp, allowDictionary);
}

Shape *
JSObject::addPropertyInternal(JSContext *cx, jsid id,
                              PropertyOp getter, StrictPropertyOp setter,
                              uint32_t slot, unsigned attrs,
                              unsigned flags, int shortid, Shape **spp,
                              bool allowDictionary)
{
    JS_ASSERT_IF(!allowDictionary, !inDictionaryMode());

    RootId idRoot(cx, &id);
    RootedVarObject self(cx, this);

    PropertyTable *table = NULL;
    if (!inDictionaryMode()) {
        bool stableSlot =
            (slot == SHAPE_INVALID_SLOT) ||
            lastProperty()->hasMissingSlot() ||
            (slot == lastProperty()->maybeSlot() + 1);
        JS_ASSERT_IF(!allowDictionary, stableSlot);
        if (allowDictionary &&
            (!stableSlot || lastProperty()->entryCount() >= PropertyTree::MAX_HEIGHT)) {
            if (!toDictionaryMode(cx))
                return NULL;
            table = &lastProperty()->table();
            spp = table->search(id, true);
        }
    } else {
        table = &lastProperty()->table();
        if (table->needsToGrow()) {
            if (!table->grow(cx))
                return NULL;
            spp = table->search(id, true);
            JS_ASSERT(!SHAPE_FETCH(spp));
        }
    }

    JS_ASSERT(!!table == !!spp);

    
    Shape *shape;
    {
        shape = self->lastProperty();

        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);
        UnownedBaseShape *nbase;
        if (shape->base()->matchesGetterSetter(getter, setter) && !indexed) {
            nbase = shape->base()->unowned();
        } else {
            StackBaseShape base(shape->base());
            base.updateGetterSetter(attrs, getter, setter);
            if (indexed)
                base.flags |= BaseShape::INDEXED;
            nbase = BaseShape::getUnowned(cx, base);
            if (!nbase)
                return NULL;
        }

        StackShape child(nbase, id, slot, self->numFixedSlots(), attrs, flags, shortid);
        shape = self->getChildProperty(cx, self->lastProperty(), child);
    }

    if (shape) {
        JS_ASSERT(shape == self->lastProperty());

        if (table) {
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, shape);
            ++table->entryCount;

            
            JS_ASSERT(&shape->parent->table() == table);
            shape->parent->handoffTableTo(shape);
        }

        self->checkShapeConsistency();
        return shape;
    }

    self->checkShapeConsistency();
    return NULL;
}






inline bool
CheckCanChangeAttrs(JSContext *cx, JSObject *obj, const Shape *shape, unsigned *attrsp)
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

Shape *
JSObject::putProperty(JSContext *cx, jsid id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32_t slot, unsigned attrs,
                      unsigned flags, int shortid)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    RootId idRoot(cx, &id);

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    RootedVarObject self(cx, this);

    
    Shape **spp;
    Shape *shape = Shape::search(cx, lastProperty(), id, &spp, true);
    if (!shape) {
        



        if (!self->isExtensible()) {
            self->reportNotExtensible(cx);
            return NULL;
        }

        return self->addPropertyInternal(cx, id, getter, setter, slot, attrs, flags, shortid, spp, true);
    }

    
    JS_ASSERT_IF(spp, !SHAPE_IS_REMOVED(*spp));

    RootShape shapeRoot(cx, &shape);

    if (!CheckCanChangeAttrs(cx, self, shape, &attrs))
        return NULL;
    
    




    bool hadSlot = shape->hasSlot();
    uint32_t oldSlot = shape->maybeSlot();
    if (!(attrs & JSPROP_SHARED) && slot == SHAPE_INVALID_SLOT && hadSlot)
        slot = oldSlot;

    RootedVar<UnownedBaseShape*> nbase(cx);
    {
        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);
        StackBaseShape base(self->lastProperty()->base());
        base.updateGetterSetter(attrs, getter, setter);
        if (indexed)
            base.flags |= BaseShape::INDEXED;
        nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;
    }

    



    if (shape->matchesParamsAfterId(nbase, slot, attrs, flags, shortid))
        return shape;

    




    if (shape != self->lastProperty() && !self->inDictionaryMode()) {
        if (!self->toDictionaryMode(cx))
            return NULL;
        spp = self->lastProperty()->table().search(shape->propid(), false);
        shape = SHAPE_FETCH(spp);
    }

    JS_ASSERT_IF(shape->hasSlot() && !(attrs & JSPROP_SHARED), shape->slot() == slot);

    if (self->inDictionaryMode()) {
        





        bool updateLast = (shape == self->lastProperty());
        shape = self->replaceWithNewEquivalentShape(cx, shape);
        if (!shape)
            return NULL;
        if (!updateLast && !self->generateOwnShape(cx))
            return NULL;

        
        if (slot == SHAPE_INVALID_SLOT && !(attrs & JSPROP_SHARED)) {
            if (!self->allocSlot(cx, &slot))
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
        



        StackBaseShape base(self->lastProperty()->base());
        base.updateGetterSetter(attrs, getter, setter);
        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;

        JS_ASSERT(shape == self->lastProperty());

        
        StackShape child(nbase, id, slot, self->numFixedSlots(), attrs, flags, shortid);
        Shape *newShape = self->getChildProperty(cx, shape->parent, child);

        if (!newShape) {
            self->checkShapeConsistency();
            return NULL;
        }

        shape = newShape;
    }

    





    if (hadSlot && !shape->hasSlot()) {
        if (oldSlot < self->slotSpan())
            self->freeSlot(cx, oldSlot);
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    self->checkShapeConsistency();

    return shape;
}

Shape *
JSObject::changeProperty(JSContext *cx, Shape *shape, unsigned attrs, unsigned mask,
                         PropertyOp getter, StrictPropertyOp setter)
{
    JS_ASSERT(nativeContains(cx, *shape));

    attrs |= shape->attrs & mask;

    
    JS_ASSERT(!((attrs ^ shape->attrs) & JSPROP_SHARED) ||
              !(attrs & JSPROP_SHARED));

    
    JS_ASSERT_IF(shape->isMethod(), !getter && !setter);

    types::MarkTypePropertyConfigured(cx, this, shape->propid());
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER))
        types::AddTypePropertyId(cx, this, shape->propid(), types::Type::UnknownType());

    if (getter == JS_PropertyStub)
        getter = NULL;
    if (setter == JS_StrictPropertyStub)
        setter = NULL;

    if (!CheckCanChangeAttrs(cx, this, shape, &attrs))
        return NULL;
    
    if (shape->attrs == attrs && shape->getter() == getter && shape->setter() == setter)
        return shape;

    





    Shape *newShape = putProperty(cx, shape->propid(), getter, setter, shape->maybeSlot(),
                                  attrs, shape->flags, shape->maybeShortid());

    checkShapeConsistency();
    return newShape;
}

bool
JSObject::removeProperty(JSContext *cx, jsid id)
{
    RootedVarObject self(cx, this);

    RootId idRoot(cx, &id);
    RootedVarShape shape(cx);

    Shape **spp;
    shape = Shape::search(cx, lastProperty(), id, &spp);
    if (!shape)
        return true;

    



    if (!self->inDictionaryMode() && (shape != self->lastProperty() || !self->canRemoveLastProperty())) {
        if (!self->toDictionaryMode(cx))
            return false;
        spp = self->lastProperty()->table().search(shape->propid(), false);
        shape = SHAPE_FETCH(spp);
    }

    






    Shape *spare = NULL;
    if (self->inDictionaryMode()) {
        spare = js_NewGCShape(cx);
        if (!spare)
            return false;
        new (spare) Shape(shape->base()->unowned(), 0);
        if (shape == lastProperty()) {
            





            Shape *previous = lastProperty()->parent;
            StackBaseShape base(lastProperty()->base());
            base.updateGetterSetter(previous->attrs, previous->getter(), previous->setter());
            BaseShape *nbase = BaseShape::getUnowned(cx, base);
            if (!nbase)
                return false;
            previous->base_ = nbase;
        }
    }

    
    if (shape->hasSlot()) {
        self->freeSlot(cx, shape->slot());
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    




    if (self->inDictionaryMode()) {
        PropertyTable &table = self->lastProperty()->table();

        if (SHAPE_HAD_COLLISION(*spp)) {
            *spp = SHAPE_REMOVED;
            ++table.removedCount;
            --table.entryCount;
        } else {
            *spp = NULL;
            --table.entryCount;

#ifdef DEBUG
            




            const Shape *aprop = self->lastProperty();
            for (int n = 50; --n >= 0 && aprop->parent; aprop = aprop->parent)
                JS_ASSERT_IF(aprop != shape, self->nativeContains(cx, *aprop));
#endif
        }

        
        Shape *oldLastProp = self->lastProperty();
        shape->removeFromDictionary(self);

        
        oldLastProp->handoffTableTo(self->lastProperty());

        
        JS_ALWAYS_TRUE(self->generateOwnShape(cx, spare));

        
        uint32_t size = table.capacity();
        if (size > PropertyTable::MIN_SIZE && table.entryCount <= size >> 2)
            (void) table.change(-1, cx);
    } else {
        





        JS_ASSERT(shape == self->lastProperty());
        self->removeLastProperty(cx);
    }

    self->checkShapeConsistency();
    return true;
}

void
JSObject::clear(JSContext *cx)
{
    Shape *shape = lastProperty();
    JS_ASSERT(inDictionaryMode() == shape->inDictionary());

    while (shape->parent) {
        shape = shape->parent;
        JS_ASSERT(inDictionaryMode() == shape->inDictionary());
    }
    JS_ASSERT(shape->isEmptyShape());

    if (inDictionaryMode())
        shape->listp = &shape_;

    JS_ALWAYS_TRUE(setLastProperty(cx, shape));

    JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    checkShapeConsistency();
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
    JS_ASSERT_IF(oldShape != lastProperty(),
                 inDictionaryMode() &&
                 nativeLookup(cx, oldShape->propidRef()) == oldShape);

    JSObject *self = this;

    if (!inDictionaryMode()) {
        RootObject selfRoot(cx, &self);
        RootShape newRoot(cx, &newShape);
        if (!toDictionaryMode(cx))
            return NULL;
        oldShape = lastProperty();
    }

    if (!newShape) {
        RootObject selfRoot(cx, &self);
        RootShape oldRoot(cx, &oldShape);
        newShape = js_NewGCShape(cx);
        if (!newShape)
            return NULL;
        new (newShape) Shape(oldShape->base()->unowned(), 0);
    }

    PropertyTable &table = self->lastProperty()->table();
    Shape **spp = oldShape->isEmptyShape()
                  ? NULL
                  : table.search(oldShape->propidRef(), false);

    



    StackShape nshape(oldShape);
    newShape->initDictionaryShape(nshape, self->numFixedSlots(), oldShape->listp);

    JS_ASSERT(newShape->parent == oldShape);
    oldShape->removeFromDictionary(self);

    if (newShape == lastProperty())
        oldShape->handoffTableTo(newShape);

    if (spp)
        SHAPE_STORE_PRESERVING_COLLISION(spp, newShape);
    return newShape;
}

Shape *
JSObject::methodShapeChange(JSContext *cx, const Shape &shape)
{
    JS_ASSERT(shape.isMethod());

    if (!inDictionaryMode() && !toDictionaryMode(cx))
        return NULL;

    Shape *spare = js_NewGCShape(cx);
    if (!spare)
        return NULL;
    new (spare) Shape(shape.base()->unowned(), 0);

#ifdef DEBUG
    JS_ASSERT(canHaveMethodBarrier());
    JS_ASSERT(!shape.setter());
    JS_ASSERT(!shape.hasShortID());
#endif

    




    Shape *result =
        putProperty(cx, shape.propid(), NULL, NULL, shape.slot(),
                    shape.attrs,
                    shape.getFlags() & ~Shape::METHOD,
                    0);
    if (!result)
        return NULL;

    if (result != lastProperty())
        JS_ALWAYS_TRUE(generateOwnShape(cx, spare));

    return result;
}

bool
JSObject::shadowingShapeChange(JSContext *cx, const Shape &shape)
{
    return generateOwnShape(cx);
}

bool
JSObject::clearParent(JSContext *cx)
{
    return setParent(cx, NULL);
}

bool
JSObject::setParent(JSContext *cx, JSObject *parent)
{
    if (parent && !parent->setDelegate(cx))
        return false;

    if (inDictionaryMode()) {
        StackBaseShape base(lastProperty());
        base.parent = parent;
        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return false;

        lastProperty()->base()->adoptUnowned(nbase);
        return true;
    }

    Shape *newShape = Shape::setObjectParent(cx, parent, getProto(), shape_);
    if (!newShape)
        return false;

    shape_ = newShape;
    return true;
}

 Shape *
Shape::setObjectParent(JSContext *cx, JSObject *parent, JSObject *proto, Shape *last)
{
    if (last->getObjectParent() == parent)
        return last;

    StackBaseShape base(last);
    base.parent = parent;

    return replaceLastProperty(cx, base, proto, last);
}

bool
JSObject::preventExtensions(JSContext *cx, js::AutoIdVector *props)
{
    JS_ASSERT(isExtensible());

    RootedVarObject self(cx, this);

    if (props) {
        if (js::FixOp fix = getOps()->fix) {
            bool success;
            if (!fix(cx, this, &success, props))
                return false;
            if (!success) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CHANGE_EXTENSIBILITY);
                return false;
            }
        } else {
            if (!js::GetPropertyNames(cx, this, JSITER_HIDDEN | JSITER_OWNONLY, props))
                return false;
        }
    }

    return self->setFlag(cx, BaseShape::NOT_EXTENSIBLE, GENERATE_SHAPE);
}

bool
JSObject::setFlag(JSContext *cx,  uint32_t flag_, GenerateShape generateShape)
{
    BaseShape::Flag flag = (BaseShape::Flag) flag_;

    if (lastProperty()->getObjectFlags() & flag)
        return true;

    RootedVarObject self(cx, this);

    if (inDictionaryMode()) {
        if (generateShape == GENERATE_SHAPE && !generateOwnShape(cx))
            return false;
        StackBaseShape base(self->lastProperty());
        base.flags |= flag;
        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return false;

        self->lastProperty()->base()->adoptUnowned(nbase);
        return true;
    }

    Shape *newShape = Shape::setObjectFlag(cx, flag, getProto(), lastProperty());
    if (!newShape)
        return false;

    self->shape_ = newShape;
    return true;
}

 Shape *
Shape::setObjectFlag(JSContext *cx, BaseShape::Flag flag, JSObject *proto, Shape *last)
{
    if (last->getObjectFlags() & flag)
        return last;

    StackBaseShape base(last);
    base.flags |= flag;

    return replaceLastProperty(cx, base, proto, last);
}

 inline HashNumber
StackBaseShape::hash(const StackBaseShape *base)
{
    JSDHashNumber hash = base->flags;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(base->clasp) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(base->parent) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ uintptr_t(base->rawGetter);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ uintptr_t(base->rawSetter);
    return hash;
}

 inline bool
StackBaseShape::match(UnownedBaseShape *key, const StackBaseShape *lookup)
{
    return key->flags == lookup->flags
        && key->clasp == lookup->clasp
        && key->parent == lookup->parent
        && key->rawGetter == lookup->rawGetter
        && key->rawSetter == lookup->rawSetter;
}


class RootStackBaseShape
{
    Root<const JSObject*> parentRoot;
    Maybe<RootObject> getterRoot;
    Maybe<RootObject> setterRoot;

  public:
    RootStackBaseShape(JSContext *cx, const StackBaseShape *base)
        : parentRoot(cx, &base->parent)
    {
        if (base->flags & BaseShape::HAS_GETTER_OBJECT)
            getterRoot.construct(cx, (JSObject **) &base->rawGetter);
        if (base->flags & BaseShape::HAS_SETTER_OBJECT)
            setterRoot.construct(cx, (JSObject **) &base->rawSetter);
    }
};

 UnownedBaseShape *
BaseShape::getUnowned(JSContext *cx, const StackBaseShape &base)
{
    BaseShapeSet &table = cx->compartment->baseShapes;

    if (!table.initialized() && !table.init())
        return NULL;

    BaseShapeSet::AddPtr p = table.lookupForAdd(&base);

    if (p)
        return *p;

    RootStackBaseShape root(cx, &base);

    BaseShape *nbase_ = js_NewGCBaseShape(cx);
    if (!nbase_)
        return NULL;
    new (nbase_) BaseShape(base);

    UnownedBaseShape *nbase = static_cast<UnownedBaseShape *>(nbase_);

    if (!table.relookupOrAdd(p, &base, nbase))
        return NULL;

    return nbase;
}

void
JSCompartment::sweepBaseShapeTable(JSContext *cx)
{
    if (baseShapes.initialized()) {
        for (BaseShapeSet::Enum e(baseShapes); !e.empty(); e.popFront()) {
            UnownedBaseShape *base = e.front();
            if (!base->isMarked())
                e.removeFront();
        }
    }
}

void
BaseShape::finalize(JSContext *cx, bool background)
{
    if (table_) {
        cx->delete_(table_);
        table_ = NULL;
    }
}

 Shape *
Shape::setExtensibleParents(JSContext *cx, Shape *shape)
{
    JS_ASSERT(!shape->inDictionary());

    StackBaseShape base(shape);
    base.flags |= BaseShape::EXTENSIBLE_PARENTS;

    
    return replaceLastProperty(cx, base, NULL, shape);
}

bool
Bindings::setExtensibleParents(JSContext *cx)
{
    if (!ensureShape(cx))
        return false;
    Shape *newShape = Shape::setExtensibleParents(cx, lastBinding);
    if (!newShape)
        return false;
    lastBinding = newShape;
    return true;
}

bool
Bindings::setParent(JSContext *cx, JSObject *obj)
{
    





    Bindings *self = this;
    CheckRoot root(cx, &self);

    RootObject rootObj(cx, &obj);

    if (!ensureShape(cx))
        return false;

    
    Shape *newShape = Shape::setObjectParent(cx, obj, NULL, self->lastBinding);
    if (!newShape)
        return false;
    self->lastBinding = newShape;
    return true;
}

 inline HashNumber
InitialShapeEntry::hash(const Lookup &lookup)
{
    JSDHashNumber hash = uintptr_t(lookup.clasp) >> 3;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(lookup.proto) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(lookup.parent) >> 3);
    return hash + lookup.nfixed;
}

 inline bool
InitialShapeEntry::match(const InitialShapeEntry &key, const Lookup &lookup)
{
    return lookup.clasp == key.shape->getObjectClass()
        && lookup.proto == key.proto
        && lookup.parent == key.shape->getObjectParent()
        && lookup.nfixed == key.shape->numFixedSlots()
        && lookup.baseFlags == key.shape->getObjectFlags();
}

 Shape *
EmptyShape::getInitialShape(JSContext *cx, Class *clasp, JSObject *proto, JSObject *parent,
                            AllocKind kind, uint32_t objectFlags)
{
    InitialShapeSet &table = cx->compartment->initialShapes;

    if (!table.initialized() && !table.init())
        return NULL;

    size_t nfixed = GetGCKindSlots(kind, clasp);
    InitialShapeEntry::Lookup lookup(clasp, proto, parent, nfixed, objectFlags);

    InitialShapeSet::AddPtr p = table.lookupForAdd(lookup);

    if (p)
        return p->shape;

    RootedVar<UnownedBaseShape*> nbase(cx);

    StackBaseShape base(clasp, parent, objectFlags);
    nbase = BaseShape::getUnowned(cx, base);
    if (!nbase)
        return NULL;

    Shape *shape = JS_PROPERTY_TREE(cx).newShape(cx);
    if (!shape)
        return NULL;
    new (shape) EmptyShape(nbase, nfixed);

    InitialShapeEntry entry;
    entry.shape = shape;
    entry.proto = proto;

    if (!table.relookupOrAdd(p, lookup, entry))
        return NULL;

    return shape;
}

void
NewObjectCache::invalidateEntriesForShape(JSContext *cx, Shape *shape, JSObject *proto)
{
    Class *clasp = shape->getObjectClass();

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    if (CanBeFinalizedInBackground(kind, clasp))
        kind = GetBackgroundAllocKind(kind);

    GlobalObject *global = &shape->getObjectParent()->global();
    types::TypeObject *type = proto->getNewType(cx);

    EntryIndex entry;
    if (lookupGlobal(clasp, global, kind, &entry))
        PodZero(&entries[entry]);
    if (!proto->isGlobal() && lookupProto(clasp, proto, kind, &entry))
        PodZero(&entries[entry]);
    if (lookupType(clasp, type, kind, &entry))
        PodZero(&entries[entry]);
}

 void
EmptyShape::insertInitialShape(JSContext *cx, Shape *shape, JSObject *proto)
{
    InitialShapeEntry::Lookup lookup(shape->getObjectClass(), proto, shape->getObjectParent(),
                                     shape->numFixedSlots(), shape->getObjectFlags());

    InitialShapeSet::Ptr p = cx->compartment->initialShapes.lookup(lookup);
    JS_ASSERT(p);

    InitialShapeEntry &entry = const_cast<InitialShapeEntry &>(*p);
    JS_ASSERT(entry.shape->isEmptyShape());

    
#ifdef DEBUG
    const Shape *nshape = shape;
    while (!nshape->isEmptyShape())
        nshape = nshape->previous();
    JS_ASSERT(nshape == entry.shape);
#endif

    entry.shape = shape;

    







    cx->compartment->newObjectCache.invalidateEntriesForShape(cx, shape, proto);
}

void
JSCompartment::sweepInitialShapeTable(JSContext *cx)
{
    if (initialShapes.initialized()) {
        for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
            const InitialShapeEntry &entry = e.front();
            if (!entry.shape->isMarked() || (entry.proto && !entry.proto->isMarked()))
                e.removeFront();
        }
    }
}
