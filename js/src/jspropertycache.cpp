







































#include "jspropertycache.h"
#include "jspropertycacheinlines.h"
#include "jscntxt.h"

using namespace js;

JS_STATIC_ASSERT(sizeof(PCVal) == sizeof(jsuword));

JS_REQUIRES_STACK PropertyCacheEntry *
PropertyCache::fill(JSContext *cx, JSObject *obj, uintN scopeIndex, uintN protoIndex,
                    JSObject *pobj, JSScopeProperty *sprop, JSBool adding)
{
    jsbytecode *pc;
    JSScope *scope;
    jsuword kshape, vshape;
    JSOp op;
    const JSCodeSpec *cs;
    PCVal vword;
    PropertyCacheEntry *entry;

    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(!cx->runtime->gcRunning);

    
    if (js_IsPropertyCacheDisabled(cx) || (cx->fp->flags & JSFRAME_EVAL)) {
        PCMETER(disfills++);
        return JS_NO_PROP_CACHE_FILL;
    }

    



    scope = OBJ_SCOPE(pobj);
    if (!scope->hasProperty(sprop)) {
        PCMETER(oddfills++);
        return JS_NO_PROP_CACHE_FILL;
    }

    












    JS_ASSERT_IF(scopeIndex == 0 && protoIndex == 0, obj == pobj);

    if (protoIndex != 0) {
        JSObject *tmp = obj;

        for (uintN i = 0; i != scopeIndex; i++)
            tmp = tmp->getParent();
        JS_ASSERT(tmp != pobj);

        protoIndex = 1;
        for (;;) {
            tmp = tmp->getProto();

            




            if (!tmp || !tmp->isNative()) {
                PCMETER(noprotos++);
                return JS_NO_PROP_CACHE_FILL;
            }
            if (tmp == pobj)
                break;
            ++protoIndex;
        }
    }

    if (scopeIndex > PCVCAP_SCOPEMASK || protoIndex > PCVCAP_PROTOMASK) {
        PCMETER(longchains++);
        return JS_NO_PROP_CACHE_FILL;
    }

    



    pc = cx->fp->regs->pc;
    op = js_GetOpcode(cx, cx->fp->script, pc);
    cs = &js_CodeSpec[op];
    kshape = 0;

    do {
        




        if (cs->format & JOF_CALLOP) {
            jsval v;

            if (sprop->isMethod()) {
                



                JS_ASSERT(scope->hasMethodBarrier());
                v = sprop->methodValue();
                JS_ASSERT(VALUE_IS_FUNCTION(cx, v));
                JS_ASSERT(v == LOCKED_OBJ_GET_SLOT(pobj, sprop->slot));
                vword.setObject(JSVAL_TO_OBJECT(v));
                break;
            }

            if (!scope->generic() &&
                sprop->hasDefaultGetter() &&
                SPROP_HAS_VALID_SLOT(sprop, scope)) {
                v = LOCKED_OBJ_GET_SLOT(pobj, sprop->slot);
                if (VALUE_IS_FUNCTION(cx, v)) {
                    











                    if (!scope->branded()) {
                        PCMETER(brandfills++);
#ifdef DEBUG_notme
                        fprintf(stderr,
                                "branding %p (%s) for funobj %p (%s), shape %lu\n",
                                pobj, pobj->getClass()->name,
                                JSVAL_TO_OBJECT(v),
                                JS_GetFunctionName(GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(v))),
                                OBJ_SHAPE(obj));
#endif
                        if (!scope->brand(cx, sprop->slot, v))
                            return JS_NO_PROP_CACHE_FILL;
                    }
                    vword.setObject(JSVAL_TO_OBJECT(v));
                    break;
                }
            }
        }

        



        if (!(cs->format & (JOF_SET | JOF_FOR)) &&
            (!(cs->format & JOF_INCDEC) || sprop->hasDefaultSetter()) &&
            sprop->hasDefaultGetter() &&
            SPROP_HAS_VALID_SLOT(sprop, scope)) {
            
            vword.setSlot(sprop->slot);
        } else {
            
            vword.setSprop(sprop);
            if (adding &&
                sprop == scope->lastProperty() &&
                scope->shape == sprop->shape) {
                


























                JS_ASSERT(!scope->isSharedEmpty());
                if (sprop->parent) {
                    kshape = sprop->parent->shape;
                } else {
                    





                    JSObject *proto = obj->getProto();
                    if (!proto || !proto->isNative())
                        return JS_NO_PROP_CACHE_FILL;
                    JSScope *protoscope = OBJ_SCOPE(proto);
                    if (!protoscope->emptyScope ||
                        protoscope->emptyScope->clasp != obj->getClass()) {
                        return JS_NO_PROP_CACHE_FILL;
                    }
                    kshape = protoscope->emptyScope->shape;
                }

                



                vshape = cx->runtime->protoHazardShape;
            }
        }
    } while (0);

    if (kshape == 0) {
        kshape = OBJ_SHAPE(obj);
        vshape = scope->shape;
    }
    JS_ASSERT(kshape < SHAPE_OVERFLOW_BIT);

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
    JS_ASSERT(vshape < SHAPE_OVERFLOW_BIT);

    entry = &table[hash(pc, kshape)];
    PCMETER(entry->vword.isNull() || recycles++);
    entry->assign(pc, kshape, vshape, scopeIndex, protoIndex, vword);

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

    ptrdiff_t pcoff = (JOF_TYPE(cs.format) == JOF_SLOTATOM) ? SLOTNO_LEN : 0;
    JSAtom *atom;
    GET_ATOM_FROM_BYTECODE(cx->fp->script, pc, pcoff, atom);
    return atom;
}

