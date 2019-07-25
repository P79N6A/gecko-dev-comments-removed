










































#include <stdio.h>
#include <string.h>
#include <math.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h" 
#include "jsutil.h" 
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsdate.h"
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
#include "jspropertycache.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsstaticcheck.h"
#include "jstracer.h"
#include "jslibmath.h"
#include "jsvector.h"

#include "jsatominlines.h"
#include "jspropertycacheinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsstrinlines.h"
#include "jsdtracef.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsautooplen.h"

using namespace js;


#if !JS_LONE_INTERPRET ^ defined jsinvoke_cpp___





static JS_REQUIRES_STACK JSBool
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

JS_STATIC_INTERPRET JS_REQUIRES_STACK jsval *
js_AllocRawStack(JSContext *cx, uintN nslots, void **markp)
{
    jsval *sp;

    JS_ASSERT(nslots != 0);
    JS_ASSERT_NOT_ON_TRACE(cx);

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

JS_STATIC_INTERPRET JS_REQUIRES_STACK void
js_FreeRawStack(JSContext *cx, void *mark)
{
    JS_ARENA_RELEASE(&cx->stackPool, mark);
}

JS_REQUIRES_STACK JS_FRIEND_API(jsval *)
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

JS_REQUIRES_STACK JS_FRIEND_API(void)
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
    JSObject *sharedBlock = fp->blockChain;

    if (!sharedBlock) {
        



        JS_ASSERT(!fp->fun ||
                  !(fp->fun->flags & JSFUN_HEAVYWEIGHT) ||
                  fp->callobj);
        JS_ASSERT(fp->scopeChain);
        return fp->scopeChain;
    }

    
    LeaveTrace(cx);

    






    JSObject *limitBlock, *limitClone;
    if (fp->fun && !fp->callobj) {
        JS_ASSERT(fp->scopeChain->getClass() != &js_BlockClass ||
                  fp->scopeChain->getPrivate() != fp);
        if (!js_GetCallObject(cx, fp))
            return NULL;

        
        limitBlock = limitClone = NULL;
    } else {
        





        limitClone = fp->scopeChain;
        while (limitClone->getClass() == &js_WithClass)
            limitClone = limitClone->getParent();
        JS_ASSERT(limitClone);

        

















        limitBlock = limitClone->getProto();

        
        if (limitBlock == sharedBlock)
            return fp->scopeChain;
    }

    






    JSObject *innermostNewChild = js_CloneBlockObject(cx, sharedBlock, fp);
    if (!innermostNewChild)
        return NULL;
    AutoValueRooter tvr(cx, innermostNewChild);

    



    JSObject *newChild = innermostNewChild;
    for (;;) {
        JS_ASSERT(newChild->getProto() == sharedBlock);
        sharedBlock = sharedBlock->getParent();

        
        if (sharedBlock == limitBlock || !sharedBlock)
            break;

        
        JSObject *clone
            = js_CloneBlockObject(cx, sharedBlock, fp);
        if (!clone)
            return NULL;

        newChild->setParent(clone);
        newChild = clone;
    }
    newChild->setParent(fp->scopeChain);


    



    JS_ASSERT_IF(limitBlock &&
                 limitBlock->getClass() == &js_BlockClass &&
                 limitClone->getPrivate() == fp,
                 sharedBlock);

    
    fp->scopeChain = innermostNewChild;
    return fp->scopeChain;
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
        v = obj->getPrimitiveThis();
    }
    *thisvp = v;
    return JS_TRUE;
}


static inline JSObject *
CallThisObjectHook(JSContext *cx, JSObject *obj, jsval *argv)
{
    JSObject *thisp = obj->thisObject(cx);
    if (!thisp)
        return NULL;
    argv[-1] = OBJECT_TO_JSVAL(thisp);
    return thisp;
}
















JS_STATIC_INTERPRET JSObject *
js_ComputeGlobalThis(JSContext *cx, jsval *argv)
{
    JSObject *thisp;

    if (JSVAL_IS_PRIMITIVE(argv[-2]) ||
        !JSVAL_TO_OBJECT(argv[-2])->getParent()) {
        thisp = cx->globalObject;
    } else {
        thisp = JSVAL_TO_OBJECT(argv[-2])->getGlobal();
    }

    return CallThisObjectHook(cx, thisp, argv);
}

static JSObject *
ComputeThis(JSContext *cx, jsval *argv)
{
    JSObject *thisp;

    JS_ASSERT(!JSVAL_IS_NULL(argv[-1]));
    if (!JSVAL_IS_OBJECT(argv[-1])) {
        if (!js_PrimitiveToObject(cx, &argv[-1]))
            return NULL;
        thisp = JSVAL_TO_OBJECT(argv[-1]);
        return thisp;
    } 

    thisp = JSVAL_TO_OBJECT(argv[-1]);
    if (thisp->getClass() == &js_CallClass || thisp->getClass() == &js_BlockClass)
        return js_ComputeGlobalThis(cx, argv);

    return CallThisObjectHook(cx, thisp, argv);
}

JSObject *
js_ComputeThis(JSContext *cx, jsval *argv)
{
    JS_ASSERT(argv[-1] != JSVAL_HOLE);  
    if (JSVAL_IS_NULL(argv[-1]))
        return js_ComputeGlobalThis(cx, argv);
    return ComputeThis(cx, argv);
}

#if JS_HAS_NO_SUCH_METHOD

const uint32 JSSLOT_FOUND_FUNCTION  = JSSLOT_PRIVATE;
const uint32 JSSLOT_SAVED_ID        = JSSLOT_PRIVATE + 1;

JSClass js_NoSuchMethodClass = {
    "NoSuchMethod",
    JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,   JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,    NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};















