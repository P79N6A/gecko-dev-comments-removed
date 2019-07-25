










































#include <new>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h"
#include "jsbit.h"
#include "jsclist.h"
#include "jsdhash.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"      
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jstracer.h"

#include "jsdbgapiinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;
using namespace js::gc;

uint32
js_GenerateShape(JSRuntime *rt)
{
    uint32 shape;

    shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
    JS_ASSERT(shape != 0);
    if (shape >= SHAPE_OVERFLOW_BIT) {
        





        rt->shapeGen = SHAPE_OVERFLOW_BIT;
        shape = SHAPE_OVERFLOW_BIT;

#ifdef JS_THREADSAFE
        AutoLockGC lockIf(rt);
#endif
        TriggerGC(rt);
    }
    return shape;
}

uint32
js_GenerateShape(JSContext *cx)
{
    return js_GenerateShape(cx->runtime);
}

bool
JSObject::ensureClassReservedSlotsForEmptyObject(JSContext *cx)
{
    JS_ASSERT(nativeEmpty());

    


















    uint32 nfixed = JSSLOT_FREE(getClass());
    if (nfixed > numSlots() && !allocSlots(cx, nfixed))
        return false;

    return true;
}

#define PROPERTY_TABLE_NBYTES(n) ((n) * sizeof(Shape *))

#ifdef DEBUG
JS_FRIEND_DATA(JSScopeStats) js_scope_stats = {0};

# define METER(x)       JS_ATOMIC_INCREMENT(&js_scope_stats.x)
#else
# define METER(x)       ((void) 0)
#endif

bool
PropertyTable::init(JSRuntime *rt, Shape *lastProp)
{
    






    uint32 sizeLog2 = JS_CeilingLog2(2 * entryCount);
    if (sizeLog2 < MIN_SIZE_LOG2)
        sizeLog2 = MIN_SIZE_LOG2;

    



    entries = (Shape **) rt->calloc_(JS_BIT(sizeLog2) * sizeof(Shape *));
    if (!entries) {
        METER(tableAllocFails);
        return false;
    }

    hashShift = JS_DHASH_BITS - sizeLog2;
    for (Shape::Range r = lastProp->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
        METER(searches);
        METER(initSearches);
        Shape **spp = search(shape.propid, true);

        



        if (!SHAPE_FETCH(spp))
            SHAPE_STORE_PRESERVING_COLLISION(spp, &shape);
    }
    return true;
}

bool
Shape::hashify(JSRuntime *rt)
{
    JS_ASSERT(!hasTable());
    PropertyTable *table = rt->new_<PropertyTable>(entryCount());
    if (!table)
        return false;
    setTable(table);
    return getTable()->init(rt, this);
}

#ifdef DEBUG
# include "jsprf.h"
# define LIVE_SCOPE_METER(cx,expr) JS_LOCK_RUNTIME_VOID(cx->runtime,expr)
#else
# define LIVE_SCOPE_METER(cx,expr)
#endif

JS_STATIC_ASSERT(sizeof(JSHashNumber) == 4);
JS_STATIC_ASSERT(sizeof(jsid) == JS_BYTES_PER_WORD);

#if JS_BYTES_PER_WORD == 4
# define HASH_ID(id) ((JSHashNumber)(JSID_BITS(id)))
#elif JS_BYTES_PER_WORD == 8
# define HASH_ID(id) ((JSHashNumber)(JSID_BITS(id)) ^ (JSHashNumber)((JSID_BITS(id)) >> 32))
#else
# error "Unsupported configuration"
#endif







