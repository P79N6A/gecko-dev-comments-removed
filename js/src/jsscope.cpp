










































#include <new>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
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
#include "jstracer.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;
using namespace js::gc;

bool
PropertyTable::init(JSRuntime *rt, Shape *lastProp)
{
    






    uint32 sizeLog2 = JS_CeilingLog2(2 * entryCount);
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

    BaseShape *nbase = js_NewGCBaseShape(cx);
    if (!nbase)
        return false;

    new (nbase) BaseShape(*base());
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
Shape::hashify(JSContext *cx)
{
    JS_ASSERT(!hasTable());

    if (!ensureOwnBaseShape(cx))
        return false;

    JSRuntime *rt = cx->runtime;
    PropertyTable *table = rt->new_<PropertyTable>(entryCount());
    if (!table)
        return false;

    if (!table->init(rt, this)) {
        rt->free_(table);
        return false;
    }

    base()->setTable(table);
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
    uint32 sizeMask;

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
    jsuword collision_flag = SHAPE_COLLISION;
#endif

    
    if (SHAPE_IS_REMOVED(stored)) {
        firstRemoved = spp;
    } else {
        firstRemoved = NULL;
        if (adding && !SHAPE_HAD_COLLISION(stored))
            SHAPE_FLAG_COLLISION(spp, shape);
#ifdef DEBUG
        collision_flag &= jsuword(*spp) & SHAPE_COLLISION;
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
            collision_flag &= jsuword(*spp) & SHAPE_COLLISION;
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
    uint32 oldsize = JS_BIT(oldlog2);
    uint32 newsize = JS_BIT(newlog2);
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

    uint32 size = capacity();
    int delta = removedCount < size >> 2;

    if (!change(delta, cx) && entryCount + removedCount == size - 1) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

Shape *
Shape::getChildBinding(JSContext *cx, const js::Shape &child, Shape **lastBinding)
{
    JS_ASSERT(!inDictionary());
    JS_ASSERT(!child.inDictionary());

    Shape *shape = JS_PROPERTY_TREE(cx).getChild(cx, this, child);
    if (shape) {
        JS_ASSERT(shape->parent == this);
        JS_ASSERT(this == *lastBinding);
        *lastBinding = shape;
    }
    return shape;
}

 bool
Shape::replaceLastProperty(JSContext *cx, const js::Shape &child, Shape **lastp)
{
    Shape *shape = *lastp;

    JS_ASSERT(!child.inDictionary());
    JS_ASSERT(!shape->inDictionary());

    Shape *newShape;
    if (shape->parent) {
        newShape = JS_PROPERTY_TREE(cx).getChild(cx, shape->parent, child);
        if (!newShape)
            return false;
    } else {
        newShape = js_NewGCShape(cx);
        if (!newShape)
            return false;
        new (newShape) Shape(&child);
    }

    *lastp = newShape;
    return true;
}






Shape *
JSObject::getChildProperty(JSContext *cx, Shape *parent, Shape &child)
{
    




    if (!child.hasSlot()) {
        child.slot_ = parent->maybeSlot();
    } else {
        if (child.hasMissingSlot()) {
            uint32 slot;
            if (!allocSlot(cx, &slot))
                return NULL;
            child.slot_ = slot;
        } else {
            
            JS_ASSERT(inDictionaryMode() ||
                      parent->hasMissingSlot() ||
                      child.slot() == parent->slot() + 1);
        }
    }

    Shape *shape;

    if (inDictionaryMode()) {
        JS_ASSERT(parent == lastProperty());
        shape = js_NewGCShape(cx);
        if (!shape)
            return NULL;
        if (child.hasSlot() && child.slot() >= lastProperty()->base()->slotSpan()) {
            if (!setSlotSpan(cx, child.slot() + 1))
                return NULL;
        }
        shape->initDictionaryShape(child, &shape_);
    } else {
        shape = JS_PROPERTY_TREE(cx).getChild(cx, parent, child);
        if (!shape)
            return NULL;
        JS_ASSERT(shape->parent == parent);
        JS_ASSERT_IF(parent != lastProperty(), parent == lastProperty()->parent);
        if (!setLastProperty(cx, shape))
            return NULL;
    }

    updateFlags(shape);
    return shape;
}

Shape *
Shape::newDictionaryList(JSContext *cx, Shape **listp)
{
    Shape *shape = *listp;
    Shape *list = shape;

    




    Shape *root = NULL;
    Shape **childp = &root;

    while (shape) {
        JS_ASSERT(!shape->inDictionary());

        Shape *dprop = js_NewGCShape(cx);
        if (!dprop) {
            *listp = list;
            return NULL;
        }
        dprop->initDictionaryShape(*shape, childp);

        JS_ASSERT(!dprop->hasTable());
        childp = &dprop->parent;
        shape = shape->parent;
    }

    *listp = root;
    root->listp = listp;

    JS_ASSERT(root->inDictionary());
    root->hashify(cx);
    return root;
}

bool
JSObject::toDictionaryMode(JSContext *cx)
{
    JS_ASSERT(!inDictionaryMode());

    
    JS_ASSERT(compartment() == cx->compartment);

    uint32 span = slotSpan();

    





    Shape *last = lastProperty();
    if (!Shape::newDictionaryList(cx, &last))
        return false;

    JS_ASSERT(last->listp == &last);
    last->listp = &shape_;
    shape_ = last;

    JS_ASSERT(lastProperty()->hasTable());
    lastProperty()->base()->setSlotSpan(span);

    return true;
}





static inline bool
NormalizeGetterAndSetter(JSContext *cx, JSObject *obj,
                         jsid id, uintN attrs, uintN flags,
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

#ifdef DEBUG
# define CHECK_SHAPE_CONSISTENCY(obj) obj->checkShapeConsistency()

void
JSObject::checkShapeConsistency()
{
    static int throttle = -1;
    if (throttle < 0) {
        if (const char *var = getenv("JS_CHECK_SHAPE_THROTTLE"))
            throttle = atoi(var);
        if (throttle < 0)
            throttle = 0;
    }
    if (throttle == 0)
        return;

    JS_ASSERT(isNative());

    Shape *shape = lastProperty();
    Shape *prev = NULL;

    if (inDictionaryMode()) {
        JS_ASSERT(shape->hasTable());

        PropertyTable &table = shape->table();
        for (uint32 fslot = table.freelist; fslot != SHAPE_INVALID_SLOT;
             fslot = getSlot(fslot).toPrivateUint32()) {
            JS_ASSERT(fslot < slotSpan());
        }

        for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
            JS_ASSERT_IF(shape != lastProperty(), !shape->hasTable());

            Shape **spp = table.search(shape->propid(), false);
            JS_ASSERT(SHAPE_FETCH(spp) == shape);
        }

        shape = lastProperty();
        for (int n = throttle; --n >= 0 && shape; shape = shape->parent) {
            JS_ASSERT_IF(shape->slot() != SHAPE_INVALID_SLOT, shape->slot() < slotSpan());
            if (!prev) {
                JS_ASSERT(shape == lastProperty());
                JS_ASSERT(shape->listp == &shape_);
            } else {
                JS_ASSERT(shape->listp == &prev->parent);
            }
            prev = shape;
        }
    } else {
        for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
            if (shape->hasTable()) {
                PropertyTable &table = shape->table();
                JS_ASSERT(shape->parent);
                for (Shape::Range r(shape); !r.empty(); r.popFront()) {
                    Shape **spp = table.search(r.front().propid(), false);
                    JS_ASSERT(SHAPE_FETCH(spp) == &r.front());
                }
            }
            if (prev) {
                JS_ASSERT(prev->maybeSlot() >= shape->maybeSlot());
                shape->kids.checkConsistency(prev);
            }
            prev = shape;
        }
    }
}
#else
# define CHECK_SHAPE_CONSISTENCY(obj) ((void)0)
#endif

const Shape *
JSObject::addProperty(JSContext *cx, jsid id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32 slot, uintN attrs,
                      uintN flags, intN shortid, bool allowDictionary)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    if (!isExtensible()) {
        reportNotExtensible(cx);
        return NULL;
    }

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    Shape **spp = nativeSearch(cx, id, true);
    JS_ASSERT(!SHAPE_FETCH(spp));
    return addPropertyInternal(cx, id, getter, setter, slot, attrs, flags, shortid, spp, allowDictionary);
}

const Shape *
JSObject::addPropertyInternal(JSContext *cx, jsid id,
                              PropertyOp getter, StrictPropertyOp setter,
                              uint32 slot, uintN attrs,
                              uintN flags, intN shortid,
                              Shape **spp, bool allowDictionary)
{
    JS_ASSERT_IF(!allowDictionary, !inDictionaryMode());

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
            spp = nativeSearch(cx, id, true);
            table = &lastProperty()->table();
        }
    } else if (lastProperty()->hasTable()) {
        table = &lastProperty()->table();
        if (table->needsToGrow()) {
            if (!table->grow(cx))
                return NULL;

            spp = table->search(id, true);
            JS_ASSERT(!SHAPE_FETCH(spp));
        }
    }

    
    Shape *shape;
    {
        BaseShape base(getClass(), getParentMaybeScope(), attrs, getter, setter);
        BaseShape *nbase = BaseShape::lookup(cx, base);
        if (!nbase)
            return NULL;

        Shape child(nbase, id, slot, attrs, flags, shortid);
        shape = getChildProperty(cx, lastProperty(), child);
    }

    if (shape) {
        JS_ASSERT(shape == lastProperty());

        if (table) {
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, shape);
            ++table->entryCount;

            
            JS_ASSERT(&shape->parent->table() == table);
            shape->parent->handoffTableTo(shape);
        }

        CHECK_SHAPE_CONSISTENCY(this);
        return shape;
    }

    CHECK_SHAPE_CONSISTENCY(this);
    return NULL;
}






