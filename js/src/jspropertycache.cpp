






#include "jspropertycache.h"
#include "jscntxt.h"
#include "jsnum.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"
#include "jspropertycacheinlines.h"

using namespace js;

PropertyCacheEntry *
PropertyCache::fill(JSContext *cx, JSObject *obj, unsigned scopeIndex, JSObject *pobj,
                    const Shape *shape)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(!cx->runtime->gcRunning);

    










    JS_ASSERT_IF(obj == pobj, scopeIndex == 0);

    JSObject *tmp = obj;
    for (unsigned i = 0; i < scopeIndex; i++)
        tmp = &tmp->asScope().enclosingScope();

    unsigned protoIndex = 0;
    while (tmp != pobj) {
        



        if (tmp->hasUncacheableProto()) {
            PCMETER(noprotos++);
            return JS_NO_PROP_CACHE_FILL;
        }

        tmp = tmp->getProto();

        




        if (!tmp || !tmp->isNative()) {
            PCMETER(noprotos++);
            return JS_NO_PROP_CACHE_FILL;
        }
        ++protoIndex;
    }

    typedef PropertyCacheEntry Entry;
    if (scopeIndex > Entry::MaxScopeIndex || protoIndex > Entry::MaxProtoIndex) {
        PCMETER(longchains++);
        return JS_NO_PROP_CACHE_FILL;
    }

    



    jsbytecode *pc;
    (void) cx->stack.currentScript(&pc);
    JSOp op = JSOp(*pc);
    const JSCodeSpec *cs = &js_CodeSpec[op];

    if ((cs->format & JOF_SET) && obj->watched())
        return JS_NO_PROP_CACHE_FILL;

    if (obj == pobj) {
        JS_ASSERT(scopeIndex == 0 && protoIndex == 0);
    } else {
#ifdef DEBUG
        if (scopeIndex == 0) {
            JS_ASSERT(protoIndex != 0);
            JS_ASSERT((protoIndex == 1) == (obj->getProto() == pobj));
        }
#endif

        if (scopeIndex != 0 || protoIndex != 1) {
            



            if (!obj->isDelegate())
                return JS_NO_PROP_CACHE_FILL;
        }
    }

    PropertyCacheEntry *entry = &table[hash(pc, obj->lastProperty())];
    PCMETER(entry->vword.isNull() || recycles++);
    entry->assign(pc, obj->lastProperty(), pobj->lastProperty(), shape, scopeIndex, protoIndex);

    empty = false;
    PCMETER(fills++);

    



    PCMETER(entry == pctestentry || modfills++);
    PCMETER(pctestentry = NULL);
    return entry;
}

PropertyName *
PropertyCache::fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp, JSObject **pobjp,
                        PropertyCacheEntry *entry)
{
    JSObject *obj, *pobj, *tmp;
#ifdef DEBUG
    JSScript *script = cx->stack.currentScript();
#endif

    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(uint32_t(pc - script->code) < script->length);

    JSOp op = JSOp(*pc);
    const JSCodeSpec &cs = js_CodeSpec[op];

    obj = *objp;

    if (entry->kpc != pc) {
        PCMETER(kpcmisses++);

        PropertyName *name = GetNameFromBytecode(cx, pc, op, cs);
#ifdef DEBUG_notme
        JSAutoByteString printable;
        fprintf(stderr,
                "id miss for %s from %s:%u"
                " (pc %u, kpc %u, kshape %p, shape %p)\n",
                js_AtomToPrintableString(cx, name, &printable),
                script->filename,
                js_PCToLineNumber(cx, script, pc),
                pc - script->code,
                entry->kpc - script->code,
                entry->kshape,
                obj->lastProperty());
                js_Disassemble1(cx, script, pc,
                                pc - script->code,
                                JS_FALSE, stderr);
#endif

        return name;
    }

    if (entry->kshape != obj->lastProperty()) {
        PCMETER(kshapemisses++);
        return GetNameFromBytecode(cx, pc, op, cs);
    }

    



    pobj = obj;

    JSObject *scopeObj = NULL;
    if (JOF_MODE(cs.format) == JOF_NAME) {
        uint8_t scopeIndex = entry->scopeIndex;
        while (scopeIndex > 0) {
            tmp = pobj->enclosingScope();
            if (!tmp || !tmp->isNative())
                break;
            pobj = tmp;
            scopeIndex--;
        }

        scopeObj = pobj;
    }

    uint8_t protoIndex = entry->protoIndex;
    while (protoIndex > 0) {
        tmp = pobj->getProto();
        if (!tmp || !tmp->isNative())
            break;
        pobj = tmp;
        protoIndex--;
    }

    if (pobj->lastProperty() == entry->pshape) {
        if (JOF_MODE(cs.format) == JOF_NAME)
            *objp = scopeObj;
#ifdef DEBUG
        PropertyName *name = GetNameFromBytecode(cx, pc, op, cs);
        JS_ASSERT(pobj->nativeContains(cx, NameToId(name)));
#endif
        *pobjp = pobj;
        return NULL;
    }

    PCMETER(vcapmisses++);
    return GetNameFromBytecode(cx, pc, op, cs);
}

#ifdef DEBUG
void
PropertyCache::assertEmpty()
{
    JS_ASSERT(empty);
    for (unsigned i = 0; i < SIZE; i++) {
        JS_ASSERT(!table[i].kpc);
        JS_ASSERT(!table[i].kshape);
        JS_ASSERT(!table[i].pshape);
        JS_ASSERT(!table[i].prop);
        JS_ASSERT(!table[i].scopeIndex);
        JS_ASSERT(!table[i].protoIndex);
    }
}
#endif

void
PropertyCache::purge(JSRuntime *rt)
{
    if (empty) {
        assertEmpty();
        return;
    }

    PodArrayZero(table);
    empty = true;

#ifdef JS_PROPERTY_CACHE_METERING
  { static FILE *fp;
    if (!fp)
        fp = fopen("/tmp/propcache.stats", "w");
    if (fp) {
        fputs("Property cache stats for ", fp);
        fprintf(fp, "GC %lu\n", (unsigned long)rt->gcNumber);

# define P(mem) fprintf(fp, "%11s %10lu\n", #mem, (unsigned long)mem)
        P(fills);
        P(nofills);
        P(rofills);
        P(disfills);
        P(oddfills);
        P(add2dictfills);
        P(modfills);
        P(brandfills);
        P(noprotos);
        P(longchains);
        P(recycles);
        P(tests);
        P(pchits);
        P(protopchits);
        P(initests);
        P(inipchits);
        P(inipcmisses);
        P(settests);
        P(addpchits);
        P(setpchits);
        P(setpcmisses);
        P(setmisses);
        P(kpcmisses);
        P(kshapemisses);
        P(vcapmisses);
        P(misses);
        P(flushes);
        P(pcpurges);
# undef P

        fprintf(fp, "hit rates: pc %g%% (proto %g%%), set %g%%, ini %g%%, full %g%%\n",
                (100. * pchits) / tests,
                (100. * protopchits) / tests,
                (100. * (addpchits + setpchits))
                / settests,
                (100. * inipchits) / initests,
                (100. * (tests - misses)) / tests);
        fflush(fp);
    }
  }
#endif

    PCMETER(flushes++);
}

void
PropertyCache::restore(PropertyCacheEntry *entry)
{
    PropertyCacheEntry *entry2;

    empty = false;

    entry2 = &table[hash(entry->kpc, entry->kshape)];
    *entry2 = *entry;
}