JS_STATIC_INTERPRET JSBool
js_OnUnknownMethod(JSContext *cx, jsval *vp)
{
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));

    JSObject *obj = JSVAL_TO_OBJECT(vp[1]);
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.noSuchMethodAtom);
    AutoValueRooter tvr(cx, JSVAL_NULL);
    if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, tvr.addr()))
        return false;
    if (JSVAL_IS_PRIMITIVE(tvr.value())) {
        vp[0] = tvr.value();
    } else {
#if JS_HAS_XML_SUPPORT
        
        if (!JSVAL_IS_PRIMITIVE(vp[0])) {
            obj = JSVAL_TO_OBJECT(vp[0]);
            if (!js_IsFunctionQName(cx, obj, &id))
                return false;
            if (id != 0)
                vp[0] = ID_TO_VALUE(id);
        }
#endif
        obj = NewObjectWithGivenProto(cx, &js_NoSuchMethodClass, NULL, NULL);
        if (!obj)
            return false;
        obj->fslots[JSSLOT_FOUND_FUNCTION] = tvr.value();
        obj->fslots[JSSLOT_SAVED_ID] = vp[0];
        vp[0] = OBJECT_TO_JSVAL(obj);
    }
    return true;
}

static JS_REQUIRES_STACK JSBool
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
    JS_ASSERT(obj->getClass() == &js_NoSuchMethodClass);

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
JS_STATIC_ASSERT(JSVAL_SPECIAL == 6);

const uint16 js_PrimitiveTestFlags[] = {
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_STRING,     
    JSFUN_THISP_NUMBER,     
    JSFUN_THISP_BOOLEAN,    
    JSFUN_THISP_NUMBER      
};