inline bool
CheckCanChangeAttrs(JSContext *cx, JSObject *obj, const Shape *shape, uintN *attrsp)
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

const Shape *
JSObject::putProperty(JSContext *cx, jsid id,
                      PropertyOp getter, StrictPropertyOp setter,
                      uint32 slot, uintN attrs,
                      uintN flags, intN shortid)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    Shape **spp = nativeSearch(cx, id, true);
    Shape *shape = SHAPE_FETCH(spp);
    if (!shape) {
        



        if (!isExtensible()) {
            reportNotExtensible(cx);
            return NULL;
        }

        return addPropertyInternal(cx, id, getter, setter, slot, attrs, flags, shortid, spp, true);
    }

    
    JS_ASSERT(!SHAPE_IS_REMOVED(*spp));

    if (!CheckCanChangeAttrs(cx, this, shape, &attrs))
        return NULL;
    
    




    bool hadSlot = shape->hasSlot();
    uint32 oldSlot = shape->maybeSlot();
    if (!(attrs & JSPROP_SHARED) && slot == SHAPE_INVALID_SLOT && hadSlot)
        slot = oldSlot;

    UnownedBaseShape *nbase;
    {
        BaseShape base(getClass(), getParentMaybeScope(), attrs, getter, setter);
        nbase = BaseShape::lookup(cx, base);
        if (!nbase)
            return NULL;
    }

    



    if (shape->matchesParamsAfterId(nbase, slot, attrs, flags, shortid))
        return shape;

    




    if (shape != lastProperty() && !inDictionaryMode()) {
        if (!toDictionaryMode(cx))
            return NULL;
        spp = nativeSearch(cx, shape->propid());
        shape = SHAPE_FETCH(spp);
    }

    JS_ASSERT_IF(shape->hasSlot() && !(attrs & JSPROP_SHARED), shape->slot() == slot);

    









    if (inDictionaryMode()) {
        bool updateLast = (shape == lastProperty());
        if (!generateOwnShape(cx))
            return NULL;
        if (updateLast)
            shape = lastProperty();

        
        if (slot == SHAPE_INVALID_SLOT && !(attrs & JSPROP_SHARED)) {
            if (!allocSlot(cx, &slot))
                return NULL;
        }

        if (shape == lastProperty())
            shape->base()->adoptUnowned(nbase);
        else
            shape->base_ = nbase;

        shape->slot_ = slot;
        shape->attrs = uint8(attrs);
        shape->flags = flags | Shape::IN_DICTIONARY;
        shape->shortid_ = int16(shortid);

        





        updateFlags(shape);
    } else {
        








        BaseShape base(getClass(), getParentMaybeScope(), attrs, getter, setter);
        BaseShape *nbase = BaseShape::lookup(cx, base);
        if (!nbase)
            return NULL;

        JS_ASSERT(shape == lastProperty());

        
        Shape child(nbase, id, slot, attrs, flags, shortid);
        Shape *newShape = getChildProperty(cx, shape->parent, child);

        if (!newShape) {
            CHECK_SHAPE_CONSISTENCY(this);
            return NULL;
        }

        shape = newShape;
    }

    





    if (hadSlot && !shape->hasSlot()) {
        if (oldSlot < slotSpan())
            freeSlot(cx, oldSlot);
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    CHECK_SHAPE_CONSISTENCY(this);

    return shape;
}

