








































#ifndef jspropertycacheinlines_h___
#define jspropertycacheinlines_h___

#include "jslock.h"
#include "jspropertycache.h"
#include "jsscope.h"

using namespace js;
















JS_ALWAYS_INLINE void
PropertyCache::test(JSContext *cx, jsbytecode *pc, JSObject *&obj,
                    JSObject *&pobj, PropertyCacheEntry *&entry, PropertyName *&name)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));

    const Shape *kshape = obj->lastProperty();
    entry = &table[hash(pc, kshape)];
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    JS_ASSERT(&obj != &pobj);
    if (entry->kpc == pc && entry->kshape == kshape) {
        JSObject *tmp;
        pobj = obj;
        if (entry->isPrototypePropertyHit() &&
            (tmp = pobj->getProto()) != NULL) {
            pobj = tmp;
        }

        if (pobj->lastProperty() == entry->pshape) {
            PCMETER(pchits++);
            PCMETER(entry->isOwnPropertyHit() || protopchits++);
            name = NULL;
            return;
        }
    }
    name = fullTest(cx, pc, &obj, &pobj, entry);
    if (name)
        PCMETER(misses++);
}

JS_ALWAYS_INLINE bool
PropertyCache::testForSet(JSContext *cx, jsbytecode *pc, JSObject *obj,
                          PropertyCacheEntry **entryp, JSObject **obj2p, PropertyName **namep)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));

    const Shape *kshape = obj->lastProperty();
    PropertyCacheEntry *entry = &table[hash(pc, kshape)];
    *entryp = entry;
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    PCMETER(settests++);
    if (entry->kpc == pc && entry->kshape == kshape)
        return true;

    PropertyName *name = fullTest(cx, pc, &obj, obj2p, entry);
    JS_ASSERT(name);

    PCMETER(misses++);
    PCMETER(setmisses++);

    *namep = name;
    return false;
}

#endif 
