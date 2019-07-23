








































#ifndef jspropertycacheinlines_h___
#define jspropertycacheinlines_h___

#include "jspropertycache.h"
#include "jsscope.h"

using namespace js;

 inline bool
PropertyCache::matchShape(JSContext *cx, JSObject *obj, uint32 shape)
{
    return CX_OWNS_OBJECT_TITLE(cx, obj) && obj->shape() == shape;
}
















JS_ALWAYS_INLINE void
PropertyCache::test(JSContext *cx, jsbytecode *pc, JSObject *&obj,
                    JSObject *&pobj, PropertyCacheEntry *&entry, JSAtom *&atom)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));

    uint32 kshape = obj->map->shape;
    entry = &table[hash(pc, kshape)];
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    JS_ASSERT(&obj != &pobj);
    JS_ASSERT(entry->kshape < SHAPE_OVERFLOW_BIT);
    if (entry->kpc == pc && entry->kshape == kshape) {
        JSObject *tmp;
        pobj = obj;
        if (entry->vcapTag() == 1 &&
            (tmp = pobj->getProto()) != NULL) {
            pobj = tmp;
        }

        if (matchShape(cx, pobj, entry->vshape())) {
            PCMETER(pchits++);
            PCMETER(!entry->vcapTag() || protopchits++);
            atom = NULL;
            return;
        }
    }
    atom = fullTest(cx, pc, &obj, &pobj, entry);
    if (atom)
        PCMETER(misses++);
}

JS_ALWAYS_INLINE bool
PropertyCache::testForSet(JSContext *cx, jsbytecode *pc, JSObject *obj,
                          PropertyCacheEntry **entryp, JSObject **obj2p, JSAtom **atomp)
{
    uint32 shape = obj->map->shape;
    PropertyCacheEntry *entry = &table[hash(pc, shape)];
    *entryp = entry;
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    PCMETER(settests++);
    JS_ASSERT(entry->kshape < SHAPE_OVERFLOW_BIT);
    if (entry->kpc == pc && entry->kshape == shape && matchShape(cx, obj, shape))
        return true;

#ifdef DEBUG
    JSObject *orig = obj;
#endif
    JSAtom *atom = fullTest(cx, pc, &obj, obj2p, entry);
    if (atom) {
        PCMETER(misses++);
        PCMETER(setmisses++);
    } else {
        JS_ASSERT(obj == orig);
    }
    *atomp = atom;
    return false;
}

JS_ALWAYS_INLINE bool
PropertyCache::testForInit(JSRuntime *rt, jsbytecode *pc, JSObject *obj, JSScope *scope,
                           JSScopeProperty **spropp, PropertyCacheEntry **entryp)
{
    JS_ASSERT(scope->object == obj);
    JS_ASSERT(!scope->sealed());
    uint32 kshape = scope->shape;
    PropertyCacheEntry *entry = &table[hash(pc, kshape)];
    *entryp = entry;
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    PCMETER(initests++);
    JS_ASSERT(entry->kshape < SHAPE_OVERFLOW_BIT);

    if (entry->kpc == pc &&
        entry->kshape == kshape &&
        entry->vshape() == rt->protoHazardShape) {
        PCMETER(pchits++);
        PCMETER(inipchits++);
        JS_ASSERT(entry->vcapTag() == 0);
        *spropp = entry->vword.toSprop();
        JS_ASSERT((*spropp)->writable());
        return true;
    }
    return false;
}

#endif 
