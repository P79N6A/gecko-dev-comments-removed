










































#include "jsstddef.h"
#include <string.h>     
#include "jstypes.h"
#include "jsutil.h"
#include "jsarena.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsexn.h"
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

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#define JSSLOT_ITER_STATE       (JSSLOT_PRIVATE)
#define JSSLOT_ITER_FLAGS       (JSSLOT_PRIVATE + 1)

#if JSSLOT_ITER_FLAGS >= JS_INITIAL_NSLOTS
#error JS_INITIAL_NSLOTS must be greater than JSSLOT_ITER_FLAGS.
#endif

#if JS_HAS_GENERATORS

static JSBool
CloseGenerator(JSContext *cx, JSObject *genobj);

#endif





void
js_CloseNativeIterator(JSContext *cx, JSObject *iterobj)
{
    jsval state;
    JSObject *iterable;

    JS_ASSERT(STOBJ_GET_CLASS(iterobj) == &js_IteratorClass);

    
    state = STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        return;

    
    iterable = STOBJ_GET_PARENT(iterobj);
    if (iterable) {
#if JS_HAS_XML_SUPPORT
        uintN flags = JSVAL_TO_INT(STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_FLAGS));
        if ((flags & JSITER_FOREACH) && OBJECT_IS_XML(cx, iterable)) {
            ((JSXMLObjectOps *) iterable->map->ops)->
                enumerateValues(cx, iterable, JSENUMERATE_DESTROY, &state,
                                NULL, NULL);
        } else
#endif
            OBJ_ENUMERATE(cx, iterable, JSENUMERATE_DESTROY, &state, NULL);
    }
    STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_STATE, JSVAL_NULL);
}

JSClass js_IteratorClass = {
    "Iterator",
    JSCLASS_HAS_RESERVED_SLOTS(2) | 
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
InitNativeIterator(JSContext *cx, JSObject *iterobj, JSObject *obj, uintN flags)
{
    jsval state;
    JSBool ok;

    JS_ASSERT(STOBJ_GET_CLASS(iterobj) == &js_IteratorClass);

    
    STOBJ_SET_PARENT(iterobj, obj);
    STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_STATE, JSVAL_NULL);
    STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_FLAGS, INT_TO_JSVAL(flags));
    if (!js_RegisterCloseableIterator(cx, iterobj))
        return JS_FALSE;
    if (!obj)
        return JS_TRUE;

    ok =
#if JS_HAS_XML_SUPPORT
         ((flags & JSITER_FOREACH) && OBJECT_IS_XML(cx, obj))
         ? ((JSXMLObjectOps *) obj->map->ops)->
               enumerateValues(cx, obj, JSENUMERATE_INIT, &state, NULL, NULL)
         :
#endif
           OBJ_ENUMERATE(cx, obj, JSENUMERATE_INIT, &state, NULL);
    if (!ok)
        return JS_FALSE;

    STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_STATE, state);
    if (flags & JSITER_ENUMERATE) {
        





        JS_ASSERT(obj != iterobj);
        STOBJ_SET_PROTO(iterobj, obj);
    }
    return JS_TRUE;
}

static JSBool
Iterator(JSContext *cx, JSObject *iterobj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool keyonly;
    uintN flags;
    JSObject *obj;

    keyonly = JS_FALSE;
    if (!js_ValueToBoolean(cx, argv[1], &keyonly))
        return JS_FALSE;
    flags = keyonly ? 0 : JSITER_FOREACH;

    if (cx->fp->flags & JSFRAME_CONSTRUCTING) {
        
        if (!JSVAL_IS_PRIMITIVE(argv[0])) {
            obj = JSVAL_TO_OBJECT(argv[0]);
        } else {
            obj = js_ValueToNonNullObject(cx, argv[0]);
            if (!obj)
                return JS_FALSE;
            argv[0] = OBJECT_TO_JSVAL(obj);
        }
        return InitNativeIterator(cx, iterobj, obj, flags);
    }

    *rval = argv[0];
    return js_ValueToIterator(cx, flags, rval);
}

