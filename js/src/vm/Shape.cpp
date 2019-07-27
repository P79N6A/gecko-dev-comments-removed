







#include "vm/Shape-inl.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/PodOperations.h"

#include "jsatom.h"
#include "jscntxt.h"
#include "jshashutil.h"
#include "jsobj.h"

#include "js/HashTable.h"

#include "jscntxtinlines.h"
#include "jsobjinlines.h"

#include "gc/ForkJoinNursery-inl.h"
#include "vm/NativeObject-inl.h"
#include "vm/Runtime-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::CeilingLog2Size;
using mozilla::DebugOnly;
using mozilla::PodZero;
using mozilla::RotateLeft;

bool
ShapeTable::init(ThreadSafeContext *cx, Shape *lastProp)
{
    uint32_t sizeLog2 = CeilingLog2Size(entryCount);
    uint32_t size = JS_BIT(sizeLog2);
    if (entryCount >= size - (size >> 2))
        sizeLog2++;
    if (sizeLog2 < MIN_SIZE_LOG2)
        sizeLog2 = MIN_SIZE_LOG2;

    



    entries = cx->pod_calloc<Shape *>(JS_BIT(sizeLog2));
    if (!entries)
        return false;

    hashShift = HASH_BITS - sizeLog2;
    for (Shape::Range<NoGC> r(lastProp); !r.empty(); r.popFront()) {
        Shape &shape = r.front();
        MOZ_ASSERT(cx->isThreadLocal(&shape));
        Shape **spp = search(shape.propid(), true);

        



        if (!SHAPE_FETCH(spp))
            SHAPE_STORE_PRESERVING_COLLISION(spp, &shape);
    }
    return true;
}

void
Shape::removeFromDictionary(NativeObject *obj)
{
    MOZ_ASSERT(inDictionary());
    MOZ_ASSERT(obj->inDictionaryMode());
    MOZ_ASSERT(listp);

    MOZ_ASSERT(obj->shape_->inDictionary());
    MOZ_ASSERT(obj->shape_->listp == &obj->shape_);

    if (parent)
        parent->listp = listp;
    *listp = parent;
    listp = nullptr;
}

void
Shape::insertIntoDictionary(HeapPtrShape *dictp)
{
    
    
    MOZ_ASSERT(inDictionary());
    MOZ_ASSERT(!listp);

    MOZ_ASSERT_IF(*dictp, (*dictp)->inDictionary());
    MOZ_ASSERT_IF(*dictp, (*dictp)->listp == dictp);
    MOZ_ASSERT_IF(*dictp, compartment() == (*dictp)->compartment());

    setParent(dictp->get());
    if (parent)
        parent->listp = &parent;
    listp = (HeapPtrShape *) dictp;
    *dictp = this;
}

bool
Shape::makeOwnBaseShape(ThreadSafeContext *cx)
{
    MOZ_ASSERT(!base()->isOwned());
    MOZ_ASSERT(cx->isThreadLocal(this));
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
    MOZ_ASSERT(inDictionary() && shape->inDictionary());

    if (this == shape)
        return;

    MOZ_ASSERT(base()->isOwned() && !shape->base()->isOwned());

    BaseShape *nbase = base();

    MOZ_ASSERT_IF(shape->hasSlot(), nbase->slotSpan() > shape->slot());

    this->base_ = nbase->baseUnowned();
    nbase->adoptUnowned(shape->base()->toUnowned());

    shape->base_ = nbase;
}

 bool
