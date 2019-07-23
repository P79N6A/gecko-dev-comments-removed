








































#ifndef jspropertycacheinlines_h___
#define jspropertycacheinlines_h___

#include "jspropertycache.h"
#include "jsscope.h"

 inline bool
JSPropCacheEntry::matchShape(JSContext *cx, JSObject *obj, uint32 shape)
{
    return CX_OWNS_OBJECT_TITLE(cx, obj) && OBJ_SHAPE(obj) == shape;
}

















JS_ALWAYS_INLINE void
PROPERTY_CACHE_TEST(JSContext *cx, jsbytecode *pc, JSObject *&obj,
		    JSObject *&pobj, JSPropCacheEntry *&entry, JSAtom *&atom)
{
    JSPropertyCache *cache = &JS_PROPERTY_CACHE(cx);
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    uint32 kshape = OBJ_SHAPE(obj);
    entry = &cache->table[PROPERTY_CACHE_HASH(pc, kshape)];
    PCMETER(cache->pctestentry = entry);
    PCMETER(cache->tests++);
    JS_ASSERT(&obj != &pobj);
    if (entry->kpc == pc && entry->kshape == kshape) {
        JSObject *tmp;
        pobj = obj;
        if (PCVCAP_TAG(entry->vcap) == 1 &&
            (tmp = pobj->getProto()) != NULL) {
            pobj = tmp;
        }

        if (JSPropCacheEntry::matchShape(cx, pobj, PCVCAP_SHAPE(entry->vcap))) {
            PCMETER(cache->pchits++);
            PCMETER(!PCVCAP_TAG(entry->vcap) || cache->protopchits++);
            atom = NULL;
            return;
        }
    }
    atom = js_FullTestPropertyCache(cx, pc, &obj, &pobj, entry);
    if (atom)
        PCMETER(cache->misses++);
}

#endif 
