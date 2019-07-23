










































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
#include "jsversion.h"
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
#include "jsstaticcheck.h"
#include "jstracer.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#if JSSLOT_ITER_FLAGS >= JS_INITIAL_NSLOTS
#error JS_INITIAL_NSLOTS must be greater than JSSLOT_ITER_FLAGS.
#endif

#if JS_HAS_GENERATORS

static JS_REQUIRES_STACK JSBool
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

    keyonly = js_ValueToBoolean(argv[1]);
    flags = keyonly ? 0 : JSITER_FOREACH;

    if (JS_IsConstructing(cx)) {
        
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
    state = STOBJ_GET_SLOT(obj, JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        goto stop;

    flags = JSVAL_TO_INT(STOBJ_GET_SLOT(obj, JSSLOT_ITER_FLAGS));
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

    STOBJ_SET_SLOT(obj, JSSLOT_ITER_STATE, state);
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
    JS_ASSERT(STOBJ_GET_SLOT(obj, JSSLOT_ITER_STATE) == JSVAL_NULL);
    *rval = JSVAL_HOLE;
    return JS_TRUE;
}

JSBool
js_ThrowStopIteration(JSContext *cx)
{
    jsval v;

    JS_ASSERT(!JS_IsExceptionPending(cx));
    if (js_FindClassObject(cx, NULL, INT_TO_JSID(JSProto_StopIteration), &v))
        JS_SetPendingException(cx, v);
    return JS_FALSE;
}

static JSBool
iterator_next(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!JS_InstanceOf(cx, obj, &js_IteratorClass, vp + 2))
        return JS_FALSE;

    if (!IteratorNextImpl(cx, obj, vp))
        return JS_FALSE;

    if (*vp == JSVAL_HOLE) {
        *vp = JSVAL_NULL;
        js_ThrowStopIteration(cx);
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
iterator_self(JSContext *cx, uintN argc, jsval *vp)
{
    *vp = JS_THIS(cx, vp);
    return !JSVAL_IS_NULL(*vp);
}

#define JSPROP_ROPERM   (JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec iterator_methods[] = {
    JS_FN(js_iterator_str,  iterator_self,  0,JSPROP_ROPERM),
    JS_FN(js_next_str,      iterator_next,  0,JSPROP_ROPERM),
    JS_FS_END
};

uintN
js_GetNativeIteratorFlags(JSContext *cx, JSObject *iterobj)
{
    if (OBJ_GET_CLASS(cx, iterobj) != &js_IteratorClass)
        return 0;
    return JSVAL_TO_INT(STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_FLAGS));
}





JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp)
{
    JSObject *obj;
    JSTempValueRooter tvr;
    JSAtom *atom;
    JSClass *clasp;
    JSExtendedClass *xclasp;
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

    clasp = OBJ_GET_CLASS(cx, obj);
    if ((clasp->flags & JSCLASS_IS_EXTENDED) &&
        (xclasp = (JSExtendedClass *) clasp)->iteratorObject) {
        iterobj = xclasp->iteratorObject(cx, obj, !(flags & JSITER_FOREACH));
        if (!iterobj)
            goto bad;
        *vp = OBJECT_TO_JSVAL(iterobj);
    } else {
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
            







            iterobj = js_NewObject(cx, &js_IteratorClass, NULL, NULL, 0);
            if (!iterobj)
                goto bad;

            
            *vp = OBJECT_TO_JSVAL(iterobj);

            if (!InitNativeIterator(cx, iterobj, obj, flags))
                goto bad;
        } else {
            js_LeaveTrace(cx);
            arg = BOOLEAN_TO_JSVAL((flags & JSITER_FOREACH) == 0);
            if (!js_InternalInvoke(cx, obj, *vp, JSINVOKE_ITERATOR, 1, &arg,
                                   vp)) {
                goto bad;
            }
            if (JSVAL_IS_PRIMITIVE(*vp)) {
                js_ReportValueError(cx, JSMSG_BAD_ITERATOR_RETURN,
                                    JSDVG_SEARCH_STACK, *vp, NULL);
                goto bad;
            }
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

JS_FRIEND_API(JSBool) JS_FASTCALL
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
        JS_ASSERT_NOT_ON_TRACE(cx);
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
            return JS_FALSE;

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
        
        str = js_ValueToString(cx, ID_TO_VALUE(id));
        if (!str)
            return JS_FALSE;
        *rval = STRING_TO_JSVAL(str);
    }
    return JS_TRUE;

  stop:
    JS_ASSERT(STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_STATE) == JSVAL_NULL);
    *rval = JSVAL_HOLE;
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_CallIteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval)
{
    uintN flags;

    
    if (OBJ_GET_CLASS(cx, iterobj) == &js_IteratorClass) {
        flags = JSVAL_TO_INT(STOBJ_GET_SLOT(iterobj, JSSLOT_ITER_FLAGS));
        if (flags & JSITER_ENUMERATE)
            return CallEnumeratorNext(cx, iterobj, flags, rval);

        



        if (!IteratorNextImpl(cx, iterobj, rval))
            return JS_FALSE;
    } else {
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.nextAtom);

        if (!JS_GetMethodById(cx, iterobj, id, &iterobj, rval))
            return JS_FALSE;
        if (!js_InternalCall(cx, iterobj, *rval, 0, NULL, rval)) {
            
            if (!cx->throwing || !js_ValueIsStopIteration(cx->exception))
                return JS_FALSE;

            
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
    *bp = js_ValueIsStopIteration(v);
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
    if (!gen)
        return;

    




    JS_ASSERT_IF(gen->state != JSGEN_RUNNING && gen->state != JSGEN_CLOSING,
                 !gen->frame.down);

    




    js_TraceStackFrame(trc, &gen->frame);
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
    uintN argc, nargs, nslots;
    JSGenerator *gen;
    jsval *slots;

    
    obj = js_NewObject(cx, &js_GeneratorClass, NULL, NULL, 0);
    if (!obj)
        return NULL;

    
    argc = fp->argc;
    nargs = JS_MAX(argc, fp->fun->nargs);
    nslots = 2 + nargs + fp->script->nslots;

    
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
    gen->frame.callee = fp->callee;
    gen->frame.fun = fp->fun;

    
    slots = gen->slots;
    gen->arena.next = NULL;
    gen->arena.base = (jsuword) slots;
    gen->arena.limit = gen->arena.avail = (jsuword) (slots + nslots);

    
    gen->frame.rval = fp->rval;
    memcpy(slots, fp->argv - 2, (2 + nargs) * sizeof(jsval));
    gen->frame.argc = nargs;
    gen->frame.argv = slots + 2;
    slots += 2 + nargs;
    memcpy(slots, fp->slots, fp->script->nfixed * sizeof(jsval));

    
    gen->frame.down = NULL;
    gen->frame.annotation = NULL;
    gen->frame.scopeChain = fp->scopeChain;

    gen->frame.imacpc = NULL;
    gen->frame.slots = slots;
    JS_ASSERT(StackBase(fp) == fp->regs->sp);
    gen->savedRegs.sp = slots + fp->script->nfixed;
    gen->savedRegs.pc = fp->regs->pc;
    gen->frame.regs = &gen->savedRegs;

    
    gen->frame.sharpDepth = 0;
    gen->frame.sharpArray = NULL;
    gen->frame.flags = (fp->flags & ~JSFRAME_ROOTED_ARGV) | JSFRAME_GENERATOR;
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





static JS_REQUIRES_STACK JSBool
SendToGenerator(JSContext *cx, JSGeneratorOp op, JSObject *obj,
                JSGenerator *gen, jsval arg)
{
    JSStackFrame *fp;
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
            



            gen->savedRegs.sp[-1] = arg;
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

    
    fp = js_GetTopStackFrame(cx);
    cx->fp = &gen->frame;
    gen->frame.down = fp;
    ok = js_Interpret(cx);
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
        return JS_TRUE;
    }

    gen->frame.rval = JSVAL_VOID;
    gen->state = JSGEN_CLOSED;
    if (ok) {
        
        if (op == JSGENOP_CLOSE)
            return JS_TRUE;
        return js_ThrowStopIteration(cx);
    }

    



    return JS_FALSE;
}

static JS_REQUIRES_STACK JSBool
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

    return SendToGenerator(cx, JSGENOP_CLOSE, obj, gen, JSVAL_VOID);
}




static JSBool
generator_op(JSContext *cx, JSGeneratorOp op, jsval *vp, uintN argc)
{
    JSObject *obj;
    JSGenerator *gen;
    jsval arg;

    js_LeaveTrace(cx);

    obj = JS_THIS_OBJECT(cx, vp);
    if (!JS_InstanceOf(cx, obj, &js_GeneratorClass, vp + 2))
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
            if (argc >= 1 && !JSVAL_IS_VOID(vp[2])) {
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_SEND,
                                    JSDVG_SEARCH_STACK, vp[2], NULL);
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
            return js_ThrowStopIteration(cx);
          case JSGENOP_THROW:
            JS_SetPendingException(cx, argc >= 1 ? vp[2] : JSVAL_VOID);
            return JS_FALSE;
          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            return JS_TRUE;
        }
    }

    js_LeaveTrace(cx);

    arg = ((op == JSGENOP_SEND || op == JSGENOP_THROW) && argc != 0)
          ? vp[2]
          : JSVAL_VOID;
    if (!SendToGenerator(cx, op, obj, gen, arg))
        return JS_FALSE;
    *vp = gen->frame.rval;
    return JS_TRUE;
}

static JSBool
generator_send(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_SEND, vp, argc);
}

static JSBool
generator_next(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_NEXT, vp, argc);
}

static JSBool
generator_throw(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_THROW, vp, argc);
}

static JSBool
generator_close(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_CLOSE, vp, argc);
}

static JSFunctionSpec generator_methods[] = {
    JS_FN(js_iterator_str,  iterator_self,      0,JSPROP_ROPERM),
    JS_FN(js_next_str,      generator_next,     0,JSPROP_ROPERM),
    JS_FN(js_send_str,      generator_send,     1,JSPROP_ROPERM),
    JS_FN(js_throw_str,     generator_throw,    1,JSPROP_ROPERM),
    JS_FN(js_close_str,     generator_close,    0,JSPROP_ROPERM),
    JS_FS_END
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