static JSBool
NewKeyValuePair(JSContext *cx, jsid key, jsval val, jsval *rval)
{
    jsval vec[2];
    JSTempValueRooter tvr;
    JSObject *aobj;

    vec[0] = ID_TO_VALUE(key);
    vec[1] = val;

    JS_PUSH_TEMP_ROOT(cx, 2, vec, &tvr);
    aobj = js_NewArrayObject(cx, 2, vec);
    *rval = OBJECT_TO_JSVAL(aobj);
    JS_POP_TEMP_ROOT(cx, &tvr);

    return aobj != NULL;
}

static JSBool
IteratorNextImpl(JSContext *cx, JSObject *obj, jsval *rval)
{
    JSObject *iterable;
    jsval state;
    uintN flags;
    JSBool foreach, ok;
    jsid id;

    JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_IteratorClass);

    iterable = OBJ_GET_PARENT(cx, obj);
    JS_ASSERT(iterable);
    state = OBJ_GET_SLOT(cx, obj, JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        goto stop;

    flags = JSVAL_TO_INT(OBJ_GET_SLOT(cx, obj, JSSLOT_ITER_FLAGS));
    JS_ASSERT(!(flags & JSITER_ENUMERATE));
    foreach = (flags & JSITER_FOREACH) != 0;
    ok =
#if JS_HAS_XML_SUPPORT
         (foreach && OBJECT_IS_XML(cx, iterable))
         ? ((JSXMLObjectOps *) iterable->map->ops)->
               enumerateValues(cx, iterable, JSENUMERATE_NEXT, &state,
                               &id, rval)
         :
#endif
           OBJ_ENUMERATE(cx, iterable, JSENUMERATE_NEXT, &state, &id);
    if (!ok)
        return JS_FALSE;

    OBJ_SET_SLOT(cx, obj, JSSLOT_ITER_STATE, state);
    if (JSVAL_IS_NULL(state))
        goto stop;

    if (foreach) {
#if JS_HAS_XML_SUPPORT
        if (!OBJECT_IS_XML(cx, iterable) &&
            !OBJ_GET_PROPERTY(cx, iterable, id, rval)) {
            return JS_FALSE;
        }
#endif
        if (!NewKeyValuePair(cx, id, *rval, rval))
            return JS_FALSE;
    } else {
        *rval = ID_TO_VALUE(id);
    }
    return JS_TRUE;

  stop:
    JS_ASSERT(OBJ_GET_SLOT(cx, obj, JSSLOT_ITER_STATE) == JSVAL_NULL);
    *rval = JSVAL_HOLE;
    return JS_TRUE;
}

static JSBool
js_ThrowStopIteration(JSContext *cx, JSObject *obj)
{
    jsval v;

    JS_ASSERT(!JS_IsExceptionPending(cx));
    if (js_FindClassObject(cx, NULL, INT_TO_JSID(JSProto_StopIteration), &v))
        JS_SetPendingException(cx, v);
    return JS_FALSE;
}

