








































#ifndef jspropertycacheinlines_h___
#define jspropertycacheinlines_h___

#include "jspropertycache.h"
#include "jsscope.h"

using namespace js;

 inline bool
PropertyCache::matchShape(JSContext *cx, JSObject *obj, uint32 shape)
{
    return CX_OWNS_OBJECT_TITLE(cx, obj) && OBJ_SHAPE(obj) == shape;
}
















JS_ALWAYS_INLINE void
PropertyCache::test(JSContext *cx, jsbytecode *pc, JSObject *&obj,
                    JSObject *&pobj, PropertyCacheEntry *&entry, JSAtom *&atom)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(OBJ_IS_NATIVE(obj));

    uint32 kshape = OBJ_SHAPE(obj);
    entry = &table[hash(pc, kshape)];
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    JS_ASSERT(&obj != &pobj);
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

#endif 