Shape::hashify(ThreadSafeContext *cx, Shape *shape)
{
    MOZ_ASSERT(!shape->hasTable());

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

    MOZ_ASSERT(entries);
    MOZ_ASSERT(!JSID_IS_EMPTY(id));

    
    hash0 = HashId(id);
    hash1 = HASH1(hash0, hashShift);
    spp = entries + hash1;

    
    stored = *spp;
    if (SHAPE_IS_FREE(stored))
        return spp;

    
    shape = SHAPE_CLEAR_COLLISION(stored);
    if (shape && shape->propidRaw() == id)
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
        firstRemoved = nullptr;
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
        if (shape && shape->propidRaw() == id) {
            MOZ_ASSERT(collision_flag);
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

    MOZ_CRASH("Shape::search failed to find an expected entry.");
    return nullptr;
}

#ifdef JSGC_COMPACTING
void
ShapeTable::fixupAfterMovingGC()
{
    int log2 = HASH_BITS - hashShift;
    uint32_t size = JS_BIT(log2);
    for (HashNumber i = 0; i < size; i++) {
        Shape *shape = SHAPE_FETCH(&entries[i]);
        if (shape && IsForwarded(shape))
            SHAPE_STORE_PRESERVING_COLLISION(&entries[i], Forwarded(shape));
    }
}
#endif

bool
ShapeTable::change(int log2Delta, ThreadSafeContext *cx)
{
    MOZ_ASSERT(entries);

    


    int oldlog2 = HASH_BITS - hashShift;
    int newlog2 = oldlog2 + log2Delta;
    uint32_t oldsize = JS_BIT(oldlog2);
    uint32_t newsize = JS_BIT(newlog2);
    Shape **newTable = cx->pod_calloc<Shape *>(newsize);
    if (!newTable)
        return false;

    
    hashShift = HASH_BITS - newlog2;
    removedCount = 0;
    Shape **oldTable = entries;
    entries = newTable;

    
    for (Shape **oldspp = oldTable; oldsize != 0; oldspp++) {
        Shape *shape = SHAPE_FETCH(oldspp);
        MOZ_ASSERT(cx->isThreadLocal(shape));
        if (shape) {
            Shape **spp = search(shape->propid(), true);
            MOZ_ASSERT(SHAPE_IS_FREE(*spp));
            *spp = shape;
        }
        oldsize--;
    }

    
    js_free(oldTable);
    return true;
}

bool
ShapeTable::grow(ThreadSafeContext *cx)
{
    MOZ_ASSERT(needsToGrow());

    uint32_t size = capacity();
    int delta = removedCount < size >> 2;

    if (!change(delta, cx) && entryCount + removedCount == size - 1) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

 Shape *
Shape::replaceLastProperty(ExclusiveContext *cx, StackBaseShape &base,
                           TaggedProto proto, HandleShape shape)
{
    MOZ_ASSERT(!shape->inDictionary());

    if (!shape->parent) {
        
        AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
        return EmptyShape::getInitialShape(cx, base.clasp, proto,
                                           base.parent, base.metadata, kind,
                                           base.flags & BaseShape::OBJECT_FLAG_MASK);
    }

    UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
    if (!nbase)
        return nullptr;

    StackShape child(shape);
    child.base = nbase;

    return cx->compartment()->propertyTree.getChild(cx, shape->parent, child);
}






 Shape *
NativeObject::getChildPropertyOnDictionary(ThreadSafeContext *cx, HandleNativeObject obj,
                                           HandleShape parent, StackShape &child)
{
    




    if (!child.hasSlot()) {
        child.setSlot(parent->maybeSlot());
    } else {
        if (child.hasMissingSlot()) {
            uint32_t slot;
            if (!allocSlot(cx, obj, &slot))
                return nullptr;
            child.setSlot(slot);
        } else {
            








            MOZ_ASSERT(obj->inDictionaryMode() ||
                       parent->hasMissingSlot() ||
                       child.slot() == parent->maybeSlot() + 1 ||
                       (parent->maybeSlot() + 1 < JSSLOT_FREE(obj->getClass()) &&
                        child.slot() == JSSLOT_FREE(obj->getClass())));
        }
    }

    RootedShape shape(cx);

    if (obj->inDictionaryMode()) {
        MOZ_ASSERT(parent == obj->lastProperty());
        RootedGeneric<StackShape*> childRoot(cx, &child);
        shape = childRoot->isAccessorShape() ? NewGCAccessorShape(cx) : NewGCShape(cx);
        if (!shape)
            return nullptr;
        if (childRoot->hasSlot() && childRoot->slot() >= obj->lastProperty()->base()->slotSpan()) {
            if (!setSlotSpan(cx, obj, childRoot->slot() + 1))
                return nullptr;
        }
        shape->initDictionaryShape(*childRoot, obj->numFixedSlots(), &obj->shape_);
    }

    return shape;
}

 Shape *
NativeObject::getChildProperty(ExclusiveContext *cx,
                               HandleNativeObject obj, HandleShape parent, StackShape &unrootedChild)
{
    RootedGeneric<StackShape*> child(cx, &unrootedChild);
    RootedShape shape(cx, getChildPropertyOnDictionary(cx, obj, parent, *child));

    if (!obj->inDictionaryMode()) {
        shape = cx->compartment()->propertyTree.getChild(cx, parent, *child);
        if (!shape)
            return nullptr;
        
        
        if (!setLastProperty(cx, obj, shape))
            return nullptr;
    }

    return shape;
}

 Shape *
NativeObject::lookupChildProperty(ThreadSafeContext *cx,
                                  HandleNativeObject obj, HandleShape parent, StackShape &unrootedChild)
{
    RootedGeneric<StackShape*> child(cx, &unrootedChild);
    MOZ_ASSERT(cx->isThreadLocal(obj));

    RootedShape shape(cx, getChildPropertyOnDictionary(cx, obj, parent, *child));

    if (!obj->inDictionaryMode()) {
        shape = cx->compartment_->propertyTree.lookupChild(cx, parent, *child);
        if (!shape)
            return nullptr;
        if (!setLastProperty(cx, obj, shape))
            return nullptr;
    }

    return shape;
}

bool
js::NativeObject::toDictionaryMode(ThreadSafeContext *cx)
{
    MOZ_ASSERT(!inDictionaryMode());

    
    MOZ_ASSERT(cx->isInsideCurrentCompartment(this));

    




    MOZ_ASSERT(cx->isThreadLocal(this));

    uint32_t span = slotSpan();

    Rooted<NativeObject*> self(cx, this);

    
    
    
    RootedShape root(cx);
    RootedShape dictionaryShape(cx);

    RootedShape shape(cx, lastProperty());
    while (shape) {
        MOZ_ASSERT(!shape->inDictionary());

        Shape *dprop = shape->isAccessorShape() ? NewGCAccessorShape(cx) : NewGCShape(cx);
        if (!dprop) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        HeapPtrShape *listp = dictionaryShape ? &dictionaryShape->parent : nullptr;
        StackShape child(shape);
        dprop->initDictionaryShape(child, self->numFixedSlots(), listp);

        if (!dictionaryShape)
            root = dprop;

        MOZ_ASSERT(!dprop->hasTable());
        dictionaryShape = dprop;
        shape = shape->previous();
    }

    if (!Shape::hashify(cx, root)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    MOZ_ASSERT(root->listp == nullptr);
    root->listp = &self->shape_;
    self->shape_ = root;

    MOZ_ASSERT(self->inDictionaryMode());
    root->base()->setSlotSpan(span);

    return true;
}

 Shape *
NativeObject::addProperty(ExclusiveContext *cx, HandleNativeObject obj, HandleId id,
                          PropertyOp getter, StrictPropertyOp setter,
                          uint32_t slot, unsigned attrs,
                          unsigned flags, bool allowDictionary)
{
    MOZ_ASSERT(!JSID_IS_VOID(id));
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return nullptr;
    if (!extensible) {
        if (cx->isJSContext())
            obj->reportNotExtensible(cx->asJSContext());
        return nullptr;
    }

    Shape **spp = nullptr;
    if (obj->inDictionaryMode())
        spp = obj->lastProperty()->table().search(id, true);

    return addPropertyInternal<SequentialExecution>(cx, obj, id, getter, setter, slot, attrs,
                                                    flags, spp, allowDictionary);
}

static bool
ShouldConvertToDictionary(JSObject *obj)
{
    



    if (obj->hadElementsAccess())
        return obj->lastProperty()->entryCount() >= PropertyTree::MAX_HEIGHT_WITH_ELEMENTS_ACCESS;
    return obj->lastProperty()->entryCount() >= PropertyTree::MAX_HEIGHT;
}

template <ExecutionMode mode>
static inline UnownedBaseShape *
GetOrLookupUnownedBaseShape(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                            StackBaseShape &base)
{
    if (mode == ParallelExecution)
        return BaseShape::lookupUnowned(cx, base);
    return BaseShape::getUnowned(cx->asExclusiveContext(), base);
}

template <ExecutionMode mode>
 Shape *
NativeObject::addPropertyInternal(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                                  HandleNativeObject obj, HandleId id,
                                  PropertyOp getter, StrictPropertyOp setter,
                                  uint32_t slot, unsigned attrs,
                                  unsigned flags, Shape **spp,
                                  bool allowDictionary)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT_IF(!allowDictionary, !obj->inDictionaryMode());
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    




    ShapeTable *table = nullptr;
    if (!obj->inDictionaryMode()) {
        bool stableSlot =
            (slot == SHAPE_INVALID_SLOT) ||
            obj->lastProperty()->hasMissingSlot() ||
            (slot == obj->lastProperty()->maybeSlot() + 1);
        MOZ_ASSERT_IF(!allowDictionary, stableSlot);
        if (allowDictionary &&
            (!stableSlot || ShouldConvertToDictionary(obj)))
        {
            if (!obj->toDictionaryMode(cx))
                return nullptr;
            table = &obj->lastProperty()->table();
            spp = table->search(id, true);
        }
    } else {
        table = &obj->lastProperty()->table();
        if (table->needsToGrow()) {
            if (!table->grow(cx))
                return nullptr;
            spp = table->search(id, true);
            MOZ_ASSERT(!SHAPE_FETCH(spp));
        }
    }

    MOZ_ASSERT(!!table == !!spp);

    
    RootedShape shape(cx);
    {
        RootedShape last(cx, obj->lastProperty());

        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);

        Rooted<UnownedBaseShape*> nbase(cx);
        if (!indexed) {
            nbase = last->base()->unowned();
        } else {
            StackBaseShape base(last->base());
            base.flags |= BaseShape::INDEXED;
            nbase = GetOrLookupUnownedBaseShape<mode>(cx, base);
            if (!nbase)
                return nullptr;
        }

        StackShape child(nbase, id, slot, attrs, flags);
        child.updateGetterSetter(getter, setter);
        shape = getOrLookupChildProperty<mode>(cx, obj, last, child);
    }

    if (shape) {
        MOZ_ASSERT(shape == obj->lastProperty());

        if (table) {
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, static_cast<Shape *>(shape));
            ++table->entryCount;

            
            MOZ_ASSERT(&shape->parent->table() == table);
            shape->parent->handoffTableTo(shape);
        }

        obj->checkShapeConsistency();
        return shape;
    }

    obj->checkShapeConsistency();
    return nullptr;
}

