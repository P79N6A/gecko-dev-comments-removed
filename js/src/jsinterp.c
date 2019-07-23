










































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

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif







#define PUSH(v)         (*sp++ = (v))
#define POP()           (*--sp)
#ifdef DEBUG
#define SAVE_SP(fp)                                                           \
    (JS_ASSERT((fp)->script || !(fp)->spbase || (sp) == (fp)->spbase),        \
     (fp)->sp = sp)
#else
#define SAVE_SP(fp)     ((fp)->sp = sp)
#endif
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
            ok = js_NewDoubleValue(cx, d, &v_);                               \
            if (!ok)                                                          \
                goto out;                                                     \
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
            ok = js_NewDoubleValue(cx, (jsdouble)(i), &v_);                   \
            if (!ok)                                                          \
                goto out;                                                     \
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
            ok = js_NewDoubleValue(cx, (jsdouble)(u), &v_);                   \
            if (!ok)                                                          \
                goto out;                                                     \
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
            ok = js_ValueToECMAInt32(cx, v_, &i);                             \
            if (!ok)                                                          \
                goto out;                                                     \
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
            ok = js_ValueToECMAUint32(cx, v_, &ui);                           \
            if (!ok)                                                          \
                goto out;                                                     \
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
            ok = js_ValueToNumber(cx, v, &d);                                 \
            if (!ok)                                                          \
                goto out;                                                     \
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
            SAVE_SP_AND_PC(fp);                                               \
            ok = js_ValueToBoolean(cx, v, &b);                                \
            if (!ok)                                                          \
                goto out;                                                     \
        }                                                                     \
        sp--;                                                                 \
    JS_END_MACRO


#define VALUE_TO_OBJECT(cx, n, v, obj)                                        \
    JS_BEGIN_MACRO                                                            \
        ASSERT_SAVED_SP_AND_PC(fp);                                           \
        if (!JSVAL_IS_PRIMITIVE(v)) {                                         \
            obj = JSVAL_TO_OBJECT(v);                                         \
        } else {                                                              \
            obj = js_ValueToNonNullObject(cx, v);                             \
            if (!obj) {                                                       \
                ok = JS_FALSE;                                                \
                goto out;                                                     \
            }                                                                 \
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
        ok = OBJ_DEFAULT_VALUE(cx, JSVAL_TO_OBJECT(v), hint, &sp[n]);         \
        if (!ok)                                                              \
            goto out;                                                         \
        v = sp[n];                                                            \
    JS_END_MACRO

JS_FRIEND_API(jsval *)
js_AllocRawStack(JSContext *cx, uintN nslots, void **markp)
{
    jsval *sp;

    if (markp)
        *markp = JS_ARENA_MARK(&cx->stackPool);
    JS_ARENA_ALLOCATE_CAST(sp, jsval *, &cx->stackPool, nslots * sizeof(jsval));
    if (!sp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_STACK_OVERFLOW,
                             (cx->fp && cx->fp->fun)
                             ? JS_GetFunctionName(cx->fp->fun)
                             : "script");
    }
    return sp;
}

JS_FRIEND_API(void)
js_FreeRawStack(JSContext *cx, void *mark)
{
    JS_ARENA_RELEASE(&cx->stackPool, mark);
}

JS_FRIEND_API(jsval *)
js_AllocStack(JSContext *cx, uintN nslots, void **markp)
{
    jsval *sp, *vp, *end;
    JSArena *a;
    JSStackHeader *sh;
    JSStackFrame *fp;

    
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
        





        fp = cx->fp;
        if (fp && fp->script && fp->spbase) {
#ifdef DEBUG
            jsuword depthdiff = fp->script->depth * sizeof(jsval);
            JS_ASSERT(JS_UPTRDIFF(fp->sp, fp->spbase) <= depthdiff);
            JS_ASSERT(JS_UPTRDIFF(*markp, fp->spbase) >= depthdiff);
#endif
            end = fp->spbase + fp->script->depth;
            for (vp = fp->sp; vp < end; vp++)
                *vp = JSVAL_VOID;
        }

        
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

JSBool
js_GetArgument(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return JS_TRUE;
}

JSBool
js_SetArgument(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return JS_TRUE;
}

JSBool
js_GetLocalVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return JS_TRUE;
}

JSBool
js_SetLocalVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return JS_TRUE;
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
                  JS_GetPrivate(cx, fp->scopeChain) != fp);
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





static JSBool
PutBlockObjects(JSContext *cx, JSStackFrame *fp)
{
    JSBool ok;
    JSObject *obj;

    ok = JS_TRUE;
    for (obj = fp->scopeChain; obj; obj = OBJ_GET_PARENT(cx, obj)) {
        if (OBJ_GET_CLASS(cx, obj) == &js_BlockClass) {
            if (JS_GetPrivate(cx, obj) != fp)
                break;
            ok &= js_PutBlockObject(cx, obj);
        }
    }
    return ok;
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
















static JSBool
ComputeGlobalThis(JSContext *cx, jsval *argv)
{
    JSObject *thisp;

    if (JSVAL_IS_PRIMITIVE(argv[-2]) ||
        !OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(argv[-2]))) {
        thisp = cx->globalObject;
    } else {
        jsid id;
        jsval v;
        uintN attrs;
        JSObject *parent;

        
        thisp = JSVAL_TO_OBJECT(argv[-2]);
        id = ATOM_TO_JSID(cx->runtime->atomState.parentAtom);
        for (;;) {
            if (!OBJ_CHECK_ACCESS(cx, thisp, id, JSACC_PARENT, &v, &attrs))
                return JS_FALSE;
            parent = JSVAL_IS_VOID(v)
                     ? OBJ_GET_PARENT(cx, thisp)
                     : JSVAL_TO_OBJECT(v);
            if (!parent)
                break;
            thisp = parent;
        }
    }
    argv[-1] = OBJECT_TO_JSVAL(thisp);
    return JS_TRUE;
}

static JSBool
ComputeThis(JSContext *cx, jsval *argv)
{
    JSObject *thisp;

    JS_ASSERT(!JSVAL_IS_NULL(argv[-1]));
    if (!JSVAL_IS_OBJECT(argv[-1]))
        return js_PrimitiveToObject(cx, &argv[-1]);

    thisp = JSVAL_TO_OBJECT(argv[-1]);
    if (OBJ_GET_CLASS(cx, thisp) == &js_CallClass)
        return ComputeGlobalThis(cx, argv);

    if (!thisp->map->ops->thisObject)
        return JS_TRUE;

    
    thisp = thisp->map->ops->thisObject(cx, thisp);
    if (!thisp)
        return JS_FALSE;
    argv[-1] = OBJECT_TO_JSVAL(thisp);
    return JS_TRUE;
}

JSBool
js_ComputeThis(JSContext *cx, jsval *argv)
{
    if (JSVAL_IS_NULL(argv[-1]))
        return ComputeGlobalThis(cx, argv);
    return ComputeThis(cx, argv);
}

#if JS_HAS_NO_SUCH_METHOD

