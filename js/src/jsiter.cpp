










































#include <string.h>     
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsarena.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
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

using namespace js;

JS_STATIC_ASSERT(JSSLOT_ITER_FLAGS < JS_INITIAL_NSLOTS);

#if JS_HAS_GENERATORS

static JS_REQUIRES_STACK JSBool
CloseGenerator(JSContext *cx, JSObject *genobj);

#endif





void
js_CloseNativeIterator(JSContext *cx, JSObject *iterobj)
{
    jsval state;
    JSObject *iterable;

    JS_ASSERT(iterobj->getClass() == &js_IteratorClass);

    
    state = iterobj->getSlot(JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        return;

    
    iterable = iterobj->getParent();
    if (iterable) {
#if JS_HAS_XML_SUPPORT
        uintN flags = JSVAL_TO_INT(iterobj->getSlot(JSSLOT_ITER_FLAGS));
        if ((flags & JSITER_FOREACH) && OBJECT_IS_XML(cx, iterable)) {
            js_EnumerateXMLValues(cx, iterable, JSENUMERATE_DESTROY, &state,
                                  NULL, NULL);
        } else
#endif
            iterable->enumerate(cx, JSENUMERATE_DESTROY, &state, NULL);
    }
    iterobj->setSlot(JSSLOT_ITER_STATE, JSVAL_NULL);
}

static void
iterator_trace(JSTracer *trc, JSObject *obj)
{
    





    JSObject *iterable = obj->getParent();
    if (!iterable) {
        
        return;
    }
    jsval iter_state = obj->fslots[JSSLOT_ITER_STATE];
    js_MarkEnumeratorState(trc, iterable, iter_state);
}

JSClass js_IteratorClass = {
    "Iterator",
    JSCLASS_HAS_RESERVED_SLOTS(2) | 
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator) |
    JSCLASS_MARK_IS_TRACE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   NULL,
    NULL,             NULL,            NULL,            NULL,
    NULL,             NULL,            JS_CLASS_TRACE(iterator_trace), NULL
};