static JSBool
iterator_next(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    if (!JS_InstanceOf(cx, obj, &js_IteratorClass, argv))
        return JS_FALSE;

    if (!IteratorNextImpl(cx, obj, rval))
        return JS_FALSE;

    if (*rval == JSVAL_HOLE) {
        *rval = JSVAL_NULL;
        js_ThrowStopIteration(cx, obj);
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
iterator_self(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSFunctionSpec iterator_methods[] = {
    {js_iterator_str, iterator_self, 0,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {js_next_str,     iterator_next, 0,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {0,0,0,0,0}
};

uintN
js_GetNativeIteratorFlags(JSContext *cx, JSObject *iterobj)
{
    if (OBJ_GET_CLASS(cx, iterobj) != &js_IteratorClass)
        return 0;
    return JSVAL_TO_INT(OBJ_GET_SLOT(cx, iterobj, JSSLOT_ITER_FLAGS));
}





JSBool
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp)
{
    JSObject *obj;
    JSTempValueRooter tvr;
    JSAtom *atom;
    JSBool ok;
    JSObject *iterobj;
    jsval arg;

    JS_ASSERT(!(flags & ~(JSITER_ENUMERATE |
                          JSITER_FOREACH |
                          JSITER_KEYVALUE)));

    
    JS_ASSERT(!(flags & JSITER_KEYVALUE) || (flags & JSITER_FOREACH));

    
    if (!JSVAL_IS_PRIMITIVE(*vp)) {
        obj = JSVAL_TO_OBJECT(*vp);
    } else {
        





        if ((flags & JSITER_ENUMERATE)) {
            if (!js_ValueToObject(cx, *vp, &obj))
                return JS_FALSE;
            if (!obj)
                goto default_iter;
        } else {
            obj = js_ValueToNonNullObject(cx, *vp);
            if (!obj)
                return JS_FALSE;
        }
    }

    JS_ASSERT(obj);
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &tvr);

    atom = cx->runtime->atomState.iteratorAtom;
#if JS_HAS_XML_SUPPORT
    if (OBJECT_IS_XML(cx, obj)) {
        if (!js_GetXMLFunction(cx, obj, ATOM_TO_JSID(atom), vp))
            goto bad;
    } else
#endif
    {
        if (!OBJ_GET_PROPERTY(cx, obj, ATOM_TO_JSID(atom), vp))
            goto bad;
    }

    if (JSVAL_IS_VOID(*vp)) {
      default_iter:
        






        iterobj = js_NewObject(cx, &js_IteratorClass, NULL, NULL);
        if (!iterobj)
            goto bad;

        
        *vp = OBJECT_TO_JSVAL(iterobj);

        if (!InitNativeIterator(cx, iterobj, obj, flags))
            goto bad;
    } else {
        arg = BOOLEAN_TO_JSVAL((flags & JSITER_FOREACH) == 0);
        if (!js_InternalInvoke(cx, obj, *vp, JSINVOKE_ITERATOR, 1, &arg, vp))
            goto bad;
        if (JSVAL_IS_PRIMITIVE(*vp)) {
            const char *printable = js_AtomToPrintableString(cx, atom);
            if (printable) {
                js_ReportValueError2(cx, JSMSG_BAD_ITERATOR_RETURN,
                                     JSDVG_SEARCH_STACK, *vp, NULL, printable);
            }
            goto bad;
        }
    }

    ok = JS_TRUE;
  out:
    if (obj)
        JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
  bad:
    ok = JS_FALSE;
    goto out;
}

JSBool
js_CloseIterator(JSContext *cx, jsval v)
{
    JSObject *obj;
    JSClass *clasp;

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));
    obj = JSVAL_TO_OBJECT(v);
    clasp = OBJ_GET_CLASS(cx, obj);

    if (clasp == &js_IteratorClass) {
        js_CloseNativeIterator(cx, obj);
    }
#if JS_HAS_GENERATORS
    else if (clasp == &js_GeneratorClass) {
        if (!CloseGenerator(cx, obj))
            return JS_FALSE;
    }
#endif
    return JS_TRUE;
}

static JSBool
CallEnumeratorNext(JSContext *cx, JSObject *iterobj, uintN flags, jsval *rval)
{
    JSObject *obj, *origobj;
    jsval state;
    JSBool foreach;
    jsid id;
    JSObject *obj2;
    JSBool cond;
    JSClass *clasp;
    JSExtendedClass *xclasp;
    JSProperty *prop;
    JSString *str;

    JS_ASSERT(flags & JSITER_ENUMERATE);
    JS_ASSERT(STOBJ_GET_CLASS(iterobj) == &js_IteratorClass);

    obj = STOBJ_GET_PARENT(iterobj);
    origobj = STOBJ_GET_PROTO(iterobj);
    state = STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        goto stop;

    foreach = (flags & JSITER_FOREACH) != 0;
#if JS_HAS_XML_SUPPORT
    



    if (obj == origobj && OBJECT_IS_XML(cx, obj)) {
        if (foreach) {
            JSXMLObjectOps *xmlops = (JSXMLObjectOps *) obj->map->ops;

            if (!xmlops->enumerateValues(cx, obj, JSENUMERATE_NEXT, &state,
                                         &id, rval)) {
                return JS_FALSE;
            }
        } else {
            if (!OBJ_ENUMERATE(cx, obj, JSENUMERATE_NEXT, &state, &id))
                return JS_FALSE;
        }
        STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_STATE, state);
        if (JSVAL_IS_NULL(state))
            goto stop;
    } else
#endif
    {
      restart:
        if (!OBJ_ENUMERATE(cx, obj, JSENUMERATE_NEXT, &state, &id))
            return JS_TRUE;

        STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_STATE, state);
        if (JSVAL_IS_NULL(state)) {
#if JS_HAS_XML_SUPPORT
            if (OBJECT_IS_XML(cx, obj)) {
                





                JS_ASSERT(origobj != obj);
                JS_ASSERT(!OBJECT_IS_XML(cx, origobj));
            } else
#endif
            {
                obj = OBJ_GET_PROTO(cx, obj);
                if (obj) {
                    STOBJ_SET_PARENT(iterobj, obj);
                    if (!OBJ_ENUMERATE(cx, obj, JSENUMERATE_INIT, &state, NULL))
                        return JS_FALSE;
                    STOBJ_SET_SLOT(iterobj, JSSLOT_ITER_STATE, state);
                    if (!JSVAL_IS_NULL(state))
                        goto restart;
                }
            }
            goto stop;
        }

        
        if (!OBJ_LOOKUP_PROPERTY(cx, origobj, id, &obj2, &prop))
            return JS_FALSE;
        if (!prop)
            goto restart;
        OBJ_DROP_PROPERTY(cx, obj2, prop);

        






        if (obj != obj2) {
            cond = JS_FALSE;
            clasp = OBJ_GET_CLASS(cx, obj2);
            if (clasp->flags & JSCLASS_IS_EXTENDED) {
                xclasp = (JSExtendedClass *) clasp;
                cond = xclasp->outerObject &&
                    xclasp->outerObject(cx, obj2) == obj;
            }
            if (!cond)
                goto restart;
        }

        if (foreach) {
            
            if (!OBJ_GET_PROPERTY(cx, origobj, id, rval))
                return JS_FALSE;
        }
    }

    if (foreach) {
        if (flags & JSITER_KEYVALUE) {
            if (!NewKeyValuePair(cx, id, *rval, rval))
                return JS_FALSE;
        }
    } else {
        
        if (JSID_IS_ATOM(id)) {
            *rval = ATOM_KEY(JSID_TO_ATOM(id));
        }
#if JS_HAS_XML_SUPPORT
        else if (JSID_IS_OBJECT(id)) {
            str = js_ValueToString(cx, OBJECT_JSID_TO_JSVAL(id));
            if (!str)
                return JS_FALSE;
            *rval = STRING_TO_JSVAL(str);
        }
#endif
        else {
            str = js_NumberToString(cx, (jsdouble)JSID_TO_INT(id));
            if (!str)
                return JS_FALSE;
            *rval = STRING_TO_JSVAL(str);
        }
    }
    return JS_TRUE;

  stop:
    JS_ASSERT(STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_STATE) == JSVAL_NULL);
    *rval = JSVAL_HOLE;
    return JS_TRUE;
}

