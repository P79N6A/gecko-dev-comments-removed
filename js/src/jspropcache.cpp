







































#include <string.h>
#include "jsapi.h"
#include "jscntxt.h"
#include "jsinterp.h"
#include "jsscope.h"
#include "jspropcacheinlines.h"

JS_REQUIRES_STACK JSPropCacheEntry *
JSPropertyCache::fill(JSContext *cx, JSObject *obj, uintN scopeIndex, uintN protoIndex,
                      JSObject *pobj, JSScopeProperty *sprop, JSBool adding)
{
    jsbytecode *pc;
    JSScope *scope;
    jsuword kshape, vshape;
    JSOp op;
    const JSCodeSpec *cs;
    jsuword vword;
    ptrdiff_t pcoff;
    JSAtom *atom;

    JS_ASSERT(!cx->runtime->gcRunning);
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));

    
    if (js_IsPropertyCacheDisabled(cx) || (cx->fp->flags & JSFRAME_EVAL)) {
        PCMETER(disfills++);
        return JS_NO_PROP_CACHE_FILL;
    }

    



    scope = OBJ_SCOPE(pobj);
    if (!scope->has(sprop)) {
        PCMETER(oddfills++);
        return JS_NO_PROP_CACHE_FILL;
    }

    












    JS_ASSERT_IF(scopeIndex == 0 && protoIndex == 0, obj == pobj);

    if (protoIndex != 0) {
        JSObject *tmp = obj;

        for (uintN i = 0; i != scopeIndex; i++)
            tmp = OBJ_GET_PARENT(cx, tmp);
        JS_ASSERT(tmp != pobj);

        protoIndex = 1;
        for (;;) {
            tmp = OBJ_GET_PROTO(cx, tmp);

            




            if (!tmp || !OBJ_IS_NATIVE(tmp)) {
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
                vword = JSVAL_OBJECT_TO_PCVAL(v);
                break;
            }

            if (SPROP_HAS_STUB_GETTER(sprop) &&
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
                        scope->brandingShapeChange(cx, sprop->slot, v);
                        if (js_IsPropertyCacheDisabled(cx))  
                            return JS_NO_PROP_CACHE_FILL;
                        scope->setBranded();
                    }
                    vword = JSVAL_OBJECT_TO_PCVAL(v);
                    break;
                }
            }
        }

        
        if (!(cs->format & (JOF_SET | JOF_INCDEC | JOF_FOR)) &&
            SPROP_HAS_STUB_GETTER(sprop) &&
            SPROP_HAS_VALID_SLOT(sprop, scope)) {
            
            vword = SLOT_TO_PCVAL(sprop->slot);
        } else {
            
            vword = SPROP_TO_PCVAL(sprop);
            if (adding &&
                sprop == scope->lastProp &&
                scope->shape == sprop->shape) {
                


























                JS_ASSERT(scope->owned());
                if (sprop->parent) {
                    kshape = sprop->parent->shape;
                } else {
                    





                    JSObject *proto = STOBJ_GET_PROTO(obj);
                    if (!proto || !OBJ_IS_NATIVE(proto))
                        return JS_NO_PROP_CACHE_FILL;
                    JSScope *protoscope = OBJ_SCOPE(proto);
                    if (!protoscope->emptyScope ||
                        !js_ObjectIsSimilarToProto(cx, obj, obj->map->ops, OBJ_GET_CLASS(cx, obj),
                                                   proto)) {
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

    if (obj == pobj) {
        JS_ASSERT(scopeIndex == 0 && protoIndex == 0);
    } else {
        if (op == JSOP_LENGTH) {
            atom = cx->runtime->atomState.lengthAtom;
        } else {
            pcoff = (JOF_TYPE(cs->format) == JOF_SLOTATOM) ? SLOTNO_LEN : 0;
            GET_ATOM_FROM_BYTECODE(cx->fp->script, pc, pcoff, atom);
        }

#ifdef DEBUG
        if (scopeIndex == 0) {
            JS_ASSERT(protoIndex != 0);
            JS_ASSERT((protoIndex == 1) == (OBJ_GET_PROTO(cx, obj) == pobj));
        }
#endif

        if (scopeIndex != 0 || protoIndex != 1) {
            









            obj->setDelegate();

            return fillByAtom(atom, obj, vshape, scopeIndex, protoIndex, vword);
        }
    }

    return fillByPC(pc, kshape, vshape, scopeIndex, protoIndex, vword);
}

JS_REQUIRES_STACK JSAtom *
JSPropertyCache::fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp, JSObject **pobjp,
                         JSPropCacheEntry **entryp)
{
    JSOp op;
    const JSCodeSpec *cs;
    ptrdiff_t pcoff;
    JSAtom *atom;
    JSObject *obj, *pobj, *tmp;
    JSPropCacheEntry *entry;
    uint32 vcap;

    JS_ASSERT(uintN((cx->fp->imacpc ? cx->fp->imacpc : pc) - cx->fp->script->code)
              < cx->fp->script->length);

    op = js_GetOpcode(cx, cx->fp->script, pc);
    cs = &js_CodeSpec[op];
    if (op == JSOP_LENGTH) {
        atom = cx->runtime->atomState.lengthAtom;
    } else {
        pcoff = (JOF_TYPE(cs->format) == JOF_SLOTATOM) ? SLOTNO_LEN : 0;
        GET_ATOM_FROM_BYTECODE(cx->fp->script, pc, pcoff, atom);
    }

    obj = *objp;
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    entry = &table[PROPERTY_CACHE_HASH_ATOM(atom, obj)];
    *entryp = entry;
    vcap = entry->vcap;

    if (entry->kpc != (jsbytecode *) atom) {
        PCMETER(idmisses++);

#ifdef DEBUG_notme
        entry = &table[PROPERTY_CACHE_HASH_PC(pc, OBJ_SHAPE(obj))];
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

    if (entry->kshape != (jsuword) obj) {
        PCMETER(komisses++);
        return atom;
    }

    pobj = obj;

    if (JOF_MODE(cs->format) == JOF_NAME) {
        while (vcap & (PCVCAP_SCOPEMASK << PCVCAP_PROTOBITS)) {
            tmp = OBJ_GET_PARENT(cx, pobj);
            if (!tmp || !OBJ_IS_NATIVE(tmp))
                break;
            pobj = tmp;
            vcap -= PCVCAP_PROTOSIZE;
        }

        *objp = pobj;
    }

    while (vcap & PCVCAP_PROTOMASK) {
        tmp = OBJ_GET_PROTO(cx, pobj);
        if (!tmp || !OBJ_IS_NATIVE(tmp))
            break;
        pobj = tmp;
        --vcap;
    }

    if (JS_LOCK_OBJ_IF_SHAPE(cx, pobj, PCVCAP_SHAPE(vcap))) {
#ifdef DEBUG
        jsid id = ATOM_TO_JSID(atom);

        id = js_CheckForStringIndex(id);
        JS_ASSERT(OBJ_SCOPE(pobj)->lookup(id));
        JS_ASSERT_IF(OBJ_SCOPE(pobj)->object, OBJ_SCOPE(pobj)->object == pobj);
#endif
        *pobjp = pobj;
        return NULL;
    }

    PCMETER(vcmisses++);
    return atom;
}

inline void
JSPropertyCache::assertEmpty()
{
#ifdef DEBUG
    JS_ASSERT(empty);
    for (uintN i = 0; i < PROPERTY_CACHE_SIZE; i++) {
        JS_ASSERT(!table[i].kpc);
        JS_ASSERT(!table[i].kshape);
        JS_ASSERT(!table[i].vcap);
        JS_ASSERT(!table[i].vword);
    }
#endif
}


JS_STATIC_ASSERT(PCVAL_NULL == 0);

void
JSPropertyCache::purge(JSContext *cx)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));

    if (empty) {
        assertEmpty();
        return;
    }

    memset(table, 0, sizeof table);
    empty = JS_TRUE;

#ifdef JS_PROPERTY_CACHE_METERING
    {
        static FILE *fp;
        if (!fp)
            fp = fopen("/tmp/propcache.stats", "w");
        if (fp) {
            fputs("Property cache stats for ", fp);
# ifdef JS_THREADSAFE
            fprintf(fp, "thread %lu, ", (unsigned long) cx->thread->id);
# endif
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
            P(pcrecycles);
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
            P(slotchanges);
            P(setmisses);
            P(idmisses);
            P(komisses);
            P(vcmisses);
            P(misses);
            P(flushes);
            P(pcpurges);
# undef P

            fprintf(fp, "hit rates: pc %g%% (proto %g%%), set %g%%, ini %g%%, full %g%%\n",
                    (100. * pchits) / tests,
                    (100. * protopchits) / tests,
                    (100. * (addpchits + setpchits)) / settests,
                    (100. * inipchits) / initests,
                    (100. * (tests - misses)) / tests);
            fflush(fp);
        }
    }
#endif

    PCMETER(flushes++);
}

void
JSPropertyCache::purgeForScript(JSContext *cx, JSScript *script)
{
    JS_ASSERT(this == &JS_PROPERTY_CACHE(cx));

    for (JSPropCacheEntry *entry = table; entry < table + PROPERTY_CACHE_SIZE; entry++) {
        if (JS_UPTRDIFF(entry->kpc, script->code) < script->length) {
            entry->kpc = NULL;
            entry->kshape = 0;
#ifdef DEBUG
            entry->vcap = entry->vword = 0;
#endif
        }
    }
}