template  Shape *
NativeObject::addPropertyInternal<SequentialExecution>(ExclusiveContext *cx,
                                                       HandleNativeObject obj, HandleId id,
                                                       PropertyOp getter, StrictPropertyOp setter,
                                                       uint32_t slot, unsigned attrs,
                                                       unsigned flags, Shape **spp,
                                                       bool allowDictionary);
template  Shape *
NativeObject::addPropertyInternal<ParallelExecution>(ForkJoinContext *cx,
                                                     HandleNativeObject obj, HandleId id,
                                                     PropertyOp getter, StrictPropertyOp setter,
                                                     uint32_t slot, unsigned attrs,
                                                     unsigned flags, Shape **spp,
                                                     bool allowDictionary);

JSObject *
js::NewReshapedObject(JSContext *cx, HandleTypeObject type, JSObject *parent,
                      gc::AllocKind allocKind, HandleShape shape, NewObjectKind newKind)
{
    RootedPlainObject res(cx, NewObjectWithType<PlainObject>(cx, type, parent, allocKind, newKind));
    if (!res)
        return nullptr;

    if (shape->isEmptyShape())
        return res;

    
    js::AutoIdVector ids(cx);
    {
        for (unsigned i = 0; i <= shape->slot(); i++) {
            if (!ids.append(JSID_VOID))
                return nullptr;
        }
        Shape *nshape = shape;
        while (!nshape->isEmptyShape()) {
            ids[nshape->slot()].set(nshape->propid());
            nshape = nshape->previous();
        }
    }

    
    RootedId id(cx);
    RootedShape newShape(cx, res->lastProperty());
    for (unsigned i = 0; i < ids.length(); i++) {
        id = ids[i];
        MOZ_ASSERT(!res->contains(cx, id));

        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);

        Rooted<UnownedBaseShape*> nbase(cx, newShape->base()->unowned());
        if (indexed) {
            StackBaseShape base(nbase);
            base.flags |= BaseShape::INDEXED;
            nbase = GetOrLookupUnownedBaseShape<SequentialExecution>(cx, base);
            if (!nbase)
                return nullptr;
        }

        StackShape child(nbase, id, i, JSPROP_ENUMERATE, 0);
        newShape = cx->compartment()->propertyTree.getChild(cx, newShape, child);
        if (!newShape)
            return nullptr;
        if (!NativeObject::setLastProperty(cx, res, newShape))
            return nullptr;
    }

    return res;
}