JSBool
js_CallIteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval)
{
    uintN flags;

    
    if (OBJ_GET_CLASS(cx, iterobj) == &js_IteratorClass) {
        flags = JSVAL_TO_INT(OBJ_GET_SLOT(cx, iterobj, JSSLOT_ITER_FLAGS));
        if (flags & JSITER_ENUMERATE)
            return CallEnumeratorNext(cx, iterobj, flags, rval);

        



        if (!IteratorNextImpl(cx, iterobj, rval))
            return JS_FALSE;
    } else {
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.nextAtom);

        if (!JS_GetMethodById(cx, iterobj, id, &iterobj, rval))
            return JS_FALSE;
        if (!js_InternalCall(cx, iterobj, *rval, 0, NULL, rval)) {
            
            if (!cx->throwing ||
                JSVAL_IS_PRIMITIVE(cx->exception) ||
                OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(cx->exception))
                    != &js_StopIterationClass) {
                return JS_FALSE;
            }

            
            cx->throwing = JS_FALSE;
            cx->exception = JSVAL_VOID;
            *rval = JSVAL_HOLE;
            return JS_TRUE;
        }
    }

    return JS_TRUE;
}

static JSBool
stopiter_hasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    *bp = !JSVAL_IS_PRIMITIVE(v) &&
          OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_StopIterationClass;
    return JS_TRUE;
}

