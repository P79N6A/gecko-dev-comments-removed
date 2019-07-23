










































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
#include "jsconfig.h"
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

#ifdef INCLUDE_MOZILLA_DTRACE
#include "jsdtracef.h"
#endif

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

uint32
js_GenerateShape(JSContext *cx)
{
    JSRuntime *rt;
    uint32 shape;

    rt = cx->runtime;
    shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
    JS_ASSERT(shape != 0);
    if (shape & SHAPE_OVERFLOW_BIT) {
        rt->gcPoke = JS_TRUE;
        js_GC(cx, GC_NORMAL);
        shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
        JS_ASSERT(shape != 0);
        JS_ASSERT_IF(shape & SHAPE_OVERFLOW_BIT,
                     JS_PROPERTY_CACHE(cx).disabled);
    }
    return shape;
}

void
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

    cache = &JS_PROPERTY_CACHE(cx);
    pc = cx->fp->pc;
    if (cache->disabled) {
        PCMETER(cache->disfills++);
        *entryp = NULL;
        return;
    }

    



    scope = OBJ_SCOPE(pobj);
    JS_ASSERT(scope->object == pobj);
    if (!SCOPE_HAS_PROPERTY(scope, sprop)) {
        PCMETER(cache->oddfills++);
        return;
    }

    
    if (scopeIndex > PCVCAP_SCOPEMASK || protoIndex > PCVCAP_PROTOMASK) {
        PCMETER(cache->longchains++);
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
                        SCOPE_GENERATE_PCTYPE(cx, scope);
                        SCOPE_SET_BRANDED(scope);
                        kshape = scope->shape;
                    }
                    vword = JSVAL_OBJECT_TO_PCVAL(v);
                    break;
                }
            }
        }

        
        if (!(cs->format & JOF_SET) &&
            SPROP_HAS_STUB_GETTER(sprop) &&
            SPROP_HAS_VALID_SLOT(sprop, scope)) {
            
            vword = SLOT_TO_PCVAL(sprop->slot);
        } else {
            
            vword = SPROP_TO_PCVAL(sprop);
        }
    } while (0);

    khash = PROPERTY_CACHE_HASH_PC(pc, kshape);
    if (obj == pobj) {
        JS_ASSERT(kshape != 0 || scope->shape != 0);
        JS_ASSERT(scopeIndex == 0 && protoIndex == 0);
        JS_ASSERT(OBJ_SCOPE(obj)->object == obj);
        if (!(cs->format & JOF_SET))
            kshape = scope->shape;
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

JSAtom *
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

    JS_ASSERT(JS_UPTRDIFF(pc, cx->fp->script->code) < cx->fp->script->length);

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
        entry = &JS_PROPERTY_CACHE(cx)
                 .table[PROPERTY_CACHE_HASH_PC(pc, OBJ_SCOPE(obj)->shape)];
        fprintf(stderr,
                "id miss for %s from %s:%u"
                " (pc %u, kpc %u, kshape %u, shape %u)\n",
                js_AtomToPrintableString(cx, atom),
                cx->fp->script->filename,
                js_PCToLineNumber(cx, cx->fp->script, pc),
                pc - cx->fp->script->code,
                entry->kpc - cx->fp->script->code,
                entry->kshape,
                OBJ_SCOPE(obj)->shape);
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

    if (PCVCAP_PCTYPE(vcap) == OBJ_SCOPE(pobj)->shape) {
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
        P(longchains);
        P(recycles);
        P(pcrecycles);
        P(tests);
        P(pchits);
        P(protopchits);
        P(settests);
        P(addpchits);
        P(setpchits);
        P(setpcmisses);
        P(setmisses);
        P(idmisses);
        P(komisses);
        P(vcmisses);
        P(misses);
        P(flushes);
# undef P

        fprintf(fp, "hit rates: pc %g%% (proto %g%%), set %g%%, full %g%%\n",
                (100. * cache->pchits) / cache->tests,
                (100. * cache->protopchits) / cache->tests,
                (100. * (cache->addpchits + cache->setpchits))
                / cache->settests,
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







#define PUSH(v)         (*sp++ = (v))
#define POP()           (*--sp)
#define SAVE_SP(fp)                                                           \
    (JS_ASSERT((fp)->script || !(fp)->spbase || (sp) == (fp)->spbase),        \
     (fp)->sp = sp)
#define RESTORE_SP(fp)  (sp = (fp)->sp)








#define SAVE_SP_AND_PC(fp)      (SAVE_SP(fp), (fp)->pc = pc)
#define RESTORE_SP_AND_PC(fp)   (RESTORE_SP(fp), pc = (fp)->pc)
#define ASSERT_SAVED_SP_AND_PC(fp) JS_ASSERT((fp)->sp == sp && (fp)->pc == pc);








#define PUSH_OPND(v)    (sp[-depth] = (jsval)pc, PUSH(v))
#define STORE_OPND(n,v) (sp[(n)-depth] = (jsval)pc, sp[n] = (v))
#define POP_OPND()      POP()
#define FETCH_OPND(n)   (sp[n])






#define STORE_NUMBER(cx, n, d)                                                \
    JS_BEGIN_MACRO                                                            \
        jsint i_;                                                             \
        jsval v_;                                                             \
                                                                              \
        if (JSDOUBLE_IS_INT(d, i_) && INT_FITS_IN_JSVAL(i_)) {                \
            v_ = INT_TO_JSVAL(i_);                                            \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_NewDoubleValue(cx, d, &v_))                               \
                goto error;                                                   \
        }                                                                     \
        STORE_OPND(n, v_);                                                    \
    JS_END_MACRO

#define STORE_INT(cx, n, i)                                                   \
    JS_BEGIN_MACRO                                                            \
        jsval v_;                                                             \
                                                                              \
        if (INT_FITS_IN_JSVAL(i)) {                                           \
            v_ = INT_TO_JSVAL(i);                                             \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_NewDoubleValue(cx, (jsdouble)(i), &v_))                   \
                goto error;                                                   \
        }                                                                     \
        STORE_OPND(n, v_);                                                    \
    JS_END_MACRO

#define STORE_UINT(cx, n, u)                                                  \
    JS_BEGIN_MACRO                                                            \
        jsval v_;                                                             \
                                                                              \
        if ((u) <= JSVAL_INT_MAX) {                                           \
            v_ = INT_TO_JSVAL(u);                                             \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_NewDoubleValue(cx, (jsdouble)(u), &v_))                   \
                goto error;                                                   \
        }                                                                     \
        STORE_OPND(n, v_);                                                    \
    JS_END_MACRO

#define FETCH_NUMBER(cx, n, d)                                                \
    JS_BEGIN_MACRO                                                            \
        jsval v_;                                                             \
                                                                              \
        v_ = FETCH_OPND(n);                                                   \
        VALUE_TO_NUMBER(cx, v_, d);                                           \
    JS_END_MACRO

#define FETCH_INT(cx, n, i)                                                   \
    JS_BEGIN_MACRO                                                            \
        jsval v_ = FETCH_OPND(n);                                             \
        if (JSVAL_IS_INT(v_)) {                                               \
            i = JSVAL_TO_INT(v_);                                             \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_ValueToECMAInt32(cx, v_, &i))                             \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#define FETCH_UINT(cx, n, ui)                                                 \
    JS_BEGIN_MACRO                                                            \
        jsval v_ = FETCH_OPND(n);                                             \
        jsint i_;                                                             \
        if (JSVAL_IS_INT(v_) && (i_ = JSVAL_TO_INT(v_)) >= 0) {               \
            ui = (uint32) i_;                                                 \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_ValueToECMAUint32(cx, v_, &ui))                           \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO





#define VALUE_TO_NUMBER(cx, v, d)                                             \
    JS_BEGIN_MACRO                                                            \
        if (JSVAL_IS_INT(v)) {                                                \
            d = (jsdouble)JSVAL_TO_INT(v);                                    \
        } else if (JSVAL_IS_DOUBLE(v)) {                                      \
            d = *JSVAL_TO_DOUBLE(v);                                          \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_ValueToNumber(cx, v, &d))                                 \
                goto error;                                                   \
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
        sp--;                                                                 \
    JS_END_MACRO

#define VALUE_TO_OBJECT(cx, n, v, obj)                                        \
    JS_BEGIN_MACRO                                                            \
        if (!JSVAL_IS_PRIMITIVE(v)) {                                         \
            obj = JSVAL_TO_OBJECT(v);                                         \
        } else {                                                              \
            SAVE_SP_AND_PC(fp);                                               \
            obj = js_ValueToNonNullObject(cx, v);                             \
            if (!obj)                                                         \
                goto error;                                                   \
            STORE_OPND(n, OBJECT_TO_JSVAL(obj));                              \
        }                                                                     \
    JS_END_MACRO


#define FETCH_OBJECT(cx, n, v, obj)                                           \
    JS_BEGIN_MACRO                                                            \
        ASSERT_SAVED_SP_AND_PC(fp);                                           \
        v = FETCH_OPND(n);                                                    \
        VALUE_TO_OBJECT(cx, n, v, obj);                                       \
    JS_END_MACRO

#define DEFAULT_VALUE(cx, n, hint, v)                                         \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));                                    \
        JS_ASSERT(v == sp[n]);                                                \
        SAVE_SP_AND_PC(fp);                                                   \
        if (!OBJ_DEFAULT_VALUE(cx, JSVAL_TO_OBJECT(v), hint, &sp[n]))         \
            goto error;                                                       \
        v = sp[n];                                                            \
    JS_END_MACRO





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

static jsval *
AllocRawStack(JSContext *cx, uintN nslots, void **markp)
{
    jsval *sp;

    if (!cx->stackPool.first.next) {
        int64 *timestamp;

        JS_ARENA_ALLOCATE(timestamp, &cx->stackPool, sizeof *timestamp);
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

static void
FreeRawStack(JSContext *cx, void *mark)
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

    
    sp = AllocRawStack(cx, 2 + nslots, markp);
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
        obj = JSVAL_TO_OBJECT(v);
        if (!JS_InstanceOf(cx, obj, clasp, vp + 2))
            return JS_FALSE;
        v = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    }
    *thisvp = v;
    return JS_TRUE;
}
















static JSObject *
ComputeGlobalThis(JSContext *cx, JSBool lazy, jsval *argv)
{
    JSObject *thisp;
    JSClass *clasp;

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

        









        fp = cx->fp;    
        if (lazy) {
            JS_ASSERT(!fp->thisp && fp->argv == argv);
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

    clasp = OBJ_GET_CLASS(cx, thisp);
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->outerObject && !(thisp = xclasp->outerObject(cx, thisp)))
            return NULL;
    }

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
        if (OBJ_GET_CLASS(cx, thisp) == &js_CallClass)
            return ComputeGlobalThis(cx, lazy, argv);

        if (thisp->map->ops->thisObject) {
            
            thisp = thisp->map->ops->thisObject(cx, thisp);
            if (!thisp)
                return NULL;
        }
        argv[-1] = OBJECT_TO_JSVAL(thisp);
    }
    return thisp;
}

JSObject *
js_ComputeThis(JSContext *cx, JSBool lazy, jsval *argv)
{
    if (JSVAL_IS_NULL(argv[-1]))
        return ComputeGlobalThis(cx, lazy, argv);
    return ComputeThis(cx, lazy, argv);
}

#if JS_HAS_NO_SUCH_METHOD

static JSBool
NoSuchMethod(JSContext *cx, uintN argc, jsval *vp, uint32 flags)
{
    JSStackFrame *fp;
    JSObject *thisp, *argsobj;
    JSAtom *atom;
    jsval *sp, roots[3];
    JSTempValueRooter tvr;
    jsid id;
    JSBool ok;
    jsbytecode *pc;

    
    JS_ASSERT(JSVAL_IS_PRIMITIVE(vp[0]));
    JS_ASSERT(JSVAL_IS_OBJECT(vp[1]));
    fp = cx->fp;
    RESTORE_SP(fp);

    
    memset(roots, 0, sizeof roots);
    JS_PUSH_TEMP_ROOT(cx, JS_ARRAY_LENGTH(roots), roots, &tvr);

    thisp = JSVAL_TO_OBJECT(vp[1]);
    if (!thisp) {
        thisp = ComputeGlobalThis(cx, JS_FALSE, vp + 2);
        if (!thisp) {
            ok = JS_FALSE;
            goto out;
        }
        fp->thisp = thisp;
    }

    id = ATOM_TO_JSID(cx->runtime->atomState.noSuchMethodAtom);
#if JS_HAS_XML_SUPPORT
    if (OBJECT_IS_XML(cx, thisp)) {
        JSXMLObjectOps *ops;

        ops = (JSXMLObjectOps *) thisp->map->ops;
        thisp = ops->getMethod(cx, thisp, id, &roots[2]);
        if (!thisp) {
            ok = JS_FALSE;
            goto out;
        }
        vp[1] = OBJECT_TO_JSVAL(thisp);
    } else
#endif
    {
        ok = OBJ_GET_PROPERTY(cx, thisp, id, &roots[2]);
        if (!ok)
            goto out;
    }
    if (JSVAL_IS_PRIMITIVE(roots[2]))
        goto not_function;

    pc = (jsbytecode *) vp[-(intN)fp->script->depth];
    switch ((JSOp) *pc) {
      case JSOP_NAME:
      case JSOP_GETPROP:
      case JSOP_CALLPROP:
        GET_ATOM_FROM_BYTECODE(fp->script, pc, 0, atom);
        roots[0] = ATOM_KEY(atom);
        argsobj = js_NewArrayObject(cx, argc, vp + 2);
        if (!argsobj) {
            ok = JS_FALSE;
            goto out;
        }
        roots[1] = OBJECT_TO_JSVAL(argsobj);
        ok = js_InternalInvoke(cx, thisp, roots[2], flags | JSINVOKE_INTERNAL,
                               2, roots, &vp[0]);
        break;

      default:
        goto not_function;
    }

  out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;

  not_function:
    js_ReportIsNotFunction(cx, vp, flags & JSINVOKE_FUNFLAGS);
    ok = JS_FALSE;
    goto out;
}

#endif 





#if defined DEBUG_brendan || defined DEBUG_mrbkap || defined DEBUG_shaver
# define DEBUG_NOT_THROWING 1
#endif

#ifdef DEBUG_NOT_THROWING
# define ASSERT_NOT_THROWING(cx) JS_ASSERT(!(cx)->throwing)
#else
# define ASSERT_NOT_THROWING(cx)
#endif





JS_STATIC_ASSERT(JSVAL_INT == 1);
JS_STATIC_ASSERT(JSVAL_DOUBLE == 2);
JS_STATIC_ASSERT(JSVAL_STRING == 4);
JS_STATIC_ASSERT(JSVAL_BOOLEAN == 6);

static const uint16 PrimitiveTestFlags[] = {
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_STRING,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_BOOLEAN,    
    JSFUN_THISP_NUMBER      
};

