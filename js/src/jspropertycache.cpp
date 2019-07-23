




































#include "jspropertycache.h"
#include "jspropertycacheinlines.h"
#include "jscntxt.h"

using namespace js;

JS_REQUIRES_STACK JSPropCacheEntry *
js_FillPropertyCache(JSContext *cx, JSObject *obj,
                     uintN scopeIndex, uintN protoIndex, JSObject *pobj,
                     JSScopeProperty *sprop, JSBool adding)
{
    JSPropertyCache *cache;
    jsbytecode *pc;
    JSScope *scope;
    jsuword kshape, vshape;
    JSOp op;
    const JSCodeSpec *cs;
    jsuword vword;
    JSPropCacheEntry *entry;

    JS_ASSERT(!cx->runtime->gcRunning);
    cache = &JS_PROPERTY_CACHE(cx);

    
    if (js_IsPropertyCacheDisabled(cx) || (cx->fp->flags & JSFRAME_EVAL)) {
        PCMETER(cache->disfills++);
        return JS_NO_PROP_CACHE_FILL;
    }

    



    scope = OBJ_SCOPE(pobj);
    if (!scope->hasProperty(sprop)) {
        PCMETER(cache->oddfills++);
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

            




            if (!tmp || !OBJ_IS_NATIVE(tmp)) {
                PCMETER(cache->noprotos++);
                return JS_NO_PROP_CACHE_FILL;
            }
            if (tmp == pobj)
                break;
            ++protoIndex;
        }
    }

    if (scopeIndex > PCVCAP_SCOPEMASK || protoIndex > PCVCAP_PROTOMASK) {
        PCMETER(cache->longchains++);
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
                vword = JSVAL_OBJECT_TO_PCVAL(v);
                break;
            }

            if (!scope->generic() &&
                sprop->hasDefaultGetter() &&
                SPROP_HAS_VALID_SLOT(sprop, scope)) {
                v = LOCKED_OBJ_GET_SLOT(pobj, sprop->slot);
                if (VALUE_IS_FUNCTION(cx, v)) {
                    











                    if (!scope->branded()) {
                        PCMETER(cache->brandfills++);
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
                    vword = JSVAL_OBJECT_TO_PCVAL(v);
                    break;
                }
            }
        }

        
        if (!(cs->format & (JOF_SET | JOF_INCDEC | JOF_FOR)) &&
            sprop->hasDefaultGetter() &&
            SPROP_HAS_VALID_SLOT(sprop, scope)) {
            
            vword = SLOT_TO_PCVAL(sprop->slot);
        } else {
            
            vword = SPROP_TO_PCVAL(sprop);
            if (adding &&
                sprop == scope->lastProperty() &&
                scope->shape == sprop->shape) {
                


























                JS_ASSERT(!scope->isSharedEmpty());
                if (sprop->parent) {
                    kshape = sprop->parent->shape;
                } else {
                    





                    JSObject *proto = obj->getProto();
                    if (!proto || !OBJ_IS_NATIVE(proto))
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

    entry = &cache->table[PROPERTY_CACHE_HASH(pc, kshape)];
    PCMETER(PCVAL_IS_NULL(entry->vword) || cache->recycles++);
    entry->kpc = pc;
    entry->kshape = kshape;
    entry->vcap = PCVCAP_MAKE(vshape, scopeIndex, protoIndex);
    entry->vword = vword;

    cache->empty = JS_FALSE;
    PCMETER(cache->fills++);

    



    PCMETER(entry == cache->pctestentry || cache->modfills++);
    PCMETER(cache->pctestentry = NULL);
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
js_FullTestPropertyCache(JSContext *cx, jsbytecode *pc,
                         JSObject **objp, JSObject **pobjp,
                         JSPropCacheEntry *entry)
{
    JSObject *obj, *pobj, *tmp;
    uint32 vcap;

    JS_ASSERT(uintN((cx->fp->imacpc ? cx->fp->imacpc : pc) - cx->fp->script->code)
              < cx->fp->script->length);

    JSOp op = js_GetOpcode(cx, cx->fp->script, pc);
    const JSCodeSpec &cs = js_CodeSpec[op];

    obj = *objp;
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    vcap = entry->vcap;

    if (entry->kpc != pc) {
        PCMETER(JS_PROPERTY_CACHE(cx).kpcmisses++);

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

    if (entry->kshape != OBJ_SHAPE(obj)) {
        PCMETER(JS_PROPERTY_CACHE(cx).kshapemisses++);
        return GetAtomFromBytecode(cx, pc, op, cs);
    }

    




    pobj = obj;

    if (JOF_MODE(cs.format) == JOF_NAME) {
        while (vcap & (PCVCAP_SCOPEMASK << PCVCAP_PROTOBITS)) {
            tmp = pobj->getParent();
            if (!tmp || !OBJ_IS_NATIVE(tmp))
                break;
            pobj = tmp;
            vcap -= PCVCAP_PROTOSIZE;
        }

        *objp = pobj;
    }

    while (vcap & PCVCAP_PROTOMASK) {
        tmp = pobj->getProto();
        if (!tmp || !OBJ_IS_NATIVE(tmp))
            break;
        pobj = tmp;
        --vcap;
    }

    if (JSPropCacheEntry::matchShape(cx, pobj, PCVCAP_SHAPE(vcap))) {
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

    PCMETER(JS_PROPERTY_CACHE(cx).vcapmisses++);
    return GetAtomFromBytecode(cx, pc, op, cs);
}

#ifdef DEBUG
#define ASSERT_CACHE_IS_EMPTY(cache)                                          \
    JS_BEGIN_MACRO                                                            \
        JSPropertyCache *cache_ = (cache);                                    \
        uintN i_;                                                             \
        JS_ASSERT(cache_->empty);                                             \
        for (i_ = 0; i_ < PROPERTY_CACHE_SIZE; i_++) {                        \
            JS_ASSERT(!cache_->table[i_].kpc);                                \
            JS_ASSERT(!cache_->table[i_].kshape);                             \
            JS_ASSERT(!cache_->table[i_].vcap);                               \
            JS_ASSERT(!cache_->table[i_].vword);                              \
        }                                                                     \
    JS_END_MACRO
#else
#define ASSERT_CACHE_IS_EMPTY(cache) ((void)0)
#endif

JS_STATIC_ASSERT(PCVAL_NULL == 0);

void
js_PurgePropertyCache(JSContext *cx, JSPropertyCache *cache)
{
    if (cache->empty) {
        ASSERT_CACHE_IS_EMPTY(cache);
        return;
    }

    PodArrayZero(cache->table);
    cache->empty = JS_TRUE;

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

# define P(mem) fprintf(fp, "%11s %10lu\n", #mem, (unsigned long)cache->mem)
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
                (100. * cache->pchits) / cache->tests,
                (100. * cache->protopchits) / cache->tests,
                (100. * (cache->addpchits + cache->setpchits))
                / cache->settests,
                (100. * cache->inipchits) / cache->initests,
                (100. * (cache->tests - cache->misses)) / cache->tests);
        fflush(fp);
    }
  }
#endif

    PCMETER(cache->flushes++);
}

void
js_PurgePropertyCacheForScript(JSContext *cx, JSScript *script)
{
    JSPropertyCache *cache = &JS_PROPERTY_CACHE(cx);

    for (JSPropCacheEntry *entry = cache->table;
         entry < cache->table + PROPERTY_CACHE_SIZE;
         entry++) {
        if (JS_UPTRDIFF(entry->kpc, script->code) < script->length) {
            entry->kpc = NULL;
#ifdef DEBUG
            entry->kshape = entry->vcap = entry->vword = 0;
#endif
        }
    }
}