JSClass js_StopIterationClass = {
    js_StopIteration_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_StopIteration),
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   JS_FinalizeStub,
    NULL,             NULL,
    NULL,             NULL,
    NULL,             stopiter_hasInstance,
    NULL,             NULL
};

#if JS_HAS_GENERATORS

static void
generator_finalize(JSContext *cx, JSObject *obj)
{
    JSGenerator *gen;

    gen = (JSGenerator *) JS_GetPrivate(cx, obj);
    if (gen) {
        



        JS_ASSERT(gen->state == JSGEN_NEWBORN || gen->state == JSGEN_CLOSED ||
                  gen->state == JSGEN_OPEN);
        JS_free(cx, gen);
    }
}

static void
generator_trace(JSTracer *trc, JSObject *obj)
{
    JSGenerator *gen;

    gen = (JSGenerator *) JS_GetPrivate(trc->context, obj);
    if (gen) {
        




        JS_ASSERT(!JSVAL_IS_PRIMITIVE(gen->frame.argv[-2]));
        JS_CALL_OBJECT_TRACER(trc, JSVAL_TO_OBJECT(gen->frame.argv[-2]),
                              "generator");
        js_TraceStackFrame(trc, &gen->frame);
    }
}

JSClass js_GeneratorClass = {
    js_Generator_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_IS_ANONYMOUS |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Generator),
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,  JS_ConvertStub,  generator_finalize,
    NULL,             NULL,            NULL,            NULL,
    NULL,             NULL,            JS_CLASS_TRACE(generator_trace), NULL
};









JSObject *
js_NewGenerator(JSContext *cx, JSStackFrame *fp)
{
    JSObject *obj;
    uintN argc, nargs, nvars, depth, nslots;
    JSGenerator *gen;
    jsval *newsp;

    
    obj = js_NewObject(cx, &js_GeneratorClass, NULL, NULL);
    if (!obj)
        return NULL;

    
    argc = fp->argc;
    nargs = JS_MAX(argc, fp->fun->nargs);
    nvars = fp->nvars;
    depth = fp->script->depth;
    nslots = 2 + nargs + nvars + 2 * depth;

    
    gen = (JSGenerator *)
          JS_malloc(cx, sizeof(JSGenerator) + (nslots - 1) * sizeof(jsval));
    if (!gen)
        goto bad;

    gen->obj = obj;

    
    gen->frame.callobj = fp->callobj;
    if (fp->callobj) {
        JS_SetPrivate(cx, fp->callobj, &gen->frame);
        fp->callobj = NULL;
    }
    gen->frame.argsobj = fp->argsobj;
    if (fp->argsobj) {
        JS_SetPrivate(cx, fp->argsobj, &gen->frame);
        fp->argsobj = NULL;
    }

    
    gen->frame.varobj = fp->varobj;
    gen->frame.thisp = fp->thisp;

    
    gen->frame.script = fp->script;
    gen->frame.fun = fp->fun;

    
    newsp = gen->stack;
    gen->arena.next = NULL;
    gen->arena.base = (jsuword) newsp;
    gen->arena.limit = gen->arena.avail = (jsuword) (newsp + nslots);

#define COPY_STACK_ARRAY(vec,cnt,num)                                         \
    JS_BEGIN_MACRO                                                            \
        gen->frame.cnt = cnt;                                                 \
        gen->frame.vec = newsp;                                               \
        newsp += (num);                                                       \
        memcpy(gen->frame.vec, fp->vec, (num) * sizeof(jsval));               \
    JS_END_MACRO

    
    *newsp++ = fp->argv[-2];
    *newsp++ = fp->argv[-1];
    COPY_STACK_ARRAY(argv, argc, nargs);
    gen->frame.rval = fp->rval;
    COPY_STACK_ARRAY(vars, nvars, nvars);

#undef COPY_STACK_ARRAY

    
    gen->frame.down = NULL;
    gen->frame.annotation = NULL;
    gen->frame.scopeChain = fp->scopeChain;
    gen->frame.pc = fp->pc;

    
    gen->frame.spbase = gen->frame.sp = newsp + depth;

    
    gen->frame.sharpDepth = 0;
    gen->frame.sharpArray = NULL;
    gen->frame.flags = fp->flags | JSFRAME_GENERATOR;
    gen->frame.dormantNext = NULL;
    gen->frame.xmlNamespace = NULL;
    gen->frame.blockChain = NULL;

    
    gen->state = JSGEN_NEWBORN;

    if (!JS_SetPrivate(cx, obj, gen)) {
        JS_free(cx, gen);
        goto bad;
    }
    return obj;

  bad:
    cx->weakRoots.newborn[GCX_OBJECT] = NULL;
    return NULL;
}

