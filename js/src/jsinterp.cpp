










































#include "jsstddef.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "jstypes.h"
#include "jsarena.h" 
#include "jsutil.h" 
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsstaticcheck.h"
#include "jstracer.h"

#ifdef INCLUDE_MOZILLA_DTRACE
#include "jsdtracef.h"
#endif

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsautooplen.h"


#if !JS_LONE_INTERPRET ^ defined jsinvoke_cpp___

uint32
js_GenerateShape(JSContext *cx, JSBool gcLocked, JSScopeProperty *sprop)
{
    JSRuntime *rt;
    uint32 shape;
    JSTempValueRooter tvr;

    rt = cx->runtime;
    shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
    JS_ASSERT(shape != 0);
    if (shape & SHAPE_OVERFLOW_BIT) {
        rt->gcPoke = JS_TRUE;
        if (sprop)
            JS_PUSH_TEMP_ROOT_SPROP(cx, sprop, &tvr);
        js_GC(cx, gcLocked ? GC_LOCK_HELD : GC_NORMAL);
        if (sprop)
            JS_POP_TEMP_ROOT(cx, &tvr);
        shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
        JS_ASSERT(shape != 0);
        JS_ASSERT_IF(shape & SHAPE_OVERFLOW_BIT,
                     JS_PROPERTY_CACHE(cx).disabled);
    }
    return shape;
}

JS_REQUIRES_STACK void
js_FillPropertyCache(JSContext *cx, JSObject *obj, jsuword kshape,
                     uintN scopeIndex, uintN protoIndex,
                     JSObject *pobj, JSScopeProperty *sprop,
                     JSPropCacheEntry **entryp)
{
    JSPropertyCache *cache;
    jsbytecode *pc;
    JSScope *scope;
    JSOp op;
    const JSCodeSpec *cs;
    jsuword vword;
    ptrdiff_t pcoff;
    jsuword khash;
    JSAtom *atom;
    JSPropCacheEntry *entry;

    JS_ASSERT(!cx->runtime->gcRunning);
    cache = &JS_PROPERTY_CACHE(cx);
    pc = cx->fp->regs->pc;
    if (cache->disabled || (cx->fp->flags & JSFRAME_EVAL)) {
        PCMETER(cache->disfills++);
        *entryp = NULL;
        return;
    }

    



    scope = OBJ_SCOPE(pobj);
    JS_ASSERT(scope->object == pobj);
    if (!SCOPE_HAS_PROPERTY(scope, sprop)) {
        PCMETER(cache->oddfills++);
        *entryp = NULL;
        return;
    }

    












    JS_ASSERT_IF(scopeIndex == 0 && protoIndex == 0, obj == pobj);
    if (protoIndex != 0) {
        JSObject *tmp;

        JS_ASSERT(pobj != obj);
        protoIndex = 1;
        tmp = obj;
        for (;;) {
            tmp = OBJ_GET_PROTO(cx, tmp);
            if (!tmp) {
                PCMETER(cache->noprotos++);
                *entryp = NULL;
                return;
            }
            if (tmp == pobj)
                break;
            ++protoIndex;
        }
    }
    if (scopeIndex > PCVCAP_SCOPEMASK || protoIndex > PCVCAP_PROTOMASK) {
        PCMETER(cache->longchains++);
        *entryp = NULL;
        return;
    }

    



    op = (JSOp) *pc;
    cs = &js_CodeSpec[op];

    do {
        





        if (cs->format & JOF_CALLOP) {
            if (SPROP_HAS_STUB_GETTER(sprop) &&
                SPROP_HAS_VALID_SLOT(sprop, scope)) {
                jsval v;

                v = LOCKED_OBJ_GET_SLOT(pobj, sprop->slot);
                if (VALUE_IS_FUNCTION(cx, v)) {
                    











                    if (!SCOPE_IS_BRANDED(scope)) {
                        PCMETER(cache->brandfills++);
#ifdef DEBUG_notme
                        fprintf(stderr,
                            "branding %p (%s) for funobj %p (%s), kshape %lu\n",
                            pobj, LOCKED_OBJ_GET_CLASS(pobj)->name,
                            JSVAL_TO_OBJECT(v),
                            JS_GetFunctionName(GET_FUNCTION_PRIVATE(cx,
                                                 JSVAL_TO_OBJECT(v))),
                            kshape);
#endif
                        SCOPE_MAKE_UNIQUE_SHAPE(cx, scope);
                        SCOPE_SET_BRANDED(scope);
                        if (OBJ_SCOPE(obj) == scope)
                            kshape = scope->shape;
                    }
                    vword = JSVAL_OBJECT_TO_PCVAL(v);
                    break;
                }
            }
        }

        
        if (!(cs->format & JOF_SET) &&
            !((cs->format & (JOF_INCDEC | JOF_FOR)) &&
              (sprop->attrs & JSPROP_READONLY)) &&
            SPROP_HAS_STUB_GETTER(sprop) &&
            SPROP_HAS_VALID_SLOT(sprop, scope)) {
            
            vword = SLOT_TO_PCVAL(sprop->slot);
        } else {
            
            vword = SPROP_TO_PCVAL(sprop);
        }
    } while (0);

    










    if (!(cs->format & (JOF_SET | JOF_INCDEC)) && obj == pobj)
        kshape = scope->shape;

    khash = PROPERTY_CACHE_HASH_PC(pc, kshape);
    if (obj == pobj) {
        JS_ASSERT(kshape != 0 || scope->shape != 0);
        JS_ASSERT(scopeIndex == 0 && protoIndex == 0);
        JS_ASSERT(OBJ_SCOPE(obj)->object == obj);
    } else {
        if (op == JSOP_LENGTH) {
            atom = cx->runtime->atomState.lengthAtom;
        } else {
            pcoff = (JOF_TYPE(cs->format) == JOF_SLOTATOM) ? 2 : 0;
            GET_ATOM_FROM_BYTECODE(cx->fp->script, pc, pcoff, atom);
        }
        JS_ASSERT_IF(scopeIndex == 0,
                     protoIndex != 1 || OBJ_GET_PROTO(cx, obj) == pobj);
        if (scopeIndex != 0 || protoIndex != 1) {
            khash = PROPERTY_CACHE_HASH_ATOM(atom, obj, pobj);
            PCMETER(if (PCVCAP_TAG(cache->table[khash].vcap) <= 1)
                        cache->pcrecycles++);
            pc = (jsbytecode *) atom;
            kshape = (jsuword) obj;
        }
    }

    entry = &cache->table[khash];
    PCMETER(if (entry != *entryp) cache->modfills++);
    PCMETER(if (!PCVAL_IS_NULL(entry->vword)) cache->recycles++);
    entry->kpc = pc;
    entry->kshape = kshape;
    entry->vcap = PCVCAP_MAKE(scope->shape, scopeIndex, protoIndex);
    entry->vword = vword;
    *entryp = entry;

    cache->empty = JS_FALSE;
    PCMETER(cache->fills++);
}

JS_REQUIRES_STACK JSAtom *
js_FullTestPropertyCache(JSContext *cx, jsbytecode *pc,
                         JSObject **objp, JSObject **pobjp,
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

    op = (JSOp) *pc;
    cs = &js_CodeSpec[op];
    if (op == JSOP_LENGTH) {
        atom = cx->runtime->atomState.lengthAtom;
    } else {
        pcoff = (JOF_TYPE(cs->format) == JOF_SLOTATOM) ? 2 : 0;
        GET_ATOM_FROM_BYTECODE(cx->fp->script, pc, pcoff, atom);
    }

    obj = *objp;
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    entry = &JS_PROPERTY_CACHE(cx).table[PROPERTY_CACHE_HASH_ATOM(atom, obj, NULL)];
    *entryp = entry;
    vcap = entry->vcap;

    if (entry->kpc != (jsbytecode *) atom) {
        PCMETER(JS_PROPERTY_CACHE(cx).idmisses++);

#ifdef DEBUG_notme
        entry = &JS_PROPERTY_CACHE(cx).table[PROPERTY_CACHE_HASH_PC(pc, OBJ_SHAPE(obj))];
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
                                PTRDIFF(pc, cx->fp->script->code, jsbytecode),
                                JS_FALSE, stderr);
#endif

        return atom;
    }

    if (entry->kshape != (jsuword) obj) {
        PCMETER(JS_PROPERTY_CACHE(cx).komisses++);
        return atom;
    }

    pobj = obj;
    JS_LOCK_OBJ(cx, pobj);

    if (JOF_MODE(cs->format) == JOF_NAME) {
        while (vcap & (PCVCAP_SCOPEMASK << PCVCAP_PROTOBITS)) {
            tmp = LOCKED_OBJ_GET_PARENT(pobj);
            if (!tmp || !OBJ_IS_NATIVE(tmp))
                break;
            JS_UNLOCK_OBJ(cx, pobj);
            pobj = tmp;
            JS_LOCK_OBJ(cx, pobj);
            vcap -= PCVCAP_PROTOSIZE;
        }

        *objp = pobj;
    }

    while (vcap & PCVCAP_PROTOMASK) {
        tmp = LOCKED_OBJ_GET_PROTO(pobj);
        if (!tmp || !OBJ_IS_NATIVE(tmp))
            break;
        JS_UNLOCK_OBJ(cx, pobj);
        pobj = tmp;
        JS_LOCK_OBJ(cx, pobj);
        --vcap;
    }

    if (PCVCAP_SHAPE(vcap) == OBJ_SHAPE(pobj)) {
#ifdef DEBUG
        jsid id = ATOM_TO_JSID(atom);

        CHECK_FOR_STRING_INDEX(id);
        JS_ASSERT(SCOPE_GET_PROPERTY(OBJ_SCOPE(pobj), id));
        JS_ASSERT(OBJ_SCOPE(pobj)->object == pobj);
#endif
        *pobjp = pobj;
        return NULL;
    }

    PCMETER(JS_PROPERTY_CACHE(cx).vcmisses++);
    JS_UNLOCK_OBJ(cx, pobj);
    return atom;
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
js_FlushPropertyCache(JSContext *cx)
{
    JSPropertyCache *cache;

    cache = &JS_PROPERTY_CACHE(cx);
    if (cache->empty) {
        ASSERT_CACHE_IS_EMPTY(cache);
        return;
    }

    memset(cache->table, 0, sizeof cache->table);
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
js_FlushPropertyCacheForScript(JSContext *cx, JSScript *script)
{
    JSPropertyCache *cache;
    JSPropCacheEntry *entry;

    cache = &JS_PROPERTY_CACHE(cx);
    for (entry = cache->table; entry < cache->table + PROPERTY_CACHE_SIZE;
         entry++) {
        if (JS_UPTRDIFF(entry->kpc, script->code) < script->length) {
            entry->kpc = NULL;
            entry->kshape = 0;
#ifdef DEBUG
            entry->vcap = entry->vword = 0;
#endif
        }
    }
}

void
js_DisablePropertyCache(JSContext *cx)
{
    JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled >= 0);
    ++JS_PROPERTY_CACHE(cx).disabled;
}

void
js_EnablePropertyCache(JSContext *cx)
{
    --JS_PROPERTY_CACHE(cx).disabled;
    JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled >= 0);
}





static JSBool
AllocateAfterSP(JSContext *cx, jsval *sp, uintN nslots)
{
    uintN surplus;
    jsval *sp2;

    JS_ASSERT((jsval *) cx->stackPool.current->base <= sp);
    JS_ASSERT(sp <= (jsval *) cx->stackPool.current->avail);
    surplus = (jsval *) cx->stackPool.current->avail - sp;
    if (nslots <= surplus)
        return JS_TRUE;

    



    if (nslots > (size_t) ((jsval *) cx->stackPool.current->limit - sp))
        return JS_FALSE;

    JS_ARENA_ALLOCATE_CAST(sp2, jsval *, &cx->stackPool,
                           (nslots - surplus) * sizeof(jsval));
    JS_ASSERT(sp2 == sp + surplus);
    return JS_TRUE;
}

JS_STATIC_INTERPRET jsval *
js_AllocRawStack(JSContext *cx, uintN nslots, void **markp)
{
    jsval *sp;

    if (!cx->stackPool.first.next) {
        int64 *timestamp;

        JS_ARENA_ALLOCATE_CAST(timestamp, int64 *,
                               &cx->stackPool, sizeof *timestamp);
        if (!timestamp) {
            js_ReportOutOfScriptQuota(cx);
            return NULL;
        }
        *timestamp = JS_Now();
    }

    if (markp)
        *markp = JS_ARENA_MARK(&cx->stackPool);
    JS_ARENA_ALLOCATE_CAST(sp, jsval *, &cx->stackPool, nslots * sizeof(jsval));
    if (!sp)
        js_ReportOutOfScriptQuota(cx);
    return sp;
}

JS_STATIC_INTERPRET void
js_FreeRawStack(JSContext *cx, void *mark)
{
    JS_ARENA_RELEASE(&cx->stackPool, mark);
}

JS_FRIEND_API(jsval *)
js_AllocStack(JSContext *cx, uintN nslots, void **markp)
{
    jsval *sp;
    JSArena *a;
    JSStackHeader *sh;

    
    if (nslots == 0) {
        *markp = NULL;
        return (jsval *) JS_ARENA_MARK(&cx->stackPool);
    }

    
    sp = js_AllocRawStack(cx, 2 + nslots, markp);
    if (!sp)
        return NULL;

    
    a = cx->stackPool.current;
    sh = cx->stackHeaders;
    if (sh && JS_STACK_SEGMENT(sh) + sh->nslots == sp) {
        
        sh->nslots += nslots;
        a->avail -= 2 * sizeof(jsval);
    } else {
        



        sh = (JSStackHeader *)sp;
        sh->nslots = nslots;
        sh->down = cx->stackHeaders;
        cx->stackHeaders = sh;
        sp += 2;
    }

    




    memset(sp, 0, nslots * sizeof(jsval));
    return sp;
}

JS_FRIEND_API(void)
js_FreeStack(JSContext *cx, void *mark)
{
    JSStackHeader *sh;
    jsuword slotdiff;

    
    if (!mark)
        return;

    
    sh = cx->stackHeaders;
    JS_ASSERT(sh);

    
    slotdiff = JS_UPTRDIFF(mark, JS_STACK_SEGMENT(sh)) / sizeof(jsval);
    if (slotdiff < (jsuword)sh->nslots)
        sh->nslots = slotdiff;
    else
        cx->stackHeaders = sh->down;

    
    JS_ARENA_RELEASE(&cx->stackPool, mark);
}

JSObject *
js_GetScopeChain(JSContext *cx, JSStackFrame *fp)
{
    JSObject *obj, *cursor, *clonedChild, *parent;
    JSTempValueRooter tvr;

    obj = fp->blockChain;
    if (!obj) {
        



        JS_ASSERT(!fp->fun ||
                  !(fp->fun->flags & JSFUN_HEAVYWEIGHT) ||
                  fp->callobj);
        JS_ASSERT(fp->scopeChain);
        return fp->scopeChain;
    }

    




    if (fp->fun && !fp->callobj) {
        JS_ASSERT(OBJ_GET_CLASS(cx, fp->scopeChain) != &js_BlockClass ||
                  OBJ_GET_PRIVATE(cx, fp->scopeChain) != fp);
        if (!js_GetCallObject(cx, fp, fp->scopeChain))
            return NULL;
    }

    







    cursor = obj;
    clonedChild = NULL;
    for (;;) {
        parent = OBJ_GET_PARENT(cx, cursor);

        




        cursor = js_CloneBlockObject(cx, cursor, fp->scopeChain, fp);
        if (!cursor) {
            if (clonedChild)
                JS_POP_TEMP_ROOT(cx, &tvr);
            return NULL;
        }
        if (!clonedChild) {
            



            obj = cursor;
            if (!parent)
                break;
            JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &tvr);
        } else {
            



            STOBJ_SET_PARENT(clonedChild, cursor);
            if (!parent) {
                JS_ASSERT(tvr.u.value == OBJECT_TO_JSVAL(obj));
                JS_POP_TEMP_ROOT(cx, &tvr);
                break;
            }
        }
        clonedChild = cursor;
        cursor = parent;
    }
    fp->flags |= JSFRAME_POP_BLOCKS;
    fp->scopeChain = obj;
    fp->blockChain = NULL;
    return obj;
}

JSBool
js_GetPrimitiveThis(JSContext *cx, jsval *vp, JSClass *clasp, jsval *thisvp)
{
    jsval v;
    JSObject *obj;

    v = vp[1];
    if (JSVAL_IS_OBJECT(v)) {
        obj = JS_THIS_OBJECT(cx, vp);
        if (!JS_InstanceOf(cx, obj, clasp, vp + 2))
            return JS_FALSE;
        v = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    }
    *thisvp = v;
    return JS_TRUE;
}
















JS_STATIC_INTERPRET JSObject *
js_ComputeGlobalThis(JSContext *cx, JSBool lazy, jsval *argv)
{
    JSObject *thisp;

    if (JSVAL_IS_PRIMITIVE(argv[-2]) ||
        !OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(argv[-2]))) {
        thisp = cx->globalObject;
    } else {
        JSStackFrame *fp;
        jsid id;
        jsval v;
        uintN attrs;
        JSBool ok;
        JSObject *parent;

        









        fp = js_GetTopStackFrame(cx);    
        if (lazy) {
            JS_ASSERT(fp->argv == argv);
            fp->dormantNext = cx->dormantFrameChain;
            cx->dormantFrameChain = fp;
            cx->fp = fp->down;
            fp->down = NULL;
        }
        thisp = JSVAL_TO_OBJECT(argv[-2]);
        id = ATOM_TO_JSID(cx->runtime->atomState.parentAtom);

        ok = OBJ_CHECK_ACCESS(cx, thisp, id, JSACC_PARENT, &v, &attrs);
        if (lazy) {
            cx->dormantFrameChain = fp->dormantNext;
            fp->dormantNext = NULL;
            fp->down = cx->fp;
            cx->fp = fp;
        }
        if (!ok)
            return NULL;

        thisp = JSVAL_IS_VOID(v)
                ? OBJ_GET_PARENT(cx, thisp)
                : JSVAL_TO_OBJECT(v);
        while ((parent = OBJ_GET_PARENT(cx, thisp)) != NULL)
            thisp = parent;
    }

    OBJ_TO_OUTER_OBJECT(cx, thisp);
    if (!thisp)
        return NULL;
    argv[-1] = OBJECT_TO_JSVAL(thisp);
    return thisp;
}

static JSObject *
ComputeThis(JSContext *cx, JSBool lazy, jsval *argv)
{
    JSObject *thisp;

    JS_ASSERT(!JSVAL_IS_NULL(argv[-1]));
    if (!JSVAL_IS_OBJECT(argv[-1])) {
        if (!js_PrimitiveToObject(cx, &argv[-1]))
            return NULL;
        thisp = JSVAL_TO_OBJECT(argv[-1]);
    } else {
        thisp = JSVAL_TO_OBJECT(argv[-1]);
        if (OBJ_GET_CLASS(cx, thisp) == &js_CallClass ||
            OBJ_GET_CLASS(cx, thisp) == &js_BlockClass) {
            return js_ComputeGlobalThis(cx, lazy, argv);
        }

        if (thisp->map->ops->thisObject) {
            
            thisp = thisp->map->ops->thisObject(cx, thisp);
            if (!thisp)
                return NULL;
        }
        OBJ_TO_OUTER_OBJECT(cx, thisp);
        if (!thisp)
            return NULL;
        argv[-1] = OBJECT_TO_JSVAL(thisp);
    }
    return thisp;
}

JSObject *
js_ComputeThis(JSContext *cx, JSBool lazy, jsval *argv)
{
    if (JSVAL_IS_NULL(argv[-1]))
        return js_ComputeGlobalThis(cx, lazy, argv);
    return ComputeThis(cx, lazy, argv);
}

#if JS_HAS_NO_SUCH_METHOD

#define JSSLOT_FOUND_FUNCTION   JSSLOT_PRIVATE
#define JSSLOT_SAVED_ID         (JSSLOT_PRIVATE + 1)

JSClass js_NoSuchMethodClass = {
    "NoSuchMethod",
    JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_IS_ANONYMOUS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_NoSuchMethod),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,   JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,    JS_FinalizeStub,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

JS_BEGIN_EXTERN_C

JSObject*
js_InitNoSuchMethodClass(JSContext *cx, JSObject* obj);

JS_END_EXTERN_C

JSObject*
js_InitNoSuchMethodClass(JSContext *cx, JSObject* obj)
{
    JSObject *proto;

    proto = JS_InitClass(cx, obj, NULL, &js_NoSuchMethodClass, NULL, 0, NULL,
                         NULL, NULL, NULL);
    if (!proto)
        return NULL;

    OBJ_CLEAR_PROTO(cx, proto);
    return proto;
}















JS_STATIC_INTERPRET JSBool
js_OnUnknownMethod(JSContext *cx, jsval *vp)
{
    JSObject *obj;
    jsid id;
    JSTempValueRooter tvr;
    JSBool ok;

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
    obj = JSVAL_TO_OBJECT(vp[1]);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_NULL, &tvr);

    MUST_FLOW_THROUGH("out");
    id = ATOM_TO_JSID(cx->runtime->atomState.noSuchMethodAtom);
#if JS_HAS_XML_SUPPORT
    if (OBJECT_IS_XML(cx, obj)) {
        JSXMLObjectOps *ops;

        ops = (JSXMLObjectOps *) obj->map->ops;
        obj = ops->getMethod(cx, obj, id, &tvr.u.value);
        if (!obj) {
            ok = JS_FALSE;
            goto out;
        }
        vp[1] = OBJECT_TO_JSVAL(obj);
    } else
#endif
    {
        ok = OBJ_GET_PROPERTY(cx, obj, id, &tvr.u.value);
        if (!ok)
            goto out;
    }
    if (JSVAL_IS_PRIMITIVE(tvr.u.value)) {
        vp[0] = tvr.u.value;
    } else {
#if JS_HAS_XML_SUPPORT
        
        if (!JSVAL_IS_PRIMITIVE(vp[0])) {
            obj = JSVAL_TO_OBJECT(vp[0]);
            ok = js_IsFunctionQName(cx, obj, &id);
            if (!ok)
                goto out;
            if (id != 0)
                vp[0] = ID_TO_VALUE(id);
        }
#endif
        obj = js_NewObject(cx, &js_NoSuchMethodClass, NULL, NULL, 0);
        if (!obj) {
            ok = JS_FALSE;
            goto out;
        }
        obj->fslots[JSSLOT_FOUND_FUNCTION] = tvr.u.value;
        obj->fslots[JSSLOT_SAVED_ID] = vp[0];
        vp[0] = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

  out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

static JSBool
NoSuchMethod(JSContext *cx, uintN argc, jsval *vp, uint32 flags)
{
    jsval *invokevp;
    void *mark;
    JSBool ok;
    JSObject *obj, *argsobj;

    invokevp = js_AllocStack(cx, 2 + 2, &mark);
    if (!invokevp)
        return JS_FALSE;

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[0]));
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
    obj = JSVAL_TO_OBJECT(vp[0]);
    JS_ASSERT(STOBJ_GET_CLASS(obj) == &js_NoSuchMethodClass);

    invokevp[0] = obj->fslots[JSSLOT_FOUND_FUNCTION];
    invokevp[1] = vp[1];
    invokevp[2] = obj->fslots[JSSLOT_SAVED_ID];
    argsobj = js_NewArrayObject(cx, argc, vp + 2);
    if (!argsobj) {
        ok = JS_FALSE;
    } else {
        invokevp[3] = OBJECT_TO_JSVAL(argsobj);
        ok = (flags & JSINVOKE_CONSTRUCT)
             ? js_InvokeConstructor(cx, 2, JS_TRUE, invokevp)
             : js_Invoke(cx, 2, invokevp, flags);
        vp[0] = invokevp[0];
    }
    js_FreeStack(cx, mark);
    return ok;
}

#endif 





JS_STATIC_ASSERT(JSVAL_INT == 1);
JS_STATIC_ASSERT(JSVAL_DOUBLE == 2);
JS_STATIC_ASSERT(JSVAL_STRING == 4);
JS_STATIC_ASSERT(JSVAL_BOOLEAN == 6);

const uint16 js_PrimitiveTestFlags[] = {
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_STRING,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_BOOLEAN,    
    JSFUN_THISP_NUMBER      
};