JS_REQUIRES_STACK JSAtom *
PropertyCache::fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp, JSObject **pobjp,
                        PropertyCacheEntry *entry)
{
    JSObject *obj, *pobj, *tmp;
    uint32 vcap;

    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));
    JS_ASSERT(uintN((cx->fp->imacpc ? cx->fp->imacpc : pc) - cx->fp->script->code)
              < cx->fp->script->length);

    JSOp op = js_GetOpcode(cx, cx->fp->script, pc);
    const JSCodeSpec &cs = js_CodeSpec[op];

    obj = *objp;
    vcap = entry->vcap;

    if (entry->kpc != pc) {
        PCMETER(kpcmisses++);

        JSAtom *atom = GetAtomFromBytecode(cx, pc, op, cs);
#ifdef DEBUG_notme
        fprintf(stderr,
                "id miss for %s from %s:%u"
                " (pc %u, kpc %u, kshape %u, shape %u)\n",
                js_AtomToPrintableString(cx, atom),
                cx->fp->script->filename,
                js_PCToLineNumber(cx, cx->fp->script, pc),
                pc - cx->fp->script->code,
                entry->kpc - cx->fp->script->code,
                entry->kshape,
                OBJ_SHAPE(obj));
                js_Disassemble1(cx, cx->fp->script, pc,
                                pc - cx->fp->script->code,
                                JS_FALSE, stderr);
#endif

        return atom;
    }

    if (entry->kshape != obj->map->shape) {
        PCMETER(kshapemisses++);
        return GetAtomFromBytecode(cx, pc, op, cs);
    }

    




    pobj = obj;

    if (JOF_MODE(cs.format) == JOF_NAME) {
        while (vcap & (PCVCAP_SCOPEMASK << PCVCAP_PROTOBITS)) {
            tmp = pobj->getParent();
            if (!tmp || !tmp->isNative())
                break;
            pobj = tmp;
            vcap -= PCVCAP_PROTOSIZE;
        }

        *objp = pobj;
    }

    while (vcap & PCVCAP_PROTOMASK) {
        tmp = pobj->getProto();
        if (!tmp || !tmp->isNative())
            break;
        pobj = tmp;
        --vcap;
    }

    if (matchShape(cx, pobj, vcap >> PCVCAP_TAGBITS)) {
#ifdef DEBUG
        JSAtom *atom = GetAtomFromBytecode(cx, pc, op, cs);
        jsid id = ATOM_TO_JSID(atom);

        id = js_CheckForStringIndex(id);
        JS_ASSERT(OBJ_SCOPE(pobj)->lookup(id));
        JS_ASSERT_IF(OBJ_SCOPE(pobj)->object, OBJ_SCOPE(pobj)->object == pobj);
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
        JS_ASSERT(!table[i].vcap);
        JS_ASSERT(table[i].vword.isNull());
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
    JS_ASSERT(table[0].vword.isNull());
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
PropertyCache::purgeForScript(JSScript *script)
{
    for (PropertyCacheEntry *entry = table; entry < table + SIZE; entry++) {
        if (JS_UPTRDIFF(entry->kpc, script->code) < script->length) {
            entry->kpc = NULL;
#ifdef DEBUG
            entry->kshape = entry->vcap = 0;
            entry->vword.setNull();
#endif
        }
    }
}