static JSBool
NoSuchMethod(JSContext *cx, JSStackFrame *fp, jsval *vp, uint32 flags,
             uintN argc)
{
    JSObject *thisp, *argsobj;
    JSAtom *atom;
    jsval *sp, roots[3];
    JSTempValueRooter tvr;
    jsid id;
    JSBool ok;
    jsbytecode *pc;

    
    JS_ASSERT(JSVAL_IS_PRIMITIVE(vp[0]));
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
    RESTORE_SP(fp);

    
    memset(roots, 0, sizeof roots);
    JS_PUSH_TEMP_ROOT(cx, JS_ARRAY_LENGTH(roots), roots, &tvr);

    id = ATOM_TO_JSID(cx->runtime->atomState.noSuchMethodAtom);
    thisp = JSVAL_TO_OBJECT(vp[1]);
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
#if JS_HAS_XML_SUPPORT
      case JSOP_CALLPROP:
#endif
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

#ifdef DUMP_CALL_TABLE

#include "jsclist.h"
#include "jshash.h"
#include "jsdtoa.h"

typedef struct CallKey {
    jsval               callee;                 
    const char          *filename;              
    uintN               lineno;                 
} CallKey;


#define JSTYPE_NULL     JSTYPE_LIMIT
#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))
#define TYPENAME(t)     (((t) == JSTYPE_NULL) ? js_null_str : js_type_strs[t])
#define NTYPEHIST       (JSTYPE_LIMIT + 1)

typedef struct CallValue {
    uint32              total;                  
    uint32              recycled;               
    uint16              minargc;                
    uint16              maxargc;                
    struct ArgInfo {
        uint32          typeHist[NTYPEHIST];    
        JSCList         lruList;                
        struct ArgValCount {
            JSCList     lruLink;                
            jsval       value;                  
            uint32      count;                  
            char        strbuf[112];            
        } topValCounts[10];                     
    } argInfo[8];
} CallValue;

typedef struct CallEntry {
    JSHashEntry         entry;
    CallKey             key;
    CallValue           value;
    char                name[32];               
} CallEntry;

static void *
AllocCallTable(void *pool, size_t size)
{
    return malloc(size);
}

static void
FreeCallTable(void *pool, void *item)
{
    free(item);
}

static JSHashEntry *
AllocCallEntry(void *pool, const void *key)
{
    return (JSHashEntry*) calloc(1, sizeof(CallEntry));
}

static void
FreeCallEntry(void *pool, JSHashEntry *he, uintN flag)
{
    JS_ASSERT(flag == HT_FREE_ENTRY);
    free(he);
}

static JSHashAllocOps callTableAllocOps = {
    AllocCallTable, FreeCallTable,
    AllocCallEntry, FreeCallEntry
};

JS_STATIC_DLL_CALLBACK(JSHashNumber)
js_hash_call_key(const void *key)
{
    CallKey *ck = (CallKey *) key;
    JSHashNumber hash = (jsuword)ck->callee >> 3;

    if (ck->filename) {
        hash = (hash << 4) ^ JS_HashString(ck->filename);
        hash = (hash << 4) ^ ck->lineno;
    }
    return hash;
}

JS_STATIC_DLL_CALLBACK(intN)
js_compare_call_keys(const void *k1, const void *k2)
{
    CallKey *ck1 = (CallKey *)k1, *ck2 = (CallKey *)k2;

    return ck1->callee == ck2->callee &&
           ((ck1->filename && ck2->filename)
            ? strcmp(ck1->filename, ck2->filename) == 0
            : ck1->filename == ck2->filename) &&
           ck1->lineno == ck2->lineno;
}

JSHashTable *js_CallTable;
size_t      js_LogCallToSourceLimit;

JS_STATIC_DLL_CALLBACK(intN)
CallTableDumper(JSHashEntry *he, intN k, void *arg)
{
    CallEntry *ce = (CallEntry *)he;
    FILE *fp = (FILE *)arg;
    uintN argc, i, n;
    struct ArgInfo *ai;
    JSType save, type;
    JSCList *cl;
    struct ArgValCount *avc;
    jsval argval;

    if (ce->key.filename) {
        
        js_MarkScriptFilename(ce->key.filename);
        fprintf(fp, "%s:%u ", ce->key.filename, ce->key.lineno);
    } else {
        fprintf(fp, "@%p ", (void *) ce->key.callee);
    }

    if (ce->name[0])
        fprintf(fp, "name %s ", ce->name);
    fprintf(fp, "calls %lu (%lu) argc %u/%u\n",
            (unsigned long) ce->value.total,
            (unsigned long) ce->value.recycled,
            ce->value.minargc, ce->value.maxargc);

    argc = JS_MIN(ce->value.maxargc, 8);
    for (i = 0; i < argc; i++) {
        ai = &ce->value.argInfo[i];

        n = 0;
        save = -1;
        for (type = JSTYPE_VOID; type <= JSTYPE_LIMIT; type++) {
            if (ai->typeHist[type]) {
                save = type;
                ++n;
            }
        }
        if (n == 1) {
            fprintf(fp, "  arg %u type %s: %lu\n",
                    i, TYPENAME(save), (unsigned long) ai->typeHist[save]);
        } else {
            fprintf(fp, "  arg %u type histogram:\n", i);
            for (type = JSTYPE_VOID; type <= JSTYPE_LIMIT; type++) {
                fprintf(fp, "  %9s: %8lu ",
                       TYPENAME(type), (unsigned long) ai->typeHist[type]);
                for (n = (uintN) JS_HOWMANY(ai->typeHist[type], 10); n > 0; --n)
                    fputc('*', fp);
                fputc('\n', fp);
            }
        }

        fprintf(fp, "  arg %u top 10 values:\n", i);
        n = 1;
        for (cl = ai->lruList.prev; cl != &ai->lruList; cl = cl->prev) {
            avc = (struct ArgValCount *)cl;
            if (!avc->count)
                break;
            argval = avc->value;
            fprintf(fp, "  %9u: %8lu %.*s (%#lx)\n",
                    n, (unsigned long) avc->count,
                    (int) sizeof avc->strbuf, avc->strbuf,
                    argval);
            ++n;
        }
    }

    return HT_ENUMERATE_NEXT;
}

void
js_DumpCallTable(JSContext *cx)
{
    char name[24];
    FILE *fp;
    static uintN dumpCount;

    if (!js_CallTable)
        return;

    JS_snprintf(name, sizeof name, "/tmp/calltable.dump.%u", dumpCount & 7);
    dumpCount++;
    fp = fopen(name, "w");
    if (!fp)
        return;

    JS_HashTableEnumerateEntries(js_CallTable, CallTableDumper, fp);
    fclose(fp);
}

static void
LogCall(JSContext *cx, jsval callee, uintN argc, jsval *argv)
{
    CallKey key;
    const char *name, *cstr;
    JSFunction *fun;
    JSHashNumber keyHash;
    JSHashEntry **hep, *he;
    CallEntry *ce;
    uintN i, j;
    jsval argval;
    JSType type;
    struct ArgInfo *ai;
    struct ArgValCount *avc;
    JSString *str;

    if (!js_CallTable) {
        js_CallTable = JS_NewHashTable(1024, js_hash_call_key,
                                       js_compare_call_keys, NULL,
                                       &callTableAllocOps, NULL);
        if (!js_CallTable)
            return;
    }

    key.callee = callee;
    key.filename = NULL;
    key.lineno = 0;
    name = "";
    if (VALUE_IS_FUNCTION(cx, callee)) {
        fun = (JSFunction *) OBJ_GET_PRIVATE(cx, JSVAL_TO_OBJECT(callee));
        if (fun->atom)
            name = js_AtomToPrintableString(cx, fun->atom);
        if (FUN_INTERPRETED(fun)) {
            key.filename = fun->u.i.script->filename;
            key.lineno = fun->u.i.script->lineno;
        }
    }
    keyHash = js_hash_call_key(&key);

    hep = JS_HashTableRawLookup(js_CallTable, keyHash, &key);
    he = *hep;
    if (he) {
        ce = (CallEntry *) he;
        JS_ASSERT(strncmp(ce->name, name, sizeof ce->name) == 0);
    } else {
        he = JS_HashTableRawAdd(js_CallTable, hep, keyHash, &key, NULL);
        if (!he)
            return;
        ce = (CallEntry *) he;
        ce->entry.key = &ce->key;
        ce->entry.value = &ce->value;
        ce->key = key;
        for (i = 0; i < 8; i++) {
            ai = &ce->value.argInfo[i];
            JS_INIT_CLIST(&ai->lruList);
            for (j = 0; j < 10; j++)
                JS_APPEND_LINK(&ai->topValCounts[j].lruLink, &ai->lruList);
        }
        strncpy(ce->name, name, sizeof ce->name);
    }

    ++ce->value.total;
    if (ce->value.minargc < argc)
        ce->value.minargc = argc;
    if (ce->value.maxargc < argc)
        ce->value.maxargc = argc;
    if (argc > 8)
        argc = 8;
    for (i = 0; i < argc; i++) {
        ai = &ce->value.argInfo[i];
        argval = argv[i];
        type = TYPEOF(cx, argval);
        ++ai->typeHist[type];

        for (j = 0; ; j++) {
            if (j == 10) {
                avc = (struct ArgValCount *) ai->lruList.next;
                ce->value.recycled += avc->count;
                avc->value = argval;
                avc->count = 1;
                break;
            }
            avc = &ai->topValCounts[j];
            if (avc->value == argval) {
                ++avc->count;
                break;
            }
        }

        
        JS_REMOVE_LINK(&avc->lruLink);
        JS_APPEND_LINK(&avc->lruLink, &ai->lruList);

        str = NULL;
        cstr = "";
        switch (TYPEOF(cx, argval)) {
          case JSTYPE_VOID:
            cstr = js_type_strs[JSTYPE_VOID];
            break;
          case JSTYPE_NULL:
            cstr = js_null_str;
            break;
          case JSTYPE_BOOLEAN:
            cstr = js_boolean_strs[JSVAL_TO_BOOLEAN(argval)];
            break;
          case JSTYPE_NUMBER:
            if (JSVAL_IS_INT(argval)) {
                JS_snprintf(avc->strbuf, sizeof avc->strbuf, "%ld",
                            JSVAL_TO_INT(argval));
            } else {
                JS_dtostr(avc->strbuf, sizeof avc->strbuf, DTOSTR_STANDARD, 0,
                          *JSVAL_TO_DOUBLE(argval));
            }
            continue;
          case JSTYPE_STRING:
            str = js_QuoteString(cx, JSVAL_TO_STRING(argval), (jschar)'"');
            break;
          case JSTYPE_FUNCTION:
            if (VALUE_IS_FUNCTION(cx, argval)) {
                fun = (JSFunction *) OBJ_GET_PRIVATE(cx, JSVAL_TO_OBJECT(argval));
                if (fun && fun->atom) {
                    str = ATOM_TO_STRING(fun->atom);
                    break;
                }
            }
            
          case JSTYPE_OBJECT:
            js_LogCallToSourceLimit = sizeof avc->strbuf;
            cx->options |= JSOPTION_LOGCALL_TOSOURCE;
            str = js_ValueToSource(cx, argval);
            cx->options &= ~JSOPTION_LOGCALL_TOSOURCE;
            break;
        }
        if (str)
            js_PutEscapedString(avc->strbuf, sizeof avc->strbuf, str, 0);
        else
            strncpy(avc->strbuf, cstr, sizeof avc->strbuf);
    }
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

#define START_FAST_CALL(fp) (JS_ASSERT(!((fp)->flags & JSFRAME_IN_FAST_CALL)),\
                             (fp)->flags |= JSFRAME_IN_FAST_CALL)
#define END_FAST_CALL(fp)   (JS_ASSERT((fp)->flags & JSFRAME_IN_FAST_CALL),   \
                             (fp)->flags &= ~JSFRAME_IN_FAST_CALL)





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
js_Invoke(JSContext *cx, uintN argc, uintN flags)
{
    void *mark;
    JSStackFrame *fp, frame;
    jsval *sp, *newsp, *limit;
    jsval *vp, v;
    JSObject *funobj, *parent;
    JSBool ok;
    JSClass *clasp;
    JSObjectOps *ops;
    JSNative native;
    JSFunction *fun;
    JSScript *script;
    uintN nslots, nvars, nalloc, surplus;
    JSInterpreterHook hook;
    void *hookData;

    
    mark = JS_ARENA_MARK(&cx->stackPool);
    fp = cx->fp;
    sp = fp->sp;

    





    vp = sp - (2 + argc);
    v = *vp;
    frame.rval = JSVAL_VOID;

    










    if (JSVAL_IS_PRIMITIVE(v)) {
#if JS_HAS_NO_SUCH_METHOD
        if (fp->script && !(flags & JSINVOKE_INTERNAL)) {
            ok = NoSuchMethod(cx, fp, vp, flags, argc);
            if (ok)
                frame.rval = *vp;
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
        
        fun = (JSFunction *) OBJ_GET_PRIVATE(cx, funobj);
        nalloc = FUN_MINARGS(fun);
        nslots = (nalloc > argc) ? nalloc - argc : 0;
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
                goto init_frame;
        }
    }

    if (flags & JSINVOKE_CONSTRUCT) {
        
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
        frame.rval = vp[1];
    } else {
        




        ok = js_ComputeThis(cx, vp + 2);
        if (!ok)
            goto out2;
    }

  init_frame:
    






    frame.thisp = (JSObject *)vp[1];
    frame.varobj = NULL;
    frame.callobj = frame.argsobj = NULL;
    frame.script = script;
    frame.callee = funobj;
    frame.fun = fun;
    frame.argc = argc;
    frame.argv = sp - argc;
    frame.nvars = nvars;
    frame.vars = sp;
    frame.down = fp;
    frame.annotation = NULL;
    frame.scopeChain = NULL;    
    frame.pc = NULL;
    frame.spbase = NULL;
    frame.sharpDepth = 0;
    frame.sharpArray = NULL;
    frame.flags = flags;
    frame.dormantNext = NULL;
    frame.xmlNamespace = NULL;
    frame.blockChain = NULL;

    
    cx->fp = &frame;

    
    hook = cx->debugHooks->callHook;
    hookData = NULL;

    
    if (nslots) {
        
        nalloc = nslots;
        limit = (jsval *) cx->stackPool.current->limit;
        JS_ASSERT((jsval *) cx->stackPool.current->base <= sp && sp <= limit);
        if (sp + nslots > limit) {
            
            nalloc += 2 + argc;
        } else {
            
            JS_ASSERT((jsval *)mark >= sp);
            surplus = (jsval *)mark - sp;
            nalloc -= surplus;
        }

        
        if ((intN)nalloc > 0) {
            
            newsp = js_AllocRawStack(cx, nalloc, NULL);
            if (!newsp) {
                ok = JS_FALSE;
                goto out;
            }

            
            if (newsp != mark) {
                JS_ASSERT(sp + nslots > limit);
                JS_ASSERT(2 + argc + nslots == nalloc);
                *newsp++ = vp[0];
                *newsp++ = vp[1];
                if (argc)
                    memcpy(newsp, frame.argv, argc * sizeof(jsval));
                frame.argv = newsp;
                sp = frame.vars = newsp + argc;
            }
        }

        
        frame.vars += nslots;

        
        do {
            PUSH(JSVAL_VOID);
        } while (--nslots != 0);
    }
    JS_ASSERT(nslots == 0);

    
    if (nvars) {
        JS_ASSERT((jsval *)cx->stackPool.current->avail >= frame.vars);
        surplus = (jsval *)cx->stackPool.current->avail - frame.vars;
        if (surplus < nvars) {
            newsp = js_AllocRawStack(cx, nvars, NULL);
            if (!newsp) {
                ok = JS_FALSE;
                goto out;
            }
            if (newsp != sp) {
                
                sp = frame.vars = newsp;
            }
        }

        
        do {
            PUSH(JSVAL_VOID);
        } while (--nvars != 0);
    }
    JS_ASSERT(nvars == 0);

    
    SAVE_SP(&frame);

    
    if (hook && (native || script))
        hookData = hook(cx, &frame, JS_TRUE, 0, cx->debugHooks->callHookData);

    
    if (native) {
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif

#if JS_HAS_LVALUE_RETURN
        
        cx->rval2set = JS_FALSE;
#endif

        
        frame.varobj = fp->varobj;
        frame.scopeChain = fp->scopeChain;

        
        if (!frame.scopeChain)
            frame.scopeChain = parent;

        if (fun && (fun->flags & JSFUN_FAST_NATIVE)) {
            




            ok = ((JSFastNative) native)(cx, argc, frame.argv - 2);
            frame.rval = frame.argv[-2];
        } else {
#ifdef DEBUG_brendan
            static FILE *fp;
            if (!fp) {
                fp = fopen("/tmp/slow-natives.dump", "w");
                if (fp)
                    setlinebuf(fp);
            }
            if (fp) {
                fprintf(fp, "%p %s.%s\n",
                        native,
                        JSVAL_IS_OBJECT(vp[1])
                        ? ((OBJ_GET_CLASS(cx, frame.thisp) == &js_FunctionClass)
                           ? JS_GetFunctionName(JS_GetPrivate(cx, frame.thisp))
                           : OBJ_GET_CLASS(cx, frame.thisp)->name)
                        : JSVAL_IS_BOOLEAN(vp[1])
                        ? js_BooleanClass.name
                        : JSVAL_IS_STRING(vp[1])
                        ? js_StringClass.name
                        : js_NumberClass.name,
                        fun && fun->atom
                        ? JS_GetFunctionName(fun)
                        : "???");
            }
#endif
            ok = native(cx, frame.thisp, argc, frame.argv, &frame.rval);
        }

        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
    } else if (script) {
#ifdef DUMP_CALL_TABLE
        LogCall(cx, *vp, argc, frame.argv);
#endif
        
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

    
    cx->fp = fp;

out2:
    
    JS_ARENA_RELEASE(&cx->stackPool, mark);

    
    *vp = frame.rval;
    fp->sp = vp + 1;

    




    if (fp->script && !(flags & JSINVOKE_INTERNAL))
        vp[-(intN)fp->script->depth] = (jsval)fp->pc;
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
    JSStackFrame *fp, *oldfp, frame;
    jsval *oldsp, *sp;
    void *mark;
    uintN i;
    JSBool ok;

    fp = oldfp = cx->fp;
    if (!fp) {
        memset(&frame, 0, sizeof frame);
        cx->fp = fp = &frame;
    }
    oldsp = fp->sp;
    sp = js_AllocStack(cx, 2 + argc, &mark);
    if (!sp) {
        ok = JS_FALSE;
        goto out;
    }

    PUSH(fval);
    PUSH(OBJECT_TO_JSVAL(obj));
    for (i = 0; i < argc; i++)
        PUSH(argv[i]);
    SAVE_SP(fp);
    ok = js_Invoke(cx, argc, flags | JSINVOKE_INTERNAL);
    if (ok) {
        RESTORE_SP(fp);

        






        *rval = POP_OPND();
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
out:
    fp->sp = oldsp;
    if (oldfp != fp)
        cx->fp = oldfp;

    return ok;
}

JSBool
js_InternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, jsval fval,
                    JSAccessMode mode, uintN argc, jsval *argv, jsval *rval)
{
    int stackDummy;

    



    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }

    














    JS_ASSERT(mode == JSACC_READ || mode == JSACC_WRITE);
    if (cx->runtime->checkObjectAccess &&
        VALUE_IS_FUNCTION(cx, fval) &&
        FUN_INTERPRETED((JSFunction *)
                        JS_GetPrivate(cx, JSVAL_TO_OBJECT(fval))) &&
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
            frame.vars = js_AllocRawStack(cx, frame.nvars, &mark);
            if (!frame.vars)
                return JS_FALSE;
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
    frame.sp = oldfp ? oldfp->sp : NULL;
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
        js_FreeRawStack(cx, mark);
    cx->fp = oldfp;

    if (oldfp && oldfp != down) {
        JS_ASSERT(cx->dormantFrameChain == oldfp);
        cx->dormantFrameChain = oldfp->dormantNext;
        oldfp->dormantNext = NULL;
    }

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
            ok = OBJ_DEFINE_PROPERTY(cx, target, id, value, NULL, NULL,
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
js_StrictlyEqual(jsval lval, jsval rval)
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
    JSFunction *fun;
    JSObject *obj, *obj2, *proto, *parent;
    jsval lval, rval;
    JSClass *clasp, *funclasp;

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
            funclasp = ((JSFunction *) OBJ_GET_PRIVATE(cx, obj2))->clasp;
            if (funclasp)
                clasp = funclasp;
        }
    }
    obj = js_NewObject(cx, clasp, proto, parent);
    if (!obj)
        return JS_FALSE;

    
    vp[1] = OBJECT_TO_JSVAL(obj);
    if (!js_Invoke(cx, argc, JSINVOKE_CONSTRUCT)) {
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
    JSAtom *atom;

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

    atom = js_ValueToStringAtom(cx, idval);
    if (!atom)
        return JS_FALSE;
    *idp = ATOM_TO_JSID(atom);
    return JS_TRUE;
}

#ifndef MAX_INTERP_LEVEL
#if defined(XP_OS2)
#define MAX_INTERP_LEVEL 250
#else
#define MAX_INTERP_LEVEL 1000
#endif
#endif

#define MAX_INLINE_CALL_COUNT 1000








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
    jsval *sp, *newsp;
    void *mark;
    jsbytecode *endpc, *pc2;
    JSOp op, op2;
    jsatomid index;
    JSAtom *atom;
    uintN argc, attrs, flags, slot;
    jsval *vp, lval, rval, ltmp, rtmp;
    jsid id;
    JSObject *withobj, *iterobj;
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
    int stackDummy;

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

    








    currentVersion = (JSVersion) script->version;
    originalVersion = (JSVersion) cx->version;
    if (currentVersion != originalVersion)
        js_SetVersion(cx, currentVersion);

#ifdef __GNUC__
    flags = 0;  
    id = 0;
#endif

    



#define CHECK_BRANCH(len)                                                     \
    JS_BEGIN_MACRO                                                            \
        if (len <= 0 && cx->branchCallback) {                                 \
            SAVE_SP_AND_PC(fp);                                               \
            if (!(ok = cx->branchCallback(cx, script)))                       \
                goto out;                                                     \
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

    
    ++cx->interpLevel;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        ok = JS_FALSE;
        goto out2;
    }

    




    depth = (jsint) script->depth;
    if (JS_LIKELY(!fp->spbase)) {
        newsp = js_AllocRawStack(cx, (uintN)(2 * depth), &mark);
        if (!newsp) {
            ok = JS_FALSE;
            goto out2;
        }
        sp = newsp + depth;
        fp->spbase = sp;
        SAVE_SP(fp);
    } else {
        sp = fp->sp;
        JS_ASSERT(JS_UPTRDIFF(sp, fp->spbase) <= depth * sizeof(jsval));
        newsp = fp->spbase - depth;
        mark = NULL;
    }

    





    ok = !cx->throwing;
    if (!ok) {
#ifdef DEBUG_NOT_THROWING
        printf("JS INTERPRETER CALLED WITH PENDING EXCEPTION %lx\n",
               (unsigned long) cx->exception);
#endif
        goto out;
    }

#if JS_THREADED_INTERP

    











    op = (JSOp) *pc;
    if (interruptHandler) {
interrupt:
        SAVE_SP_AND_PC(fp);
        switch (interruptHandler(cx, script, pc, &rval,
                                 cx->debugHooks->interruptHandlerData)) {
          case JSTRAP_ERROR:
            ok = JS_FALSE;
            goto out;
          case JSTRAP_CONTINUE:
            break;
          case JSTRAP_RETURN:
            fp->rval = rval;
            goto out;
          case JSTRAP_THROW:
            cx->throwing = JS_TRUE;
            cx->exception = rval;
            ok = JS_FALSE;
            goto out;
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
                ok = JS_FALSE;
                goto out;
              case JSTRAP_CONTINUE:
                break;
              case JSTRAP_RETURN:
                fp->rval = rval;
                goto out;
              case JSTRAP_THROW:
                cx->throwing = JS_TRUE;
                cx->exception = rval;
                ok = JS_FALSE;
                goto out;
              default:;
            }
            LOAD_INTERRUPT_HANDLER(cx);
        }

        switch (op) {

#endif

          BEGIN_CASE(JSOP_STOP)
            goto out;

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
                if (JS_GetPrivate(cx, obj) != fp)
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
            FETCH_OBJECT(cx, -1, rval, obj);
            OBJ_TO_INNER_OBJECT(cx, obj);
            if (!obj || !(obj2 = js_GetScopeChain(cx, fp))) {
                ok = JS_FALSE;
                goto out;
            }
            withobj = js_NewWithObject(cx, obj, obj2, sp - fp->spbase - 1);
            if (!withobj) {
                ok = JS_FALSE;
                goto out;
            }
            fp->scopeChain = withobj;
            STORE_OPND(-1, OBJECT_TO_JSVAL(withobj));
          END_CASE(JSOP_ENTERWITH)

          BEGIN_CASE(JSOP_LEAVEWITH)
            rval = POP_OPND();
            JS_ASSERT(JSVAL_IS_OBJECT(rval));
            withobj = JSVAL_TO_OBJECT(rval);
            JS_ASSERT(OBJ_GET_CLASS(cx, withobj) == &js_WithClass);
            fp->scopeChain = OBJ_GET_PARENT(cx, withobj);
            JS_SetPrivate(cx, withobj, NULL);
          END_CASE(JSOP_LEAVEWITH)

          BEGIN_CASE(JSOP_SETRVAL)
            ASSERT_NOT_THROWING(cx);
            fp->rval = POP_OPND();
          END_CASE(JSOP_SETRVAL)

          BEGIN_CASE(JSOP_RETURN)
            CHECK_BRANCH(-1);
            fp->rval = POP_OPND();
            

          BEGIN_CASE(JSOP_RETRVAL)    
            ASSERT_NOT_THROWING(cx);
            if (inlineCallCount)
          inline_return:
            {
                JSInlineFrame *ifp = (JSInlineFrame *) fp;
                void *hookData = ifp->hookData;

                






                if (fp->flags & JSFRAME_POP_BLOCKS) {
                    SAVE_SP_AND_PC(fp);
                    ok &= PutBlockObjects(cx, fp);
                }

                if (hookData) {
                    JSInterpreterHook hook = cx->debugHooks->callHook;
                    if (hook) {
                        SAVE_SP_AND_PC(fp);
                        hook(cx, fp, JS_FALSE, &ok, hookData);
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
#if !JS_THREADED_INTERP
                endpc = script->code + script->length;
#endif

                
                vp[-depth] = (jsval)pc;

                
                inlineCallCount--;
                if (JS_LIKELY(ok)) {
                    JS_ASSERT(js_CodeSpec[*pc].length == JSOP_CALL_LENGTH);
                    len = JSOP_CALL_LENGTH;
                    DO_NEXT_OP(len);
                }
            }
            goto out;

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
            ok = InternNonIntElementId(cx, obj, idval_, &id);                 \
            if (!ok)                                                          \
                goto out;                                                     \
        }                                                                     \
    JS_END_MACRO

          BEGIN_CASE(JSOP_IN)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            if (JSVAL_IS_PRIMITIVE(rval)) {
                js_ReportValueError(cx, JSMSG_IN_NOT_OBJECT, -1, rval, NULL);
                ok = JS_FALSE;
                goto out;
            }
            obj = JSVAL_TO_OBJECT(rval);
            FETCH_ELEMENT_ID(obj, -2, id);
            ok = OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop);
            if (!ok)
                goto out;
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
            ok = js_ValueToIterator(cx, flags, &sp[-1]);
            if (!ok)
                goto out;
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
            ok = js_CallIteratorNext(cx, iterobj, &rval);
            if (!ok)
                goto out;
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
                




                FETCH_OBJECT(cx, -1, lval, obj);
                goto set_for_property;

              case JSOP_FORNAME:
                





                ok = js_FindProperty(cx, id, &obj, &obj2, &prop);
                if (!ok)
                    goto out;
                if (prop)
                    OBJ_DROP_PROPERTY(cx, obj2, prop);

              set_for_property:
                
                fp->flags |= JSFRAME_ASSIGNING;
                ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
                fp->flags &= ~JSFRAME_ASSIGNING;
                if (!ok)
                    goto out;
                break;

              default:
                JS_ASSERT(0);
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
        /* Get or set the property, set ok false if error, true if success. */\
        call;                                                                 \
        if (!ok)                                                              \
            goto out;                                                         \
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
        /* Get or set the element, set ok false if error, true if success. */ \
        call;                                                                 \
        if (!ok)                                                              \
            goto out;                                                         \
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
            ok = js_NativeGet(cx, obj, pobj, sprop, vp);                      \
            if (!ok)                                                          \
                goto out;                                                     \
        }                                                                     \
    JS_END_MACRO

          BEGIN_CASE(JSOP_SETCONST)
            LOAD_ATOM(0);
            obj = fp->varobj;
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            ok = OBJ_DEFINE_PROPERTY(cx, obj, ATOM_TO_JSID(atom), rval,
                                     NULL, NULL,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL);
            if (!ok)
                goto out;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETCONST)

