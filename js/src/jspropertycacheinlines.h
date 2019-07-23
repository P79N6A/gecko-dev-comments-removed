








































#ifndef jspropertycacheinlines_h___
#define jspropertycacheinlines_h___

#include "jsinterp.h"
#include "jslock.h"
#include "jsscope.h"

 inline bool
JSPropCacheEntry::matchShape(JSContext *cx, JSObject *obj, uint32 shape)
{
    return CX_OWNS_OBJECT_TITLE(cx, obj) && OBJ_SHAPE(obj) == shape;
}

















#define PROPERTY_CACHE_TEST(cx, pc, obj, pobj, entry, atom)                   \
    do {                                                                      \
        JSPropertyCache *cache_ = &JS_PROPERTY_CACHE(cx);                     \
        uint32 kshape_ = (JS_ASSERT(OBJ_IS_NATIVE(obj)), OBJ_SHAPE(obj));     \
        entry = &cache_->table[PROPERTY_CACHE_HASH(pc, kshape_)];             \
        PCMETER(cache_->pctestentry = entry);                                 \
        PCMETER(cache_->tests++);                                             \
        JS_ASSERT(&obj != &pobj);                                             \
        if (entry->kpc == pc && entry->kshape == kshape_) {                   \
            JSObject *tmp_;                                                   \
            pobj = obj;                                                       \
            if (PCVCAP_TAG(entry->vcap) == 1 &&                               \
                (tmp_ = pobj->getProto()) != NULL) {                          \
                pobj = tmp_;                                                  \
            }                                                                 \
                                                                              \
            if (JSPropCacheEntry::matchShape(cx, pobj,                        \
                                             PCVCAP_SHAPE(entry->vcap))) {    \
                PCMETER(cache_->pchits++);                                    \
                PCMETER(!PCVCAP_TAG(entry->vcap) || cache_->protopchits++);   \
                atom = NULL;                                                  \
                break;                                                        \
            }                                                                 \
        }                                                                     \
        atom = js_FullTestPropertyCache(cx, pc, &obj, &pobj, entry);          \
        if (atom)                                                             \
            PCMETER(cache_->misses++);                                        \
    } while (0)

#endif 