JS_FRIEND_API(JSBool)
js_Invoke(JSContext *cx, uintN argc, jsval *vp, uintN flags)
{
    void *mark;
    JSStackFrame frame;
    jsval *sp, *argv, *newvp;
    jsval v;
    JSObject *funobj, *parent;
    JSBool ok;
    JSClass *clasp;
    JSObjectOps *ops;
    JSNative native;
    JSFunction *fun;
    JSScript *script;
    uintN nslots, i;
    uint32 rootedArgsFlag;
    JSInterpreterHook hook;
    void *hookData;

    
    JS_ASSERT((jsval *) cx->stackPool.current->base <= vp);
    JS_ASSERT(vp + 2 + argc <= (jsval *) cx->stackPool.current->avail);

    



    mark = JS_ARENA_MARK(&cx->stackPool);
    v = *vp;

    if (JSVAL_IS_PRIMITIVE(v))
        goto bad;

    funobj = JSVAL_TO_OBJECT(v);
    parent = OBJ_GET_PARENT(cx, funobj);
    clasp = OBJ_GET_CLASS(cx, funobj);
    if (clasp != &js_FunctionClass) {
#if JS_HAS_NO_SUCH_METHOD
        if (clasp == &js_NoSuchMethodClass) {
            ok = NoSuchMethod(cx, argc, vp, flags);
            goto out2;
        }
#endif

        
        ops = funobj->map->ops;

        








        if ((ops == &js_ObjectOps) ? clasp->call : ops->call) {
            ok = clasp->convert(cx, funobj, JSTYPE_FUNCTION, &v);
            if (!ok)
                goto out2;

            if (VALUE_IS_FUNCTION(cx, v)) {
                
                *vp = v;
                funobj = JSVAL_TO_OBJECT(v);
                parent = OBJ_GET_PARENT(cx, funobj);
                goto have_fun;
            }
        }
        fun = NULL;
        script = NULL;
        nslots = 0;

        
        if (flags & JSINVOKE_CONSTRUCT) {
            if (!JSVAL_IS_OBJECT(vp[1])) {
                ok = js_PrimitiveToObject(cx, &vp[1]);
                if (!ok)
                    goto out2;
            }
            native = ops->construct;
        } else {
            native = ops->call;
        }
        if (!native)
            goto bad;
    } else {
have_fun:
        
        fun = GET_FUNCTION_PRIVATE(cx, funobj);
        nslots = FUN_MINARGS(fun);
        nslots = (nslots > argc) ? nslots - argc : 0;
        if (FUN_INTERPRETED(fun)) {
            native = NULL;
            script = fun->u.i.script;
        } else {
            native = fun->u.n.native;
            script = NULL;
            nslots += fun->u.n.extra;
        }

        if (JSFUN_BOUND_METHOD_TEST(fun->flags)) {
            
            vp[1] = OBJECT_TO_JSVAL(parent);
        } else if (!JSVAL_IS_OBJECT(vp[1])) {
            JS_ASSERT(!(flags & JSINVOKE_CONSTRUCT));
            if (PRIMITIVE_THIS_TEST(fun, vp[1]))
                goto start_call;
        }
    }

    if (flags & JSINVOKE_CONSTRUCT) {
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
    } else {
        









        if (native && (!fun || !(fun->flags & JSFUN_FAST_NATIVE))) {
            if (!js_ComputeThis(cx, JS_FALSE, vp + 2)) {
                ok = JS_FALSE;
                goto out2;
            }
            flags |= JSFRAME_COMPUTED_THIS;
        }
    }

  start_call:
    if (native && fun && (fun->flags & JSFUN_FAST_NATIVE)) {
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif
        JS_ASSERT(nslots == 0);
#if JS_HAS_LVALUE_RETURN
        
        cx->rval2set = JS_FALSE;
#endif
        ok = ((JSFastNative) native)(cx, argc, vp);
        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
        goto out2;
    }

    argv = vp + 2;
    sp = argv + argc;

    rootedArgsFlag = JSFRAME_ROOTED_ARGV;
    if (nslots != 0) {
        





        if (!AllocateAfterSP(cx, sp, nslots)) {
            rootedArgsFlag = 0;
            newvp = js_AllocRawStack(cx, 2 + argc + nslots, NULL);
            if (!newvp) {
                ok = JS_FALSE;
                goto out2;
            }
            memcpy(newvp, vp, (2 + argc) * sizeof(jsval));
            argv = newvp + 2;
            sp = argv + argc;
        }

        
        i = nslots;
        do {
            *sp++ = JSVAL_VOID;
        } while (--i != 0);
    }

    
    if (script && script->nslots != 0) {
        if (!AllocateAfterSP(cx, sp, script->nslots)) {
            
            sp = js_AllocRawStack(cx, script->nslots, NULL);
            if (!sp) {
                ok = JS_FALSE;
                goto out2;
            }
        }

        
        for (jsval *end = sp + fun->u.i.nvars; sp != end; ++sp)
            *sp = JSVAL_VOID;
    }

    






    frame.thisp = (JSObject *)vp[1];
    frame.varobj = NULL;
    frame.callobj = frame.argsobj = NULL;
    frame.script = script;
    frame.callee = funobj;
    frame.fun = fun;
    frame.argc = argc;
    frame.argv = argv;

    
    frame.rval = (flags & JSINVOKE_CONSTRUCT) ? vp[1] : JSVAL_VOID;
    frame.down = js_GetTopStackFrame(cx);
    frame.annotation = NULL;
    frame.scopeChain = NULL;    
    frame.regs = NULL;
    frame.imacpc = NULL;
    frame.slots = NULL;
    frame.sharpDepth = 0;
    frame.sharpArray = NULL;
    frame.flags = flags | rootedArgsFlag;
    frame.dormantNext = NULL;
    frame.xmlNamespace = NULL;
    frame.blockChain = NULL;

    MUST_FLOW_THROUGH("out");
    cx->fp = &frame;

    
    hook = cx->debugHooks->callHook;
    hookData = NULL;

    
    if (hook && (native || script))
        hookData = hook(cx, &frame, JS_TRUE, 0, cx->debugHooks->callHookData);

    
    if (native) {
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif

#if JS_HAS_LVALUE_RETURN
        
        cx->rval2set = JS_FALSE;
#endif

        
        JS_ASSERT(!frame.varobj);
        JS_ASSERT(!frame.scopeChain);
        if (frame.down) {
            frame.varobj = frame.down->varobj;
            frame.scopeChain = frame.down->scopeChain;
        }

        
        if (!frame.scopeChain)
            frame.scopeChain = parent;

        frame.displaySave = NULL;
        ok = native(cx, frame.thisp, argc, frame.argv, &frame.rval);
        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
    } else if (script) {
        
        frame.scopeChain = parent;
        if (JSFUN_HEAVYWEIGHT_TEST(fun->flags)) {
            
            if (!js_GetCallObject(cx, &frame, parent)) {
                ok = JS_FALSE;
                goto out;
            }
        }
        frame.slots = sp - fun->u.i.nvars;

        ok = js_Interpret(cx);
    } else {
        
        frame.scopeChain = NULL;
        frame.displaySave = NULL;
        ok = JS_TRUE;
    }

out:
    if (hookData) {
        hook = cx->debugHooks->callHook;
        if (hook)
            hook(cx, &frame, JS_FALSE, &ok, hookData);
    }

    
    if (frame.callobj)
        ok &= js_PutCallObject(cx, &frame);

    
    if (frame.argsobj)
        ok &= js_PutArgsObject(cx, &frame);

    *vp = frame.rval;

    
    cx->fp = frame.down;

out2:
    
    JS_ARENA_RELEASE(&cx->stackPool, mark);
    if (!ok)
        *vp = JSVAL_NULL;
    return ok;

bad:
    js_ReportIsNotFunction(cx, vp, flags & JSINVOKE_FUNFLAGS);
    ok = JS_FALSE;
    goto out2;
}

JSBool
js_InternalInvoke(JSContext *cx, JSObject *obj, jsval fval, uintN flags,
                  uintN argc, jsval *argv, jsval *rval)
{
    jsval *invokevp;
    void *mark;
    JSBool ok;

    invokevp = js_AllocStack(cx, 2 + argc, &mark);
    if (!invokevp)
        return JS_FALSE;

    invokevp[0] = fval;
    invokevp[1] = OBJECT_TO_JSVAL(obj);
    memcpy(invokevp + 2, argv, argc * sizeof *argv);

    ok = js_Invoke(cx, argc, invokevp, flags);
    if (ok) {
        






        *rval = *invokevp;
        if (JSVAL_IS_GCTHING(*rval) && *rval != JSVAL_NULL) {
            if (cx->localRootStack) {
                if (js_PushLocalRoot(cx, cx->localRootStack, *rval) < 0)
                    ok = JS_FALSE;
            } else {
                cx->weakRoots.lastInternalResult = *rval;
            }
        }
    }

    js_FreeStack(cx, mark);
    return ok;
}

JSBool
js_InternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, jsval fval,
                    JSAccessMode mode, uintN argc, jsval *argv, jsval *rval)
{
    JSSecurityCallbacks *callbacks;

    



    JS_CHECK_RECURSION(cx, return JS_FALSE);

    














    JS_ASSERT(mode == JSACC_READ || mode == JSACC_WRITE);
    callbacks = JS_GetSecurityCallbacks(cx);
    if (callbacks &&
        callbacks->checkObjectAccess &&
        VALUE_IS_FUNCTION(cx, fval) &&
        FUN_INTERPRETED(GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(fval))) &&
        !callbacks->checkObjectAccess(cx, obj, ID_TO_VALUE(id), mode, &fval)) {
        return JS_FALSE;
    }

    return js_InternalCall(cx, obj, fval, argc, argv, rval);
}

JSBool
js_Execute(JSContext *cx, JSObject *chain, JSScript *script,
           JSStackFrame *down, uintN flags, jsval *result)
{
    JSInterpreterHook hook;
    void *hookData, *mark;
    JSStackFrame *oldfp, frame;
    JSObject *obj, *tmp;
    JSBool ok;

#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_START_ENABLED())
        jsdtrace_execute_start(script);
#endif

    hook = cx->debugHooks->executeHook;
    hookData = mark = NULL;
    oldfp = js_GetTopStackFrame(cx);
    frame.script = script;
    if (down) {
        
        frame.callobj = down->callobj;
        frame.argsobj = down->argsobj;
        frame.varobj = down->varobj;
        frame.callee = down->callee;
        frame.fun = down->fun;
        frame.thisp = down->thisp;
        if (down->flags & JSFRAME_COMPUTED_THIS)
            flags |= JSFRAME_COMPUTED_THIS;
        frame.argc = down->argc;
        frame.argv = down->argv;
        frame.annotation = down->annotation;
        frame.sharpArray = down->sharpArray;
        JS_ASSERT(script->nfixed == 0);
    } else {
        frame.callobj = frame.argsobj = NULL;
        obj = chain;
        if (cx->options & JSOPTION_VAROBJFIX) {
            while ((tmp = OBJ_GET_PARENT(cx, obj)) != NULL)
                obj = tmp;
        }
        frame.varobj = obj;
        frame.callee = NULL;
        frame.fun = NULL;
        frame.thisp = chain;
        frame.argc = 0;
        frame.argv = NULL;
        frame.annotation = NULL;
        frame.sharpArray = NULL;
    }

    frame.imacpc = NULL;
    if (script->nslots != 0) {
        frame.slots = js_AllocRawStack(cx, script->nslots, &mark);
        if (!frame.slots) {
            ok = JS_FALSE;
            goto out;
        }
        memset(frame.slots, 0, script->nfixed * sizeof(jsval));
    } else {
        frame.slots = NULL;
    }

    frame.rval = JSVAL_VOID;
    frame.down = down;
    frame.scopeChain = chain;
    frame.regs = NULL;
    frame.sharpDepth = 0;
    frame.flags = flags;
    frame.dormantNext = NULL;
    frame.xmlNamespace = NULL;
    frame.blockChain = NULL;

    












    if (oldfp && oldfp != down) {
        JS_ASSERT(!oldfp->dormantNext);
        oldfp->dormantNext = cx->dormantFrameChain;
        cx->dormantFrameChain = oldfp;
    }

    cx->fp = &frame;
    if (!down) {
        OBJ_TO_OUTER_OBJECT(cx, frame.thisp);
        if (!frame.thisp) {
            ok = JS_FALSE;
            goto out2;
        }
        frame.flags |= JSFRAME_COMPUTED_THIS;
    }

    if (hook) {
        hookData = hook(cx, &frame, JS_TRUE, 0,
                        cx->debugHooks->executeHookData);
    }

    ok = js_Interpret(cx);
    if (result)
        *result = frame.rval;

    if (hookData) {
        hook = cx->debugHooks->executeHook;
        if (hook)
            hook(cx, &frame, JS_FALSE, &ok, hookData);
    }

out2:
    if (mark)
        js_FreeRawStack(cx, mark);
    cx->fp = oldfp;

    if (oldfp && oldfp != down) {
        JS_ASSERT(cx->dormantFrameChain == oldfp);
        cx->dormantFrameChain = oldfp->dormantNext;
        oldfp->dormantNext = NULL;
    }

out:
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_EXECUTE_DONE_ENABLED())
        jsdtrace_execute_done(script);
#endif
    return ok;
}

JSBool
js_CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                      JSObject **objp, JSProperty **propp)
{
    JSObject *obj2;
    JSProperty *prop;
    uintN oldAttrs, report;
    JSBool isFunction;
    jsval value;
    const char *type, *name;

    if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
        return JS_FALSE;
    if (propp) {
        *objp = obj2;
        *propp = prop;
    }
    if (!prop)
        return JS_TRUE;

    



    if (!OBJ_GET_ATTRIBUTES(cx, obj2, id, prop, &oldAttrs)) {
        OBJ_DROP_PROPERTY(cx, obj2, prop);
#ifdef DEBUG
        prop = NULL;
#endif
        goto bad;
    }

    



    if (!propp) {
        OBJ_DROP_PROPERTY(cx, obj2, prop);
        prop = NULL;
    }

    if (attrs == JSPROP_INITIALIZER) {
        
        if (obj2 != obj)
            return JS_TRUE;
        report = JSREPORT_WARNING | JSREPORT_STRICT;
    } else {
        
        if (((oldAttrs | attrs) & JSPROP_READONLY) == 0) {
            





            if (!(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
                return JS_TRUE;
            if ((~(oldAttrs ^ attrs) & (JSPROP_GETTER | JSPROP_SETTER)) == 0)
                return JS_TRUE;
            if (!(oldAttrs & JSPROP_PERMANENT))
                return JS_TRUE;
        }

        report = JSREPORT_ERROR;
        isFunction = (oldAttrs & (JSPROP_GETTER | JSPROP_SETTER)) != 0;
        if (!isFunction) {
            if (!OBJ_GET_PROPERTY(cx, obj, id, &value))
                goto bad;
            isFunction = VALUE_IS_FUNCTION(cx, value);
        }
    }

    type = (attrs == JSPROP_INITIALIZER)
           ? "property"
           : (oldAttrs & attrs & JSPROP_GETTER)
           ? js_getter_str
           : (oldAttrs & attrs & JSPROP_SETTER)
           ? js_setter_str
           : (oldAttrs & JSPROP_READONLY)
           ? js_const_str
           : isFunction
           ? js_function_str
           : js_var_str;
    name = js_ValueToPrintableString(cx, ID_TO_VALUE(id));
    if (!name)
        goto bad;
    return JS_ReportErrorFlagsAndNumber(cx, report,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_REDECLARED_VAR,
                                        type, name);

bad:
    if (propp) {
        *objp = NULL;
        *propp = NULL;
    }
    JS_ASSERT(!prop);
    return JS_FALSE;
}

JSBool
js_StrictlyEqual(JSContext *cx, jsval lval, jsval rval)
{
    jsval ltag = JSVAL_TAG(lval), rtag = JSVAL_TAG(rval);
    jsdouble ld, rd;

    if (ltag == rtag) {
        if (ltag == JSVAL_STRING) {
            JSString *lstr = JSVAL_TO_STRING(lval),
                     *rstr = JSVAL_TO_STRING(rval);
            return js_EqualStrings(lstr, rstr);
        }
        if (ltag == JSVAL_DOUBLE) {
            ld = *JSVAL_TO_DOUBLE(lval);
            rd = *JSVAL_TO_DOUBLE(rval);
            return JSDOUBLE_COMPARE(ld, ==, rd, JS_FALSE);
        }
        if (ltag == JSVAL_OBJECT &&
            lval != rval &&
            !JSVAL_IS_NULL(lval) &&
            !JSVAL_IS_NULL(rval)) {
            JSObject *lobj, *robj;

            lobj = js_GetWrappedObject(cx, JSVAL_TO_OBJECT(lval));
            robj = js_GetWrappedObject(cx, JSVAL_TO_OBJECT(rval));
            lval = OBJECT_TO_JSVAL(lobj);
            rval = OBJECT_TO_JSVAL(robj);
        }
        return lval == rval;
    }
    if (ltag == JSVAL_DOUBLE && JSVAL_IS_INT(rval)) {
        ld = *JSVAL_TO_DOUBLE(lval);
        rd = JSVAL_TO_INT(rval);
        return JSDOUBLE_COMPARE(ld, ==, rd, JS_FALSE);
    }
    if (JSVAL_IS_INT(lval) && rtag == JSVAL_DOUBLE) {
        ld = JSVAL_TO_INT(lval);
        rd = *JSVAL_TO_DOUBLE(rval);
        return JSDOUBLE_COMPARE(ld, ==, rd, JS_FALSE);
    }
    return lval == rval;
}

JSBool
js_InvokeConstructor(JSContext *cx, uintN argc, JSBool clampReturn, jsval *vp)
{
    JSFunction *fun, *fun2;
    JSObject *obj, *obj2, *proto, *parent;
    jsval lval, rval;
    JSClass *clasp;

    fun = NULL;
    obj2 = NULL;
    lval = *vp;
    if (!JSVAL_IS_OBJECT(lval) ||
        (obj2 = JSVAL_TO_OBJECT(lval)) == NULL ||
        
        OBJ_GET_CLASS(cx, obj2) == &js_FunctionClass ||
        !obj2->map->ops->construct)
    {
        fun = js_ValueToFunction(cx, vp, JSV2F_CONSTRUCT);
        if (!fun)
            return JS_FALSE;
    }

    clasp = &js_ObjectClass;
    if (!obj2) {
        proto = parent = NULL;
        fun = NULL;
    } else {
        





        if (!OBJ_GET_PROPERTY(cx, obj2,
                              ATOM_TO_JSID(cx->runtime->atomState
                                           .classPrototypeAtom),
                              &vp[1])) {
            return JS_FALSE;
        }
        rval = vp[1];
        proto = JSVAL_IS_OBJECT(rval) ? JSVAL_TO_OBJECT(rval) : NULL;
        parent = OBJ_GET_PARENT(cx, obj2);

        if (OBJ_GET_CLASS(cx, obj2) == &js_FunctionClass) {
            fun2 = GET_FUNCTION_PRIVATE(cx, obj2);
            if (!FUN_INTERPRETED(fun2) &&
                !(fun2->flags & JSFUN_TRACEABLE) &&
                fun2->u.n.u.clasp) {
                clasp = fun2->u.n.u.clasp;
            }
        }
    }
    obj = js_NewObject(cx, clasp, proto, parent, 0);
    if (!obj)
        return JS_FALSE;

    
    vp[1] = OBJECT_TO_JSVAL(obj);
    if (!js_Invoke(cx, argc, vp, JSINVOKE_CONSTRUCT)) {
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;
        return JS_FALSE;
    }

    
    rval = *vp;
    if (clampReturn && JSVAL_IS_PRIMITIVE(rval)) {
        if (!fun) {
            
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_NEW_RESULT,
                                 js_ValueToPrintableString(cx, rval));
            return JS_FALSE;
        }
        *vp = OBJECT_TO_JSVAL(obj);
    }

    JS_RUNTIME_METER(cx->runtime, constructs);
    return JS_TRUE;
}

JSBool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, jsval idval, jsid *idp)
{
    JS_ASSERT(!JSVAL_IS_INT(idval));

#if JS_HAS_XML_SUPPORT
    if (!JSVAL_IS_PRIMITIVE(idval)) {
        if (OBJECT_IS_XML(cx, obj)) {
            *idp = OBJECT_JSVAL_TO_JSID(idval);
            return JS_TRUE;
        }
        if (!js_IsFunctionQName(cx, JSVAL_TO_OBJECT(idval), idp))
            return JS_FALSE;
        if (*idp != 0)
            return JS_TRUE;
    }
#endif

    return js_ValueToStringId(cx, idval, idp);
}





JS_STATIC_INTERPRET JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex)
{
    JSStackFrame *fp;
    jsval *sp;
    JSObject *obj, *parent, *withobj;

    fp = cx->fp;
    sp = fp->regs->sp;
    JS_ASSERT(stackIndex < 0);
    JS_ASSERT(StackBase(fp) <= sp + stackIndex);

    if (!JSVAL_IS_PRIMITIVE(sp[-1])) {
        obj = JSVAL_TO_OBJECT(sp[-1]);
    } else {
        obj = js_ValueToNonNullObject(cx, sp[-1]);
        if (!obj)
            return JS_FALSE;
        sp[-1] = OBJECT_TO_JSVAL(obj);
    }

    parent = js_GetScopeChain(cx, fp);
    if (!parent)
        return JS_FALSE;

    OBJ_TO_INNER_OBJECT(cx, obj);
    if (!obj)
        return JS_FALSE;

    withobj = js_NewWithObject(cx, obj, parent,
                               sp + stackIndex - StackBase(fp));
    if (!withobj)
        return JS_FALSE;

    fp->scopeChain = withobj;
    js_DisablePropertyCache(cx);
    return JS_TRUE;
}

JS_STATIC_INTERPRET JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx)
{
    JSObject *withobj;

    withobj = cx->fp->scopeChain;
    JS_ASSERT(OBJ_GET_CLASS(cx, withobj) == &js_WithClass);
    JS_ASSERT(OBJ_GET_PRIVATE(cx, withobj) == cx->fp);
    JS_ASSERT(OBJ_BLOCK_DEPTH(cx, withobj) >= 0);
    cx->fp->scopeChain = OBJ_GET_PARENT(cx, withobj);
    JS_SetPrivate(cx, withobj, NULL);
    js_EnablePropertyCache(cx);
}

JS_REQUIRES_STACK JSClass *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, obj);
    if ((clasp == &js_WithClass || clasp == &js_BlockClass) &&
        OBJ_GET_PRIVATE(cx, obj) == cx->fp &&
        OBJ_BLOCK_DEPTH(cx, obj) >= stackDepth) {
        return clasp;
    }
    return NULL;
}

JS_STATIC_INTERPRET JS_REQUIRES_STACK jsint
js_CountWithBlocks(JSContext *cx, JSStackFrame *fp)
{
    jsint n;
    JSObject *obj;
    JSClass *clasp;

    n = 0;
    for (obj = fp->scopeChain;
         (clasp = js_IsActiveWithOrBlock(cx, obj, 0)) != NULL;
         obj = OBJ_GET_PARENT(cx, obj)) {
        if (clasp == &js_WithClass)
            ++n;
    }
    return n;
}





JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, JSStackFrame *fp, jsint stackDepth,
               JSBool normalUnwind)
{
    JSObject *obj;
    JSClass *clasp;

    JS_ASSERT(stackDepth >= 0);
    JS_ASSERT(StackBase(fp) + stackDepth <= fp->regs->sp);

    for (obj = fp->blockChain; obj; obj = OBJ_GET_PARENT(cx, obj)) {
        JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_BlockClass);
        if (OBJ_BLOCK_DEPTH(cx, obj) < stackDepth)
            break;
    }
    fp->blockChain = obj;

    for (;;) {
        obj = fp->scopeChain;
        clasp = js_IsActiveWithOrBlock(cx, obj, stackDepth);
        if (!clasp)
            break;
        if (clasp == &js_BlockClass) {
            
            normalUnwind &= js_PutBlockObject(cx, normalUnwind);
        } else {
            js_LeaveWith(cx);
        }
    }

    fp->regs->sp = StackBase(fp) + stackDepth;
    return normalUnwind;
}

JS_STATIC_INTERPRET JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, jsval *vp, jsval *vp2)
{
    jsval v;
    jsdouble d;

    v = *vp;
    if (JSVAL_IS_DOUBLE(v)) {
        d = *JSVAL_TO_DOUBLE(v);
    } else if (JSVAL_IS_INT(v)) {
        d = JSVAL_TO_INT(v);
    } else {
        d = js_ValueToNumber(cx, vp);
        if (JSVAL_IS_NULL(*vp))
            return JS_FALSE;
        JS_ASSERT(JSVAL_IS_NUMBER(*vp) || *vp == JSVAL_TRUE);

        
        if ((cs->format & JOF_POST) &&
            *vp == JSVAL_TRUE
            && !js_NewNumberInRootedValue(cx, d, vp)) {
            return JS_FALSE;
        }
    }

    (cs->format & JOF_INC) ? d++ : d--;
    if (!js_NewNumberInRootedValue(cx, d, vp2))
        return JS_FALSE;

    if (!(cs->format & JOF_POST))
        *vp = *vp2;
    return JS_TRUE;
}

#ifdef DEBUG

JS_STATIC_INTERPRET JS_REQUIRES_STACK void
js_TraceOpcode(JSContext *cx, jsint len)
{
    FILE *tracefp;
    JSStackFrame *fp;
    JSFrameRegs *regs;
    JSOp prevop;
    intN ndefs, n, nuses;
    jsval *siter;
    JSString *str;
    JSOp op;

    tracefp = (FILE *) cx->tracefp;
    JS_ASSERT(tracefp);
    fp = cx->fp;
    regs = fp->regs;
    if (len != 0) {
        prevop = (JSOp) regs->pc[-len];
        ndefs = js_CodeSpec[prevop].ndefs;
        if (ndefs != 0) {
            for (n = -ndefs; n < 0; n++) {
                char *bytes = js_DecompileValueGenerator(cx, n, regs->sp[n],
                                                         NULL);
                if (bytes) {
                    fprintf(tracefp, "%s %s",
                            (n == -ndefs) ? "  output:" : ",",
                            bytes);
                    JS_free(cx, bytes);
                }
            }
            fprintf(tracefp, " @ %u\n", (uintN) (regs->sp - StackBase(fp)));
        }
        fprintf(tracefp, "  stack: ");
        for (siter = StackBase(fp); siter < regs->sp; siter++) {
            str = js_ValueToString(cx, *siter);
            if (!str)
                fputs("<null>", tracefp);
            else
                js_FileEscapedString(tracefp, str, 0);
            fputc(' ', tracefp);
        }
        fputc('\n', tracefp);
    }

    fprintf(tracefp, "%4u: ",
            js_PCToLineNumber(cx, fp->script, fp->imacpc ? fp->imacpc : regs->pc));
    js_Disassemble1(cx, fp->script, regs->pc,
                    PTRDIFF(regs->pc, fp->script->code, jsbytecode),
                    JS_FALSE, tracefp);
    op = (JSOp) *regs->pc;
    nuses = js_CodeSpec[op].nuses;
    if (nuses != 0) {
        for (n = -nuses; n < 0; n++) {
            char *bytes = js_DecompileValueGenerator(cx, n, regs->sp[n],
                                                     NULL);
            if (bytes) {
                fprintf(tracefp, "%s %s",
                        (n == -nuses) ? "  inputs:" : ",",
                        bytes);
                JS_free(cx, bytes);
            }
        }
        fprintf(tracefp, " @ %u\n", (uintN) (regs->sp - StackBase(fp)));
    }
}

#endif 

#ifdef JS_OPMETER

# include <stdlib.h>

# define HIST_NSLOTS            8






static uint32 succeeds[JSOP_LIMIT][256];
static uint32 slot_ops[JSOP_LIMIT][HIST_NSLOTS];