#if JS_HAS_DESTRUCTURING
          BEGIN_CASE(JSOP_ENUMCONSTELEM)
            rval = FETCH_OPND(-3);
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -2, lval, obj);
            FETCH_ELEMENT_ID(obj, -1, id);
            ok = OBJ_DEFINE_PROPERTY(cx, obj, id, rval, NULL, NULL,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL);
            if (!ok)
                goto out;
            sp -= 3;
          END_CASE(JSOP_ENUMCONSTELEM)
#endif

          BEGIN_CASE(JSOP_BINDNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            SAVE_SP_AND_PC(fp);
            obj = js_FindIdentifierBase(cx, id);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_BINDNAME)

          BEGIN_CASE(JSOP_SETNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(lval));
            obj  = JSVAL_TO_OBJECT(lval);
            SAVE_SP_AND_PC(fp);
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETNAME)

#define INTEGER_OP(OP, EXTRA_CODE)                                            \
    JS_BEGIN_MACRO                                                            \
        FETCH_INT(cx, -1, j);                                                 \
        FETCH_INT(cx, -2, i);                                                 \
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
        ok = ops->equality(cx, obj2, rval, &cond);                            \
        if (!ok)                                                              \
            goto out;                                                         \
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
        ok = xclasp->equality(cx, obj2, rval, &cond);                         \
        if (!ok)                                                              \
            goto out;                                                         \
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
        cond = js_StrictlyEqual(lval, rval) OP JS_TRUE;                       \
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

            FETCH_INT(cx, -1, j);
            FETCH_UINT(cx, -2, u);
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
                ok = ops->concatenate(cx, obj2, rval, &rval);
                if (!ok)
                    goto out;
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
                        ok = (str2 = js_ValueToString(cx, rval)) != NULL;
                        if (!ok)
                            goto out;
                        sp[-1] = STRING_TO_JSVAL(str2);
                    } else {
                        str2 = JSVAL_TO_STRING(rval);
                        ok = (str = js_ValueToString(cx, lval)) != NULL;
                        if (!ok)
                            goto out;
                        sp[-2] = STRING_TO_JSVAL(str);
                    }
                    str = js_ConcatStrings(cx, str, str2);
                    if (!str) {
                        ok = JS_FALSE;
                        goto out;
                    }
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
                if (JSVAL_IS_DOUBLE(rval)) {
                    d = *JSVAL_TO_DOUBLE(rval);
                } else {
                    SAVE_SP_AND_PC(fp);
                    ok = js_ValueToNumber(cx, rval, &d);
                    if (!ok)
                        goto out;
                }