JS_REQUIRES_STACK JS_FRIEND_API(JSBool)
js_Invoke(JSContext *cx, uintN argc, jsval *vp, uintN flags)
{
    void *mark;
    CallStack callStack(cx);
    JSStackFrame frame;
    jsval *sp, *argv, *newvp;
    jsval v;
    JSObject *funobj, *parent;
    JSBool ok;
    JSClass *clasp;
    const JSObjectOps *ops;
    JSNative native;
    JSFunction *fun;
    JSScript *script;
    uintN nslots, i;
    uint32 rootedArgsFlag;
    JSInterpreterHook hook;
    void *hookData;
    bool pushCall;

    JS_ASSERT(argc <= JS_ARGS_LENGTH_MAX);

    
    JS_ASSERT((jsval *) cx->stackPool.current->base <= vp);
    JS_ASSERT(vp + 2 + argc <= (jsval *) cx->stackPool.current->avail);

    
    mark = JS_ARENA_MARK(&cx->stackPool);
    MUST_FLOW_THROUGH("out2");
    v = *vp;

    if (JSVAL_IS_PRIMITIVE(v))
        goto bad;

    funobj = JSVAL_TO_OBJECT(v);
    parent = funobj->getParent();
    clasp = funobj->getClass();
    if (clasp != &js_FunctionClass) {
#if JS_HAS_NO_SUCH_METHOD
        if (clasp == &js_NoSuchMethodClass) {
            ok = NoSuchMethod(cx, argc, vp, flags);
            goto out2;
        }
#endif

        
        ops = funobj->map->ops;

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
        
        fun = GET_FUNCTION_PRIVATE(cx, funobj);
        nslots = FUN_MINARGS(fun);
        nslots = (nslots > argc) ? nslots - argc : 0;
        if (FUN_INTERPRETED(fun)) {
            native = NULL;
            script = fun->u.i.script;
            JS_ASSERT(script);

            if (script->isEmpty()) {
                if (flags & JSINVOKE_CONSTRUCT) {
                    JS_ASSERT(!JSVAL_IS_PRIMITIVE(vp[1]));
                    *vp = vp[1];
                } else {
                    *vp = JSVAL_VOID;
                }
                ok = JS_TRUE;
                goto out2;
            }
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
            if (!js_ComputeThis(cx, vp + 2)) {
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

    


    frame.thisv = vp[1];
    frame.callobj = NULL;
    frame.argsobj = NULL;
    frame.script = script;
    frame.fun = fun;
    frame.argc = argc;
    frame.argv = argv;

    
    frame.rval = (flags & JSINVOKE_CONSTRUCT) ? vp[1] : JSVAL_VOID;
    frame.down = cx->fp;
    frame.annotation = NULL;
    frame.scopeChain = NULL;    
    frame.blockChain = NULL;
    frame.regs = NULL;
    frame.imacpc = NULL;
    frame.slots = NULL;
    frame.flags = flags | rootedArgsFlag;
    frame.displaySave = NULL;

    MUST_FLOW_THROUGH("out");
    pushCall = !cx->fp;
    if (pushCall) {
        



        callStack.setInitialFrame(&frame);
        cx->pushCallStack(&callStack);
    }
    cx->fp = &frame;

    
    hook = cx->debugHooks->callHook;
    hookData = NULL;

    if (native) {
        
        if (frame.down) {
            JS_ASSERT(!pushCall);
            frame.scopeChain = frame.down->scopeChain;
        }

        
        if (!frame.scopeChain)
            frame.scopeChain = parent;
    } else {
        
        frame.scopeChain = parent;
        if (JSFUN_HEAVYWEIGHT_TEST(fun->flags)) {
            
            if (!js_GetCallObject(cx, &frame)) {
                ok = JS_FALSE;
                goto out;
            }
        }
        frame.slots = sp - fun->u.i.nvars;
    }

    
    if (hook)
        hookData = hook(cx, &frame, JS_TRUE, 0, cx->debugHooks->callHookData);

    DTrace::enterJSFun(cx, &frame, fun, frame.down, frame.argc, frame.argv);

    
    if (native) {
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif
        
        JSObject *thisp = JSVAL_TO_OBJECT(frame.thisv);
        ok = native(cx, thisp, argc, frame.argv, &frame.rval);
        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
    } else {
        JS_ASSERT(script);
        ok = js_Interpret(cx);
    }

    DTrace::exitJSFun(cx, &frame, fun, frame.rval);

out:
    if (hookData) {
        hook = cx->debugHooks->callHook;
        if (hook)
            hook(cx, &frame, JS_FALSE, &ok, hookData);
    }

    frame.putActivationObjects(cx);

    *vp = frame.rval;

    
    if (pushCall)
        cx->popCallStack();
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

    LeaveTrace(cx);
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
            JSLocalRootStack *lrs = JS_THREAD_DATA(cx)->localRootStack;
            if (lrs) {
                if (js_PushLocalRoot(cx, lrs, *rval) < 0)
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
    LeaveTrace(cx);

    



    JS_CHECK_RECURSION(cx, return JS_FALSE);

    return js_InternalCall(cx, obj, fval, argc, argv, rval);
}

JSBool
js_Execute(JSContext *cx, JSObject *chain, JSScript *script,
           JSStackFrame *down, uintN flags, jsval *result)
{
    if (script->isEmpty()) {
        if (result)
            *result = JSVAL_VOID;
        return JS_TRUE;
    }

    LeaveTrace(cx);

    DTrace::ExecutionScope executionScope(script);

    JSInterpreterHook hook = cx->debugHooks->executeHook;
    void *hookData = NULL;
    JSStackFrame frame;
    CallStack callStack(cx);
    frame.script = script;
    if (down) {
        
        frame.callobj = down->callobj;
        frame.argsobj = down->argsobj;
        frame.fun = (script->staticLevel > 0) ? down->fun : NULL;
        frame.thisv = down->thisv;
        if (down->flags & JSFRAME_COMPUTED_THIS)
            flags |= JSFRAME_COMPUTED_THIS;
        frame.argc = down->argc;
        frame.argv = down->argv;
        frame.annotation = down->annotation;

        






        if (down == cx->fp) {
            callStack.setInitialVarObj(down->varobj(cx));
        } else {
            CallStack *cs = cx->containingCallStack(down);
            callStack.setInitialVarObj(down->varobj(cs));
        }
    } else {
        frame.callobj = NULL;
        frame.argsobj = NULL;
        JSObject *obj = chain;
        if (cx->options & JSOPTION_VAROBJFIX) {
            while (JSObject *tmp = obj->getParent())
                obj = tmp;
        }
        frame.fun = NULL;
        frame.thisv = OBJECT_TO_JSVAL(chain);
        frame.argc = 0;
        frame.argv = NULL;
        frame.annotation = NULL;
        callStack.setInitialVarObj(obj);
    }

    frame.imacpc = NULL;

    struct RawStackGuard {
        JSContext *cx;
        void *mark;
        RawStackGuard(JSContext *cx) : cx(cx), mark(NULL) {}
        ~RawStackGuard() { if (mark) js_FreeRawStack(cx, mark); }
    } rawStackGuard(cx);

    if (script->nslots != 0) {
        frame.slots = js_AllocRawStack(cx, script->nslots, &rawStackGuard.mark);
        if (!frame.slots)
            return false;
        memset(frame.slots, 0, script->nfixed * sizeof(jsval));

#if JS_HAS_SHARP_VARS
        JS_STATIC_ASSERT(SHARP_NSLOTS == 2);

        if (script->hasSharps) {
            JS_ASSERT(script->nfixed >= SHARP_NSLOTS);
            jsval *sharps = &frame.slots[script->nfixed - SHARP_NSLOTS];

            if (down && down->script && down->script->hasSharps) {
                JS_ASSERT(down->script->nfixed >= SHARP_NSLOTS);
                int base = (down->fun && !(down->flags & JSFRAME_SPECIAL))
                           ? down->fun->sharpSlotBase(cx)
                           : down->script->nfixed - SHARP_NSLOTS;
                if (base < 0)
                    return false;
                sharps[0] = down->slots[base];
                sharps[1] = down->slots[base + 1];
            } else {
                sharps[0] = sharps[1] = JSVAL_VOID;
            }
        }
#endif
    } else {
        frame.slots = NULL;
    }

    frame.rval = JSVAL_VOID;
    frame.down = down;
    frame.scopeChain = chain;
    frame.regs = NULL;
    frame.flags = flags;
    frame.blockChain = NULL;

    




    JSStackFrame *oldfp = cx->fp;
    bool newCallStack = !oldfp || oldfp != down;
    if (newCallStack) {
        callStack.setInitialFrame(&frame);
        cx->pushCallStack(&callStack);
    }
    cx->fp = &frame;

    struct FinishGuard {
        JSContext *cx;
        JSStackFrame *oldfp;
        bool newCallStack;
        FinishGuard(JSContext *cx, JSStackFrame *oldfp, bool newCallStack)
            : cx(cx), oldfp(oldfp), newCallStack(newCallStack) {}
        ~FinishGuard() {
            if (newCallStack)
                cx->popCallStack();
            cx->fp = oldfp;
        }
    } finishGuard(cx, oldfp, newCallStack);

    if (!down) {
        OBJ_TO_INNER_OBJECT(cx, chain);
        if (!chain)
            return false;
        frame.scopeChain = chain;

        JSObject *thisp = JSVAL_TO_OBJECT(frame.thisv)->thisObject(cx);
        if (!thisp)
            return false;
        frame.thisv = OBJECT_TO_JSVAL(thisp);
        frame.flags |= JSFRAME_COMPUTED_THIS;
    }

    if (hook) {
        hookData = hook(cx, &frame, JS_TRUE, 0,
                        cx->debugHooks->executeHookData);
    }

    JSBool ok = js_Interpret(cx);
    if (result)
        *result = frame.rval;

    if (hookData) {
        hook = cx->debugHooks->executeHook;
        if (hook)
            hook(cx, &frame, JS_FALSE, &ok, hookData);
    }

    return ok;
}

JSBool
js_CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                      JSObject **objp, JSProperty **propp)
{
    JSObject *obj2;
    JSProperty *prop;
    uintN oldAttrs, report;
    bool isFunction;
    jsval value;
    const char *type, *name;

    




    JS_ASSERT(!objp == !propp);
    JS_ASSERT_IF(propp, !*propp);

    



    JS_ASSERT_IF(attrs & JSPROP_INITIALIZER, attrs == JSPROP_INITIALIZER);
    JS_ASSERT_IF(attrs == JSPROP_INITIALIZER, !propp);

    if (!obj->lookupProperty(cx, id, &obj2, &prop))
        return JS_FALSE;
    if (!prop)
        return JS_TRUE;

    
    if (!obj2->getAttributes(cx, id, prop, &oldAttrs)) {
        obj2->dropProperty(cx, prop);
        return JS_FALSE;
    }

    


    if (!propp) {
        obj2->dropProperty(cx, prop);
        prop = NULL;
    } else {
        *objp = obj2;
        *propp = prop;
    }

    if (attrs == JSPROP_INITIALIZER) {
        
        if (obj2 != obj)
            return JS_TRUE;

        
        JS_ASSERT(!prop);
        report = JSREPORT_WARNING | JSREPORT_STRICT;

#ifdef __GNUC__
        isFunction = false;     
#endif
    } else {
        
        if (((oldAttrs | attrs) & JSPROP_READONLY) == 0) {
            
            if (!(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
                return JS_TRUE;

            







            if ((~(oldAttrs ^ attrs) & (JSPROP_GETTER | JSPROP_SETTER)) == 0)
                return JS_TRUE;

            



            if (!(oldAttrs & JSPROP_PERMANENT))
                return JS_TRUE;
        }
        if (prop)
            obj2->dropProperty(cx, prop);

        report = JSREPORT_ERROR;
        isFunction = (oldAttrs & (JSPROP_GETTER | JSPROP_SETTER)) != 0;
        if (!isFunction) {
            if (!obj->getProperty(cx, id, &value))
                return JS_FALSE;
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
        return JS_FALSE;
    return JS_ReportErrorFlagsAndNumber(cx, report,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_REDECLARED_VAR,
                                        type, name);
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

static inline bool
IsNegativeZero(jsval v)
{
    return JSVAL_IS_DOUBLE(v) && JSDOUBLE_IS_NEGZERO(*JSVAL_TO_DOUBLE(v));
}

static inline bool
IsNaN(jsval v)
{
    return JSVAL_IS_DOUBLE(v) && JSDOUBLE_IS_NaN(*JSVAL_TO_DOUBLE(v));
}

JSBool
js_SameValue(jsval v1, jsval v2, JSContext *cx)
{
    if (IsNegativeZero(v1))
        return IsNegativeZero(v2);
    if (IsNegativeZero(v2))
        return JS_FALSE;
    if (IsNaN(v1) && IsNaN(v2))
        return JS_TRUE;
    return js_StrictlyEqual(cx, v1, v2);
}

JS_REQUIRES_STACK JSBool
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
        
        obj2->getClass() == &js_FunctionClass ||
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
        





        if (!obj2->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                               &vp[1])) {
            return JS_FALSE;
        }
        rval = vp[1];
        proto = JSVAL_IS_OBJECT(rval) ? JSVAL_TO_OBJECT(rval) : NULL;
        parent = obj2->getParent();

        if (obj2->getClass() == &js_FunctionClass) {
            fun2 = GET_FUNCTION_PRIVATE(cx, obj2);
            if (!FUN_INTERPRETED(fun2) && fun2->u.n.clasp)
                clasp = fun2->u.n.clasp;
        }
    }
    obj = NewObject(cx, clasp, proto, parent);
    if (!obj)
        return JS_FALSE;

    
    vp[1] = OBJECT_TO_JSVAL(obj);
    if (!js_Invoke(cx, argc, vp, JSINVOKE_CONSTRUCT))
        return JS_FALSE;

    
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
    return JS_TRUE;
}

JS_STATIC_INTERPRET JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx)
{
    JSObject *withobj;

    withobj = cx->fp->scopeChain;
    JS_ASSERT(withobj->getClass() == &js_WithClass);
    JS_ASSERT(withobj->getPrivate() == cx->fp);
    JS_ASSERT(OBJ_BLOCK_DEPTH(cx, withobj) >= 0);
    cx->fp->scopeChain = withobj->getParent();
    withobj->setPrivate(NULL);
}

JS_REQUIRES_STACK JSClass *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth)
{
    JSClass *clasp;

    clasp = obj->getClass();
    if ((clasp == &js_WithClass || clasp == &js_BlockClass) &&
        obj->getPrivate() == cx->fp &&
        OBJ_BLOCK_DEPTH(cx, obj) >= stackDepth) {
        return clasp;
    }
    return NULL;
}





JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, JSStackFrame *fp, jsint stackDepth,
               JSBool normalUnwind)
{
    JSObject *obj;
    JSClass *clasp;

    JS_ASSERT(stackDepth >= 0);
    JS_ASSERT(StackBase(fp) + stackDepth <= fp->regs->sp);

    for (obj = fp->blockChain; obj; obj = obj->getParent()) {
        JS_ASSERT(obj->getClass() == &js_BlockClass);
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
    if (cs->format & JOF_POST) {
        double d;
        if (!ValueToNumberValue(cx, vp, &d))
            return JS_FALSE;
        (cs->format & JOF_INC) ? ++d : --d;
        return js_NewNumberInRootedValue(cx, d, vp2);
    }

    double d;
    if (!ValueToNumber(cx, *vp, &d))
        return JS_FALSE;
    (cs->format & JOF_INC) ? ++d : --d;
    if (!js_NewNumberInRootedValue(cx, d, vp2))
        return JS_FALSE;
    *vp = *vp2;
    return JS_TRUE;
}

jsval&
js_GetUpvar(JSContext *cx, uintN level, uintN cookie)
{
    level -= UPVAR_FRAME_SKIP(cookie);
    JS_ASSERT(level < JS_DISPLAY_SIZE);

    JSStackFrame *fp = cx->display[level];
    JS_ASSERT(fp->script);

    uintN slot = UPVAR_FRAME_SLOT(cookie);
    jsval *vp;

    if (!fp->fun || (fp->flags & JSFRAME_EVAL)) {
        vp = fp->slots + fp->script->nfixed;
    } else if (slot < fp->fun->nargs) {
        vp = fp->argv;
    } else if (slot == CALLEE_UPVAR_SLOT) {
        vp = &fp->argv[-2];
        slot = 0;
    } else {
        slot -= fp->fun->nargs;
        JS_ASSERT(slot < fp->script->nslots);
        vp = fp->slots;
    }

    return vp[slot];
}

#ifdef DEBUG

JS_STATIC_INTERPRET JS_REQUIRES_STACK void
js_TraceOpcode(JSContext *cx)
{
    FILE *tracefp;
    JSStackFrame *fp;
    JSFrameRegs *regs;
    intN ndefs, n, nuses;
    jsval *siter;
    JSString *str;
    JSOp op;

    tracefp = (FILE *) cx->tracefp;
    JS_ASSERT(tracefp);
    fp = cx->fp;
    regs = fp->regs;

    



    if (cx->tracePrevPc && regs->pc >= fp->script->main) {
        JSOp tracePrevOp = JSOp(*cx->tracePrevPc);
        ndefs = js_GetStackDefs(cx, &js_CodeSpec[tracePrevOp], tracePrevOp,
                                fp->script, cx->tracePrevPc);

        




        if (ndefs != 0 &&
            ndefs < regs->sp - fp->slots) {
            for (n = -ndefs; n < 0; n++) {
                char *bytes = js_DecompileValueGenerator(cx, n, regs->sp[n],
                                                         NULL);
                if (bytes) {
                    fprintf(tracefp, "%s %s",
                            (n == -ndefs) ? "  output:" : ",",
                            bytes);
                    cx->free(bytes);
                } else {
                    JS_ClearPendingException(cx);
                }
            }
            fprintf(tracefp, " @ %u\n", (uintN) (regs->sp - StackBase(fp)));
        }
        fprintf(tracefp, "  stack: ");
        for (siter = StackBase(fp); siter < regs->sp; siter++) {
            str = js_ValueToString(cx, *siter);
            if (!str) {
                fputs("<null>", tracefp);
            } else {
                JS_ClearPendingException(cx);
                js_FileEscapedString(tracefp, str, 0);
            }
            fputc(' ', tracefp);
        }
        fputc('\n', tracefp);
    }

    fprintf(tracefp, "%4u: ",
            js_PCToLineNumber(cx, fp->script, fp->imacpc ? fp->imacpc : regs->pc));
    js_Disassemble1(cx, fp->script, regs->pc,
                    regs->pc - fp->script->code,
                    JS_FALSE, tracefp);
    op = (JSOp) *regs->pc;
    nuses = js_GetStackUses(&js_CodeSpec[op], op, regs->pc);
    if (nuses != 0) {
        for (n = -nuses; n < 0; n++) {
            char *bytes = js_DecompileValueGenerator(cx, n, regs->sp[n],
                                                     NULL);
            if (bytes) {
                fprintf(tracefp, "%s %s",
                        (n == -nuses) ? "  inputs:" : ",",
                        bytes);
                cx->free(bytes);
            } else {
                JS_ClearPendingException(cx);
            }
        }
        fprintf(tracefp, " @ %u\n", (uintN) (regs->sp - StackBase(fp)));
    }
    cx->tracePrevPc = regs->pc;

    
    fflush(tracefp);
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

    graph = (Edge *) js_calloc(nedges * sizeof graph[0]);
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
    js_free(graph);
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

#ifdef JS_REPRMETER


namespace reprmeter {
    enum Repr {
        NONE,
        INT,
        DOUBLE,
        BOOLEAN_PROPER,
        BOOLEAN_OTHER,
        STRING,
        OBJECT_NULL,
        OBJECT_PLAIN,
        FUNCTION_INTERPRETED,
        FUNCTION_FASTNATIVE,
        FUNCTION_SLOWNATIVE,
        ARRAY_SLOW,
        ARRAY_DENSE
    };

    
    static Repr
    GetRepr(jsval v)
    {
        if (JSVAL_IS_INT(v))
            return INT;
        if (JSVAL_IS_DOUBLE(v))
            return DOUBLE;
        if (JSVAL_IS_SPECIAL(v)) {
            return (v == JSVAL_TRUE || v == JSVAL_FALSE)
                   ? BOOLEAN_PROPER
                   : BOOLEAN_OTHER;
        }
        if (JSVAL_IS_STRING(v))
            return STRING;

        JS_ASSERT(JSVAL_IS_OBJECT(v));

        JSObject *obj = JSVAL_TO_OBJECT(v);
        if (VALUE_IS_FUNCTION(cx, v)) {
            JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
            if (FUN_INTERPRETED(fun))
                return FUNCTION_INTERPRETED;
            if (fun->flags & JSFUN_FAST_NATIVE)
                return FUNCTION_FASTNATIVE;
            return FUNCTION_SLOWNATIVE;
        }
        
        
        if (!obj)
            return OBJECT_NULL;
        if (obj->isDenseArray())
            return ARRAY_DENSE;
        if (obj->isArray())
            return ARRAY_SLOW;
        return OBJECT_PLAIN;
    }

    static const char *reprName[] = { "invalid", "int", "double", "bool", "special",
                                      "string", "null", "object", 
                                      "fun:interp", "fun:fast", "fun:slow",
                                      "array:slow", "array:dense" };

    
    
    struct OpInput {
        enum { max_uses = 16 };

        JSOp op;
        Repr uses[max_uses];

        OpInput() : op(JSOp(255)) {
            for (int i = 0; i < max_uses; ++i)
                uses[i] = NONE;
        }

        OpInput(JSOp op) : op(op) {
            for (int i = 0; i < max_uses; ++i)
                uses[i] = NONE;
        }

        
        operator uint32() const {
            uint32 h = op;
            for (int i = 0; i < max_uses; ++i)
                h = h * 7 + uses[i] * 13;
            return h;
        }

        bool operator==(const OpInput &opinput) const {
            if (op != opinput.op)
                return false;
            for (int i = 0; i < max_uses; ++i) {
                if (uses[i] != opinput.uses[i])
                    return false;
            }
            return true;
        }

        OpInput &operator=(const OpInput &opinput) {
            op = opinput.op;
            for (int i = 0; i < max_uses; ++i)
                uses[i] = opinput.uses[i];
            return *this;
        }
    };

    typedef HashMap<OpInput, uint64, DefaultHasher<OpInput>, SystemAllocPolicy> OpInputHistogram;

    OpInputHistogram opinputs;
    bool             opinputsInitialized = false;

    
    
    static void
    MeterRepr(JSStackFrame *fp)
    {
        
        

        if (!opinputsInitialized) {
            opinputs.init();
            opinputsInitialized = true;
        }

        JSOp op = JSOp(*fp->regs->pc);
        unsigned nuses = js_GetStackUses(&js_CodeSpec[op], op, fp->regs->pc);

        
        OpInput opinput(op);
        for (unsigned i = 0; i < nuses; ++i) {
            jsval v = fp->regs->sp[-nuses+i];
            opinput.uses[i] = GetRepr(v);
        }

        OpInputHistogram::AddPtr p = opinputs.lookupForAdd(opinput);
        if (p)
            ++p->value;
        else
            opinputs.add(p, opinput, 1);
    }

    void
    js_DumpReprMeter()
    {
        FILE *f = fopen("/tmp/reprmeter.txt", "w");
        JS_ASSERT(f);
        for (OpInputHistogram::Range r = opinputs.all(); !r.empty(); r.popFront()) {
            const OpInput &o = r.front().key;
            uint64 c = r.front().value;
            fprintf(f, "%3d,%s", o.op, js_CodeName[o.op]);
            for (int i = 0; i < OpInput::max_uses && o.uses[i] != NONE; ++i)
                fprintf(f, ",%s", reprName[o.uses[i]]);
            fprintf(f, ",%llu\n", c);
        }
        fclose(f);
    }
}
#endif 

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
        VALUE_TO_NUMBER(cx, v_, d);                                           \
    JS_END_MACRO

#define FETCH_INT(cx, n, i)                                                   \
    JS_BEGIN_MACRO                                                            \
        if (!ValueToECMAInt32(cx, regs.sp[n], &i))                            \
            goto error;                                                       \
    JS_END_MACRO

#define FETCH_UINT(cx, n, ui)                                                 \
    JS_BEGIN_MACRO                                                            \
        if (!ValueToECMAUint32(cx, regs.sp[n], &ui))                          \
            goto error;                                                       \
    JS_END_MACRO

#define VALUE_TO_NUMBER(cx, v, d)                                             \
    JS_BEGIN_MACRO                                                            \
        if (!ValueToNumber(cx, v, &d))                                        \
            goto error;                                                       \
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
        if (!JSVAL_TO_OBJECT(v)->defaultValue(cx, hint, &regs.sp[n]))         \
            goto error;                                                       \
        v = regs.sp[n];                                                       \
    JS_END_MACRO







#define CAN_DO_FAST_INC_DEC(v)     (((((v) << 1) ^ v) & 0x80000001) == 1)

JS_STATIC_ASSERT(JSVAL_INT == 1);
JS_STATIC_ASSERT(!CAN_DO_FAST_INC_DEC(INT_TO_JSVAL_CONSTEXPR(JSVAL_INT_MIN)));
JS_STATIC_ASSERT(!CAN_DO_FAST_INC_DEC(INT_TO_JSVAL_CONSTEXPR(JSVAL_INT_MAX)));





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

#ifdef JS_REPRMETER
# define METER_REPR(fp)         (reprmeter::MeterRepr(fp))
#else
# define METER_REPR(fp)         ((void) 0)
#endif 








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





#if defined DEBUG && !defined JS_THREADSAFE

# define ASSERT_VALID_PROPERTY_CACHE_HIT(pcoff,obj,pobj,entry)                \
    JS_BEGIN_MACRO                                                            \
        if (!AssertValidPropertyCacheHit(cx, script, regs, pcoff, obj, pobj,  \
                                         entry)) {                            \
            goto error;                                                       \
        }                                                                     \
    JS_END_MACRO

static bool
AssertValidPropertyCacheHit(JSContext *cx, JSScript *script, JSFrameRegs& regs,
                            ptrdiff_t pcoff, JSObject *start, JSObject *found,
                            PropertyCacheEntry *entry)
{
    uint32 sample = cx->runtime->gcNumber;

    JSAtom *atom;
    if (pcoff >= 0)
        GET_ATOM_FROM_BYTECODE(script, regs.pc, pcoff, atom);
    else
        atom = cx->runtime->atomState.lengthAtom;

    JSObject *obj, *pobj;
    JSProperty *prop;
    JSBool ok;

    if (JOF_OPMODE(*regs.pc) == JOF_NAME) {
        ok = js_FindProperty(cx, ATOM_TO_JSID(atom), &obj, &pobj, &prop);
    } else {
        obj = start;
        ok = js_LookupProperty(cx, obj, ATOM_TO_JSID(atom), &pobj, &prop);
    }
    if (!ok)
        return false;
    if (cx->runtime->gcNumber != sample || entry->vshape() != pobj->shape()) {
        pobj->dropProperty(cx, prop);
        return true;
    }
    JS_ASSERT(prop);
    JS_ASSERT(pobj == found);

    JSScopeProperty *sprop = (JSScopeProperty *) prop;
    if (entry->vword.isSlot()) {
        JS_ASSERT(entry->vword.toSlot() == sprop->slot);
        JS_ASSERT(!sprop->isMethod());
    } else if (entry->vword.isSprop()) {
        JS_ASSERT(entry->vword.toSprop() == sprop);
        JS_ASSERT_IF(sprop->isMethod(),
                     sprop->methodValue() == pobj->lockedGetSlot(sprop->slot));
    } else {
        jsval v;
        JS_ASSERT(entry->vword.isObject());
        JS_ASSERT(!entry->vword.isNull());
        JS_ASSERT(pobj->scope()->brandedOrHasMethodBarrier());
        JS_ASSERT(sprop->hasDefaultGetterOrIsMethod());
        JS_ASSERT(SPROP_HAS_VALID_SLOT(sprop, pobj->scope()));
        v = pobj->lockedGetSlot(sprop->slot);
        JS_ASSERT(VALUE_IS_FUNCTION(cx, v));
        JS_ASSERT(entry->vword.toObject() == JSVAL_TO_OBJECT(v));

        if (sprop->isMethod()) {
            JS_ASSERT(js_CodeSpec[*regs.pc].format & JOF_CALLOP);
            JS_ASSERT(sprop->methodValue() == v);
        }
    }

    pobj->dropProperty(cx, prop);
    return true;
}

#else
# define ASSERT_VALID_PROPERTY_CACHE_HIT(pcoff,obj,pobj,entry) ((void) 0)
#endif





JS_STATIC_ASSERT(JSOP_NAME_LENGTH == JSOP_CALLNAME_LENGTH);
JS_STATIC_ASSERT(JSOP_GETGVAR_LENGTH == JSOP_CALLGVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETUPVAR_LENGTH == JSOP_CALLUPVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETUPVAR_DBG_LENGTH == JSOP_CALLUPVAR_DBG_LENGTH);
JS_STATIC_ASSERT(JSOP_GETUPVAR_DBG_LENGTH == JSOP_GETUPVAR_LENGTH);
JS_STATIC_ASSERT(JSOP_GETDSLOT_LENGTH == JSOP_CALLDSLOT_LENGTH);
JS_STATIC_ASSERT(JSOP_GETARG_LENGTH == JSOP_CALLARG_LENGTH);
JS_STATIC_ASSERT(JSOP_GETLOCAL_LENGTH == JSOP_CALLLOCAL_LENGTH);
JS_STATIC_ASSERT(JSOP_XMLNAME_LENGTH == JSOP_CALLXMLNAME_LENGTH);





JS_STATIC_ASSERT(JSOP_DEFFUN_FC_LENGTH == JSOP_DEFFUN_DBGFC_LENGTH);





JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);
JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETMETHOD_LENGTH);
JS_STATIC_ASSERT(JSOP_INITPROP_LENGTH == JSOP_INITMETHOD_LENGTH);


JS_STATIC_ASSERT(JSOP_IFNE_LENGTH == JSOP_IFEQ_LENGTH);
JS_STATIC_ASSERT(JSOP_IFNE == JSOP_IFEQ + 1);


JS_STATIC_ASSERT(JSOP_INCNAME_LENGTH == JSOP_DECNAME_LENGTH);
JS_STATIC_ASSERT(JSOP_INCNAME_LENGTH == JSOP_NAMEINC_LENGTH);
JS_STATIC_ASSERT(JSOP_INCNAME_LENGTH == JSOP_NAMEDEC_LENGTH);

#ifdef JS_TRACER
# define ABORT_RECORDING(cx, reason)                                          \
    JS_BEGIN_MACRO                                                            \
        if (TRACE_RECORDER(cx))                                               \
            AbortRecording(cx, reason);                                       \
    JS_END_MACRO
#else
# define ABORT_RECORDING(cx, reason)    ((void) 0)
#endif





static inline bool
IteratorMore(JSContext *cx, JSObject *iterobj, JSBool *cond, jsval *rval)
{
    if (iterobj->getClass() == &js_IteratorClass.base) {
        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();
        *cond = (ni->props_cursor < ni->props_end);
    } else {
        if (!js_IteratorMore(cx, iterobj, rval))
            return false;
        *cond = (*rval == JSVAL_TRUE);
    }
    return true;
}

static inline bool
IteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval)
{
    if (iterobj->getClass() == &js_IteratorClass.base) {
        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();
        JS_ASSERT(ni->props_cursor < ni->props_end);
        *rval = *ni->props_cursor;
        if (JSVAL_IS_STRING(*rval) || (ni->flags & JSITER_FOREACH)) {
            ni->props_cursor++;
            return true;
        }
        
    }
    return js_IteratorNext(cx, iterobj, rval);
}

JS_REQUIRES_STACK JSBool
js_Interpret(JSContext *cx)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_INTERP);
#endif

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
    int32_t i, j;
    jsdouble d, d2;
    JSClass *clasp;
    JSFunction *fun;
    JSType type;
    jsint low, high, off, npairs;
    JSBool match;
    JSPropertyOp getter, setter;
    JSAutoResolveFlags rf(cx, JSRESOLVE_INFER);