typedef enum JSGeneratorOp {
    JSGENOP_NEXT,
    JSGENOP_SEND,
    JSGENOP_THROW,
    JSGENOP_CLOSE
} JSGeneratorOp;





static JSBool
SendToGenerator(JSContext *cx, JSGeneratorOp op, JSObject *obj,
                JSGenerator *gen, jsval arg, jsval *rval)
{
    JSStackFrame *fp;
    jsval junk;
    JSArena *arena;
    JSBool ok;

    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING) {
        js_ReportValueError(cx, JSMSG_NESTING_GENERATOR,
                            JSDVG_SEARCH_STACK, OBJECT_TO_JSVAL(obj),
                            JS_GetFunctionId(gen->frame.fun));
        return JS_FALSE;
    }

    JS_ASSERT(gen->state ==  JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
    switch (op) {
      case JSGENOP_NEXT:
      case JSGENOP_SEND:
        if (gen->state == JSGEN_OPEN) {
            



            gen->frame.sp[-1] = arg;
        }
        gen->state = JSGEN_RUNNING;
        break;

      case JSGENOP_THROW:
        JS_SetPendingException(cx, arg);
        gen->state = JSGEN_RUNNING;
        break;

      default:
        JS_ASSERT(op == JSGENOP_CLOSE);
        JS_SetPendingException(cx, JSVAL_ARETURN);
        gen->state = JSGEN_CLOSING;
        break;
    }

    
    arena = cx->stackPool.current;
    JS_ASSERT(!arena->next);
    JS_ASSERT(!gen->arena.next);
    JS_ASSERT(cx->stackPool.current != &gen->arena);
    cx->stackPool.current = arena->next = &gen->arena;

    
    fp = cx->fp;
    cx->fp = &gen->frame;
    gen->frame.down = fp;
    ok = js_Interpret(cx, gen->frame.pc, &junk);
    cx->fp = fp;
    gen->frame.down = NULL;

    
    JS_ASSERT(!gen->arena.next);
    JS_ASSERT(arena->next == &gen->arena);
    JS_ASSERT(cx->stackPool.current == &gen->arena);
    cx->stackPool.current = arena;
    arena->next = NULL;

    if (gen->frame.flags & JSFRAME_YIELDING) {
        
        JS_ASSERT(ok);
        JS_ASSERT(!cx->throwing);
        JS_ASSERT(gen->state == JSGEN_RUNNING);
        JS_ASSERT(op != JSGENOP_CLOSE);
        gen->frame.flags &= ~JSFRAME_YIELDING;
        gen->state = JSGEN_OPEN;
        *rval = gen->frame.rval;
        return JS_TRUE;
    }

    gen->state = JSGEN_CLOSED;

    if (ok) {
        
        if (op == JSGENOP_CLOSE)
            return JS_TRUE;
        return js_ThrowStopIteration(cx, obj);
    }

    



    return JS_FALSE;
}

