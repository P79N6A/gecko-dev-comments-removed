










































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

#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;

uint32
js_GenerateShape(JSContext *cx, bool gcLocked)
{
    JSRuntime *rt;
    uint32 shape;

    rt = cx->runtime;
    shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
    JS_ASSERT(shape != 0);
    if (shape >= SHAPE_OVERFLOW_BIT) {
        





        rt->shapeGen = SHAPE_OVERFLOW_BIT;
        shape = SHAPE_OVERFLOW_BIT;
        
#ifdef JS_THREADSAFE
        Conditionally<AutoLockGC> lockIf(!gcLocked, rt);
#endif
        TriggerGC(rt);
    }
    return shape;
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
# define METER(x)
#endif

bool
PropertyTable::init(JSContext *cx, Shape *lastProp)
{
    int sizeLog2;

    if (entryCount > HASH_THRESHOLD) {
        






        sizeLog2 = JS_CeilingLog2(2 * entryCount);
    } else {
        JS_ASSERT(hashShift == JS_DHASH_BITS - MIN_SIZE_LOG2);
        sizeLog2 = MIN_SIZE_LOG2;
    }

    entries = (Shape **) js_calloc(JS_BIT(sizeLog2) * sizeof(Shape *));
    if (!entries) {
        METER(tableAllocFails);
        return false;
    }
    cx->runtime->updateMallocCounter(JS_BIT(sizeLog2) * sizeof(Shape *));

    hashShift = JS_DHASH_BITS - sizeLog2;
    for (Shape::Range r = lastProp->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
        METER(searches);
        METER(initSearches);
        Shape **spp = search(shape.id, true);
        SHAPE_STORE_PRESERVING_COLLISION(spp, &shape);
    }
    return true;
}

bool
Shape::maybeHash(JSContext *cx)
{
    JS_ASSERT(!table);
    uint32 nentries = entryCount();
    if (nentries >= PropertyTable::HASH_THRESHOLD) {
        table = cx->create<PropertyTable>(nentries);
        return table && table->init(cx, this);
    }
    return true;
}

#ifdef DEBUG
# include "jsprf.h"
# define LIVE_SCOPE_METER(cx,expr) JS_LOCK_RUNTIME_VOID(cx->runtime,expr)
#else
# define LIVE_SCOPE_METER(cx,expr)
#endif


bool
Shape::initRuntimeState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

#define SHAPE(Name) rt->empty##Name##Shape
#define CLASP(Name) &js_##Name##Class

#define INIT_EMPTY_SHAPE(Name,NAME)                                           \
    INIT_EMPTY_SHAPE_WITH_CLASS(Name, NAME, CLASP(Name))

#define INIT_EMPTY_SHAPE_WITH_CLASS(Name,NAME,clasp)                          \
    JS_BEGIN_MACRO                                                            \
        SHAPE(Name) = EmptyShape::create(cx, clasp);                          \
        if (!SHAPE(Name))                                                     \
            return false;                                                     \
        JS_ASSERT(SHAPE(Name)->shape == Shape::EMPTY_##NAME##_SHAPE);         \
    JS_END_MACRO

    









    INIT_EMPTY_SHAPE(Arguments, ARGUMENTS);

    INIT_EMPTY_SHAPE(Block, BLOCK);

    




    INIT_EMPTY_SHAPE(Call, CALL);

    
    INIT_EMPTY_SHAPE(DeclEnv, DECL_ENV);

    
    INIT_EMPTY_SHAPE_WITH_CLASS(Enumerator, ENUMERATOR, &js_IteratorClass);

    
    INIT_EMPTY_SHAPE(With, WITH);

#undef SHAPE
#undef CLASP
#undef INIT_EMPTY_SHAPE
#undef INIT_EMPTY_SHAPE_WITH_CLASS

    return true;
}


void
Shape::finishRuntimeState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    rt->emptyArgumentsShape = NULL;
    rt->emptyBlockShape = NULL;
    rt->emptyCallShape = NULL;
    rt->emptyDeclEnvShape = NULL;
    rt->emptyEnumeratorShape = NULL;
    rt->emptyWithShape = NULL;
}

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
    if (shape && shape->id == id) {
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
        if (shape && shape->id == id) {
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
PropertyTable::change(JSContext *cx, int change)
{
    int oldlog2, newlog2;
    uint32 oldsize, newsize, nbytes;
    Shape **newTable, **oldTable, **spp, **oldspp, *shape;

    JS_ASSERT(entries);

    
    oldlog2 = JS_DHASH_BITS - hashShift;
    newlog2 = oldlog2 + change;
    oldsize = JS_BIT(oldlog2);
    newsize = JS_BIT(newlog2);
    nbytes = PROPERTY_TABLE_NBYTES(newsize);
    newTable = (Shape **) cx->calloc(nbytes);
    if (!newTable) {
        METER(tableAllocFails);
        return false;
    }

    
    hashShift = JS_DHASH_BITS - newlog2;
    removedCount = 0;
    oldTable = entries;
    entries = newTable;

    
    cx->runtime->updateMallocCounter(nbytes);

    
    for (oldspp = oldTable; oldsize != 0; oldspp++) {
        shape = SHAPE_FETCH(oldspp);
        if (shape) {
            METER(searches);
            METER(changeSearches);
            spp = search(shape->id, true);
            JS_ASSERT(SHAPE_IS_FREE(*spp));
            *spp = shape;
        }
        oldsize--;
    }

    
    cx->free(oldTable);
    return true;
}

Shape *
Shape::getChild(JSContext *cx, const js::Shape &child, Shape **listp)
{
    JS_ASSERT(!JSID_IS_VOID(child.id));
    JS_ASSERT(!child.inDictionary());

    if (inDictionary()) {
        if (newDictionaryShape(cx, child, listp))
            return *listp;
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
        if (!shape->table)
            shape->maybeHash(cx);
        *listp = shape;
    }
    return shape;
}






Shape *
JSObject::getChildProperty(JSContext *cx, Shape *parent, Shape &child)
{
    JS_ASSERT(!JSID_IS_VOID(child.id));
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
        if (shape) {
            JS_ASSERT(shape->parent == parent);
            JS_ASSERT_IF(parent != lastProp, parent == lastProp->parent);
            setLastProperty(shape);
        }
    }

    updateFlags(shape);
    updateShape(cx);
    return shape;
}

void
JSObject::reportReadOnlyScope(JSContext *cx)
{
    JSString *str;
    const char *bytes;

    str = js_ValueToString(cx, ObjectValue(*this));
    if (!str)
        return;
    bytes = js_GetStringBytes(cx, str);
    if (!bytes)
        return;
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_READ_ONLY, bytes);
}