static JSBool
InitNativeIterator(JSContext *cx, JSObject *iterobj, JSObject *obj, uintN flags)
{
    jsval state;
    JSBool ok;

    JS_ASSERT(iterobj->getClass() == &js_IteratorClass);

    
    iterobj->setParent(obj);
    iterobj->setSlot(JSSLOT_ITER_STATE, JSVAL_NULL);
    iterobj->setSlot(JSSLOT_ITER_FLAGS, INT_TO_JSVAL(flags));
    if (!js_RegisterCloseableIterator(cx, iterobj))
        return JS_FALSE;
    if (!obj)
        return JS_TRUE;

    ok =
#if JS_HAS_XML_SUPPORT
         ((flags & JSITER_FOREACH) && OBJECT_IS_XML(cx, obj))
         ? js_EnumerateXMLValues(cx, obj, JSENUMERATE_INIT, &state, NULL, NULL)
         :
#endif
           obj->enumerate(cx, JSENUMERATE_INIT, &state, NULL);
    if (!ok)
        return JS_FALSE;

    iterobj->setSlot(JSSLOT_ITER_STATE, state);
    if (flags & JSITER_ENUMERATE) {
        





        JS_ASSERT(obj != iterobj);
        iterobj->setProto(obj);
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
    jsval vec[2] = { ID_TO_VALUE(key), val };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vec), vec);

    JSObject *aobj = js_NewArrayObject(cx, 2, vec);
    *rval = OBJECT_TO_JSVAL(aobj);
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

    JS_ASSERT(obj->getClass() == &js_IteratorClass);

    iterable = obj->getParent();
    JS_ASSERT(iterable);
    state = obj->getSlot(JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        goto stop;

    flags = JSVAL_TO_INT(obj->getSlot(JSSLOT_ITER_FLAGS));
    JS_ASSERT(!(flags & JSITER_ENUMERATE));
    foreach = (flags & JSITER_FOREACH) != 0;
    ok =
#if JS_HAS_XML_SUPPORT
         (foreach && OBJECT_IS_XML(cx, iterable))
         ? js_EnumerateXMLValues(cx, iterable, JSENUMERATE_NEXT, &state,
                                 &id, rval)
         :
#endif
           iterable->enumerate(cx, JSENUMERATE_NEXT, &state, &id);
    if (!ok)
        return JS_FALSE;

    obj->setSlot(JSSLOT_ITER_STATE, state);
    if (JSVAL_IS_NULL(state))
        goto stop;

    if (foreach) {
#if JS_HAS_XML_SUPPORT
        if (!OBJECT_IS_XML(cx, iterable) &&
            !iterable->getProperty(cx, id, rval)) {
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
    JS_ASSERT(obj->getSlot(JSSLOT_ITER_STATE) == JSVAL_NULL);
    *rval = JSVAL_HOLE;
    return JS_TRUE;
}

JSBool
js_ThrowStopIteration(JSContext *cx)
{
    jsval v;

    JS_ASSERT(!JS_IsExceptionPending(cx));
    if (js_FindClassObject(cx, NULL, JSProto_StopIteration, &v))
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
    if (iterobj->getClass() != &js_IteratorClass)
        return 0;
    return JSVAL_TO_INT(iterobj->getSlot(JSSLOT_ITER_FLAGS));
}





JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp)
{
    JSObject *obj;
    JSAtom *atom;
    JSClass *clasp;
    JSExtendedClass *xclasp;
    JSObject *iterobj;
    jsval arg;

    JS_ASSERT(!(flags & ~(JSITER_ENUMERATE | JSITER_FOREACH | JSITER_KEYVALUE)));

    
    JS_ASSERT(!(flags & JSITER_KEYVALUE) || (flags & JSITER_FOREACH));

    AutoValueRooter tvr(cx);

    
    if (!JSVAL_IS_PRIMITIVE(*vp)) {
        obj = JSVAL_TO_OBJECT(*vp);
    } else {
        





        if ((flags & JSITER_ENUMERATE)) {
            if (!js_ValueToObject(cx, *vp, &obj))
                return false;
            if (!obj)
                goto default_iter;
        } else {
            obj = js_ValueToNonNullObject(cx, *vp);
            if (!obj)
                return false;
        }
    }

    tvr.setObject(obj);

    clasp = obj->getClass();
    if ((clasp->flags & JSCLASS_IS_EXTENDED) &&
        (xclasp = (JSExtendedClass *) clasp)->iteratorObject) {
        iterobj = xclasp->iteratorObject(cx, obj, !(flags & JSITER_FOREACH));
        if (!iterobj)
            return false;
        *vp = OBJECT_TO_JSVAL(iterobj);
    } else {
        atom = cx->runtime->atomState.iteratorAtom;
        if (!js_GetMethod(cx, obj, ATOM_TO_JSID(atom), JSGET_NO_METHOD_BARRIER, vp))
            return false;
        if (JSVAL_IS_VOID(*vp)) {
          default_iter:
            


            if (flags & JSITER_ENUMERATE) {
                





                if ((iterobj = JS_THREAD_DATA(cx)->cachedIteratorObject) != NULL) {
                    JS_THREAD_DATA(cx)->cachedIteratorObject = NULL;
                } else {
                    if (!(iterobj = js_NewObjectWithGivenProto(cx, &js_IteratorClass, NULL, NULL)))
                        return false;
                }
            } else {
                




                if (!(iterobj = js_NewObject(cx, &js_IteratorClass, NULL, NULL)))
                    return false;
            }

            
            *vp = OBJECT_TO_JSVAL(iterobj);

            if (!InitNativeIterator(cx, iterobj, obj, flags))
                return false;
        } else {
            LeaveTrace(cx);
            arg = BOOLEAN_TO_JSVAL((flags & JSITER_FOREACH) == 0);
            if (!js_InternalInvoke(cx, obj, *vp, JSINVOKE_ITERATOR, 1, &arg, vp))
                return false;
            if (JSVAL_IS_PRIMITIVE(*vp)) {
                js_ReportValueError(cx, JSMSG_BAD_ITERATOR_RETURN, JSDVG_SEARCH_STACK, *vp, NULL);
                return false;
            }
        }
    }

    return true;
}

JS_FRIEND_API(JSBool) JS_FASTCALL
js_CloseIterator(JSContext *cx, jsval v)
{
    JSObject *obj;
    JSClass *clasp;

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));
    obj = JSVAL_TO_OBJECT(v);
    clasp = obj->getClass();

    if (clasp == &js_IteratorClass) {
        js_CloseNativeIterator(cx, obj);

        




        JS_THREAD_DATA(cx)->cachedIteratorObject = obj;
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
JS_DEFINE_CALLINFO_2(FRIEND, BOOL, js_CloseIterator, CONTEXT, JSVAL, 0, nanojit::ACC_STORE_ANY)

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
    JS_ASSERT(iterobj->getClass() == &js_IteratorClass);

    obj = iterobj->getParent();
    origobj = iterobj->getProto();
    state = iterobj->getSlot(JSSLOT_ITER_STATE);
    if (JSVAL_IS_NULL(state))
        goto stop;

    foreach = (flags & JSITER_FOREACH) != 0;
#if JS_HAS_XML_SUPPORT
    



    if (obj == origobj && OBJECT_IS_XML(cx, obj)) {
        if (foreach) {
            if (!js_EnumerateXMLValues(cx, obj, JSENUMERATE_NEXT, &state,
                                       &id, rval)) {
                return JS_FALSE;
            }
        } else {
            if (!obj->enumerate(cx, JSENUMERATE_NEXT, &state, &id))
                return JS_FALSE;
        }
        iterobj->setSlot(JSSLOT_ITER_STATE, state);
        if (JSVAL_IS_NULL(state))
            goto stop;
    } else
#endif
    {
      restart:
        if (!obj->enumerate(cx, JSENUMERATE_NEXT, &state, &id))
            return JS_FALSE;

        iterobj->setSlot(JSSLOT_ITER_STATE, state);
        if (JSVAL_IS_NULL(state)) {
#if JS_HAS_XML_SUPPORT
            if (OBJECT_IS_XML(cx, obj)) {
                





                JS_ASSERT(origobj != obj);
                JS_ASSERT(!OBJECT_IS_XML(cx, origobj));
            } else
#endif
            {
                obj = obj->getProto();
                if (obj) {
                    iterobj->setParent(obj);
                    if (!obj->enumerate(cx, JSENUMERATE_INIT, &state, NULL))
                        return JS_FALSE;
                    iterobj->setSlot(JSSLOT_ITER_STATE, state);
                    if (!JSVAL_IS_NULL(state))
                        goto restart;
                }
            }
            goto stop;
        }

        
        if (!origobj->lookupProperty(cx, id, &obj2, &prop))
            return JS_FALSE;
        if (!prop)
            goto restart;
        obj2->dropProperty(cx, prop);

        






        if (obj != obj2) {
            cond = JS_FALSE;
            clasp = obj2->getClass();
            if (clasp->flags & JSCLASS_IS_EXTENDED) {
                xclasp = (JSExtendedClass *) clasp;
                cond = xclasp->outerObject &&
                    xclasp->outerObject(cx, obj2) == obj;
            }
            if (!cond)
                goto restart;
        }

        if (foreach) {
            
            if (!origobj->getProperty(cx, id, rval))
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
    JS_ASSERT(iterobj->getSlot(JSSLOT_ITER_STATE) == JSVAL_NULL);
    *rval = JSVAL_HOLE;
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_CallIteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval)
{
    uintN flags;

    
    if (iterobj->getClass() == &js_IteratorClass) {
        flags = JSVAL_TO_INT(iterobj->getSlot(JSSLOT_ITER_FLAGS));
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
    JS_ConvertStub,   NULL,
    NULL,             NULL,
    NULL,             NULL,
    NULL,             stopiter_hasInstance,
    NULL,             NULL
};

#if JS_HAS_GENERATORS

static void
generator_finalize(JSContext *cx, JSObject *obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    



    JS_ASSERT(gen->state == JSGEN_NEWBORN ||
              gen->state == JSGEN_CLOSED ||
              gen->state == JSGEN_OPEN);
    cx->free(gen);
}

static void
generator_trace(JSTracer *trc, JSObject *obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    



    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING)
        return;

    JSStackFrame *fp = gen->getFloatingFrame();
    JS_ASSERT(gen->getLiveFrame() == fp);
    TraceValues(trc, gen->floatingStack, fp->argEnd(), "generator slots");
    js_TraceStackFrame(trc, fp);
    TraceValues(trc, fp->slots(), gen->savedRegs.sp, "generator slots");
}

JS_FRIEND_DATA(JSClass) js_GeneratorClass = {
    js_Generator_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_IS_ANONYMOUS |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Generator),
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,  JS_ConvertStub,  generator_finalize,
    NULL,             NULL,            NULL,            NULL,
    NULL,             NULL,            JS_CLASS_TRACE(generator_trace), NULL
};









JS_REQUIRES_STACK JSObject *
js_NewGenerator(JSContext *cx)
{
    JSObject *obj = js_NewObject(cx, &js_GeneratorClass, NULL, NULL);
    if (!obj)
        return NULL;

    
    JSStackFrame *fp = cx->fp;
    uintN argc = fp->argc;
    uintN nargs = JS_MAX(argc, fp->fun->nargs);
    uintN vplen = 2 + nargs;

    
    uintN nbytes = sizeof(JSGenerator) +
                   (-1 + 
                    vplen +
                    ValuesPerStackFrame +
                    fp->script->nslots) * sizeof(jsval);

    JSGenerator *gen = (JSGenerator *) cx->malloc(nbytes);
    if (!gen)
        return NULL;

    
    jsval *vp = gen->floatingStack;
    JSStackFrame *newfp = reinterpret_cast<JSStackFrame *>(vp + vplen);
    jsval *slots = newfp->slots();

    
    gen->obj = obj;
    gen->state = JSGEN_NEWBORN;
    gen->savedRegs.pc = fp->regs->pc;
    JS_ASSERT(fp->regs->sp == fp->slots() + fp->script->nfixed);
    gen->savedRegs.sp = slots + fp->script->nfixed;
    gen->vplen = vplen;
    gen->liveFrame = newfp;

    
    newfp->regs = &gen->savedRegs;
    newfp->imacpc = NULL;
    newfp->callobj = fp->callobj;
    if (fp->callobj) {      
        fp->callobj->setPrivate(newfp);
        fp->callobj = NULL;
    }
    newfp->argsobj = fp->argsobj;
    if (fp->argsobj) {      
        JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(newfp);
        fp->argsobj = NULL;
    }
    newfp->script = fp->script;
    newfp->fun = fp->fun;
    newfp->thisv = fp->thisv;
    newfp->argc = fp->argc;
    newfp->argv = vp + 2;
    newfp->rval = fp->rval;
    newfp->annotation = NULL;
    newfp->scopeChain = fp->scopeChain;
    JS_ASSERT(!fp->blockChain);
    newfp->blockChain = NULL;
    newfp->flags = fp->flags | JSFRAME_GENERATOR | JSFRAME_FLOATING_GENERATOR;

    
    memcpy(vp, fp->argv - 2, vplen * sizeof(jsval));
    memcpy(slots, fp->slots(), fp->script->nfixed * sizeof(jsval));

    obj->setPrivate(gen);
    return obj;
}

JSGenerator *
js_FloatingFrameToGenerator(JSStackFrame *fp)
{
    JS_ASSERT(fp->isGenerator() && fp->isFloatingGenerator());
    char *floatingStackp = (char *)(fp->argv - 2);
    char *p = floatingStackp - offsetof(JSGenerator, floatingStack);
    return reinterpret_cast<JSGenerator *>(p);
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
    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING) {
        js_ReportValueError(cx, JSMSG_NESTING_GENERATOR,
                            JSDVG_SEARCH_STACK, OBJECT_TO_JSVAL(obj),
                            JS_GetFunctionId(gen->getFloatingFrame()->fun));
        return JS_FALSE;
    }

    
    if (!cx->ensureGeneratorStackSpace())
        return JS_FALSE;

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

    JSStackFrame *genfp = gen->getFloatingFrame();
    JSBool ok;
    {
        jsval *genVp = gen->floatingStack;
        uintN vplen = gen->vplen;
        uintN nslots = genfp->script->nslots;

        



        ExecuteFrameGuard frame;
        if (!cx->stack().getExecuteFrame(cx, cx->fp, vplen, nslots, frame)) {
            gen->state = JSGEN_CLOSED;
            return JS_FALSE;
        }

        jsval *vp = frame.getvp();
        JSStackFrame *fp = frame.getFrame();

        



        uintN usedBefore = gen->savedRegs.sp - genVp;
        memcpy(vp, genVp, usedBefore * sizeof(jsval));
        fp->flags &= ~JSFRAME_FLOATING_GENERATOR;
        fp->argv = vp + 2;
        fp->regs = &gen->savedRegs;
        gen->savedRegs.sp = fp->slots() + (gen->savedRegs.sp - genfp->slots());
        JS_ASSERT(uintN(gen->savedRegs.sp - fp->slots()) <= fp->script->nslots);

#ifdef DEBUG
        JSObject *callobjBefore = fp->callobj;
        jsval argsobjBefore = fp->argsobj;
#endif

        





        if (genfp->callobj)
            fp->callobj->setPrivate(fp);
        if (genfp->argsobj)
            JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(fp);
        gen->liveFrame = fp;
        (void)cx->enterGenerator(gen); 

        
        cx->stack().pushExecuteFrame(cx, frame, NULL);

        ok = js_Interpret(cx);

        
        cx->leaveGenerator(gen);
        gen->liveFrame = genfp;
        if (fp->argsobj)
            JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(genfp);
        if (fp->callobj)
            fp->callobj->setPrivate(genfp);

        JS_ASSERT_IF(argsobjBefore, argsobjBefore == fp->argsobj);
        JS_ASSERT_IF(callobjBefore, callobjBefore == fp->callobj);

        
        JS_ASSERT(uintN(gen->savedRegs.sp - fp->slots()) <= fp->script->nslots);
        uintN usedAfter = gen->savedRegs.sp - vp;
        memcpy(genVp, vp, usedAfter * sizeof(jsval));
        genfp->flags |= JSFRAME_FLOATING_GENERATOR;
        genfp->argv = genVp + 2;
        gen->savedRegs.sp = genfp->slots() + (gen->savedRegs.sp - fp->slots());
        JS_ASSERT(uintN(gen->savedRegs.sp - genfp->slots()) <= genfp->script->nslots);
    }

    if (gen->getFloatingFrame()->flags & JSFRAME_YIELDING) {
        
        JS_ASSERT(ok);
        JS_ASSERT(!cx->throwing);
        JS_ASSERT(gen->state == JSGEN_RUNNING);
        JS_ASSERT(op != JSGENOP_CLOSE);
        genfp->flags &= ~JSFRAME_YIELDING;
        gen->state = JSGEN_OPEN;
        return JS_TRUE;
    }

    genfp->rval = JSVAL_VOID;
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
    JS_ASSERT(obj->getClass() == &js_GeneratorClass);

    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
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
    jsval arg;

    LeaveTrace(cx);

    obj = JS_THIS_OBJECT(cx, vp);
    if (!JS_InstanceOf(cx, obj, &js_GeneratorClass, vp + 2))
        return JS_FALSE;

    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen) {
        
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

    arg = ((op == JSGENOP_SEND || op == JSGENOP_THROW) && argc != 0)
          ? vp[2]
          : JSVAL_VOID;
    if (!SendToGenerator(cx, op, obj, gen, arg))
        return JS_FALSE;
    *vp = gen->getFloatingFrame()->rval;
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
    proto->setSlot(JSSLOT_ITER_STATE, JSVAL_NULL);
    proto->setSlot(JSSLOT_ITER_FLAGS, JSVAL_ZERO);

#if JS_HAS_GENERATORS
    
    if (!JS_InitClass(cx, obj, NULL, &js_GeneratorClass, NULL, 0,
                      NULL, generator_methods, NULL, NULL)) {
        return NULL;
    }
#endif

    return JS_InitClass(cx, obj, NULL, &js_StopIterationClass, NULL, 0,
                        NULL, NULL, NULL, NULL);
}