JS_STATIC_INTERPRET void
js_MeterOpcodePair(JSOp op1, JSOp op2)
{
    if (op1 != JSOP_STOP)
        ++succeeds[op1][op2];
}

JS_STATIC_INTERPRET void
js_MeterSlotOpcode(JSOp op, uint32 slot)
{
    if (slot < HIST_NSLOTS)
        ++slot_ops[op][slot];
}

typedef struct Edge {
    const char  *from;
    const char  *to;
    uint32      count;
} Edge;

static int
compare_edges(const void *a, const void *b)
{
    const Edge *ea = (const Edge *) a;
    const Edge *eb = (const Edge *) b;

    return (int32)eb->count - (int32)ea->count;
}

void
js_DumpOpMeters()
{
    const char *name, *from, *style;
    FILE *fp;
    uint32 total, count;
    uint32 i, j, nedges;
    Edge *graph;

    name = getenv("JS_OPMETER_FILE");
    if (!name)
        name = "/tmp/ops.dot";
    fp = fopen(name, "w");
    if (!fp) {
        perror(name);
        return;
    }

    total = nedges = 0;
    for (i = 0; i < JSOP_LIMIT; i++) {
        for (j = 0; j < JSOP_LIMIT; j++) {
            count = succeeds[i][j];
            if (count != 0) {
                total += count;
                ++nedges;
            }
        }
    }

# define SIGNIFICANT(count,total) (200. * (count) >= (total))

    graph = (Edge *) calloc(nedges, sizeof graph[0]);
    for (i = nedges = 0; i < JSOP_LIMIT; i++) {
        from = js_CodeName[i];
        for (j = 0; j < JSOP_LIMIT; j++) {
            count = succeeds[i][j];
            if (count != 0 && SIGNIFICANT(count, total)) {
                graph[nedges].from = from;
                graph[nedges].to = js_CodeName[j];
                graph[nedges].count = count;
                ++nedges;
            }
        }
    }
    qsort(graph, nedges, sizeof(Edge), compare_edges);

# undef SIGNIFICANT

    fputs("digraph {\n", fp);
    for (i = 0, style = NULL; i < nedges; i++) {
        JS_ASSERT(i == 0 || graph[i-1].count >= graph[i].count);
        if (!style || graph[i-1].count != graph[i].count) {
            style = (i > nedges * .75) ? "dotted" :
                    (i > nedges * .50) ? "dashed" :
                    (i > nedges * .25) ? "solid" : "bold";
        }
        fprintf(fp, "  %s -> %s [label=\"%lu\" style=%s]\n",
                graph[i].from, graph[i].to,
                (unsigned long)graph[i].count, style);
    }
    free(graph);
    fputs("}\n", fp);
    fclose(fp);

    name = getenv("JS_OPMETER_HIST");
    if (!name)
        name = "/tmp/ops.hist";
    fp = fopen(name, "w");
    if (!fp) {
        perror(name);
        return;
    }
    fputs("bytecode", fp);
    for (j = 0; j < HIST_NSLOTS; j++)
        fprintf(fp, "  slot %1u", (unsigned)j);
    putc('\n', fp);
    fputs("========", fp);
    for (j = 0; j < HIST_NSLOTS; j++)
        fputs(" =======", fp);
    putc('\n', fp);
    for (i = 0; i < JSOP_LIMIT; i++) {
        for (j = 0; j < HIST_NSLOTS; j++) {
            if (slot_ops[i][j] != 0) {
                
                fprintf(fp, "%-8.8s", js_CodeName[i]);
                for (j = 0; j < HIST_NSLOTS; j++)
                    fprintf(fp, " %7lu", (unsigned long)slot_ops[i][j]);
                putc('\n', fp);
                break;
            }
        }
    }
    fclose(fp);
}

#endif 

#endif 

#ifndef  jsinvoke_cpp___

#define PUSH(v)         (*regs.sp++ = (v))
#define PUSH_OPND(v)    PUSH(v)
#define STORE_OPND(n,v) (regs.sp[n] = (v))
#define POP()           (*--regs.sp)
#define POP_OPND()      POP()
#define FETCH_OPND(n)   (regs.sp[n])






#define STORE_NUMBER(cx, n, d)                                                \
    JS_BEGIN_MACRO                                                            \
        jsint i_;                                                             \
                                                                              \
        if (JSDOUBLE_IS_INT(d, i_) && INT_FITS_IN_JSVAL(i_))                  \
            regs.sp[n] = INT_TO_JSVAL(i_);                                    \
        else if (!js_NewDoubleInRootedValue(cx, d, &regs.sp[n]))              \
            goto error;                                                       \
    JS_END_MACRO

#define STORE_INT(cx, n, i)                                                   \
    JS_BEGIN_MACRO                                                            \
        if (INT_FITS_IN_JSVAL(i))                                             \
            regs.sp[n] = INT_TO_JSVAL(i);                                     \
        else if (!js_NewDoubleInRootedValue(cx, (jsdouble) (i), &regs.sp[n])) \
            goto error;                                                       \
    JS_END_MACRO

#define STORE_UINT(cx, n, u)                                                  \
    JS_BEGIN_MACRO                                                            \
        if ((u) <= JSVAL_INT_MAX)                                             \
            regs.sp[n] = INT_TO_JSVAL(u);                                     \
        else if (!js_NewDoubleInRootedValue(cx, (jsdouble) (u), &regs.sp[n])) \
            goto error;                                                       \
    JS_END_MACRO

#define FETCH_NUMBER(cx, n, d)                                                \
    JS_BEGIN_MACRO                                                            \
        jsval v_;                                                             \
                                                                              \
        v_ = FETCH_OPND(n);                                                   \
        VALUE_TO_NUMBER(cx, n, v_, d);                                        \
    JS_END_MACRO

#define FETCH_INT(cx, n, i)                                                   \
    JS_BEGIN_MACRO                                                            \
        jsval v_;                                                             \
                                                                              \
        v_= FETCH_OPND(n);                                                    \
        if (JSVAL_IS_INT(v_)) {                                               \
            i = JSVAL_TO_INT(v_);                                             \
        } else {                                                              \
            i = js_ValueToECMAInt32(cx, &regs.sp[n]);                         \
            if (JSVAL_IS_NULL(regs.sp[n]))                                    \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#define FETCH_UINT(cx, n, ui)                                                 \
    JS_BEGIN_MACRO                                                            \
        jsval v_;                                                             \
                                                                              \
        v_= FETCH_OPND(n);                                                    \
        if (JSVAL_IS_INT(v_)) {                                               \
            ui = (uint32) JSVAL_TO_INT(v_);                                   \
        } else {                                                              \
            ui = js_ValueToECMAUint32(cx, &regs.sp[n]);                       \
            if (JSVAL_IS_NULL(regs.sp[n]))                                    \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO





#define VALUE_TO_NUMBER(cx, n, v, d)                                          \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(v == regs.sp[n]);                                           \
        if (JSVAL_IS_INT(v)) {                                                \
            d = (jsdouble)JSVAL_TO_INT(v);                                    \
        } else if (JSVAL_IS_DOUBLE(v)) {                                      \
            d = *JSVAL_TO_DOUBLE(v);                                          \
        } else {                                                              \
            d = js_ValueToNumber(cx, &regs.sp[n]);                            \
            if (JSVAL_IS_NULL(regs.sp[n]))                                    \
                goto error;                                                   \
            JS_ASSERT(JSVAL_IS_NUMBER(regs.sp[n]) ||                          \
                      regs.sp[n] == JSVAL_TRUE);                              \
        }                                                                     \
    JS_END_MACRO

#define POP_BOOLEAN(cx, v, b)                                                 \
    JS_BEGIN_MACRO                                                            \
        v = FETCH_OPND(-1);                                                   \
        if (v == JSVAL_NULL) {                                                \
            b = JS_FALSE;                                                     \
        } else if (JSVAL_IS_BOOLEAN(v)) {                                     \
            b = JSVAL_TO_BOOLEAN(v);                                          \
        } else {                                                              \
            b = js_ValueToBoolean(v);                                         \
        }                                                                     \
        regs.sp--;                                                            \
    JS_END_MACRO

#define VALUE_TO_OBJECT(cx, n, v, obj)                                        \
    JS_BEGIN_MACRO                                                            \
        if (!JSVAL_IS_PRIMITIVE(v)) {                                         \
            obj = JSVAL_TO_OBJECT(v);                                         \
        } else {                                                              \
            obj = js_ValueToNonNullObject(cx, v);                             \
            if (!obj)                                                         \
                goto error;                                                   \
            STORE_OPND(n, OBJECT_TO_JSVAL(obj));                              \
        }                                                                     \
    JS_END_MACRO

#define FETCH_OBJECT(cx, n, v, obj)                                           \
    JS_BEGIN_MACRO                                                            \
        v = FETCH_OPND(n);                                                    \
        VALUE_TO_OBJECT(cx, n, v, obj);                                       \
    JS_END_MACRO

#define DEFAULT_VALUE(cx, n, hint, v)                                         \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));                                    \
        JS_ASSERT(v == regs.sp[n]);                                           \
        if (!OBJ_DEFAULT_VALUE(cx, JSVAL_TO_OBJECT(v), hint, &regs.sp[n]))    \
            goto error;                                                       \
        v = regs.sp[n];                                                       \
    JS_END_MACRO







#define CAN_DO_FAST_INC_DEC(v)     (((((v) << 1) ^ v) & 0x80000001) == 1)

JS_STATIC_ASSERT(JSVAL_INT == 1);
JS_STATIC_ASSERT(!CAN_DO_FAST_INC_DEC(INT_TO_JSVAL(JSVAL_INT_MIN)));
JS_STATIC_ASSERT(!CAN_DO_FAST_INC_DEC(INT_TO_JSVAL(JSVAL_INT_MAX)));





#if defined DEBUG_brendan || defined DEBUG_mrbkap || defined DEBUG_shaver
# define DEBUG_NOT_THROWING 1
#endif

#ifdef DEBUG_NOT_THROWING
# define ASSERT_NOT_THROWING(cx) JS_ASSERT(!(cx)->throwing)
#else
# define ASSERT_NOT_THROWING(cx)
#endif











#ifndef JS_OPMETER
# define METER_OP_INIT(op)
# define METER_OP_PAIR(op1,op2)
# define METER_SLOT_OP(op,slot)
#else






# define METER_OP_INIT(op)      ((op) = JSOP_STOP)
# define METER_OP_PAIR(op1,op2) (js_MeterOpcodePair(op1, op2))
# define METER_SLOT_OP(op,slot) (js_MeterSlotOpcode(op, slot))

#endif

#define MAX_INLINE_CALL_COUNT 3000








#ifndef JS_THREADED_INTERP
# if JS_VERSION >= 160 && (                                                   \
    __GNUC__ >= 3 ||                                                          \
    (__IBMC__ >= 700 && defined __IBM_COMPUTED_GOTO) ||                       \
    __SUNPRO_C >= 0x570)
#  define JS_THREADED_INTERP 1
# else
#  define JS_THREADED_INTERP 0
# endif
#endif





JS_STATIC_ASSERT(JSOP_NAME_LENGTH == JSOP_CALLNAME_LENGTH);
JS_STATIC_ASSERT(JSOP_GETGVAR_LENGTH == JSOP_CALLGVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETUPVAR_LENGTH == JSOP_CALLUPVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETARG_LENGTH == JSOP_CALLARG_LENGTH);
JS_STATIC_ASSERT(JSOP_GETLOCAL_LENGTH == JSOP_CALLLOCAL_LENGTH);
JS_STATIC_ASSERT(JSOP_XMLNAME_LENGTH == JSOP_CALLXMLNAME_LENGTH);





JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);
JS_STATIC_ASSERT(JSOP_NULL_LENGTH == JSOP_NULLTHIS_LENGTH);


JS_STATIC_ASSERT(JSOP_IFNE_LENGTH == JSOP_IFEQ_LENGTH);
JS_STATIC_ASSERT(JSOP_IFNE == JSOP_IFEQ + 1);


JS_STATIC_ASSERT(JSOP_INCNAME_LENGTH == JSOP_DECNAME_LENGTH);
JS_STATIC_ASSERT(JSOP_INCNAME_LENGTH == JSOP_NAMEINC_LENGTH);
JS_STATIC_ASSERT(JSOP_INCNAME_LENGTH == JSOP_NAMEDEC_LENGTH);

JS_REQUIRES_STACK JSBool
js_Interpret(JSContext *cx)
{
    JSRuntime *rt;
    JSStackFrame *fp;
    JSScript *script;
    uintN inlineCallCount;
    JSAtom **atoms;
    JSVersion currentVersion, originalVersion;
    JSFrameRegs regs;
    JSObject *obj, *obj2, *parent;
    JSBool ok, cond;
    jsint len;
    jsbytecode *endpc, *pc2;
    JSOp op, op2;
    jsatomid index;
    JSAtom *atom;
    uintN argc, attrs, flags;
    uint32 slot;
    jsval *vp, lval, rval, ltmp, rtmp;
    jsid id;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSString *str, *str2;
    jsint i, j;
    jsdouble d, d2;
    JSClass *clasp;
    JSFunction *fun;
    JSType type;
    jsint low, high, off, npairs;
    JSBool match;
#if JS_HAS_GETTER_SETTER
    JSPropertyOp getter, setter;
#endif
    JSAutoResolveFlags rf(cx, JSRESOLVE_INFER);

#ifdef __GNUC__
# define JS_EXTENSION __extension__
# define JS_EXTENSION_(s) __extension__ ({ s; })
#else
# define JS_EXTENSION
# define JS_EXTENSION_(s) s
#endif

#if JS_THREADED_INTERP
    static void *const normalJumpTable[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
        JS_EXTENSION &&L_##op,
# include "jsopcode.tbl"
# undef OPDEF
    };

    static void *const interruptJumpTable[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)              \
        JS_EXTENSION &&interrupt,
# include "jsopcode.tbl"
# undef OPDEF
    };

    register void * const *jumpTable = normalJumpTable;

    METER_OP_INIT(op);      

# define ENABLE_INTERRUPTS() ((void) (jumpTable = interruptJumpTable))

# ifdef JS_TRACER
#  define CHECK_RECORDER()                                                    \
    JS_ASSERT_IF(TRACE_RECORDER(cx), jumpTable == interruptJumpTable)
# else
#  define CHECK_RECORDER()  ((void)0)
# endif

# define DO_OP()            JS_BEGIN_MACRO                                    \
                                CHECK_RECORDER();                             \
                                JS_EXTENSION_(goto *jumpTable[op]);           \
                            JS_END_MACRO
# define DO_NEXT_OP(n)      JS_BEGIN_MACRO                                    \
                                METER_OP_PAIR(op, regs.pc[n]);                \
                                op = (JSOp) *(regs.pc += (n));                \
                                DO_OP();                                      \
                            JS_END_MACRO

# define BEGIN_CASE(OP)     L_##OP: CHECK_RECORDER();
# define END_CASE(OP)       DO_NEXT_OP(OP##_LENGTH);
# define END_VARLEN_CASE    DO_NEXT_OP(len);
# define ADD_EMPTY_CASE(OP) BEGIN_CASE(OP)                                    \
                                JS_ASSERT(js_CodeSpec[OP].length == 1);       \
                                op = (JSOp) *++regs.pc;                       \
                                DO_OP();

# define END_EMPTY_CASES

#else 

    register intN switchMask = 0;
    intN switchOp;

# define ENABLE_INTERRUPTS() ((void) (switchMask = -1))

# ifdef JS_TRACER
#  define CHECK_RECORDER()                                                    \
    JS_ASSERT_IF(TRACE_RECORDER(cx), switchMask == -1)
# else
#  define CHECK_RECORDER()  ((void)0)
# endif

# define DO_OP()            goto do_op
# define DO_NEXT_OP(n)      JS_BEGIN_MACRO                                    \
                                JS_ASSERT((n) == len);                        \
                                goto advance_pc;                              \
                            JS_END_MACRO

# define BEGIN_CASE(OP)     case OP: CHECK_RECORDER();
# define END_CASE(OP)       END_CASE_LEN(OP##_LENGTH)
# define END_CASE_LEN(n)    END_CASE_LENX(n)
# define END_CASE_LENX(n)   END_CASE_LEN##n





# define END_CASE_LEN1      goto advance_pc_by_one;
# define END_CASE_LEN2      len = 2; goto advance_pc;
# define END_CASE_LEN3      len = 3; goto advance_pc;
# define END_CASE_LEN4      len = 4; goto advance_pc;
# define END_CASE_LEN5      len = 5; goto advance_pc;
# define END_VARLEN_CASE    goto advance_pc;
# define ADD_EMPTY_CASE(OP) BEGIN_CASE(OP)
# define END_EMPTY_CASES    goto advance_pc_by_one;

#endif 

#ifdef JS_TRACER
    
    TraceRecorder *tr = NULL;
    if (JS_ON_TRACE(cx)) {
        tr = TRACE_RECORDER(cx);
        SET_TRACE_RECORDER(cx, NULL);
        JS_TRACE_MONITOR(cx).onTrace = JS_FALSE;
        



        if (tr) {
            if (tr->wasDeepAborted())
                tr->removeFragmentoReferences();
            else
                tr->pushAbortStack();
        }
    }
#endif

    
    JS_CHECK_RECURSION(cx, return JS_FALSE);

    rt = cx->runtime;

    
    fp = cx->fp;
    script = fp->script;
    JS_ASSERT(script->length != 0);

    
    inlineCallCount = 0;

    






    atoms = script->atomMap.vector;

#define LOAD_ATOM(PCOFF)                                                      \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(fp->imacpc                                                  \
                  ? atoms == COMMON_ATOMS_START(&rt->atomState) &&            \
                    GET_INDEX(regs.pc + PCOFF) < js_common_atom_count         \
                  : (size_t)(atoms - script->atomMap.vector) <                \
                    (size_t)(script->atomMap.length -                         \
                             GET_INDEX(regs.pc + PCOFF)));                    \
        atom = atoms[GET_INDEX(regs.pc + PCOFF)];                             \
    JS_END_MACRO

#define GET_FULL_INDEX(PCOFF)                                                 \
    (atoms - script->atomMap.vector + GET_INDEX(regs.pc + PCOFF))

#define LOAD_OBJECT(PCOFF)                                                    \
    JS_GET_SCRIPT_OBJECT(script, GET_FULL_INDEX(PCOFF), obj)

#define LOAD_FUNCTION(PCOFF)                                                  \
    JS_GET_SCRIPT_FUNCTION(script, GET_FULL_INDEX(PCOFF), fun)

#ifdef JS_TRACER

#define MONITOR_BRANCH()                                                      \
    JS_BEGIN_MACRO                                                            \
        if (TRACING_ENABLED(cx)) {                                            \
            if (js_MonitorLoopEdge(cx, inlineCallCount)) {                    \
                JS_ASSERT(TRACE_RECORDER(cx));                                \
                ENABLE_INTERRUPTS();                                          \
            }                                                                 \
            fp = cx->fp;                                                      \
            script = fp->script;                                              \
            atoms = fp->imacpc                                                \
                    ? COMMON_ATOMS_START(&rt->atomState)                      \
                    : script->atomMap.vector;                                 \
            currentVersion = (JSVersion) script->version;                     \
            JS_ASSERT(fp->regs == &regs);                                     \
            if (cx->throwing)                                                 \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#else 

#define MONITOR_BRANCH() ((void) 0)

#endif 

    



#define CHECK_BRANCH()                                                        \
    JS_BEGIN_MACRO                                                            \
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_SCRIPT_JUMP))                  \
            goto error;                                                       \
    JS_END_MACRO

#define BRANCH(n)                                                             \
    JS_BEGIN_MACRO                                                            \
        regs.pc += n;                                                         \
        if (n <= 0) {                                                         \
            CHECK_BRANCH();                                                   \
            MONITOR_BRANCH();                                                 \
        }                                                                     \
        op = (JSOp) *regs.pc;                                                 \
        DO_OP();                                                              \
    JS_END_MACRO

    MUST_FLOW_THROUGH("exit");
    ++cx->interpLevel;

    








    currentVersion = (JSVersion) script->version;
    originalVersion = (JSVersion) cx->version;
    if (currentVersion != originalVersion)
        js_SetVersion(cx, currentVersion);

    
    if (script->staticDepth < JS_DISPLAY_SIZE) {
        JSStackFrame **disp = &cx->display[script->staticDepth];
        fp->displaySave = *disp;
        *disp = fp;
    }
#ifdef DEBUG
    fp->pcDisabledSave = JS_PROPERTY_CACHE(cx).disabled;
#endif

# define CHECK_INTERRUPT_HANDLER()                                            \
    JS_BEGIN_MACRO                                                            \
        if (cx->debugHooks->interruptHandler)                                 \
            ENABLE_INTERRUPTS();                                              \
    JS_END_MACRO

    





    CHECK_INTERRUPT_HANDLER();

#if !JS_HAS_GENERATORS
    JS_ASSERT(!fp->regs);
#else
    
    if (JS_LIKELY(!fp->regs)) {
#endif
        ASSERT_NOT_THROWING(cx);
        regs.pc = script->code;
        regs.sp = StackBase(fp);
        fp->regs = &regs;
#if JS_HAS_GENERATORS
    } else {
        JSGenerator *gen;

        JS_ASSERT(fp->flags & JSFRAME_GENERATOR);
        gen = FRAME_TO_GENERATOR(fp);
        JS_ASSERT(fp->regs == &gen->savedRegs);
        regs = gen->savedRegs;
        fp->regs = &regs;
        JS_ASSERT((size_t) (regs.pc - script->code) <= script->length);
        JS_ASSERT((size_t) (regs.sp - StackBase(fp)) <= StackDepth(script));
        JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled >= 0);
        JS_PROPERTY_CACHE(cx).disabled += js_CountWithBlocks(cx, fp);

        



        if (cx->throwing) {
#ifdef DEBUG_NOT_THROWING
            if (cx->exception != JSVAL_ARETURN) {
                printf("JS INTERPRETER CALLED WITH PENDING EXCEPTION %lx\n",
                       (unsigned long) cx->exception);
            }
#endif
            goto error;
        }
    }
#endif 

    





    len = 0;
    DO_NEXT_OP(len);

#if JS_THREADED_INTERP
    







  interrupt:
#else 
    for (;;) {
      advance_pc_by_one:
        JS_ASSERT(js_CodeSpec[op].length == 1);
        len = 1;
      advance_pc:
        regs.pc += len;
        op = (JSOp) *regs.pc;
# ifdef DEBUG
        if (cx->tracefp)
            js_TraceOpcode(cx, len);
# endif

      do_op:
        CHECK_RECORDER();
        switchOp = intN(op) | switchMask;
      do_switch:
        switch (switchOp) {
          case -1:
            JS_ASSERT(switchMask == -1);
#endif 
          {
            bool moreInterrupts = false;
            JSTrapHandler handler = cx->debugHooks->interruptHandler;
            if (handler) {
#ifdef JS_TRACER
                if (TRACE_RECORDER(cx))
                    js_AbortRecording(cx, "interrupt handler");
#endif
                switch (handler(cx, script, regs.pc, &rval,
                                cx->debugHooks->interruptHandlerData)) {
                  case JSTRAP_ERROR:
                    goto error;
                  case JSTRAP_CONTINUE:
                    break;
                  case JSTRAP_RETURN:
                    fp->rval = rval;
                    ok = JS_TRUE;
                    goto forced_return;
                  case JSTRAP_THROW:
                    cx->throwing = JS_TRUE;
                    cx->exception = rval;
                    goto error;
                  default:;
                }
                moreInterrupts = true;
            }

#ifdef JS_TRACER
            TraceRecorder* tr = TRACE_RECORDER(cx);
            if (tr) {
                JSMonitorRecordingStatus status = TraceRecorder::monitorRecording(cx, tr, op);
                if (status == JSMRS_CONTINUE) {
                    moreInterrupts = true;
                } else if (status == JSMRS_IMACRO) {
                    atoms = COMMON_ATOMS_START(&rt->atomState);
                    op = JSOp(*regs.pc);
                    DO_OP();    
                } else {
                    JS_ASSERT(status == JSMRS_STOP);
                }
            }
#endif 

#if JS_THREADED_INTERP
            jumpTable = moreInterrupts ? interruptJumpTable : normalJumpTable;
            JS_EXTENSION_(goto *normalJumpTable[op]);
#else
            switchMask = moreInterrupts ? -1 : 0;
            switchOp = intN(op);
            goto do_switch;
#endif
          }

          
          ADD_EMPTY_CASE(JSOP_NOP)
          ADD_EMPTY_CASE(JSOP_CONDSWITCH)
          ADD_EMPTY_CASE(JSOP_TRY)
          ADD_EMPTY_CASE(JSOP_FINALLY)
#if JS_HAS_XML_SUPPORT
          ADD_EMPTY_CASE(JSOP_STARTXML)
          ADD_EMPTY_CASE(JSOP_STARTXMLEXPR)
#endif
          END_EMPTY_CASES

          
          BEGIN_CASE(JSOP_LINENO)
          END_CASE(JSOP_LINENO)

          BEGIN_CASE(JSOP_PUSH)
            PUSH_OPND(JSVAL_VOID);
          END_CASE(JSOP_PUSH)

          BEGIN_CASE(JSOP_POP)
            regs.sp--;
          END_CASE(JSOP_POP)

          BEGIN_CASE(JSOP_POPN)
            regs.sp -= GET_UINT16(regs.pc);
#ifdef DEBUG
            JS_ASSERT(StackBase(fp) <= regs.sp);
            obj = fp->blockChain;
            JS_ASSERT_IF(obj,
                         OBJ_BLOCK_DEPTH(cx, obj) + OBJ_BLOCK_COUNT(cx, obj)
                         <= (size_t) (regs.sp - StackBase(fp)));
            for (obj = fp->scopeChain; obj; obj = OBJ_GET_PARENT(cx, obj)) {
                clasp = OBJ_GET_CLASS(cx, obj);
                if (clasp != &js_BlockClass && clasp != &js_WithClass)
                    continue;
                if (OBJ_GET_PRIVATE(cx, obj) != fp)
                    break;
                JS_ASSERT(StackBase(fp) + OBJ_BLOCK_DEPTH(cx, obj)
                                     + ((clasp == &js_BlockClass)
                                        ? OBJ_BLOCK_COUNT(cx, obj)
                                        : 1)
                          <= regs.sp);
            }
#endif
          END_CASE(JSOP_POPN)

          BEGIN_CASE(JSOP_SETRVAL)
          BEGIN_CASE(JSOP_POPV)
            ASSERT_NOT_THROWING(cx);
            fp->rval = POP_OPND();
          END_CASE(JSOP_POPV)

          BEGIN_CASE(JSOP_ENTERWITH)
            if (!js_EnterWith(cx, -1))
                goto error;

            








            regs.sp[-1] = OBJECT_TO_JSVAL(fp->scopeChain);
          END_CASE(JSOP_ENTERWITH)

          BEGIN_CASE(JSOP_LEAVEWITH)
            JS_ASSERT(regs.sp[-1] == OBJECT_TO_JSVAL(fp->scopeChain));
            regs.sp--;
            js_LeaveWith(cx);
          END_CASE(JSOP_LEAVEWITH)

          BEGIN_CASE(JSOP_RETURN)
            CHECK_BRANCH();
            fp->rval = POP_OPND();
            

          BEGIN_CASE(JSOP_RETRVAL)    
          BEGIN_CASE(JSOP_STOP)
            



            ASSERT_NOT_THROWING(cx);

            if (fp->imacpc) {
                



                JS_ASSERT(op == JSOP_STOP);

              end_imacro:
                JS_ASSERT((uintN)(regs.sp - fp->slots) <= script->nslots);
                regs.pc = fp->imacpc + js_CodeSpec[*fp->imacpc].length;
                fp->imacpc = NULL;
                atoms = script->atomMap.vector;
                op = JSOp(*regs.pc);
                DO_OP();
            }

            JS_ASSERT(regs.sp == StackBase(fp));
            if ((fp->flags & JSFRAME_CONSTRUCTING) &&
                JSVAL_IS_PRIMITIVE(fp->rval)) {
                if (!fp->fun) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_BAD_NEW_RESULT,
                                         js_ValueToPrintableString(cx, rval));
                    goto error;
                }
                fp->rval = OBJECT_TO_JSVAL(fp->thisp);
            }
            ok = JS_TRUE;
            if (inlineCallCount)
          inline_return:
            {
                JSInlineFrame *ifp = (JSInlineFrame *) fp;
                void *hookData = ifp->hookData;

                JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled == fp->pcDisabledSave);
                JS_ASSERT(!fp->blockChain);
                JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));

                if (script->staticDepth < JS_DISPLAY_SIZE)
                    cx->display[script->staticDepth] = fp->displaySave;

                if (hookData) {
                    JSInterpreterHook hook;
                    JSBool status;

                    hook = cx->debugHooks->callHook;
                    if (hook) {
                        



                        status = ok;
                        hook(cx, fp, JS_FALSE, &status, hookData);
                        ok = status;
                        CHECK_INTERRUPT_HANDLER();
                    }
                }

                





                if (fp->callobj)
                    ok &= js_PutCallObject(cx, fp);

                if (fp->argsobj)
                    ok &= js_PutArgsObject(cx, fp);