#ifdef HPUX
                




                JSDOUBLE_HI32(d) ^= JSDOUBLE_HI32_SIGNBIT;
#else
                d = -d;
#endif
                ok = js_NewNumberValue(cx, d, &rval);
                if (!ok)
                    goto out;
            }
            STORE_OPND(-1, rval);
          END_CASE(JSOP_NEG)

          BEGIN_CASE(JSOP_POS)
            rval = FETCH_OPND(-1);
            if (!JSVAL_IS_NUMBER(rval)) {
                SAVE_SP_AND_PC(fp);
                ok = js_ValueToNumber(cx, rval, &d);
                if (!ok)
                    goto out;
                ok = js_NewNumberValue(cx, d, &rval);
                if (!ok)
                    goto out;
                sp[-1] = rval;
            }
            sp[-1-depth] = (jsval)pc;
          END_CASE(JSOP_POS)

          BEGIN_CASE(JSOP_NEW)
            
            argc = GET_ARGC(pc);

          do_new:
            SAVE_SP_AND_PC(fp);
            vp = sp - (2 + argc);
            JS_ASSERT(vp >= fp->spbase);

            ok = js_InvokeConstructor(cx, vp, argc);
            if (!ok)
                goto out;
            RESTORE_SP(fp);
            LOAD_INTERRUPT_HANDLER(cx);
            obj = JSVAL_TO_OBJECT(*vp);
            len = js_CodeSpec[op].length;
            DO_NEXT_OP(len);

          BEGIN_CASE(JSOP_DELNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);

            SAVE_SP_AND_PC(fp);
            ok = js_FindProperty(cx, id, &obj, &obj2, &prop);
            if (!ok)
                goto out;

            
            rval = JSVAL_TRUE;
            if (prop) {
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                ok = OBJ_DELETE_PROPERTY(cx, obj, id, &rval);
                if (!ok)
                    goto out;
            }
            PUSH_OPND(rval);
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            PROPERTY_OP(-1, ok = OBJ_DELETE_PROPERTY(cx, obj, id, &rval));
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DELPROP)

          BEGIN_CASE(JSOP_DELELEM)
            ELEMENT_OP(-1, ok = OBJ_DELETE_PROPERTY(cx, obj, id, &rval));
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
            STORE_OPND(i, OBJECT_TO_JSVAL(obj));
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
            ok = js_FindProperty(cx, id, &obj, &obj2, &prop);
            if (!ok)
                goto out;
            if (!prop)
                goto atom_not_defined;

            OBJ_DROP_PROPERTY(cx, obj2, prop);
            lval = OBJECT_TO_JSVAL(obj);
            i = 0;

          do_incop:
          {
            const JSCodeSpec *cs;

            
            ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;

            
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
            if (!JSVAL_IS_NUMBER(rtmp)) {                                     \
                ok = js_NewNumberValue(cx, d, &rtmp);                         \
                if (!ok)                                                      \
                    goto out;                                                 \
            }                                                                 \
            *vp = rtmp;                                                       \
            (cs->format & JOF_INC) ? d++ : d--;                               \
            ok = js_NewNumberValue(cx, d, &rval);                             \
        } else {                                                              \
            (cs->format & JOF_INC) ? ++d : --d;                               \
            ok = js_NewNumberValue(cx, d, &rval);                             \
            rtmp = rval;                                                      \
        }                                                                     \
        if (!ok)                                                              \
            goto out;                                                         \
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
                goto out;
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
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            obj = fp->thisp;
            SAVE_SP_AND_PC(fp);
            ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
            PUSH_OPND(rval);
          END_CASE(JSOP_GETTHISPROP)

          BEGIN_CASE(JSOP_GETARGPROP)
            LOAD_ATOM(ARGNO_LEN);
            slot = GET_ARGNO(pc);
            JS_ASSERT(slot < fp->fun->nargs);
            PUSH_OPND(fp->argv[slot]);
            len = JSOP_GETARGPROP_LENGTH;
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETVARPROP)
            LOAD_ATOM(VARNO_LEN);
            slot = GET_VARNO(pc);
            JS_ASSERT(slot < fp->fun->u.i.nvars);
            PUSH_OPND(fp->vars[slot]);
            len = JSOP_GETVARPROP_LENGTH;
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETLOCALPROP)
            LOAD_ATOM(2);
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            PUSH_OPND(fp->spbase[slot]);
            len = JSOP_GETLOCALPROP_LENGTH;
            goto do_getprop_body;

          BEGIN_CASE(JSOP_GETPROP)
          BEGIN_CASE(JSOP_GETXPROP)
            
            LOAD_ATOM(0);
            len = JSOP_GETPROP_LENGTH;

          do_getprop_body:
            lval = FETCH_OPND(-1);
            if (JSVAL_IS_STRING(lval) && atom == rt->atomState.lengthAtom) {
                str = JSVAL_TO_STRING(lval);
                rval = INT_TO_JSVAL(JSSTRING_LENGTH(str));
            } else {
                id = ATOM_TO_JSID(atom);
                SAVE_SP_AND_PC(fp);
                VALUE_TO_OBJECT(cx, -1, lval, obj);
                ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
                if (!ok)
                    goto out;
            }
            STORE_OPND(-1, rval);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_SETPROP)
            
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);

            
            rval = FETCH_OPND(-1);
            PROPERTY_OP(-2, ok = OBJ_SET_PROPERTY(cx, obj, id, &rval));
            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETPROP)

          BEGIN_CASE(JSOP_GETELEM)
            ELEMENT_OP(-1, ok = OBJ_GET_PROPERTY(cx, obj, id, &rval));
            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_CALLELEM)
            



            ELEMENT_OP(-1, ok = OBJ_GET_PROPERTY(cx, obj, id, &rval));
            STORE_OPND(-2, rval);
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_CALLELEM)

          BEGIN_CASE(JSOP_SETELEM)
            rval = FETCH_OPND(-1);
            ELEMENT_OP(-2, ok = OBJ_SET_PROPERTY(cx, obj, id, &rval));
            sp -= 2;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETELEM)

          BEGIN_CASE(JSOP_ENUMELEM)
            
            rval = FETCH_OPND(-3);
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -2, lval, obj);
            FETCH_ELEMENT_ID(obj, -1, id);
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
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
                fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);

                if (fun->flags & JSFUN_INTERPRETED) {
                    uintN nframeslots, nvars, nslots, missing;
                    JSArena *a;
                    jsuword avail, nbytes;
                    JSBool overflow;
                    void *newmark;
                    jsval *rvp;
                    JSInlineFrame *newifp;
                    JSInterpreterHook hook;

                    
                    if (inlineCallCount == MAX_INLINE_CALL_COUNT) {
                        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                             JSMSG_OVER_RECURSED);
                        ok = JS_FALSE;
                        goto out;
                    }

                    
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
                            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                                 JSMSG_STACK_OVERFLOW,
                                                 (fp && fp->fun)
                                                 ? JS_GetFunctionName(fp->fun)
                                                 : "script");
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
                    newifp->rvp = rvp;
                    newifp->mark = newmark;

                    
                    JS_ASSERT(!JSFUN_BOUND_METHOD_TEST(fun->flags));
                    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
                    newifp->frame.thisp = (JSObject *)vp[1];