#define PRIMITIVE_THIS_TEST(fun,thisv)                                        \
    (JS_ASSERT(thisv != JSVAL_VOID),                                          \
     JSFUN_THISP_TEST(JSFUN_THISP_FLAGS((fun)->flags),                        \
                      PrimitiveTestFlags[JSVAL_TAG(thisv) - 1]))







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
    uintN nslots, nvars, i, skip;
    uint32 rootedArgsFlag;
    JSInterpreterHook hook;
    void *hookData;

    
    JS_ASSERT((jsval *) cx->stackPool.current->base <= vp);
    JS_ASSERT(vp + 2 + argc <= (jsval *) cx->stackPool.current->avail);

    



    mark = JS_ARENA_MARK(&cx->stackPool);
    v = *vp;

    










    if (JSVAL_IS_PRIMITIVE(v)) {
#if JS_HAS_NO_SUCH_METHOD
        if (cx->fp && cx->fp->script && !(flags & JSINVOKE_INTERNAL)) {
            ok = NoSuchMethod(cx, argc, vp, flags);
            goto out2;
        }
#endif
        goto bad;
    }

    funobj = JSVAL_TO_OBJECT(v);
    parent = OBJ_GET_PARENT(cx, funobj);
    clasp = OBJ_GET_CLASS(cx, funobj);
    if (clasp != &js_FunctionClass) {
        
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
        nslots = nvars = 0;

        
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
            nvars = fun->u.i.nvars;
        } else {
            native = fun->u.n.native;
            script = NULL;
            nvars = 0;
            nslots += fun->u.n.extra;
        }

        if (JSFUN_BOUND_METHOD_TEST(fun->flags)) {
            
            vp[1] = OBJECT_TO_JSVAL(parent);
        } else if (!JSVAL_IS_OBJECT(vp[1])) {
            JS_ASSERT(!(flags & JSINVOKE_CONSTRUCT));
            if (PRIMITIVE_THIS_TEST(fun, vp[1]))
                goto init_slots;
        }
    }

    if (flags & JSINVOKE_CONSTRUCT) {
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
    } else {
        









        if (native && fun && !(fun->flags & JSFUN_FAST_NATIVE) &&
            !js_ComputeThis(cx, JS_FALSE, vp + 2)) {
            ok = JS_FALSE;
            goto out2;
        }
    }

  init_slots:
    argv = vp + 2;
    sp = argv + argc;

    rootedArgsFlag = JSFRAME_ROOTED_ARGV;
    if (nslots != 0) {
        





        if (!AllocateAfterSP(cx, sp, nslots)) {
            rootedArgsFlag = 0;
            newvp = AllocRawStack(cx, 2 + argc + nslots, NULL);
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
            PUSH(JSVAL_VOID);
        } while (--i != 0);
    }

    if (native && fun && (fun->flags & JSFUN_FAST_NATIVE)) {
        JSTempValueRooter tvr;
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif
#if JS_HAS_LVALUE_RETURN
        
        cx->rval2set = JS_FALSE;
#endif
        
        skip = rootedArgsFlag ? 2 + argc : 0;
        JS_PUSH_TEMP_ROOT(cx, 2 + argc + nslots - skip, argv - 2 + skip, &tvr);
        ok = ((JSFastNative) native)(cx, argc, argv - 2);

        



        *vp = argv[-2];
        JS_POP_TEMP_ROOT(cx, &tvr);

        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
        goto out2;
    }

    
    if (nvars) {
        if (!AllocateAfterSP(cx, sp, nvars)) {
            
            sp = AllocRawStack(cx, nvars, NULL);
            if (!sp) {
                ok = JS_FALSE;
                goto out2;
            }
        }

        
        i = nvars;
        do {
            PUSH(JSVAL_VOID);
        } while (--i != 0);
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
    frame.nvars = nvars;
    frame.vars = sp - nvars;
    frame.down = cx->fp;
    frame.annotation = NULL;
    frame.scopeChain = NULL;    
    frame.pc = NULL;
    frame.sp = NULL;
    frame.spbase = NULL;
    frame.sharpDepth = 0;
    frame.sharpArray = NULL;
    frame.flags = flags | rootedArgsFlag;
    frame.dormantNext = NULL;
    frame.xmlNamespace = NULL;
    frame.blockChain = NULL;

    
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
        ok = js_Interpret(cx, script->code, &v);
    } else {
        
        frame.scopeChain = NULL;
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

    ok = js_Invoke(cx, argc, invokevp, flags | JSINVOKE_INTERNAL);
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
    



    JS_CHECK_RECURSION(cx, return JS_FALSE);

    














    JS_ASSERT(mode == JSACC_READ || mode == JSACC_WRITE);
    if (cx->runtime->checkObjectAccess &&
        VALUE_IS_FUNCTION(cx, fval) &&
        FUN_INTERPRETED(GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(fval))) &&
        !cx->runtime->checkObjectAccess(cx, obj, ID_TO_VALUE(id), mode,
                                        &fval)) {
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
    oldfp = cx->fp;
    frame.script = script;
    if (down) {
        
        frame.callobj = down->callobj;
        frame.argsobj = down->argsobj;
        frame.varobj = down->varobj;
        frame.callee = down->callee;
        frame.fun = down->fun;
        frame.thisp = down->thisp;
        frame.argc = down->argc;
        frame.argv = down->argv;
        frame.nvars = down->nvars;
        frame.vars = down->vars;
        frame.annotation = down->annotation;
        frame.sharpArray = down->sharpArray;
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
        frame.nvars = script->ngvars;
        if (script->regexpsOffset != 0)
            frame.nvars += JS_SCRIPT_REGEXPS(script)->length;
        if (frame.nvars != 0) {
            frame.vars = AllocRawStack(cx, frame.nvars, &mark);
            if (!frame.vars) {
                ok = JS_FALSE;
                goto out;
            }
            memset(frame.vars, 0, frame.nvars * sizeof(jsval));
        } else {
            frame.vars = NULL;
        }
        frame.annotation = NULL;
        frame.sharpArray = NULL;
    }
    frame.rval = JSVAL_VOID;
    frame.down = down;
    frame.scopeChain = chain;
    frame.pc = NULL;
    frame.sp = NULL;
    frame.spbase = NULL;
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
    if (hook) {
        hookData = hook(cx, &frame, JS_TRUE, 0,
                        cx->debugHooks->executeHookData);
    }

    



    ok = js_Interpret(cx, script->code, &frame.rval);
    *result = frame.rval;

    if (hookData) {
        hook = cx->debugHooks->executeHook;
        if (hook)
            hook(cx, &frame, JS_FALSE, &ok, hookData);
    }
    if (mark)
        FreeRawStack(cx, mark);
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

#if JS_HAS_EXPORT_IMPORT



static JSBool
ImportProperty(JSContext *cx, JSObject *obj, jsid id)
{
    JSBool ok;
    JSIdArray *ida;
    JSProperty *prop;
    JSObject *obj2, *target, *funobj, *closure;
    uintN attrs;
    jsint i;
    jsval value;

    if (JSVAL_IS_VOID(id)) {
        ida = JS_Enumerate(cx, obj);
        if (!ida)
            return JS_FALSE;
        ok = JS_TRUE;
        if (ida->length == 0)
            goto out;
    } else {
        ida = NULL;
        if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
            return JS_FALSE;
        if (!prop) {
            js_ReportValueError(cx, JSMSG_NOT_DEFINED,
                                JSDVG_IGNORE_STACK, ID_TO_VALUE(id), NULL);
            return JS_FALSE;
        }
        ok = OBJ_GET_ATTRIBUTES(cx, obj, id, prop, &attrs);
        OBJ_DROP_PROPERTY(cx, obj2, prop);
        if (!ok)
            return JS_FALSE;
        if (!(attrs & JSPROP_EXPORTED)) {
            js_ReportValueError(cx, JSMSG_NOT_EXPORTED,
                                JSDVG_IGNORE_STACK, ID_TO_VALUE(id), NULL);
            return JS_FALSE;
        }
    }

    target = cx->fp->varobj;
    i = 0;
    do {
        if (ida) {
            id = ida->vector[i];
            ok = OBJ_GET_ATTRIBUTES(cx, obj, id, NULL, &attrs);
            if (!ok)
                goto out;
            if (!(attrs & JSPROP_EXPORTED))
                continue;
        }
        ok = OBJ_CHECK_ACCESS(cx, obj, id, JSACC_IMPORT, &value, &attrs);
        if (!ok)
            goto out;
        if (VALUE_IS_FUNCTION(cx, value)) {
            funobj = JSVAL_TO_OBJECT(value);
            closure = js_CloneFunctionObject(cx, funobj, obj);
            if (!closure) {
                ok = JS_FALSE;
                goto out;
            }
            value = OBJECT_TO_JSVAL(closure);
        }

        








        if (OBJ_GET_CLASS(cx, target) == &js_CallClass) {
            ok = OBJ_LOOKUP_PROPERTY(cx, target, id, &obj2, &prop);
            if (!ok)
                goto out;
        } else {
            prop = NULL;
        }
        if (prop && target == obj2) {
            ok = OBJ_SET_PROPERTY(cx, target, id, &value);
        } else {
            ok = OBJ_DEFINE_PROPERTY(cx, target, id, value,
                                     JS_PropertyStub, JS_PropertyStub,
                                     attrs & ~(JSPROP_EXPORTED |
                                               JSPROP_GETTER |
                                               JSPROP_SETTER),
                                     NULL);
        }
        if (prop)
            OBJ_DROP_PROPERTY(cx, obj2, prop);
        if (!ok)
            goto out;
    } while (ida && ++i < ida->length);

out:
    if (ida)
        JS_DestroyIdArray(cx, ida);
    return ok;
}
#endif 

#define JSPROP_INITIALIZER 0x100   /* NB: Not a valid property attribute. */

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
            JSClass *lclasp, *rclasp;

            lobj = JSVAL_TO_OBJECT(lval);
            robj = JSVAL_TO_OBJECT(rval);
            lclasp = OBJ_GET_CLASS(cx, lobj);
            rclasp = OBJ_GET_CLASS(cx, robj);
            if (lclasp->flags & JSCLASS_IS_EXTENDED) {
                JSExtendedClass *xclasp = (JSExtendedClass *) lclasp;
                if (xclasp->wrappedObject &&
                    (lobj = xclasp->wrappedObject(cx, lobj))) {
                    lval = OBJECT_TO_JSVAL(lobj);
                }
            }
            if (rclasp->flags & JSCLASS_IS_EXTENDED) {
                JSExtendedClass *xclasp = (JSExtendedClass *) rclasp;
                if (xclasp->wrappedObject &&
                    (robj = xclasp->wrappedObject(cx, robj))) {
                    rval = OBJECT_TO_JSVAL(robj);
                }
            }
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
js_InvokeConstructor(JSContext *cx, jsval *vp, uintN argc)
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
            if (!FUN_INTERPRETED(fun2) && fun2->u.n.clasp)
                clasp = fun2->u.n.clasp;
        }
    }
    obj = js_NewObject(cx, clasp, proto, parent);
    if (!obj)
        return JS_FALSE;

    
    vp[1] = OBJECT_TO_JSVAL(obj);
    if (!js_Invoke(cx, argc, vp, JSINVOKE_CONSTRUCT)) {
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;
        return JS_FALSE;
    }

    
    rval = *vp;
    if (JSVAL_IS_PRIMITIVE(rval)) {
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

static JSBool
InternNonIntElementId(JSContext *cx, JSObject *obj, jsval idval, jsid *idp)
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





static JSBool
EnterWith(JSContext *cx, jsint stackIndex)
{
    JSStackFrame *fp;
    jsval *sp;
    JSObject *obj, *parent, *withobj;

    fp = cx->fp;
    sp = fp->sp;
    JS_ASSERT(stackIndex < 0);
    JS_ASSERT(fp->spbase <= sp + stackIndex);

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
                               sp + stackIndex - fp->spbase);
    if (!withobj)
        return JS_FALSE;

    fp->scopeChain = withobj;
    js_DisablePropertyCache(cx);
    return JS_TRUE;
}

static void
LeaveWith(JSContext *cx)
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

static JSClass *
IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth)
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

static jsint
CountWithBlocks(JSContext *cx, JSStackFrame *fp)
{
    jsint n;
    JSObject *obj;
    JSClass *clasp;

    n = 0;
    for (obj = fp->scopeChain;
         (clasp = IsActiveWithOrBlock(cx, obj, 0)) != NULL;
         obj = OBJ_GET_PARENT(cx, obj)) {
        if (clasp == &js_WithClass)
            ++n;
    }
    return n;
}





static JSBool
UnwindScope(JSContext *cx, JSStackFrame *fp, jsint stackDepth,
            JSBool normalUnwind)
{
    JSObject *obj;
    JSClass *clasp;

    JS_ASSERT(stackDepth >= 0);
    JS_ASSERT(fp->spbase + stackDepth <= fp->sp);

    for (obj = fp->blockChain; obj; obj = OBJ_GET_PARENT(cx, obj)) {
        JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_BlockClass);
        if (OBJ_BLOCK_DEPTH(cx, obj) < stackDepth)
            break;
    }
    fp->blockChain = obj;

    for (;;) {
        obj = fp->scopeChain;
        clasp = IsActiveWithOrBlock(cx, obj, stackDepth);
        if (!clasp)
            break;
        if (clasp == &js_BlockClass) {
            
            normalUnwind &= js_PutBlockObject(cx, normalUnwind);
        } else {
            LeaveWith(cx);
        }
    }

    fp->sp = fp->spbase + stackDepth;
    return normalUnwind;
}








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











#ifndef JS_OPMETER
# define METER_OP_INIT(op)
# define METER_OP_PAIR(op1,op2)
# define METER_SLOT_OP(op,slot)
#else

# include <stdlib.h>






# define METER_OP_INIT(op)      ((op) = JSOP_STOP)
# define METER_OP_PAIR(op1,op2) ((op1) != JSOP_STOP && ++succeeds[op1][op2])
# define HIST_NSLOTS            8
# define METER_SLOT_OP(op,slot) ((slot) < HIST_NSLOTS && ++slot_ops[op][slot])