Shape *
Shape::newDictionaryShape(JSContext *cx, const Shape &child, Shape **listp)
{
    Shape *dprop = JS_PROPERTY_TREE(cx).newShape(cx);
    if (!dprop)
        return NULL;

    new (dprop) Shape(child.id, child.rawGetter, child.rawSetter, child.slot, child.attrs,
                      (child.flags & ~FROZEN) | IN_DICTIONARY, child.shortid,
                      js_GenerateShape(cx, false), child.slotSpan);

    dprop->listp = NULL;
    dprop->insertIntoDictionary(listp);

    JS_RUNTIME_METER(cx->runtime, liveDictModeNodes);
    return dprop;
}

Shape *
Shape::newDictionaryList(JSContext *cx, Shape **listp)
{
    Shape *shape = *listp;
    Shape *list = shape;

    Shape **childp = listp;
    *childp = NULL;

    while (shape) {
        JS_ASSERT(!shape->inDictionary());

        Shape *dprop = Shape::newDictionaryShape(cx, *shape, childp);
        if (!dprop) {
            METER(toDictFails);
            *listp = list;
            return NULL;
        }

        JS_ASSERT(!dprop->table);
        childp = &dprop->parent;
        shape = shape->parent;
    }

    list = *listp;
    JS_ASSERT(list->inDictionary());
    list->maybeHash(cx);
    return list;
}

bool
JSObject::toDictionaryMode(JSContext *cx)
{
    JS_ASSERT(!inDictionaryMode());
    if (!Shape::newDictionaryList(cx, &lastProp))
        return false;

    clearOwnShape();
    return true;
}