# ifdef DEBUG
    













#  define TRACE_OPCODE(OP)  JS_BEGIN_MACRO                                    \
                                if (JS_UNLIKELY(cx->tracefp != NULL) &&       \
                                    (OP) == *regs.pc)                         \
                                    js_TraceOpcode(cx);                       \
                            JS_END_MACRO
# else
#  define TRACE_OPCODE(OP)  ((void) 0)
# endif

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
                                METER_OP_PAIR(op, JSOp(regs.pc[n]));          \
                                op = (JSOp) *(regs.pc += (n));                \
                                METER_REPR(fp);                               \
                                DO_OP();                                      \
                            JS_END_MACRO

# define BEGIN_CASE(OP)     L_##OP: TRACE_OPCODE(OP); CHECK_RECORDER();
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

    
    JS_CHECK_RECURSION(cx, return JS_FALSE);

    rt = cx->runtime;

    
    fp = cx->fp;
    script = fp->script;
    JS_ASSERT(!script->isEmpty());
    JS_ASSERT(script->length > 1);

    
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
    (obj = script->getObject(GET_FULL_INDEX(PCOFF)))

#define LOAD_FUNCTION(PCOFF)                                                  \
    (fun = script->getFunction(GET_FULL_INDEX(PCOFF)))

#ifdef JS_TRACER