static uint32 succeeds[JSOP_LIMIT][256];
static uint32 slot_ops[JSOP_LIMIT][HIST_NSLOTS];

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
        from = js_CodeSpec[i].name;
        for (j = 0; j < JSOP_LIMIT; j++) {
            count = succeeds[i][j];
            if (count != 0 && SIGNIFICANT(count, total)) {
                graph[nedges].from = from;
                graph[nedges].to = js_CodeSpec[j].name;
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
                
                fprintf(fp, "%-8.8s", js_CodeSpec[i].name);
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





JS_STATIC_ASSERT(JSOP_NAME_LENGTH == JSOP_CALLNAME_LENGTH);
JS_STATIC_ASSERT(JSOP_GETGVAR_LENGTH == JSOP_CALLGVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETVAR_LENGTH == JSOP_CALLVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETARG_LENGTH == JSOP_CALLARG_LENGTH);
JS_STATIC_ASSERT(JSOP_GETLOCAL_LENGTH == JSOP_CALLLOCAL_LENGTH);
JS_STATIC_ASSERT(JSOP_XMLNAME_LENGTH == JSOP_CALLXMLNAME_LENGTH);





JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);


JS_STATIC_ASSERT(JSOP_DEFFUN_LENGTH == JSOP_CLOSURE_LENGTH);

JSBool
js_Interpret(JSContext *cx, jsbytecode *pc, jsval *result)
{
    JSRuntime *rt;
    JSStackFrame *fp;
    JSScript *script;
    uintN inlineCallCount;
    JSAtom **atoms;
    JSObject *obj, *obj2, *parent;
    JSVersion currentVersion, originalVersion;
    JSBool ok, cond;
    JSTrapHandler interruptHandler;
    jsint depth, len;
    jsval *sp;
    void *mark;
    jsbytecode *endpc, *pc2;
    JSOp op, op2;
    jsatomid index;
    JSAtom *atom;
    uintN argc, attrs, flags, slot;
    jsval *vp, lval, rval, ltmp, rtmp;
    jsid id;
    JSObject *iterobj;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSString *str, *str2;
    jsint i, j;
    jsdouble d, d2;
    JSClass *clasp;
    JSFunction *fun;
    JSType type;
#if !JS_THREADED_INTERP && defined DEBUG
    FILE *tracefp = NULL;
#endif
#if JS_HAS_EXPORT_IMPORT
    JSIdArray *ida;
#endif
    jsint low, high, off, npairs;
    JSBool match;
#if JS_HAS_GETTER_SETTER
    JSPropertyOp getter, setter;
#endif

#ifdef __GNUC__
# define JS_EXTENSION __extension__
# define JS_EXTENSION_(s) __extension__ ({ s; })
#else
# define JS_EXTENSION
# define JS_EXTENSION_(s) s
#endif

#if JS_THREADED_INTERP
    static void *normalJumpTable[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
        JS_EXTENSION &&L_##op,
# include "jsopcode.tbl"
# undef OPDEF
    };

    static void *interruptJumpTable[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)              \
        JS_EXTENSION &&interrupt,
# include "jsopcode.tbl"
# undef OPDEF
    };

    register void **jumpTable = normalJumpTable;

    METER_OP_INIT(op);      

# define DO_OP()            JS_EXTENSION_(goto *jumpTable[op])
# define DO_NEXT_OP(n)      do { METER_OP_PAIR(op, pc[n]);                    \
                                 op = (JSOp) *(pc += (n));                    \
                                 DO_OP(); } while (0)
# define BEGIN_CASE(OP)     L_##OP:
# define END_CASE(OP)       DO_NEXT_OP(OP##_LENGTH);
# define END_VARLEN_CASE    DO_NEXT_OP(len);
# define EMPTY_CASE(OP)     BEGIN_CASE(OP) op = (JSOp) *++pc; DO_OP();
#else
# define DO_OP()            goto do_op
# define DO_NEXT_OP(n)      goto advance_pc
# define BEGIN_CASE(OP)     case OP:
# define END_CASE(OP)       break;
# define END_VARLEN_CASE    break;
# define EMPTY_CASE(OP)     BEGIN_CASE(OP) END_CASE(OP)
#endif

    
    JS_CHECK_RECURSION(cx, return JS_FALSE);

    *result = JSVAL_VOID;
    rt = cx->runtime;

    
    fp = cx->fp;
    script = fp->script;
    JS_ASSERT(script->length != 0);

    
    inlineCallCount = 0;

    






    atoms = script->atomMap.vector;

#define LOAD_ATOM(PCOFF)                                                      \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT((size_t)(atoms - script->atomMap.vector) <                  \
                  (size_t)(script->atomMap.length - GET_INDEX(pc + PCOFF)));  \
        atom = atoms[GET_INDEX(pc + PCOFF)];                                  \
    JS_END_MACRO

#define GET_FULL_INDEX(PCOFF)                                                 \
    (atoms - script->atomMap.vector + GET_INDEX(pc + PCOFF))

#define LOAD_OBJECT(PCOFF)                                                    \
    JS_GET_SCRIPT_OBJECT(script, GET_FULL_INDEX(PCOFF), obj)

#define LOAD_FUNCTION(PCOFF)                                                  \
    JS_BEGIN_MACRO                                                            \
        LOAD_OBJECT(PCOFF);                                                   \
        JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_FunctionClass);               \
    JS_END_MACRO

    



#define CHECK_BRANCH(len)                                                     \
    JS_BEGIN_MACRO                                                            \
        if (len <= 0 && (cx->operationCount -= JSOW_SCRIPT_JUMP) <= 0) {      \
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_ResetOperationCount(cx))                                  \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

    





#if JS_THREADED_INTERP
# define LOAD_JUMP_TABLE()                                                    \
    (jumpTable = interruptHandler ? interruptJumpTable : normalJumpTable)
#else
# define LOAD_JUMP_TABLE()
#endif

#define LOAD_INTERRUPT_HANDLER(cx)                                            \
    JS_BEGIN_MACRO                                                            \
        interruptHandler = (cx)->debugHooks->interruptHandler;                \
        LOAD_JUMP_TABLE();                                                    \
    JS_END_MACRO

    LOAD_INTERRUPT_HANDLER(cx);

    








    currentVersion = (JSVersion) script->version;
    originalVersion = (JSVersion) cx->version;
    if (currentVersion != originalVersion)
        js_SetVersion(cx, currentVersion);

    ++cx->interpLevel;
#ifdef DEBUG
    fp->pcDisabledSave = JS_PROPERTY_CACHE(cx).disabled;
#endif

    




    depth = (jsint) script->depth;
    if (JS_LIKELY(!fp->spbase)) {
        ASSERT_NOT_THROWING(cx);
        JS_ASSERT(!fp->sp);
        sp = AllocRawStack(cx, (uintN)(2 * depth), &mark);
        if (!sp) {
            mark = NULL;
            ok = JS_FALSE;
            goto exit;
        }
        JS_ASSERT(mark);
        sp += depth;        
        fp->spbase = sp;
        SAVE_SP(fp);
    } else {
        JS_ASSERT(fp->flags & JSFRAME_GENERATOR);
        mark = NULL;
        RESTORE_SP(fp);
        JS_ASSERT(JS_UPTRDIFF(sp, fp->spbase) <= depth * sizeof(jsval));
        JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled >= 0);
        JS_PROPERTY_CACHE(cx).disabled += CountWithBlocks(cx, fp);

        



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

