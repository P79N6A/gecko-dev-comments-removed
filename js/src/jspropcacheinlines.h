






































#ifndef jspropcacheinlines_h___
#define jspropcacheinlines_h___

inline JSBool
js_IsPropertyCacheDisabled(JSContext *cx)
{
    return cx->runtime->shapeGen >= SHAPE_OVERFLOW_BIT;
}






inline jsuword
PROPERTY_CACHE_HASH(jsuword pc, jsuword kshape)
{
    return (((pc >> PROPERTY_CACHE_LOG2) ^ pc) + (kshape)) & PROPERTY_CACHE_MASK;
}

inline jsuword
PROPERTY_CACHE_HASH_PC(jsbytecode *pc, jsuword kshape)
{
    return PROPERTY_CACHE_HASH((jsuword) pc, kshape);
}

inline jsuword
PROPERTY_CACHE_HASH_ATOM(JSAtom *atom, JSObject *obj)
{
    return PROPERTY_CACHE_HASH((jsuword) atom >> 2, OBJ_SHAPE(obj));
}














#define PROPERTY_CACHE_TEST(cx, pc, obj, pobj, entry, atom) \
    JS_PROPERTY_CACHE(cx).test(cx, pc, &obj, &pobj, &entry, &atom)

JS_ALWAYS_INLINE void
JSPropertyCache::test(JSContext *cx, jsbytecode *pc, JSObject **objp,
                      JSObject **pobjp, JSPropCacheEntry **entryp, JSAtom **atomp)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(OBJ_IS_NATIVE(*objp));

    uint32 kshape = OBJ_SHAPE(*objp);
    JSPropCacheEntry *entry = &table[PROPERTY_CACHE_HASH_PC(pc, kshape)];
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    JS_ASSERT(objp != pobjp);
    if (entry->kpc == pc && entry->kshape == kshape) {
        JSObject *tmp;
        JSObject *pobj = *objp;

        JS_ASSERT(PCVCAP_TAG(entry->vcap) <= 1);
        if (PCVCAP_TAG(entry->vcap) == 1 && (tmp = pobj->getProto()) != NULL)
            pobj = tmp;

        if (JS_LOCK_OBJ_IF_SHAPE(cx, pobj, PCVCAP_SHAPE(entry->vcap))) {
            PCMETER(pchits++);
            PCMETER(!PCVCAP_TAG(entry->vcap) || protopchits++);
            *pobjp = pobj;
            *entryp = entry;
            *atomp = NULL;
            return;
        }
    }
    *atomp = fullTest(cx, pc, objp, pobjp, entryp);
    if (*atomp)
        PCMETER(misses++);
}

JS_ALWAYS_INLINE bool
JSPropertyCache::testForSet(JSContext *cx, jsbytecode *pc, JSObject **objp,
                            JSObject **pobjp, JSPropCacheEntry **entryp, JSAtom **atomp)
{
    uint32 kshape = OBJ_SHAPE(*objp);
    JSPropCacheEntry *entry = &table[PROPERTY_CACHE_HASH_PC(pc, kshape)];
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    PCMETER(settests++);

    *entryp = entry;
    if (entry->kpc == pc && entry->kshape == kshape) {
        JS_ASSERT(PCVCAP_TAG(entry->vcap) <= 1);
        if (JS_LOCK_OBJ_IF_SHAPE(cx, *objp, kshape))
            return true;
    }

    *atomp = JS_PROPERTY_CACHE(cx).fullTest(cx, pc, objp, pobjp, entryp);
    if (*atomp) {
        PCMETER(cache->misses++);
        PCMETER(cache->setmisses++);
    }

    return false;
}

JS_ALWAYS_INLINE bool
JSPropertyCache::testForInit(JSContext *cx, jsbytecode *pc, JSObject *obj,
                             JSPropCacheEntry **entryp, JSScopeProperty **spropp)
{
    uint32 shape = OBJ_SCOPE(obj)->shape;
    JSPropCacheEntry *entry = &table[PROPERTY_CACHE_HASH_PC(pc, shape)];
    PCMETER(pctestentry = entry);
    PCMETER(tests++);
    PCMETER(initests++);

    if (entry->kpc == pc &&
        entry->kshape == shape &&
        PCVCAP_SHAPE(entry->vcap) == cx->runtime->protoHazardShape)
    {
        JS_ASSERT(PCVCAP_TAG(entry->vcap) == 0);
        PCMETER(cache->pchits++);
        PCMETER(cache->inipchits++);
        JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
        JSScopeProperty *sprop = PCVAL_TO_SPROP(entry->vword);
        JS_ASSERT(!(sprop->attrs & JSPROP_READONLY));
        *spropp = sprop;
        *entryp = entry;
        return true;
    }
    return false;
}

inline JSPropCacheEntry *
JSPropertyCache::fillEntry(jsuword khash, jsbytecode *pc, jsuword kshape,
                           jsuword vcap, jsuword vword)
{
    JSPropCacheEntry *entry = &table[khash];
    PCMETER(PCVAL_IS_NULL(entry->vword) || recycles++);
    entry->kpc = pc;
    entry->kshape = kshape;
    entry->vcap = vcap;
    entry->vword = vword;

    empty = JS_FALSE;
    PCMETER(fills++);

    



    PCMETER(entry == pctestentry || modfills++);
    PCMETER(pctestentry = NULL);
    return entry;
}

inline JSPropCacheEntry *
JSPropertyCache::fillByPC(jsbytecode *pc, jsuword kshape,
                          jsuword vshape, jsuword scopeIndex, jsuword protoIndex,
                          jsuword vword)
{
    return fillEntry(PROPERTY_CACHE_HASH_PC(pc, kshape), pc, kshape,
                     PCVCAP_MAKE(vshape, scopeIndex, protoIndex), vword);
}

inline JSPropCacheEntry *
JSPropertyCache::fillByAtom(JSAtom *atom, JSObject *obj,
                            jsuword vshape, jsuword scopeIndex, jsuword protoIndex,
                            jsuword vword)
{
    JS_ASSERT(obj->isDelegate());
    jsuword khash = PROPERTY_CACHE_HASH_ATOM(atom, obj);
    PCMETER(if (PCVCAP_TAG(table[khash].vcap) <= 1)
                pcrecycles++);
    return fillEntry(khash,
                     reinterpret_cast<jsbytecode *>(atom),
                     reinterpret_cast<jsuword>(obj),
                     PCVCAP_MAKE(vshape, scopeIndex, protoIndex),
                     vword);
}

#endif 