#ifdef DUMP_CALL_TABLE
                    LogCall(cx, *vp, argc, vp + 2);
#endif

                    
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
#if !JS_THREADED_INTERP
                    endpc = pc + script->length;
#endif
                    inlineCallCount++;
                    JS_RUNTIME_METER(rt, inlineCalls);

                    
                    op = (JSOp) *pc;
                    DO_OP();

                  bad_inline_call:
                    RESTORE_SP(fp);
                    JS_ASSERT(fp->pc == pc);
                    script = fp->script;
                    depth = (jsint) script->depth;
                    atoms = script->atomMap.vector;
                    js_FreeRawStack(cx, newmark);
                    ok = JS_FALSE;
                    goto out;
                }

                if (fun->flags & JSFUN_FAST_NATIVE) {
                    uintN nargs = JS_MAX(argc, fun->u.n.minargs);

                    nargs += fun->u.n.extra;
                    if (argc < nargs) {
                        



                        nargs -= argc;
                        if (sp + nargs > fp->spbase + depth)
                            goto do_invoke;
                        do {
                            PUSH(JSVAL_VOID);
                        } while (--nargs != 0);
                        SAVE_SP(fp);
                    }

                    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]) ||
                              PRIMITIVE_THIS_TEST(fun, vp[1]));

                    START_FAST_CALL(fp);
                    ok = ((JSFastNative) fun->u.n.native)(cx, argc, vp);
                    END_FAST_CALL(fp);
                    if (!ok)
                        goto out;
                    sp = vp + 1;
                    vp[-depth] = (jsval)pc;
                    goto end_call;
                }
            }

          do_invoke:
            ok = js_Invoke(cx, argc, 0);
            RESTORE_SP(fp);
            LOAD_INTERRUPT_HANDLER(cx);
            if (!ok)
                goto out;
            JS_RUNTIME_METER(rt, nonInlineCalls);

          end_call:
#if JS_HAS_LVALUE_RETURN
            if (cx->rval2set) {
                













                PUSH_OPND(cx->rval2);
                ELEMENT_OP(-1, ok = OBJ_GET_PROPERTY(cx, obj, id, &rval));

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
            ok = js_Invoke(cx, argc, 0);
            RESTORE_SP(fp);
            LOAD_INTERRUPT_HANDLER(cx);
            if (!ok)
                goto out;
            if (!cx->rval2set) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_LEFTSIDE_OF_ASS);
                ok = JS_FALSE;
                goto out;
            }
            PUSH_OPND(cx->rval2);
            cx->rval2set = JS_FALSE;
          END_CASE(JSOP_SETCALL)
#endif

          BEGIN_CASE(JSOP_NAME)
          BEGIN_CASE(JSOP_CALLNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);

            SAVE_SP_AND_PC(fp);
            ok = js_FindProperty(cx, id, &obj, &obj2, &prop);
            if (!ok)
                goto out;
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
                ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
                if (!ok)
                    goto out;
            } else {
                sprop = (JSScopeProperty *)prop;
                NATIVE_GET(cx, obj, obj2, sprop, &rval);
                OBJ_DROP_PROPERTY(cx, obj2, prop);
            }
            PUSH_OPND(rval);
            if (op == JSOP_CALLNAME) {
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
                SAVE_SP(fp);
                ok = ComputeThis(cx, sp);
                if (!ok)
                    goto out;
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
                    if (!obj) {
                        ok = JS_FALSE;
                        goto out;
                    }
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
            clasp = OBJ_GET_CLASS(cx, obj);
            if (clasp->flags & JSCLASS_IS_EXTENDED) {
                JSExtendedClass *xclasp;

                xclasp = (JSExtendedClass *) clasp;
                if (xclasp->outerObject) {
                    obj = xclasp->outerObject(cx, obj);
                    if (!obj) {
                        ok = JS_FALSE;
                        goto out;
                    }
                }
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
            if (!ida) {
                ok = JS_FALSE;
            } else {
                for (i = 0, j = ida->length; i < j; i++) {
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
                JS_DestroyIdArray(cx, ida);
            }
          END_CASE(JSOP_EXPORTALL)

          BEGIN_CASE(JSOP_EXPORTNAME)
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            obj = fp->varobj;
            SAVE_SP_AND_PC(fp);
            ok = OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop);
            if (!ok)
                goto out;
            if (!prop) {
                ok = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID, NULL, NULL,
                                         JSPROP_EXPORTED, NULL);
            } else {
                ok = OBJ_GET_ATTRIBUTES(cx, obj, id, prop, &attrs);
                if (ok) {
                    attrs |= JSPROP_EXPORTED;
                    ok = OBJ_SET_ATTRIBUTES(cx, obj, id, prop, &attrs);
                }
                OBJ_DROP_PROPERTY(cx, obj2, prop);
            }
            if (!ok)
                goto out;
          END_CASE(JSOP_EXPORTNAME)

          BEGIN_CASE(JSOP_IMPORTALL)
            id = (jsid) JSVAL_VOID;
            PROPERTY_OP(-1, ok = ImportProperty(cx, obj, id));
            sp--;
          END_CASE(JSOP_IMPORTALL)

          BEGIN_CASE(JSOP_IMPORTPROP)
            
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            PROPERTY_OP(-1, ok = ImportProperty(cx, obj, id));
            sp--;
          END_CASE(JSOP_IMPORTPROP)

          BEGIN_CASE(JSOP_IMPORTELEM)
            ELEMENT_OP(-1, ok = ImportProperty(cx, obj, id));
            sp -= 2;
          END_CASE(JSOP_IMPORTELEM)
#endif 

          BEGIN_CASE(JSOP_TRAP)
            SAVE_SP_AND_PC(fp);
            switch (JS_HandleTrap(cx, script, pc, &rval)) {
              case JSTRAP_ERROR:
                ok = JS_FALSE;
                goto out;
              case JSTRAP_CONTINUE:
                JS_ASSERT(JSVAL_IS_INT(rval));
                op = (JSOp) JSVAL_TO_INT(rval);
                JS_ASSERT((uintN)op < (uintN)JSOP_LIMIT);
                LOAD_INTERRUPT_HANDLER(cx);
                DO_OP();
              case JSTRAP_RETURN:
                fp->rval = rval;
                goto out;
              case JSTRAP_THROW:
                cx->throwing = JS_TRUE;
                cx->exception = rval;
                ok = JS_FALSE;
                goto out;
              default:;
            }
            LOAD_INTERRUPT_HANDLER(cx);
          END_CASE(JSOP_TRAP)

          BEGIN_CASE(JSOP_ARGUMENTS)
            SAVE_SP_AND_PC(fp);
            ok = js_GetArgsValue(cx, fp, &rval);
            if (!ok)
                goto out;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_ARGSUB)
            id = INT_TO_JSID(GET_ARGNO(pc));
            SAVE_SP_AND_PC(fp);
            ok = js_GetArgsProperty(cx, fp, id, &rval);
            if (!ok)
                goto out;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
            id = ATOM_TO_JSID(rt->atomState.lengthAtom);
            SAVE_SP_AND_PC(fp);
            ok = js_GetArgsProperty(cx, fp, id, &rval);
            if (!ok)
                goto out;
            PUSH_OPND(rval);
          END_CASE(JSOP_ARGCNT)