#ifdef INCLUDE_MOZILLA_DTRACE
                
                if (JAVASCRIPT_FUNCTION_RVAL_ENABLED())
                    jsdtrace_function_rval(cx, fp, fp->fun);
                if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
                    jsdtrace_function_return(cx, fp, fp->fun);
#endif

                
                if (JS_LIKELY(cx->version == currentVersion)) {
                    currentVersion = ifp->callerVersion;
                    if (currentVersion != cx->version)
                        js_SetVersion(cx, currentVersion);
                }

                




                if (fp->flags & JSFRAME_CONSTRUCTING) {
                    if (JSVAL_IS_PRIMITIVE(fp->rval))
                        fp->rval = OBJECT_TO_JSVAL(fp->thisp);
                    JS_RUNTIME_METER(cx->runtime, constructs);
                }

                
                regs = ifp->callerRegs;

                
                regs.sp -= 1 + (size_t) ifp->frame.argc;
                regs.sp[-1] = fp->rval;

                
                cx->fp = fp = fp->down;
                JS_ASSERT(fp->regs == &ifp->callerRegs);
                fp->regs = &regs;
                JS_ARENA_RELEASE(&cx->stackPool, ifp->mark);

                
                script = fp->script;
                atoms = fp->imacpc
                        ? COMMON_ATOMS_START(&rt->atomState)
                        : script->atomMap.vector;

                
                inlineCallCount--;
                if (JS_LIKELY(ok)) {
                    TRACE_0(LeaveFrame);
                    JS_ASSERT(js_CodeSpec[*regs.pc].length == JSOP_CALL_LENGTH);
                    len = JSOP_CALL_LENGTH;
                    DO_NEXT_OP(len);
                }
                goto error;
            }
            goto exit;

          BEGIN_CASE(JSOP_DEFAULT)
            (void) POP();
            
          BEGIN_CASE(JSOP_GOTO)
            len = GET_JUMP_OFFSET(regs.pc);
            BRANCH(len);
          END_CASE(JSOP_GOTO)

          BEGIN_CASE(JSOP_IFEQ)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMP_OFFSET(regs.pc);
                BRANCH(len);
            }
          END_CASE(JSOP_IFEQ)

          BEGIN_CASE(JSOP_IFNE)
            POP_BOOLEAN(cx, rval, cond);
            if (cond != JS_FALSE) {
                len = GET_JUMP_OFFSET(regs.pc);
                BRANCH(len);
            }
          END_CASE(JSOP_IFNE)

          BEGIN_CASE(JSOP_OR)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_TRUE) {
                len = GET_JUMP_OFFSET(regs.pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_OR)

          BEGIN_CASE(JSOP_AND)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMP_OFFSET(regs.pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_AND)

          BEGIN_CASE(JSOP_DEFAULTX)
            (void) POP();
            
          BEGIN_CASE(JSOP_GOTOX)
            len = GET_JUMPX_OFFSET(regs.pc);
            BRANCH(len);
          END_CASE(JSOP_GOTOX);

          BEGIN_CASE(JSOP_IFEQX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMPX_OFFSET(regs.pc);
                BRANCH(len);
            }
          END_CASE(JSOP_IFEQX)

          BEGIN_CASE(JSOP_IFNEX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond != JS_FALSE) {
                len = GET_JUMPX_OFFSET(regs.pc);
                BRANCH(len);
            }
          END_CASE(JSOP_IFNEX)

          BEGIN_CASE(JSOP_ORX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_TRUE) {
                len = GET_JUMPX_OFFSET(regs.pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_ORX)

          BEGIN_CASE(JSOP_ANDX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMPX_OFFSET(regs.pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_ANDX)







#define FETCH_ELEMENT_ID(obj, n, id)                                          \
    JS_BEGIN_MACRO                                                            \
        jsval idval_ = FETCH_OPND(n);                                         \
        if (JSVAL_IS_INT(idval_)) {                                           \
            id = INT_JSVAL_TO_JSID(idval_);                                   \
        } else {                                                              \
            if (!js_InternNonIntElementId(cx, obj, idval_, &id))              \
                goto error;                                                   \
            regs.sp[n] = ID_TO_VALUE(id);                                     \
        }                                                                     \
    JS_END_MACRO

#define TRY_BRANCH_AFTER_COND(cond,spdec)                                     \
    JS_BEGIN_MACRO                                                            \
        uintN diff_;                                                          \
        JS_ASSERT(js_CodeSpec[op].length == 1);                               \
        diff_ = (uintN) regs.pc[1] - (uintN) JSOP_IFEQ;                       \
        if (diff_ <= 1) {                                                     \
            regs.sp -= spdec;                                                 \
            if (cond == (diff_ != 0)) {                                       \
                ++regs.pc;                                                    \
                len = GET_JUMP_OFFSET(regs.pc);                               \
                BRANCH(len);                                                  \
            }                                                                 \
            len = 1 + JSOP_IFEQ_LENGTH;                                       \
            DO_NEXT_OP(len);                                                  \
        }                                                                     \
    JS_END_MACRO

          BEGIN_CASE(JSOP_IN)
            rval = FETCH_OPND(-1);
            if (JSVAL_IS_PRIMITIVE(rval)) {
                js_ReportValueError(cx, JSMSG_IN_NOT_OBJECT, -1, rval, NULL);
                goto error;
            }
            obj = JSVAL_TO_OBJECT(rval);
            FETCH_ELEMENT_ID(obj, -2, id);
            if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
                goto error;
            cond = prop != NULL;
            if (prop)
                OBJ_DROP_PROPERTY(cx, obj2, prop);
            TRY_BRANCH_AFTER_COND(cond, 2);
            regs.sp--;
            STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));
          END_CASE(JSOP_IN)

          BEGIN_CASE(JSOP_ITER)
            JS_ASSERT(regs.sp > StackBase(fp));
            flags = regs.pc[1];
            if (!js_ValueToIterator(cx, flags, &regs.sp[-1]))
                goto error;
            CHECK_INTERRUPT_HANDLER();
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(regs.sp[-1]));
            PUSH(JSVAL_VOID);
          END_CASE(JSOP_ITER)

          BEGIN_CASE(JSOP_NEXTITER)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(regs.sp[-2]));
            if (!js_CallIteratorNext(cx, JSVAL_TO_OBJECT(regs.sp[-2]), &regs.sp[-1]))
                goto error;
            CHECK_INTERRUPT_HANDLER();
            rval = BOOLEAN_TO_JSVAL(regs.sp[-1] != JSVAL_HOLE);
            PUSH(rval);
            TRACE_0(IteratorNextComplete);
          END_CASE(JSOP_NEXTITER)

          BEGIN_CASE(JSOP_ENDITER)
            



            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            ok = js_CloseIterator(cx, regs.sp[-2]);
            regs.sp -= 2;
            if (!ok)
                goto error;
          END_CASE(JSOP_ENDITER)

          BEGIN_CASE(JSOP_FORARG)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            slot = GET_ARGNO(regs.pc);
            JS_ASSERT(slot < fp->fun->nargs);
            fp->argv[slot] = regs.sp[-1];
          END_CASE(JSOP_FORARG)

          BEGIN_CASE(JSOP_FORLOCAL)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < fp->script->nslots);
            vp = &fp->slots[slot];
            GC_POKE(cx, *vp);
            *vp = regs.sp[-1];
          END_CASE(JSOP_FORLOCAL)

          BEGIN_CASE(JSOP_FORNAME)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
                goto error;
            if (prop)
                OBJ_DROP_PROPERTY(cx, obj2, prop);
            ok = OBJ_SET_PROPERTY(cx, obj, id, &regs.sp[-1]);
            if (!ok)
                goto error;
          END_CASE(JSOP_FORNAME)

          BEGIN_CASE(JSOP_FORPROP)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            FETCH_OBJECT(cx, -1, lval, obj);
            ok = OBJ_SET_PROPERTY(cx, obj, id, &regs.sp[-2]);
            if (!ok)
                goto error;
            regs.sp--;
          END_CASE(JSOP_FORPROP)

          BEGIN_CASE(JSOP_FORELEM)
            





            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            rval = FETCH_OPND(-1);
            PUSH(rval);
          END_CASE(JSOP_FORELEM)

          BEGIN_CASE(JSOP_DUP)
            JS_ASSERT(regs.sp > StackBase(fp));
            rval = FETCH_OPND(-1);
            PUSH(rval);
          END_CASE(JSOP_DUP)

          BEGIN_CASE(JSOP_DUP2)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            lval = FETCH_OPND(-2);
            rval = FETCH_OPND(-1);
            PUSH(lval);
            PUSH(rval);
          END_CASE(JSOP_DUP2)

          BEGIN_CASE(JSOP_SWAP)
            JS_ASSERT(regs.sp - 2 >= StackBase(fp));
            lval = FETCH_OPND(-2);
            rval = FETCH_OPND(-1);
            STORE_OPND(-1, lval);
            STORE_OPND(-2, rval);
          END_CASE(JSOP_SWAP)
          
          BEGIN_CASE(JSOP_PICK)
            i = regs.pc[1];
            JS_ASSERT(regs.sp - (i+1) >= StackBase(fp));
            lval = regs.sp[-(i+1)];
            memmove(regs.sp - (i+1), regs.sp - i, sizeof(jsval)*i);
            regs.sp[-1] = lval;
          END_CASE(JSOP_PICK)

#define PROPERTY_OP(n, call)                                                  \
    JS_BEGIN_MACRO                                                            \
        /* Fetch the left part and resolve it to a non-null object. */        \
        FETCH_OBJECT(cx, n, lval, obj);                                       \
                                                                              \
        /* Get or set the property. */                                        \
        if (!call)                                                            \
            goto error;                                                       \
    JS_END_MACRO

#define ELEMENT_OP(n, call)                                                   \
    JS_BEGIN_MACRO                                                            \
        /* Fetch the left part and resolve it to a non-null object. */        \
        FETCH_OBJECT(cx, n - 1, lval, obj);                                   \
                                                                              \
        /* Fetch index and convert it to id suitable for use with obj. */     \
        FETCH_ELEMENT_ID(obj, n, id);                                         \
                                                                              \
        /* Get or set the element. */                                         \
        if (!call)                                                            \
            goto error;                                                       \
    JS_END_MACRO

#define NATIVE_GET(cx,obj,pobj,sprop,vp)                                      \
    JS_BEGIN_MACRO                                                            \
        if (SPROP_HAS_STUB_GETTER(sprop)) {                                   \
            /* Fast path for Object instance properties. */                   \
            JS_ASSERT((sprop)->slot != SPROP_INVALID_SLOT ||                  \
                      !SPROP_HAS_STUB_SETTER(sprop));                         \
            *vp = ((sprop)->slot != SPROP_INVALID_SLOT)                       \
                  ? LOCKED_OBJ_GET_SLOT(pobj, (sprop)->slot)                  \
                  : JSVAL_VOID;                                               \
        } else {                                                              \
            if (!js_NativeGet(cx, obj, pobj, sprop, vp))                      \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#define NATIVE_SET(cx,obj,sprop,vp)                                           \
    JS_BEGIN_MACRO                                                            \
        if (SPROP_HAS_STUB_SETTER(sprop) &&                                   \
            (sprop)->slot != SPROP_INVALID_SLOT) {                            \
            /* Fast path for, e.g., Object instance properties. */            \
            LOCKED_OBJ_WRITE_BARRIER(cx, obj, (sprop)->slot, *vp);            \
        } else {                                                              \
            if (!js_NativeSet(cx, obj, sprop, vp))                            \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO





#if defined DEBUG && !defined JS_THREADSAFE
# define ASSERT_VALID_PROPERTY_CACHE_HIT(pcoff,obj,pobj,entry)                \
    do {                                                                      \
        JSAtom *atom_;                                                        \
        JSObject *obj_, *pobj_;                                               \
        JSProperty *prop_;                                                    \
        JSScopeProperty *sprop_;                                              \
        uint32 sample_ = rt->gcNumber;                                        \
        if (pcoff >= 0)                                                       \
            GET_ATOM_FROM_BYTECODE(script, regs.pc, pcoff, atom_);            \
        else                                                                  \
            atom_ = rt->atomState.lengthAtom;                                 \
        if (JOF_OPMODE(*regs.pc) == JOF_NAME) {                               \
            ok = js_FindProperty(cx, ATOM_TO_JSID(atom_), &obj_, &pobj_,      \
                                 &prop_);                                     \
        } else {                                                              \
            obj_ = obj;                                                       \
            ok = js_LookupProperty(cx, obj, ATOM_TO_JSID(atom_), &pobj_,      \
                                   &prop_);                                   \
        }                                                                     \
        if (!ok)                                                              \
            goto error;                                                       \
        if (rt->gcNumber != sample_)                                          \
            break;                                                            \
        JS_ASSERT(prop_);                                                     \
        JS_ASSERT(pobj_ == pobj);                                             \
        sprop_ = (JSScopeProperty *) prop_;                                   \
        if (PCVAL_IS_SLOT(entry->vword)) {                                    \
            JS_ASSERT(PCVAL_TO_SLOT(entry->vword) == sprop_->slot);           \
        } else if (PCVAL_IS_SPROP(entry->vword)) {                            \
            JS_ASSERT(PCVAL_TO_SPROP(entry->vword) == sprop_);                \
        } else {                                                              \
            jsval v_;                                                         \
            JS_ASSERT(PCVAL_IS_OBJECT(entry->vword));                         \
            JS_ASSERT(entry->vword != PCVAL_NULL);                            \
            JS_ASSERT(SCOPE_IS_BRANDED(OBJ_SCOPE(pobj)));                     \
            JS_ASSERT(SPROP_HAS_STUB_GETTER(sprop_));                         \
            JS_ASSERT(SPROP_HAS_VALID_SLOT(sprop_, OBJ_SCOPE(pobj_)));        \
            v_ = LOCKED_OBJ_GET_SLOT(pobj_, sprop_->slot);                    \
            JS_ASSERT(VALUE_IS_FUNCTION(cx, v_));                             \
            JS_ASSERT(PCVAL_TO_OBJECT(entry->vword) == JSVAL_TO_OBJECT(v_));  \
        }                                                                     \
        OBJ_DROP_PROPERTY(cx, pobj_, prop_);                                  \
    } while (0)
#else
# define ASSERT_VALID_PROPERTY_CACHE_HIT(pcoff,obj,pobj,entry) ((void) 0)
#endif












#define SKIP_POP_AFTER_SET(oplen,spdec)                                       \
            if (regs.pc[oplen] == JSOP_POP) {                                 \
                regs.sp -= spdec;                                             \
                regs.pc += oplen + JSOP_POP_LENGTH;                           \
                op = (JSOp) *regs.pc;                                         \
                DO_OP();                                                      \
            }

#define END_SET_CASE(OP)                                                      \
            SKIP_POP_AFTER_SET(OP##_LENGTH, 1);                               \
          END_CASE(OP)

#define END_SET_CASE_STORE_RVAL(OP,spdec)                                     \
            SKIP_POP_AFTER_SET(OP##_LENGTH, spdec);                           \
            rval = FETCH_OPND(-1);                                            \
            regs.sp -= (spdec) - 1;                                           \
            STORE_OPND(-1, rval);                                             \
          END_CASE(OP)

          BEGIN_CASE(JSOP_SETCONST)
            LOAD_ATOM(0);
            obj = fp->varobj;
            rval = FETCH_OPND(-1);
            if (!OBJ_DEFINE_PROPERTY(cx, obj, ATOM_TO_JSID(atom), rval,
                                     JS_PropertyStub, JS_PropertyStub,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL)) {
                goto error;
            }
          END_SET_CASE(JSOP_SETCONST);

#if JS_HAS_DESTRUCTURING
          BEGIN_CASE(JSOP_ENUMCONSTELEM)
            rval = FETCH_OPND(-3);
            FETCH_OBJECT(cx, -2, lval, obj);
            FETCH_ELEMENT_ID(obj, -1, id);
            if (!OBJ_DEFINE_PROPERTY(cx, obj, id, rval,
                                     JS_PropertyStub, JS_PropertyStub,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL)) {
                goto error;
            }
            regs.sp -= 3;
          END_CASE(JSOP_ENUMCONSTELEM)
#endif

          BEGIN_CASE(JSOP_BINDNAME)
            do {
                JSPropCacheEntry *entry;

                obj = fp->scopeChain;
                if (JS_LIKELY(OBJ_IS_NATIVE(obj))) {
                    PROPERTY_CACHE_TEST(cx, regs.pc, obj, obj2, entry, atom);
                    if (!atom) {
                        ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
                        JS_UNLOCK_OBJ(cx, obj2);
                        break;
                    }
                } else {
                    entry = NULL;
                    LOAD_ATOM(0);
                }
                id = ATOM_TO_JSID(atom);
                obj = js_FindIdentifierBase(cx, id, entry);
                if (!obj)
                    goto error;
            } while (0);
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_BINDNAME)

          BEGIN_CASE(JSOP_IMACOP)
            JS_ASSERT(JS_UPTRDIFF(fp->imacpc, script->code) < script->length);
            op = JSOp(*fp->imacpc);
            DO_OP();

#define BITWISE_OP(OP)                                                        \
    JS_BEGIN_MACRO                                                            \
        FETCH_INT(cx, -2, i);                                                 \
        FETCH_INT(cx, -1, j);                                                 \
        i = i OP j;                                                           \
        regs.sp--;                                                            \
        STORE_INT(cx, -1, i);                                                 \
    JS_END_MACRO

          BEGIN_CASE(JSOP_BITOR)
            BITWISE_OP(|);
          END_CASE(JSOP_BITOR)

          BEGIN_CASE(JSOP_BITXOR)
            BITWISE_OP(^);
          END_CASE(JSOP_BITXOR)

          BEGIN_CASE(JSOP_BITAND)
            BITWISE_OP(&);
          END_CASE(JSOP_BITAND)

#define RELATIONAL_OP(OP)                                                     \
    JS_BEGIN_MACRO                                                            \
        rval = FETCH_OPND(-1);                                                \
        lval = FETCH_OPND(-2);                                                \
        /* Optimize for two int-tagged operands (typical loop control). */    \
        if ((lval & rval) & JSVAL_INT) {                                      \
            cond = JSVAL_TO_INT(lval) OP JSVAL_TO_INT(rval);                  \
        } else {                                                              \
            if (!JSVAL_IS_PRIMITIVE(lval))                                    \
                DEFAULT_VALUE(cx, -2, JSTYPE_NUMBER, lval);                   \
            if (!JSVAL_IS_PRIMITIVE(rval))                                    \
                DEFAULT_VALUE(cx, -1, JSTYPE_NUMBER, rval);                   \
            if (JSVAL_IS_STRING(lval) && JSVAL_IS_STRING(rval)) {             \
                str  = JSVAL_TO_STRING(lval);                                 \
                str2 = JSVAL_TO_STRING(rval);                                 \
                cond = js_CompareStrings(str, str2) OP 0;                     \
            } else {                                                          \
                VALUE_TO_NUMBER(cx, -2, lval, d);                             \
                VALUE_TO_NUMBER(cx, -1, rval, d2);                            \
                cond = JSDOUBLE_COMPARE(d, OP, d2, JS_FALSE);                 \
            }                                                                 \
        }                                                                     \
        TRY_BRANCH_AFTER_COND(cond, 2);                                       \
        regs.sp--;                                                            \
        STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));                               \
    JS_END_MACRO






#if JS_HAS_XML_SUPPORT
#define XML_EQUALITY_OP(OP)                                                   \
    if ((ltmp == JSVAL_OBJECT &&                                              \
         (obj2 = JSVAL_TO_OBJECT(lval)) &&                                    \
         OBJECT_IS_XML(cx, obj2)) ||                                          \
        (rtmp == JSVAL_OBJECT &&                                              \
         (obj2 = JSVAL_TO_OBJECT(rval)) &&                                    \
         OBJECT_IS_XML(cx, obj2))) {                                          \
        JSXMLObjectOps *ops;                                                  \
                                                                              \
        ops = (JSXMLObjectOps *) obj2->map->ops;                              \
        if (obj2 == JSVAL_TO_OBJECT(rval))                                    \
            rval = lval;                                                      \
        if (!ops->equality(cx, obj2, rval, &cond))                            \
            goto error;                                                       \
        cond = cond OP JS_TRUE;                                               \
    } else

#define EXTENDED_EQUALITY_OP(OP)                                              \
    if (ltmp == JSVAL_OBJECT &&                                               \
        (obj2 = JSVAL_TO_OBJECT(lval)) &&                                     \
        ((clasp = OBJ_GET_CLASS(cx, obj2))->flags & JSCLASS_IS_EXTENDED)) {   \
        JSExtendedClass *xclasp;                                              \
                                                                              \
        xclasp = (JSExtendedClass *) clasp;                                   \
        if (!xclasp->equality(cx, obj2, rval, &cond))                         \
            goto error;                                                       \
        cond = cond OP JS_TRUE;                                               \
    } else
#else
#define XML_EQUALITY_OP(OP)
#define EXTENDED_EQUALITY_OP(OP)
#endif

#define EQUALITY_OP(OP, IFNAN)                                                \
    JS_BEGIN_MACRO                                                            \
        rval = FETCH_OPND(-1);                                                \
        lval = FETCH_OPND(-2);                                                \
        ltmp = JSVAL_TAG(lval);                                               \
        rtmp = JSVAL_TAG(rval);                                               \
        XML_EQUALITY_OP(OP)                                                   \
        if (ltmp == rtmp) {                                                   \
            if (ltmp == JSVAL_STRING) {                                       \
                str  = JSVAL_TO_STRING(lval);                                 \
                str2 = JSVAL_TO_STRING(rval);                                 \
                cond = js_EqualStrings(str, str2) OP JS_TRUE;                 \
            } else if (ltmp == JSVAL_DOUBLE) {                                \
                d  = *JSVAL_TO_DOUBLE(lval);                                  \
                d2 = *JSVAL_TO_DOUBLE(rval);                                  \
                cond = JSDOUBLE_COMPARE(d, OP, d2, IFNAN);                    \
            } else {                                                          \
                EXTENDED_EQUALITY_OP(OP)                                      \
                /* Handle all undefined (=>NaN) and int combinations. */      \
                cond = lval OP rval;                                          \
            }                                                                 \
        } else {                                                              \
            if (JSVAL_IS_NULL(lval) || JSVAL_IS_VOID(lval)) {                 \
                cond = (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval)) OP 1;     \
            } else if (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval)) {          \
                cond = 1 OP 0;                                                \
            } else {                                                          \
                if (ltmp == JSVAL_OBJECT) {                                   \
                    DEFAULT_VALUE(cx, -2, JSTYPE_VOID, lval);                 \
                    ltmp = JSVAL_TAG(lval);                                   \
                } else if (rtmp == JSVAL_OBJECT) {                            \
                    DEFAULT_VALUE(cx, -1, JSTYPE_VOID, rval);                 \
                    rtmp = JSVAL_TAG(rval);                                   \
                }                                                             \
                if (ltmp == JSVAL_STRING && rtmp == JSVAL_STRING) {           \
                    str  = JSVAL_TO_STRING(lval);                             \
                    str2 = JSVAL_TO_STRING(rval);                             \
                    cond = js_EqualStrings(str, str2) OP JS_TRUE;             \
                } else {                                                      \
                    VALUE_TO_NUMBER(cx, -2, lval, d);                         \
                    VALUE_TO_NUMBER(cx, -1, rval, d2);                        \
                    cond = JSDOUBLE_COMPARE(d, OP, d2, IFNAN);                \
                }                                                             \
            }                                                                 \
        }                                                                     \
        TRY_BRANCH_AFTER_COND(cond, 2);                                       \
        regs.sp--;                                                            \
        STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));                               \
    JS_END_MACRO

          BEGIN_CASE(JSOP_EQ)
            EQUALITY_OP(==, JS_FALSE);
          END_CASE(JSOP_EQ)

          BEGIN_CASE(JSOP_NE)
            EQUALITY_OP(!=, JS_TRUE);
          END_CASE(JSOP_NE)