#if JS_THREADED_INTERP

    











    op = (JSOp) *pc;
    if (interruptHandler) {
interrupt:
        SAVE_SP_AND_PC(fp);
        switch (interruptHandler(cx, script, pc, &rval,
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
        LOAD_INTERRUPT_HANDLER(cx);
    }

    JS_ASSERT((uintN)op < (uintN)JSOP_LIMIT);
    JS_EXTENSION_(goto *normalJumpTable[op]);

#else  

    for (;;) {
        op = (JSOp) *pc;
      do_op:
        len = js_CodeSpec[op].length;

#ifdef DEBUG
        tracefp = (FILE *) cx->tracefp;
        if (tracefp) {
            intN nuses, n;

            fprintf(tracefp, "%4u: ", js_PCToLineNumber(cx, script, pc));
            js_Disassemble1(cx, script, pc,
                            PTRDIFF(pc, script->code, jsbytecode), JS_FALSE,
                            tracefp);
            nuses = js_CodeSpec[op].nuses;
            if (nuses) {
                SAVE_SP_AND_PC(fp);
                for (n = -nuses; n < 0; n++) {
                    char *bytes = js_DecompileValueGenerator(cx, n, sp[n],
                                                             NULL);
                    if (bytes) {
                        fprintf(tracefp, "%s %s",
                                (n == -nuses) ? "  inputs:" : ",",
                                bytes);
                        JS_free(cx, bytes);
                    }
                }
                fprintf(tracefp, " @ %d\n", sp - fp->spbase);
            }
        }
#endif 

        if (interruptHandler) {
            SAVE_SP_AND_PC(fp);
            switch (interruptHandler(cx, script, pc, &rval,
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
            LOAD_INTERRUPT_HANDLER(cx);
        }

        switch (op) {

#endif

          EMPTY_CASE(JSOP_NOP)

          EMPTY_CASE(JSOP_GROUP)

          BEGIN_CASE(JSOP_PUSH)
            PUSH_OPND(JSVAL_VOID);
          END_CASE(JSOP_PUSH)

          BEGIN_CASE(JSOP_POP)
            sp--;
          END_CASE(JSOP_POP)

          BEGIN_CASE(JSOP_POPN)
            sp -= GET_UINT16(pc);
#ifdef DEBUG
            JS_ASSERT(fp->spbase <= sp);
            obj = fp->blockChain;
            JS_ASSERT(!obj ||
                      fp->spbase + OBJ_BLOCK_DEPTH(cx, obj)
                                 + OBJ_BLOCK_COUNT(cx, obj)
                      <= sp);
            for (obj = fp->scopeChain; obj; obj = OBJ_GET_PARENT(cx, obj)) {
                clasp = OBJ_GET_CLASS(cx, obj);
                if (clasp != &js_BlockClass && clasp != &js_WithClass)
                    continue;
                if (OBJ_GET_PRIVATE(cx, obj) != fp)
                    break;
                JS_ASSERT(fp->spbase + OBJ_BLOCK_DEPTH(cx, obj)
                                     + ((clasp == &js_BlockClass)
                                         ? OBJ_BLOCK_COUNT(cx, obj)
                                         : 1)
                          <= sp);
            }
#endif
          END_CASE(JSOP_POPN)

          BEGIN_CASE(JSOP_SWAP)
            vp = sp - depth;    
            ltmp = vp[-1];
            vp[-1] = vp[-2];
            sp[-2] = ltmp;
            rtmp = sp[-1];
            sp[-1] = sp[-2];
            sp[-2] = rtmp;
          END_CASE(JSOP_SWAP)

          BEGIN_CASE(JSOP_POPV)
            *result = POP_OPND();
          END_CASE(JSOP_POPV)

          BEGIN_CASE(JSOP_ENTERWITH)
            SAVE_SP_AND_PC(fp);
            if (!EnterWith(cx, -1))
                goto error;

            








            sp[-1] = OBJECT_TO_JSVAL(fp->scopeChain);
          END_CASE(JSOP_ENTERWITH)

          BEGIN_CASE(JSOP_LEAVEWITH)
            JS_ASSERT(sp[-1] == OBJECT_TO_JSVAL(fp->scopeChain));
            sp--;
            SAVE_SP_AND_PC(fp);
            LeaveWith(cx);
          END_CASE(JSOP_LEAVEWITH)

          BEGIN_CASE(JSOP_SETRVAL)
            ASSERT_NOT_THROWING(cx);
            fp->rval = POP_OPND();
          END_CASE(JSOP_SETRVAL)

          BEGIN_CASE(JSOP_RETURN)
            CHECK_BRANCH(-1);
            fp->rval = POP_OPND();
            

          BEGIN_CASE(JSOP_RETRVAL)    
          BEGIN_CASE(JSOP_STOP)
            



            ASSERT_NOT_THROWING(cx);
            JS_ASSERT(sp == fp->spbase);
            ok = JS_TRUE;
            if (inlineCallCount)
          inline_return:
            {
                JSInlineFrame *ifp = (JSInlineFrame *) fp;
                void *hookData = ifp->hookData;

                JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled == fp->pcDisabledSave);
                JS_ASSERT(!fp->blockChain);
                JS_ASSERT(!IsActiveWithOrBlock(cx, fp->scopeChain, 0));

                if (hookData) {
                    JSInterpreterHook hook;
                    JSBool status;

                    hook = cx->debugHooks->callHook;
                    if (hook) {
                        



                        SAVE_SP_AND_PC(fp);
                        status = ok;
                        hook(cx, fp, JS_FALSE, &status, hookData);
                        ok = status;
                        LOAD_INTERRUPT_HANDLER(cx);
                    }
                }

                





                if (fp->callobj) {
                    SAVE_SP_AND_PC(fp);
                    ok &= js_PutCallObject(cx, fp);
                }

                if (fp->argsobj) {
                    SAVE_SP_AND_PC(fp);
                    ok &= js_PutArgsObject(cx, fp);
                }

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

                
                vp = ifp->rvp;
                *vp = fp->rval;

                
                cx->fp = fp = fp->down;
                JS_ARENA_RELEASE(&cx->stackPool, ifp->mark);

                
                fp->sp = vp + 1;
                RESTORE_SP(fp);

                
                script = fp->script;
                depth = (jsint) script->depth;
                atoms = script->atomMap.vector;
                pc = fp->pc;

                
                vp[-depth] = (jsval)pc;

                
                inlineCallCount--;
                if (JS_LIKELY(ok)) {
                    JS_ASSERT(js_CodeSpec[*pc].length == JSOP_CALL_LENGTH);
                    len = JSOP_CALL_LENGTH;
                    DO_NEXT_OP(len);
                }
                goto error;
            }
            goto exit;

          BEGIN_CASE(JSOP_DEFAULT)
            (void) POP();
            
          BEGIN_CASE(JSOP_GOTO)
            len = GET_JUMP_OFFSET(pc);
            CHECK_BRANCH(len);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_IFEQ)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMP_OFFSET(pc);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_IFEQ)

          BEGIN_CASE(JSOP_IFNE)
            POP_BOOLEAN(cx, rval, cond);
            if (cond != JS_FALSE) {
                len = GET_JUMP_OFFSET(pc);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_IFNE)

          BEGIN_CASE(JSOP_OR)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_TRUE) {
                len = GET_JUMP_OFFSET(pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_OR)

          BEGIN_CASE(JSOP_AND)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMP_OFFSET(pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_AND)

          BEGIN_CASE(JSOP_DEFAULTX)
            (void) POP();
            
          BEGIN_CASE(JSOP_GOTOX)
            len = GET_JUMPX_OFFSET(pc);
            CHECK_BRANCH(len);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_IFEQX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMPX_OFFSET(pc);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_IFEQX)

          BEGIN_CASE(JSOP_IFNEX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond != JS_FALSE) {
                len = GET_JUMPX_OFFSET(pc);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_IFNEX)

          BEGIN_CASE(JSOP_ORX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_TRUE) {
                len = GET_JUMPX_OFFSET(pc);
                PUSH_OPND(rval);
                DO_NEXT_OP(len);
            }
          END_CASE(JSOP_ORX)

          BEGIN_CASE(JSOP_ANDX)
            POP_BOOLEAN(cx, rval, cond);
            if (cond == JS_FALSE) {
                len = GET_JUMPX_OFFSET(pc);
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
            if (!InternNonIntElementId(cx, obj, idval_, &id))                 \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

          BEGIN_CASE(JSOP_IN)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            if (JSVAL_IS_PRIMITIVE(rval)) {
                js_ReportValueError(cx, JSMSG_IN_NOT_OBJECT, -1, rval, NULL);
                goto error;
            }
            obj = JSVAL_TO_OBJECT(rval);
            FETCH_ELEMENT_ID(obj, -2, id);
            if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
                goto error;
            sp--;
            STORE_OPND(-1, BOOLEAN_TO_JSVAL(prop != NULL));
            if (prop)
                OBJ_DROP_PROPERTY(cx, obj2, prop);
          END_CASE(JSOP_IN)

          BEGIN_CASE(JSOP_FOREACH)
            flags = JSITER_ENUMERATE | JSITER_FOREACH;
            goto value_to_iter;

#if JS_HAS_DESTRUCTURING
          BEGIN_CASE(JSOP_FOREACHKEYVAL)
            flags = JSITER_ENUMERATE | JSITER_FOREACH | JSITER_KEYVALUE;
            goto value_to_iter;
#endif

          BEGIN_CASE(JSOP_FORIN)
            





            flags = JSITER_ENUMERATE;

          value_to_iter:
            JS_ASSERT(sp > fp->spbase);
            SAVE_SP_AND_PC(fp);
            if (!js_ValueToIterator(cx, flags, &sp[-1]))
                goto error;
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(sp[-1]));
            JS_ASSERT(JSOP_FORIN_LENGTH == js_CodeSpec[op].length);
          END_CASE(JSOP_FORIN)

          BEGIN_CASE(JSOP_FORPROP)
            



            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            i = -2;
            goto do_forinloop;

          BEGIN_CASE(JSOP_FORNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            

          BEGIN_CASE(JSOP_FORARG)
          BEGIN_CASE(JSOP_FORVAR)
          BEGIN_CASE(JSOP_FORCONST)
          BEGIN_CASE(JSOP_FORLOCAL)
            





            

          BEGIN_CASE(JSOP_FORELEM)
            





            i = -1;

          do_forinloop:
            



            JS_ASSERT(!JSVAL_IS_PRIMITIVE(sp[i]));
            iterobj = JSVAL_TO_OBJECT(sp[i]);

            SAVE_SP_AND_PC(fp);
            if (!js_CallIteratorNext(cx, iterobj, &rval))
                goto error;
            if (rval == JSVAL_HOLE) {
                rval = JSVAL_FALSE;
                goto end_forinloop;
            }

            switch (op) {
              case JSOP_FORARG:
                slot = GET_ARGNO(pc);
                JS_ASSERT(slot < fp->fun->nargs);
                fp->argv[slot] = rval;
                break;

              case JSOP_FORVAR:
                slot = GET_VARNO(pc);
                JS_ASSERT(slot < fp->fun->u.i.nvars);
                fp->vars[slot] = rval;
                break;

              case JSOP_FORCONST:
                
                break;

              case JSOP_FORLOCAL:
                slot = GET_UINT16(pc);
                JS_ASSERT(slot < (uintN)depth);
                vp = &fp->spbase[slot];
                GC_POKE(cx, *vp);
                *vp = rval;
                break;

              case JSOP_FORELEM:
                
                PUSH_OPND(rval);
                break;

              case JSOP_FORPROP:
                




                SAVE_SP_AND_PC(fp);
                FETCH_OBJECT(cx, -1, lval, obj);
                goto set_for_property;

              default:
                JS_ASSERT(op == JSOP_FORNAME);

                





                if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
                    goto error;
                if (prop)
                    OBJ_DROP_PROPERTY(cx, obj2, prop);

              set_for_property:
                
                fp->flags |= JSFRAME_ASSIGNING;
                ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
                fp->flags &= ~JSFRAME_ASSIGNING;
                if (!ok)
                    goto error;
                break;
            }

            
            rval = JSVAL_TRUE;

          end_forinloop:
            sp += i + 1;
            PUSH_OPND(rval);
            len = js_CodeSpec[op].length;
            DO_NEXT_OP(len);

          BEGIN_CASE(JSOP_DUP)
            JS_ASSERT(sp > fp->spbase);
            vp = sp - 1;                
            rval = *vp;
            vp -= depth;                
            vp[1] = *vp;
            PUSH(rval);
          END_CASE(JSOP_DUP)

          BEGIN_CASE(JSOP_DUP2)
            JS_ASSERT(sp - 2 >= fp->spbase);
            vp = sp - 1;                
            lval = vp[-1];
            rval = *vp;
            vp -= depth;                
            vp[1] = vp[2] = *vp;
            PUSH(lval);
            PUSH(rval);
          END_CASE(JSOP_DUP2)

#define PROPERTY_OP(n, call)                                                  \
    JS_BEGIN_MACRO                                                            \
        /* Fetch the left part and resolve it to a non-null object. */        \
        SAVE_SP_AND_PC(fp);                                                   \
        FETCH_OBJECT(cx, n, lval, obj);                                       \
                                                                              \
        /* Get or set the property. */                                        \
        if (!call)                                                            \
            goto error;                                                       \
    JS_END_MACRO

#define ELEMENT_OP(n, call)                                                   \
    JS_BEGIN_MACRO                                                            \
        /* Fetch the left part and resolve it to a non-null object. */        \
        SAVE_SP_AND_PC(fp);                                                   \
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
            SAVE_SP_AND_PC(fp);                                               \
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
            SAVE_SP_AND_PC(fp);                                               \
            if (!js_NativeSet(cx, obj, sprop, vp))                            \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#ifdef DEBUG
# define ASSERT_VALID_PROPERTY_CACHE_HIT(pcoff,obj,pobj,entry)                \
    do {                                                                      \
        JSAtom *atom_;                                                        \
        JSObject *obj_, *pobj_;                                               \
        JSProperty *prop_;                                                    \
        JSScopeProperty *sprop_;                                              \
        uint32 sample_ = rt->gcNumber;                                        \
        SAVE_SP_AND_PC(fp);                                                   \
        if (pcoff >= 0)                                                       \
            GET_ATOM_FROM_BYTECODE(script, pc, pcoff, atom_);                 \
        else                                                                  \
            atom_ = rt->atomState.lengthAtom;                                 \
        if (JOF_OPMODE(*pc) == JOF_NAME) {                                    \
             ok = js_FindProperty(cx, ATOM_TO_JSID(atom_), &obj_, &pobj_,     \
                                  &prop_);                                    \
        } else {                                                              \
             obj_ = obj;                                                      \
             ok = js_LookupProperty(cx, obj, ATOM_TO_JSID(atom_), &pobj_,     \
                                    &prop_);                                  \
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
# define ASSERT_VALID_PROPERTY_CACHE_HIT(pcoff,obj,pobj,entry)
#endif

          BEGIN_CASE(JSOP_SETCONST)
            LOAD_ATOM(0);
            obj = fp->varobj;
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            if (!OBJ_DEFINE_PROPERTY(cx, obj, ATOM_TO_JSID(atom), rval,
                                     JS_PropertyStub, JS_PropertyStub,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL)) {
                goto error;
            }
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETCONST)

#if JS_HAS_DESTRUCTURING
          BEGIN_CASE(JSOP_ENUMCONSTELEM)
            rval = FETCH_OPND(-3);
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -2, lval, obj);
            FETCH_ELEMENT_ID(obj, -1, id);
            if (!OBJ_DEFINE_PROPERTY(cx, obj, id, rval,
                                     JS_PropertyStub, JS_PropertyStub,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL)) {
                goto error;
            }
            sp -= 3;
          END_CASE(JSOP_ENUMCONSTELEM)
#endif

          BEGIN_CASE(JSOP_BINDNAME)
            do {
                JSPropCacheEntry *entry;

                obj = fp->scopeChain;
                if (JS_LIKELY(OBJ_IS_NATIVE(obj))) {
                    PROPERTY_CACHE_TEST(cx, pc, obj, obj2, entry, atom);
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
                SAVE_SP_AND_PC(fp);
                obj = js_FindIdentifierBase(cx, id, entry);
                if (!obj)
                    goto error;
            } while (0);
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_BINDNAME)

#define INTEGER_OP(OP, EXTRA_CODE)                                            \
    JS_BEGIN_MACRO                                                            \
        FETCH_INT(cx, -2, i);                                                 \
        FETCH_INT(cx, -1, j);                                                 \
        EXTRA_CODE                                                            \
        i = i OP j;                                                           \
        sp--;                                                                 \
        STORE_INT(cx, -1, i);                                                 \
    JS_END_MACRO

#define BITWISE_OP(OP)          INTEGER_OP(OP, (void) 0;)
#define SIGNED_SHIFT_OP(OP)     INTEGER_OP(OP, j &= 31;)

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
            ltmp = lval ^ JSVAL_VOID;                                         \
            rtmp = rval ^ JSVAL_VOID;                                         \
            if (ltmp && rtmp) {                                               \
                cond = JSVAL_TO_INT(lval) OP JSVAL_TO_INT(rval);              \
            } else {                                                          \
                d  = ltmp ? JSVAL_TO_INT(lval) : *rt->jsNaN;                  \
                d2 = rtmp ? JSVAL_TO_INT(rval) : *rt->jsNaN;                  \
                cond = JSDOUBLE_COMPARE(d, OP, d2, JS_FALSE);                 \
            }                                                                 \
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
                VALUE_TO_NUMBER(cx, lval, d);                                 \
                VALUE_TO_NUMBER(cx, rval, d2);                                \
                cond = JSDOUBLE_COMPARE(d, OP, d2, JS_FALSE);                 \
            }                                                                 \
        }                                                                     \
        sp--;                                                                 \
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
        SAVE_SP_AND_PC(fp);                                                   \
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
        SAVE_SP_AND_PC(fp);                                                   \
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
                    VALUE_TO_NUMBER(cx, lval, d);                             \
                    VALUE_TO_NUMBER(cx, rval, d2);                            \
                    cond = JSDOUBLE_COMPARE(d, OP, d2, IFNAN);                \
                }                                                             \
            }                                                                 \
        }                                                                     \
        sp--;                                                                 \
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
        sp--;                                                                 \
        STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));                               \
    JS_END_MACRO

          BEGIN_CASE(JSOP_STRICTEQ)
            STRICT_EQUALITY_OP(==);
          END_CASE(JSOP_STRICTEQ)

          BEGIN_CASE(JSOP_STRICTNE)
            STRICT_EQUALITY_OP(!=);
          END_CASE(JSOP_STRICTNE)

          BEGIN_CASE(JSOP_CASE)
            pc2 = (jsbytecode *) sp[-2-depth];
            STRICT_EQUALITY_OP(==);
            (void) POP();
            if (cond) {
                len = GET_JUMP_OFFSET(pc);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
            sp[-depth] = (jsval)pc2;
            PUSH(lval);
          END_CASE(JSOP_CASE)

          BEGIN_CASE(JSOP_CASEX)
            pc2 = (jsbytecode *) sp[-2-depth];
            STRICT_EQUALITY_OP(==);
            (void) POP();
            if (cond) {
                len = GET_JUMPX_OFFSET(pc);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
            sp[-depth] = (jsval)pc2;
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
            u >>= j & 31;
            sp--;
            STORE_UINT(cx, -1, u);
          }
          END_CASE(JSOP_URSH)

#undef INTEGER_OP
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
                SAVE_SP_AND_PC(fp);
                if (!ops->concatenate(cx, obj2, rval, &rval))
                    goto error;
                sp--;
                STORE_OPND(-1, rval);
            } else
#endif
            {
                if (!JSVAL_IS_PRIMITIVE(lval))
                    DEFAULT_VALUE(cx, -2, JSTYPE_VOID, lval);
                if (!JSVAL_IS_PRIMITIVE(rval))
                    DEFAULT_VALUE(cx, -1, JSTYPE_VOID, rval);
                if ((cond = JSVAL_IS_STRING(lval)) || JSVAL_IS_STRING(rval)) {
                    SAVE_SP_AND_PC(fp);
                    if (cond) {
                        str = JSVAL_TO_STRING(lval);
                        str2 = js_ValueToString(cx, rval);
                        if (!str2)
                            goto error;
                        sp[-1] = STRING_TO_JSVAL(str2);
                    } else {
                        str2 = JSVAL_TO_STRING(rval);
                        str = js_ValueToString(cx, lval);
                        if (!str)
                            goto error;
                        sp[-2] = STRING_TO_JSVAL(str);
                    }
                    str = js_ConcatStrings(cx, str, str2);
                    if (!str)
                        goto error;
                    sp--;
                    STORE_OPND(-1, STRING_TO_JSVAL(str));
                } else {
                    VALUE_TO_NUMBER(cx, lval, d);
                    VALUE_TO_NUMBER(cx, rval, d2);
                    d += d2;
                    sp--;
                    STORE_NUMBER(cx, -1, d);
                }
            }
          END_CASE(JSOP_ADD)