const Shape *
JSObject::changeProperty(JSContext *cx, const Shape *shape, uintN attrs, uintN mask,
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

    





    const Shape *newShape = putProperty(cx, shape->propid(), getter, setter, shape->maybeSlot(),
                                        attrs, shape->flags, shape->maybeShortid());

    CHECK_SHAPE_CONSISTENCY(this);
    return newShape;
}

bool
JSObject::removeProperty(JSContext *cx, jsid id)
{
    Shape **spp = nativeSearch(cx, id);
    Shape *shape = SHAPE_FETCH(spp);
    if (!shape)
        return true;

    



    if (!inDictionaryMode() && (shape != lastProperty() || !canRemoveLastProperty())) {
        if (!toDictionaryMode(cx))
            return false;
        spp = nativeSearch(cx, shape->propid());
        shape = SHAPE_FETCH(spp);
    }

    






    Shape *spare = NULL;
    if (inDictionaryMode()) {
        spare = js_NewGCShape(cx);
        if (!spare)
            return false;
        PodZero(spare);
    }

    
    if (shape->hasSlot()) {
        freeSlot(cx, shape->slot());
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    




    if (inDictionaryMode()) {
        PropertyTable &table = lastProperty()->table();

        if (SHAPE_HAD_COLLISION(*spp)) {
            *spp = SHAPE_REMOVED;
            ++table.removedCount;
            --table.entryCount;
        } else {
            *spp = NULL;
            --table.entryCount;

#ifdef DEBUG
            




            const Shape *aprop = lastProperty();
            for (int n = 50; --n >= 0 && aprop->parent; aprop = aprop->parent)
                JS_ASSERT_IF(aprop != shape, nativeContains(cx, *aprop));
#endif
        }

        
        Shape *oldLastProp = lastProperty();
        shape->removeFromDictionary(this);

        
        oldLastProp->handoffTableTo(lastProperty());

        
        JS_ALWAYS_TRUE(generateOwnShape(cx, spare));

        
        uint32 size = table.capacity();
        if (size > PropertyTable::MIN_SIZE && table.entryCount <= size >> 2)
            (void) table.change(-1, cx);
    } else {
        





        JS_ASSERT(shape == lastProperty());
        removeLastProperty(cx);
    }

    CHECK_SHAPE_CONSISTENCY(this);
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

    LeaveTraceIfGlobalObject(cx, this);
    JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    CHECK_SHAPE_CONSISTENCY(this);
}

void
JSObject::rollbackProperties(JSContext *cx, uint32 slotSpan)
{
    




    JS_ASSERT(!inDictionaryMode() && slotSpan <= this->slotSpan());
    while (this->slotSpan() != slotSpan) {
        JS_ASSERT(lastProperty()->hasSlot() && getSlot(lastProperty()->slot()).isUndefined());
        removeLastProperty(cx);
    }
}

bool
JSObject::generateOwnShape(JSContext *cx, Shape *newShape)
{
#if 0
#ifdef JS_TRACER
    JS_ASSERT_IF(!parent && JS_ON_TRACE(cx), JS_TRACE_MONITOR_ON_TRACE(cx)->bailExit);
    LeaveTraceIfGlobalObject(cx, this);

    




    if (TraceRecorder *tr = TRACE_RECORDER(cx))
        tr->forgetGuardedShapesForObject(this);
#endif
#endif

    if (!inDictionaryMode() && !toDictionaryMode(cx))
        return false;

    if (!newShape) {
        newShape = js_NewGCShape(cx);
        if (!newShape)
            return false;
    }

    PropertyTable &table = lastProperty()->table();
    Shape **spp = lastProperty()->isEmptyShape() ? NULL : table.search(lastProperty()->maybePropid(), false);

    Shape *oldShape = lastProperty();
    newShape->initDictionaryShape(*oldShape, &shape_);

    JS_ASSERT(newShape->parent == oldShape);
    oldShape->removeFromDictionary(this);

    oldShape->handoffTableTo(newShape);

    if (spp)
        SHAPE_STORE_PRESERVING_COLLISION(spp, newShape);
    return true;
}

const Shape *
JSObject::methodShapeChange(JSContext *cx, const Shape &shape)
{
    const Shape *result = &shape;

    if (!inDictionaryMode() && !toDictionaryMode(cx))
        return NULL;

    Shape *spare = js_NewGCShape(cx);
    if (!spare)
        return NULL;

    if (shape.isMethod()) {
#ifdef DEBUG
        JS_ASSERT(canHaveMethodBarrier());
        JS_ASSERT(!shape.setter());
        JS_ASSERT(!shape.hasShortID());
#endif

        




        result = putProperty(cx, shape.propid(), NULL, NULL, shape.slot(),
                             shape.attrs,
                             shape.getFlags() & ~Shape::METHOD,
                             0);
        if (!result)
            return NULL;
    }

    JS_ALWAYS_TRUE(generateOwnShape(cx, spare));
    return result;
}

bool
JSObject::protoShapeChange(JSContext *cx)
{
    return generateOwnShape(cx);
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
    if (inDictionaryMode()) {
        lastProperty()->base()->setParent(parent);
        return true;
    }

    return Shape::setObjectParent(cx, parent, &shape_);
}

 bool
Shape::setObjectParent(JSContext *cx, JSObject *parent, Shape **listp)
{
    BaseShape base(*(*listp)->base()->unowned());
    base.setParent(parent);
    BaseShape *nbase = BaseShape::lookup(cx, base);
    if (!nbase)
        return false;

    Shape child(*listp);
    child.base_ = nbase;

    return replaceLastProperty(cx, child, listp);
}

 inline HashNumber
JSCompartment::BaseShapeEntry::hash(const js::BaseShape *base)
{
    JS_ASSERT(!base->isOwned());

    JSDHashNumber hash = base->flags;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (jsuword(base->clasp) >> 3);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (jsuword(base->parent) >> 3);
    if (base->rawGetter)
        hash = JS_ROTATE_LEFT32(hash, 4) ^ jsuword(base->rawGetter);
    if (base->rawSetter)
        hash = JS_ROTATE_LEFT32(hash, 4) ^ jsuword(base->rawSetter);
    return hash;
}

 inline bool
JSCompartment::BaseShapeEntry::match(const BaseShapeEntry &entry, const BaseShape *lookup)
{
    BaseShape *key = entry.base;
    JS_ASSERT(!key->isOwned() && !lookup->isOwned());

    return key->flags == lookup->flags
        && key->clasp == lookup->clasp
        && key->parent == lookup->parent
        && key->getterObj == lookup->getterObj
        && key->setterObj == lookup->setterObj;
}

static inline JSCompartment::BaseShapeEntry *
LookupBaseShape(JSContext *cx, const BaseShape &base)
{
    JSCompartment::BaseShapeSet &table = cx->compartment->baseShapes;

    if (!table.initialized() && !table.init())
        return false;

    JSCompartment::BaseShapeSet::AddPtr p = table.lookupForAdd(&base);
    if (p)
        return &const_cast<JSCompartment::BaseShapeEntry &>(*p);

    BaseShape *nbase = js_NewGCBaseShape(cx);
    if (!nbase)
        return false;
    new (nbase) BaseShape(base);

    JSCompartment::BaseShapeEntry entry;
    entry.base = static_cast<UnownedBaseShape *>(nbase);
    entry.shape = NULL;

    if (!table.relookupOrAdd(p, &base, entry))
        return NULL;

    return &const_cast<JSCompartment::BaseShapeEntry &>(*p);
}

 UnownedBaseShape *
BaseShape::lookup(JSContext *cx, const BaseShape &base)
{
    JSCompartment::BaseShapeEntry *entry = LookupBaseShape(cx, base);
    return entry ? entry->base : NULL;
}

 Shape *
BaseShape::lookupInitialShape(JSContext *cx, Class *clasp, JSObject *parent, Shape *initial)
{
    js::BaseShape base(clasp, parent);
    JSCompartment::BaseShapeEntry *entry = LookupBaseShape(cx, base);
    if (!entry)
        return NULL;
    if (entry->shape)
        return entry->shape;

    if (initial) {
        entry->shape = initial;
        return entry->shape;
    }

    entry->shape = JS_PROPERTY_TREE(cx).newShape(cx);
    if (!entry->shape)
        return NULL;
    return new (entry->shape) EmptyShape(entry->base);
}

 void
BaseShape::insertInitialShape(JSContext *cx, const Shape *initial)
{
    JSCompartment::BaseShapeEntry *entry = LookupBaseShape(cx, *initial->base());
    JS_ASSERT(entry && entry->base == initial->base());
    entry->shape = const_cast<Shape *>(initial);
}

void
JSCompartment::sweepBaseShapeTable(JSContext *cx)
{
    if (baseShapes.initialized()) {
        for (BaseShapeSet::Enum e(baseShapes); !e.empty(); e.popFront()) {
            JSCompartment::BaseShapeEntry &entry =
                const_cast<JSCompartment::BaseShapeEntry &>(e.front());
            if (!entry.base->isMarked())
                e.removeFront();
            else if (entry.shape && !entry.shape->isMarked())
                entry.shape = NULL;
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

 bool
Shape::setExtensibleParents(JSContext *cx, Shape **listp)
{
    Shape *shape = *listp;
    JS_ASSERT(!shape->inDictionary());

    BaseShape base(*shape->base()->unowned());
    base.flags |= BaseShape::EXTENSIBLE_PARENTS;
    BaseShape *nbase = BaseShape::lookup(cx, base);
    if (!nbase)
        return NULL;

    Shape child(shape);
    child.base_ = nbase;

    return replaceLastProperty(cx, child, listp);
}

bool
Bindings::setExtensibleParents(JSContext *cx)
{
    if (!ensureShape(cx))
        return false;
    return Shape::setExtensibleParents(cx, &lastBinding);
}

bool
Bindings::setParent(JSContext *cx, JSObject *obj)
{
    if (!ensureShape(cx))
        return false;
    return Shape::setObjectParent(cx, obj, &lastBinding);
}