#define STRICT_EQUALITY_OP(OP)                                                \
    JS_BEGIN_MACRO                                                            \
        rval = FETCH_OPND(-1);                                                \
        lval = FETCH_OPND(-2);                                                \
        cond = js_StrictlyEqual(cx, lval, rval) OP JS_TRUE;                   \
        regs.sp--;                                                            \
        STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));                               \
    JS_END_MACRO

          BEGIN_CASE(JSOP_STRICTEQ)
            STRICT_EQUALITY_OP(==);
          END_CASE(JSOP_STRICTEQ)

          BEGIN_CASE(JSOP_STRICTNE)
            STRICT_EQUALITY_OP(!=);
          END_CASE(JSOP_STRICTNE)

          BEGIN_CASE(JSOP_CASE)
            STRICT_EQUALITY_OP(==);
            (void) POP();
            if (cond) {
                len = GET_JUMP_OFFSET(regs.pc);
                BRANCH(len);
            }
            PUSH(lval);
          END_CASE(JSOP_CASE)

          BEGIN_CASE(JSOP_CASEX)
            STRICT_EQUALITY_OP(==);
            (void) POP();
            if (cond) {
                len = GET_JUMPX_OFFSET(regs.pc);
                BRANCH(len);
            }
            PUSH(lval);
          END_CASE(JSOP_CASEX)

          BEGIN_CASE(JSOP_LT)
            RELATIONAL_OP(<);
          END_CASE(JSOP_LT)

          BEGIN_CASE(JSOP_LE)
            RELATIONAL_OP(<=);
          END_CASE(JSOP_LE)

          BEGIN_CASE(JSOP_GT)
            RELATIONAL_OP(>);
          END_CASE(JSOP_GT)

          BEGIN_CASE(JSOP_GE)
            RELATIONAL_OP(>=);
          END_CASE(JSOP_GE)

#undef EQUALITY_OP
#undef RELATIONAL_OP

#define SIGNED_SHIFT_OP(OP)                                                   \
    JS_BEGIN_MACRO                                                            \
        FETCH_INT(cx, -2, i);                                                 \
        FETCH_INT(cx, -1, j);                                                 \
        i = i OP (j & 31);                                                    \
        regs.sp--;                                                            \
        STORE_INT(cx, -1, i);                                                 \
    JS_END_MACRO

          BEGIN_CASE(JSOP_LSH)
            SIGNED_SHIFT_OP(<<);
          END_CASE(JSOP_LSH)

          BEGIN_CASE(JSOP_RSH)
            SIGNED_SHIFT_OP(>>);
          END_CASE(JSOP_RSH)

          BEGIN_CASE(JSOP_URSH)
          {
            uint32 u;

            FETCH_UINT(cx, -2, u);
            FETCH_INT(cx, -1, j);
            u >>= (j & 31);
            regs.sp--;
            STORE_UINT(cx, -1, u);
          }
          END_CASE(JSOP_URSH)

#undef BITWISE_OP
#undef SIGNED_SHIFT_OP

          BEGIN_CASE(JSOP_ADD)
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
#if JS_HAS_XML_SUPPORT
            if (!JSVAL_IS_PRIMITIVE(lval) &&
                (obj2 = JSVAL_TO_OBJECT(lval), OBJECT_IS_XML(cx, obj2)) &&
                VALUE_IS_XML(cx, rval)) {
                JSXMLObjectOps *ops;

                ops = (JSXMLObjectOps *) obj2->map->ops;
                if (!ops->concatenate(cx, obj2, rval, &rval))
                    goto error;
                regs.sp--;
                STORE_OPND(-1, rval);
            } else
#endif
            {
                if (!JSVAL_IS_PRIMITIVE(lval))
                    DEFAULT_VALUE(cx, -2, JSTYPE_VOID, lval);
                if (!JSVAL_IS_PRIMITIVE(rval))
                    DEFAULT_VALUE(cx, -1, JSTYPE_VOID, rval);
                if ((cond = JSVAL_IS_STRING(lval)) || JSVAL_IS_STRING(rval)) {
                    if (cond) {
                        str = JSVAL_TO_STRING(lval);
                        str2 = js_ValueToString(cx, rval);
                        if (!str2)
                            goto error;
                        regs.sp[-1] = STRING_TO_JSVAL(str2);
                    } else {
                        str2 = JSVAL_TO_STRING(rval);
                        str = js_ValueToString(cx, lval);
                        if (!str)
                            goto error;
                        regs.sp[-2] = STRING_TO_JSVAL(str);
                    }
                    str = js_ConcatStrings(cx, str, str2);
                    if (!str)
                        goto error;
                    regs.sp--;
                    STORE_OPND(-1, STRING_TO_JSVAL(str));
                } else {
                    VALUE_TO_NUMBER(cx, -2, lval, d);
                    VALUE_TO_NUMBER(cx, -1, rval, d2);
                    d += d2;
                    regs.sp--;
                    STORE_NUMBER(cx, -1, d);
                }
            }
          END_CASE(JSOP_ADD)

#define BINARY_OP(OP)                                                         \
    JS_BEGIN_MACRO                                                            \
        FETCH_NUMBER(cx, -2, d);                                              \
        FETCH_NUMBER(cx, -1, d2);                                             \
        d = d OP d2;                                                          \
        regs.sp--;                                                            \
        STORE_NUMBER(cx, -1, d);                                              \
    JS_END_MACRO

          BEGIN_CASE(JSOP_SUB)
            BINARY_OP(-);
          END_CASE(JSOP_SUB)

          BEGIN_CASE(JSOP_MUL)
            BINARY_OP(*);
          END_CASE(JSOP_MUL)

          BEGIN_CASE(JSOP_DIV)
            FETCH_NUMBER(cx, -1, d2);
            FETCH_NUMBER(cx, -2, d);
            regs.sp--;
            if (d2 == 0) {
#ifdef XP_WIN
                
                if (JSDOUBLE_IS_NaN(d2))
                    rval = DOUBLE_TO_JSVAL(rt->jsNaN);
                else
#endif
                if (d == 0 || JSDOUBLE_IS_NaN(d))
                    rval = DOUBLE_TO_JSVAL(rt->jsNaN);
                else if ((JSDOUBLE_HI32(d) ^ JSDOUBLE_HI32(d2)) >> 31)
                    rval = DOUBLE_TO_JSVAL(rt->jsNegativeInfinity);
                else
                    rval = DOUBLE_TO_JSVAL(rt->jsPositiveInfinity);
                STORE_OPND(-1, rval);
            } else {
                d /= d2;
                STORE_NUMBER(cx, -1, d);
            }
          END_CASE(JSOP_DIV)

          BEGIN_CASE(JSOP_MOD)
            FETCH_NUMBER(cx, -1, d2);
            FETCH_NUMBER(cx, -2, d);
            regs.sp--;
            if (d2 == 0) {
                STORE_OPND(-1, DOUBLE_TO_JSVAL(rt->jsNaN));
            } else {
#ifdef XP_WIN
              
              if (!(JSDOUBLE_IS_FINITE(d) && JSDOUBLE_IS_INFINITE(d2)))
#endif
                d = fmod(d, d2);
                STORE_NUMBER(cx, -1, d);
            }
          END_CASE(JSOP_MOD)

          BEGIN_CASE(JSOP_NOT)
            POP_BOOLEAN(cx, rval, cond);
            PUSH_OPND(BOOLEAN_TO_JSVAL(!cond));
          END_CASE(JSOP_NOT)

          BEGIN_CASE(JSOP_BITNOT)
            FETCH_INT(cx, -1, i);
            i = ~i;
            STORE_INT(cx, -1, i);
          END_CASE(JSOP_BITNOT)

          BEGIN_CASE(JSOP_NEG)
            




            rval = FETCH_OPND(-1);
            if (JSVAL_IS_INT(rval) &&
                rval != INT_TO_JSVAL(JSVAL_INT_MIN) &&
                (i = JSVAL_TO_INT(rval)) != 0) {
                JS_STATIC_ASSERT(!INT_FITS_IN_JSVAL(-JSVAL_INT_MIN));
                i = -i;
                JS_ASSERT(INT_FITS_IN_JSVAL(i));
                regs.sp[-1] = INT_TO_JSVAL(i);
            } else {
                if (JSVAL_IS_DOUBLE(rval)) {
                    d = *JSVAL_TO_DOUBLE(rval);
                } else {
                    d = js_ValueToNumber(cx, &regs.sp[-1]);
                    if (JSVAL_IS_NULL(regs.sp[-1]))
                        goto error;
                    JS_ASSERT(JSVAL_IS_NUMBER(regs.sp[-1]) ||
                              regs.sp[-1] == JSVAL_TRUE);
                }
#ifdef HPUX
                




                JSDOUBLE_HI32(d) ^= JSDOUBLE_HI32_SIGNBIT;
#else
                d = -d;
#endif
                if (!js_NewNumberInRootedValue(cx, d, &regs.sp[-1]))
                    goto error;
            }
          END_CASE(JSOP_NEG)

          BEGIN_CASE(JSOP_POS)
            rval = FETCH_OPND(-1);
            if (!JSVAL_IS_NUMBER(rval)) {
                d = js_ValueToNumber(cx, &regs.sp[-1]);
                rval = regs.sp[-1];
                if (JSVAL_IS_NULL(rval))
                    goto error;
                if (rval == JSVAL_TRUE) {
                    if (!js_NewNumberInRootedValue(cx, d, &regs.sp[-1]))
                        goto error;
                } else {
                    JS_ASSERT(JSVAL_IS_NUMBER(rval));
                }
            }
          END_CASE(JSOP_POS)

          BEGIN_CASE(JSOP_DELNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
                goto error;

            
            PUSH_OPND(JSVAL_TRUE);
            if (prop) {
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                if (!OBJ_DELETE_PROPERTY(cx, obj, id, &regs.sp[-1]))
                    goto error;
            }
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            PROPERTY_OP(-1, OBJ_DELETE_PROPERTY(cx, obj, id, &rval));
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DELPROP)

          BEGIN_CASE(JSOP_DELELEM)
            ELEMENT_OP(-1, OBJ_DELETE_PROPERTY(cx, obj, id, &rval));
            regs.sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DELELEM)

          BEGIN_CASE(JSOP_TYPEOFEXPR)
          BEGIN_CASE(JSOP_TYPEOF)
            rval = FETCH_OPND(-1);
            type = JS_TypeOfValue(cx, rval);
            atom = rt->atomState.typeAtoms[type];
            STORE_OPND(-1, ATOM_KEY(atom));
          END_CASE(JSOP_TYPEOF)

          BEGIN_CASE(JSOP_VOID)
            STORE_OPND(-1, JSVAL_VOID);
          END_CASE(JSOP_VOID)

          BEGIN_CASE(JSOP_INCELEM)
          BEGIN_CASE(JSOP_DECELEM)
          BEGIN_CASE(JSOP_ELEMINC)
          BEGIN_CASE(JSOP_ELEMDEC)
            



            id = 0;
            i = -2;
            goto fetch_incop_obj;

          BEGIN_CASE(JSOP_INCPROP)
          BEGIN_CASE(JSOP_DECPROP)
          BEGIN_CASE(JSOP_PROPINC)
          BEGIN_CASE(JSOP_PROPDEC)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            i = -1;

          fetch_incop_obj:
            FETCH_OBJECT(cx, i, lval, obj);
            if (id == 0)
                FETCH_ELEMENT_ID(obj, -1, id);
            goto do_incop;

          BEGIN_CASE(JSOP_INCNAME)
          BEGIN_CASE(JSOP_DECNAME)
          BEGIN_CASE(JSOP_NAMEINC)
          BEGIN_CASE(JSOP_NAMEDEC)
          {
            JSPropCacheEntry *entry;

            obj = fp->scopeChain;
            if (JS_LIKELY(OBJ_IS_NATIVE(obj))) {
                PROPERTY_CACHE_TEST(cx, regs.pc, obj, obj2, entry, atom);
                if (!atom) {
                    ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
                    if (obj == obj2 && PCVAL_IS_SLOT(entry->vword)) {
                        slot = PCVAL_TO_SLOT(entry->vword);
                        JS_ASSERT(slot < obj->map->freeslot);
                        rval = LOCKED_OBJ_GET_SLOT(obj, slot);
                        if (JS_LIKELY(CAN_DO_FAST_INC_DEC(rval))) {
                            rtmp = rval;
                            rval += (js_CodeSpec[op].format & JOF_INC) ? 2 : -2;
                            if (!(js_CodeSpec[op].format & JOF_POST))
                                rtmp = rval;
                            LOCKED_OBJ_SET_SLOT(obj, slot, rval);
                            JS_UNLOCK_OBJ(cx, obj);
                            PUSH_OPND(rtmp);
                            len = JSOP_INCNAME_LENGTH;
                            DO_NEXT_OP(len);
                        }
                    }
                    JS_UNLOCK_OBJ(cx, obj2);
                    LOAD_ATOM(0);
                }
            } else {
                entry = NULL;
                LOAD_ATOM(0);
            }
            id = ATOM_TO_JSID(atom);
            if (js_FindPropertyHelper(cx, id, &obj, &obj2, &prop, &entry) < 0)
                goto error;
            if (!prop)
                goto atom_not_defined;
            OBJ_DROP_PROPERTY(cx, obj2, prop);
          }

          do_incop:
          {
            const JSCodeSpec *cs;
            jsval v;

            



            PUSH_OPND(JSVAL_NULL);
            if (!OBJ_GET_PROPERTY(cx, obj, id, &regs.sp[-1]))
                goto error;

            cs = &js_CodeSpec[op];
            JS_ASSERT(cs->ndefs == 1);
            JS_ASSERT((cs->format & JOF_TMPSLOT_MASK) == JOF_TMPSLOT2);
            v = regs.sp[-1];
            if (JS_LIKELY(CAN_DO_FAST_INC_DEC(v))) {
                jsval incr;

                incr = (cs->format & JOF_INC) ? 2 : -2;
                if (cs->format & JOF_POST) {
                    regs.sp[-1] = v + incr;
                } else {
                    v += incr;
                    regs.sp[-1] = v;
                }
                fp->flags |= JSFRAME_ASSIGNING;
                ok = OBJ_SET_PROPERTY(cx, obj, id, &regs.sp[-1]);
                fp->flags &= ~JSFRAME_ASSIGNING;
                if (!ok)
                    goto error;

                



                regs.sp[-1] = v;
            } else {
                
                PUSH_OPND(JSVAL_NULL);
                if (!js_DoIncDec(cx, cs, &regs.sp[-2], &regs.sp[-1]))
                    goto error;
                fp->flags |= JSFRAME_ASSIGNING;
                ok = OBJ_SET_PROPERTY(cx, obj, id, &regs.sp[-1]);
                fp->flags &= ~JSFRAME_ASSIGNING;
                if (!ok)
                    goto error;
                regs.sp--;
            }

            if (cs->nuses == 0) {
                
            } else {
                rtmp = regs.sp[-1];
                regs.sp -= cs->nuses;
                regs.sp[-1] = rtmp;
            }
            len = cs->length;
            DO_NEXT_OP(len);
          }

          {
            jsval incr, incr2;

            
          BEGIN_CASE(JSOP_DECARG)
            incr = -2; incr2 = -2; goto do_arg_incop;
          BEGIN_CASE(JSOP_ARGDEC)
            incr = -2; incr2 =  0; goto do_arg_incop;
          BEGIN_CASE(JSOP_INCARG)
            incr =  2; incr2 =  2; goto do_arg_incop;
          BEGIN_CASE(JSOP_ARGINC)
            incr =  2; incr2 =  0;

          do_arg_incop:
            slot = GET_ARGNO(regs.pc);
            JS_ASSERT(slot < fp->fun->nargs);
            METER_SLOT_OP(op, slot);
            vp = fp->argv + slot;
            goto do_int_fast_incop;

          BEGIN_CASE(JSOP_DECLOCAL)
            incr = -2; incr2 = -2; goto do_local_incop;
          BEGIN_CASE(JSOP_LOCALDEC)
            incr = -2; incr2 =  0; goto do_local_incop;
          BEGIN_CASE(JSOP_INCLOCAL)
            incr =  2; incr2 =  2; goto do_local_incop;
          BEGIN_CASE(JSOP_LOCALINC)
            incr =  2; incr2 =  0;

          




          do_local_incop:
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < fp->script->nslots);
            vp = fp->slots + slot;
            METER_SLOT_OP(op, slot);
            vp = fp->slots + slot;

          do_int_fast_incop:
            rval = *vp;
            if (JS_LIKELY(CAN_DO_FAST_INC_DEC(rval))) {
                *vp = rval + incr;
                JS_ASSERT(JSOP_INCARG_LENGTH == js_CodeSpec[op].length);
                SKIP_POP_AFTER_SET(JSOP_INCARG_LENGTH, 0);
                PUSH_OPND(rval + incr2);
            } else {
                PUSH_OPND(rval);
                if (!js_DoIncDec(cx, &js_CodeSpec[op], &regs.sp[-1], vp))
                    goto error;
            }
            len = JSOP_INCARG_LENGTH;
            JS_ASSERT(len == js_CodeSpec[op].length);
            DO_NEXT_OP(len);
          }


#define FAST_GLOBAL_INCREMENT_OP(SLOWOP,INCR,INCR2)                           \
    op2 = SLOWOP;                                                             \
    incr = INCR;                                                              \
    incr2 = INCR2;                                                            \
    goto do_global_incop

          {
            jsval incr, incr2;

          BEGIN_CASE(JSOP_DECGVAR)
            FAST_GLOBAL_INCREMENT_OP(JSOP_DECNAME, -2, -2);
          BEGIN_CASE(JSOP_GVARDEC)
            FAST_GLOBAL_INCREMENT_OP(JSOP_NAMEDEC, -2,  0);
          BEGIN_CASE(JSOP_INCGVAR)
              FAST_GLOBAL_INCREMENT_OP(JSOP_INCNAME,  2,  2);
          BEGIN_CASE(JSOP_GVARINC)
            FAST_GLOBAL_INCREMENT_OP(JSOP_NAMEINC,  2,  0);

#undef FAST_GLOBAL_INCREMENT_OP

          do_global_incop:
            JS_ASSERT((js_CodeSpec[op].format & JOF_TMPSLOT_MASK) ==
                      JOF_TMPSLOT2);
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < GlobalVarCount(fp));
            METER_SLOT_OP(op, slot);
            lval = fp->slots[slot];
            if (JSVAL_IS_NULL(lval)) {
                op = op2;
                DO_OP();
            }
            slot = JSVAL_TO_INT(lval);
            rval = OBJ_GET_SLOT(cx, fp->varobj, slot);
            if (JS_LIKELY(CAN_DO_FAST_INC_DEC(rval))) {
                PUSH_OPND(rval + incr2);
                rval += incr;
            } else {
                PUSH_OPND(rval);
                PUSH_OPND(JSVAL_NULL);  
                if (!js_DoIncDec(cx, &js_CodeSpec[op], &regs.sp[-2], &regs.sp[-1]))
                    goto error;
                rval = regs.sp[-1];
                --regs.sp;
            }
            OBJ_SET_SLOT(cx, fp->varobj, slot, rval);
            len = JSOP_INCGVAR_LENGTH;  
            JS_ASSERT(len == js_CodeSpec[op].length);
            DO_NEXT_OP(len);
          }

#define COMPUTE_THIS(cx, fp, obj)                                             \
    JS_BEGIN_MACRO                                                            \
        if (fp->flags & JSFRAME_COMPUTED_THIS) {                              \
            obj = fp->thisp;                                                  \
        } else {                                                              \
            obj = js_ComputeThis(cx, JS_TRUE, fp->argv);                      \
            if (!obj)                                                         \
                goto error;                                                   \
            fp->thisp = obj;                                                  \
            fp->flags |= JSFRAME_COMPUTED_THIS;                               \
        }                                                                     \
    JS_END_MACRO

          BEGIN_CASE(JSOP_THIS)
            COMPUTE_THIS(cx, fp, obj);
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_THIS)

          BEGIN_CASE(JSOP_GETTHISPROP)
            i = 0;
            COMPUTE_THIS(cx, fp, obj);
            PUSH(JSVAL_NULL);
            goto do_getprop_with_obj;