#ifdef MOZ_TRACEVIS
#if JS_THREADED_INTERP
#define MONITOR_BRANCH_TRACEVIS                                               \
    JS_BEGIN_MACRO                                                            \
        if (jumpTable != interruptJumpTable)                                  \
            EnterTraceVisState(cx, S_RECORD, R_NONE);                         \
    JS_END_MACRO
#else 
#define MONITOR_BRANCH_TRACEVIS                                               \
    JS_BEGIN_MACRO                                                            \
        EnterTraceVisState(cx, S_RECORD, R_NONE);                             \
    JS_END_MACRO
#endif
#else
#define MONITOR_BRANCH_TRACEVIS
#endif

#define RESTORE_INTERP_VARS()                                                 \
    JS_BEGIN_MACRO                                                            \
        fp = cx->fp;                                                          \
        script = fp->script;                                                  \
        atoms = FrameAtomBase(cx, fp);                                        \
        currentVersion = (JSVersion) script->version;                         \
        JS_ASSERT(fp->regs == &regs);                                         \
    JS_END_MACRO

#define MONITOR_BRANCH(reason)                                                \
    JS_BEGIN_MACRO                                                            \
        if (TRACING_ENABLED(cx)) {                                            \
            MonitorResult r = MonitorLoopEdge(cx, inlineCallCount, reason);   \
            if (r == MONITOR_RECORDING) {                                     \
                JS_ASSERT(TRACE_RECORDER(cx));                                \
                MONITOR_BRANCH_TRACEVIS;                                      \
                ENABLE_INTERRUPTS();                                          \
            }                                                                 \
            RESTORE_INTERP_VARS();                                            \
            JS_ASSERT_IF(cx->throwing, r == MONITOR_ERROR);                   \
            if (r == MONITOR_ERROR)                                           \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#else 

#define MONITOR_BRANCH(reason) ((void) 0)

#endif 

    



#define CHECK_BRANCH()                                                        \
    JS_BEGIN_MACRO                                                            \
        if (!JS_CHECK_OPERATION_LIMIT(cx))                                    \
            goto error;                                                       \
    JS_END_MACRO

#ifndef TRACE_RECORDER
#define TRACE_RECORDER(cx) (false)
#endif

#define BRANCH(n)                                                             \
    JS_BEGIN_MACRO                                                            \
        regs.pc += (n);                                                       \
        op = (JSOp) *regs.pc;                                                 \
        if ((n) <= 0) {                                                       \
            CHECK_BRANCH();                                                   \
            if (op == JSOP_NOP) {                                             \
                if (TRACE_RECORDER(cx)) {                                     \
                    MONITOR_BRANCH(Record_Branch);                            \
                    op = (JSOp) *regs.pc;                                     \
                } else {                                                      \
                    op = (JSOp) *++regs.pc;                                   \
                }                                                             \
            } else if (op == JSOP_TRACE) {                                    \
                MONITOR_BRANCH(Record_Branch);                                \
                op = (JSOp) *regs.pc;                                         \
            }                                                                 \
        }                                                                     \
        DO_OP();                                                              \
    JS_END_MACRO

    MUST_FLOW_THROUGH("exit");
    ++cx->interpLevel;

    








    currentVersion = (JSVersion) script->version;
    originalVersion = (JSVersion) cx->version;
    if (currentVersion != originalVersion)
        js_SetVersion(cx, currentVersion);

    
    if (script->staticLevel < JS_DISPLAY_SIZE) {
        JSStackFrame **disp = &cx->display[script->staticLevel];
        fp->displaySave = *disp;
        *disp = fp;
    }

# define CHECK_INTERRUPT_HANDLER()                                            \
    JS_BEGIN_MACRO                                                            \
        if (cx->debugHooks->interruptHook)                                    \
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

#ifdef JS_TRACER
    



    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "attempt to reenter interpreter while recording");