static inline bool
NormalizeGetterAndSetter(JSContext *cx, JSObject *obj,
                         jsid id, uintN attrs, uintN flags,
                         PropertyOp &getter,
                         PropertyOp &setter)
{
    if (setter == PropertyStub) {
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

    




    if (!JS_CLIST_IS_EMPTY(&cx->runtime->watchPointList) &&
        js_FindWatchPoint(cx->runtime, obj, id)) {
        setter = js_WrapWatchedSetter(cx, id, attrs, setter);
        if (!setter) {
            METER(wrapWatchFails);
            return false;
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
        JS_ASSERT(objShape != lastProp->shape);
    else
        JS_ASSERT(objShape == lastProp->shape);

    Shape *shape = lastProp;
    Shape *prev = NULL;

    if (inDictionaryMode()) {
        if (PropertyTable *table = shape->table) {
            for (uint32 fslot = table->freelist; fslot != SHAPE_INVALID_SLOT;
                 fslot = getSlotRef(fslot).toPrivateUint32()) {
                JS_ASSERT(fslot < shape->slotSpan);
            }

            for (int n = throttle; --n >= 0 && shape->parent; shape = shape->parent) {
                JS_ASSERT_IF(shape != lastProp, !shape->table);

                Shape **spp = table->search(shape->id, false);
                JS_ASSERT(SHAPE_FETCH(spp) == shape);
            }
        } else {
            shape = shape->parent;
            for (int n = throttle; --n >= 0 && shape; shape = shape->parent)
                JS_ASSERT(!shape->table);
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
            if (PropertyTable *table = shape->table) {
                JS_ASSERT(shape->parent);
                for (Shape::Range r(shape); !r.empty(); r.popFront()) {
                    Shape **spp = table->search(r.front().id, false);
                    JS_ASSERT(SHAPE_FETCH(spp) == &r.front());
                }
            }
            if (prev) {
                JS_ASSERT(prev->slotSpan >= shape->slotSpan);
                shape->kids.checkConsistency(prev);
            }
            prev = shape;
        }

        if (throttle == 0) {
            JS_ASSERT(!shape->table);
            JS_ASSERT(JSID_IS_EMPTY(shape->id));
            JS_ASSERT(shape->slot == SHAPE_INVALID_SLOT);
        }
    }
}
#else
# define CHECK_SHAPE_CONSISTENCY(obj) ((void)0)
#endif

const Shape *
JSObject::addProperty(JSContext *cx, jsid id,
                      PropertyOp getter, PropertyOp setter,
                      uint32 slot, uintN attrs,
                      uintN flags, intN shortid)
{
    JS_ASSERT(!JSID_IS_VOID(id));

    





    if (sealed()) {
        reportReadOnlyScope(cx);
        return NULL;
    }

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    Shape **spp = nativeSearch(id, true);
    JS_ASSERT(!SHAPE_FETCH(spp));
    return addPropertyCommon(cx, id, getter, setter, slot, attrs, flags, shortid, spp);
}

const Shape *
JSObject::addPropertyCommon(JSContext *cx, jsid id,
                            PropertyOp getter, PropertyOp setter,
                            uint32 slot, uintN attrs,
                            uintN flags, intN shortid,
                            Shape **spp)
{
    PropertyTable *table = NULL;
    if (!inDictionaryMode()) {
        if (lastProp->entryCount() >= PropertyTree::MAX_HEIGHT) {
            if (!toDictionaryMode(cx))
                return NULL;
            spp = nativeSearch(id, true);
            table = lastProp->table;
        }
    } else if ((table = lastProp->table) != NULL) {
        
        uint32 size = table->capacity();
        if (table->entryCount + table->removedCount >= size - (size >> 2)) {
            int change = table->removedCount < size >> 2;
            if (!change)
                METER(compresses);
            else
                METER(grows);
            if (!table->change(cx, change) && table->entryCount + table->removedCount == size - 1)
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

            
            JS_ASSERT(shape->parent->table == table);
            shape->parent->setTable(NULL);
            shape->setTable(table);
        }
#ifdef DEBUG
        LIVE_SCOPE_METER(cx, ++cx->runtime->liveObjectProps);
        JS_RUNTIME_METER(cx->runtime, totalObjectProps);
#endif

        







        if (!lastProp->table)
            lastProp->maybeHash(cx);

        CHECK_SHAPE_CONSISTENCY(this);
        METER(adds);
        return shape;
    }

    CHECK_SHAPE_CONSISTENCY(this);
    METER(addFails);
    return NULL;
}

const Shape *
JSObject::putProperty(JSContext *cx, jsid id,
                      PropertyOp getter, PropertyOp setter,
                      uint32 slot, uintN attrs,
                      uintN flags, intN shortid)
{
    Shape **spp, *shape, *overwriting;

    JS_ASSERT(!JSID_IS_VOID(id));

    if (sealed()) {
        reportReadOnlyScope(cx);
        return NULL;
    }

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    spp = nativeSearch(id, true);
    shape = SHAPE_FETCH(spp);
    if (!shape)
        return addPropertyCommon(cx, id, getter, setter, slot, attrs, flags, shortid, spp);

    
    JS_ASSERT(!SHAPE_IS_REMOVED(*spp));
    overwriting = shape;

    





    if (!(attrs & JSPROP_SHARED) && slot == SHAPE_INVALID_SLOT && containsSlot(shape->slot))
        slot = shape->slot;
    if (shape->matchesParamsAfterId(getter, setter, slot, attrs, flags, shortid)) {
        METER(redundantPuts);
        return shape;
    }

    PropertyTable *table = inDictionaryMode() ? lastProp->table : NULL;

    









    Shape *oldLastProp = lastProp;
    if (shape == lastProp && !inDictionaryMode()) {
        removeLastProperty();
    } else {
        if (!inDictionaryMode()) {
            if (!toDictionaryMode(cx))
                return NULL;

            spp = nativeSearch(id);
            shape = SHAPE_FETCH(spp);
            table = lastProp->table;
            oldLastProp = lastProp;
        }
        shape->removeFromDictionary(this);
    }

#ifdef DEBUG
    if (shape == oldLastProp) {
        JS_ASSERT(lastProp->slotSpan <= shape->slotSpan);
        if (shape->hasSlot())
            JS_ASSERT(shape->slot < shape->slotSpan);
        if (lastProp->slotSpan < numSlots())
            getSlotRef(lastProp->slotSpan).setUndefined();
    }
#endif

    





    if (table)
        SHAPE_STORE_PRESERVING_COLLISION(spp, NULL);

    {
        
        Shape child(id, getter, setter, slot, attrs, flags, shortid);
        shape = getChildProperty(cx, lastProp, child);
    }

    if (shape) {
        JS_ASSERT(shape == lastProp);

        if (table) {
            
            SHAPE_STORE_PRESERVING_COLLISION(spp, shape);

            
            JS_ASSERT(oldLastProp->table == table);
            oldLastProp->setTable(NULL);
            shape->setTable(table);
        }

        if (!lastProp->table) {
            
            lastProp->maybeHash(cx);
        }

        CHECK_SHAPE_CONSISTENCY(this);
        METER(puts);
        return shape;
    }

    if (table)
        SHAPE_STORE_PRESERVING_COLLISION(spp, overwriting);
    CHECK_SHAPE_CONSISTENCY(this);
    METER(putFails);
    return NULL;
}

const Shape *
JSObject::changeProperty(JSContext *cx, const Shape *shape, uintN attrs, uintN mask,
                         PropertyOp getter, PropertyOp setter)
{
    const Shape *newShape;

    JS_ASSERT(!JSID_IS_VOID(shape->id));
    JS_ASSERT(nativeContains(*shape));

    attrs |= shape->attrs & mask;

    
    JS_ASSERT(!((attrs ^ shape->attrs) & JSPROP_SHARED) ||
              !(attrs & JSPROP_SHARED));

    
    JS_ASSERT_IF(getter != shape->rawGetter, !shape->isMethod());

    if (getter == PropertyStub)
        getter = NULL;
    if (setter == PropertyStub)
        setter = NULL;
    if (shape->attrs == attrs && shape->getter() == getter && shape->setter() == setter)
        return shape;

    Shape child(shape->id, getter, setter, shape->slot, attrs, shape->flags, shape->shortid);

    if (inDictionaryMode()) {
        shape->removeFromDictionary(this);
        newShape = Shape::newDictionaryShape(cx, child, &lastProp);
        if (newShape) {
            JS_ASSERT(newShape == lastProp);

            



            const Shape *tableShape = shape->table ? shape : lastProp->parent;

            if (PropertyTable *table = tableShape->table) {
                
                Shape **spp = table->search(shape->id, true);
                SHAPE_STORE_PRESERVING_COLLISION(spp, newShape);

                
                tableShape->setTable(NULL);
                newShape->setTable(table);
            }

            updateFlags(newShape);
            updateShape(cx);
        }
    } else if (shape == lastProp) {
        newShape = getChildProperty(cx, shape->parent, child);
#ifdef DEBUG
        if (newShape) {
            JS_ASSERT(newShape == lastProp);
            if (newShape->table) {
                Shape **spp = nativeSearch(shape->id);
                JS_ASSERT(SHAPE_FETCH(spp) == newShape);
            }
        }
#endif
    } else {
        





        newShape = putProperty(cx, child.id, child.rawGetter, child.rawSetter, child.slot,
                               child.attrs, child.flags, child.shortid);
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
    if (sealed()) {
        reportReadOnlyScope(cx);
        return false;
    }

    Shape **spp = nativeSearch(id);
    Shape *shape = SHAPE_FETCH(spp);
    if (!shape) {
        METER(uselessRemoves);
        return true;
    }

    
    if (shape != lastProp) {
        if (!inDictionaryMode()) {
            if (!toDictionaryMode(cx))
                return false;
            spp = nativeSearch(shape->id);
            shape = SHAPE_FETCH(spp);
        }
        JS_ASSERT(SHAPE_FETCH(spp) == shape);
    }

    
    bool hadSlot = !shape->isAlias() && containsSlot(shape->slot);
    if (hadSlot) {
        freeSlot(cx, shape->slot);
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    



    if (inDictionaryMode()) {
        PropertyTable *table = lastProp->table;

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

        






        setOwnShape(lastProp->shape);

        Shape *oldLastProp = lastProp;
        shape->removeFromDictionary(this);
        if (table) {
            if (shape == oldLastProp) {
                JS_ASSERT(shape->table == table);
                JS_ASSERT(shape->parent == lastProp);
                JS_ASSERT(shape->slotSpan >= lastProp->slotSpan);
                JS_ASSERT_IF(hadSlot, shape->slot + 1 <= shape->slotSpan);

                




                if (table->freelist != SHAPE_INVALID_SLOT)
                    lastProp->slotSpan = shape->slotSpan;
            }

            
            oldLastProp->setTable(NULL);
            lastProp->setTable(table);
        }
    } else {
        




        JS_ASSERT(shape == lastProp);
        removeLastProperty();
    }
    updateShape(cx);

    
    if (PropertyTable *table = lastProp->table) {
        uint32 size = table->capacity();
        if (size > PropertyTable::MIN_SIZE && table->entryCount <= size >> 2) {
            METER(shrinks);
            (void) table->change(cx, -1);
        }
    }

    CHECK_SHAPE_CONSISTENCY(this);
    LIVE_SCOPE_METER(cx, --cx->runtime->liveObjectProps);
    METER(removes);
    return true;
}

void
JSObject::clear(JSContext *cx)
{
    LIVE_SCOPE_METER(cx, cx->runtime->liveObjectProps -= propertyCount());

    Shape *shape = lastProp;
    JS_ASSERT(inDictionaryMode() == shape->inDictionary());

    while (shape->parent) {
        shape = shape->parent;
        JS_ASSERT(inDictionaryMode() == shape->inDictionary());
    }
    JS_ASSERT(shape->isEmptyShape());

    if (inDictionaryMode())
        shape->listp = &lastProp;

    



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
    JS_ASSERT_IF(!parent && JS_ON_TRACE(cx), cx->bailExit);
     LeaveTraceIfGlobalObject(cx, this);

    




    TraceMonitor *tm = &JS_TRACE_MONITOR(cx);
    if (TraceRecorder *tr = tm->recorder)
        tr->forgetGuardedShapesForObject(this);
#endif

    setOwnShape(js_GenerateShape(cx, false));
}

void
JSObject::deletingShapeChange(JSContext *cx, const Shape &shape)
{
    JS_ASSERT(!JSID_IS_VOID(shape.id));
    generateOwnShape(cx);
}

bool
JSObject::methodShapeChange(JSContext *cx, const Shape &shape)
{
    JS_ASSERT(!JSID_IS_VOID(shape.id));
    if (shape.isMethod()) {
#ifdef DEBUG
        const Value &prev = lockedGetSlot(shape.slot);
        JS_ASSERT(&shape.methodObject() == &prev.toObject());
        JS_ASSERT(canHaveMethodBarrier());
        JS_ASSERT(hasMethodBarrier());
        JS_ASSERT(!shape.rawSetter || shape.rawSetter == js_watch_set);
#endif

        





        if (!putProperty(cx, shape.id, NULL, shape.rawSetter, shape.slot,
                         shape.attrs,
                         shape.getFlags() & ~Shape::METHOD,
                         shape.shortid)) {
            return false;
        }
    }

    generateOwnShape(cx);
    return true;
}

bool
JSObject::methodShapeChange(JSContext *cx, uint32 slot)
{
    if (!hasMethodBarrier()) {
        generateOwnShape(cx);
    } else {
        for (Shape::Range r = lastProp->all(); !r.empty(); r.popFront()) {
            const Shape &shape = r.front();
            JS_ASSERT(!JSID_IS_VOID(shape.id));
            if (shape.slot == slot)
                return methodShapeChange(cx, shape);
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
    JS_ASSERT(!JSID_IS_VOID(shape.id));
    generateOwnShape(cx);
}

bool
JSObject::globalObjectOwnShapeChange(JSContext *cx)
{
    generateOwnShape(cx);
    return !js_IsPropertyCacheDisabled(cx);
}

#ifdef DEBUG
static void
PrintPropertyGetterOrSetter(JSTracer *trc, char *buf, size_t bufsize)
{
    Shape *shape;
    jsid id;
    size_t n;
    const char *name;

    JS_ASSERT(trc->debugPrinter == PrintPropertyGetterOrSetter);
    shape = (Shape *)trc->debugPrintArg;
    id = shape->id;
    JS_ASSERT(!JSID_IS_VOID(id));
    name = trc->debugPrintIndex ? js_setter_str : js_getter_str;

    if (JSID_IS_ATOM(id)) {
        n = js_PutEscapedString(buf, bufsize - 1,
                                JSID_TO_STRING(id), 0);
        if (n < bufsize - 1)
            JS_snprintf(buf + n, bufsize - n, " %s", name);
    } else if (JSID_IS_INT(shape->id)) {
        JS_snprintf(buf, bufsize, "%d %s", JSID_TO_INT(id), name);
    } else {
        JS_snprintf(buf, bufsize, "<object> %s", name);
    }
}

static void
PrintPropertyMethod(JSTracer *trc, char *buf, size_t bufsize)
{
    Shape *shape;
    jsid id;
    size_t n;

    JS_ASSERT(trc->debugPrinter == PrintPropertyMethod);
    shape = (Shape *)trc->debugPrintArg;
    id = shape->id;
    JS_ASSERT(!JSID_IS_VOID(id));

    JS_ASSERT(JSID_IS_ATOM(id));
    n = js_PutEscapedString(buf, bufsize - 1, JSID_TO_STRING(id), 0);
    if (n < bufsize - 1)
        JS_snprintf(buf + n, bufsize - n, " method");
}
#endif

void
Shape::trace(JSTracer *trc) const
{
    if (IS_GC_MARKING_TRACER(trc))
        mark();

    MarkId(trc, id, "id");

    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        if ((attrs & JSPROP_GETTER) && rawGetter) {
            JS_SET_TRACING_DETAILS(trc, PrintPropertyGetterOrSetter, this, 0);
            Mark(trc, getterObject(), JSTRACE_OBJECT);
        }
        if ((attrs & JSPROP_SETTER) && rawSetter) {
            JS_SET_TRACING_DETAILS(trc, PrintPropertyGetterOrSetter, this, 1);
            Mark(trc, setterObject(), JSTRACE_OBJECT);
        }
    }

    if (isMethod()) {
        JS_SET_TRACING_DETAILS(trc, PrintPropertyMethod, this, 0);
        Mark(trc, &methodObject(), JSTRACE_OBJECT);
    }
}