#define BINARY_OP(OP)                                                         \
    JS_BEGIN_MACRO                                                            \
        FETCH_NUMBER(cx, -1, d2);                                             \
        FETCH_NUMBER(cx, -2, d);                                              \
        d = d OP d2;                                                          \
        sp--;                                                                 \
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
            sp--;
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
            sp--;
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
            if (JSVAL_IS_INT(rval) && (i = JSVAL_TO_INT(rval)) != 0) {
                i = -i;
                JS_ASSERT(INT_FITS_IN_JSVAL(i));
                rval = INT_TO_JSVAL(i);
            } else {
                SAVE_SP_AND_PC(fp);
                if (JSVAL_IS_DOUBLE(rval))
                    d = *JSVAL_TO_DOUBLE(rval);
                else if (!js_ValueToNumber(cx, rval, &d))
                    goto error;
#ifdef HPUX
                




                JSDOUBLE_HI32(d) ^= JSDOUBLE_HI32_SIGNBIT;
#else
                d = -d;
#endif
                if (!js_NewNumberValue(cx, d, &rval))
                    goto error;
            }
            STORE_OPND(-1, rval);
          END_CASE(JSOP_NEG)

          BEGIN_CASE(JSOP_POS)
            rval = FETCH_OPND(-1);
            if (!JSVAL_IS_NUMBER(rval)) {
                SAVE_SP_AND_PC(fp);
                if (!js_ValueToNumber(cx, rval, &d))
                    goto error;
                if (!js_NewNumberValue(cx, d, &rval))
                    goto error;
                sp[-1] = rval;
            }
            sp[-1-depth] = (jsval)pc;
          END_CASE(JSOP_POS)

          BEGIN_CASE(JSOP_NEW)
            
            argc = GET_ARGC(pc);
            SAVE_SP_AND_PC(fp);
            vp = sp - (2 + argc);
            JS_ASSERT(vp >= fp->spbase);

            if (!js_InvokeConstructor(cx, vp, argc))
                goto error;
            sp = vp + 1;
            vp[-depth] = (jsval)pc;
            LOAD_INTERRUPT_HANDLER(cx);
          END_CASE(JSOP_NEW)

          BEGIN_CASE(JSOP_DELNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);

            SAVE_SP_AND_PC(fp);
            if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
                goto error;

            
            rval = JSVAL_TRUE;
            if (prop) {
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                if (!OBJ_DELETE_PROPERTY(cx, obj, id, &rval))
                    goto error;
            }
            PUSH_OPND(rval);
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            PROPERTY_OP(-1, OBJ_DELETE_PROPERTY(cx, obj, id, &rval));
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DELPROP)

          BEGIN_CASE(JSOP_DELELEM)
            ELEMENT_OP(-1, OBJ_DELETE_PROPERTY(cx, obj, id, &rval));
            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DELELEM)

          BEGIN_CASE(JSOP_TYPEOFEXPR)
          BEGIN_CASE(JSOP_TYPEOF)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            type = JS_TypeOfValue(cx, rval);
            atom = rt->atomState.typeAtoms[type];
            STORE_OPND(-1, ATOM_KEY(atom));
          END_CASE(JSOP_TYPEOF)

          BEGIN_CASE(JSOP_VOID)
            (void) POP_OPND();
            PUSH_OPND(JSVAL_VOID);
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
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, i, lval, obj);
            if (id == 0)
                FETCH_ELEMENT_ID(obj, -1, id);
            goto do_incop;

          BEGIN_CASE(JSOP_INCNAME)
          BEGIN_CASE(JSOP_DECNAME)
          BEGIN_CASE(JSOP_NAMEINC)
          BEGIN_CASE(JSOP_NAMEDEC)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);

            SAVE_SP_AND_PC(fp);
            if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
                goto error;
            if (!prop)
                goto atom_not_defined;

            OBJ_DROP_PROPERTY(cx, obj2, prop);
            lval = OBJECT_TO_JSVAL(obj);
            i = 0;

          do_incop:
          {
            const JSCodeSpec *cs;

            
            if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                goto error;

            
            cs = &js_CodeSpec[op];

            
            if (JSVAL_IS_INT(rval) &&
                rval != INT_TO_JSVAL(JSVAL_INT_MIN) &&
                rval != INT_TO_JSVAL(JSVAL_INT_MAX)) {
                if (cs->format & JOF_POST) {
                    rtmp = rval;
                    (cs->format & JOF_INC) ? (rval += 2) : (rval -= 2);
                } else {
                    (cs->format & JOF_INC) ? (rval += 2) : (rval -= 2);
                    rtmp = rval;
                }
            } else {








#define NONINT_INCREMENT_OP_MIDDLE()                                          \
    JS_BEGIN_MACRO                                                            \
        VALUE_TO_NUMBER(cx, rval, d);                                         \
        if (cs->format & JOF_POST) {                                          \
            rtmp = rval;                                                      \
            if (!JSVAL_IS_NUMBER(rtmp) && !js_NewNumberValue(cx, d, &rtmp))   \
                goto error;                                                   \
            *vp = rtmp;                                                       \
            (cs->format & JOF_INC) ? d++ : d--;                               \
            if (!js_NewNumberValue(cx, d, &rval))                             \
                goto error;                                                   \
        } else {                                                              \
            (cs->format & JOF_INC) ? ++d : --d;                               \
            if (!js_NewNumberValue(cx, d, &rval))                             \
                goto error;                                                   \
            rtmp = rval;                                                      \
        }                                                                     \
    JS_END_MACRO

                if (cs->format & JOF_POST) {
                    




                    vp = sp;
                    PUSH(JSVAL_VOID);
                    SAVE_SP(fp);
                    --i;
                }
#ifdef __GNUC__
                else vp = NULL; 
#endif

                NONINT_INCREMENT_OP_MIDDLE();
            }

            fp->flags |= JSFRAME_ASSIGNING;
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            fp->flags &= ~JSFRAME_ASSIGNING;
            if (!ok)
                goto error;
            sp += i;
            PUSH_OPND(rtmp);
            len = js_CodeSpec[op].length;
            DO_NEXT_OP(len);
          }


