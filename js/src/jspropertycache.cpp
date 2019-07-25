







































#include "jspropertycache.h"
#include "jscntxt.h"
#include "jsnum.h"
#include "jsobjinlines.h"
#include "jspropertycacheinlines.h"

using namespace js;

JS_REQUIRES_STACK PropertyCacheEntry *
PropertyCache::fill(JSContext *cx, JSObject *obj, uintN scopeIndex, JSObject *pobj,
                    const Shape *shape)
{
    JSOp op;
    const JSCodeSpec *cs;
    PropertyCacheEntry *entry;

    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(!cx->runtime->gcRunning);

    



    if (!pobj->nativeContains(cx, *shape)) {
        PCMETER(oddfills++);
        return JS_NO_PROP_CACHE_FILL;
    }

    










    JS_ASSERT_IF(obj == pobj, scopeIndex == 0);

    JSObject *tmp = obj;
    for (uintN i = 0; i != scopeIndex; i++)
        tmp = tmp->getParent();

    uintN protoIndex = 0;
    while (tmp != pobj) {
        tmp = tmp->getProto();

        




        if (!tmp || !tmp->isNative()) {
            PCMETER(noprotos++);
            return JS_NO_PROP_CACHE_FILL;
        }
        ++protoIndex;
    }

    if (scopeIndex > PCINDEX_SCOPEMASK || protoIndex > PCINDEX_PROTOMASK) {
        PCMETER(longchains++);
        return JS_NO_PROP_CACHE_FILL;
    }

    



    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    op = js_GetOpcode(cx, script, pc);
    cs = &js_CodeSpec[op];

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
            



            obj->setDelegate();
        }
    }

    entry = &table[hash(pc, obj->lastProperty())];
    PCMETER(entry->vword.isNull() || recycles++);
    entry->assign(pc, obj->lastProperty(), pobj->lastProperty(), shape, scopeIndex, protoIndex);

    empty = false;
    PCMETER(fills++);

    



    PCMETER(entry == pctestentry || modfills++);
    PCMETER(pctestentry = NULL);
    return entry;
}

static inline JSAtom *
GetAtomFromBytecode(JSContext *cx, jsbytecode *pc, JSOp op, const JSCodeSpec &cs)
{
    if (op == JSOP_LENGTH)
        return cx->runtime->atomState.lengthAtom;

    
    
    if (op == JSOP_INSTANCEOF)
        return cx->runtime->atomState.classPrototypeAtom;

    JSScript *script = cx->stack.currentScript();
    ptrdiff_t pcoff = (JOF_TYPE(cs.format) == JOF_SLOTATOM) ? SLOTNO_LEN : 0;
    JSAtom *atom;
    GET_ATOM_FROM_BYTECODE(script, pc, pcoff, atom);
    return atom;
}

JS_REQUIRES_STACK JSAtom *
PropertyCache::fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp, JSObject **pobjp,
                        PropertyCacheEntry *entry)
{
    JSObject *obj, *pobj, *tmp;
    JSScript *script = cx->stack.currentScript();

    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(uintN((cx->fp()->hasImacropc() ? cx->fp()->imacropc() : pc) - script->code)
              < script->length);

    JSOp op = js_GetOpcode(cx, script, pc);
    const JSCodeSpec &cs = js_CodeSpec[op];

    obj = *objp;
    uint32 vindex = entry->vindex;

    if (entry->kpc != pc) {
        PCMETER(kpcmisses++);

        JSAtom *atom = GetAtomFromBytecode(cx, pc, op, cs);
#ifdef DEBUG_notme
        JSAutoByteString printable;
        fprintf(stderr,
                "id miss for %s from %s:%u"
                " (pc %u, kpc %u, kshape %p, shape %p)\n",
                js_AtomToPrintableString(cx, atom, &printable),
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

        return atom;
    }

    if (entry->kshape != obj->lastProperty()) {
        PCMETER(kshapemisses++);
        return GetAtomFromBytecode(cx, pc, op, cs);
    }

    



    pobj = obj;

    if (JOF_MODE(cs.format) == JOF_NAME) {
        while (vindex & (PCINDEX_SCOPEMASK << PCINDEX_PROTOBITS)) {
            tmp = pobj->getParent();
            if (!tmp || !tmp->isNative())
                break;
            pobj = tmp;
            vindex -= PCINDEX_PROTOSIZE;
        }

        *objp = pobj;
    }

    while (vindex & PCINDEX_PROTOMASK) {
        tmp = pobj->getProto();
        if (!tmp || !tmp->isNative())
            break;
        pobj = tmp;
        --vindex;
    }

    if (pobj->lastProperty() == entry->pshape) {
#ifdef DEBUG
        JSAtom *atom = GetAtomFromBytecode(cx, pc, op, cs);
        jsid id = ATOM_TO_JSID(atom);

        id = js_CheckForStringIndex(id);
        JS_ASSERT(pobj->nativeContains(cx, id));
#endif
        *pobjp = pobj;
        return NULL;
    }

    PCMETER(vcapmisses++);
    return GetAtomFromBytecode(cx, pc, op, cs);
}

#ifdef DEBUG
void
PropertyCache::assertEmpty()
{
    JS_ASSERT(empty);
    for (uintN i = 0; i < SIZE; i++) {
        JS_ASSERT(!table[i].kpc);
        JS_ASSERT(!table[i].kshape);
        JS_ASSERT(!table[i].pshape);
        JS_ASSERT(!table[i].prop);
        JS_ASSERT(!table[i].vindex);
    }
}
#endif

void
PropertyCache::purge(JSContext *cx)
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
#ifdef JS_THREADSAFE
        fprintf(fp, "thread %lu, ", (unsigned long) cx->thread->id);
#endif
        fprintf(fp, "GC %u\n", cx->runtime->gcNumber);

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