#undef COMPUTE_THIS

          BEGIN_CASE(JSOP_GETARGPROP)
            i = ARGNO_LEN;
            slot = GET_ARGNO(regs.pc);
            JS_ASSERT(slot < fp->fun->nargs);
            PUSH_OPND(fp->argv[slot]);
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETLOCALPROP)
            i = SLOTNO_LEN;
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < script->nslots);
            PUSH_OPND(fp->slots[slot]);
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETPROP)
          BEGIN_CASE(JSOP_GETXPROP)
            i = 0;

          do_getprop_body:
            lval = FETCH_OPND(-1);

          do_getprop_with_lval:
            VALUE_TO_OBJECT(cx, -1, lval, obj);

          do_getprop_with_obj:
            do {
                JSObject *aobj;
                JSPropCacheEntry *entry;

                aobj = OBJ_IS_DENSE_ARRAY(cx, obj) ? OBJ_GET_PROTO(cx, obj) : obj;
                if (JS_LIKELY(aobj->map->ops->getProperty == js_GetProperty)) {
                    PROPERTY_CACHE_TEST(cx, regs.pc, aobj, obj2, entry, atom);
                    if (!atom) {
                        ASSERT_VALID_PROPERTY_CACHE_HIT(i, aobj, obj2, entry);
                        if (PCVAL_IS_OBJECT(entry->vword)) {
                            rval = PCVAL_OBJECT_TO_JSVAL(entry->vword);
                        } else if (PCVAL_IS_SLOT(entry->vword)) {
                            slot = PCVAL_TO_SLOT(entry->vword);
                            JS_ASSERT(slot < obj2->map->freeslot);
                            rval = LOCKED_OBJ_GET_SLOT(obj2, slot);
                        } else {
                            JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
                            sprop = PCVAL_TO_SPROP(entry->vword);
                            NATIVE_GET(cx, obj, obj2, sprop, &rval);
                        }
                        JS_UNLOCK_OBJ(cx, obj2);
                        break;
                    }
                } else {
                    entry = NULL;
                    if (i < 0)
                        atom = rt->atomState.lengthAtom;
                    else
                        LOAD_ATOM(i);
                }
                id = ATOM_TO_JSID(atom);
                if (entry
                    ? !js_GetPropertyHelper(cx, aobj, id, &rval, &entry)
                    : !OBJ_GET_PROPERTY(cx, obj, id, &rval)) {
                    goto error;
                }
            } while (0);

            STORE_OPND(-1, rval);
            JS_ASSERT(JSOP_GETPROP_LENGTH + i == js_CodeSpec[op].length);
            len = JSOP_GETPROP_LENGTH + i;
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_LENGTH)
            lval = FETCH_OPND(-1);
            if (JSVAL_IS_STRING(lval)) {
                str = JSVAL_TO_STRING(lval);
                regs.sp[-1] = INT_TO_JSVAL(JSSTRING_LENGTH(str));
            } else if (!JSVAL_IS_PRIMITIVE(lval) &&
                       (obj = JSVAL_TO_OBJECT(lval), OBJ_IS_ARRAY(cx, obj))) {
                jsuint length;

                




                length = obj->fslots[JSSLOT_ARRAY_LENGTH];
                if (length <= JSVAL_INT_MAX) {
                    regs.sp[-1] = INT_TO_JSVAL(length);
                } else if (!js_NewDoubleInRootedValue(cx, (jsdouble) length,
                                                      &regs.sp[-1])) {
                    goto error;
                }
            } else {
                i = -2;
                goto do_getprop_with_lval;
            }
          END_CASE(JSOP_LENGTH)

          BEGIN_CASE(JSOP_CALLPROP)
          {
            JSObject *aobj;
            JSPropCacheEntry *entry;

            lval = FETCH_OPND(-1);
            if (!JSVAL_IS_PRIMITIVE(lval)) {
                obj = JSVAL_TO_OBJECT(lval);
            } else {
                if (JSVAL_IS_STRING(lval)) {
                    i = JSProto_String;
                } else if (JSVAL_IS_NUMBER(lval)) {
                    i = JSProto_Number;
                } else if (JSVAL_IS_BOOLEAN(lval)) {
                    i = JSProto_Boolean;
                } else {
                    JS_ASSERT(JSVAL_IS_NULL(lval) || JSVAL_IS_VOID(lval));
                    js_ReportIsNullOrUndefined(cx, -1, lval, NULL);
                    goto error;
                }

                if (!js_GetClassPrototype(cx, NULL, INT_TO_JSID(i), &obj))
                    goto error;
            }

            aobj = OBJ_IS_DENSE_ARRAY(cx, obj) ? OBJ_GET_PROTO(cx, obj) : obj;
            if (JS_LIKELY(aobj->map->ops->getProperty == js_GetProperty)) {
                PROPERTY_CACHE_TEST(cx, regs.pc, aobj, obj2, entry, atom);
                if (!atom) {
                    ASSERT_VALID_PROPERTY_CACHE_HIT(0, aobj, obj2, entry);
                    if (PCVAL_IS_OBJECT(entry->vword)) {
                        rval = PCVAL_OBJECT_TO_JSVAL(entry->vword);
                    } else if (PCVAL_IS_SLOT(entry->vword)) {
                        slot = PCVAL_TO_SLOT(entry->vword);
                        JS_ASSERT(slot < obj2->map->freeslot);
                        rval = LOCKED_OBJ_GET_SLOT(obj2, slot);
                    } else {
                        JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
                        sprop = PCVAL_TO_SPROP(entry->vword);
                        NATIVE_GET(cx, obj, obj2, sprop, &rval);
                    }
                    JS_UNLOCK_OBJ(cx, obj2);
                    STORE_OPND(-1, rval);
                    PUSH_OPND(lval);
                    goto end_callprop;
                }
            } else {
                entry = NULL;
                LOAD_ATOM(0);
            }

            



            id = ATOM_TO_JSID(atom);
            PUSH(JSVAL_NULL);
            if (!JSVAL_IS_PRIMITIVE(lval)) {
#if JS_HAS_XML_SUPPORT
                
                if (OBJECT_IS_XML(cx, obj)) {
                    JSXMLObjectOps *ops;

                    ops = (JSXMLObjectOps *) obj->map->ops;
                    obj = ops->getMethod(cx, obj, id, &rval);
                    if (!obj)
                        goto error;
                } else
#endif
                if (entry
                    ? !js_GetPropertyHelper(cx, aobj, id, &rval, &entry)
                    : !OBJ_GET_PROPERTY(cx, obj, id, &rval)) {
                    goto error;
                }
                STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
                STORE_OPND(-2, rval);
            } else {
                JS_ASSERT(obj->map->ops->getProperty == js_GetProperty);
                if (!js_GetPropertyHelper(cx, obj, id, &rval, &entry))
                    goto error;
                STORE_OPND(-1, lval);
                STORE_OPND(-2, rval);
            }

          end_callprop:
            
            if (JSVAL_IS_PRIMITIVE(lval)) {
                
                if (!VALUE_IS_FUNCTION(cx, rval) ||
                    (obj = JSVAL_TO_OBJECT(rval),
                     fun = GET_FUNCTION_PRIVATE(cx, obj),
                     !PRIMITIVE_THIS_TEST(fun, lval))) {
                    if (!js_PrimitiveToObject(cx, &regs.sp[-1]))
                        goto error;
                }
            }
#if JS_HAS_NO_SUCH_METHOD
            if (JS_UNLIKELY(JSVAL_IS_VOID(rval))) {
                LOAD_ATOM(0);
                regs.sp[-2] = ATOM_KEY(atom);
                if (!js_OnUnknownMethod(cx, regs.sp - 2))
                    goto error;
            }
#endif
          }
          END_CASE(JSOP_CALLPROP)

          BEGIN_CASE(JSOP_SETNAME)
          BEGIN_CASE(JSOP_SETPROP)
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(lval) || op == JSOP_SETPROP);
            VALUE_TO_OBJECT(cx, -2, lval, obj);

            do {
                JSPropCacheEntry *entry;

                entry = NULL;
                atom = NULL;
                if (JS_LIKELY(obj->map->ops->setProperty == js_SetProperty)) {
                    JSPropertyCache *cache = &JS_PROPERTY_CACHE(cx);
                    uint32 kshape = OBJ_SHAPE(obj);

                    


















                    entry = &cache->table[PROPERTY_CACHE_HASH_PC(regs.pc, kshape)];
                    PCMETER(cache->tests++);
                    PCMETER(cache->settests++);
                    if (entry->kpc == regs.pc && entry->kshape == kshape) {
                        JSScope *scope;

                        JS_LOCK_OBJ(cx, obj);
                        scope = OBJ_SCOPE(obj);
                        if (scope->shape == kshape) {
                            JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
                            sprop = PCVAL_TO_SPROP(entry->vword);
                            JS_ASSERT(!(sprop->attrs & JSPROP_READONLY));
                            JS_ASSERT(!SCOPE_IS_SEALED(OBJ_SCOPE(obj)));

                            if (scope->object == obj) {
                                




                                if (sprop == scope->lastProp ||
                                    SCOPE_HAS_PROPERTY(scope, sprop)) {
                                    PCMETER(cache->pchits++);
                                    PCMETER(cache->setpchits++);
                                    NATIVE_SET(cx, obj, sprop, &rval);
                                    JS_UNLOCK_SCOPE(cx, scope);
                                    TRACE_2(SetPropHit, entry, sprop);
                                    break;
                                }
                            } else {
                                scope = js_GetMutableScope(cx, obj);
                                if (!scope) {
                                    JS_UNLOCK_OBJ(cx, obj);
                                    goto error;
                                }
                            }

                            if (sprop->parent == scope->lastProp &&
                                !SCOPE_HAD_MIDDLE_DELETE(scope) &&
                                SPROP_HAS_STUB_SETTER(sprop) &&
                                (slot = sprop->slot) == scope->map.freeslot) {
                                










                                JS_ASSERT(!(LOCKED_OBJ_GET_CLASS(obj)->flags &
                                            JSCLASS_SHARE_ALL_PROPERTIES));

                                PCMETER(cache->pchits++);
                                PCMETER(cache->addpchits++);

                                




                                if (slot < STOBJ_NSLOTS(obj) &&
                                    !OBJ_GET_CLASS(cx, obj)->reserveSlots) {
                                    ++scope->map.freeslot;
                                } else {
                                    if (!js_AllocSlot(cx, obj, &slot)) {
                                        JS_UNLOCK_SCOPE(cx, scope);
                                        goto error;
                                    }
                                }

                                










                                if (slot != sprop->slot || scope->table) {
                                    JSScopeProperty *sprop2 =
                                        js_AddScopeProperty(cx, scope,
                                                            sprop->id,
                                                            sprop->getter,
                                                            sprop->setter,
                                                            slot,
                                                            sprop->attrs,
                                                            sprop->flags,
                                                            sprop->shortid);
                                    if (!sprop2) {
                                        js_FreeSlot(cx, obj, slot);
                                        JS_UNLOCK_SCOPE(cx, scope);
                                        goto error;
                                    }
                                    if (sprop2 != sprop) {
                                        PCMETER(cache->slotchanges++);
                                        JS_ASSERT(slot != sprop->slot &&
                                                  slot == sprop2->slot &&
                                                  sprop2->id == sprop->id);
                                        entry->vword = SPROP_TO_PCVAL(sprop2);
                                    }
                                    sprop = sprop2;
                                } else {
                                    SCOPE_EXTEND_SHAPE(cx, scope, sprop);
                                    ++scope->entryCount;
                                    scope->lastProp = sprop;
                                }

                                GC_WRITE_BARRIER(cx, scope,
                                                 LOCKED_OBJ_GET_SLOT(obj, slot),
                                                 rval);
                                LOCKED_OBJ_SET_SLOT(obj, slot, rval);
                                JS_UNLOCK_SCOPE(cx, scope);
                                TRACE_2(SetPropHit, entry, sprop);
                                break;
                            }

                            PCMETER(cache->setpcmisses++);
                            atom = NULL;
                        }

                        JS_UNLOCK_OBJ(cx, obj);
                    }

                    atom = js_FullTestPropertyCache(cx, regs.pc, &obj, &obj2,
                                                    &entry);
                    if (atom) {
                        PCMETER(cache->misses++);
                        PCMETER(cache->setmisses++);
                    } else {
                        ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
                        sprop = NULL;
                        if (obj == obj2) {
                            JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
                            sprop = PCVAL_TO_SPROP(entry->vword);
                            JS_ASSERT(!(sprop->attrs & JSPROP_READONLY));
                            JS_ASSERT(!SCOPE_IS_SEALED(OBJ_SCOPE(obj2)));
                            NATIVE_SET(cx, obj, sprop, &rval);
                        }
                        JS_UNLOCK_OBJ(cx, obj2);
                        if (sprop) {
                            TRACE_2(SetPropHit, entry, sprop);
                            break;
                        }
                    }
                }

                if (!atom)
                    LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                if (entry) {
                    if (!js_SetPropertyHelper(cx, obj, id, &rval, &entry))
                        goto error;
#ifdef JS_TRACER
                    if (entry)
                        TRACE_1(SetPropMiss, entry);
#endif
                } else {
                    if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                        goto error;
                }
#ifdef JS_TRACER
                if (!entry && TRACE_RECORDER(cx))
                    js_AbortRecording(cx, "SetPropUncached");
#endif
            } while (0);
          END_SET_CASE_STORE_RVAL(JSOP_SETPROP, 2);

          BEGIN_CASE(JSOP_GETELEM)
            
            lval = FETCH_OPND(-2);
            rval = FETCH_OPND(-1);
            if (JSVAL_IS_STRING(lval) && JSVAL_IS_INT(rval)) {
                str = JSVAL_TO_STRING(lval);
                i = JSVAL_TO_INT(rval);
                if ((size_t)i < JSSTRING_LENGTH(str)) {
                    str = js_GetUnitString(cx, str, (size_t)i);
                    if (!str)
                        goto error;
                    rval = STRING_TO_JSVAL(str);
                    goto end_getelem;
                }
            }

            VALUE_TO_OBJECT(cx, -2, lval, obj);
            if (JSVAL_IS_INT(rval)) {
                if (OBJ_IS_DENSE_ARRAY(cx, obj)) {
                    jsuint length;

                    length = ARRAY_DENSE_LENGTH(obj);
                    i = JSVAL_TO_INT(rval);
                    if ((jsuint)i < length &&
                        i < obj->fslots[JSSLOT_ARRAY_LENGTH]) {
                        rval = obj->dslots[i];
                        if (rval != JSVAL_HOLE)
                            goto end_getelem;

                        
                        rval = FETCH_OPND(-1);
                    }
                }
                id = INT_JSVAL_TO_JSID(rval);
            } else {
                if (!js_InternNonIntElementId(cx, obj, rval, &id))
                    goto error;
            }

            if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                goto error;
          end_getelem:
            regs.sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_CALLELEM)
            



            ELEMENT_OP(-1, OBJ_GET_PROPERTY(cx, obj, id, &rval));
#if JS_HAS_NO_SUCH_METHOD
            if (JS_UNLIKELY(JSVAL_IS_VOID(rval))) {
                regs.sp[-2] = regs.sp[-1];
                regs.sp[-1] = OBJECT_TO_JSVAL(obj);
                if (!js_OnUnknownMethod(cx, regs.sp - 2))
                    goto error;
            } else
#endif
            {
                STORE_OPND(-2, rval);
                STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
            }
          END_CASE(JSOP_CALLELEM)

          BEGIN_CASE(JSOP_SETELEM)
            rval = FETCH_OPND(-1);
            FETCH_OBJECT(cx, -3, lval, obj);
            FETCH_ELEMENT_ID(obj, -2, id);
            do {
                if (OBJ_IS_DENSE_ARRAY(cx, obj) && JSID_IS_INT(id)) {
                    jsuint length;

                    length = ARRAY_DENSE_LENGTH(obj);
                    i = JSID_TO_INT(id);
                    if ((jsuint)i < length) {
                        if (obj->dslots[i] == JSVAL_HOLE) {
                            if (rt->anyArrayProtoHasElement)
                                break;
                            if (i >= obj->fslots[JSSLOT_ARRAY_LENGTH])
                                obj->fslots[JSSLOT_ARRAY_LENGTH] = i + 1;
                            obj->fslots[JSSLOT_ARRAY_COUNT]++;
                        }
                        obj->dslots[i] = rval;
                        goto end_setelem;
                    }
                }
            } while (0);
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
        end_setelem:
          END_SET_CASE_STORE_RVAL(JSOP_SETELEM, 3)

          BEGIN_CASE(JSOP_ENUMELEM)
            
            rval = FETCH_OPND(-3);
            FETCH_OBJECT(cx, -2, lval, obj);
            FETCH_ELEMENT_ID(obj, -1, id);
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
            regs.sp -= 3;
          END_CASE(JSOP_ENUMELEM)

          BEGIN_CASE(JSOP_NEW)
            
            argc = GET_ARGC(regs.pc);
            vp = regs.sp - (2 + argc);
            JS_ASSERT(vp >= StackBase(fp));

            




            lval = *vp;
            if (VALUE_IS_FUNCTION(cx, lval)) {
                obj = JSVAL_TO_OBJECT(lval);
                fun = GET_FUNCTION_PRIVATE(cx, obj);
                if (FUN_INTERPRETED(fun)) {
                    
                    if (!OBJ_GET_PROPERTY(cx, obj,
                                          ATOM_TO_JSID(cx->runtime->atomState
                                                       .classPrototypeAtom),
                                          &vp[1])) {
                        goto error;
                    }
                    rval = vp[1];
                    obj2 = js_NewObject(cx, &js_ObjectClass,
                                        JSVAL_IS_OBJECT(rval)
                                        ? JSVAL_TO_OBJECT(rval)
                                        : NULL,
                                        OBJ_GET_PARENT(cx, obj),
                                        0);
                    if (!obj2)
                        goto error;
                    vp[1] = OBJECT_TO_JSVAL(obj2);
                    flags = JSFRAME_CONSTRUCTING;
                    goto inline_call;
                }
            }

            if (!js_InvokeConstructor(cx, argc, JS_FALSE, vp))
                goto error;
            regs.sp = vp + 1;
            CHECK_INTERRUPT_HANDLER();
          END_CASE(JSOP_NEW)
          
          BEGIN_CASE(JSOP_CALL)
          BEGIN_CASE(JSOP_EVAL)
          BEGIN_CASE(JSOP_APPLY)
            argc = GET_ARGC(regs.pc);
            vp = regs.sp - (argc + 2);
            
            lval = *vp;
            if (VALUE_IS_FUNCTION(cx, lval)) {
                obj = JSVAL_TO_OBJECT(lval);
                fun = GET_FUNCTION_PRIVATE(cx, obj);

                
                flags = 0;
                if (FUN_INTERPRETED(fun))
              inline_call:
                {
                    uintN nframeslots, nvars, missing;
                    JSArena *a;
                    jsuword nbytes;
                    void *newmark;
                    jsval *newsp;
                    JSInlineFrame *newifp;
                    JSInterpreterHook hook;

                    
                    if (inlineCallCount == MAX_INLINE_CALL_COUNT) {
                        js_ReportOverRecursed(cx);
                        goto error;
                    }

                    
                    nframeslots = JS_HOWMANY(sizeof(JSInlineFrame),
                                             sizeof(jsval));
                    script = fun->u.i.script;
                    atoms = script->atomMap.vector;
                    nbytes = (nframeslots + script->nslots) * sizeof(jsval);

                    
                    a = cx->stackPool.current;
                    newmark = (void *) a->avail;
                    if (fun->nargs <= argc) {
                        missing = 0;
                    } else {
                        newsp = vp + 2 + fun->nargs;
                        JS_ASSERT(newsp > regs.sp);
                        if ((jsuword) newsp <= a->limit) {
                            if ((jsuword) newsp > a->avail)
                                a->avail = (jsuword) newsp;
                            jsval *argsp = newsp;
                            do {
                                *--argsp = JSVAL_VOID;
                            } while (argsp != regs.sp);
                            missing = 0;
                        } else {
                            missing = fun->nargs - argc;
                            nbytes += (2 + fun->nargs) * sizeof(jsval);
                        }
                    }

                    
                    if (a->avail + nbytes <= a->limit) {
                        newsp = (jsval *) a->avail;
                        a->avail += nbytes;
                        JS_ASSERT(missing == 0);
                    } else {
                        JS_ARENA_ALLOCATE_CAST(newsp, jsval *, &cx->stackPool,
                                               nbytes);
                        if (!newsp) {
                            js_ReportOutOfScriptQuota(cx);
                            goto bad_inline_call;
                        }

                        



                        if (missing) {
                            memcpy(newsp, vp, (2 + argc) * sizeof(jsval));
                            vp = newsp;
                            newsp = vp + 2 + argc;
                            do {
                                *newsp++ = JSVAL_VOID;
                            } while (--missing != 0);
                        }
                    }

                    
                    newifp = (JSInlineFrame *) newsp;
                    newsp += nframeslots;
                    newifp->frame.callobj = NULL;
                    newifp->frame.argsobj = NULL;
                    newifp->frame.varobj = NULL;
                    newifp->frame.script = script;
                    newifp->frame.callee = obj;
                    newifp->frame.fun = fun;
                    newifp->frame.argc = argc;
                    newifp->frame.argv = vp + 2;
                    newifp->frame.rval = JSVAL_VOID;
                    newifp->frame.down = fp;
                    newifp->frame.annotation = NULL;
                    newifp->frame.scopeChain = parent = OBJ_GET_PARENT(cx, obj);
                    newifp->frame.sharpDepth = 0;
                    newifp->frame.sharpArray = NULL;
                    newifp->frame.flags = flags;
                    newifp->frame.dormantNext = NULL;
                    newifp->frame.xmlNamespace = NULL;
                    newifp->frame.blockChain = NULL;
                    if (script->staticDepth < JS_DISPLAY_SIZE) {
                        JSStackFrame **disp = &cx->display[script->staticDepth];
                        newifp->frame.displaySave = *disp;
                        *disp = &newifp->frame;
                    }
#ifdef DEBUG
                    newifp->frame.pcDisabledSave =
                        JS_PROPERTY_CACHE(cx).disabled;
#endif
                    newifp->mark = newmark;

                    
                    JS_ASSERT(!JSFUN_BOUND_METHOD_TEST(fun->flags));
                    JS_ASSERT(JSVAL_IS_OBJECT(vp[1]));
                    newifp->frame.thisp = (JSObject *)vp[1];

                    newifp->frame.regs = NULL;
                    newifp->frame.imacpc = NULL;
                    newifp->frame.slots = newsp;

                    
                    nvars = fun->u.i.nvars;
                    while (nvars--)
                        *newsp++ = JSVAL_VOID;

                    
                    hook = cx->debugHooks->callHook;
                    if (hook) {
                        newifp->hookData = hook(cx, &newifp->frame, JS_TRUE, 0,
                                                cx->debugHooks->callHookData);
                        CHECK_INTERRUPT_HANDLER();
                    } else {
                        newifp->hookData = NULL;
                    }

                    
                    if (JSFUN_HEAVYWEIGHT_TEST(fun->flags) &&
                        !js_GetCallObject(cx, &newifp->frame, parent)) {
                        goto bad_inline_call;
                    }

                    
                    newifp->callerVersion = (JSVersion) cx->version;
                    if (JS_LIKELY(cx->version == currentVersion)) {
                        currentVersion = (JSVersion) script->version;
                        if (currentVersion != cx->version)
                            js_SetVersion(cx, currentVersion);
                    }

                    
                    newifp->callerRegs = regs;
                    fp->regs = &newifp->callerRegs;
                    regs.sp = newsp;
                    regs.pc = script->code;
                    newifp->frame.regs = &regs;
                    cx->fp = fp = &newifp->frame;

                    TRACE_0(EnterFrame);

                    inlineCallCount++;
                    JS_RUNTIME_METER(rt, inlineCalls);

#ifdef INCLUDE_MOZILLA_DTRACE
                    
                    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
                        jsdtrace_function_entry(cx, fp, fun);
                    if (JAVASCRIPT_FUNCTION_INFO_ENABLED())
                        jsdtrace_function_info(cx, fp, fp->down, fun);
                    if (JAVASCRIPT_FUNCTION_ARGS_ENABLED())
                        jsdtrace_function_args(cx, fp, fun);
#endif

                    
                    op = (JSOp) *regs.pc;
                    DO_OP();

                  bad_inline_call:
                    JS_ASSERT(fp->regs == &regs);
                    script = fp->script;
                    atoms = script->atomMap.vector;
                    js_FreeRawStack(cx, newmark);
                    goto error;
                }

#ifdef INCLUDE_MOZILLA_DTRACE
                
                if (VALUE_IS_FUNCTION(cx, lval)) {
                    if (JAVASCRIPT_FUNCTION_ENTRY_ENABLED())
                        jsdtrace_function_entry(cx, fp, fun);
                    if (JAVASCRIPT_FUNCTION_INFO_ENABLED())
                        jsdtrace_function_info(cx, fp, fp, fun);
                    if (JAVASCRIPT_FUNCTION_ARGS_ENABLED())
                        jsdtrace_function_args(cx, fp, fun);
                }
#endif

                if (fun->flags & JSFUN_FAST_NATIVE) {
                    JS_ASSERT(fun->u.n.extra == 0);
                    JS_ASSERT(JSVAL_IS_OBJECT(vp[1]) ||
                              PRIMITIVE_THIS_TEST(fun, vp[1]));
                    ok = ((JSFastNative) fun->u.n.native)(cx, argc, vp);
#ifdef INCLUDE_MOZILLA_DTRACE
                    if (VALUE_IS_FUNCTION(cx, lval)) {
                        if (JAVASCRIPT_FUNCTION_RVAL_ENABLED())
                            jsdtrace_function_rval(cx, fp, fun);
                        if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
                            jsdtrace_function_return(cx, fp, fun);
                    }
#endif
                    regs.sp = vp + 1;
                    if (!ok)
                        goto error;
                    TRACE_0(FastNativeCallComplete);
                    goto end_call;
                }
            }

            ok = js_Invoke(cx, argc, vp, 0);
#ifdef INCLUDE_MOZILLA_DTRACE
            
            if (VALUE_IS_FUNCTION(cx, lval)) {
                if (JAVASCRIPT_FUNCTION_RVAL_ENABLED())
                    jsdtrace_function_rval(cx, fp, fun);
                if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
                    jsdtrace_function_return(cx, fp, fun);
            }
#endif
            regs.sp = vp + 1;
            CHECK_INTERRUPT_HANDLER();
            if (!ok)
                goto error;
            JS_RUNTIME_METER(rt, nonInlineCalls);

          end_call:
#if JS_HAS_LVALUE_RETURN
            if (cx->rval2set) {
                













                PUSH_OPND(cx->rval2);
                ELEMENT_OP(-1, OBJ_GET_PROPERTY(cx, obj, id, &rval));

                regs.sp--;
                STORE_OPND(-1, rval);
                cx->rval2set = JS_FALSE;
            }
#endif 
          END_CASE(JSOP_CALL)

#if JS_HAS_LVALUE_RETURN
          BEGIN_CASE(JSOP_SETCALL)
            argc = GET_ARGC(regs.pc);
            vp = regs.sp - argc - 2;
            ok = js_Invoke(cx, argc, vp, 0);
            regs.sp = vp + 1;
            CHECK_INTERRUPT_HANDLER();
            if (!ok)
                goto error;
            if (!cx->rval2set) {
                op2 = (JSOp) regs.pc[JSOP_SETCALL_LENGTH];
                if (op2 != JSOP_DELELEM) {
                    JS_ASSERT(!(js_CodeSpec[op2].format & JOF_DEL));
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_BAD_LEFTSIDE_OF_ASS);
                    goto error;
                }

                




                *vp = JSVAL_TRUE;
                regs.pc += JSOP_SETCALL_LENGTH + JSOP_DELELEM_LENGTH;
                op = (JSOp) *regs.pc;
                DO_OP();
            }
            PUSH_OPND(cx->rval2);
            cx->rval2set = JS_FALSE;
          END_CASE(JSOP_SETCALL)