static JSBool
CloseGenerator(JSContext *cx, JSObject *obj)
{
    JSGenerator *gen;

    JS_ASSERT(STOBJ_GET_CLASS(obj) == &js_GeneratorClass);
    gen = (JSGenerator *) JS_GetPrivate(cx, obj);
    if (!gen) {
        
        return JS_TRUE;
    }

    if (gen->state == JSGEN_CLOSED)
        return JS_TRUE;

    
    return SendToGenerator(cx, JSGENOP_CLOSE, obj, gen, JSVAL_VOID, NULL);
}




static JSBool
generator_op(JSContext *cx, JSGeneratorOp op,
             JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSGenerator *gen;
    jsval arg;

    if (!JS_InstanceOf(cx, obj, &js_GeneratorClass, argv))
        return JS_FALSE;

    gen = (JSGenerator *) JS_GetPrivate(cx, obj);
    if (gen == NULL) {
        
        goto closed_generator;
    }

    if (gen->state == JSGEN_NEWBORN) {
        switch (op) {
          case JSGENOP_NEXT:
          case JSGENOP_THROW:
            break;

          case JSGENOP_SEND:
            if (!JSVAL_IS_VOID(argv[0])) {
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_SEND,
                                    JSDVG_SEARCH_STACK, argv[0], NULL);
                return JS_FALSE;
            }
            break;

          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            gen->state = JSGEN_CLOSED;
            return JS_TRUE;
        }
    } else if (gen->state == JSGEN_CLOSED) {
      closed_generator:
        switch (op) {
          case JSGENOP_NEXT:
          case JSGENOP_SEND:
            return js_ThrowStopIteration(cx, obj);
          case JSGENOP_THROW:
            JS_SetPendingException(cx, argv[0]);
            return JS_FALSE;
          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            return JS_TRUE;
        }
    }

    arg = (op == JSGENOP_SEND || op == JSGENOP_THROW)
          ? argv[0]
          : JSVAL_VOID;
    if (!SendToGenerator(cx, op, obj, gen, arg, rval))
        return JS_FALSE;
    return JS_TRUE;
}

static JSBool
generator_send(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    return generator_op(cx, JSGENOP_SEND, obj, argc, argv, rval);
}

static JSBool
generator_next(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    return generator_op(cx, JSGENOP_NEXT, obj, argc, argv, rval);
}

static JSBool
generator_throw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
    return generator_op(cx, JSGENOP_THROW, obj, argc, argv, rval);
}

static JSBool
generator_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
    return generator_op(cx, JSGENOP_CLOSE, obj, argc, argv, rval);
}

static JSFunctionSpec generator_methods[] = {
    {js_iterator_str, iterator_self,     0,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {js_next_str,     generator_next,    0,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {js_send_str,     generator_send,    1,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {js_throw_str,    generator_throw,   1,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {js_close_str,    generator_close,   0,JSPROP_READONLY|JSPROP_PERMANENT,0},
    {0,0,0,0,0}
};

#endif 

JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj)
{
    JSObject *proto, *stop;

    
    if (!js_GetClassObject(cx, obj, JSProto_StopIteration, &stop))
        return NULL;
    if (stop)
        return stop;

    proto = JS_InitClass(cx, obj, NULL, &js_IteratorClass, Iterator, 2,
                         NULL, iterator_methods, NULL, NULL);
    if (!proto)
        return NULL;
    STOBJ_SET_SLOT(proto, JSSLOT_ITER_STATE, JSVAL_NULL);

#if JS_HAS_GENERATORS
    
    if (!JS_InitClass(cx, obj, NULL, &js_GeneratorClass, NULL, 0,
                      NULL, generator_methods, NULL, NULL)) {
        return NULL;
    }
#endif

    return JS_InitClass(cx, obj, NULL, &js_StopIterationClass, NULL, 0,
                        NULL, NULL, NULL, NULL);
}
