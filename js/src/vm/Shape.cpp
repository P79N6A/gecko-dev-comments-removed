







#include "vm/Shape-inl.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/PodOperations.h"

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsobj.h"

#include "js/HashTable.h"

#include "jscntxtinlines.h"
#include "jsobjinlines.h"

#include "vm/Runtime-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;
using mozilla::PodZero;
using mozilla::CeilingLog2Size;

bool
ShapeTable::init(ExclusiveContext *cx, Shape *lastProp)
{
    






    uint32_t sizeLog2 = CeilingLog2Size(2 * entryCount);
    if (sizeLog2 < MIN_SIZE_LOG2)
        sizeLog2 = MIN_SIZE_LOG2;

    



    entries = (Shape **) cx->calloc_(sizeOfEntries(JS_BIT(sizeLog2)));
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
Shape::makeOwnBaseShape(ExclusiveContext *cx)
{
    JS_ASSERT(!base()->isOwned());
    assertSameCompartmentDebugOnly(cx, compartment());

    BaseShape *nbase = js_NewGCBaseShape<NoGC>(cx);
    if (!nbase)
        return false;

    new (nbase) BaseShape(StackBaseShape(this));
    nbase->setOwned(base()->toUnowned());

    this->base_ = nbase;

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
Shape::hashify(ExclusiveContext *cx, Shape *shape)
{
    JS_ASSERT(!shape->hasTable());

    if (!shape->ensureOwnBaseShape(cx))
        return false;

    ShapeTable *table = cx->new_<ShapeTable>(shape->entryCount());
    if (!table)
        return false;

    if (!table->init(cx, shape)) {
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
ShapeTable::change(int log2Delta, ExclusiveContext *cx)
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
ShapeTable::grow(ExclusiveContext *cx)
{
    JS_ASSERT(needsToGrow());

    uint32_t size = capacity();
    int delta = removedCount < size >> 2;

    if (!change(delta, cx) && entryCount + removedCount == size - 1) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

Shape *
Shape::getChildBinding(ExclusiveContext *cx, const StackShape &child)
{
    JS_ASSERT(!inDictionary());

    
    uint32_t slots = child.slotSpan();
    gc::AllocKind kind = gc::GetGCObjectKind(slots);
    uint32_t nfixed = gc::GetGCKindSlots(kind);

    return cx->propertyTree().getChild(cx, this, nfixed, child);
}

 Shape *
Shape::replaceLastProperty(ExclusiveContext *cx, const StackBaseShape &base,
                           TaggedProto proto, HandleShape shape)
{
    JS_ASSERT(!shape->inDictionary());

    if (!shape->parent) {
        
        AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
        return EmptyShape::getInitialShape(cx, base.clasp, proto,
                                           base.parent, base.metadata, kind,
                                           base.flags & BaseShape::OBJECT_FLAG_MASK);
    }

    StackShape child(shape);
    StackShape::AutoRooter childRoot(cx, &child);
    {
        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;

        child.base = nbase;
    }

    return cx->propertyTree().getChild(cx, shape->parent, shape->numFixedSlots(), child);
}






 Shape *
JSObject::getChildProperty(ExclusiveContext *cx,
                           HandleObject obj, HandleShape parent, StackShape &child)
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
js::ObjectImpl::toDictionaryMode(ExclusiveContext *cx)
{
    JS_ASSERT(!inDictionaryMode());

    
    JS_ASSERT(cx->isInsideCurrentCompartment(this));

    uint32_t span = slotSpan();

    Rooted<ObjectImpl*> self(cx, this);

    





    RootedShape root(cx);
    RootedShape dictionaryShape(cx);

    RootedShape shape(cx, lastProperty());
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

 Shape *
JSObject::addProperty(ExclusiveContext *cx, HandleObject obj, HandleId id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32_t slot, unsigned attrs,
                      unsigned flags, int shortid, bool allowDictionary)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return NULL;
    if (!extensible) {
        if (cx->isJSContext())
            obj->reportNotExtensible(cx->asJSContext());
        return NULL;
    }

    NormalizeGetterAndSetter(obj, id, attrs, flags, getter, setter);

    Shape **spp = NULL;
    if (obj->inDictionaryMode())
        spp = obj->lastProperty()->table().search(id, true);

    return addPropertyInternal(cx, obj, id, getter, setter, slot, attrs, flags, shortid,
                               spp, allowDictionary);
}

static bool
ShouldConvertToDictionary(JSObject *obj)
{
    



    if (obj->hadElementsAccess())
        return obj->lastProperty()->entryCount() >= PropertyTree::MAX_HEIGHT_WITH_ELEMENTS_ACCESS;
    return obj->lastProperty()->entryCount() >= PropertyTree::MAX_HEIGHT;
}

 Shape *
JSObject::addPropertyInternal(ExclusiveContext *cx, HandleObject obj, HandleId id,
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
            (!stableSlot || ShouldConvertToDictionary(obj)))
        {
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
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, static_cast<Shape *>(shape));
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
CheckCanChangeAttrs(ExclusiveContext *cx, JSObject *obj, Shape *shape, unsigned *attrsp)
{
    if (shape->configurable())
        return true;

    
    *attrsp |= JSPROP_PERMANENT;

    
    if (shape->isDataDescriptor() && shape->hasSlot() &&
        (*attrsp & (JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED)))
    {
        if (cx->isJSContext())
            obj->reportNotConfigurable(cx->asJSContext(), shape->propid());
        return false;
    }

    return true;
}

 Shape *
JSObject::putProperty(ExclusiveContext *cx, HandleObject obj, HandleId id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32_t slot, unsigned attrs,
                      unsigned flags, int shortid)
{
    JS_ASSERT(!JSID_IS_VOID(id));

#ifdef DEBUG
    if (obj->is<ArrayObject>()) {
        ArrayObject *arr = &obj->as<ArrayObject>();
        uint32_t index;
        if (js_IdIsIndex(id, &index))
            JS_ASSERT(index < arr->length() || arr->lengthIsWritable());
    }
#endif

    NormalizeGetterAndSetter(obj, id, attrs, flags, getter, setter);

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    
    Shape **spp;
    RootedShape shape(cx, Shape::search(cx, obj->lastProperty(), id, &spp, true));
    if (!shape) {
        



        bool extensible;
        if (!JSObject::isExtensible(cx, obj, &extensible))
            return NULL;
        if (!extensible) {
            if (cx->isJSContext())
                obj->reportNotExtensible(cx->asJSContext());
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

        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return NULL;

        JS_ASSERT(shape == obj->lastProperty());

        
        StackShape child(nbase, id, slot, obj->numFixedSlots(), attrs, flags, shortid);
        RootedShape parent(cx, shape->parent);
        Shape *newShape = JSObject::getChildProperty(cx, obj, parent, child);

        if (!newShape) {
            obj->checkShapeConsistency();
            return NULL;
        }

        shape = newShape;
    }

    





    if (hadSlot && !shape->hasSlot()) {
        if (oldSlot < obj->slotSpan())
            obj->freeSlot(oldSlot);
        
        if (cx->isJSContext())
            ++cx->asJSContext()->runtime()->propertyRemovals;
    }

    obj->checkShapeConsistency();

    return shape;
}

 Shape *
JSObject::changeProperty(ExclusiveContext *cx, HandleObject obj, HandleShape shape, unsigned attrs,
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
    Shape *newShape = putProperty(cx, obj, propid, getter, setter, shape->maybeSlot(),
                                    attrs, shape->flags, shape->maybeShortid());

    obj->checkShapeConsistency();
    return newShape;
}

bool
JSObject::removeProperty(ExclusiveContext *cx, jsid id_)
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
            BaseShape *nbase = BaseShape::getUnowned(cx, base);
            if (!nbase)
                return false;
            previous->base_ = nbase;
        }
    }

    
    if (shape->hasSlot()) {
        self->freeSlot(shape->slot());
        if (cx->isJSContext())
            ++cx->asJSContext()->runtime()->propertyRemovals;
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
            




            Shape *aprop = self->lastProperty();
            for (int n = 50; --n >= 0 && aprop->parent; aprop = aprop->parent)
                JS_ASSERT_IF(aprop != shape, self->nativeContains(cx, aprop));
#endif
        }

        {
            
            Shape *oldLastProp = self->lastProperty();
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

    ++cx->runtime()->propertyRemovals;
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
ObjectImpl::replaceWithNewEquivalentShape(ExclusiveContext *cx, Shape *oldShape, Shape *newShape)
{
    JS_ASSERT(cx->isInsideCurrentCompartment(oldShape));
    JS_ASSERT_IF(oldShape != lastProperty(),
                 inDictionaryMode() &&
                 nativeLookup(cx, oldShape->propidRef()) == oldShape);

    ObjectImpl *self = this;

    if (!inDictionaryMode()) {
        Rooted<ObjectImpl*> selfRoot(cx, self);
        RootedShape newRoot(cx, newShape);
        if (!toDictionaryMode(cx))
            return NULL;
        oldShape = selfRoot->lastProperty();
        self = selfRoot;
        newShape = newRoot;
    }

    if (!newShape) {
        Rooted<ObjectImpl*> selfRoot(cx, self);
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
JSObject::shadowingShapeChange(ExclusiveContext *cx, const Shape &shape)
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
        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return false;

        obj->lastProperty()->base()->adoptUnowned(nbase);
        return true;
    }

    Shape *newShape = Shape::setObjectParent(cx, parent, obj->getTaggedProto(), obj->shape_);
    if (!newShape)
        return false;

    obj->shape_ = newShape;
    return true;
}

 Shape *
Shape::setObjectParent(ExclusiveContext *cx, JSObject *parent, TaggedProto proto, Shape *last)
{
    if (last->getObjectParent() == parent)
        return last;

    StackBaseShape base(last);
    base.parent = parent;

    RootedShape lastRoot(cx, last);
    return replaceLastProperty(cx, base, proto, lastRoot);
}

 bool
JSObject::setMetadata(JSContext *cx, HandleObject obj, HandleObject metadata)
{
    if (obj->inDictionaryMode()) {
        StackBaseShape base(obj->lastProperty());
        base.metadata = metadata;
        UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
        if (!nbase)
            return false;

        obj->lastProperty()->base()->adoptUnowned(nbase);
        return true;
    }

    Shape *newShape = Shape::setObjectMetadata(cx, metadata, obj->getTaggedProto(), obj->shape_);
    if (!newShape)
        return false;

    obj->shape_ = newShape;
    return true;
}

 Shape *
Shape::setObjectMetadata(JSContext *cx, JSObject *metadata, TaggedProto proto, Shape *last)
{
    if (last->getObjectMetadata() == metadata)
        return last;

    StackBaseShape base(last);
    base.metadata = metadata;

    RootedShape lastRoot(cx, last);
    return replaceLastProperty(cx, base, proto, lastRoot);
}

 bool
js::ObjectImpl::preventExtensions(JSContext *cx, Handle<ObjectImpl*> obj)
{
#ifdef DEBUG
    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return false;
    MOZ_ASSERT(extensible,
               "Callers must ensure |obj| is extensible before calling "
               "preventExtensions");
#endif

    if (Downcast(obj)->is<ProxyObject>()) {
        RootedObject object(cx, obj->asObjectPtr());
        return js::Proxy::preventExtensions(cx, object);
    }

    RootedObject self(cx, obj->asObjectPtr());

    



    AutoIdVector props(cx);
    if (!js::GetPropertyNames(cx, self, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    





    if (self->isNative() && !JSObject::sparsifyDenseElements(cx, self))
        return false;

    return self->setFlag(cx, BaseShape::NOT_EXTENSIBLE, GENERATE_SHAPE);
}

bool
js::ObjectImpl::setFlag(ExclusiveContext *cx,  uint32_t flag_,
                        GenerateShape generateShape)
{
    BaseShape::Flag flag = (BaseShape::Flag) flag_;

    if (lastProperty()->getObjectFlags() & flag)
        return true;

    Rooted<ObjectImpl*> self(cx, this);

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

    Shape *newShape =
        Shape::setObjectFlag(cx, flag, self->getTaggedProto(), self->lastProperty());
    if (!newShape)
        return false;

    self->shape_ = newShape;
    return true;
}

bool
js::ObjectImpl::clearFlag(ExclusiveContext *cx,  uint32_t flag)
{
    JS_ASSERT(inDictionaryMode());
    JS_ASSERT(lastProperty()->getObjectFlags() & flag);

    RootedObject self(cx, this->asObjectPtr());

    StackBaseShape base(self->lastProperty());
    base.flags &= ~flag;
    UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
    if (!nbase)
        return false;

    self->lastProperty()->base()->adoptUnowned(nbase);
    return true;
}

 Shape *
Shape::setObjectFlag(ExclusiveContext *cx, BaseShape::Flag flag, TaggedProto proto, Shape *last)
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
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(base->metadata) >> 3);
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
        && key->metadata == lookup->metadata
        && key->rawGetter == lookup->rawGetter
        && key->rawSetter == lookup->rawSetter;
}

void
StackBaseShape::AutoRooter::trace(JSTracer *trc)
{
    if (base->parent) {
        gc::MarkObjectRoot(trc, (JSObject**)&base->parent,
                           "StackBaseShape::AutoRooter parent");
    }
    if (base->metadata) {
        gc::MarkObjectRoot(trc, (JSObject**)&base->metadata,
                           "StackBaseShape::AutoRooter metadata");
    }
    if ((base->flags & BaseShape::HAS_GETTER_OBJECT) && base->rawGetter) {
        gc::MarkObjectRoot(trc, (JSObject**)&base->rawGetter,
                           "StackBaseShape::AutoRooter getter");
    }
    if ((base->flags & BaseShape::HAS_SETTER_OBJECT) && base->rawSetter) {
        gc::MarkObjectRoot(trc, (JSObject**)&base->rawSetter,
                           "StackBaseShape::AutoRooter setter");
    }
}

 UnownedBaseShape*
BaseShape::getUnowned(ExclusiveContext *cx, const StackBaseShape &base)
{
    BaseShapeSet &table = cx->baseShapes();

    if (!table.initialized() && !table.init())
        return NULL;

    BaseShapeSet::AddPtr p = table.lookupForAdd(&base);

    if (p)
        return *p;

    StackBaseShape::AutoRooter root(cx, &base);

    BaseShape *nbase_ = js_NewGCBaseShape<CanGC>(cx);
    if (!nbase_)
        return NULL;

    new (nbase_) BaseShape(base);

    UnownedBaseShape *nbase = static_cast<UnownedBaseShape *>(nbase_);

    if (!table.relookupOrAdd(p, &base, nbase))
        return NULL;

    return nbase;
}

void
JSCompartment::sweepBaseShapeTable()
{
    gcstats::AutoPhase ap(runtimeFromMainThread()->gcStats,
                          gcstats::PHASE_SWEEP_TABLES_BASE_SHAPE);

    if (baseShapes.initialized()) {
        for (BaseShapeSet::Enum e(baseShapes); !e.empty(); e.popFront()) {
            UnownedBaseShape *base = e.front();
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
    return Lookup(shape->getObjectClass(), proto, shape->getObjectParent(), shape->getObjectMetadata(),
                  shape->numFixedSlots(), shape->getObjectFlags());
}

 inline HashNumber
InitialShapeEntry::hash(const Lookup &lookup)
{
    HashNumber hash = uintptr_t(lookup.clasp) >> 3;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(lookup.proto.toWord()) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (uintptr_t(lookup.parent) >> 3) ^ (uintptr_t(lookup.metadata) >> 3);
    return hash + lookup.nfixed;
}

 inline bool
InitialShapeEntry::match(const InitialShapeEntry &key, const Lookup &lookup)
{
    const Shape *shape = *key.shape.unsafeGet();
    return lookup.clasp == shape->getObjectClass()
        && lookup.proto.toWord() == key.proto.toWord()
        && lookup.parent == shape->getObjectParent()
        && lookup.metadata == shape->getObjectMetadata()
        && lookup.nfixed == shape->numFixedSlots()
        && lookup.baseFlags == shape->getObjectFlags();
}

 Shape *
EmptyShape::getInitialShape(ExclusiveContext *cx, Class *clasp, TaggedProto proto,
                            JSObject *parent, JSObject *metadata,
                            size_t nfixed, uint32_t objectFlags)
{
    JS_ASSERT_IF(proto.isObject(), cx->isInsideCurrentCompartment(proto.toObject()));
    JS_ASSERT_IF(parent, cx->isInsideCurrentCompartment(parent));

    InitialShapeSet &table = cx->initialShapes();

    if (!table.initialized() && !table.init())
        return NULL;

    typedef InitialShapeEntry::Lookup Lookup;
    InitialShapeSet::AddPtr p =
        table.lookupForAdd(Lookup(clasp, proto, parent, metadata, nfixed, objectFlags));

    if (p)
        return p->shape;

    SkipRoot skip(cx, &p); 
    Rooted<TaggedProto> protoRoot(cx, proto);
    RootedObject parentRoot(cx, parent);
    RootedObject metadataRoot(cx, metadata);

    StackBaseShape base(cx, clasp, parent, metadata, objectFlags);
    Rooted<UnownedBaseShape*> nbase(cx, BaseShape::getUnowned(cx, base));
    if (!nbase)
        return NULL;

    Shape *shape = cx->propertyTree().newShape(cx);
    if (!shape)
        return NULL;
    new (shape) EmptyShape(nbase, nfixed);

    if (!table.relookupOrAdd(p, Lookup(clasp, protoRoot, parentRoot, metadataRoot, nfixed, objectFlags),
                             InitialShapeEntry(shape, protoRoot)))
    {
        return NULL;
    }

    return shape;
}

 Shape *
EmptyShape::getInitialShape(ExclusiveContext *cx, Class *clasp, TaggedProto proto,
                            JSObject *parent, JSObject *metadata,
                            AllocKind kind, uint32_t objectFlags)
{
    return getInitialShape(cx, clasp, proto, parent, metadata, GetGCKindSlots(kind, clasp), objectFlags);
}

void
NewObjectCache::invalidateEntriesForShape(JSContext *cx, HandleShape shape, HandleObject proto)
{
    Class *clasp = shape->getObjectClass();

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    if (CanBeFinalizedInBackground(kind, clasp))
        kind = GetBackgroundAllocKind(kind);

    Rooted<GlobalObject *> global(cx, &shape->getObjectParent()->global());
    Rooted<types::TypeObject *> type(cx, cx->getNewType(clasp, proto.get()));

    EntryIndex entry;
    if (lookupGlobal(clasp, global, kind, &entry))
        PodZero(&entries[entry]);
    if (!proto->is<GlobalObject>() && lookupProto(clasp, proto, kind, &entry))
        PodZero(&entries[entry]);
    if (lookupType(clasp, type, kind, &entry))
        PodZero(&entries[entry]);
}

 void
EmptyShape::insertInitialShape(ExclusiveContext *cx, HandleShape shape, HandleObject proto)
{
    InitialShapeEntry::Lookup lookup(shape->getObjectClass(), TaggedProto(proto),
                                     shape->getObjectParent(), shape->getObjectMetadata(),
                                     shape->numFixedSlots(), shape->getObjectFlags());

    InitialShapeSet::Ptr p = cx->initialShapes().lookup(lookup);
    JS_ASSERT(p);

    InitialShapeEntry &entry = const_cast<InitialShapeEntry &>(*p);

    
#ifdef DEBUG
    Shape *nshape = shape;
    while (!nshape->isEmptyShape())
        nshape = nshape->previous();
    JS_ASSERT(nshape == entry.shape);
#endif

    entry.shape = shape.get();

    









    if (cx->isJSContext()) {
        JSContext *ncx = cx->asJSContext();
        ncx->runtime()->newObjectCache.invalidateEntriesForShape(ncx, shape, proto);
    }
}





void
JSCompartment::markAllInitialShapeTableEntries(JSTracer *trc)
{
    if (!initialShapes.initialized())
        return;

    for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
        if (!e.front().proto.isObject())
            continue;
        JSObject *proto = e.front().proto.toObject();
        JS_SET_TRACING_LOCATION(trc, (void*)&e.front().proto);
        MarkObjectRoot(trc, &proto, "InitialShapeSet proto");
        if (proto != e.front().proto.toObject()) {
            InitialShapeEntry moved = e.front();
            moved.proto = TaggedProto(proto);
            e.rekeyFront(e.front().getLookup(), moved);
        }
    }
}

void
JSCompartment::sweepInitialShapeTable()
{
    gcstats::AutoPhase ap(runtimeFromMainThread()->gcStats,
                          gcstats::PHASE_SWEEP_TABLES_INITIAL_SHAPE);

    if (initialShapes.initialized()) {
        for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
            const InitialShapeEntry &entry = e.front();
            Shape *shape = entry.shape;
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

void
AutoRooterGetterSetter::Inner::trace(JSTracer *trc)
{
    if ((attrs & JSPROP_GETTER) && *pgetter)
        gc::MarkObjectRoot(trc, (JSObject**) pgetter, "AutoRooterGetterSetter getter");
    if ((attrs & JSPROP_SETTER) && *psetter)
        gc::MarkObjectRoot(trc, (JSObject**) psetter, "AutoRooterGetterSetter setter");
}