#define HASH0(id)               (HASH_ID(id) * JS_GOLDEN_RATIO)
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
    JS_ASSERT(!JSID_IS_VOID(id));

    
    METER(hashes);
    hash0 = HASH0(id);
    hash1 = HASH1(hash0, hashShift);
    spp = entries + hash1;

    
    stored = *spp;
    if (SHAPE_IS_FREE(stored)) {
        METER(misses);
        METER(hashMisses);
        return spp;
    }

    
    shape = SHAPE_CLEAR_COLLISION(stored);
    if (shape && shape->propid == id) {
        METER(hits);
        METER(hashHits);
        return spp;
    }

    
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
        METER(steps);
        hash1 -= hash2;
        hash1 &= sizeMask;
        spp = entries + hash1;

        stored = *spp;
        if (SHAPE_IS_FREE(stored)) {
            METER(misses);
            METER(stepMisses);
            return (adding && firstRemoved) ? firstRemoved : spp;
        }

        shape = SHAPE_CLEAR_COLLISION(stored);
        if (shape && shape->propid == id) {
            METER(hits);
            METER(stepHits);
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
    int oldlog2, newlog2;
    uint32 oldsize, newsize, nbytes;
    Shape **newTable, **oldTable, **spp, **oldspp, *shape;

    JS_ASSERT(entries);

    


    oldlog2 = JS_DHASH_BITS - hashShift;
    newlog2 = oldlog2 + log2Delta;
    oldsize = JS_BIT(oldlog2);
    newsize = JS_BIT(newlog2);
    nbytes = PROPERTY_TABLE_NBYTES(newsize);
    newTable = (Shape **) cx->calloc_(nbytes);
    if (!newTable) {
        METER(tableAllocFails);
        return false;
    }

    
    hashShift = JS_DHASH_BITS - newlog2;
    removedCount = 0;
    oldTable = entries;
    entries = newTable;

    
    for (oldspp = oldTable; oldsize != 0; oldspp++) {
        shape = SHAPE_FETCH(oldspp);
        if (shape) {
            METER(searches);
            METER(changeSearches);
            spp = search(shape->propid, true);
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
    if (!delta)
        METER(compresses);
    else
        METER(grows);

    if (!change(delta, cx) && entryCount + removedCount == size - 1) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

Shape *
Shape::getChild(JSContext *cx, const js::Shape &child, Shape **listp)
{
    JS_ASSERT(!JSID_IS_VOID(child.propid));
    JS_ASSERT(!child.inDictionary());

    if (inDictionary()) {
        Shape *oldShape = *listp;
        PropertyTable *table = (oldShape && oldShape->hasTable()) ? oldShape->getTable() : NULL;

        




        if (table && table->needsToGrow() && !table->grow(cx))
            return NULL;

        if (newDictionaryShape(cx, child, listp)) {
            Shape *newShape = *listp;

            JS_ASSERT(oldShape == newShape->parent);
            if (table) {
                
                METER(searches);
                Shape **spp = table->search(newShape->propid, true);

                






                if (!SHAPE_FETCH(spp))
                    ++table->entryCount;
                SHAPE_STORE_PRESERVING_COLLISION(spp, newShape);

                
                oldShape->setTable(NULL);
                newShape->setTable(table);
            } else {
                if (!newShape->hasTable())
                    newShape->hashify(cx->runtime);
            }
            return newShape;
        }

        return NULL;
    }

    if ((*listp)->entryCount() >= PropertyTree::MAX_HEIGHT) {
        Shape *dprop = Shape::newDictionaryList(cx, listp);
        if (!dprop)
            return NULL;
        return dprop->getChild(cx, child, listp);
    }

    Shape *shape = JS_PROPERTY_TREE(cx).getChild(cx, this, child);
    if (shape) {
        JS_ASSERT(shape->parent == this);
        JS_ASSERT(this == *listp);
        *listp = shape;
    }
    return shape;
}






Shape *
JSObject::getChildProperty(JSContext *cx, Shape *parent, Shape &child)
{
    JS_ASSERT(!JSID_IS_VOID(child.propid));
    JS_ASSERT(!child.inDictionary());

    





    if (!child.isAlias()) {
        if (child.attrs & JSPROP_SHARED) {
            child.slot = SHAPE_INVALID_SLOT;
        } else {
            





            if (child.slot == SHAPE_INVALID_SLOT && !allocSlot(cx, &child.slot))
                return NULL;
        }
    }

    Shape *shape;

    if (inDictionaryMode()) {
        JS_ASSERT(parent == lastProp);
        if (parent->frozen()) {
            parent = Shape::newDictionaryList(cx, &lastProp);
            if (!parent)
                return NULL;
            JS_ASSERT(!parent->frozen());
        }
        shape = Shape::newDictionaryShape(cx, child, &lastProp);
        if (!shape)
            return NULL;
    } else {
        shape = JS_PROPERTY_TREE(cx).getChild(cx, parent, child);
        if (!shape)
            return NULL;
        JS_ASSERT(shape->parent == parent);
        JS_ASSERT_IF(parent != lastProp, parent == lastProp->parent);
        setLastProperty(shape);
    }

    updateFlags(shape);
    updateShape(cx);
    return shape;
}

Shape *
Shape::newDictionaryShape(JSContext *cx, const Shape &child, Shape **listp)
{
    Shape *dprop = JS_PROPERTY_TREE(cx).newShape(cx);
    if (!dprop)
        return NULL;

    new (dprop) Shape(child.propid, child.rawGetter, child.rawSetter, child.slot, child.attrs,
                      (child.flags & ~FROZEN) | IN_DICTIONARY, child.shortid,
                      js_GenerateShape(cx), child.slotSpan);

    dprop->listp = NULL;
    dprop->insertIntoDictionary(listp);

    JS_COMPARTMENT_METER(cx->compartment->liveDictModeNodes++);
    return dprop;
}

Shape *
Shape::newDictionaryList(JSContext *cx, Shape **listp)
{
    Shape *shape = *listp;
    Shape *list = shape;

    




    Shape *root = NULL;
    Shape **childp = &root;

    while (shape) {
        JS_ASSERT_IF(!shape->frozen(), !shape->inDictionary());

        Shape *dprop = Shape::newDictionaryShape(cx, *shape, childp);
        if (!dprop) {
            METER(toDictFails);
            *listp = list;
            return NULL;
        }

        JS_ASSERT(!dprop->hasTable());
        childp = &dprop->parent;
        shape = shape->parent;
    }

    *listp = root;
    root->listp = listp;

    JS_ASSERT(root->inDictionary());
    root->hashify(cx->runtime);
    return root;
}

bool
JSObject::toDictionaryMode(JSContext *cx)
{
    JS_ASSERT(!inDictionaryMode());

    
    JS_ASSERT(compartment() == cx->compartment);
    if (!Shape::newDictionaryList(cx, &lastProp))
        return false;

    clearOwnShape();
    return true;
}





static inline bool
NormalizeGetterAndSetter(JSContext *cx, JSObject *obj,
                         jsid id, uintN attrs, uintN flags,
                         PropertyOp &getter,
                         StrictPropertyOp &setter)
{
    if (setter == StrictPropertyStub) {
        JS_ASSERT(!(attrs & JSPROP_SETTER));
        setter = NULL;
    }
    if (flags & Shape::METHOD) {
        
        JS_ASSERT(getter);
        JS_ASSERT(!setter || setter == js_watch_set);
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    } else {
        if (getter == PropertyStub) {
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
    if (hasOwnShape())
        JS_ASSERT(objShape != lastProp->shapeid);
    else
        JS_ASSERT(objShape == lastProp->shapeid);

    Shape *shape = lastProp;
    Shape *prev = NULL;

    if (inDictionaryMode()) {
        if (shape->hasTable()) {
            PropertyTable *table = shape->getTable();
            for (uint32 fslot = table->freelist; fslot != SHAPE_INVALID_SLOT;
                 fslot = getSlotRef(fslot).toPrivateUint32()) {
                JS_ASSERT(fslot < shape->slotSpan);
            }

            for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
                JS_ASSERT_IF(shape != lastProp, !shape->hasTable());

                Shape **spp = table->search(shape->propid, false);
                JS_ASSERT(SHAPE_FETCH(spp) == shape);
            }
        } else {
            shape = shape->parent;
            for (int n = throttle; --n >= 0 && shape; shape = shape->parent)
                JS_ASSERT(!shape->hasTable());
        }

        shape = lastProp;
        for (int n = throttle; --n >= 0 && shape; shape = shape->parent) {
            JS_ASSERT_IF(shape->slot != SHAPE_INVALID_SLOT, shape->slot < shape->slotSpan);
            if (!prev) {
                JS_ASSERT(shape == lastProp);
                JS_ASSERT(shape->listp == &lastProp);
            } else {
                JS_ASSERT(shape->listp == &prev->parent);
                JS_ASSERT(prev->slotSpan >= shape->slotSpan);
            }
            prev = shape;
        }
    } else {
        for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
            if (shape->hasTable()) {
                PropertyTable *table = shape->getTable();
                JS_ASSERT(shape->parent);
                for (Shape::Range r(shape); !r.empty(); r.popFront()) {
                    Shape **spp = table->search(r.front().propid, false);
                    JS_ASSERT(SHAPE_FETCH(spp) == &r.front());
                }
            }
            if (prev) {
                JS_ASSERT(prev->slotSpan >= shape->slotSpan);
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
                      uintN flags, intN shortid)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    if (!isExtensible()) {
        reportNotExtensible(cx);
        return NULL;
    }

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    Shape **spp = nativeSearch(id, true);
    JS_ASSERT(!SHAPE_FETCH(spp));
    const Shape *shape = addPropertyInternal(cx, id, getter, setter, slot, attrs, 
                                             flags, shortid, spp);
    if (!shape)
        return NULL;

    
    shape = js_UpdateWatchpointsForShape(cx, this, shape);
    if (!shape)
        METER(wrapWatchFails);

    return shape;
}

const Shape *
JSObject::addPropertyInternal(JSContext *cx, jsid id,
                              PropertyOp getter, StrictPropertyOp setter,
                              uint32 slot, uintN attrs,
                              uintN flags, intN shortid,
                              Shape **spp)
{
    JS_ASSERT_IF(inDictionaryMode(), !lastProp->frozen());

    PropertyTable *table = NULL;
    if (!inDictionaryMode()) {
        if (lastProp->entryCount() >= PropertyTree::MAX_HEIGHT) {
            if (!toDictionaryMode(cx))
                return NULL;
            spp = nativeSearch(id, true);
            table = lastProp->getTable();
        }
    } else if (lastProp->hasTable()) {
        table = lastProp->getTable();
        if (table->needsToGrow()) {
            if (!table->grow(cx))
                return NULL;

            METER(searches);
            METER(changeSearches);
            spp = table->search(id, true);
            JS_ASSERT(!SHAPE_FETCH(spp));
        }
    }

    
    const Shape *shape;
    {
        Shape child(id, getter, setter, slot, attrs, flags, shortid);
        shape = getChildProperty(cx, lastProp, child);
    }

    if (shape) {
        JS_ASSERT(shape == lastProp);

        if (table) {
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, shape);
            ++table->entryCount;

            
            JS_ASSERT(shape->parent->getTable() == table);
            shape->parent->setTable(NULL);
            shape->setTable(table);
        }

        CHECK_SHAPE_CONSISTENCY(this);
        METER(adds);
        return shape;
    }

    CHECK_SHAPE_CONSISTENCY(this);
    METER(addFails);
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
        obj->reportNotConfigurable(cx, shape->propid);
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

    



    if (lastProp->frozen()) {
        if (!Shape::newDictionaryList(cx, &lastProp))
            return NULL;
        JS_ASSERT(!lastProp->frozen());
    }

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    Shape **spp = nativeSearch(id, true);
    Shape *shape = SHAPE_FETCH(spp);
    if (!shape) {
        



        if (!isExtensible()) {
            reportNotExtensible(cx);
            return NULL;
        }

        const Shape *newShape =
            addPropertyInternal(cx, id, getter, setter, slot, attrs, flags, shortid, spp);
        if (!newShape)
            return NULL;
        newShape = js_UpdateWatchpointsForShape(cx, this, newShape);
        if (!newShape)
            METER(wrapWatchFails);
        return newShape;
    }

    
    JS_ASSERT(!SHAPE_IS_REMOVED(*spp));

    if (!CheckCanChangeAttrs(cx, this, shape, &attrs))
        return NULL;
    
    




    bool hadSlot = !shape->isAlias() && shape->hasSlot();
    uint32 oldSlot = shape->slot;
    if (!(attrs & JSPROP_SHARED) && slot == SHAPE_INVALID_SLOT && hadSlot)
        slot = oldSlot;

    



    if (shape->matchesParamsAfterId(getter, setter, slot, attrs, flags, shortid)) {
        METER(redundantPuts);
        return shape;
    }

    




    if (shape != lastProp && !inDictionaryMode()) {
        if (!toDictionaryMode(cx))
            return NULL;
        spp = nativeSearch(shape->propid);
        shape = SHAPE_FETCH(spp);
    }

    











    if (inDictionaryMode()) {
        
        if (slot == SHAPE_INVALID_SLOT && !(attrs & JSPROP_SHARED) && !(flags & Shape::ALIAS)) {
            if (!allocSlot(cx, &slot))
                return NULL;
        }

        shape->slot = slot;
        if (slot != SHAPE_INVALID_SLOT && slot >= shape->slotSpan) {
            shape->slotSpan = slot + 1;

            for (Shape *temp = lastProp; temp != shape; temp = temp->parent) {
                if (temp->slotSpan <= slot)
                    temp->slotSpan = slot + 1;
            }
        }

        shape->rawGetter = getter;
        shape->rawSetter = setter;
        shape->attrs = uint8(attrs);
        shape->flags = flags | Shape::IN_DICTIONARY;
        shape->shortid = int16(shortid);

        





        updateFlags(shape);

        





        lastProp->shapeid = js_GenerateShape(cx);
        clearOwnShape();
    } else {
        







        JS_ASSERT(shape == lastProp);
        removeLastProperty();

        
        Shape child(id, getter, setter, slot, attrs, flags, shortid);

        Shape *newShape = getChildProperty(cx, lastProp, child);
        if (!newShape) {
            setLastProperty(shape);
            CHECK_SHAPE_CONSISTENCY(this);
            METER(putFails);
            return NULL;
        }

        shape = newShape;
    }

    





    if (hadSlot && !shape->hasSlot()) {
        if (oldSlot < shape->slotSpan)
            freeSlot(cx, oldSlot);
        else
            getSlotRef(oldSlot).setUndefined();
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    CHECK_SHAPE_CONSISTENCY(this);
    METER(puts);

    const Shape *newShape = js_UpdateWatchpointsForShape(cx, this, shape);
    if (!newShape)
        METER(wrapWatchFails);
    return newShape;
}

const Shape *
JSObject::changeProperty(JSContext *cx, const Shape *shape, uintN attrs, uintN mask,
                         PropertyOp getter, StrictPropertyOp setter)
{
    JS_ASSERT_IF(inDictionaryMode(), !lastProp->frozen());
    JS_ASSERT(!JSID_IS_VOID(shape->propid));
    JS_ASSERT(nativeContains(*shape));

    attrs |= shape->attrs & mask;

    
    JS_ASSERT(!((attrs ^ shape->attrs) & JSPROP_SHARED) ||
              !(attrs & JSPROP_SHARED));

    
    JS_ASSERT_IF(getter != shape->rawGetter, !shape->isMethod());

    if (getter == PropertyStub)
        getter = NULL;
    if (setter == StrictPropertyStub)
        setter = NULL;

    if (!CheckCanChangeAttrs(cx, this, shape, &attrs))
        return NULL;
    
    if (shape->attrs == attrs && shape->getter() == getter && shape->setter() == setter)
        return shape;

    const Shape *newShape;

    



    if (inDictionaryMode()) {
        
        uint32 slot = shape->slot;
        if (slot == SHAPE_INVALID_SLOT && !(attrs & JSPROP_SHARED) && !(flags & Shape::ALIAS)) {
            if (!allocSlot(cx, &slot))
                return NULL;
        }

        Shape *mutableShape = const_cast<Shape *>(shape);
        mutableShape->slot = slot;
        if (slot != SHAPE_INVALID_SLOT && slot >= shape->slotSpan) {
            mutableShape->slotSpan = slot + 1;

            for (Shape *temp = lastProp; temp != shape; temp = temp->parent) {
                if (temp->slotSpan <= slot)
                    temp->slotSpan = slot + 1;
            }
        }

        mutableShape->rawGetter = getter;
        mutableShape->rawSetter = setter;
        mutableShape->attrs = uint8(attrs);

        updateFlags(shape);

        
        lastProp->shapeid = js_GenerateShape(cx);
        clearOwnShape();

        shape = js_UpdateWatchpointsForShape(cx, this, shape);
        if (!shape) {
            METER(wrapWatchFails);
            return NULL;
        }
        JS_ASSERT(shape == mutableShape);
        newShape = mutableShape;
    } else if (shape == lastProp) {
        Shape child(shape->propid, getter, setter, shape->slot, attrs, shape->flags,
                    shape->shortid);

        newShape = getChildProperty(cx, shape->parent, child);
#ifdef DEBUG
        if (newShape) {
            JS_ASSERT(newShape == lastProp);
            if (newShape->hasTable()) {
                Shape **spp = nativeSearch(shape->propid);
                JS_ASSERT(SHAPE_FETCH(spp) == newShape);
            }
        }
#endif
    } else {
        





        Shape child(shape->propid, getter, setter, shape->slot, attrs, shape->flags,
                    shape->shortid);
        newShape = putProperty(cx, child.propid, child.rawGetter, child.rawSetter, child.slot,
                               child.attrs, child.flags, child.shortid);
#ifdef DEBUG
        if (newShape)
            METER(changePuts);
#endif
    }

#ifdef DEBUG
    CHECK_SHAPE_CONSISTENCY(this);
    if (newShape)
        METER(changes);
    else
        METER(changeFails);
#endif
    return newShape;
}

bool
JSObject::removeProperty(JSContext *cx, jsid id)
{
    Shape **spp = nativeSearch(id);
    Shape *shape = SHAPE_FETCH(spp);
    if (!shape) {
        METER(uselessRemoves);
        return true;
    }

    
    bool addedToFreelist = false;
    bool hadSlot = !shape->isAlias() && shape->hasSlot();
    if (hadSlot) {
        addedToFreelist = freeSlot(cx, shape->slot);
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }


    
    if (shape != lastProp && !inDictionaryMode()) {
        if (!toDictionaryMode(cx))
            return false;
        spp = nativeSearch(shape->propid);
        shape = SHAPE_FETCH(spp);
    }

    




    if (inDictionaryMode()) {
        PropertyTable *table = lastProp->hasTable() ? lastProp->getTable() : NULL;

        if (SHAPE_HAD_COLLISION(*spp)) {
            JS_ASSERT(table);
            *spp = SHAPE_REMOVED;
            ++table->removedCount;
            --table->entryCount;
        } else {
            METER(removeFrees);
            if (table) {
                *spp = NULL;
                --table->entryCount;

#ifdef DEBUG
                




                const Shape *aprop = lastProp;
                for (int n = 50; --n >= 0 && aprop->parent; aprop = aprop->parent)
                    JS_ASSERT_IF(aprop != shape, nativeContains(*aprop));
#endif
            }
        }

        







        flags |= OWN_SHAPE;

        Shape *oldLastProp = lastProp;
        shape->removeFromDictionary(this);
        if (table) {
            if (shape == oldLastProp) {
                JS_ASSERT(shape->getTable() == table);
                JS_ASSERT(shape->parent == lastProp);
                JS_ASSERT(shape->slotSpan >= lastProp->slotSpan);
                JS_ASSERT_IF(hadSlot, shape->slot + 1 <= shape->slotSpan);

                




 
                if (table->freelist != SHAPE_INVALID_SLOT) {
                    lastProp->slotSpan = shape->slotSpan;
                    
                    
                    if (hadSlot && !addedToFreelist) {
                        getSlotRef(shape->slot).setPrivateUint32(table->freelist);
                        table->freelist = shape->slot;
                    }
                }
            }

            
            oldLastProp->setTable(NULL);
            lastProp->setTable(table);
        }
    } else {
        




        JS_ASSERT(shape == lastProp);
        removeLastProperty();

        




        size_t fixed = numFixedSlots();
        if (shape->slot == fixed) {
            JS_ASSERT_IF(!lastProp->isEmptyShape() && lastProp->hasSlot(),
                         lastProp->slot == fixed - 1);
            revertToFixedSlots(cx);
        }
    }
    updateShape(cx);

    
    if (lastProp->hasTable()) {
        PropertyTable *table = lastProp->getTable();
        uint32 size = table->capacity();
        if (size > PropertyTable::MIN_SIZE && table->entryCount <= size >> 2) {
            METER(shrinks);
            (void) table->change(-1, cx);
        }
    }

    
    if (hasSlotsArray()) {
        JS_ASSERT(slotSpan() <= numSlots());
        if ((slotSpan() + (slotSpan() >> 2)) < numSlots())
            shrinkSlots(cx, slotSpan());
    }

    CHECK_SHAPE_CONSISTENCY(this);
    METER(removes);
    return true;
}

void
JSObject::clear(JSContext *cx)
{
    Shape *shape = lastProp;
    JS_ASSERT(inDictionaryMode() == shape->inDictionary());

    while (shape->parent) {
        shape = shape->parent;
        JS_ASSERT(inDictionaryMode() == shape->inDictionary());
    }
    JS_ASSERT(shape->isEmptyShape());

    if (inDictionaryMode())
        shape->listp = &lastProp;

    




    if (hasSlotsArray() && JSSLOT_FREE(getClass()) <= numFixedSlots())
        revertToFixedSlots(cx);

    



    clearOwnShape();
    setMap(shape);

    LeaveTraceIfGlobalObject(cx, this);
    JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    CHECK_SHAPE_CONSISTENCY(this);
}

void
JSObject::generateOwnShape(JSContext *cx)
{
#ifdef JS_TRACER
    JS_ASSERT_IF(!parent && JS_ON_TRACE(cx), JS_TRACE_MONITOR_ON_TRACE(cx)->bailExit);
    LeaveTraceIfGlobalObject(cx, this);

    




    if (TraceRecorder *tr = TRACE_RECORDER(cx))
        tr->forgetGuardedShapesForObject(this);
#endif

    setOwnShape(js_GenerateShape(cx));
}

void
JSObject::deletingShapeChange(JSContext *cx, const Shape &shape)
{
    JS_ASSERT(!JSID_IS_VOID(shape.propid));
    generateOwnShape(cx);
}

const Shape *
JSObject::methodShapeChange(JSContext *cx, const Shape &shape)
{
    const Shape *result = &shape;

    JS_ASSERT(!JSID_IS_VOID(shape.propid));
    if (shape.isMethod()) {
#ifdef DEBUG
        const Value &prev = nativeGetSlot(shape.slot);
        JS_ASSERT(shape.methodObject() == prev.toObject());
        JS_ASSERT(canHaveMethodBarrier());
        JS_ASSERT(hasMethodBarrier());
        JS_ASSERT(!shape.rawSetter || shape.rawSetter == js_watch_set);
#endif

        





        result = putProperty(cx, shape.propid, NULL, shape.rawSetter, shape.slot,
                             shape.attrs,
                             shape.getFlags() & ~Shape::METHOD,
                             shape.shortid);
        if (!result)
            return NULL;
    }

    if (branded()) {
        uintN thrashCount = getMethodThrashCount();
        if (thrashCount < JSObject::METHOD_THRASH_COUNT_MAX) {
            ++thrashCount;
            setMethodThrashCount(thrashCount);
            if (thrashCount == JSObject::METHOD_THRASH_COUNT_MAX) {
                unbrand(cx);
                return result;
            }
        }
    }

    generateOwnShape(cx);
    return result;
}

bool
JSObject::methodShapeChange(JSContext *cx, uint32 slot)
{
    if (!hasMethodBarrier()) {
        generateOwnShape(cx);
    } else {
        for (Shape::Range r = lastProp->all(); !r.empty(); r.popFront()) {
            const Shape &shape = r.front();
            JS_ASSERT(!JSID_IS_VOID(shape.propid));
            if (shape.slot == slot)
                return methodShapeChange(cx, shape) != NULL;
        }
    }
    return true;
}

void
JSObject::protoShapeChange(JSContext *cx)
{
    generateOwnShape(cx);
}

void
JSObject::shadowingShapeChange(JSContext *cx, const Shape &shape)
{
    JS_ASSERT(!JSID_IS_VOID(shape.propid));
    generateOwnShape(cx);
}

bool
JSObject::globalObjectOwnShapeChange(JSContext *cx)
{
    generateOwnShape(cx);
    return !js_IsPropertyCacheDisabled(cx);
}