static inline bool
CheckCanChangeAttrs(ThreadSafeContext *cx, JSObject *obj, Shape *shape, unsigned *attrsp)
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

template <ExecutionMode mode>
 Shape *
NativeObject::putProperty(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                          HandleNativeObject obj, HandleId id,
                          PropertyOp getter, StrictPropertyOp setter,
                          uint32_t slot, unsigned attrs, unsigned flags)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(!JSID_IS_VOID(id));
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

#ifdef DEBUG
    if (obj->is<ArrayObject>()) {
        ArrayObject *arr = &obj->as<ArrayObject>();
        uint32_t index;
        if (js_IdIsIndex(id, &index))
            MOZ_ASSERT(index < arr->length() || arr->lengthIsWritable());
    }
#endif

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    









    Shape **spp;
    RootedShape shape(cx, (mode == ParallelExecution
                           ? Shape::searchThreadLocal(cx, obj->lastProperty(), id, &spp,
                                                      cx->isThreadLocal(obj->lastProperty()))
                           : Shape::search(cx->asExclusiveContext(), obj->lastProperty(), id,
                                           &spp, true)));
    if (!shape) {
        



        bool extensible;

        if (mode == ParallelExecution) {
            if (obj->is<ProxyObject>())
                return nullptr;
            extensible = obj->nonProxyIsExtensible();
        } else {
            if (!JSObject::isExtensible(cx->asExclusiveContext(), obj, &extensible))
                return nullptr;
        }

        if (!extensible) {
            if (cx->isJSContext())
                obj->reportNotExtensible(cx->asJSContext());
            return nullptr;
        }

        return addPropertyInternal<mode>(cx, obj, id, getter, setter, slot, attrs, flags,
                                         spp, true);
    }

    
    MOZ_ASSERT_IF(spp, !SHAPE_IS_REMOVED(*spp));

    if (!CheckCanChangeAttrs(cx, obj, shape, &attrs))
        return nullptr;

    




    bool hadSlot = shape->hasSlot();
    uint32_t oldSlot = shape->maybeSlot();
    if (!(attrs & JSPROP_SHARED) && slot == SHAPE_INVALID_SLOT && hadSlot)
        slot = oldSlot;

    Rooted<UnownedBaseShape*> nbase(cx);
    {
        uint32_t index;
        bool indexed = js_IdIsIndex(id, &index);
        StackBaseShape base(obj->lastProperty()->base());
        if (indexed)
            base.flags |= BaseShape::INDEXED;
        nbase = GetOrLookupUnownedBaseShape<mode>(cx, base);
        if (!nbase)
            return nullptr;
    }

    



    if (shape->matchesParamsAfterId(nbase, slot, attrs, flags, getter, setter))
        return shape;

    




    if (shape != obj->lastProperty() && !obj->inDictionaryMode()) {
        if (!obj->toDictionaryMode(cx))
            return nullptr;
        spp = obj->lastProperty()->table().search(shape->propid(), false);
        shape = SHAPE_FETCH(spp);
    }

    MOZ_ASSERT_IF(shape->hasSlot() && !(attrs & JSPROP_SHARED), shape->slot() == slot);

    if (obj->inDictionaryMode()) {
        





        bool updateLast = (shape == obj->lastProperty());
        bool accessorShape = getter || setter || (attrs & (JSPROP_GETTER | JSPROP_SETTER));
        shape = obj->replaceWithNewEquivalentShape(cx, shape, nullptr, accessorShape);
        if (!shape)
            return nullptr;
        if (!updateLast && !obj->generateOwnShape(cx))
            return nullptr;

        
        if (slot == SHAPE_INVALID_SLOT && !(attrs & JSPROP_SHARED)) {
            if (!allocSlot(cx, obj, &slot))
                return nullptr;
        }

        if (updateLast)
            shape->base()->adoptUnowned(nbase);
        else
            shape->base_ = nbase;

        MOZ_ASSERT_IF(attrs & (JSPROP_GETTER | JSPROP_SETTER), attrs & JSPROP_SHARED);

        shape->setSlot(slot);
        shape->attrs = uint8_t(attrs);
        shape->flags = flags | Shape::IN_DICTIONARY | (accessorShape ? Shape::ACCESSOR_SHAPE : 0);
        if (shape->isAccessorShape()) {
            AccessorShape &accShape = shape->asAccessorShape();
            accShape.rawGetter = getter;
            if (accShape.hasGetterObject())
                GetterSetterWriteBarrierPost(&accShape, &accShape.getterObj);
            accShape.rawSetter = setter;
            if (accShape.hasSetterObject())
                GetterSetterWriteBarrierPost(&accShape, &accShape.setterObj);
        } else {
            MOZ_ASSERT(!getter);
            MOZ_ASSERT(!setter);
        }
    } else {
        



        StackBaseShape base(obj->lastProperty()->base());

        UnownedBaseShape *nbase = GetOrLookupUnownedBaseShape<mode>(cx, base);
        if (!nbase)
            return nullptr;

        MOZ_ASSERT(shape == obj->lastProperty());

        
        StackShape child(nbase, id, slot, attrs, flags);
        child.updateGetterSetter(getter, setter);
        RootedShape parent(cx, shape->parent);
        Shape *newShape = getOrLookupChildProperty<mode>(cx, obj, parent, child);

        if (!newShape) {
            obj->checkShapeConsistency();
            return nullptr;
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

template  Shape *
NativeObject::putProperty<SequentialExecution>(ExclusiveContext *cx,
                                               HandleNativeObject obj, HandleId id,
                                               PropertyOp getter, StrictPropertyOp setter,
                                               uint32_t slot, unsigned attrs,
                                               unsigned flags);
template  Shape *
NativeObject::putProperty<ParallelExecution>(ForkJoinContext *cx,
                                             HandleNativeObject obj, HandleId id,
                                             PropertyOp getter, StrictPropertyOp setter,
                                             uint32_t slot, unsigned attrs,
                                             unsigned flags);

template <ExecutionMode mode>
 Shape *
NativeObject::changeProperty(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                             HandleNativeObject obj, HandleShape shape, unsigned attrs,
                             unsigned mask, PropertyOp getter, StrictPropertyOp setter)
{
    MOZ_ASSERT(cx->isThreadLocal(obj));
    MOZ_ASSERT(obj->containsPure(shape));
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    attrs |= shape->attrs & mask;
    MOZ_ASSERT_IF(attrs & (JSPROP_GETTER | JSPROP_SETTER), attrs & JSPROP_SHARED);

    
    MOZ_ASSERT(!((attrs ^ shape->attrs) & JSPROP_SHARED) ||
               !(attrs & JSPROP_SHARED));

    if (mode == ParallelExecution) {
        if (!types::IsTypePropertyIdMarkedNonData(obj, shape->propid()))
            return nullptr;
    } else {
        types::MarkTypePropertyNonData(cx->asExclusiveContext(), obj, shape->propid());
    }

    if (!CheckCanChangeAttrs(cx, obj, shape, &attrs))
        return nullptr;

    if (shape->attrs == attrs && shape->getter() == getter && shape->setter() == setter)
        return shape;

    





    RootedId propid(cx, shape->propid());
    Shape *newShape = putProperty<mode>(cx, obj, propid, getter, setter,
                                        shape->maybeSlot(), attrs, shape->flags);

    obj->checkShapeConsistency();
    return newShape;
}

template  Shape *
NativeObject::changeProperty<SequentialExecution>(ExclusiveContext *cx,
                                                  HandleNativeObject obj, HandleShape shape,
                                                  unsigned attrs, unsigned mask,
                                                  PropertyOp getter, StrictPropertyOp setter);
template  Shape *
NativeObject::changeProperty<ParallelExecution>(ForkJoinContext *cx,
                                                HandleNativeObject obj, HandleShape shape,
                                                unsigned attrs, unsigned mask,
                                                PropertyOp getter, StrictPropertyOp setter);

bool
NativeObject::removeProperty(ExclusiveContext *cx, jsid id_)
{
    RootedId id(cx, id_);
    RootedNativeObject self(cx, this);

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
        
        spare = NewGCAccessorShape(cx);
        if (!spare)
            return false;
        new (spare) Shape(shape->base()->unowned(), 0);
        if (shape == self->lastProperty()) {
            





            RootedShape previous(cx, self->lastProperty()->parent);
            StackBaseShape base(self->lastProperty()->base());
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
            *spp = nullptr;
            --table.entryCount;

#ifdef DEBUG
            




            Shape *aprop = self->lastProperty();
            for (int n = 50; --n >= 0 && aprop->parent; aprop = aprop->parent)
                MOZ_ASSERT_IF(aprop != shape, self->contains(cx, aprop));
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
        





        MOZ_ASSERT(shape == self->lastProperty());
        self->removeLastProperty(cx);
    }

    self->checkShapeConsistency();
    return true;
}

 void
NativeObject::clear(JSContext *cx, HandleNativeObject obj)
{
    RootedShape shape(cx, obj->lastProperty());
    MOZ_ASSERT(obj->inDictionaryMode() == shape->inDictionary());

    while (shape->parent) {
        shape = shape->parent;
        MOZ_ASSERT(obj->inDictionaryMode() == shape->inDictionary());
    }
    MOZ_ASSERT(shape->isEmptyShape());

    if (obj->inDictionaryMode())
        shape->listp = &obj->shape_;

    JS_ALWAYS_TRUE(setLastProperty(cx, obj, shape));

    ++cx->runtime()->propertyRemovals;
    obj->checkShapeConsistency();
}

 bool
NativeObject::rollbackProperties(ExclusiveContext *cx, HandleNativeObject obj, uint32_t slotSpan)
{
    




    MOZ_ASSERT(!obj->inDictionaryMode() && slotSpan <= obj->slotSpan());
    while (true) {
        if (obj->lastProperty()->isEmptyShape()) {
            MOZ_ASSERT(slotSpan == 0);
            break;
        } else {
            uint32_t slot = obj->lastProperty()->slot();
            if (slot < slotSpan)
                break;
            MOZ_ASSERT(obj->getSlot(slot).isUndefined());
        }
        if (!obj->removeProperty(cx, obj->lastProperty()->propid()))
            return false;
    }

    return true;
}

Shape *
NativeObject::replaceWithNewEquivalentShape(ThreadSafeContext *cx, Shape *oldShape, Shape *newShape,
                                            bool accessorShape)
{
    MOZ_ASSERT(cx->isThreadLocal(this));
    MOZ_ASSERT(cx->isThreadLocal(oldShape));
    MOZ_ASSERT(cx->isInsideCurrentCompartment(oldShape));
    MOZ_ASSERT_IF(oldShape != lastProperty(),
                  inDictionaryMode() &&
                  ((cx->isExclusiveContext()
                    ? lookup(cx->asExclusiveContext(), oldShape->propidRef())
                    : lookupPure(oldShape->propidRef())) == oldShape));

    NativeObject *self = this;

    if (!inDictionaryMode()) {
        RootedNativeObject selfRoot(cx, self);
        RootedShape newRoot(cx, newShape);
        if (!toDictionaryMode(cx))
            return nullptr;
        oldShape = selfRoot->lastProperty();
        self = selfRoot;
        newShape = newRoot;
    }

    if (!newShape) {
        RootedNativeObject selfRoot(cx, self);
        RootedShape oldRoot(cx, oldShape);
        newShape = (oldShape->isAccessorShape() || accessorShape)
                   ? NewGCAccessorShape(cx)
                   : NewGCShape(cx);
        if (!newShape)
            return nullptr;
        new (newShape) Shape(oldRoot->base()->unowned(), 0);
        self = selfRoot;
        oldShape = oldRoot;
    }

    ShapeTable &table = self->lastProperty()->table();
    Shape **spp = oldShape->isEmptyShape()
                  ? nullptr
                  : table.search(oldShape->propidRef(), false);

    



    StackShape nshape(oldShape);
    newShape->initDictionaryShape(nshape, self->numFixedSlots(), oldShape->listp);

    MOZ_ASSERT(newShape->parent == oldShape);
    oldShape->removeFromDictionary(self);

    if (newShape == self->lastProperty())
        oldShape->handoffTableTo(newShape);

    if (spp)
        SHAPE_STORE_PRESERVING_COLLISION(spp, newShape);
    return newShape;
}

bool
NativeObject::shadowingShapeChange(ExclusiveContext *cx, const Shape &shape)
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

    if (obj->isNative() && obj->as<NativeObject>().inDictionaryMode()) {
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
    if (obj->isNative() && obj->as<NativeObject>().inDictionaryMode()) {
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
JSObject::preventExtensions(JSContext *cx, HandleObject obj, bool *succeeded)
{
    if (obj->is<ProxyObject>())
        return js::Proxy::preventExtensions(cx, obj, succeeded);

    if (!obj->nonProxyIsExtensible()) {
        *succeeded = true;
        return true;
    }

    



    AutoIdVector props(cx);
    if (!js::GetPropertyKeys(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    





    if (obj->isNative() && !NativeObject::sparsifyDenseElements(cx, obj.as<NativeObject>()))
        return false;

    *succeeded = true;
    return obj->setFlag(cx, BaseShape::NOT_EXTENSIBLE, GENERATE_SHAPE);
}

bool
JSObject::setFlag(ExclusiveContext *cx,  uint32_t flag_,
                  GenerateShape generateShape)
{
    BaseShape::Flag flag = (BaseShape::Flag) flag_;

    if (lastProperty()->getObjectFlags() & flag)
        return true;

    RootedObject self(cx, this);

    if (isNative() && as<NativeObject>().inDictionaryMode()) {
        if (generateShape == GENERATE_SHAPE && !as<NativeObject>().generateOwnShape(cx))
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
NativeObject::clearFlag(ExclusiveContext *cx, BaseShape::Flag flag)
{
    MOZ_ASSERT(inDictionaryMode());
    MOZ_ASSERT(lastProperty()->getObjectFlags() & flag);

    RootedObject self(cx, this);

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
    hash = RotateLeft(hash, 4) ^ (uintptr_t(base->clasp) >> 3);
    hash = RotateLeft(hash, 4) ^ (uintptr_t(base->parent) >> 3);
    hash = RotateLeft(hash, 4) ^ (uintptr_t(base->metadata) >> 3);
    return hash;
}

 inline bool
StackBaseShape::match(UnownedBaseShape *key, const StackBaseShape *lookup)
{
    return key->flags == lookup->flags
        && key->clasp_ == lookup->clasp
        && key->parent == lookup->parent
        && key->metadata == lookup->metadata;
}

void
StackBaseShape::trace(JSTracer *trc)
{
    if (parent)
        gc::MarkObjectRoot(trc, (JSObject**)&parent, "StackBaseShape parent");

    if (metadata)
        gc::MarkObjectRoot(trc, (JSObject**)&metadata, "StackBaseShape metadata");
}

 UnownedBaseShape*
BaseShape::getUnowned(ExclusiveContext *cx, StackBaseShape &base)
{
    BaseShapeSet &table = cx->compartment()->baseShapes;

    if (!table.initialized() && !table.init())
        return nullptr;

    DependentAddPtr<BaseShapeSet> p(cx, table, &base);
    if (p)
        return *p;

    RootedGeneric<StackBaseShape*> root(cx, &base);

    BaseShape *nbase_ = js_NewGCBaseShape<CanGC>(cx);
    if (!nbase_)
        return nullptr;

    new (nbase_) BaseShape(*root);

    UnownedBaseShape *nbase = static_cast<UnownedBaseShape *>(nbase_);

    if (!p.add(cx, table, root, nbase))
        return nullptr;

    return nbase;
}

 UnownedBaseShape *
BaseShape::lookupUnowned(ThreadSafeContext *cx, const StackBaseShape &base)
{
    BaseShapeSet &table = cx->compartment_->baseShapes;

    if (!table.initialized())
        return nullptr;

    BaseShapeSet::Ptr p = table.readonlyThreadsafeLookup(&base);
    return *p;
}

void
BaseShape::assertConsistency()
{
#ifdef DEBUG
    if (isOwned()) {
        UnownedBaseShape *unowned = baseUnowned();
        MOZ_ASSERT(getObjectParent() == unowned->getObjectParent());
        MOZ_ASSERT(getObjectMetadata() == unowned->getObjectMetadata());
        MOZ_ASSERT(getObjectFlags() == unowned->getObjectFlags());
    }
#endif
}

void
JSCompartment::sweepBaseShapeTable()
{
    if (baseShapes.initialized()) {
        for (BaseShapeSet::Enum e(baseShapes); !e.empty(); e.popFront()) {
            UnownedBaseShape *base = e.front().unbarrieredGet();
            if (IsBaseShapeAboutToBeFinalizedFromAnyThread(&base)) {
                e.removeFront();
            } else if (base != e.front().unbarrieredGet()) {
                StackBaseShape sbase(base);
                ReadBarriered<UnownedBaseShape *> b(base);
                e.rekeyFront(&sbase, b);
            }
        }
    }
}

void
BaseShape::finalize(FreeOp *fop)
{
    if (table_) {
        fop->delete_(table_);
        table_ = nullptr;
    }
}

inline
InitialShapeEntry::InitialShapeEntry() : shape(nullptr), proto(nullptr)
{
}

inline
InitialShapeEntry::InitialShapeEntry(const ReadBarrieredShape &shape, TaggedProto proto)
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
    hash = RotateLeft(hash, 4) ^
        (uintptr_t(lookup.hashProto.toWord()) >> 3);
    hash = RotateLeft(hash, 4) ^
        (uintptr_t(lookup.hashParent) >> 3) ^
        (uintptr_t(lookup.hashMetadata) >> 3);
    return hash + lookup.nfixed;
}

 inline bool
InitialShapeEntry::match(const InitialShapeEntry &key, const Lookup &lookup)
{
    const Shape *shape = *key.shape.unsafeGet();
    return lookup.clasp == shape->getObjectClass()
        && lookup.matchProto.toWord() == key.proto.toWord()
        && lookup.matchParent == shape->getObjectParent()
        && lookup.matchMetadata == shape->getObjectMetadata()
        && lookup.nfixed == shape->numFixedSlots()
        && lookup.baseFlags == shape->getObjectFlags();
}





class InitialShapeSetRef : public BufferableRef
{
    InitialShapeSet *set;
    const Class *clasp;
    TaggedProto proto;
    JSObject *parent;
    JSObject *metadata;
    size_t nfixed;
    uint32_t objectFlags;

  public:
    InitialShapeSetRef(InitialShapeSet *set,
                       const Class *clasp,
                       TaggedProto proto,
                       JSObject *parent,
                       JSObject *metadata,
                       size_t nfixed,
                       uint32_t objectFlags)
        : set(set),
          clasp(clasp),
          proto(proto),
          parent(parent),
          metadata(metadata),
          nfixed(nfixed),
          objectFlags(objectFlags)
    {}

    void mark(JSTracer *trc) {
        TaggedProto priorProto = proto;
        JSObject *priorParent = parent;
        JSObject *priorMetadata = metadata;
        if (proto.isObject())
            Mark(trc, reinterpret_cast<JSObject**>(&proto), "initialShapes set proto");
        if (parent)
            Mark(trc, &parent, "initialShapes set parent");
        if (metadata)
            Mark(trc, &metadata, "initialShapes set metadata");
        if (proto == priorProto && parent == priorParent && metadata == priorMetadata)
            return;

        
        InitialShapeEntry::Lookup lookup(clasp, priorProto,
                                         priorParent, parent,
                                         priorMetadata, metadata,
                                         nfixed, objectFlags);
        InitialShapeSet::Ptr p = set->lookup(lookup);
        MOZ_ASSERT(p);

        
        InitialShapeEntry &entry = const_cast<InitialShapeEntry&>(*p);
        entry.proto = proto;
        lookup.matchProto = proto;

        
        set->rekeyAs(lookup,
                     InitialShapeEntry::Lookup(clasp, proto, parent, metadata, nfixed, objectFlags),
                     *p);
    }
};

#ifdef JSGC_HASH_TABLE_CHECKS

void
JSCompartment::checkInitialShapesTableAfterMovingGC()
{
    if (!initialShapes.initialized())
        return;

    




    for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
        InitialShapeEntry entry = e.front();
        TaggedProto proto = entry.proto;
        Shape *shape = entry.shape.get();

        if (proto.isObject())
            CheckGCThingAfterMovingGC(proto.toObject());
        if (shape->getObjectParent())
            CheckGCThingAfterMovingGC(shape->getObjectParent());
        if (shape->getObjectMetadata())
            CheckGCThingAfterMovingGC(shape->getObjectMetadata());

        InitialShapeEntry::Lookup lookup(shape->getObjectClass(),
                                         proto,
                                         shape->getObjectParent(),
                                         shape->getObjectMetadata(),
                                         shape->numFixedSlots(),
                                         shape->getObjectFlags());
        InitialShapeSet::Ptr ptr = initialShapes.lookup(lookup);
        MOZ_ASSERT(ptr.found() && &*ptr == &e.front());
    }
}

#endif 

Shape *
EmptyShape::new_(ExclusiveContext *cx, Handle<UnownedBaseShape *> base, uint32_t nfixed)
{
    Shape *shape = NewGCShape(cx);
    if (!shape) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    new (shape) EmptyShape(base, nfixed);
    return shape;
}

 Shape *
EmptyShape::getInitialShape(ExclusiveContext *cx, const Class *clasp, TaggedProto proto,
                            JSObject *parent, JSObject *metadata,
                            size_t nfixed, uint32_t objectFlags)
{
    MOZ_ASSERT_IF(proto.isObject(), cx->isInsideCurrentCompartment(proto.toObject()));
    MOZ_ASSERT_IF(parent, cx->isInsideCurrentCompartment(parent));

    InitialShapeSet &table = cx->compartment()->initialShapes;

    if (!table.initialized() && !table.init())
        return nullptr;

    typedef InitialShapeEntry::Lookup Lookup;
    DependentAddPtr<InitialShapeSet>
        p(cx, table, Lookup(clasp, proto, parent, metadata, nfixed, objectFlags));
    if (p)
        return p->shape;

    Rooted<TaggedProto> protoRoot(cx, proto);
    RootedObject parentRoot(cx, parent);
    RootedObject metadataRoot(cx, metadata);

    StackBaseShape base(cx, clasp, parent, metadata, objectFlags);
    Rooted<UnownedBaseShape*> nbase(cx, BaseShape::getUnowned(cx, base));
    if (!nbase)
        return nullptr;

    Shape *shape = EmptyShape::new_(cx, nbase, nfixed);
    if (!shape)
        return nullptr;

    Lookup lookup(clasp, protoRoot, parentRoot, metadataRoot, nfixed, objectFlags);
    if (!p.add(cx, table, lookup, InitialShapeEntry(ReadBarrieredShape(shape), protoRoot)))
        return nullptr;

    
    if (cx->isJSContext()) {
        if ((protoRoot.isObject() && IsInsideNursery(protoRoot.toObject())) ||
            IsInsideNursery(parentRoot.get()) ||
            IsInsideNursery(metadataRoot.get()))
        {
            InitialShapeSetRef ref(
                &table, clasp, protoRoot, parentRoot, metadataRoot, nfixed, objectFlags);
            cx->asJSContext()->runtime()->gc.storeBuffer.putGeneric(ref);
        }
    }

    return shape;
}

 Shape *
EmptyShape::getInitialShape(ExclusiveContext *cx, const Class *clasp, TaggedProto proto,
                            JSObject *parent, JSObject *metadata,
                            AllocKind kind, uint32_t objectFlags)
{
    return getInitialShape(cx, clasp, proto, parent, metadata, GetGCKindSlots(kind, clasp), objectFlags);
}

void
NewObjectCache::invalidateEntriesForShape(JSContext *cx, HandleShape shape, HandleObject proto)
{
    const Class *clasp = shape->getObjectClass();

    gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());
    if (CanBeFinalizedInBackground(kind, clasp))
        kind = GetBackgroundAllocKind(kind);

    Rooted<GlobalObject *> global(cx, &shape->getObjectParent()->global());
    Rooted<types::TypeObject *> type(cx, cx->getNewType(clasp, TaggedProto(proto)));

    EntryIndex entry;
    if (lookupGlobal(clasp, global, kind, &entry))
        PodZero(&entries[entry]);
    if (!proto->is<GlobalObject>() && lookupProto(clasp, proto, kind, &entry))
        PodZero(&entries[entry]);
    if (lookupType(type, kind, &entry))
        PodZero(&entries[entry]);
}

 void
EmptyShape::insertInitialShape(ExclusiveContext *cx, HandleShape shape, HandleObject proto)
{
    InitialShapeEntry::Lookup lookup(shape->getObjectClass(), TaggedProto(proto),
                                     shape->getObjectParent(), shape->getObjectMetadata(),
                                     shape->numFixedSlots(), shape->getObjectFlags());

    InitialShapeSet::Ptr p = cx->compartment()->initialShapes.lookup(lookup);
    MOZ_ASSERT(p);

    InitialShapeEntry &entry = const_cast<InitialShapeEntry &>(*p);

    
#ifdef DEBUG
    Shape *nshape = shape;
    while (!nshape->isEmptyShape())
        nshape = nshape->previous();
    MOZ_ASSERT(nshape == entry.shape);
#endif

    entry.shape = ReadBarrieredShape(shape);

    









    if (cx->isJSContext()) {
        JSContext *ncx = cx->asJSContext();
        ncx->runtime()->newObjectCache.invalidateEntriesForShape(ncx, shape, proto);
    }
}

void
JSCompartment::sweepInitialShapeTable()
{
    if (initialShapes.initialized()) {
        for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
            const InitialShapeEntry &entry = e.front();
            Shape *shape = entry.shape.unbarrieredGet();
            JSObject *proto = entry.proto.raw();
            if (IsShapeAboutToBeFinalizedFromAnyThread(&shape) ||
                (entry.proto.isObject() && IsObjectAboutToBeFinalizedFromAnyThread(&proto)))
            {
                e.removeFront();
            } else {
#ifdef DEBUG
                DebugOnly<JSObject *> parent = shape->getObjectParent();
                MOZ_ASSERT(!parent || !IsObjectAboutToBeFinalizedFromAnyThread(&parent));
                MOZ_ASSERT(parent == shape->getObjectParent());
#endif
                if (shape != entry.shape.unbarrieredGet() || proto != entry.proto.raw()) {
                    ReadBarrieredShape readBarrieredShape(shape);
                    InitialShapeEntry newKey(readBarrieredShape, TaggedProto(proto));
                    e.rekeyFront(newKey.getLookup(), newKey);
                }
            }
        }
    }
}

#ifdef JSGC_COMPACTING
void
JSCompartment::fixupInitialShapeTable()
{
    if (!initialShapes.initialized())
        return;

    for (InitialShapeSet::Enum e(initialShapes); !e.empty(); e.popFront()) {
        InitialShapeEntry entry = e.front();
        bool needRekey = false;
        if (IsForwarded(entry.shape.get())) {
            entry.shape.set(Forwarded(entry.shape.get()));
            needRekey = true;
        }
        if (entry.proto.isObject() && IsForwarded(entry.proto.toObject())) {
            entry.proto = TaggedProto(Forwarded(entry.proto.toObject()));
            needRekey = true;
        }
        JSObject *parent = entry.shape->getObjectParent();
        if (parent) {
            parent = MaybeForwarded(parent);
            needRekey = true;
        }
        JSObject *metadata = entry.shape->getObjectMetadata();
        if (metadata) {
            metadata = MaybeForwarded(metadata);
            needRekey = true;
        }
        if (needRekey) {
            InitialShapeEntry::Lookup relookup(entry.shape->getObjectClass(),
                                               entry.proto,
                                               parent,
                                               metadata,
                                               entry.shape->numFixedSlots(),
                                               entry.shape->getObjectFlags());
            e.rekeyFront(relookup, entry);
        }
    }
}
#endif 

void
AutoRooterGetterSetter::Inner::trace(JSTracer *trc)
{
    if ((attrs & JSPROP_GETTER) && *pgetter)
        gc::MarkObjectRoot(trc, (JSObject**) pgetter, "AutoRooterGetterSetter getter");
    if ((attrs & JSPROP_SETTER) && *psetter)
        gc::MarkObjectRoot(trc, (JSObject**) psetter, "AutoRooterGetterSetter setter");
}