#define PUSH_GLOBAL_THIS(cx,sp)                                               \
    JS_BEGIN_MACRO                                                            \
        PUSH_OPND(JSVAL_NULL);                                                \
        SAVE_SP_AND_PC(fp);                                                   \
        ok = ComputeGlobalThis(cx, sp);                                       \
        if (!ok)                                                              \
            goto out;                                                         \
    JS_END_MACRO

          BEGIN_CASE(JSOP_GLOBALTHIS)
            PUSH_GLOBAL_THIS(cx, sp);
          END_CASE(JSOP_GLOBALTHIS)

          BEGIN_CASE(JSOP_GETARG)
          BEGIN_CASE(JSOP_CALLARG)
            slot = GET_ARGNO(pc);
            JS_ASSERT(slot < fp->fun->nargs);
            METER_SLOT_OP(op, slot);
            PUSH_OPND(fp->argv[slot]);
            if (op == JSOP_CALLARG)
                PUSH_GLOBAL_THIS(cx, sp);
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
                PUSH_GLOBAL_THIS(cx, sp);
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
                ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
                if (!ok)
                    goto out;
                STORE_OPND(-1, rval);
            } else {
                slot = JSVAL_TO_INT(lval);
                GC_POKE(cx, STOBJ_GET_SLOT(obj, slot));
                OBJ_SET_SLOT(cx, obj, slot, rval);
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
            ok = js_CheckRedeclaration(cx, obj, id, attrs, &obj2, &prop);
            if (!ok)
                goto out;

            
            if (!prop) {
                ok = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID, NULL, NULL,
                                         attrs, &prop);
                if (!ok)
                    goto out;
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
            fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
            id = ATOM_TO_JSID(fun->atom);

            





























            JS_ASSERT(!fp->blockChain);
            obj2 = fp->scopeChain;
            if (OBJ_GET_PARENT(cx, obj) != obj2) {
                obj = js_CloneFunctionObject(cx, obj, obj2);
                if (!obj) {
                    ok = JS_FALSE;
                    goto out;
                }
            }

            




            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            




            attrs = JSPROP_ENUMERATE;
            if (!(fp->flags & JSFRAME_EVAL))
                attrs |= JSPROP_PERMANENT;

            




            flags = JSFUN_GSFLAG2ATTR(fun->flags);
            if (flags) {
                attrs |= flags | JSPROP_SHARED;
                rval = JSVAL_VOID;
            }

            





            parent = fp->varobj;
            SAVE_SP_AND_PC(fp);
            ok = js_CheckRedeclaration(cx, parent, id, attrs, NULL, NULL);
            if (ok) {
                ok = OBJ_DEFINE_PROPERTY(cx, parent, id, rval,
                                         (flags & JSPROP_GETTER)
                                         ? JS_EXTENSION (JSPropertyOp) obj
                                         : NULL,
                                         (flags & JSPROP_SETTER)
                                         ? JS_EXTENSION (JSPropertyOp) obj
                                         : NULL,
                                         attrs,
                                         &prop);
            }

            
            fp->scopeChain = obj2;
            if (!ok)
                goto out;
            OBJ_DROP_PROPERTY(cx, parent, prop);
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
            LOAD_FUNCTION(VARNO_LEN);

            






            slot = GET_VARNO(pc);

            JS_ASSERT(!fp->blockChain);
            if (!(fp->flags & JSFRAME_POP_BLOCKS)) {
                





                parent = OBJ_GET_PARENT(cx, obj);
                if (OBJ_GET_CLASS(cx, parent) == &js_BlockClass)
                    fp->blockChain = parent;
                parent = js_GetScopeChain(cx, fp);
            } else {
                
















                parent = fp->scopeChain;
                JS_ASSERT(OBJ_GET_CLASS(cx, parent) == &js_BlockClass);
                JS_ASSERT(OBJ_GET_PROTO(cx, parent) == OBJ_GET_PARENT(cx, obj));
                JS_ASSERT(OBJ_GET_CLASS(cx, OBJ_GET_PARENT(cx, parent))
                          == &js_CallClass);
            }

            
            if (OBJ_GET_PARENT(cx, obj) != parent) {
                SAVE_SP_AND_PC(fp);
                obj = js_CloneFunctionObject(cx, obj, parent);
                if (!obj) {
                    ok = JS_FALSE;
                    goto out;
                }
            }

            fp->vars[slot] = OBJECT_TO_JSVAL(obj);
          END_CASE(JSOP_DEFLOCALFUN)

          BEGIN_CASE(JSOP_ANONFUNOBJ)
            
            LOAD_FUNCTION(0);

            
            SAVE_SP_AND_PC(fp);
            parent = js_GetScopeChain(cx, fp);
            if (!parent) {
                ok = JS_FALSE;
                goto out;
            }
            if (OBJ_GET_PARENT(cx, obj) != parent) {
                obj = js_CloneFunctionObject(cx, obj, parent);
                if (!obj) {
                    ok = JS_FALSE;
                    goto out;
                }
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_ANONFUNOBJ)

          BEGIN_CASE(JSOP_NAMEDFUNOBJ)
            
            LOAD_FUNCTION(0);
            rval = OBJECT_TO_JSVAL(obj);

            







            SAVE_SP_AND_PC(fp);
            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2) {
                ok = JS_FALSE;
                goto out;
            }
            parent = js_NewObject(cx, &js_ObjectClass, NULL, obj2);
            if (!parent) {
                ok = JS_FALSE;
                goto out;
            }

            













            fp->scopeChain = parent;
            obj = js_CloneFunctionObject(cx, JSVAL_TO_OBJECT(rval), parent);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }

            




            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            




            fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
            attrs = JSFUN_GSFLAG2ATTR(fun->flags);
            if (attrs) {
                attrs |= JSPROP_SHARED;
                rval = JSVAL_VOID;
            }
            ok = OBJ_DEFINE_PROPERTY(cx, parent, ATOM_TO_JSID(fun->atom), rval,
                                     (attrs & JSPROP_GETTER)
                                     ? JS_EXTENSION (JSPropertyOp) obj
                                     : NULL,
                                     (attrs & JSPROP_SETTER)
                                     ? JS_EXTENSION (JSPropertyOp) obj
                                     : NULL,
                                     attrs |
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     JSPROP_READONLY,
                                     NULL);

            
            fp->scopeChain = obj2;
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                goto out;
            }

            



            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_NAMEDFUNOBJ)

          BEGIN_CASE(JSOP_CLOSURE)
            




            LOAD_FUNCTION(0);

            






            SAVE_SP_AND_PC(fp);
            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2) {
                ok = JS_FALSE;
                goto out;
            }
            if (OBJ_GET_PARENT(cx, obj) != obj2) {
                obj = js_CloneFunctionObject(cx, obj, obj2);
                if (!obj) {
                    ok = JS_FALSE;
                    goto out;
                }
            }

            




            fp->scopeChain = obj;
            rval = OBJECT_TO_JSVAL(obj);

            




            fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
            attrs = JSFUN_GSFLAG2ATTR(fun->flags);
            if (attrs) {
                attrs |= JSPROP_SHARED;
                rval = JSVAL_VOID;
            }
            parent = fp->varobj;
            ok = OBJ_DEFINE_PROPERTY(cx, parent, ATOM_TO_JSID(fun->atom), rval,
                                     (attrs & JSPROP_GETTER)
                                     ? JS_EXTENSION (JSPropertyOp) obj
                                     : NULL,
                                     (attrs & JSPROP_SETTER)
                                     ? JS_EXTENSION (JSPropertyOp) obj
                                     : NULL,
                                     attrs | JSPROP_ENUMERATE
                                           | JSPROP_PERMANENT,
                                     &prop);

            
            fp->scopeChain = obj2;
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                goto out;
            }
            OBJ_DROP_PROPERTY(cx, parent, prop);
          END_CASE(JSOP_CLOSURE)

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

              case JSOP_INITELEM:
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

              default:
                JS_ASSERT(0);
            }

            
            if (id == 0)
                FETCH_ELEMENT_ID(obj, i, id);

            if (JS_TypeOfValue(cx, rval) != JSTYPE_FUNCTION) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_GETTER_OR_SETTER,
                                     (op == JSOP_GETTER)
                                     ? js_getter_str
                                     : js_setter_str);
                ok = JS_FALSE;
                goto out;
            }

            



            ok = OBJ_CHECK_ACCESS(cx, obj, id, JSACC_WATCH, &rtmp, &attrs);
            if (!ok)
                goto out;

            if (op == JSOP_GETTER) {
                getter = JS_EXTENSION (JSPropertyOp) JSVAL_TO_OBJECT(rval);
                setter = NULL;
                attrs = JSPROP_GETTER;
            } else {
                getter = NULL;
                setter = JS_EXTENSION (JSPropertyOp) JSVAL_TO_OBJECT(rval);
                attrs = JSPROP_SETTER;
            }
            attrs |= JSPROP_ENUMERATE | JSPROP_SHARED;

            
            ok = js_CheckRedeclaration(cx, obj, id, attrs, NULL, NULL);
            if (!ok)
                goto out;

            ok = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID, getter, setter,
                                     attrs, NULL);
            if (!ok)
                goto out;

            sp += i;
            if (js_CodeSpec[op2].ndefs)
                STORE_OPND(-1, rval);
            len = js_CodeSpec[op2].length;
            DO_NEXT_OP(len);