#define FAST_INCREMENT_OP(SLOT,COUNT,BASE,PRE,OPEQ,MINMAX)                    \
    slot = SLOT;                                                              \
    JS_ASSERT(slot < fp->fun->COUNT);                                         \
    METER_SLOT_OP(op, slot);                                                  \
    vp = fp->BASE + slot;                                                     \
    rval = *vp;                                                               \
    if (!JSVAL_IS_INT(rval) || rval == INT_TO_JSVAL(JSVAL_INT_##MINMAX))      \
        goto do_nonint_fast_incop;                                            \
    PRE = rval;                                                               \
    rval OPEQ 2;                                                              \
    *vp = rval;                                                               \
    PUSH_OPND(PRE);                                                           \
    goto end_nonint_fast_incop

          BEGIN_CASE(JSOP_INCARG)
            FAST_INCREMENT_OP(GET_ARGNO(pc), nargs, argv, rval, +=, MAX);
          BEGIN_CASE(JSOP_DECARG)
            FAST_INCREMENT_OP(GET_ARGNO(pc), nargs, argv, rval, -=, MIN);
          BEGIN_CASE(JSOP_ARGINC)
            FAST_INCREMENT_OP(GET_ARGNO(pc), nargs, argv, rtmp, +=, MAX);
          BEGIN_CASE(JSOP_ARGDEC)
            FAST_INCREMENT_OP(GET_ARGNO(pc), nargs, argv, rtmp, -=, MIN);

          BEGIN_CASE(JSOP_INCVAR)
            FAST_INCREMENT_OP(GET_VARNO(pc), u.i.nvars, vars, rval, +=, MAX);
          BEGIN_CASE(JSOP_DECVAR)
            FAST_INCREMENT_OP(GET_VARNO(pc), u.i.nvars, vars, rval, -=, MIN);
          BEGIN_CASE(JSOP_VARINC)
            FAST_INCREMENT_OP(GET_VARNO(pc), u.i.nvars, vars, rtmp, +=, MAX);
          BEGIN_CASE(JSOP_VARDEC)
            FAST_INCREMENT_OP(GET_VARNO(pc), u.i.nvars, vars, rtmp, -=, MIN);

          end_nonint_fast_incop:
            len = JSOP_INCARG_LENGTH;   
            DO_NEXT_OP(len);

#undef FAST_INCREMENT_OP

          do_nonint_fast_incop:
          {
            const JSCodeSpec *cs = &js_CodeSpec[op];

            NONINT_INCREMENT_OP_MIDDLE();
            *vp = rval;
            PUSH_OPND(rtmp);
            len = cs->length;
            DO_NEXT_OP(len);
          }


#define FAST_GLOBAL_INCREMENT_OP(SLOWOP,PRE,OPEQ,MINMAX)                      \
    slot = GET_VARNO(pc);                                                     \
    JS_ASSERT(slot < fp->nvars);                                              \
    METER_SLOT_OP(op, slot);                                                  \
    lval = fp->vars[slot];                                                    \
    if (JSVAL_IS_NULL(lval)) {                                                \
        op = SLOWOP;                                                          \
        DO_OP();                                                              \
    }                                                                         \
    slot = JSVAL_TO_INT(lval);                                                \
    obj = fp->varobj;                                                         \
    rval = OBJ_GET_SLOT(cx, obj, slot);                                       \
    if (!JSVAL_IS_INT(rval) || rval == INT_TO_JSVAL(JSVAL_INT_##MINMAX))      \
        goto do_nonint_fast_global_incop;                                     \
    PRE = rval;                                                               \
    rval OPEQ 2;                                                              \
    OBJ_SET_SLOT(cx, obj, slot, rval);                                        \
    PUSH_OPND(PRE);                                                           \
    goto end_nonint_fast_global_incop

          BEGIN_CASE(JSOP_INCGVAR)
            FAST_GLOBAL_INCREMENT_OP(JSOP_INCNAME, rval, +=, MAX);
          BEGIN_CASE(JSOP_DECGVAR)
            FAST_GLOBAL_INCREMENT_OP(JSOP_DECNAME, rval, -=, MIN);
          BEGIN_CASE(JSOP_GVARINC)
            FAST_GLOBAL_INCREMENT_OP(JSOP_NAMEINC, rtmp, +=, MAX);
          BEGIN_CASE(JSOP_GVARDEC)
            FAST_GLOBAL_INCREMENT_OP(JSOP_NAMEDEC, rtmp, -=, MIN);

          end_nonint_fast_global_incop:
            len = JSOP_INCGVAR_LENGTH;  
            JS_ASSERT(len == js_CodeSpec[op].length);
            DO_NEXT_OP(len);

#undef FAST_GLOBAL_INCREMENT_OP

          do_nonint_fast_global_incop:
          {
            const JSCodeSpec *cs = &js_CodeSpec[op];

            vp = sp++;
            SAVE_SP(fp);
            NONINT_INCREMENT_OP_MIDDLE();
            OBJ_SET_SLOT(cx, obj, slot, rval);
            STORE_OPND(-1, rtmp);
            len = cs->length;
            DO_NEXT_OP(len);
          }

          BEGIN_CASE(JSOP_GETTHISPROP)
            i = 0;
            obj = fp->thisp;
            if (!obj) {
                obj = ComputeGlobalThis(cx, JS_TRUE, fp->argv);
                if (!obj)
                    goto error;
                fp->thisp = obj;
            }
            PUSH(JSVAL_NULL);
            len = JSOP_GETTHISPROP_LENGTH;
            goto do_getprop_with_obj;

          BEGIN_CASE(JSOP_GETARGPROP)
            i = ARGNO_LEN;
            slot = GET_ARGNO(pc);
            JS_ASSERT(slot < fp->fun->nargs);
            PUSH_OPND(fp->argv[slot]);
            len = JSOP_GETARGPROP_LENGTH;
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETVARPROP)
            i = VARNO_LEN;
            slot = GET_VARNO(pc);
            JS_ASSERT(slot < fp->fun->u.i.nvars);
            PUSH_OPND(fp->vars[slot]);
            len = JSOP_GETVARPROP_LENGTH;
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETLOCALPROP)
            i = UINT16_LEN;
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            PUSH_OPND(fp->spbase[slot]);
            len = JSOP_GETLOCALPROP_LENGTH;
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETPROP)
          BEGIN_CASE(JSOP_GETXPROP)
            i = 0;
            len = JSOP_GETPROP_LENGTH;

          do_getprop_body:
            lval = FETCH_OPND(-1);

          do_getprop_with_lval:
            VALUE_TO_OBJECT(cx, -1, lval, obj);

          do_getprop_with_obj:
            do {
                JSPropCacheEntry *entry;

                if (JS_LIKELY(obj->map->ops->getProperty == js_GetProperty)) {
                    PROPERTY_CACHE_TEST(cx, pc, obj, obj2, entry, atom);
                    if (!atom) {
                        ASSERT_VALID_PROPERTY_CACHE_HIT(i, obj, obj2, entry);
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
                SAVE_SP_AND_PC(fp);
                if (entry
                    ? !js_GetPropertyHelper(cx, obj, id, &rval, &entry)
                    : !OBJ_GET_PROPERTY(cx, obj, id, &rval)) {
                    goto error;
                }
            } while (0);

            STORE_OPND(-1, rval);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_LENGTH)
            lval = FETCH_OPND(-1);
            if (JSVAL_IS_STRING(lval)) {
                str = JSVAL_TO_STRING(lval);
                rval = INT_TO_JSVAL(JSSTRING_LENGTH(str));
            } else if (!JSVAL_IS_PRIMITIVE(lval) &&
                       (obj = JSVAL_TO_OBJECT(lval), OBJ_IS_ARRAY(cx, obj))) {

                jsuint length;

                




                length = obj->fslots[JSSLOT_ARRAY_LENGTH];
                if (length <= JSVAL_INT_MAX) {
                    rval = INT_TO_JSVAL(length);
                } else {
                    if (!js_NewDoubleValue(cx, (jsdouble)length, &rval))
                        goto error;
                }
            } else {
                i = -1;
                len = JSOP_LENGTH_LENGTH;
                goto do_getprop_with_lval;
            }
            STORE_OPND(-1, rval);
          END_CASE(JSOP_LENGTH)

          BEGIN_CASE(JSOP_CALLPROP)
          {
            JSObject *aobj;
            JSPropCacheEntry *entry;

            lval = FETCH_OPND(-1);
            if (!JSVAL_IS_PRIMITIVE(lval)) {
                obj = JSVAL_TO_OBJECT(lval);
            } else {
                SAVE_SP_AND_PC(fp);

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
                PROPERTY_CACHE_TEST(cx, pc, aobj, obj2, entry, atom);
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
                    goto end_callname;
                }
            } else {
                entry = NULL;
                LOAD_ATOM(0);
            }

            



            id = ATOM_TO_JSID(atom);
            PUSH(JSVAL_NULL);
            SAVE_SP_AND_PC(fp);
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
                if (JS_LIKELY(aobj->map->ops->getProperty == js_GetProperty)
                    ? !js_GetPropertyHelper(cx, aobj, id, &rval, &entry)
                    : !OBJ_GET_PROPERTY(cx, obj, id, &rval)) {
                    goto error;
                }
                STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
                STORE_OPND(-2, rval);
                if (!ComputeThis(cx, JS_FALSE, sp))
                    goto error;
            } else {
                JS_ASSERT(obj->map->ops->getProperty == js_GetProperty);
                if (!js_GetPropertyHelper(cx, obj, id, &rval, &entry))
                    goto error;
                STORE_OPND(-1, lval);
                STORE_OPND(-2, rval);
            }

          end_callname:
            
            if (JSVAL_IS_PRIMITIVE(lval)) {
                
                if (!VALUE_IS_FUNCTION(cx, rval) ||
                    (obj = JSVAL_TO_OBJECT(rval),
                     fun = GET_FUNCTION_PRIVATE(cx, obj),
                     !PRIMITIVE_THIS_TEST(fun, lval))) {
                    if (!js_PrimitiveToObject(cx, &sp[-1]))
                        goto error;
                }
            }
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
                    uint32 kshape = OBJ_SCOPE(obj)->shape;

                    


















                    entry = &cache->table[PROPERTY_CACHE_HASH_PC(pc, kshape)];
                    PCMETER(cache->tests++);
                    PCMETER(cache->settests++);
                    if (entry->kpc == pc && entry->kshape == kshape) {
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
                                    break;
                                }
                            } else {
                                scope = js_GetMutableScope(cx, obj);
                                if (!scope)
                                    goto error;
                            }

                            if (sprop->parent == scope->lastProp &&
                                !SCOPE_HAD_MIDDLE_DELETE(scope) &&
                                !scope->table &&
                                SPROP_HAS_STUB_SETTER(sprop) &&
                                (slot = sprop->slot) == scope->map.freeslot &&
                                slot < STOBJ_NSLOTS(obj)) {
                                










                                JS_ASSERT(!(LOCKED_OBJ_GET_CLASS(obj)->flags &
                                            JSCLASS_SHARE_ALL_PROPERTIES));

                                PCMETER(cache->pchits++);
                                PCMETER(cache->addpchits++);
                                ++scope->map.freeslot;
                                if (!scope->lastProp ||
                                    scope->shape == scope->lastProp->shape) {
                                    scope->shape = sprop->shape;
                                } else {
                                    SCOPE_GENERATE_PCTYPE(cx, scope);
                                }
                                ++scope->entryCount;
                                scope->lastProp = sprop;
                                GC_WRITE_BARRIER(cx, scope,
                                                 LOCKED_OBJ_GET_SLOT(obj, slot),
                                                 rval);
                                LOCKED_OBJ_SET_SLOT(obj, slot, rval);
                                JS_UNLOCK_SCOPE(cx, scope);
                                break;
                            }

                            PCMETER(cache->setpcmisses++);
                            atom = NULL;
                        }

                        JS_UNLOCK_OBJ(cx, obj);
                    }

                    atom = js_FullTestPropertyCache(cx, pc, &obj, &obj2,
                                                    &entry);
                    if (atom) {
                        PCMETER(cache->misses++);
                        PCMETER(cache->setmisses++);
                    } else {
                        ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
                        if (obj == obj2) {
                            if (PCVAL_IS_SLOT(entry->vword)) {
                                slot = PCVAL_TO_SLOT(entry->vword);
                                JS_ASSERT(slot < obj->map->freeslot);
                                LOCKED_OBJ_WRITE_BARRIER(cx, obj, slot, rval);
                            } else if (PCVAL_IS_SPROP(entry->vword)) {
                                sprop = PCVAL_TO_SPROP(entry->vword);
                                JS_ASSERT(!(sprop->attrs & JSPROP_READONLY));
                                JS_ASSERT(!SCOPE_IS_SEALED(OBJ_SCOPE(obj2)));
                                NATIVE_SET(cx, obj, sprop, &rval);
                            }
                        }
                        JS_UNLOCK_OBJ(cx, obj2);
                        if (obj == obj2 && !PCVAL_IS_OBJECT(entry->vword))
                            break;
                    }
                }

                if (!atom)
                    LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                SAVE_SP_AND_PC(fp);
                if (entry
                    ? !js_SetPropertyHelper(cx, obj, id, &rval, &entry)
                    : !OBJ_SET_PROPERTY(cx, obj, id, &rval)) {
                    goto error;
                }
            } while (0);

            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETPROP)

          BEGIN_CASE(JSOP_GETELEM)
            ELEMENT_OP(-1, OBJ_GET_PROPERTY(cx, obj, id, &rval));
            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_CALLELEM)
            



            ELEMENT_OP(-1, OBJ_GET_PROPERTY(cx, obj, id, &rval));
            STORE_OPND(-2, rval);
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_CALLELEM)

          BEGIN_CASE(JSOP_SETELEM)
            rval = FETCH_OPND(-1);
            ELEMENT_OP(-2, OBJ_SET_PROPERTY(cx, obj, id, &rval));
            sp -= 2;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETELEM)

          BEGIN_CASE(JSOP_ENUMELEM)
            
            rval = FETCH_OPND(-3);
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -2, lval, obj);
            FETCH_ELEMENT_ID(obj, -1, id);
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
            sp -= 3;
          END_CASE(JSOP_ENUMELEM)

          BEGIN_CASE(JSOP_CALL)
          BEGIN_CASE(JSOP_EVAL)
            argc = GET_ARGC(pc);
            vp = sp - (argc + 2);
            lval = *vp;
            SAVE_SP_AND_PC(fp);
            if (VALUE_IS_FUNCTION(cx, lval)) {
                obj = JSVAL_TO_OBJECT(lval);
                fun = GET_FUNCTION_PRIVATE(cx, obj);

                if (FUN_INTERPRETED(fun)) {
                    uintN nframeslots, nvars, nslots, missing;
                    JSArena *a;
                    jsuword avail, nbytes;
                    void *newmark;
                    jsval *newsp, *rvp;
                    JSBool overflow;
                    JSInlineFrame *newifp;
                    JSInterpreterHook hook;

                    
                    nframeslots = JS_HOWMANY(sizeof(JSInlineFrame),
                                             sizeof(jsval));
                    nvars = fun->u.i.nvars;
                    script = fun->u.i.script;
                    depth = (jsint) script->depth;
                    atoms = script->atomMap.vector;
                    nslots = nframeslots + nvars + 2 * depth;

                    
                    missing = (fun->nargs > argc) ? fun->nargs - argc : 0;
                    a = cx->stackPool.current;
                    avail = a->avail;
                    newmark = (void *) avail;
                    if (missing) {
                        newsp = sp + missing;
                        overflow = (jsuword) newsp > a->limit;
                        if (overflow)
                            nslots += 2 + argc + missing;
                        else if ((jsuword) newsp > avail)
                            avail = a->avail = (jsuword) newsp;
                    }
#ifdef __GNUC__
                    else overflow = JS_FALSE; 
#endif

                    
                    newsp = (jsval *) avail;
                    nbytes = nslots * sizeof(jsval);
                    avail += nbytes;
                    if (avail <= a->limit) {
                        a->avail = avail;
                    } else {
                        JS_ARENA_ALLOCATE_CAST(newsp, jsval *, &cx->stackPool,
                                               nbytes);
                        if (!newsp) {
                            js_ReportOutOfScriptQuota(cx);
                            goto bad_inline_call;
                        }
                    }

                    



                    rvp = vp;
                    if (missing) {
                        if (overflow) {
                            memcpy(newsp, vp, (2 + argc) * sizeof(jsval));
                            vp = newsp;
                            sp = vp + 2 + argc;
                            newsp = sp + missing;
                        }
                        do {
                            PUSH(JSVAL_VOID);
                        } while (--missing != 0);
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
                    newifp->frame.nvars = nvars;
                    newifp->frame.vars = newsp;
                    newifp->frame.down = fp;
                    newifp->frame.annotation = NULL;
                    newifp->frame.scopeChain = parent = OBJ_GET_PARENT(cx, obj);
                    newifp->frame.sharpDepth = 0;
                    newifp->frame.sharpArray = NULL;
                    newifp->frame.flags = 0;
                    newifp->frame.dormantNext = NULL;
                    newifp->frame.xmlNamespace = NULL;
                    newifp->frame.blockChain = NULL;
#ifdef DEBUG
                    newifp->frame.pcDisabledSave =
                        JS_PROPERTY_CACHE(cx).disabled;
#endif
                    newifp->rvp = rvp;
                    newifp->mark = newmark;

                    
                    JS_ASSERT(!JSFUN_BOUND_METHOD_TEST(fun->flags));
                    JS_ASSERT(JSVAL_IS_OBJECT(vp[1]));
                    newifp->frame.thisp = (JSObject *)vp[1];

                    
                    sp = newsp;
                    while (nvars--)
                        PUSH(JSVAL_VOID);
                    sp += depth;
                    newifp->frame.spbase = sp;
                    SAVE_SP(&newifp->frame);

                    
                    hook = cx->debugHooks->callHook;
                    if (hook) {
                        newifp->frame.pc = NULL;
                        newifp->hookData = hook(cx, &newifp->frame, JS_TRUE, 0,
                                                cx->debugHooks->callHookData);
                        LOAD_INTERRUPT_HANDLER(cx);
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

                    
                    cx->fp = fp = &newifp->frame;
                    pc = script->code;
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

                    
                    op = (JSOp) *pc;
                    DO_OP();

                  bad_inline_call:
                    RESTORE_SP(fp);
                    JS_ASSERT(fp->pc == pc);
                    script = fp->script;
                    depth = (jsint) script->depth;
                    atoms = script->atomMap.vector;
                    FreeRawStack(cx, newmark);
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
                    if (argc < fun->u.n.minargs) {
                        uintN nargs;

                        



                        nargs = fun->u.n.minargs - argc;
                        if (sp + nargs > fp->spbase + depth)
                            goto do_invoke;
                        do {
                            




                            PUSH_OPND(JSVAL_VOID);
                        } while (--nargs != 0);
                        SAVE_SP(fp);
                    }

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
                    if (!ok)
                        goto error;
                    sp = vp + 1;
                    vp[-depth] = (jsval)pc;
                    goto end_call;
                }
            }

          do_invoke:
            ok = js_Invoke(cx, argc, vp, 0);
#ifdef INCLUDE_MOZILLA_DTRACE
            
            if (VALUE_IS_FUNCTION(cx, lval)) {
                if (JAVASCRIPT_FUNCTION_RVAL_ENABLED())
                    jsdtrace_function_rval(cx, fp, fun);
                if (JAVASCRIPT_FUNCTION_RETURN_ENABLED())
                    jsdtrace_function_return(cx, fp, fun);
            }
#endif
            sp = vp + 1;
            vp[-depth] = (jsval)pc;
            LOAD_INTERRUPT_HANDLER(cx);
            if (!ok)
                goto error;
            JS_RUNTIME_METER(rt, nonInlineCalls);

          end_call:
#if JS_HAS_LVALUE_RETURN
            if (cx->rval2set) {
                













                PUSH_OPND(cx->rval2);
                ELEMENT_OP(-1, OBJ_GET_PROPERTY(cx, obj, id, &rval));

                sp--;
                STORE_OPND(-1, rval);
                cx->rval2set = JS_FALSE;
            }
#endif 
          END_CASE(JSOP_CALL)

#if JS_HAS_LVALUE_RETURN
          BEGIN_CASE(JSOP_SETCALL)
            argc = GET_ARGC(pc);
            SAVE_SP_AND_PC(fp);
            vp = sp - argc - 2;
            ok = js_Invoke(cx, argc, vp, 0);
            sp = vp + 1;
            vp[-depth] = (jsval)pc;
            LOAD_INTERRUPT_HANDLER(cx);
            if (!ok)
                goto error;
            if (!cx->rval2set) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_LEFTSIDE_OF_ASS);
                goto error;
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
                PROPERTY_CACHE_TEST(cx, pc, obj, obj2, entry, atom);
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
            SAVE_SP_AND_PC(fp);
            if (js_FindPropertyHelper(cx, id, &obj, &obj2, &prop, &entry) < 0)
                goto error;
            if (!prop) {
                
                len = JSOP_NAME_LENGTH;
                endpc = script->code + script->length;
                for (pc2 = pc + len; pc2 < endpc; pc2++) {
                    op2 = (JSOp)*pc2;
                    if (op2 == JSOP_TYPEOF) {
                        PUSH_OPND(JSVAL_VOID);
                        DO_NEXT_OP(len);
                    }
                    if (op2 != JSOP_GROUP)
                        break;
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
            if (op == JSOP_CALLNAME) {
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
                SAVE_SP(fp);
                if (!ComputeThis(cx, JS_FALSE, sp))
                    goto error;
            }
          }
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_UINT16)
            i = (jsint) GET_UINT16(pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_UINT16)

          BEGIN_CASE(JSOP_UINT24)
            i = (jsint) GET_UINT24(pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_UINT24)

          BEGIN_CASE(JSOP_INT8)
            i = GET_INT8(pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_INT8)

          BEGIN_CASE(JSOP_INT32)
            i = GET_INT32(pc);
            rval = INT_TO_JSVAL(i);
            PUSH_OPND(rval);
          END_CASE(JSOP_INT32)

          BEGIN_CASE(JSOP_INDEXBASE)
            



            atoms += GET_INDEXBASE(pc);
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
                if (!JS_GetReservedSlot(cx, funobj, slot, &rval))
                    return JS_FALSE;
                if (JSVAL_IS_VOID(rval))
                    rval = JSVAL_NULL;
            } else {
                





                slot += script->ngvars;
                rval = fp->vars[slot];
#ifdef __GNUC__
                funobj = NULL;  
#endif
            }

            if (JSVAL_IS_NULL(rval)) {
                
                obj2 = fp->scopeChain;
                while ((parent = OBJ_GET_PARENT(cx, obj2)) != NULL)
                    obj2 = parent;

                






                SAVE_SP_AND_PC(fp);

                























                JS_GET_SCRIPT_REGEXP(script, index, obj);
                if (OBJ_GET_PARENT(cx, obj) != obj2) {
                    obj = js_CloneRegExpObject(cx, obj, obj2);
                    if (!obj)
                        goto error;
                }
                rval = OBJECT_TO_JSVAL(obj);

                
                if (fp->fun) {
                    if (!JS_SetReservedSlot(cx, funobj, slot, rval))
                        return JS_FALSE;
                } else {
                    fp->vars[slot] = rval;
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
            PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_NULL)

          BEGIN_CASE(JSOP_THIS)
            obj = fp->thisp;
            if (!obj) {
                obj = ComputeGlobalThis(cx, JS_TRUE, fp->argv);
                if (!obj)
                    goto error;
                fp->thisp = obj;
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_THIS)

          BEGIN_CASE(JSOP_FALSE)
            PUSH_OPND(JSVAL_FALSE);
          END_CASE(JSOP_FALSE)

          BEGIN_CASE(JSOP_TRUE)
            PUSH_OPND(JSVAL_TRUE);
          END_CASE(JSOP_TRUE)

          BEGIN_CASE(JSOP_TABLESWITCH)
            pc2 = pc;
            len = GET_JUMP_OFFSET(pc2);

            




            rval = POP_OPND();
            if (!JSVAL_IS_INT(rval))
                DO_NEXT_OP(len);
            i = JSVAL_TO_INT(rval);

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
            pc2 = pc;
            len = GET_JUMPX_OFFSET(pc2);

            




            rval = POP_OPND();
            if (!JSVAL_IS_INT(rval))
                DO_NEXT_OP(len);
            i = JSVAL_TO_INT(rval);

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
            pc2 = pc;
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
            pc2 = pc;                                                         \
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

          EMPTY_CASE(JSOP_CONDSWITCH)

#if JS_HAS_EXPORT_IMPORT
          BEGIN_CASE(JSOP_EXPORTALL)
            obj = fp->varobj;
            SAVE_SP_AND_PC(fp);
            ida = JS_Enumerate(cx, obj);
            if (!ida)
                goto error;
            ok = JS_TRUE;
            for (i = 0; i != ida->length; i++) {
                id = ida->vector[i];
                ok = OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop);
                if (!ok)
                    break;
                if (!prop)
                    continue;
                ok = OBJ_GET_ATTRIBUTES(cx, obj, id, prop, &attrs);
                if (ok) {
                    attrs |= JSPROP_EXPORTED;
                    ok = OBJ_SET_ATTRIBUTES(cx, obj, id, prop, &attrs);
                }
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                if (!ok)
                    break;
            }
            JS_ASSERT(ok == (i == ida->length));
            JS_DestroyIdArray(cx, ida);
            if (!ok)
                goto error;
          END_CASE(JSOP_EXPORTALL)

          BEGIN_CASE(JSOP_EXPORTNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            obj = fp->varobj;
            SAVE_SP_AND_PC(fp);
            if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
                goto error;
            if (!prop) {
                if (!OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                                         JS_PropertyStub, JS_PropertyStub,
                                         JSPROP_EXPORTED, NULL)) {
                    goto error;
                }
            } else {
                ok = OBJ_GET_ATTRIBUTES(cx, obj, id, prop, &attrs);
                if (ok) {
                    attrs |= JSPROP_EXPORTED;
                    ok = OBJ_SET_ATTRIBUTES(cx, obj, id, prop, &attrs);
                }
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                if (!ok)
                    goto error;
            }
          END_CASE(JSOP_EXPORTNAME)

          BEGIN_CASE(JSOP_IMPORTALL)
            id = (jsid) JSVAL_VOID;
            PROPERTY_OP(-1, ImportProperty(cx, obj, id));
            sp--;
          END_CASE(JSOP_IMPORTALL)

          BEGIN_CASE(JSOP_IMPORTPROP)
            
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            PROPERTY_OP(-1, ImportProperty(cx, obj, id));
            sp--;
          END_CASE(JSOP_IMPORTPROP)

          BEGIN_CASE(JSOP_IMPORTELEM)
            ELEMENT_OP(-1, ImportProperty(cx, obj, id));
            sp -= 2;
          END_CASE(JSOP_IMPORTELEM)