#endif

          BEGIN_CASE(JSOP_NAME)
          BEGIN_CASE(JSOP_CALLNAME)
          {
            JSPropCacheEntry *entry;

            obj = fp->scopeChain;
            if (JS_LIKELY(OBJ_IS_NATIVE(obj))) {
                PROPERTY_CACHE_TEST(cx, regs.pc, obj, obj2, entry, atom);
                if (!atom) {
                    ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
                    if (PCVAL_IS_OBJECT(entry->vword)) {
                        rval = PCVAL_OBJECT_TO_JSVAL(entry->vword);
                        JS_UNLOCK_OBJ(cx, obj2);
                        goto do_push_rval;
                    }

                    if (PCVAL_IS_SLOT(entry->vword)) {
                        slot = PCVAL_TO_SLOT(entry->vword);
                        JS_ASSERT(slot < obj2->map->freeslot);
                        rval = LOCKED_OBJ_GET_SLOT(obj2, slot);
                        JS_UNLOCK_OBJ(cx, obj2);
                        goto do_push_rval;
                    }

                    JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
                    sprop = PCVAL_TO_SPROP(entry->vword);
                    goto do_native_get;
                }
            } else {
                entry = NULL;
                LOAD_ATOM(0);
            }

            id = ATOM_TO_JSID(atom);
            if (js_FindPropertyHelper(cx, id, &obj, &obj2, &prop, &entry) < 0)
                goto error;
            if (!prop) {
                
                endpc = script->code + script->length;
                op2 = (JSOp) regs.pc[JSOP_NAME_LENGTH];
                if (op2 == JSOP_TYPEOF) {
                    PUSH_OPND(JSVAL_VOID);
                    len = JSOP_NAME_LENGTH;
                    DO_NEXT_OP(len);
                }
                goto atom_not_defined;
            }

            
            if (!OBJ_IS_NATIVE(obj) || !OBJ_IS_NATIVE(obj2)) {
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                    goto error;
                entry = NULL;
            } else {
                sprop = (JSScopeProperty *)prop;
          do_native_get:
                NATIVE_GET(cx, obj, obj2, sprop, &rval);
                OBJ_DROP_PROPERTY(cx, obj2, (JSProperty *) sprop);
            }

          do_push_rval:
            PUSH_OPND(rval);
            if (op == JSOP_CALLNAME)
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
          }
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_UINT16)
            i = (jsint) GET_UINT16(regs.pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_UINT16)

          BEGIN_CASE(JSOP_UINT24)
            i = (jsint) GET_UINT24(regs.pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_UINT24)

          BEGIN_CASE(JSOP_INT8)
            i = GET_INT8(regs.pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_INT8)

          BEGIN_CASE(JSOP_INT32)
            i = GET_INT32(regs.pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_INT32)

          BEGIN_CASE(JSOP_INDEXBASE)
            



            atoms += GET_INDEXBASE(regs.pc);
          END_CASE(JSOP_INDEXBASE)

          BEGIN_CASE(JSOP_INDEXBASE1)
          BEGIN_CASE(JSOP_INDEXBASE2)
          BEGIN_CASE(JSOP_INDEXBASE3)
            atoms += (op - JSOP_INDEXBASE1 + 1) << 16;
          END_CASE(JSOP_INDEXBASE3)

          BEGIN_CASE(JSOP_RESETBASE0)
          BEGIN_CASE(JSOP_RESETBASE)
            atoms = script->atomMap.vector;
          END_CASE(JSOP_RESETBASE)

          BEGIN_CASE(JSOP_DOUBLE)
          BEGIN_CASE(JSOP_STRING)
            LOAD_ATOM(0);
            PUSH_OPND(ATOM_KEY(atom));
          END_CASE(JSOP_DOUBLE)

          BEGIN_CASE(JSOP_OBJECT)
            LOAD_OBJECT(0);
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_OBJECT)

          BEGIN_CASE(JSOP_REGEXP)
          {
            JSObject *funobj;

            























            index = GET_FULL_INDEX(0);
            JS_ASSERT(index < JS_SCRIPT_REGEXPS(script)->length);

            slot = index;
            if (fp->fun) {
                





                funobj = fp->callee;
                slot += JSCLASS_RESERVED_SLOTS(&js_FunctionClass);
                if (script->upvarsOffset != 0)
                    slot += JS_SCRIPT_UPVARS(script)->length;
                if (!JS_GetReservedSlot(cx, funobj, slot, &rval))
                    goto error;
                if (JSVAL_IS_VOID(rval))
                    rval = JSVAL_NULL;
            } else {
                






                JS_ASSERT(slot < script->nfixed);
                slot = script->nfixed - slot - 1;
                rval = fp->slots[slot];
#ifdef __GNUC__
                funobj = NULL;  
#endif
            }

            if (JSVAL_IS_NULL(rval)) {
                
                obj2 = fp->scopeChain;
                while ((parent = OBJ_GET_PARENT(cx, obj2)) != NULL)
                    obj2 = parent;

                























                JS_GET_SCRIPT_REGEXP(script, index, obj);
                if (OBJ_GET_PARENT(cx, obj) != obj2) {
                    obj = js_CloneRegExpObject(cx, obj, obj2);
                    if (!obj)
                        goto error;
                }
                rval = OBJECT_TO_JSVAL(obj);

                
                if (fp->fun) {
                    if (!JS_SetReservedSlot(cx, funobj, slot, rval))
                        goto error;
                } else {
                    fp->slots[slot] = rval;
                }
            }

            PUSH_OPND(rval);
          }
          END_CASE(JSOP_REGEXP)

          BEGIN_CASE(JSOP_ZERO)
            PUSH_OPND(JSVAL_ZERO);
          END_CASE(JSOP_ZERO)

          BEGIN_CASE(JSOP_ONE)
            PUSH_OPND(JSVAL_ONE);
          END_CASE(JSOP_ONE)

          BEGIN_CASE(JSOP_NULL)
          BEGIN_CASE(JSOP_NULLTHIS)
            PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_NULL)

          BEGIN_CASE(JSOP_FALSE)
            PUSH_OPND(JSVAL_FALSE);
          END_CASE(JSOP_FALSE)

          BEGIN_CASE(JSOP_TRUE)
            PUSH_OPND(JSVAL_TRUE);
          END_CASE(JSOP_TRUE)

          BEGIN_CASE(JSOP_TABLESWITCH)
            pc2 = regs.pc;
            len = GET_JUMP_OFFSET(pc2);

            




            rval = POP_OPND();
            if (JSVAL_IS_INT(rval)) {
                i = JSVAL_TO_INT(rval);
            } else if (JSVAL_IS_DOUBLE(rval) && *JSVAL_TO_DOUBLE(rval) == 0) {
                
                i = 0;
            } else {
                DO_NEXT_OP(len);
            }

            pc2 += JUMP_OFFSET_LEN;
            low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            high = GET_JUMP_OFFSET(pc2);

            i -= low;
            if ((jsuint)i < (jsuint)(high - low + 1)) {
                pc2 += JUMP_OFFSET_LEN + JUMP_OFFSET_LEN * i;
                off = (jsint) GET_JUMP_OFFSET(pc2);
                if (off)
                    len = off;
            }
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_TABLESWITCHX)
            pc2 = regs.pc;
            len = GET_JUMPX_OFFSET(pc2);

            




            rval = POP_OPND();
            if (JSVAL_IS_INT(rval)) {
                i = JSVAL_TO_INT(rval);
            } else if (JSVAL_IS_DOUBLE(rval) && *JSVAL_TO_DOUBLE(rval) == 0) {
                
                i = 0;
            } else {
                DO_NEXT_OP(len);
            }

            pc2 += JUMPX_OFFSET_LEN;
            low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            high = GET_JUMP_OFFSET(pc2);

            i -= low;
            if ((jsuint)i < (jsuint)(high - low + 1)) {
                pc2 += JUMP_OFFSET_LEN + JUMPX_OFFSET_LEN * i;
                off = (jsint) GET_JUMPX_OFFSET(pc2);
                if (off)
                    len = off;
            }
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_LOOKUPSWITCHX)
            off = JUMPX_OFFSET_LEN;
            goto do_lookup_switch;

          BEGIN_CASE(JSOP_LOOKUPSWITCH)
            off = JUMP_OFFSET_LEN;

          do_lookup_switch:
            



            JS_ASSERT(atoms == script->atomMap.vector);
            pc2 = regs.pc;
            lval = POP_OPND();

            if (!JSVAL_IS_NUMBER(lval) &&
                !JSVAL_IS_STRING(lval) &&
                !JSVAL_IS_BOOLEAN(lval)) {
                goto end_lookup_switch;
            }

            pc2 += off;
            npairs = (jsint) GET_UINT16(pc2);
            pc2 += UINT16_LEN;
            JS_ASSERT(npairs);  

#define SEARCH_PAIRS(MATCH_CODE)                                              \
    for (;;) {                                                                \
        JS_ASSERT(GET_INDEX(pc2) < script->atomMap.length);                   \
        atom = atoms[GET_INDEX(pc2)];                                         \
        rval = ATOM_KEY(atom);                                                \
        MATCH_CODE                                                            \
        pc2 += INDEX_LEN;                                                     \
        if (match)                                                            \
            break;                                                            \
        pc2 += off;                                                           \
        if (--npairs == 0) {                                                  \
            pc2 = regs.pc;                                                    \
            break;                                                            \
        }                                                                     \
    }
            if (JSVAL_IS_STRING(lval)) {
                str = JSVAL_TO_STRING(lval);
                SEARCH_PAIRS(
                    match = (JSVAL_IS_STRING(rval) &&
                             ((str2 = JSVAL_TO_STRING(rval)) == str ||
                              js_EqualStrings(str2, str)));
                )
            } else if (JSVAL_IS_DOUBLE(lval)) {
                d = *JSVAL_TO_DOUBLE(lval);
                SEARCH_PAIRS(
                    match = (JSVAL_IS_DOUBLE(rval) &&
                             *JSVAL_TO_DOUBLE(rval) == d);
                )
            } else {
                SEARCH_PAIRS(
                    match = (lval == rval);
                )
            }
#undef SEARCH_PAIRS

          end_lookup_switch:
            len = (op == JSOP_LOOKUPSWITCH)
                  ? GET_JUMP_OFFSET(pc2)
                  : GET_JUMPX_OFFSET(pc2);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_TRAP)
          {
            JSTrapStatus status;

            status = JS_HandleTrap(cx, script, regs.pc, &rval);
            switch (status) {
              case JSTRAP_ERROR:
                goto error;
              case JSTRAP_RETURN:
                fp->rval = rval;
                ok = JS_TRUE;
                goto forced_return;
              case JSTRAP_THROW:
                cx->throwing = JS_TRUE;
                cx->exception = rval;
                goto error;
              default:;
                break;
            }
            JS_ASSERT(status == JSTRAP_CONTINUE);
            CHECK_INTERRUPT_HANDLER();
            JS_ASSERT(JSVAL_IS_INT(rval));
            op = (JSOp) JSVAL_TO_INT(rval);
            JS_ASSERT((uintN)op < (uintN)JSOP_LIMIT);
            DO_OP();
          }

          BEGIN_CASE(JSOP_ARGUMENTS)
            if (!js_GetArgsValue(cx, fp, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_ARGSUB)
            id = INT_TO_JSID(GET_ARGNO(regs.pc));
            if (!js_GetArgsProperty(cx, fp, id, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
            id = ATOM_TO_JSID(rt->atomState.lengthAtom);
            if (!js_GetArgsProperty(cx, fp, id, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGCNT)

          BEGIN_CASE(JSOP_GETARG)
          BEGIN_CASE(JSOP_CALLARG)
            slot = GET_ARGNO(regs.pc);
            JS_ASSERT(slot < fp->fun->nargs);
            METER_SLOT_OP(op, slot);
            PUSH_OPND(fp->argv[slot]);
            if (op == JSOP_CALLARG)
                PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_SETARG)
            slot = GET_ARGNO(regs.pc);
            JS_ASSERT(slot < fp->fun->nargs);
            METER_SLOT_OP(op, slot);
            vp = &fp->argv[slot];
            GC_POKE(cx, *vp);
            *vp = FETCH_OPND(-1);
          END_SET_CASE(JSOP_SETARG)

          BEGIN_CASE(JSOP_GETLOCAL)
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < script->nslots);
            PUSH_OPND(fp->slots[slot]);
          END_CASE(JSOP_GETLOCAL)

          BEGIN_CASE(JSOP_CALLLOCAL)
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < script->nslots);
            PUSH_OPND(fp->slots[slot]);
            PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_CALLLOCAL)

          BEGIN_CASE(JSOP_SETLOCAL)
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < script->nslots);
            vp = &fp->slots[slot];
            GC_POKE(cx, *vp);
            *vp = FETCH_OPND(-1);
          END_SET_CASE(JSOP_SETLOCAL)

          BEGIN_CASE(JSOP_GETUPVAR)
          BEGIN_CASE(JSOP_CALLUPVAR)
          {
            JSUpvarArray *uva;
            uint32 skip;
            JSStackFrame *fp2;

            index = GET_UINT16(regs.pc);
            uva = JS_SCRIPT_UPVARS(script);
            JS_ASSERT(index < uva->length);
            skip = UPVAR_FRAME_SKIP(uva->vector[index]);
            fp2 = cx->display[script->staticDepth - skip];
            JS_ASSERT(fp2->fun && fp2->script);

            slot = UPVAR_FRAME_SLOT(uva->vector[index]);
            if (slot < fp2->fun->nargs) {
                vp = fp2->argv;
            } else {
                slot -= fp2->fun->nargs;
                JS_ASSERT(slot < fp2->script->nslots);
                vp = fp2->slots;
            }

            PUSH_OPND(vp[slot]);
            if (op == JSOP_CALLUPVAR)
                PUSH_OPND(JSVAL_NULL);
          }
          END_CASE(JSOP_GETUPVAR)

          BEGIN_CASE(JSOP_GETGVAR)
          BEGIN_CASE(JSOP_CALLGVAR)
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < GlobalVarCount(fp));
            METER_SLOT_OP(op, slot);
            lval = fp->slots[slot];
            if (JSVAL_IS_NULL(lval)) {
                op = (op == JSOP_GETGVAR) ? JSOP_NAME : JSOP_CALLNAME;
                DO_OP();
            }
            obj = fp->varobj;
            slot = JSVAL_TO_INT(lval);
            rval = OBJ_GET_SLOT(cx, obj, slot);
            PUSH_OPND(rval);
            if (op == JSOP_CALLGVAR)
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_GETGVAR)

          BEGIN_CASE(JSOP_SETGVAR)
            slot = GET_SLOTNO(regs.pc);
            JS_ASSERT(slot < GlobalVarCount(fp));
            METER_SLOT_OP(op, slot);
            rval = FETCH_OPND(-1);
            obj = fp->varobj;
            lval = fp->slots[slot];
            if (JSVAL_IS_NULL(lval)) {
                




                LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                    goto error;
            } else {
                slot = JSVAL_TO_INT(lval);
                JS_LOCK_OBJ(cx, obj);
                LOCKED_OBJ_WRITE_BARRIER(cx, obj, slot, rval);
                JS_UNLOCK_OBJ(cx, obj);
            }
          END_SET_CASE(JSOP_SETGVAR)

          BEGIN_CASE(JSOP_DEFCONST)
          BEGIN_CASE(JSOP_DEFVAR)
            index = GET_INDEX(regs.pc);
            atom = atoms[index];

            



            index += atoms - script->atomMap.vector;
            obj = fp->varobj;
            attrs = JSPROP_ENUMERATE;
            if (!(fp->flags & JSFRAME_EVAL))
                attrs |= JSPROP_PERMANENT;
            if (op == JSOP_DEFCONST)
                attrs |= JSPROP_READONLY;

            
            id = ATOM_TO_JSID(atom);
            if (!js_CheckRedeclaration(cx, obj, id, attrs, &obj2, &prop))
                goto error;

            
            if (!prop) {
                if (!OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                                         JS_PropertyStub, JS_PropertyStub,
                                         attrs, &prop)) {
                    goto error;
                }
                JS_ASSERT(prop);
                obj2 = obj;
            }

            





            if (!fp->fun &&
                index < GlobalVarCount(fp) &&
                (attrs & JSPROP_PERMANENT) &&
                obj2 == obj &&
                OBJ_IS_NATIVE(obj)) {
                sprop = (JSScopeProperty *) prop;
                if (SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(obj)) &&
                    SPROP_HAS_STUB_GETTER(sprop) &&
                    SPROP_HAS_STUB_SETTER(sprop)) {
                    






                    fp->slots[index] = INT_TO_JSVAL(sprop->slot);
                }
            }

            OBJ_DROP_PROPERTY(cx, obj2, prop);
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_DEFFUN)
            






            LOAD_FUNCTION(0);

            if (!fp->blockChain) {
                obj2 = fp->scopeChain;
            } else {
                obj2 = js_GetScopeChain(cx, fp);
                if (!obj2)
                    goto error;
            }

            








            obj = FUN_OBJECT(fun);
            if (OBJ_GET_PARENT(cx, obj) != obj2) {
                obj = js_CloneFunctionObject(cx, fun, obj2);
                if (!obj)
                    goto error;
            }

            




            MUST_FLOW_THROUGH("restore");
            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            



            attrs = (fp->flags & JSFRAME_EVAL)
                    ? JSPROP_ENUMERATE
                    : JSPROP_ENUMERATE | JSPROP_PERMANENT;

            




            flags = JSFUN_GSFLAG2ATTR(fun->flags);
            if (flags) {
                attrs |= flags | JSPROP_SHARED;
                rval = JSVAL_VOID;
            }

            





            parent = fp->varobj;
            JS_ASSERT(parent);

            





            id = ATOM_TO_JSID(fun->atom);
            ok = js_CheckRedeclaration(cx, parent, id, attrs, NULL, NULL);
            if (ok) {
                if (attrs == JSPROP_ENUMERATE) {
                    JS_ASSERT(fp->flags & JSFRAME_EVAL);
                    ok = OBJ_SET_PROPERTY(cx, parent, id, &rval);
                } else {
                    JS_ASSERT(attrs & JSPROP_PERMANENT);

                    ok = OBJ_DEFINE_PROPERTY(cx, parent, id, rval,
                                             (flags & JSPROP_GETTER)
                                             ? JS_EXTENSION (JSPropertyOp) obj
                                             : JS_PropertyStub,
                                             (flags & JSPROP_SETTER)
                                             ? JS_EXTENSION (JSPropertyOp) obj
                                             : JS_PropertyStub,
                                             attrs,
                                             NULL);
                }
            }

            
            MUST_FLOW_LABEL(restore)
            fp->scopeChain = obj2;
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                goto error;
            }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
            LOAD_FUNCTION(SLOTNO_LEN);

            






            slot = GET_SLOTNO(regs.pc);

            parent = js_GetScopeChain(cx, fp);
            if (!parent)
                goto error;

            obj = FUN_OBJECT(fun);
            if (OBJ_GET_PARENT(cx, obj) != parent) {
                obj = js_CloneFunctionObject(cx, fun, parent);
                if (!obj)
                    goto error;
            }

            TRACE_2(DefLocalFunSetSlot, slot, obj);

            fp->slots[slot] = OBJECT_TO_JSVAL(obj);
          END_CASE(JSOP_DEFLOCALFUN)

          BEGIN_CASE(JSOP_ANONFUNOBJ)
            
            LOAD_FUNCTION(0);

            
            parent = js_GetScopeChain(cx, fp);
            if (!parent)
                goto error;
            obj = FUN_OBJECT(fun);
            if (OBJ_GET_PARENT(cx, obj) != parent) {
                obj = js_CloneFunctionObject(cx, fun, parent);
                if (!obj)
                    goto error;
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_ANONFUNOBJ)

          BEGIN_CASE(JSOP_NAMEDFUNOBJ)
            LOAD_FUNCTION(0);

            









            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2)
                goto error;
            parent = js_NewObject(cx, &js_ObjectClass, NULL, obj2, 0);
            if (!parent)
                goto error;

            







            fp->scopeChain = parent;
            obj = js_CloneFunctionObject(cx, fun, parent);
            if (!obj)
                goto error;

            




            MUST_FLOW_THROUGH("restore2");
            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            




            attrs = JSFUN_GSFLAG2ATTR(fun->flags);
            if (attrs) {
                attrs |= JSPROP_SHARED;
                rval = JSVAL_VOID;
            }
            ok = OBJ_DEFINE_PROPERTY(cx, parent, ATOM_TO_JSID(fun->atom), rval,
                                     (attrs & JSPROP_GETTER)
                                     ? JS_EXTENSION (JSPropertyOp) obj
                                     : JS_PropertyStub,
                                     (attrs & JSPROP_SETTER)
                                     ? JS_EXTENSION (JSPropertyOp) obj
                                     : JS_PropertyStub,
                                     attrs |
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL);

            
            MUST_FLOW_LABEL(restore2)
            fp->scopeChain = obj2;
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                goto error;
            }

            



            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_NAMEDFUNOBJ)

#if JS_HAS_GETTER_SETTER
          BEGIN_CASE(JSOP_GETTER)
          BEGIN_CASE(JSOP_SETTER)
          do_getter_setter:
            op2 = (JSOp) *++regs.pc;
            switch (op2) {
              case JSOP_INDEXBASE:
                atoms += GET_INDEXBASE(regs.pc);
                regs.pc += JSOP_INDEXBASE_LENGTH - 1;
                goto do_getter_setter;
              case JSOP_INDEXBASE1:
              case JSOP_INDEXBASE2:
              case JSOP_INDEXBASE3:
                atoms += (op2 - JSOP_INDEXBASE1 + 1) << 16;
                goto do_getter_setter;

              case JSOP_SETNAME:
              case JSOP_SETPROP:
                LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                rval = FETCH_OPND(-1);
                i = -1;
                goto gs_pop_lval;

              case JSOP_SETELEM:
                rval = FETCH_OPND(-1);
                id = 0;
                i = -2;
              gs_pop_lval:
                FETCH_OBJECT(cx, i - 1, lval, obj);
                break;

              case JSOP_INITPROP:
                JS_ASSERT(regs.sp - StackBase(fp) >= 2);
                rval = FETCH_OPND(-1);
                i = -1;
                LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                goto gs_get_lval;

              default:
                JS_ASSERT(op2 == JSOP_INITELEM);

                JS_ASSERT(regs.sp - StackBase(fp) >= 3);
                rval = FETCH_OPND(-1);
                id = 0;
                i = -2;
              gs_get_lval:
                lval = FETCH_OPND(i-1);
                JS_ASSERT(JSVAL_IS_OBJECT(lval));
                obj = JSVAL_TO_OBJECT(lval);
                break;
            }

            
            if (id == 0)
                FETCH_ELEMENT_ID(obj, i, id);

            if (JS_TypeOfValue(cx, rval) != JSTYPE_FUNCTION) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_GETTER_OR_SETTER,
                                     (op == JSOP_GETTER)
                                     ? js_getter_str
                                     : js_setter_str);
                goto error;
            }

            



            if (!OBJ_CHECK_ACCESS(cx, obj, id, JSACC_WATCH, &rtmp, &attrs))
                goto error;

            if (op == JSOP_GETTER) {
                getter = JS_EXTENSION (JSPropertyOp) JSVAL_TO_OBJECT(rval);
                setter = JS_PropertyStub;
                attrs = JSPROP_GETTER;
            } else {
                getter = JS_PropertyStub;
                setter = JS_EXTENSION (JSPropertyOp) JSVAL_TO_OBJECT(rval);
                attrs = JSPROP_SETTER;
            }
            attrs |= JSPROP_ENUMERATE | JSPROP_SHARED;

            
            if (!js_CheckRedeclaration(cx, obj, id, attrs, NULL, NULL))
                goto error;

            if (!OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID, getter, setter,
                                     attrs, NULL)) {
                goto error;
            }

            regs.sp += i;
            if (js_CodeSpec[op2].ndefs)
                STORE_OPND(-1, rval);
            len = js_CodeSpec[op2].length;
            DO_NEXT_OP(len);
#endif 

          BEGIN_CASE(JSOP_HOLE)
            PUSH_OPND(JSVAL_HOLE);
          END_CASE(JSOP_HOLE)

          BEGIN_CASE(JSOP_NEWARRAY)
            len = GET_UINT24(regs.pc);
            JS_ASSERT(len <= regs.sp - StackBase(fp));
            obj = js_NewArrayObject(cx, len, regs.sp - len, JS_TRUE);
            if (!obj)
                goto error;
            regs.sp -= len - 1;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_NEWARRAY)

          BEGIN_CASE(JSOP_NEWINIT)
            i = GET_INT8(regs.pc);
            JS_ASSERT(i == JSProto_Array || i == JSProto_Object);
            obj = (i == JSProto_Array)
                  ? js_NewArrayObject(cx, 0, NULL)
                  : js_NewObject(cx, &js_ObjectClass, NULL, NULL, 0);
            if (!obj)
                goto error;
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
            fp->sharpDepth++;
            CHECK_INTERRUPT_HANDLER();
          END_CASE(JSOP_NEWINIT)

          BEGIN_CASE(JSOP_ENDINIT)
            if (--fp->sharpDepth == 0)
                fp->sharpArray = NULL;

            
            JS_ASSERT(regs.sp - StackBase(fp) >= 1);
            lval = FETCH_OPND(-1);
            JS_ASSERT(JSVAL_IS_OBJECT(lval));
            cx->weakRoots.newborn[GCX_OBJECT] = JSVAL_TO_GCTHING(lval);
          END_CASE(JSOP_ENDINIT)

          BEGIN_CASE(JSOP_INITPROP)
            
            JS_ASSERT(regs.sp - StackBase(fp) >= 2);
            rval = FETCH_OPND(-1);

            
            lval = FETCH_OPND(-2);
            obj = JSVAL_TO_OBJECT(lval);
            JS_ASSERT(OBJ_IS_NATIVE(obj));
            JS_ASSERT(!OBJ_GET_CLASS(cx, obj)->reserveSlots);
            JS_ASSERT(!(LOCKED_OBJ_GET_CLASS(obj)->flags &
                        JSCLASS_SHARE_ALL_PROPERTIES));

            do {
                JSScope *scope;
                uint32 kshape;
                JSPropertyCache *cache;
                JSPropCacheEntry *entry;

                JS_LOCK_OBJ(cx, obj);
                scope = OBJ_SCOPE(obj);
                JS_ASSERT(!SCOPE_IS_SEALED(scope));
                kshape = scope->shape;
                cache = &JS_PROPERTY_CACHE(cx);
                entry = &cache->table[PROPERTY_CACHE_HASH_PC(regs.pc, kshape)];
                PCMETER(cache->tests++);
                PCMETER(cache->initests++);

                if (entry->kpc == regs.pc && entry->kshape == kshape) {
                    PCMETER(cache->pchits++);
                    PCMETER(cache->inipchits++);

                    JS_ASSERT(PCVAL_IS_SPROP(entry->vword));
                    sprop = PCVAL_TO_SPROP(entry->vword);
                    JS_ASSERT(!(sprop->attrs & JSPROP_READONLY));

                    





                    if (!SPROP_HAS_STUB_SETTER(sprop))
                        goto do_initprop_miss;

                    if (scope->object != obj) {
                        scope = js_GetMutableScope(cx, obj);
                        if (!scope) {
                            JS_UNLOCK_OBJ(cx, obj);
                            goto error;
                        }
                    }

                    




                    if (sprop->parent != scope->lastProp)
                        goto do_initprop_miss;

                    TRACE_2(SetPropHit, entry, sprop);

                    




                    JS_ASSERT(PCVCAP_MAKE(sprop->shape, 0, 0) == entry->vcap);
                    JS_ASSERT(!SCOPE_HAD_MIDDLE_DELETE(scope));
                    JS_ASSERT(!scope->table ||
                              !SCOPE_HAS_PROPERTY(scope, sprop));

                    slot = sprop->slot;
                    JS_ASSERT(slot == scope->map.freeslot);
                    if (slot < STOBJ_NSLOTS(obj)) {
                        ++scope->map.freeslot;
                    } else {
                        if (!js_AllocSlot(cx, obj, &slot)) {
                            JS_UNLOCK_SCOPE(cx, scope);
                            goto error;
                        }
                        JS_ASSERT(slot == sprop->slot);
                    }

                    JS_ASSERT(!scope->lastProp ||
                              scope->shape == scope->lastProp->shape);
                    if (scope->table) {
                        JSScopeProperty *sprop2 =
                            js_AddScopeProperty(cx, scope, sprop->id,
                                                sprop->getter, sprop->setter,
                                                slot, sprop->attrs,
                                                sprop->flags, sprop->shortid);
                        if (!sprop2) {
                            js_FreeSlot(cx, obj, slot);
                            JS_UNLOCK_SCOPE(cx, scope);
                            goto error;
                        }
                        JS_ASSERT(sprop2 == sprop);
                    } else {
                        scope->shape = sprop->shape;
                        ++scope->entryCount;
                        scope->lastProp = sprop;
                    }

                    GC_WRITE_BARRIER(cx, scope,
                                     LOCKED_OBJ_GET_SLOT(obj, slot),
                                     rval);
                    LOCKED_OBJ_SET_SLOT(obj, slot, rval);
                    JS_UNLOCK_SCOPE(cx, scope);
                    break;
                }

              do_initprop_miss:
                PCMETER(cache->inipcmisses++);
                JS_UNLOCK_SCOPE(cx, scope);

                
                LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);

                
                if (!js_CheckRedeclaration(cx, obj, id, JSPROP_INITIALIZER,
                                           NULL, NULL)) {
                    goto error;
                }
                if (!js_SetPropertyHelper(cx, obj, id, &rval, &entry))
                    goto error;