#endif 

          BEGIN_CASE(JSOP_NEWINIT)
            argc = 0;
            fp->sharpDepth++;
            goto do_new;

          BEGIN_CASE(JSOP_ENDINIT)
            if (--fp->sharpDepth == 0)
                fp->sharpArray = NULL;

            
            JS_ASSERT(sp - fp->spbase >= 1);
            lval = FETCH_OPND(-1);
            JS_ASSERT(JSVAL_IS_OBJECT(lval));
            cx->weakRoots.newborn[GCX_OBJECT] =
                (JSGCThing *)JSVAL_TO_GCTHING(lval);
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

            
            ok = js_CheckRedeclaration(cx, obj, id, JSPROP_INITIALIZER, NULL,
                                       NULL);
            if (!ok)
                goto out;
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
            sp += i;
            len = js_CodeSpec[op].length;
            DO_NEXT_OP(len);

#if JS_HAS_SHARP_VARS
          BEGIN_CASE(JSOP_DEFSHARP)
            SAVE_SP_AND_PC(fp);
            obj = fp->sharpArray;
            if (!obj) {
                obj = js_NewArrayObject(cx, 0, NULL);
                if (!obj) {
                    ok = JS_FALSE;
                    goto out;
                }
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
                ok = JS_FALSE;
                goto out;
            }
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
          END_CASE(JSOP_DEFSHARP)

          BEGIN_CASE(JSOP_USESHARP)
            i = (jsint) GET_UINT16(pc);
            id = INT_TO_JSID(i);
            obj = fp->sharpArray;
            if (!obj) {
                rval = JSVAL_VOID;
            } else {
                SAVE_SP_AND_PC(fp);
                ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
                if (!ok)
                    goto out;
            }
            if (!JSVAL_IS_OBJECT(rval)) {
                char numBuf[12];
                JS_snprintf(numBuf, sizeof numBuf, "%u", (unsigned) i);

                SAVE_SP_AND_PC(fp);
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_SHARP_USE, numBuf);
                ok = JS_FALSE;
                goto out;
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
                ok = JS_FALSE;
                goto out;
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
            ok = JS_FALSE;
            
            goto out;

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
                ok = JS_FALSE;
                goto out;
            }
            lval = FETCH_OPND(-2);
            cond = JS_FALSE;
            ok = obj->map->ops->hasInstance(cx, obj, lval, &cond);
            if (!ok)
                goto out;
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
                    ok = JS_FALSE;
                    goto out;
                  case JSTRAP_CONTINUE:
                    break;
                  case JSTRAP_RETURN:
                    fp->rval = rval;
                    goto out;
                  case JSTRAP_THROW:
                    cx->throwing = JS_TRUE;
                    cx->exception = rval;
                    ok = JS_FALSE;
                    goto out;
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
            ok = js_SetDefaultXMLNamespace(cx, rval);
            if (!ok)
                goto out;
          END_CASE(JSOP_DEFXMLNS)

          BEGIN_CASE(JSOP_ANYNAME)
            SAVE_SP_AND_PC(fp);
            ok = js_GetAnyName(cx, &rval);
            if (!ok)
                goto out;
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
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_QNAMECONST)

          BEGIN_CASE(JSOP_QNAME)
            rval = FETCH_OPND(-1);
            lval = FETCH_OPND(-2);
            SAVE_SP_AND_PC(fp);
            obj = js_ConstructXMLQNameObject(cx, lval, rval);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            sp--;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_QNAME)

          BEGIN_CASE(JSOP_TOATTRNAME)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            ok = js_ToAttributeName(cx, &rval);
            if (!ok)
                goto out;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_TOATTRNAME)

          BEGIN_CASE(JSOP_TOATTRVAL)
            rval = FETCH_OPND(-1);
            JS_ASSERT(JSVAL_IS_STRING(rval));
            SAVE_SP_AND_PC(fp);
            str = js_EscapeAttributeValue(cx, JSVAL_TO_STRING(rval));
            if (!str) {
                ok = JS_FALSE;
                goto out;
            }
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
            if (!str) {
                ok = JS_FALSE;
                goto out;
            }
            sp--;
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_ADDATTRNAME)

          BEGIN_CASE(JSOP_BINDXMLNAME)
            lval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            ok = js_FindXMLProperty(cx, lval, &obj, &id);
            if (!ok)
                goto out;
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
            PUSH_OPND(ID_TO_VALUE(id));
          END_CASE(JSOP_BINDXMLNAME)

          BEGIN_CASE(JSOP_SETXMLNAME)
            obj = JSVAL_TO_OBJECT(FETCH_OPND(-3));
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            FETCH_ELEMENT_ID(obj, -2, id);
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
            sp -= 2;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_SETXMLNAME)

          BEGIN_CASE(JSOP_CALLXMLNAME)
          BEGIN_CASE(JSOP_XMLNAME)
            lval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            ok = js_FindXMLProperty(cx, lval, &obj, &id);
            if (!ok)
                goto out;
            ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
            STORE_OPND(-1, rval);
            if (op == JSOP_CALLXMLNAME) {
                PUSH_OPND(OBJECT_TO_JSVAL(obj));
                SAVE_SP(fp);
                ok = ComputeThis(cx, sp);
                if (!ok)
                    goto out;
            }
          END_CASE(JSOP_XMLNAME)

          BEGIN_CASE(JSOP_DESCENDANTS)
          BEGIN_CASE(JSOP_DELDESC)
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -2, lval, obj);
            rval = FETCH_OPND(-1);
            ok = js_GetXMLDescendants(cx, obj, rval, &rval);
            if (!ok)
                goto out;

            if (op == JSOP_DELDESC) {
                sp[-1] = rval;          
                ok = js_DeleteXMLListElements(cx, JSVAL_TO_OBJECT(rval));
                if (!ok)
                    goto out;
                rval = JSVAL_TRUE;      
            }

            sp--;
            STORE_OPND(-1, rval);
          END_CASE(JSOP_DESCENDANTS)

          BEGIN_CASE(JSOP_FILTER)
            len = GET_JUMP_OFFSET(pc);
            SAVE_SP_AND_PC(fp);
            FETCH_OBJECT(cx, -1, lval, obj);
            ok = js_FilterXMLList(cx, obj, pc + js_CodeSpec[op].length, &rval);
            if (!ok)
                goto out;
            JS_ASSERT(fp->sp == sp);
            STORE_OPND(-1, rval);
          END_VARLEN_CASE

          BEGIN_CASE(JSOP_ENDFILTER)
            *result = POP_OPND();
            goto out;

          EMPTY_CASE(JSOP_STARTXML)
          EMPTY_CASE(JSOP_STARTXMLEXPR)

          BEGIN_CASE(JSOP_TOXML)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            obj = js_ValueToXMLObject(cx, rval);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_TOXML)

          BEGIN_CASE(JSOP_TOXMLLIST)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            obj = js_ValueToXMLListObject(cx, rval);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_TOXMLLIST)

          BEGIN_CASE(JSOP_XMLTAGEXPR)
            rval = FETCH_OPND(-1);
            SAVE_SP_AND_PC(fp);
            str = js_ValueToString(cx, rval);
            if (!str) {
                ok = JS_FALSE;
                goto out;
            }
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
            if (!str) {
                ok = JS_FALSE;
                goto out;
            }
            STORE_OPND(-1, STRING_TO_JSVAL(str));
          END_CASE(JSOP_XMLELTEXPR)

          BEGIN_CASE(JSOP_XMLOBJECT)
            LOAD_OBJECT(0);
            SAVE_SP_AND_PC(fp);
            obj = js_CloneXMLObject(cx, obj);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLOBJECT)

          BEGIN_CASE(JSOP_XMLCDATA)
            LOAD_ATOM(0);
            str = ATOM_TO_STRING(atom);
            obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_TEXT, NULL, str);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            PUSH_OPND(OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLCDATA)

          BEGIN_CASE(JSOP_XMLCOMMENT)
            LOAD_ATOM(0);
            str = ATOM_TO_STRING(atom);
            obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_COMMENT, NULL, str);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
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
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
          END_CASE(JSOP_XMLPI)

          BEGIN_CASE(JSOP_CALLPROP)
            
            LOAD_ATOM(0);
            id = ATOM_TO_JSID(atom);
            PUSH(JSVAL_NULL);
            SAVE_SP_AND_PC(fp);
            lval = FETCH_OPND(-2);
            if (!JSVAL_IS_PRIMITIVE(lval)) {
                obj = JSVAL_TO_OBJECT(lval);

                
                if (OBJECT_IS_XML(cx, obj)) {
                    JSXMLObjectOps *ops;

                    ops = (JSXMLObjectOps *) obj->map->ops;
                    obj = ops->getMethod(cx, obj, id, &rval);
                    if (!obj)
                        ok = JS_FALSE;
                } else {
                    ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
                }
                if (!ok)
                    goto out;
                STORE_OPND(-1, OBJECT_TO_JSVAL(obj));
                STORE_OPND(-2, rval);
                ok = ComputeThis(cx, sp);
                if (!ok)
                    goto out;
            } else {
                if (JSVAL_IS_STRING(lval)) {
                    i = JSProto_String;
                } else if (JSVAL_IS_NUMBER(lval)) {
                    i = JSProto_Number;
                } else if (JSVAL_IS_BOOLEAN(lval)) {
                    i = JSProto_Boolean;
                } else {
                    JS_ASSERT(JSVAL_IS_NULL(lval) || JSVAL_IS_VOID(lval));
                    js_ReportValueError(cx, JSMSG_NO_PROPERTIES, -2, lval,
                                        NULL);
                    ok = JS_FALSE;
                    goto out;
                }

                ok = js_GetClassPrototype(cx, NULL, INT_TO_JSID(i), &obj);
                if (!ok)
                    goto out;
                JS_ASSERT(obj);
                ok = OBJ_GET_PROPERTY(cx, obj, id, &rval);
                if (!ok)
                    goto out;
                STORE_OPND(-1, lval);
                STORE_OPND(-2, rval);

                
                if (!VALUE_IS_FUNCTION(cx, rval) ||
                    (obj = JSVAL_TO_OBJECT(rval),
                     fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj),
                     !PRIMITIVE_THIS_TEST(fun, lval))) {
                    ok = js_PrimitiveToObject(cx, &sp[-1]);
                    if (!ok)
                        goto out;
                }
            }
          END_CASE(JSOP_CALLPROP)

          BEGIN_CASE(JSOP_GETFUNNS)
            SAVE_SP_AND_PC(fp);
            ok = js_GetFunctionNamespace(cx, &rval);
            if (!ok)
                goto out;
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

                




                parent = fp->scopeChain;
                if (OBJ_GET_PROTO(cx, parent) == obj) {
                    JS_ASSERT(OBJ_GET_CLASS(cx, parent) == &js_BlockClass);
                } else {
                    obj = js_CloneBlockObject(cx, obj, parent, fp);
                    if (!obj) {
                        ok = JS_FALSE;
                        goto out;
                    }
                    fp->scopeChain = obj;
                }
            } else {
                JS_ASSERT(!fp->blockChain ||
                          OBJ_GET_PARENT(cx, obj) == fp->blockChain);
                fp->blockChain = obj;
            }
          END_CASE(JSOP_ENTERBLOCK)

          BEGIN_CASE(JSOP_LEAVEBLOCKEXPR)
          BEGIN_CASE(JSOP_LEAVEBLOCK)
          {
            JSObject **chainp;

            
            if (op == JSOP_LEAVEBLOCKEXPR)
                rval = FETCH_OPND(-1);

            chainp = &fp->blockChain;
            obj = *chainp;
            if (!obj) {
                chainp = &fp->scopeChain;
                obj = *chainp;

                



                SAVE_SP_AND_PC(fp);
                ok = js_PutBlockObject(cx, obj);
                if (!ok)
                    goto out;
            }

            sp -= GET_UINT16(pc);
            JS_ASSERT(fp->spbase <= sp && sp <= fp->spbase + depth);

            
            if (op == JSOP_LEAVEBLOCKEXPR)
                STORE_OPND(-1, rval);

            JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_BlockClass);
            JS_ASSERT(op == JSOP_LEAVEBLOCKEXPR
                      ? fp->spbase + OBJ_BLOCK_DEPTH(cx, obj) == sp - 1
                      : fp->spbase + OBJ_BLOCK_DEPTH(cx, obj) == sp);

            *chainp = OBJ_GET_PARENT(cx, obj);
            JS_ASSERT(chainp != &fp->blockChain ||
                      !*chainp ||
                      OBJ_GET_CLASS(cx, *chainp) == &js_BlockClass);
          }
          END_CASE(JSOP_LEAVEBLOCK)

          BEGIN_CASE(JSOP_GETLOCAL)
          BEGIN_CASE(JSOP_CALLLOCAL)
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            PUSH_OPND(fp->spbase[slot]);
            if (op == JSOP_CALLLOCAL)
                PUSH_GLOBAL_THIS(cx, sp);
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
            --sp;
            if (!ok)
                goto out;
          END_CASE(JSOP_ENDITER)