#endif 

          BEGIN_CASE(JSOP_TRAP)
            SAVE_SP_AND_PC(fp);
            switch (JS_HandleTrap(cx, script, pc, &rval)) {
              case JSTRAP_ERROR:
                goto error;
              case JSTRAP_CONTINUE:
                JS_ASSERT(JSVAL_IS_INT(rval));
                op = (JSOp) JSVAL_TO_INT(rval);
                JS_ASSERT((uintN)op < (uintN)JSOP_LIMIT);
                LOAD_INTERRUPT_HANDLER(cx);
                DO_OP();
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
            LOAD_INTERRUPT_HANDLER(cx);
          END_CASE(JSOP_TRAP)

          BEGIN_CASE(JSOP_ARGUMENTS)
            SAVE_SP_AND_PC(fp);
            if (!js_GetArgsValue(cx, fp, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_ARGSUB)
            id = INT_TO_JSID(GET_ARGNO(pc));
            SAVE_SP_AND_PC(fp);
            if (!js_GetArgsProperty(cx, fp, id, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
            id = ATOM_TO_JSID(rt->atomState.lengthAtom);
            SAVE_SP_AND_PC(fp);
            if (!js_GetArgsProperty(cx, fp, id, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGCNT)

          BEGIN_CASE(JSOP_GETARG)
          BEGIN_CASE(JSOP_CALLARG)
            slot = GET_ARGNO(pc);
            JS_ASSERT(slot < fp->fun->nargs);
            METER_SLOT_OP(op, slot);
            PUSH_OPND(fp->argv[slot]);
            if (op == JSOP_CALLARG)
                PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_SETARG)
            slot = GET_ARGNO(pc);
            JS_ASSERT(slot < fp->fun->nargs);
            METER_SLOT_OP(op, slot);
            vp = &fp->argv[slot];
            GC_POKE(cx, *vp);
            *vp = FETCH_OPND(-1);
          END_CASE(JSOP_SETARG)

          BEGIN_CASE(JSOP_GETVAR)
          BEGIN_CASE(JSOP_CALLVAR)
            slot = GET_VARNO(pc);
            JS_ASSERT(slot < fp->fun->u.i.nvars);
            METER_SLOT_OP(op, slot);
            PUSH_OPND(fp->vars[slot]);
            if (op == JSOP_CALLVAR)
                PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_GETVAR)

          BEGIN_CASE(JSOP_SETVAR)
            slot = GET_VARNO(pc);
            JS_ASSERT(slot < fp->fun->u.i.nvars);
            METER_SLOT_OP(op, slot);
            vp = &fp->vars[slot];
            GC_POKE(cx, *vp);
            *vp = FETCH_OPND(-1);
          END_CASE(JSOP_SETVAR)

          BEGIN_CASE(JSOP_GETGVAR)
          BEGIN_CASE(JSOP_CALLGVAR)
            slot = GET_VARNO(pc);
            JS_ASSERT(slot < fp->nvars);
            METER_SLOT_OP(op, slot);
            lval = fp->vars[slot];
            if (JSVAL_IS_NULL(lval)) {
                op = (op == JSOP_GETGVAR) ? JSOP_NAME : JSOP_CALLNAME;
                DO_OP();
            }
            slot = JSVAL_TO_INT(lval);
            obj = fp->varobj;
            rval = OBJ_GET_SLOT(cx, obj, slot);
            PUSH_OPND(rval);
            if (op == JSOP_CALLGVAR)
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_GETGVAR)

          BEGIN_CASE(JSOP_SETGVAR)
            slot = GET_VARNO(pc);
            JS_ASSERT(slot < fp->nvars);
            METER_SLOT_OP(op, slot);
            rval = FETCH_OPND(-1);
            lval = fp->vars[slot];
            obj = fp->varobj;
            if (JSVAL_IS_NULL(lval)) {
                




                LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                SAVE_SP_AND_PC(fp);
                if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                    goto error;
                STORE_OPND(-1, rval);
            } else {
                slot = JSVAL_TO_INT(lval);
                JS_LOCK_OBJ(cx, obj);
                LOCKED_OBJ_WRITE_BARRIER(cx, obj, slot, rval);
                JS_UNLOCK_OBJ(cx, obj);
            }
          END_CASE(JSOP_SETGVAR)

          BEGIN_CASE(JSOP_DEFCONST)
          BEGIN_CASE(JSOP_DEFVAR)
            index = GET_INDEX(pc);
            atom = atoms[index];

            



            index += atoms - script->atomMap.vector;
            obj = fp->varobj;
            attrs = JSPROP_ENUMERATE;
            if (!(fp->flags & JSFRAME_EVAL))
                attrs |= JSPROP_PERMANENT;
            if (op == JSOP_DEFCONST)
                attrs |= JSPROP_READONLY;

            
            id = ATOM_TO_JSID(atom);
            SAVE_SP_AND_PC(fp);
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

            





            if (index < script->ngvars &&
                (attrs & JSPROP_PERMANENT) &&
                obj2 == obj &&
                OBJ_IS_NATIVE(obj)) {
                sprop = (JSScopeProperty *) prop;
                if (SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(obj)) &&
                    SPROP_HAS_STUB_GETTER(sprop) &&
                    SPROP_HAS_STUB_SETTER(sprop)) {
                    





                    fp->vars[index] = INT_TO_JSVAL(sprop->slot);
                }
            }

            OBJ_DROP_PROPERTY(cx, obj2, prop);
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_DEFFUN)
            LOAD_FUNCTION(0);

            
























            JS_ASSERT(!fp->blockChain);
            JS_ASSERT((fp->flags & JSFRAME_EVAL) == 0);
            JS_ASSERT(fp->scopeChain == fp->varobj);
            obj2 = fp->scopeChain;

            



            attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT;
            SAVE_SP_AND_PC(fp);

          do_deffun:
            
            ASSERT_SAVED_SP_AND_PC(fp);

            






            if (OBJ_GET_PARENT(cx, obj) != obj2) {
                obj = js_CloneFunctionObject(cx, obj, obj2);
                if (!obj)
                    goto error;
            }

            




            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            




            fun = GET_FUNCTION_PRIVATE(cx, obj);
            flags = JSFUN_GSFLAG2ATTR(fun->flags);
            if (flags) {
                attrs |= flags | JSPROP_SHARED;
                rval = JSVAL_VOID;
            }

            





            parent = fp->varobj;

            





            id = ATOM_TO_JSID(fun->atom);
            ok = js_CheckRedeclaration(cx, parent, id, attrs, NULL, NULL);
            if (ok) {
                if (attrs == JSPROP_ENUMERATE) {
                    JS_ASSERT(fp->flags & JSFRAME_EVAL);
                    JS_ASSERT(op == JSOP_CLOSURE);
                    ok = OBJ_SET_PROPERTY(cx, parent, id, &rval);
                } else {
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

            
            fp->scopeChain = obj2;
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                goto error;
            }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
            LOAD_FUNCTION(VARNO_LEN);

            






            slot = GET_VARNO(pc);

            parent = js_GetScopeChain(cx, fp);
            if (!parent)
                goto error;

            SAVE_SP_AND_PC(fp);
            obj = js_CloneFunctionObject(cx, obj, parent);
            if (!obj)
                goto error;

            fp->vars[slot] = OBJECT_TO_JSVAL(obj);
          END_CASE(JSOP_DEFLOCALFUN)

          BEGIN_CASE(JSOP_ANONFUNOBJ)
            
            LOAD_FUNCTION(0);

            
            SAVE_SP_AND_PC(fp);
            parent = js_GetScopeChain(cx, fp);
            if (!parent)
                goto error;
            if (OBJ_GET_PARENT(cx, obj) != parent) {
                obj = js_CloneFunctionObject(cx, obj, parent);
                if (!obj)
                    goto error;
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_ANONFUNOBJ)

          BEGIN_CASE(JSOP_NAMEDFUNOBJ)
            
            LOAD_FUNCTION(0);
            rval = OBJECT_TO_JSVAL(obj);

            







            SAVE_SP_AND_PC(fp);
            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2)
                goto error;
            parent = js_NewObject(cx, &js_ObjectClass, NULL, obj2);
            if (!parent)
                goto error;

            













            fp->scopeChain = parent;
            obj = js_CloneFunctionObject(cx, JSVAL_TO_OBJECT(rval), parent);
            if (!obj)
                goto error;

            




            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            




            fun = GET_FUNCTION_PRIVATE(cx, obj);
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

            
            fp->scopeChain = obj2;
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                goto error;
            }

            



            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_NAMEDFUNOBJ)

          BEGIN_CASE(JSOP_CLOSURE)
            





            LOAD_FUNCTION(0);

            





            SAVE_SP_AND_PC(fp);
            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2)
                goto error;

            



            attrs = JSPROP_ENUMERATE;
            if (!(fp->flags & JSFRAME_EVAL))
                attrs |= JSPROP_PERMANENT;

            goto do_deffun;

#if JS_HAS_GETTER_SETTER
          BEGIN_CASE(JSOP_GETTER)
          BEGIN_CASE(JSOP_SETTER)
          do_getter_setter:
            op2 = (JSOp) *++pc;
            switch (op2) {
              case JSOP_INDEXBASE:
                atoms += GET_INDEXBASE(pc);
                pc += JSOP_INDEXBASE_LENGTH - 1;
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
                SAVE_SP_AND_PC(fp);
                FETCH_OBJECT(cx, i - 1, lval, obj);
                break;

              case JSOP_INITPROP:
                JS_ASSERT(sp - fp->spbase >= 2);
                rval = FETCH_OPND(-1);
                i = -1;
                LOAD_ATOM(0);
                id = ATOM_TO_JSID(atom);
                goto gs_get_lval;

              default:
                JS_ASSERT(op2 == JSOP_INITELEM);

                JS_ASSERT(sp - fp->spbase >= 3);
                rval = FETCH_OPND(-1);
                id = 0;
                i = -2;
              gs_get_lval:
                lval = FETCH_OPND(i-1);
                JS_ASSERT(JSVAL_IS_OBJECT(lval));
                obj = JSVAL_TO_OBJECT(lval);
                SAVE_SP_AND_PC(fp);
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

            sp += i;
            if (js_CodeSpec[op2].ndefs)
                STORE_OPND(-1, rval);
            len = js_CodeSpec[op2].length;
            DO_NEXT_OP(len);
#endif 

          BEGIN_CASE(JSOP_NEWINIT)
            i = GET_INT8(pc);
            JS_ASSERT(i == JSProto_Array || i == JSProto_Object);
            SAVE_SP_AND_PC(fp);
            obj = (i == JSProto_Array)
                  ? js_NewArrayObject(cx, 0, NULL)
                  : js_NewObject(cx, &js_ObjectClass, NULL, NULL);
            if (!obj)
                goto error;
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
            fp->sharpDepth++;
            LOAD_INTERRUPT_HANDLER(cx);
          END_CASE(JSOP_NEWINIT)

          BEGIN_CASE(JSOP_ENDINIT)
            if (--fp->sharpDepth == 0)
                fp->sharpArray = NULL;

            
            JS_ASSERT(sp - fp->spbase >= 1);
            lval = FETCH_OPND(-1);
            JS_ASSERT(JSVAL_IS_OBJECT(lval));
            cx->weakRoots.newborn[GCX_OBJECT] = JSVAL_TO_GCTHING(lval);
          END_CASE(JSOP_ENDINIT)

          BEGIN_CASE(JSOP_INITPROP)
            
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            
            JS_ASSERT(sp - fp->spbase >= 2);
            rval = FETCH_OPND(-1);
            i = -1;
            SAVE_SP_AND_PC(fp);
            goto do_init;

          BEGIN_CASE(JSOP_INITELEM)
            
            JS_ASSERT(sp - fp->spbase >= 3);
            rval = FETCH_OPND(-1);
            i = -2;
            SAVE_SP_AND_PC(fp);
            FETCH_ELEMENT_ID(obj, -2, id);

          do_init:
            
            lval = FETCH_OPND(i-1);
            JS_ASSERT(JSVAL_IS_OBJECT(lval));
            obj = JSVAL_TO_OBJECT(lval);

            
            if (!js_CheckRedeclaration(cx, obj, id, JSPROP_INITIALIZER, NULL,
                                       NULL)) {
                goto error;
            }
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
            sp += i;
            len = js_CodeSpec[op].length;
            DO_NEXT_OP(len);

#if JS_HAS_SHARP_VARS
          BEGIN_CASE(JSOP_DEFSHARP)
            SAVE_SP_AND_PC(fp);
            obj = fp->sharpArray;
            if (!obj) {
                obj = js_NewArrayObject(cx, 0, NULL);
                if (!obj)
                    goto error;
                fp->sharpArray = obj;
            }
            i = (jsint) GET_UINT16(pc);
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
            i = (jsint) GET_UINT16(pc);
            id = INT_TO_JSID(i);
            obj = fp->sharpArray;
            if (!obj) {
                rval = JSVAL_VOID;
            } else {
                SAVE_SP_AND_PC(fp);
                if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                    goto error;
            }
            if (!JSVAL_IS_OBJECT(rval)) {
                char numBuf[12];
                JS_snprintf(numBuf, sizeof numBuf, "%u", (unsigned) i);

                SAVE_SP_AND_PC(fp);
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_SHARP_USE, numBuf);
                goto error;
            }
            PUSH_OPND(rval);
          END_CASE(JSOP_USESHARP)
#endif 

          
          EMPTY_CASE(JSOP_TRY)
          EMPTY_CASE(JSOP_FINALLY)

          BEGIN_CASE(JSOP_GOSUB)
            PUSH(JSVAL_FALSE);
            i = PTRDIFF(pc, script->main, jsbytecode) + JSOP_GOSUB_LENGTH;
            len = GET_JUMP_OFFSET(pc);
            PUSH(INT_TO_JSVAL(i));
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_GOSUBX)
            PUSH(JSVAL_FALSE);
            i = PTRDIFF(pc, script->main, jsbytecode) + JSOP_GOSUBX_LENGTH;
            len = GET_JUMPX_OFFSET(pc);
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
            pc = script->main;
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
            



            JS_ASSERT(sp - fp->spbase >= 2);
            slot = GET_UINT16(pc);
            JS_ASSERT(slot + 1 < (uintN)depth);
            fp->spbase[slot] = POP_OPND();
          END_CASE(JSOP_SETLOCALPOP)

          BEGIN_CASE(JSOP_INSTANCEOF)
            SAVE_SP_AND_PC(fp);
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
            sp--;
            STORE_OPND(-1, BOOLEAN_TO_JSVAL(cond));
          END_CASE(JSOP_INSTANCEOF)