#ifdef JS_TRACER
                if (entry)
                    TRACE_1(SetPropMiss, entry);
#endif
            } while (0);

            
            regs.sp--;
          END_CASE(JSOP_INITPROP);

          BEGIN_CASE(JSOP_INITELEM)
            
            JS_ASSERT(regs.sp - StackBase(fp) >= 3);
            rval = FETCH_OPND(-1);

            
            lval = FETCH_OPND(-3);
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(lval));
            obj = JSVAL_TO_OBJECT(lval);

            
            FETCH_ELEMENT_ID(obj, -2, id);

            



            if (!js_CheckRedeclaration(cx, obj, id, JSPROP_INITIALIZER, NULL,
                                       NULL)) {
                goto error;
            }

            




            if (rval == JSVAL_HOLE) {
                JS_ASSERT(OBJ_IS_ARRAY(cx, obj));
                JS_ASSERT(JSID_IS_INT(id));
                JS_ASSERT((jsuint) JSID_TO_INT(id) < ARRAY_INIT_LIMIT);
                if ((JSOp) regs.pc[JSOP_INITELEM_LENGTH] == JSOP_ENDINIT &&
                    !js_SetLengthProperty(cx, obj,
                                          (jsuint) (JSID_TO_INT(id) + 1))) {
                    goto error;
                }
            } else {
                if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                    goto error;
            }
            regs.sp -= 2;
          END_CASE(JSOP_INITELEM)

#if JS_HAS_SHARP_VARS
          BEGIN_CASE(JSOP_DEFSHARP)
            obj = fp->sharpArray;
            if (!obj) {
                obj = js_NewArrayObject(cx, 0, NULL);
                if (!obj)
                    goto error;
                fp->sharpArray = obj;
            }
            i = (jsint) GET_UINT16(regs.pc);
            id = INT_TO_JSID(i);
            rval = FETCH_OPND(-1);
            if (JSVAL_IS_PRIMITIVE(rval)) {
                char numBuf[12];
                JS_snprintf(numBuf, sizeof numBuf, "%u", (unsigned) i);
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_SHARP_DEF, numBuf);
                goto error;
            }
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
          END_CASE(JSOP_DEFSHARP)

          BEGIN_CASE(JSOP_USESHARP)
            i = (jsint) GET_UINT16(regs.pc);
            id = INT_TO_JSID(i);
            obj = fp->sharpArray;
            if (!obj) {
                rval = JSVAL_VOID;
            } else {
                if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                    goto error;
            }
            if (!JSVAL_IS_OBJECT(rval)) {
                char numBuf[12];

                JS_snprintf(numBuf, sizeof numBuf, "%u", (unsigned) i);
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_SHARP_USE, numBuf);
                goto error;
            }
            PUSH_OPND(rval);
          END_CASE(JSOP_USESHARP)
#endif 

          BEGIN_CASE(JSOP_GOSUB)
            PUSH(JSVAL_FALSE);
            i = PTRDIFF(regs.pc, script->main, jsbytecode) + JSOP_GOSUB_LENGTH;
            PUSH(INT_TO_JSVAL(i));
            len = GET_JUMP_OFFSET(regs.pc);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_GOSUBX)
            PUSH(JSVAL_FALSE);
            i = PTRDIFF(regs.pc, script->main, jsbytecode) + JSOP_GOSUBX_LENGTH;
            len = GET_JUMPX_OFFSET(regs.pc);
            PUSH(INT_TO_JSVAL(i));
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_RETSUB)
            
            rval = POP();
            lval = POP();
            JS_ASSERT(JSVAL_IS_BOOLEAN(lval));
            if (JSVAL_TO_BOOLEAN(lval)) {
                





                cx->throwing = JS_TRUE;
                cx->exception = rval;
                goto error;
            }
            JS_ASSERT(JSVAL_IS_INT(rval));
            len = JSVAL_TO_INT(rval);
            regs.pc = script->main;
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_EXCEPTION)
            JS_ASSERT(cx->throwing);
            PUSH(cx->exception);
            cx->throwing = JS_FALSE;
          END_CASE(JSOP_EXCEPTION)

          BEGIN_CASE(JSOP_THROWING)
            JS_ASSERT(!cx->throwing);
            cx->throwing = JS_TRUE;
            cx->exception = POP_OPND();
          END_CASE(JSOP_THROWING)

          BEGIN_CASE(JSOP_THROW)
            JS_ASSERT(!cx->throwing);
            cx->throwing = JS_TRUE;
            cx->exception = POP_OPND();
            
            goto error;

          BEGIN_CASE(JSOP_SETLOCALPOP)
            



            JS_ASSERT((size_t) (regs.sp - StackBase(fp)) >= 2);
            slot = GET_UINT16(regs.pc);
            JS_ASSERT(slot + 1 < script->nslots);
            fp->slots[slot] = POP_OPND();
          END_CASE(JSOP_SETLOCALPOP)

          BEGIN_CASE(JSOP_IFPRIMTOP)
            



            JS_ASSERT(regs.sp > StackBase(fp));
            rval = FETCH_OPND(-1);
            if (JSVAL_IS_PRIMITIVE(rval)) {
                len = GET_JUMP_OFFSET(regs.pc);
                BRANCH(len);
            }
          END_CASE(JSOP_IFPRIMTOP)

          BEGIN_CASE(JSOP_PRIMTOP)
            JS_ASSERT(regs.sp > StackBase(fp));
            lval = FETCH_OPND(-1);
            if (!JSVAL_IS_PRIMITIVE(lval)) {
                js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO,
                                    JSDVG_SEARCH_STACK, lval, NULL,
                                    "primitive type");
                goto error;
            }
          END_CASE(JSOP_PRIMTOP)

          BEGIN_CASE(JSOP_INSTANCEOF)
            rval = FETCH_OPND(-1);
            if (JSVAL_IS_PRIMITIVE(rval) ||
                !(obj = JSVAL_TO_OBJECT(rval))->map->ops->hasInstance) {
                js_ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS,
                                    -1, rval, NULL);
                goto error;
            }
            lval = FETCH_OPND(-2);
            cond = JS_FALSE;
            if (!obj->map->ops->hasInstance(cx, obj, lval, &cond))
                goto error;
            regs.sp--;
            STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));
          END_CASE(JSOP_INSTANCEOF)

#if JS_HAS_DEBUGGER_KEYWORD
          BEGIN_CASE(JSOP_DEBUGGER)
          {
            JSTrapHandler handler = cx->debugHooks->debuggerHandler;
            if (handler) {
                switch (handler(cx, script, regs.pc, &rval,
                                cx->debugHooks->debuggerHandlerData)) {
                  case JSTRAP_ERROR:
                    goto error;
                  case JSTRAP_CONTINUE:
                    break;
                  case JSTRAP_RETURN:
                    fp->rval = rval;
                    ok = JS_TRUE;
                    goto forced_return;
                  case JSTRAP_THROW:
                    cx->throwing = JS_TRUE;
                    cx->exception = rval;
                    goto error;
                  default:;
                }
                CHECK_INTERRUPT_HANDLER();
            }
          }
          END_CASE(JSOP_DEBUGGER)
#endif 

#if JS_HAS_XML_SUPPORT
          BEGIN_CASE(JSOP_DEFXMLNS)
            rval = POP();
            if (!js_SetDefaultXMLNamespace(cx, rval))
                goto error;
          END_CASE(JSOP_DEFXMLNS)

          BEGIN_CASE(JSOP_ANYNAME)
            if (!js_GetAnyName(cx, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ANYNAME)

          BEGIN_CASE(JSOP_QNAMEPART)
            LOAD_ATOM(0);
            PUSH_OPND(ATOM_KEY(atom));
          END_CASE(JSOP_QNAMEPART)

          BEGIN_CASE(JSOP_QNAMECONST)
            LOAD_ATOM(0);
            rval = ATOM_KEY(atom);
            lval = FETCH_OPND(-1);
            obj = js_ConstructXMLQNameObject(cx, lval, rval);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_QNAMECONST)

          BEGIN_CASE(JSOP_QNAME)
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
            obj = js_ConstructXMLQNameObject(cx, lval, rval);
            if (!obj)
                goto error;
            regs.sp--;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_QNAME)

          BEGIN_CASE(JSOP_TOATTRNAME)
            rval = FETCH_OPND(-1);
            if (!js_ToAttributeName(cx, &rval))
                goto error;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_TOATTRNAME)

          BEGIN_CASE(JSOP_TOATTRVAL)
            rval = FETCH_OPND(-1);
            JS_ASSERT(JSVAL_IS_STRING(rval));
            str = js_EscapeAttributeValue(cx, JSVAL_TO_STRING(rval), JS_FALSE);
            if (!str)
                goto error;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_TOATTRVAL)

          BEGIN_CASE(JSOP_ADDATTRNAME)
          BEGIN_CASE(JSOP_ADDATTRVAL)
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
            str = JSVAL_TO_STRING(lval);
            str2 = JSVAL_TO_STRING(rval);
            str = js_AddAttributePart(cx, op == JSOP_ADDATTRNAME, str, str2);
            if (!str)
                goto error;
            regs.sp--;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_ADDATTRNAME)

          BEGIN_CASE(JSOP_BINDXMLNAME)
            lval = FETCH_OPND(-1);
            if (!js_FindXMLProperty(cx, lval, &obj, &id))
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
            PUSH_OPND(ID_TO_VALUE(id));
          END_CASE(JSOP_BINDXMLNAME)

          BEGIN_CASE(JSOP_SETXMLNAME)
            obj = JSVAL_TO_OBJECT(FETCH_OPND(-3));
            rval = FETCH_OPND(-1);
            FETCH_ELEMENT_ID(obj, -2, id);
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
            rval = FETCH_OPND(-1);
            regs.sp -= 2;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETXMLNAME)

          BEGIN_CASE(JSOP_CALLXMLNAME)
          BEGIN_CASE(JSOP_XMLNAME)
            lval = FETCH_OPND(-1);
            if (!js_FindXMLProperty(cx, lval, &obj, &id))
                goto error;
            if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                goto error;
            STORE_OPND(-1, rval);
            if (op == JSOP_CALLXMLNAME)
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLNAME)

          BEGIN_CASE(JSOP_DESCENDANTS)
          BEGIN_CASE(JSOP_DELDESC)
            FETCH_OBJECT(cx, -2, lval, obj);
            rval = FETCH_OPND(-1);
            if (!js_GetXMLDescendants(cx, obj, rval, &rval))
                goto error;

            if (op == JSOP_DELDESC) {
                regs.sp[-1] = rval;          
                if (!js_DeleteXMLListElements(cx, JSVAL_TO_OBJECT(rval)))
                    goto error;
                rval = JSVAL_TRUE;      
            }

            regs.sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DESCENDANTS)

          BEGIN_CASE(JSOP_FILTER)
            




            PUSH_OPND(JSVAL_HOLE);
            len = GET_JUMP_OFFSET(regs.pc);
            JS_ASSERT(len > 0);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_ENDFILTER)
            cond = (regs.sp[-1] != JSVAL_HOLE);
            if (cond) {
                
                js_LeaveWith(cx);
            }
            if (!js_StepXMLListFilter(cx, cond))
                goto error;
            if (regs.sp[-1] != JSVAL_NULL) {
                



                JS_ASSERT(VALUE_IS_XML(cx, regs.sp[-1]));
                if (!js_EnterWith(cx, -2))
                    goto error;
                regs.sp--;
                len = GET_JUMP_OFFSET(regs.pc);
                JS_ASSERT(len < 0);
                BRANCH(len);
            }
            regs.sp--;
          END_CASE(JSOP_ENDFILTER);

          BEGIN_CASE(JSOP_TOXML)
            rval = FETCH_OPND(-1);
            obj = js_ValueToXMLObject(cx, rval);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_TOXML)

          BEGIN_CASE(JSOP_TOXMLLIST)
            rval = FETCH_OPND(-1);
            obj = js_ValueToXMLListObject(cx, rval);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_TOXMLLIST)

          BEGIN_CASE(JSOP_XMLTAGEXPR)
            rval = FETCH_OPND(-1);
            str = js_ValueToString(cx, rval);
            if (!str)
                goto error;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_XMLTAGEXPR)

          BEGIN_CASE(JSOP_XMLELTEXPR)
            rval = FETCH_OPND(-1);
            if (VALUE_IS_XML(cx, rval)) {
                str = js_ValueToXMLString(cx, rval);
            } else {
                str = js_ValueToString(cx, rval);
                if (str)
                    str = js_EscapeElementValue(cx, str);
            }
            if (!str)
                goto error;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_XMLELTEXPR)

          BEGIN_CASE(JSOP_XMLOBJECT)
            LOAD_OBJECT(0);
            obj = js_CloneXMLObject(cx, obj);
            if (!obj)
                goto error;
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLOBJECT)

          BEGIN_CASE(JSOP_XMLCDATA)
            LOAD_ATOM(0);
            str = ATOM_TO_STRING(atom);
            obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_TEXT, NULL, str);
            if (!obj)
                goto error;
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLCDATA)

          BEGIN_CASE(JSOP_XMLCOMMENT)
            LOAD_ATOM(0);
            str = ATOM_TO_STRING(atom);
            obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_COMMENT, NULL, str);
            if (!obj)
                goto error;
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLCOMMENT)

          BEGIN_CASE(JSOP_XMLPI)
            LOAD_ATOM(0);
            str = ATOM_TO_STRING(atom);
            rval = FETCH_OPND(-1);
            str2 = JSVAL_TO_STRING(rval);
            obj = js_NewXMLSpecialObject(cx,
                                         JSXML_CLASS_PROCESSING_INSTRUCTION,
                                         str, str2);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLPI)

          BEGIN_CASE(JSOP_GETFUNNS)
            if (!js_GetFunctionNamespace(cx, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_GETFUNNS)
#endif 

          BEGIN_CASE(JSOP_ENTERBLOCK)
            LOAD_OBJECT(0);
            JS_ASSERT(!OBJ_IS_CLONED_BLOCK(obj));
            JS_ASSERT(StackBase(fp) + OBJ_BLOCK_DEPTH(cx, obj) == regs.sp);
            vp = regs.sp + OBJ_BLOCK_COUNT(cx, obj);
            JS_ASSERT(regs.sp < vp);
            JS_ASSERT(vp <= fp->slots + script->nslots);
            while (regs.sp < vp) {
                STORE_OPND(0, JSVAL_VOID);
                regs.sp++;
            }

            








            if (fp->flags & JSFRAME_POP_BLOCKS) {
                JS_ASSERT(!fp->blockChain);
                obj = js_CloneBlockObject(cx, obj, fp->scopeChain, fp);
                if (!obj)
                    goto error;
                fp->scopeChain = obj;
            } else {
                JS_ASSERT(!fp->blockChain ||
                          OBJ_GET_PARENT(cx, obj) == fp->blockChain);
                fp->blockChain = obj;
            }
          END_CASE(JSOP_ENTERBLOCK)

          BEGIN_CASE(JSOP_LEAVEBLOCKEXPR)
          BEGIN_CASE(JSOP_LEAVEBLOCK)
          {
#ifdef DEBUG
             uintN blockDepth = OBJ_BLOCK_DEPTH(cx,
                                                fp->blockChain
                                                ? fp->blockChain
                                                : fp->scopeChain);

             JS_ASSERT(blockDepth <= StackDepth(script));
#endif
            if (fp->blockChain) {
                JS_ASSERT(OBJ_GET_CLASS(cx, fp->blockChain) == &js_BlockClass);
                fp->blockChain = OBJ_GET_PARENT(cx, fp->blockChain);
            } else {
                



                if (!js_PutBlockObject(cx, JS_TRUE))
                    goto error;
            }

            



            if (op == JSOP_LEAVEBLOCKEXPR)
                rval = FETCH_OPND(-1);
            regs.sp -= GET_UINT16(regs.pc);
            if (op == JSOP_LEAVEBLOCKEXPR) {
                JS_ASSERT(StackBase(fp) + blockDepth == regs.sp - 1);
                STORE_OPND(-1, rval);
            } else {
                JS_ASSERT(StackBase(fp) + blockDepth == regs.sp);
            }
          }
          END_CASE(JSOP_LEAVEBLOCK)

#if JS_HAS_GENERATORS
          BEGIN_CASE(JSOP_GENERATOR)
            ASSERT_NOT_THROWING(cx);
            regs.pc += JSOP_GENERATOR_LENGTH;
            obj = js_NewGenerator(cx, fp);
            if (!obj)
                goto error;
            JS_ASSERT(!fp->callobj && !fp->argsobj);
            fp->rval = OBJECT_TO_JSVAL(obj);
            ok = JS_TRUE;
            if (inlineCallCount != 0)
                goto inline_return;
            goto exit;

          BEGIN_CASE(JSOP_YIELD)
            ASSERT_NOT_THROWING(cx);
            if (FRAME_TO_GENERATOR(fp)->state == JSGEN_CLOSING) {
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_YIELD,
                                    JSDVG_SEARCH_STACK, fp->argv[-2], NULL);
                goto error;
            }
            fp->rval = FETCH_OPND(-1);
            fp->flags |= JSFRAME_YIELDING;
            regs.pc += JSOP_YIELD_LENGTH;
            ok = JS_TRUE;
            goto exit;

          BEGIN_CASE(JSOP_ARRAYPUSH)
            slot = GET_UINT16(regs.pc);
            JS_ASSERT(script->nfixed <= slot);
            JS_ASSERT(slot < script->nslots);
            lval = fp->slots[slot];
            obj  = JSVAL_TO_OBJECT(lval);
            JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_ArrayClass);
            rval = FETCH_OPND(-1);

            





            i = obj->fslots[JSSLOT_ARRAY_LENGTH];
            if (i == ARRAY_INIT_LIMIT) {
                JS_ReportErrorNumberUC(cx, js_GetErrorMessage, NULL,
                                       JSMSG_ARRAY_INIT_TOO_BIG);
                goto error;
            }
            id = INT_TO_JSID(i);
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
            regs.sp--;
          END_CASE(JSOP_ARRAYPUSH)
#endif 

#if JS_THREADED_INTERP
          L_JSOP_BACKPATCH:
          L_JSOP_BACKPATCH_POP:

# if !JS_HAS_GENERATORS
          L_JSOP_GENERATOR:
          L_JSOP_YIELD:
          L_JSOP_ARRAYPUSH:
# endif

# if !JS_HAS_DESTRUCTURING
          L_JSOP_ENUMCONSTELEM:
# endif

# if !JS_HAS_XML_SUPPORT
          L_JSOP_CALLXMLNAME:
          L_JSOP_STARTXMLEXPR:
          L_JSOP_STARTXML:
          L_JSOP_DELDESC:
          L_JSOP_GETFUNNS:
          L_JSOP_XMLPI:
          L_JSOP_XMLCOMMENT:
          L_JSOP_XMLCDATA:
          L_JSOP_XMLOBJECT:
          L_JSOP_XMLELTEXPR:
          L_JSOP_XMLTAGEXPR:
          L_JSOP_TOXMLLIST:
          L_JSOP_TOXML:
          L_JSOP_ENDFILTER:
          L_JSOP_FILTER:
          L_JSOP_DESCENDANTS:
          L_JSOP_XMLNAME:
          L_JSOP_SETXMLNAME:
          L_JSOP_BINDXMLNAME:
          L_JSOP_ADDATTRVAL:
          L_JSOP_ADDATTRNAME:
          L_JSOP_TOATTRVAL:
          L_JSOP_TOATTRNAME:
          L_JSOP_QNAME:
          L_JSOP_QNAMECONST:
          L_JSOP_QNAMEPART:
          L_JSOP_ANYNAME:
          L_JSOP_DEFXMLNS:
# endif

          L_JSOP_UNUSED135:
          L_JSOP_UNUSED203:
          L_JSOP_UNUSED204:
          L_JSOP_UNUSED205:
          L_JSOP_UNUSED206:
          L_JSOP_UNUSED207:
          L_JSOP_UNUSED208:
          L_JSOP_UNUSED209:
          L_JSOP_UNUSED219:
          L_JSOP_UNUSED226:

#else 
          default:
#endif
          {
            char numBuf[12];
            JS_snprintf(numBuf, sizeof numBuf, "%d", op);
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_BYTECODE, numBuf);
            goto error;
          }

#if !JS_THREADED_INTERP
        } 
    } 
#endif 

  error:
    if (fp->imacpc && cx->throwing) {
        
        if (*fp->imacpc == JSOP_NEXTITER) {
            JS_ASSERT(*regs.pc == JSOP_CALL);
            if (js_ValueIsStopIteration(cx->exception)) {
                cx->throwing = JS_FALSE;
                cx->exception = JSVAL_VOID;
                regs.sp[-1] = JSVAL_HOLE;
                PUSH(JSVAL_FALSE);
                goto end_imacro;
            }
        }

        
        regs.pc = fp->imacpc;
        fp->imacpc = NULL;
        atoms = script->atomMap.vector;
    }

    JS_ASSERT((size_t)(regs.pc - script->code) < script->length);
    if (!cx->throwing) {
        
        ok = JS_FALSE;
    } else {
        JSTrapHandler handler;
        JSTryNote *tn, *tnlimit;
        uint32 offset;

        
        handler = cx->debugHooks->throwHook;
        if (handler) {
            switch (handler(cx, script, regs.pc, &rval,
                            cx->debugHooks->throwHookData)) {
              case JSTRAP_ERROR:
                cx->throwing = JS_FALSE;
                goto error;
              case JSTRAP_RETURN:
                cx->throwing = JS_FALSE;
                fp->rval = rval;
                ok = JS_TRUE;
                goto forced_return;
              case JSTRAP_THROW:
                cx->exception = rval;
              case JSTRAP_CONTINUE:
              default:;
            }
            CHECK_INTERRUPT_HANDLER();
        }

        


        if (script->trynotesOffset == 0)
            goto no_catch;

        offset = (uint32)(regs.pc - script->main);
        tn = JS_SCRIPT_TRYNOTES(script)->vector;
        tnlimit = tn + JS_SCRIPT_TRYNOTES(script)->length;
        do {
            if (offset - tn->start >= tn->length)
                continue;

            


















            if (tn->stackDepth > regs.sp - StackBase(fp))
                continue;

            




            regs.pc = (script)->main + tn->start + tn->length;

            ok = js_UnwindScope(cx, fp, tn->stackDepth, JS_TRUE);
            JS_ASSERT(fp->regs->sp == StackBase(fp) + tn->stackDepth);
            if (!ok) {
                



                goto error;
            }

            switch (tn->kind) {
              case JSTRY_CATCH:
                JS_ASSERT(*regs.pc == JSOP_ENTERBLOCK);

#if JS_HAS_GENERATORS
                
                if (JS_UNLIKELY(cx->exception == JSVAL_ARETURN))
                    break;
#endif

                




                len = 0;
                DO_NEXT_OP(len);

              case JSTRY_FINALLY:
                



                PUSH(JSVAL_TRUE);
                PUSH(cx->exception);
                cx->throwing = JS_FALSE;
                len = 0;
                DO_NEXT_OP(len);

              case JSTRY_ITER:
                






                JS_ASSERT(*regs.pc == JSOP_ENDITER);
                regs.sp[-1] = cx->exception;
                cx->throwing = JS_FALSE;
                ok = js_CloseIterator(cx, regs.sp[-2]);
                regs.sp -= 2;
                if (!ok)
                    goto error;
                cx->throwing = JS_TRUE;
                cx->exception = regs.sp[1];
            }
        } while (++tn != tnlimit);

      no_catch:
        



        ok = JS_FALSE;
#if JS_HAS_GENERATORS
        if (JS_UNLIKELY(cx->throwing && cx->exception == JSVAL_ARETURN)) {
            cx->throwing = JS_FALSE;
            ok = JS_TRUE;
            fp->rval = JSVAL_VOID;
        }
#endif
    }

  forced_return:
    






    ok &= js_UnwindScope(cx, fp, 0, ok || cx->throwing);
    JS_ASSERT(regs.sp == StackBase(fp));

    if (inlineCallCount)
        goto inline_return;

  exit:
    










    JS_ASSERT(inlineCallCount == 0);
    JS_ASSERT(fp->regs == &regs);
#ifdef JS_TRACER
    if (TRACE_RECORDER(cx))
        js_AbortRecording(cx, "recording out of js_Interpret");
#endif
#if JS_HAS_GENERATORS
    if (JS_UNLIKELY(fp->flags & JSFRAME_YIELDING)) {
        JSGenerator *gen;

        gen = FRAME_TO_GENERATOR(fp);
        gen->savedRegs = regs;
        gen->frame.regs = &gen->savedRegs;
        JS_PROPERTY_CACHE(cx).disabled -= js_CountWithBlocks(cx, fp);
        JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled >= 0);
    } else
#endif 
    {
        JS_ASSERT(!fp->blockChain);
        JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));
        fp->regs = NULL;
    }

    
    if (script->staticDepth < JS_DISPLAY_SIZE)
        cx->display[script->staticDepth] = fp->displaySave;
    JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled == fp->pcDisabledSave);
    if (cx->version == currentVersion && currentVersion != originalVersion)
        js_SetVersion(cx, originalVersion);
    --cx->interpLevel;

#ifdef JS_TRACER
    if (tr) {
        JS_TRACE_MONITOR(cx).onTrace = JS_TRUE;
        SET_TRACE_RECORDER(cx, tr);
        if (!tr->wasDeepAborted()) {
            tr->popAbortStack();
            tr->deepAbort();
        }
    }
#endif
    return ok;

  atom_not_defined:
    {
        const char *printable;

        printable = js_AtomToPrintableString(cx, atom);
        if (printable)
            js_ReportIsNotDefined(cx, printable);
        goto error;
    }
}

#endif 