#if JS_HAS_GENERATORS
          BEGIN_CASE(JSOP_GENERATOR)
            pc += JSOP_GENERATOR_LENGTH;
            SAVE_SP_AND_PC(fp);
            obj = js_NewGenerator(cx, fp);
            if (!obj) {
                ok = JS_FALSE;
            } else {
                JS_ASSERT(!fp->callobj && !fp->argsobj);
                fp->rval = OBJECT_TO_JSVAL(obj);
            }
            goto out;

          BEGIN_CASE(JSOP_YIELD)
            ASSERT_NOT_THROWING(cx);
            if (fp->flags & JSFRAME_FILTERING) {
                
                JS_ReportErrorNumberUC(cx, js_GetErrorMessage, NULL,
                                       JSMSG_YIELD_FROM_FILTER);
                ok = JS_FALSE;
                goto out;
            }
            if (FRAME_TO_GENERATOR(fp)->state == JSGEN_CLOSING) {
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_YIELD,
                                    JSDVG_SEARCH_STACK, fp->argv[-2], NULL);
                ok = JS_FALSE;
                goto out;
            }
            fp->rval = FETCH_OPND(-1);
            fp->flags |= JSFRAME_YIELDING;
            pc += JSOP_YIELD_LENGTH;
            SAVE_SP_AND_PC(fp);
            goto out;

          BEGIN_CASE(JSOP_ARRAYPUSH)
            slot = GET_UINT16(pc);
            JS_ASSERT(slot < (uintN)depth);
            lval = fp->spbase[slot];
            obj  = JSVAL_TO_OBJECT(lval);
            JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_ArrayClass);
            rval = FETCH_OPND(-1);

            





            lval = obj->fslots[JSSLOT_ARRAY_LENGTH];
            JS_ASSERT(JSVAL_IS_INT(lval));
            i = JSVAL_TO_INT(lval);
            if (i == ARRAY_INIT_LIMIT) {
                JS_ReportErrorNumberUC(cx, js_GetErrorMessage, NULL,
                                       JSMSG_ARRAY_INIT_TOO_BIG);
                ok = JS_FALSE;
                goto out;
            }
            id = INT_TO_JSID(i);

            SAVE_SP_AND_PC(fp);
            ok = OBJ_SET_PROPERTY(cx, obj, id, &rval);
            if (!ok)
                goto out;
            --sp;
          END_CASE(JSOP_ARRAYPUSH)
#endif 

#if !JS_HAS_GENERATORS
          L_JSOP_GENERATOR:
          L_JSOP_YIELD:
          L_JSOP_ARRAYPUSH:
#endif

#if !JS_HAS_DESTRUCTURING
          L_JSOP_FOREACHKEYVAL:
          L_JSOP_ENUMCONSTELEM:
#endif

#if JS_THREADED_INTERP
          L_JSOP_BACKPATCH:
          L_JSOP_BACKPATCH_POP:
#else
          default:
#endif
          {
            char numBuf[12];
            JS_snprintf(numBuf, sizeof numBuf, "%d", op);
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_BYTECODE, numBuf);
            ok = JS_FALSE;
            goto out;
          }

#if !JS_THREADED_INTERP

        } 

    advance_pc:
        pc += len;

#ifdef DEBUG
        if (tracefp) {
            intN ndefs, n;
            jsval *siter;

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

out:
    JS_ASSERT((size_t)(pc - script->code) < script->length);
    if (!ok && cx->throwing && !(fp->flags & JSFRAME_FILTERING)) {
        























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
                 goto no_catch;
               case JSTRAP_RETURN:
                 ok = JS_TRUE;
                 cx->throwing = JS_FALSE;
                 fp->rval = rval;
                 goto no_catch;
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

             





             ok = JS_TRUE;
             i = tn->stackDepth;
             for (obj = fp->blockChain; obj; obj = OBJ_GET_PARENT(cx, obj)) {
                 JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_BlockClass);
                 if (OBJ_BLOCK_DEPTH(cx, obj) < i)
                     break;
             }
             fp->blockChain = obj;

             JS_ASSERT(ok);
             for (obj = fp->scopeChain; ; obj = OBJ_GET_PARENT(cx, obj)) {
                 clasp = OBJ_GET_CLASS(cx, obj);
                 if (clasp != &js_WithClass && clasp != &js_BlockClass)
                     break;
                 if (JS_GetPrivate(cx, obj) != fp ||
                     OBJ_BLOCK_DEPTH(cx, obj) < i) {
                     break;
                 }
                 if (clasp == &js_BlockClass) {
                     
                     ok &= js_PutBlockObject(cx, obj);
                 } else {
                     JS_SetPrivate(cx, obj, NULL);
                 }
             }

             fp->scopeChain = obj;
             sp = fp->spbase + i;

             








             offset = tn->start + tn->length;
             pc = (script)->main + offset;
             if (!ok)
                 goto out;

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
                 if (!ok) {
                     




                     goto out;
                 }
                 cx->throwing = JS_TRUE;
                 cx->exception = sp[1];

                 




                 ok = JS_FALSE;
                 break;
             }
         } while (++tn != tnlimit);

       no_catch:;
#if JS_HAS_GENERATORS
         if (JS_UNLIKELY(cx->throwing && cx->exception == JSVAL_ARETURN)) {
            cx->throwing = JS_FALSE;
            ok = JS_TRUE;
            fp->rval = JSVAL_VOID;
        }
#endif
    }

    




    if (inlineCallCount)
        goto inline_return;

    




    if (JS_LIKELY(mark != NULL)) {
        
        if (fp->flags & JSFRAME_POP_BLOCKS) {
            SAVE_SP_AND_PC(fp);
            ok &= PutBlockObjects(cx, fp);
        }

        fp->sp = fp->spbase;
        fp->spbase = NULL;
        js_FreeRawStack(cx, mark);
    } else {
        SAVE_SP(fp);
    }

out2:
    if (cx->version == currentVersion && currentVersion != originalVersion)
        js_SetVersion(cx, originalVersion);
    cx->interpLevel--;
    return ok;

atom_not_defined:
    {
        const char *printable = js_AtomToPrintableString(cx, atom);
        if (printable)
            js_ReportIsNotDefined(cx, printable);
        ok = JS_FALSE;
        goto out;
    }
}