#if JS_HAS_DEBUGGER_KEYWORD
          BEGIN_CASE(JSOP_DEBUGGER)
          {
            JSTrapHandler handler = cx->debugHooks->debuggerHandler;
            if (handler) {
                SAVE_SP_AND_PC(fp);
                switch (handler(cx, script, pc, &rval,
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
                LOAD_INTERRUPT_HANDLER(cx);
            }
          }
          END_CASE(JSOP_DEBUGGER)
#endif 

#if JS_HAS_XML_SUPPORT
          BEGIN_CASE(JSOP_DEFXMLNS)
            rval = POP();
            SAVE_SP_AND_PC(fp);
            if (!js_SetDefaultXMLNamespace(cx, rval))
                goto error;
          END_CASE(JSOP_DEFXMLNS)

          BEGIN_CASE(JSOP_ANYNAME)
            SAVE_SP_AND_PC(fp);
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
            SAVE_SP_AND_PC(fp);
            obj = js_ConstructXMLQNameObject(cx, lval, rval);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_QNAMECONST)

          BEGIN_CASE(JSOP_QNAME)
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
            SAVE_SP_AND_PC(fp);
            obj = js_ConstructXMLQNameObject(cx, lval, rval);
            if (!obj)
                goto error;
            sp--;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_QNAME)

          BEGIN_CASE(JSOP_TOATTRNAME)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            if (!js_ToAttributeName(cx, &rval))
                goto error;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_TOATTRNAME)

          BEGIN_CASE(JSOP_TOATTRVAL)
            rval = FETCH_OPND(-1);
            JS_ASSERT(JSVAL_IS_STRING(rval));
            SAVE_SP_AND_PC(fp);
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
            SAVE_SP_AND_PC(fp);
            str = js_AddAttributePart(cx, op == JSOP_ADDATTRNAME, str, str2);
            if (!str)
                goto error;
            sp--;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_ADDATTRNAME)

          BEGIN_CASE(JSOP_BINDXMLNAME)
            lval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            if (!js_FindXMLProperty(cx, lval, &obj, &id))
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
            PUSH_OPND(ID_TO_VALUE(id));
          END_CASE(JSOP_BINDXMLNAME)

          BEGIN_CASE(JSOP_SETXMLNAME)
            obj = JSVAL_TO_OBJECT(FETCH_OPND(-3));
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            FETCH_ELEMENT_ID(obj, -2, id);
            if (!OBJ_SET_PROPERTY(cx, obj, id, &rval))
                goto error;
            sp -= 2;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETXMLNAME)

          BEGIN_CASE(JSOP_CALLXMLNAME)
          BEGIN_CASE(JSOP_XMLNAME)
            lval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            if (!js_FindXMLProperty(cx, lval, &obj, &id))
                goto error;
            if (!OBJ_GET_PROPERTY(cx, obj, id, &rval))
                goto error;
            STORE_OPND(-1, rval);
            if (op == JSOP_CALLXMLNAME) {
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
                SAVE_SP(fp);
                if (!ComputeThis(cx, JS_FALSE, sp))
                    goto error;
            }
          END_CASE(JSOP_XMLNAME)

          BEGIN_CASE(JSOP_DESCENDANTS)
          BEGIN_CASE(JSOP_DELDESC)
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -2, lval, obj);
            rval = FETCH_OPND(-1);
            if (!js_GetXMLDescendants(cx, obj, rval, &rval))
                goto error;

            if (op == JSOP_DELDESC) {
                sp[-1] = rval;          
                if (!js_DeleteXMLListElements(cx, JSVAL_TO_OBJECT(rval)))
                    goto error;
                rval = JSVAL_TRUE;      
            }

            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DESCENDANTS)

          BEGIN_CASE(JSOP_FILTER)
            




            PUSH_OPND(JSVAL_HOLE);
            len = GET_JUMP_OFFSET(pc);
            JS_ASSERT(len > 0);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_ENDFILTER)
            SAVE_SP_AND_PC(fp);
            cond = (sp[-1] != JSVAL_HOLE);
            if (cond) {
                
                LeaveWith(cx);
            }
            if (!js_StepXMLListFilter(cx, cond))
                goto error;
            if (sp[-1] != JSVAL_NULL) {
                



                JS_ASSERT(VALUE_IS_XML(cx, sp[-1]));
                if (!EnterWith(cx, -2))
                    goto error;
                sp--;
                len = GET_JUMP_OFFSET(pc);
                JS_ASSERT(len < 0);
                CHECK_BRANCH(len);
                DO_NEXT_OP(len);
            }
            sp--;
          END_CASE(JSOP_ENDFILTER);

          EMPTY_CASE(JSOP_STARTXML)
          EMPTY_CASE(JSOP_STARTXMLEXPR)

          BEGIN_CASE(JSOP_TOXML)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            obj = js_ValueToXMLObject(cx, rval);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_TOXML)

          BEGIN_CASE(JSOP_TOXMLLIST)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            obj = js_ValueToXMLListObject(cx, rval);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_TOXMLLIST)

          BEGIN_CASE(JSOP_XMLTAGEXPR)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            str = js_ValueToString(cx, rval);
            if (!str)
                goto error;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_XMLTAGEXPR)

          BEGIN_CASE(JSOP_XMLELTEXPR)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
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
            SAVE_SP_AND_PC(fp);
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
            SAVE_SP_AND_PC(fp);
            obj = js_NewXMLSpecialObject(cx,
                                         JSXML_CLASS_PROCESSING_INSTRUCTION,
                                         str, str2);
            if (!obj)
                goto error;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLPI)

          BEGIN_CASE(JSOP_GETFUNNS)
            SAVE_SP_AND_PC(fp);
            if (!js_GetFunctionNamespace(cx, &rval))
                goto error;
            PUSH_OPND(rval);
          END_CASE(JSOP_GETFUNNS)
#endif 

          BEGIN_CASE(JSOP_ENTERBLOCK)
            LOAD_OBJECT(0);
            JS_ASSERT(fp->spbase + OBJ_BLOCK_DEPTH(cx, obj) == sp);
            vp = sp + OBJ_BLOCK_COUNT(cx, obj);
            JS_ASSERT(vp <= fp->spbase + depth);
            while (sp < vp) {
                STORE_OPND(0, JSVAL_VOID);
                sp++;
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
            jsval *blocksp = fp->spbase + OBJ_BLOCK_DEPTH(cx,
                                                          fp->blockChain
                                                          ? fp->blockChain
                                                          : fp->scopeChain);

            JS_ASSERT(fp->spbase <= blocksp && blocksp <= fp->spbase + depth);
#endif
            if (fp->blockChain) {
                JS_ASSERT(OBJ_GET_CLASS(cx, fp->blockChain) == &js_BlockClass);
                fp->blockChain = OBJ_GET_PARENT(cx, fp->blockChain);
            } else {
                



                SAVE_SP_AND_PC(fp);
                if (!js_PutBlockObject(cx, JS_TRUE))
                    goto error;
            }

            



            if (op == JSOP_LEAVEBLOCKEXPR)
                rval = FETCH_OPND(-1);
            sp -= GET_UINT16(pc);
            if (op == JSOP_LEAVEBLOCKEXPR) {
                JS_ASSERT(blocksp == sp - 1);
                STORE_OPND(-1, rval);
            } else {
                JS_ASSERT(blocksp == sp);
            }
          }
          END_CASE(JSOP_LEAVEBLOCK)

          BEGIN_CASE(JSOP_GETLOCAL)
          BEGIN_CASE(JSOP_CALLLOCAL)
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            PUSH_OPND(fp->spbase[slot]);
            if (op == JSOP_CALLLOCAL)
                PUSH_OPND(JSVAL_NULL);
          END_CASE(JSOP_GETLOCAL)

          BEGIN_CASE(JSOP_SETLOCAL)
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            vp = &fp->spbase[slot];
            GC_POKE(cx, *vp);
            *vp = FETCH_OPND(-1);
          END_CASE(JSOP_SETLOCAL)


#define FAST_LOCAL_INCREMENT_OP(PRE,OPEQ,MINMAX)                              \
    slot = GET_UINT16(pc);                                                    \
    JS_ASSERT(slot < (uintN)depth);                                           \
    vp = fp->spbase + slot;                                                   \
    rval = *vp;                                                               \
    if (!JSVAL_IS_INT(rval) || rval == INT_TO_JSVAL(JSVAL_INT_##MINMAX))      \
        goto do_nonint_fast_incop;                                            \
    PRE = rval;                                                               \
    rval OPEQ 2;                                                              \
    *vp = rval;                                                               \
    PUSH_OPND(PRE)

          BEGIN_CASE(JSOP_INCLOCAL)
            FAST_LOCAL_INCREMENT_OP(rval, +=, MAX);
          END_CASE(JSOP_INCLOCAL)

          BEGIN_CASE(JSOP_DECLOCAL)
            FAST_LOCAL_INCREMENT_OP(rval, -=, MIN);
          END_CASE(JSOP_DECLOCAL)

          BEGIN_CASE(JSOP_LOCALINC)
            FAST_LOCAL_INCREMENT_OP(rtmp, +=, MAX);
          END_CASE(JSOP_LOCALINC)

          BEGIN_CASE(JSOP_LOCALDEC)
            FAST_LOCAL_INCREMENT_OP(rtmp, -=, MIN);
          END_CASE(JSOP_LOCALDEC)

#undef FAST_LOCAL_INCREMENT_OP

          BEGIN_CASE(JSOP_ENDITER)
            



            SAVE_SP_AND_PC(fp);
            ok = js_CloseIterator(cx, sp[-1]);
            sp--;
            if (!ok)
                goto error;
          END_CASE(JSOP_ENDITER)

#if JS_HAS_GENERATORS
          BEGIN_CASE(JSOP_GENERATOR)
            ASSERT_NOT_THROWING(cx);
            pc += JSOP_GENERATOR_LENGTH;
            SAVE_SP_AND_PC(fp);
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
                SAVE_SP_AND_PC(fp);
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_YIELD,
                                    JSDVG_SEARCH_STACK, fp->argv[-2], NULL);
                goto error;
            }
            fp->rval = FETCH_OPND(-1);
            fp->flags |= JSFRAME_YIELDING;
            pc += JSOP_YIELD_LENGTH;
            SAVE_SP_AND_PC(fp);
            JS_PROPERTY_CACHE(cx).disabled -= CountWithBlocks(cx, fp);
            JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled >= 0);
            ok = JS_TRUE;
            goto exit;

          BEGIN_CASE(JSOP_ARRAYPUSH)
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            lval = fp->spbase[slot];
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
            sp--;
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
          L_JSOP_FOREACHKEYVAL:
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

          L_JSOP_UNUSED117:

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

    advance_pc:
        pc += len;

#ifdef DEBUG
        if (tracefp) {
            intN ndefs, n;
            jsval *siter;

            



            op = pc[-len];
            ndefs = js_CodeSpec[op].ndefs;
            if (ndefs) {
                SAVE_SP_AND_PC(fp);
                if (op == JSOP_FORELEM && sp[-1] == JSVAL_FALSE)
                    --ndefs;
                for (n = -ndefs; n < 0; n++) {
                    char *bytes = js_DecompileValueGenerator(cx, n, sp[n],
                                                             NULL);
                    if (bytes) {
                        fprintf(tracefp, "%s %s",
                                (n == -ndefs) ? "  output:" : ",",
                                bytes);
                        JS_free(cx, bytes);
                    }
                }
                fprintf(tracefp, " @ %d\n", sp - fp->spbase);
            }
            fprintf(tracefp, "  stack: ");
            for (siter = fp->spbase; siter < sp; siter++) {
                str = js_ValueToString(cx, *siter);
                if (!str)
                    fputs("<null>", tracefp);
                else
                    js_FileEscapedString(tracefp, str, 0);
                fputc(' ', tracefp);
            }
            fputc('\n', tracefp);
        }
#endif 
    }
#endif 

  error:
    JS_ASSERT((size_t)(pc - script->code) < script->length);
    if (!cx->throwing) {
        
        ok = JS_FALSE;
    } else {
        JSTrapHandler handler;
        JSTryNote *tn, *tnlimit;
        uint32 offset;

        
        handler = cx->debugHooks->throwHook;
        if (handler) {
            SAVE_SP_AND_PC(fp);
            switch (handler(cx, script, pc, &rval,
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
            LOAD_INTERRUPT_HANDLER(cx);
        }

        


        if (script->trynotesOffset == 0)
            goto no_catch;

        offset = (uint32)(pc - script->main);
        tn = JS_SCRIPT_TRYNOTES(script)->vector;
        tnlimit = tn + JS_SCRIPT_TRYNOTES(script)->length;
        do {
            if (offset - tn->start >= tn->length)
                continue;

            


















            if (tn->stackDepth > sp - fp->spbase)
                continue;

            




            pc = (script)->main + tn->start + tn->length;

            SAVE_SP_AND_PC(fp);
            ok = UnwindScope(cx, fp, tn->stackDepth, JS_TRUE);
            JS_ASSERT(fp->sp == fp->spbase + tn->stackDepth);
            RESTORE_SP(fp);
            if (!ok) {
                



                goto error;
            }

            switch (tn->kind) {
              case JSTN_CATCH:
                JS_ASSERT(*pc == JSOP_ENTERBLOCK);

#if JS_HAS_GENERATORS
                
                if (JS_UNLIKELY(cx->exception == JSVAL_ARETURN))
                    break;
#endif

                




                len = 0;
                DO_NEXT_OP(len);

              case JSTN_FINALLY:
                



                PUSH(JSVAL_TRUE);
                PUSH(cx->exception);
                cx->throwing = JS_FALSE;
                len = 0;
                DO_NEXT_OP(len);

              case JSTN_ITER:
                




                JS_ASSERT(*pc == JSOP_ENDITER);
                PUSH(cx->exception);
                cx->throwing = JS_FALSE;
                SAVE_SP_AND_PC(fp);
                ok = js_CloseIterator(cx, sp[-2]);
                sp -= 2;
                if (!ok)
                    goto error;
                cx->throwing = JS_TRUE;
                cx->exception = sp[1];
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
    






    SAVE_SP_AND_PC(fp);
    ok &= UnwindScope(cx, fp, 0, ok || cx->throwing);
    JS_ASSERT(fp->sp == fp->spbase);
    RESTORE_SP(fp);

    if (inlineCallCount)
        goto inline_return;

  exit:
    










    JS_ASSERT(inlineCallCount == 0);
    JS_ASSERT(JS_PROPERTY_CACHE(cx).disabled == fp->pcDisabledSave);

    if (JS_LIKELY(mark != NULL)) {
        JS_ASSERT(!fp->blockChain);
        JS_ASSERT(!IsActiveWithOrBlock(cx, fp->scopeChain, 0));
        JS_ASSERT(!(fp->flags & JSFRAME_GENERATOR));
        JS_ASSERT(fp->spbase);
        JS_ASSERT(fp->spbase <= fp->sp);
        JS_ASSERT(sp == fp->spbase);
        fp->sp = fp->spbase = NULL;
        FreeRawStack(cx, mark);
    }

    if (cx->version == currentVersion && currentVersion != originalVersion)
        js_SetVersion(cx, originalVersion);
    cx->interpLevel--;
    return ok;

  atom_not_defined:
    {
        const char *printable;

        ASSERT_SAVED_SP_AND_PC(fp);
        printable = js_AtomToPrintableString(cx, atom);
        if (printable)
            js_ReportIsNotDefined(cx, printable);
        goto error;
    }
}