#endif

    





    len = 0;
    DO_NEXT_OP(len);

#if JS_THREADED_INTERP
    








#else 
    for (;;) {
      advance_pc_by_one:
        JS_ASSERT(js_CodeSpec[op].length == 1);
        len = 1;
      advance_pc:
        regs.pc += len;
        op = (JSOp) *regs.pc;

      do_op:
        CHECK_RECORDER();
        TRACE_OPCODE(op);
        switchOp = intN(op) | switchMask;
      do_switch:
        switch (switchOp) {
#endif


#include "jsops.cpp"


#if !JS_THREADED_INTERP
          default:
#endif
#ifndef JS_TRACER
        bad_opcode:
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
#ifdef JS_TRACER
    if (fp->imacpc && cx->throwing) {
        
        regs.pc = fp->imacpc;
        fp->imacpc = NULL;
        atoms = script->atomMap.vector;
    }
#endif

    JS_ASSERT((size_t)((fp->imacpc ? fp->imacpc : regs.pc) - script->code) < script->length);

#ifdef JS_TRACER
    




    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "error or exception while recording");
#endif

    if (!cx->throwing) {
        
        ok = JS_FALSE;
    } else {
        JSThrowHook handler;
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
        tn = script->trynotes()->vector;
        tnlimit = tn + script->trynotes()->length;
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
                JS_ASSERT(js_GetOpcode(cx, fp->script, regs.pc) == JSOP_ENTERBLOCK);

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

              case JSTRY_ITER: {
                
                JS_ASSERT(js_GetOpcode(cx, fp->script, regs.pc) == JSOP_ENDITER);
                AutoValueRooter tvr(cx, cx->exception);
                cx->throwing = false;
                ok = js_CloseIterator(cx, regs.sp[-1]);
                regs.sp -= 1;
                if (!ok)
                    goto error;
                cx->throwing = true;
                cx->exception = tvr.value();
              }
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

#ifdef DEBUG
    cx->tracePrevPc = NULL;
#endif

    if (inlineCallCount)
        goto inline_return;

  exit:
    










    JS_ASSERT(inlineCallCount == 0);
    JS_ASSERT(fp->regs == &regs);
#ifdef JS_TRACER
    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "recording out of js_Interpret");
#endif
#if JS_HAS_GENERATORS
    if (JS_UNLIKELY(fp->flags & JSFRAME_YIELDING)) {
        JSGenerator *gen;

        gen = FRAME_TO_GENERATOR(fp);
        gen->savedRegs = regs;
        gen->frame.regs = &gen->savedRegs;
    } else
#endif 
    {
        JS_ASSERT(!fp->blockChain);
        JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));
        fp->regs = NULL;
    }

    
    if (script->staticLevel < JS_DISPLAY_SIZE)
        cx->display[script->staticLevel] = fp->displaySave;
    if (cx->version == currentVersion && currentVersion != originalVersion)
        js_SetVersion(cx, originalVersion);
    --cx->interpLevel;

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
