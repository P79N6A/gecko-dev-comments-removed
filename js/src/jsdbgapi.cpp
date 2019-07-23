










































#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h" 
#include "jsclist.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"

#include "jsatominlines.h"

#include "jsautooplen.h"

typedef struct JSTrap {
    JSCList         links;
    JSScript        *script;
    jsbytecode      *pc;
    JSOp            op;
    JSTrapHandler   handler;
    void            *closure;
} JSTrap;

#define DBG_LOCK(rt)            JS_ACQUIRE_LOCK((rt)->debuggerLock)
#define DBG_UNLOCK(rt)          JS_RELEASE_LOCK((rt)->debuggerLock)
#define DBG_LOCK_EVAL(rt,expr)  (DBG_LOCK(rt), (expr), DBG_UNLOCK(rt))




static JSTrap *
FindTrap(JSRuntime *rt, JSScript *script, jsbytecode *pc)
{
    JSTrap *trap;

    for (trap = (JSTrap *)rt->trapList.next;
         &trap->links != &rt->trapList;
         trap = (JSTrap *)trap->links.next) {
        if (trap->script == script && trap->pc == pc)
            return trap;
    }
    return NULL;
}

jsbytecode *
js_UntrapScriptCode(JSContext *cx, JSScript *script)
{
    jsbytecode *code;
    JSRuntime *rt;
    JSTrap *trap;

    code = script->code;
    rt = cx->runtime;
    DBG_LOCK(rt);
    for (trap = (JSTrap *)rt->trapList.next;
         &trap->links !=
                &rt->trapList;
         trap = (JSTrap *)trap->links.next) {
        if (trap->script == script &&
            (size_t)(trap->pc - script->code) < script->length) {
            if (code == script->code) {
                jssrcnote *sn, *notes;
                size_t nbytes;

                nbytes = script->length * sizeof(jsbytecode);
                notes = SCRIPT_NOTES(script);
                for (sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn))
                    continue;
                nbytes += (sn - notes + 1) * sizeof *sn;

                code = (jsbytecode *) cx->malloc(nbytes);
                if (!code)
                    break;
                memcpy(code, script->code, nbytes);
                JS_PURGE_GSN_CACHE(cx);
            }
            code[trap->pc - script->code] = trap->op;
        }
    }
    DBG_UNLOCK(rt);
    return code;
}

JS_PUBLIC_API(JSBool)
JS_SetTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
           JSTrapHandler handler, void *closure)
{
    JSTrap *junk, *trap, *twin;
    JSRuntime *rt;
    uint32 sample;

    JS_ASSERT((JSOp) *pc != JSOP_TRAP);
    junk = NULL;
    rt = cx->runtime;
    DBG_LOCK(rt);
    trap = FindTrap(rt, script, pc);
    if (trap) {
        JS_ASSERT(trap->script == script && trap->pc == pc);
        JS_ASSERT(*pc == JSOP_TRAP);
    } else {
        sample = rt->debuggerMutations;
        DBG_UNLOCK(rt);
        trap = (JSTrap *) cx->malloc(sizeof *trap);
        if (!trap)
            return JS_FALSE;
        trap->closure = NULL;
        if(!js_AddRoot(cx, &trap->closure, "trap->closure")) {
            cx->free(trap);
            return JS_FALSE;
        }
        DBG_LOCK(rt);
        twin = (rt->debuggerMutations != sample)
               ? FindTrap(rt, script, pc)
               : NULL;
        if (twin) {
            junk = trap;
            trap = twin;
        } else {
            JS_APPEND_LINK(&trap->links, &rt->trapList);
            ++rt->debuggerMutations;
            trap->script = script;
            trap->pc = pc;
            trap->op = (JSOp)*pc;
            *pc = JSOP_TRAP;
        }
    }
    trap->handler = handler;
    trap->closure = closure;
    DBG_UNLOCK(rt);
    if (junk) {
        js_RemoveRoot(rt, &junk->closure);
        cx->free(junk);
    }
    return JS_TRUE;
}

JS_PUBLIC_API(JSOp)
JS_GetTrapOpcode(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSRuntime *rt;
    JSTrap *trap;
    JSOp op;

    rt = cx->runtime;
    DBG_LOCK(rt);
    trap = FindTrap(rt, script, pc);
    op = trap ? trap->op : (JSOp) *pc;
    DBG_UNLOCK(rt);
    return op;
}

static void
DestroyTrapAndUnlock(JSContext *cx, JSTrap *trap)
{
    ++cx->runtime->debuggerMutations;
    JS_REMOVE_LINK(&trap->links);
    *trap->pc = (jsbytecode)trap->op;
    DBG_UNLOCK(cx->runtime);

    js_RemoveRoot(cx->runtime, &trap->closure);
    cx->free(trap);
}

JS_PUBLIC_API(void)
JS_ClearTrap(JSContext *cx, JSScript *script, jsbytecode *pc,
             JSTrapHandler *handlerp, void **closurep)
{
    JSTrap *trap;

    DBG_LOCK(cx->runtime);
    trap = FindTrap(cx->runtime, script, pc);
    if (handlerp)
        *handlerp = trap ? trap->handler : NULL;
    if (closurep)
        *closurep = trap ? trap->closure : NULL;
    if (trap)
        DestroyTrapAndUnlock(cx, trap);
    else
        DBG_UNLOCK(cx->runtime);
}

JS_PUBLIC_API(void)
JS_ClearScriptTraps(JSContext *cx, JSScript *script)
{
    JSRuntime *rt;
    JSTrap *trap, *next;
    uint32 sample;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (trap = (JSTrap *)rt->trapList.next;
         &trap->links != &rt->trapList;
         trap = next) {
        next = (JSTrap *)trap->links.next;
        if (trap->script == script) {
            sample = rt->debuggerMutations;
            DestroyTrapAndUnlock(cx, trap);
            DBG_LOCK(rt);
            if (rt->debuggerMutations != sample + 1)
                next = (JSTrap *)rt->trapList.next;
        }
    }
    DBG_UNLOCK(rt);
}

JS_PUBLIC_API(void)
JS_ClearAllTraps(JSContext *cx)
{
    JSRuntime *rt;
    JSTrap *trap, *next;
    uint32 sample;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (trap = (JSTrap *)rt->trapList.next;
         &trap->links != &rt->trapList;
         trap = next) {
        next = (JSTrap *)trap->links.next;
        sample = rt->debuggerMutations;
        DestroyTrapAndUnlock(cx, trap);
        DBG_LOCK(rt);
        if (rt->debuggerMutations != sample + 1)
            next = (JSTrap *)rt->trapList.next;
    }
    DBG_UNLOCK(rt);
}

JS_PUBLIC_API(JSTrapStatus)
JS_HandleTrap(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval)
{
    JSTrap *trap;
    jsint op;
    JSTrapStatus status;

    DBG_LOCK(cx->runtime);
    trap = FindTrap(cx->runtime, script, pc);
    JS_ASSERT(!trap || trap->handler);
    if (!trap) {
        op = (JSOp) *pc;
        DBG_UNLOCK(cx->runtime);

        
        JS_ASSERT(op != JSOP_TRAP);

#ifdef JS_THREADSAFE
        
        if (op == JSOP_TRAP)
            return JSTRAP_ERROR;

        
        *rval = INT_TO_JSVAL(op);
        return JSTRAP_CONTINUE;
#else
        
        return JSTRAP_ERROR;
#endif
    }
    DBG_UNLOCK(cx->runtime);

    



    op = (jsint)trap->op;
    status = trap->handler(cx, script, pc, rval, trap->closure);
    if (status == JSTRAP_CONTINUE) {
        
        *rval = INT_TO_JSVAL(op);
    }
    return status;
}

JS_PUBLIC_API(JSBool)
JS_SetInterrupt(JSRuntime *rt, JSTrapHandler handler, void *closure)
{
    rt->globalDebugHooks.interruptHandler = handler;
    rt->globalDebugHooks.interruptHandlerData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_ClearInterrupt(JSRuntime *rt, JSTrapHandler *handlerp, void **closurep)
{
    if (handlerp)
        *handlerp = (JSTrapHandler)rt->globalDebugHooks.interruptHandler;
    if (closurep)
        *closurep = rt->globalDebugHooks.interruptHandlerData;
    rt->globalDebugHooks.interruptHandler = 0;
    rt->globalDebugHooks.interruptHandlerData = 0;
    return JS_TRUE;
}



typedef struct JSWatchPoint {
    JSCList             links;
    JSObject            *object;        
    JSScopeProperty     *sprop;
    JSPropertyOp        setter;
    JSWatchPointHandler handler;
    void                *closure;
    uintN               flags;
} JSWatchPoint;

#define JSWP_LIVE       0x1             /* live because set and not cleared */
#define JSWP_HELD       0x2             /* held while running handler/setter */




static JSBool
DropWatchPointAndUnlock(JSContext *cx, JSWatchPoint *wp, uintN flag)
{
    JSBool ok, found;
    JSScopeProperty *sprop;
    JSScope *scope;
    JSPropertyOp setter;

    ok = JS_TRUE;
    wp->flags &= ~flag;
    if (wp->flags != 0) {
        DBG_UNLOCK(cx->runtime);
        return ok;
    }

    



    ++cx->runtime->debuggerMutations;
    JS_REMOVE_LINK(&wp->links);
    sprop = wp->sprop;

    





    setter = js_GetWatchedSetter(cx->runtime, NULL, sprop);
    DBG_UNLOCK(cx->runtime);
    if (!setter) {
        JS_LOCK_OBJ(cx, wp->object);
        scope = OBJ_SCOPE(wp->object);
        found = (scope->lookup(sprop->id) != NULL);
        JS_UNLOCK_SCOPE(cx, scope);

        




        if (found) {
            sprop = scope->change(cx, sprop, 0, sprop->attrs,
                                  sprop->getter, wp->setter);
            if (!sprop)
                ok = JS_FALSE;
        }
    }

    cx->free(wp);
    return ok;
}






void
js_TraceWatchPoints(JSTracer *trc, JSObject *obj)
{
    JSRuntime *rt;
    JSWatchPoint *wp;

    rt = trc->context->runtime;

    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = (JSWatchPoint *)wp->links.next) {
        if (wp->object == obj) {
            wp->sprop->trace(trc);
            if ((wp->sprop->attrs & JSPROP_SETTER) && wp->setter) {
                JS_CALL_OBJECT_TRACER(trc, js_CastAsObject(wp->setter),
                                      "wp->setter");
            }
            JS_SET_TRACING_NAME(trc, "wp->closure");
            js_CallValueTracerIfGCThing(trc, (jsval) wp->closure);
        }
    }
}

void
js_SweepWatchPoints(JSContext *cx)
{
    JSRuntime *rt;
    JSWatchPoint *wp, *next;
    uint32 sample;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = next) {
        next = (JSWatchPoint *)wp->links.next;
        if (js_IsAboutToBeFinalized(cx, wp->object)) {
            sample = rt->debuggerMutations;

            
            DropWatchPointAndUnlock(cx, wp, JSWP_LIVE);
            DBG_LOCK(rt);
            if (rt->debuggerMutations != sample + 1)
                next = (JSWatchPoint *)rt->watchPointList.next;
        }
    }
    DBG_UNLOCK(rt);
}






static JSWatchPoint *
FindWatchPoint(JSRuntime *rt, JSScope *scope, jsid id)
{
    JSWatchPoint *wp;

    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = (JSWatchPoint *)wp->links.next) {
        if (OBJ_SCOPE(wp->object) == scope && wp->sprop->id == id)
            return wp;
    }
    return NULL;
}

JSScopeProperty *
js_FindWatchPoint(JSRuntime *rt, JSScope *scope, jsid id)
{
    JSWatchPoint *wp;
    JSScopeProperty *sprop;

    DBG_LOCK(rt);
    wp = FindWatchPoint(rt, scope, id);
    sprop = wp ? wp->sprop : NULL;
    DBG_UNLOCK(rt);
    return sprop;
}





JSPropertyOp
js_GetWatchedSetter(JSRuntime *rt, JSScope *scope,
                    const JSScopeProperty *sprop)
{
    JSPropertyOp setter;
    JSWatchPoint *wp;

    setter = NULL;
    if (scope)
        DBG_LOCK(rt);
    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = (JSWatchPoint *)wp->links.next) {
        if ((!scope || OBJ_SCOPE(wp->object) == scope) && wp->sprop == sprop) {
            setter = wp->setter;
            break;
        }
    }
    if (scope)
        DBG_UNLOCK(rt);
    return setter;
}

JSBool
js_watch_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSRuntime *rt;
    JSWatchPoint *wp;
    JSScopeProperty *sprop;
    jsval propid, userid;
    JSScope *scope;
    JSBool ok;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = (JSWatchPoint *)wp->links.next) {
        sprop = wp->sprop;
        if (wp->object == obj && SPROP_USERID(sprop) == id &&
            !(wp->flags & JSWP_HELD)) {
            wp->flags |= JSWP_HELD;
            DBG_UNLOCK(rt);

            JS_LOCK_OBJ(cx, obj);
            propid = ID_TO_VALUE(sprop->id);
            userid = (sprop->flags & SPROP_HAS_SHORTID)
                     ? INT_TO_JSVAL(sprop->shortid)
                     : propid;
            scope = OBJ_SCOPE(obj);
            JS_UNLOCK_OBJ(cx, obj);

            
            ok = wp->handler(cx, obj, propid,
                             SPROP_HAS_VALID_SLOT(sprop, scope)
                             ? OBJ_GET_SLOT(cx, obj, sprop->slot)
                             : JSVAL_VOID,
                             vp, wp->closure);
            if (ok) {
                










                JSObject *closure;
                JSClass *clasp;
                JSFunction *fun;
                JSScript *script;
                JSBool injectFrame;
                uintN nslots, slotsStart;
                jsval smallv[5];
                jsval *argv;
                JSStackFrame frame;
                JSFrameRegs regs;

                closure = (JSObject *) wp->closure;
                clasp = OBJ_GET_CLASS(cx, closure);
                if (clasp == &js_FunctionClass) {
                    fun = GET_FUNCTION_PRIVATE(cx, closure);
                    script = FUN_SCRIPT(fun);
                } else if (clasp == &js_ScriptClass) {
                    fun = NULL;
                    script = (JSScript *) closure->getAssignedPrivate();
                } else {
                    fun = NULL;
                    script = NULL;
                }

                slotsStart = nslots = 2;
                injectFrame = JS_TRUE;
                if (fun) {
                    nslots += FUN_MINARGS(fun);
                    if (!FUN_INTERPRETED(fun)) {
                        nslots += fun->u.n.extra;
                        injectFrame = !(fun->flags & JSFUN_FAST_NATIVE);
                    }

                    slotsStart = nslots;
                }
                if (script)
                    nslots += script->nslots;

                if (injectFrame) {
                    if (nslots <= JS_ARRAY_LENGTH(smallv)) {
                        argv = smallv;
                    } else {
                        argv = (jsval *) cx->malloc(nslots * sizeof(jsval));
                        if (!argv) {
                            DBG_LOCK(rt);
                            DropWatchPointAndUnlock(cx, wp, JSWP_HELD);
                            return JS_FALSE;
                        }
                    }

                    argv[0] = OBJECT_TO_JSVAL(closure);
                    argv[1] = JSVAL_NULL;
                    memset(argv + 2, 0, (nslots - 2) * sizeof(jsval));

                    memset(&frame, 0, sizeof(frame));
                    frame.script = script;
                    frame.regs = NULL;
                    frame.callee = closure;
                    frame.fun = fun;
                    frame.argv = argv + 2;
                    frame.down = js_GetTopStackFrame(cx);
                    frame.scopeChain = OBJ_GET_PARENT(cx, closure);
                    if (script && script->nslots)
                        frame.slots = argv + slotsStart;
                    if (script) {
                        JS_ASSERT(script->length >= JSOP_STOP_LENGTH);
                        regs.pc = script->code + script->length
                                  - JSOP_STOP_LENGTH;
                        regs.sp = NULL;
                        frame.regs = &regs;
                        if (fun &&
                            JSFUN_HEAVYWEIGHT_TEST(fun->flags) &&
                            !js_GetCallObject(cx, &frame)) {
                            if (argv != smallv)
                                cx->free(argv);
                            DBG_LOCK(rt);
                            DropWatchPointAndUnlock(cx, wp, JSWP_HELD);
                            return JS_FALSE;
                        }
                    }

                    cx->fp = &frame;
                }
#ifdef __GNUC__
                else
                    argv = NULL;    
#endif
                ok = !wp->setter ||
                     ((sprop->attrs & JSPROP_SETTER)
                      ? js_InternalCall(cx, obj,
                                        js_CastAsObjectJSVal(wp->setter),
                                        1, vp, vp)
                      : wp->setter(cx, obj, userid, vp));
                if (injectFrame) {
                    
                    ok = frame.putActivationObjects(cx);
                    cx->fp = frame.down;
                    if (argv != smallv)
                        cx->free(argv);
                }
            }
            DBG_LOCK(rt);
            return DropWatchPointAndUnlock(cx, wp, JSWP_HELD) && ok;
        }
    }
    DBG_UNLOCK(rt);
    return JS_TRUE;
}

JSBool
js_watch_set_wrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                     jsval *rval)
{
    JSObject *funobj;
    JSFunction *wrapper;
    jsval userid;

    funobj = JSVAL_TO_OBJECT(argv[-2]);
    wrapper = GET_FUNCTION_PRIVATE(cx, funobj);
    userid = ATOM_KEY(wrapper->atom);
    *rval = argv[0];
    return js_watch_set(cx, obj, userid, rval);
}

JSPropertyOp
js_WrapWatchedSetter(JSContext *cx, jsid id, uintN attrs, JSPropertyOp setter)
{
    JSAtom *atom;
    JSFunction *wrapper;

    if (!(attrs & JSPROP_SETTER))
        return &js_watch_set;   

    if (JSID_IS_ATOM(id)) {
        atom = JSID_TO_ATOM(id);
    } else if (JSID_IS_INT(id)) {
        if (!js_ValueToStringId(cx, INT_JSID_TO_JSVAL(id), &id))
            return NULL;
        atom = JSID_TO_ATOM(id);
    } else {
        atom = NULL;
    }
    wrapper = js_NewFunction(cx, NULL, js_watch_set_wrapper, 1, 0,
                             OBJ_GET_PARENT(cx, js_CastAsObject(setter)),
                             atom);
    if (!wrapper)
        return NULL;
    return js_CastAsPropertyOp(FUN_OBJECT(wrapper));
}

JS_PUBLIC_API(JSBool)
JS_SetWatchPoint(JSContext *cx, JSObject *obj, jsval idval,
                 JSWatchPointHandler handler, void *closure)
{
    jsid propid;
    JSObject *pobj;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSRuntime *rt;
    JSBool ok;
    JSWatchPoint *wp;
    JSPropertyOp watcher;

    if (!OBJ_IS_NATIVE(obj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_WATCH,
                             OBJ_GET_CLASS(cx, obj)->name);
        return JS_FALSE;
    }

    if (JSVAL_IS_INT(idval)) {
        propid = INT_JSVAL_TO_JSID(idval);
    } else {
        if (!js_ValueToStringId(cx, idval, &propid))
            return JS_FALSE;
        propid = js_CheckForStringIndex(propid);
    }

    if (!js_LookupProperty(cx, obj, propid, &pobj, &prop))
        return JS_FALSE;
    sprop = (JSScopeProperty *) prop;
    rt = cx->runtime;
    if (!sprop) {
        
        sprop = js_FindWatchPoint(rt, OBJ_SCOPE(obj), propid);
        if (!sprop) {
            
            if (!js_DefineProperty(cx, obj, propid, JSVAL_VOID,
                                   NULL, NULL, JSPROP_ENUMERATE,
                                   &prop)) {
                return JS_FALSE;
            }
            sprop = (JSScopeProperty *) prop;
        }
    } else if (pobj != obj) {
        
        jsval value;
        JSPropertyOp getter, setter;
        uintN attrs, flags;
        intN shortid;

        if (OBJ_IS_NATIVE(pobj)) {
            value = SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(pobj))
                    ? LOCKED_OBJ_GET_SLOT(pobj, sprop->slot)
                    : JSVAL_VOID;
            getter = sprop->getter;
            setter = sprop->setter;
            attrs = sprop->attrs;
            flags = sprop->flags;
            shortid = sprop->shortid;
        } else {
            if (!OBJ_GET_PROPERTY(cx, pobj, propid, &value) ||
                !OBJ_GET_ATTRIBUTES(cx, pobj, propid, prop, &attrs)) {
                OBJ_DROP_PROPERTY(cx, pobj, prop);
                return JS_FALSE;
            }
            getter = setter = NULL;
            flags = 0;
            shortid = 0;
        }
        OBJ_DROP_PROPERTY(cx, pobj, prop);

        
        if (!js_DefineNativeProperty(cx, obj, propid, value, getter, setter,
                                     attrs, flags, shortid, &prop)) {
            return JS_FALSE;
        }
        sprop = (JSScopeProperty *) prop;
    }

    



    ok = JS_TRUE;
    DBG_LOCK(rt);
    wp = FindWatchPoint(rt, OBJ_SCOPE(obj), propid);
    if (!wp) {
        DBG_UNLOCK(rt);
        watcher = js_WrapWatchedSetter(cx, propid, sprop->attrs, sprop->setter);
        if (!watcher) {
            ok = JS_FALSE;
            goto out;
        }

        wp = (JSWatchPoint *) cx->malloc(sizeof *wp);
        if (!wp) {
            ok = JS_FALSE;
            goto out;
        }
        wp->handler = NULL;
        wp->closure = NULL;
        wp->object = obj;
        JS_ASSERT(sprop->setter != js_watch_set || pobj != obj);
        wp->setter = sprop->setter;
        wp->flags = JSWP_LIVE;

        
        sprop = js_ChangeNativePropertyAttrs(cx, obj, sprop, 0, sprop->attrs,
                                             sprop->getter, watcher);
        if (!sprop) {
            
            JS_INIT_CLIST(&wp->links);
            DBG_LOCK(rt);
            DropWatchPointAndUnlock(cx, wp, JSWP_LIVE);
            ok = JS_FALSE;
            goto out;
        }
        wp->sprop = sprop;

        




        DBG_LOCK(rt);
        JS_ASSERT(!FindWatchPoint(rt, OBJ_SCOPE(obj), propid));
        JS_APPEND_LINK(&wp->links, &rt->watchPointList);
        ++rt->debuggerMutations;
    }
    wp->handler = handler;
    wp->closure = closure;
    DBG_UNLOCK(rt);

out:
    OBJ_DROP_PROPERTY(cx, obj, prop);
    return ok;
}

JS_PUBLIC_API(JSBool)
JS_ClearWatchPoint(JSContext *cx, JSObject *obj, jsval id,
                   JSWatchPointHandler *handlerp, void **closurep)
{
    JSRuntime *rt;
    JSWatchPoint *wp;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = (JSWatchPoint *)wp->links.next) {
        if (wp->object == obj && SPROP_USERID(wp->sprop) == id) {
            if (handlerp)
                *handlerp = wp->handler;
            if (closurep)
                *closurep = wp->closure;
            return DropWatchPointAndUnlock(cx, wp, JSWP_LIVE);
        }
    }
    DBG_UNLOCK(rt);
    if (handlerp)
        *handlerp = NULL;
    if (closurep)
        *closurep = NULL;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_ClearWatchPointsForObject(JSContext *cx, JSObject *obj)
{
    JSRuntime *rt;
    JSWatchPoint *wp, *next;
    uint32 sample;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = next) {
        next = (JSWatchPoint *)wp->links.next;
        if (wp->object == obj) {
            sample = rt->debuggerMutations;
            if (!DropWatchPointAndUnlock(cx, wp, JSWP_LIVE))
                return JS_FALSE;
            DBG_LOCK(rt);
            if (rt->debuggerMutations != sample + 1)
                next = (JSWatchPoint *)rt->watchPointList.next;
        }
    }
    DBG_UNLOCK(rt);
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_ClearAllWatchPoints(JSContext *cx)
{
    JSRuntime *rt;
    JSWatchPoint *wp, *next;
    uint32 sample;

    rt = cx->runtime;
    DBG_LOCK(rt);
    for (wp = (JSWatchPoint *)rt->watchPointList.next;
         &wp->links != &rt->watchPointList;
         wp = next) {
        next = (JSWatchPoint *)wp->links.next;
        sample = rt->debuggerMutations;
        if (!DropWatchPointAndUnlock(cx, wp, JSWP_LIVE))
            return JS_FALSE;
        DBG_LOCK(rt);
        if (rt->debuggerMutations != sample + 1)
            next = (JSWatchPoint *)rt->watchPointList.next;
    }
    DBG_UNLOCK(rt);
    return JS_TRUE;
}



JS_PUBLIC_API(uintN)
JS_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    return js_PCToLineNumber(cx, script, pc);
}

JS_PUBLIC_API(jsbytecode *)
JS_LineNumberToPC(JSContext *cx, JSScript *script, uintN lineno)
{
    return js_LineNumberToPC(script, lineno);
}

JS_PUBLIC_API(JSScript *)
JS_GetFunctionScript(JSContext *cx, JSFunction *fun)
{
    return FUN_SCRIPT(fun);
}

JS_PUBLIC_API(JSNative)
JS_GetFunctionNative(JSContext *cx, JSFunction *fun)
{
    return FUN_NATIVE(fun);
}

JS_PUBLIC_API(JSFastNative)
JS_GetFunctionFastNative(JSContext *cx, JSFunction *fun)
{
    return FUN_FAST_NATIVE(fun);
}

JS_PUBLIC_API(JSPrincipals *)
JS_GetScriptPrincipals(JSContext *cx, JSScript *script)
{
    return script->principals;
}






JS_PUBLIC_API(JSStackFrame *)
JS_FrameIterator(JSContext *cx, JSStackFrame **iteratorp)
{
    *iteratorp = (*iteratorp == NULL) ? js_GetTopStackFrame(cx) : (*iteratorp)->down;
    return *iteratorp;
}

JS_PUBLIC_API(JSScript *)
JS_GetFrameScript(JSContext *cx, JSStackFrame *fp)
{
    return fp->script;
}

JS_PUBLIC_API(jsbytecode *)
JS_GetFramePC(JSContext *cx, JSStackFrame *fp)
{
    return fp->regs ? fp->regs->pc : NULL;
}

JS_PUBLIC_API(JSStackFrame *)
JS_GetScriptedCaller(JSContext *cx, JSStackFrame *fp)
{
    return js_GetScriptedCaller(cx, fp);
}

JS_PUBLIC_API(JSPrincipals *)
JS_StackFramePrincipals(JSContext *cx, JSStackFrame *fp)
{
    JSSecurityCallbacks *callbacks;

    if (fp->fun) {
        callbacks = JS_GetSecurityCallbacks(cx);
        if (callbacks && callbacks->findObjectPrincipals) {
            if (FUN_OBJECT(fp->fun) != fp->callee)
                return callbacks->findObjectPrincipals(cx, fp->callee);
            
        }
    }
    if (fp->script)
        return fp->script->principals;
    return NULL;
}

JS_PUBLIC_API(JSPrincipals *)
JS_EvalFramePrincipals(JSContext *cx, JSStackFrame *fp, JSStackFrame *caller)
{
    JSPrincipals *principals, *callerPrincipals;
    JSSecurityCallbacks *callbacks;

    callbacks = JS_GetSecurityCallbacks(cx);
    if (callbacks && callbacks->findObjectPrincipals) {
        principals = callbacks->findObjectPrincipals(cx, fp->callee);
    } else {
        principals = NULL;
    }
    if (!caller)
        return principals;
    callerPrincipals = JS_StackFramePrincipals(cx, caller);
    return (callerPrincipals && principals &&
            callerPrincipals->subsume(callerPrincipals, principals))
           ? principals
           : callerPrincipals;
}

JS_PUBLIC_API(void *)
JS_GetFrameAnnotation(JSContext *cx, JSStackFrame *fp)
{
    if (fp->annotation && fp->script) {
        JSPrincipals *principals = JS_StackFramePrincipals(cx, fp);

        if (principals && principals->globalPrivilegesEnabled(cx, principals)) {
            



            return fp->annotation;
        }
    }

    return NULL;
}

JS_PUBLIC_API(void)
JS_SetFrameAnnotation(JSContext *cx, JSStackFrame *fp, void *annotation)
{
    fp->annotation = annotation;
}

JS_PUBLIC_API(void *)
JS_GetFramePrincipalArray(JSContext *cx, JSStackFrame *fp)
{
    JSPrincipals *principals;

    principals = JS_StackFramePrincipals(cx, fp);
    if (!principals)
        return NULL;
    return principals->getPrincipalArray(cx, principals);
}

JS_PUBLIC_API(JSBool)
JS_IsNativeFrame(JSContext *cx, JSStackFrame *fp)
{
    return !fp->script;
}


JS_PUBLIC_API(JSObject *)
JS_GetFrameObject(JSContext *cx, JSStackFrame *fp)
{
    return fp->scopeChain;
}

JS_PUBLIC_API(JSObject *)
JS_GetFrameScopeChain(JSContext *cx, JSStackFrame *fp)
{
    
    (void) JS_GetFrameCallObject(cx, fp);
    return js_GetScopeChain(cx, fp);
}

JS_PUBLIC_API(JSObject *)
JS_GetFrameCallObject(JSContext *cx, JSStackFrame *fp)
{
    if (! fp->fun)
        return NULL;

    
    (void) js_GetArgsObject(cx, fp);

    



    return js_GetCallObject(cx, fp);
}

JS_PUBLIC_API(JSObject *)
JS_GetFrameThis(JSContext *cx, JSStackFrame *fp)
{
    JSStackFrame *afp;

    if (fp->flags & JSFRAME_COMPUTED_THIS)
        return fp->thisp;

    
    if (js_GetTopStackFrame(cx) != fp) {
        afp = cx->fp;
        if (afp) {
            afp->dormantNext = cx->dormantFrameChain;
            cx->dormantFrameChain = afp;
            cx->fp = fp;
        }
    } else {
        afp = NULL;
    }

    if (!fp->thisp && fp->argv)
        fp->thisp = js_ComputeThis(cx, JS_TRUE, fp->argv);

    if (afp) {
        cx->fp = afp;
        cx->dormantFrameChain = afp->dormantNext;
        afp->dormantNext = NULL;
    }

    return fp->thisp;
}

JS_PUBLIC_API(JSFunction *)
JS_GetFrameFunction(JSContext *cx, JSStackFrame *fp)
{
    return fp->fun;
}

JS_PUBLIC_API(JSObject *)
JS_GetFrameFunctionObject(JSContext *cx, JSStackFrame *fp)
{
    if (!fp->fun)
        return NULL;

    JS_ASSERT(HAS_FUNCTION_CLASS(fp->callee));
    JS_ASSERT(fp->callee->getAssignedPrivate() == fp->fun);
    return fp->callee;
}

JS_PUBLIC_API(JSBool)
JS_IsConstructorFrame(JSContext *cx, JSStackFrame *fp)
{
    return (fp->flags & JSFRAME_CONSTRUCTING) != 0;
}

JS_PUBLIC_API(JSObject *)
JS_GetFrameCalleeObject(JSContext *cx, JSStackFrame *fp)
{
    return fp->callee;
}

JS_PUBLIC_API(JSBool)
JS_IsDebuggerFrame(JSContext *cx, JSStackFrame *fp)
{
    return (fp->flags & JSFRAME_DEBUGGER) != 0;
}

JS_PUBLIC_API(jsval)
JS_GetFrameReturnValue(JSContext *cx, JSStackFrame *fp)
{
    return fp->rval;
}

JS_PUBLIC_API(void)
JS_SetFrameReturnValue(JSContext *cx, JSStackFrame *fp, jsval rval)
{
    fp->rval = rval;
}



JS_PUBLIC_API(const char *)
JS_GetScriptFilename(JSContext *cx, JSScript *script)
{
    return script->filename;
}

JS_PUBLIC_API(uintN)
JS_GetScriptBaseLineNumber(JSContext *cx, JSScript *script)
{
    return script->lineno;
}

JS_PUBLIC_API(uintN)
JS_GetScriptLineExtent(JSContext *cx, JSScript *script)
{
    return js_GetScriptLineExtent(script);
}

JS_PUBLIC_API(JSVersion)
JS_GetScriptVersion(JSContext *cx, JSScript *script)
{
    return (JSVersion) (script->version & JSVERSION_MASK);
}



JS_PUBLIC_API(void)
JS_SetNewScriptHook(JSRuntime *rt, JSNewScriptHook hook, void *callerdata)
{
    rt->globalDebugHooks.newScriptHook = hook;
    rt->globalDebugHooks.newScriptHookData = callerdata;
}

JS_PUBLIC_API(void)
JS_SetDestroyScriptHook(JSRuntime *rt, JSDestroyScriptHook hook,
                        void *callerdata)
{
    rt->globalDebugHooks.destroyScriptHook = hook;
    rt->globalDebugHooks.destroyScriptHookData = callerdata;
}



JS_PUBLIC_API(JSBool)
JS_EvaluateUCInStackFrame(JSContext *cx, JSStackFrame *fp,
                          const jschar *chars, uintN length,
                          const char *filename, uintN lineno,
                          jsval *rval)
{
    JS_ASSERT_NOT_ON_TRACE(cx);

    JSObject *scobj;
    JSScript *script;
    JSBool ok;

    scobj = JS_GetFrameScopeChain(cx, fp);
    if (!scobj)
        return JS_FALSE;

    





    script = JSCompiler::compileScript(cx, scobj, fp, JS_StackFramePrincipals(cx, fp),
                                       TCF_COMPILE_N_GO |
                                       TCF_PUT_STATIC_LEVEL(JS_DISPLAY_SIZE),
                                       chars, length, NULL,
                                       filename, lineno);

    if (!script)
        return JS_FALSE;

    JSStackFrame *displayCopy[JS_DISPLAY_SIZE];
    if (cx->fp != fp) {
        memcpy(displayCopy, cx->display, sizeof displayCopy);

        










        JSStackFrame *fp2 = fp, *last = NULL;
        while (fp2) {
            JSStackFrame *next = fp2->down;
            fp2->down = last;
            last = fp2;
            fp2 = next;
        }

        fp2 = last;
        last = NULL;
        while (fp2) {
            JSStackFrame *next = fp2->down;
            fp2->down = last;
            last = fp2;

            JSScript *script = fp2->script;
            if (script && script->staticLevel < JS_DISPLAY_SIZE)
                cx->display[script->staticLevel] = fp2;
            fp2 = next;
        }
    }

    ok = js_Execute(cx, scobj, script, fp, JSFRAME_DEBUGGER | JSFRAME_EVAL,
                    rval);

    if (cx->fp != fp)
        memcpy(cx->display, displayCopy, sizeof cx->display);
    js_DestroyScript(cx, script);
    return ok;
}

JS_PUBLIC_API(JSBool)
JS_EvaluateInStackFrame(JSContext *cx, JSStackFrame *fp,
                        const char *bytes, uintN length,
                        const char *filename, uintN lineno,
                        jsval *rval)
{
    jschar *chars;
    JSBool ok;
    size_t len = length;

    chars = js_InflateString(cx, bytes, &len);
    if (!chars)
        return JS_FALSE;
    length = (uintN) len;
    ok = JS_EvaluateUCInStackFrame(cx, fp, chars, length, filename, lineno,
                                   rval);
    cx->free(chars);

    return ok;
}





JS_PUBLIC_API(JSScopeProperty *)
JS_PropertyIterator(JSObject *obj, JSScopeProperty **iteratorp)
{
    JSScopeProperty *sprop;
    JSScope *scope;

    sprop = *iteratorp;
    scope = OBJ_SCOPE(obj);

    
    if (!sprop) {
        sprop = SCOPE_LAST_PROP(scope);
    } else {
        while ((sprop = sprop->parent) != NULL) {
            if (!scope->hadMiddleDelete())
                break;
            if (scope->has(sprop))
                break;
        }
    }
    *iteratorp = sprop;
    return sprop;
}

JS_PUBLIC_API(JSBool)
JS_GetPropertyDesc(JSContext *cx, JSObject *obj, JSScopeProperty *sprop,
                   JSPropertyDesc *pd)
{
    JSScope *scope;
    JSScopeProperty *aprop;
    jsval lastException;
    JSBool wasThrowing;

    pd->id = ID_TO_VALUE(sprop->id);

    wasThrowing = cx->throwing;
    if (wasThrowing) {
        lastException = cx->exception;
        if (JSVAL_IS_GCTHING(lastException) &&
            !js_AddRoot(cx, &lastException, "lastException")) {
                return JS_FALSE;
        }
        cx->throwing = JS_FALSE;
    }

    if (!js_GetProperty(cx, obj, sprop->id, &pd->value)) {
        if (!cx->throwing) {
            pd->flags = JSPD_ERROR;
            pd->value = JSVAL_VOID;
        } else {
            pd->flags = JSPD_EXCEPTION;
            pd->value = cx->exception;
        }
    } else {
        pd->flags = 0;
    }

    cx->throwing = wasThrowing;
    if (wasThrowing) {
        cx->exception = lastException;
        if (JSVAL_IS_GCTHING(lastException))
            js_RemoveRoot(cx->runtime, &lastException);
    }

    pd->flags |= ((sprop->attrs & JSPROP_ENUMERATE) ? JSPD_ENUMERATE : 0)
              | ((sprop->attrs & JSPROP_READONLY)  ? JSPD_READONLY  : 0)
              | ((sprop->attrs & JSPROP_PERMANENT) ? JSPD_PERMANENT : 0);
    pd->spare = 0;
    if (sprop->getter == js_GetCallArg) {
        pd->slot = sprop->shortid;
        pd->flags |= JSPD_ARGUMENT;
    } else if (sprop->getter == js_GetCallVar) {
        pd->slot = sprop->shortid;
        pd->flags |= JSPD_VARIABLE;
    } else {
        pd->slot = 0;
    }
    pd->alias = JSVAL_VOID;
    scope = OBJ_SCOPE(obj);
    if (SPROP_HAS_VALID_SLOT(sprop, scope)) {
        for (aprop = SCOPE_LAST_PROP(scope); aprop; aprop = aprop->parent) {
            if (aprop != sprop && aprop->slot == sprop->slot) {
                pd->alias = ID_TO_VALUE(aprop->id);
                break;
            }
        }
    }
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_GetPropertyDescArray(JSContext *cx, JSObject *obj, JSPropertyDescArray *pda)
{
    JSClass *clasp;
    JSScope *scope;
    uint32 i, n;
    JSPropertyDesc *pd;
    JSScopeProperty *sprop;

    clasp = OBJ_GET_CLASS(cx, obj);
    if (!OBJ_IS_NATIVE(obj) || (clasp->flags & JSCLASS_NEW_ENUMERATE)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_DESCRIBE_PROPS, clasp->name);
        return JS_FALSE;
    }
    if (!clasp->enumerate(cx, obj))
        return JS_FALSE;

    
    scope = OBJ_SCOPE(obj);
    if (scope->entryCount == 0) {
        pda->length = 0;
        pda->array = NULL;
        return JS_TRUE;
    }

    n = scope->entryCount;
    pd = (JSPropertyDesc *) cx->malloc((size_t)n * sizeof(JSPropertyDesc));
    if (!pd)
        return JS_FALSE;
    i = 0;
    for (sprop = SCOPE_LAST_PROP(scope); sprop; sprop = sprop->parent) {
        if (scope->hadMiddleDelete() && !scope->has(sprop))
            continue;
        if (!js_AddRoot(cx, &pd[i].id, NULL))
            goto bad;
        if (!js_AddRoot(cx, &pd[i].value, NULL))
            goto bad;
        if (!JS_GetPropertyDesc(cx, obj, sprop, &pd[i]))
            goto bad;
        if ((pd[i].flags & JSPD_ALIAS) && !js_AddRoot(cx, &pd[i].alias, NULL))
            goto bad;
        if (++i == n)
            break;
    }
    pda->length = i;
    pda->array = pd;
    return JS_TRUE;

bad:
    pda->length = i + 1;
    pda->array = pd;
    JS_PutPropertyDescArray(cx, pda);
    return JS_FALSE;
}

JS_PUBLIC_API(void)
JS_PutPropertyDescArray(JSContext *cx, JSPropertyDescArray *pda)
{
    JSPropertyDesc *pd;
    uint32 i;

    pd = pda->array;
    for (i = 0; i < pda->length; i++) {
        js_RemoveRoot(cx->runtime, &pd[i].id);
        js_RemoveRoot(cx->runtime, &pd[i].value);
        if (pd[i].flags & JSPD_ALIAS)
            js_RemoveRoot(cx->runtime, &pd[i].alias);
    }
    cx->free(pd);
}



JS_PUBLIC_API(JSBool)
JS_SetDebuggerHandler(JSRuntime *rt, JSTrapHandler handler, void *closure)
{
    rt->globalDebugHooks.debuggerHandler = handler;
    rt->globalDebugHooks.debuggerHandlerData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_SetSourceHandler(JSRuntime *rt, JSSourceHandler handler, void *closure)
{
    rt->globalDebugHooks.sourceHandler = handler;
    rt->globalDebugHooks.sourceHandlerData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_SetExecuteHook(JSRuntime *rt, JSInterpreterHook hook, void *closure)
{
    rt->globalDebugHooks.executeHook = hook;
    rt->globalDebugHooks.executeHookData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_SetCallHook(JSRuntime *rt, JSInterpreterHook hook, void *closure)
{
    rt->globalDebugHooks.callHook = hook;
    rt->globalDebugHooks.callHookData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_SetObjectHook(JSRuntime *rt, JSObjectHook hook, void *closure)
{
    rt->globalDebugHooks.objectHook = hook;
    rt->globalDebugHooks.objectHookData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_SetThrowHook(JSRuntime *rt, JSTrapHandler hook, void *closure)
{
    rt->globalDebugHooks.throwHook = hook;
    rt->globalDebugHooks.throwHookData = closure;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_SetDebugErrorHook(JSRuntime *rt, JSDebugErrorHook hook, void *closure)
{
    rt->globalDebugHooks.debugErrorHook = hook;
    rt->globalDebugHooks.debugErrorHookData = closure;
    return JS_TRUE;
}



JS_PUBLIC_API(size_t)
JS_GetObjectTotalSize(JSContext *cx, JSObject *obj)
{
    size_t nbytes;
    JSScope *scope;

    nbytes = sizeof *obj;
    if (obj->dslots) {
        nbytes += ((uint32)obj->dslots[-1] - JS_INITIAL_NSLOTS + 1)
                  * sizeof obj->dslots[0];
    }
    if (OBJ_IS_NATIVE(obj)) {
        scope = OBJ_SCOPE(obj);
        if (scope->owned()) {
            nbytes += sizeof *scope;
            nbytes += SCOPE_CAPACITY(scope) * sizeof(JSScopeProperty *);
        }
    }
    return nbytes;
}

static size_t
GetAtomTotalSize(JSContext *cx, JSAtom *atom)
{
    size_t nbytes;

    nbytes = sizeof(JSAtom *) + sizeof(JSDHashEntryStub);
    if (ATOM_IS_STRING(atom)) {
        nbytes += sizeof(JSString);
        nbytes += (ATOM_TO_STRING(atom)->flatLength() + 1) * sizeof(jschar);
    } else if (ATOM_IS_DOUBLE(atom)) {
        nbytes += sizeof(jsdouble);
    }
    return nbytes;
}

JS_PUBLIC_API(size_t)
JS_GetFunctionTotalSize(JSContext *cx, JSFunction *fun)
{
    size_t nbytes;

    nbytes = sizeof *fun;
    nbytes += JS_GetObjectTotalSize(cx, FUN_OBJECT(fun));
    if (FUN_INTERPRETED(fun))
        nbytes += JS_GetScriptTotalSize(cx, fun->u.i.script);
    if (fun->atom)
        nbytes += GetAtomTotalSize(cx, fun->atom);
    return nbytes;
}

#include "jsemit.h"

JS_PUBLIC_API(size_t)
JS_GetScriptTotalSize(JSContext *cx, JSScript *script)
{
    size_t nbytes, pbytes;
    jsatomid i;
    jssrcnote *sn, *notes;
    JSObjectArray *objarray;
    JSPrincipals *principals;

    nbytes = sizeof *script;
    if (script->u.object)
        nbytes += JS_GetObjectTotalSize(cx, script->u.object);

    nbytes += script->length * sizeof script->code[0];
    nbytes += script->atomMap.length * sizeof script->atomMap.vector[0];
    for (i = 0; i < script->atomMap.length; i++)
        nbytes += GetAtomTotalSize(cx, script->atomMap.vector[i]);

    if (script->filename)
        nbytes += strlen(script->filename) + 1;

    notes = SCRIPT_NOTES(script);
    for (sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn))
        continue;
    nbytes += (sn - notes + 1) * sizeof *sn;

    if (script->objectsOffset != 0) {
        objarray = JS_SCRIPT_OBJECTS(script);
        i = objarray->length;
        nbytes += sizeof *objarray + i * sizeof objarray->vector[0];
        do {
            nbytes += JS_GetObjectTotalSize(cx, objarray->vector[--i]);
        } while (i != 0);
    }

    if (script->regexpsOffset != 0) {
        objarray = JS_SCRIPT_REGEXPS(script);
        i = objarray->length;
        nbytes += sizeof *objarray + i * sizeof objarray->vector[0];
        do {
            nbytes += JS_GetObjectTotalSize(cx, objarray->vector[--i]);
        } while (i != 0);
    }

    if (script->trynotesOffset != 0) {
        nbytes += sizeof(JSTryNoteArray) +
            JS_SCRIPT_TRYNOTES(script)->length * sizeof(JSTryNote);
    }

    principals = script->principals;
    if (principals) {
        JS_ASSERT(principals->refcount);
        pbytes = sizeof *principals;
        if (principals->refcount > 1)
            pbytes = JS_HOWMANY(pbytes, principals->refcount);
        nbytes += pbytes;
    }

    return nbytes;
}

JS_PUBLIC_API(uint32)
JS_GetTopScriptFilenameFlags(JSContext *cx, JSStackFrame *fp)
{
    if (!fp)
        fp = js_GetTopStackFrame(cx);
    while (fp) {
        if (fp->script)
            return JS_GetScriptFilenameFlags(fp->script);
        fp = fp->down;
    }
    return 0;
 }

JS_PUBLIC_API(uint32)
JS_GetScriptFilenameFlags(JSScript *script)
{
    JS_ASSERT(script);
    if (!script->filename)
        return JSFILENAME_NULL;
    return js_GetScriptFilenameFlags(script->filename);
}

JS_PUBLIC_API(JSBool)
JS_FlagScriptFilenamePrefix(JSRuntime *rt, const char *prefix, uint32 flags)
{
    if (!js_SaveScriptFilenameRT(rt, prefix, flags))
        return JS_FALSE;
    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_IsSystemObject(JSContext *cx, JSObject *obj)
{
    return STOBJ_IS_SYSTEM(obj);
}

JS_PUBLIC_API(JSObject *)
JS_NewSystemObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                   JSObject *parent, JSBool system)
{
    JSObject *obj;

    obj = js_NewObject(cx, clasp, proto, parent);
    if (obj && system)
        STOBJ_SET_SYSTEM(obj);
    return obj;
}



JS_PUBLIC_API(JSDebugHooks *)
JS_GetGlobalDebugHooks(JSRuntime *rt)
{
    return &rt->globalDebugHooks;
}

JS_PUBLIC_API(JSDebugHooks *)
JS_SetContextDebugHooks(JSContext *cx, JSDebugHooks *hooks)
{
    JSDebugHooks *old;

    JS_ASSERT(hooks);
    old = cx->debugHooks;
    cx->debugHooks = hooks;
    return old;
}

#ifdef MOZ_SHARK

#include <CHUD/CHUD.h>

JS_PUBLIC_API(JSBool)
JS_StartChudRemote()
{
    if (chudIsRemoteAccessAcquired() &&
        (chudStartRemotePerfMonitor("Mozilla") == chudSuccess)) {
        return JS_TRUE;
    }

    return JS_FALSE;
}

JS_PUBLIC_API(JSBool)
JS_StopChudRemote()
{
    if (chudIsRemoteAccessAcquired() &&
        (chudStopRemotePerfMonitor() == chudSuccess)) {
        return JS_TRUE;
    }

    return JS_FALSE;
}

JS_PUBLIC_API(JSBool)
JS_ConnectShark()
{
    if (!chudIsInitialized() && (chudInitialize() != chudSuccess))
        return JS_FALSE;

    if (chudAcquireRemoteAccess() != chudSuccess)
        return JS_FALSE;

    return JS_TRUE;
}

JS_PUBLIC_API(JSBool)
JS_DisconnectShark()
{
    if (chudIsRemoteAccessAcquired() && (chudReleaseRemoteAccess() != chudSuccess))
        return JS_FALSE;

    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_StartShark(JSContext *cx, JSObject *obj,
              uintN argc, jsval *argv, jsval *rval)
{
    if (!JS_StartChudRemote()) {
        JS_ReportError(cx, "Error starting CHUD.");
        return JS_FALSE;
    }

    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_StopShark(JSContext *cx, JSObject *obj,
             uintN argc, jsval *argv, jsval *rval)
{
    if (!JS_StopChudRemote()) {
        JS_ReportError(cx, "Error stopping CHUD.");
        return JS_FALSE;
    }

    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_ConnectShark(JSContext *cx, JSObject *obj,
                uintN argc, jsval *argv, jsval *rval)
{
    if (!JS_ConnectShark()) {
        JS_ReportError(cx, "Error connecting to Shark.");
        return JS_FALSE;
    }

    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_DisconnectShark(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval)
{
    if (!JS_DisconnectShark()) {
        JS_ReportError(cx, "Error disconnecting from Shark.");
        return JS_FALSE;
    }

    return JS_TRUE;
}

#endif 

#ifdef MOZ_CALLGRIND

#include <valgrind/callgrind.h>

JS_FRIEND_API(JSBool)
js_StartCallgrind(JSContext *cx, JSObject *obj,
                  uintN argc, jsval *argv, jsval *rval)
{
    CALLGRIND_START_INSTRUMENTATION;
    CALLGRIND_ZERO_STATS;
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_StopCallgrind(JSContext *cx, JSObject *obj,
                 uintN argc, jsval *argv, jsval *rval)
{
    CALLGRIND_STOP_INSTRUMENTATION;
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_DumpCallgrind(JSContext *cx, JSObject *obj,
                 uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    char *cstr;

    if (argc > 0 && JSVAL_IS_STRING(argv[0])) {
        str = JSVAL_TO_STRING(argv[0]);
        cstr = js_DeflateString(cx, str->chars(), str->length());
        if (cstr) {
            CALLGRIND_DUMP_STATS_AT(cstr);
            cx->free(cstr);
            return JS_TRUE;
        }
    }
    CALLGRIND_DUMP_STATS;

    return JS_TRUE;
}

#endif 

#ifdef MOZ_VTUNE
#include <VTuneApi.h>

static const char *vtuneErrorMessages[] = {
  "unknown, error #0",
  "invalid 'max samples' field",
  "invalid 'samples per buffer' field",
  "invalid 'sample interval' field",
  "invalid path",
  "sample file in use",
  "invalid 'number of events' field",
  "unknown, error #7",
  "internal error",
  "bad event name",
  "VTStopSampling called without calling VTStartSampling",
  "no events selected for event-based sampling",
  "events selected cannot be run together",
  "no sampling parameters",
  "sample database already exists",
  "sampling already started",
  "time-based sampling not supported",
  "invalid 'sampling parameters size' field",
  "invalid 'event size' field",
  "sampling file already bound",
  "invalid event path",
  "invalid license",
  "invalid 'global options' field",

};

JS_FRIEND_API(JSBool)
js_StartVtune(JSContext *cx, JSObject *obj,
              uintN argc, jsval *argv, jsval *rval)
{
    VTUNE_EVENT events[] = {
        { 1000000, 0, 0, 0, "CPU_CLK_UNHALTED.CORE" },
        { 1000000, 0, 0, 0, "INST_RETIRED.ANY" },
    };

    U32 n_events = sizeof(events) / sizeof(VTUNE_EVENT);
    char *default_filename = "mozilla-vtune.tb5";
    JSString *str;
    U32 status;

    VTUNE_SAMPLING_PARAMS params =
        sizeof(VTUNE_SAMPLING_PARAMS),
        sizeof(VTUNE_EVENT),
        0, 0, 
        1,    
        0,    
        4096, 
        0.1,  
        1,    

        n_events,
        events,
        default_filename,
    };

    if (argc > 0 && JSVAL_IS_STRING(argv[0])) {
        str = JSVAL_TO_STRING(argv[0]);
        params.tb5Filename = js_DeflateString(cx, str->chars(), str->length());
    }

    status = VTStartSampling(&params);

    if (params.tb5Filename != default_filename)
        cx->free(params.tb5Filename);

    if (status != 0) {
        if (status == VTAPI_MULTIPLE_RUNS)
            VTStopSampling(0);
        if (status < sizeof(vtuneErrorMessages))
            JS_ReportError(cx, "Vtune setup error: %s",
                           vtuneErrorMessages[status]);
        else
            JS_ReportError(cx, "Vtune setup error: %d",
                           status);
        return JS_FALSE;
    }
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_StopVtune(JSContext *cx, JSObject *obj,
             uintN argc, jsval *argv, jsval *rval)
{
    U32 status = VTStopSampling(1);
    if (status) {
        if (status < sizeof(vtuneErrorMessages))
            JS_ReportError(cx, "Vtune shutdown error: %s",
                           vtuneErrorMessages[status]);
        else
            JS_ReportError(cx, "Vtune shutdown error: %d",
                           status);
        return JS_FALSE;
    }
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_PauseVtune(JSContext *cx, JSObject *obj,
              uintN argc, jsval *argv, jsval *rval)
{
    VTPause();
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_ResumeVtune(JSContext *cx, JSObject *obj,
               uintN argc, jsval *argv, jsval *rval)
{
    VTResume();
    return JS_TRUE;
}

#endif 

#ifdef MOZ_TRACEVIS









#if defined(XP_WIN)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "jstracer.h"

#define ETHOGRAM_BUF_SIZE 65536

static JSBool
ethogram_construct(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval);
static void
ethogram_finalize(JSContext *cx, JSObject *obj);

static JSClass ethogram_class = {
    "Ethogram",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ethogram_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

struct EthogramEvent {
    TraceVisState s;
    TraceVisExitReason r;
    int ts;
    int tus;
    JSString *filename;
    int lineno;
};

static int
compare_strings(const void *k1, const void *k2)
{
    return strcmp((const char *) k1, (const char *) k2) == 0;
}

class EthogramEventBuffer {
private:
    EthogramEvent mBuf[ETHOGRAM_BUF_SIZE];
    int mReadPos;
    int mWritePos;
    JSObject *mFilenames;
    int mStartSecond;

    struct EthogramScriptEntry {
        char *filename;
        JSString *jsfilename;

        EthogramScriptEntry *next;
    };
    EthogramScriptEntry *mScripts;

public:
    friend JSBool
    ethogram_construct(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *rval);

    inline void push(TraceVisState s, TraceVisExitReason r, char *filename, int lineno) {
        mBuf[mWritePos].s = s;
        mBuf[mWritePos].r = r;
#if defined(XP_WIN)
        FILETIME now;
        GetSystemTimeAsFileTime(&now);
        unsigned long long raw_us = 0.1 *
            (((unsigned long long) now.dwHighDateTime << 32ULL) |
             (unsigned long long) now.dwLowDateTime);
        unsigned int sec = raw_us / 1000000L;
        unsigned int usec = raw_us % 1000000L;
        mBuf[mWritePos].ts = sec - mStartSecond;
        mBuf[mWritePos].tus = usec;
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        mBuf[mWritePos].ts = tv.tv_sec - mStartSecond;
        mBuf[mWritePos].tus = tv.tv_usec;
#endif

        JSString *jsfilename = findScript(filename);
        mBuf[mWritePos].filename = jsfilename;
        mBuf[mWritePos].lineno = lineno;

        mWritePos = (mWritePos + 1) % ETHOGRAM_BUF_SIZE;
        if (mWritePos == mReadPos) {
            mReadPos = (mWritePos + 1) % ETHOGRAM_BUF_SIZE;
        }
    }

    inline EthogramEvent *pop() {
        EthogramEvent *e = &mBuf[mReadPos];
        mReadPos = (mReadPos + 1) % ETHOGRAM_BUF_SIZE;
        return e;
    }

    bool isEmpty() {
        return (mReadPos == mWritePos);
    }

    EthogramScriptEntry *addScript(JSContext *cx, JSObject *obj, char *filename, JSString *jsfilename) {
        JSHashNumber hash = JS_HashString(filename);
        JSHashEntry **hep = JS_HashTableRawLookup(traceVisScriptTable, hash, filename);
        if (*hep != NULL)
            return JS_FALSE;

        JS_HashTableRawAdd(traceVisScriptTable, hep, hash, filename, this);

        EthogramScriptEntry * entry = (EthogramScriptEntry *) JS_malloc(cx, sizeof(EthogramScriptEntry));
        if (entry == NULL)
            return NULL;

        entry->next = mScripts;
        mScripts = entry;
        entry->filename = filename;
        entry->jsfilename = jsfilename;

        return mScripts;
    }

    void removeScripts(JSContext *cx) {
        EthogramScriptEntry *se = mScripts;
        while (se != NULL) {
            char *filename = se->filename;

            JSHashNumber hash = JS_HashString(filename);
            JSHashEntry **hep = JS_HashTableRawLookup(traceVisScriptTable, hash, filename);
            JSHashEntry *he = *hep;
            if (he) {
                
                JS_HashTableRawRemove(traceVisScriptTable, hep, he);
            }

            EthogramScriptEntry *se_head = se;
            se = se->next;
            JS_free(cx, se_head);
        }
    }

    JSString *findScript(char *filename) {
        EthogramScriptEntry *se = mScripts;
        while (se != NULL) {
            if (compare_strings(se->filename, filename))
                return (se->jsfilename);
            se = se->next;
        }
        return NULL;
    }

    JSObject *filenames() {
        return mFilenames;
    }

    int length() {
        if (mWritePos < mReadPos)
            return (mWritePos + ETHOGRAM_BUF_SIZE) - mReadPos;
        else
            return mWritePos - mReadPos;
    }
};

static char jstv_empty[] = "<null>";

inline char *
jstv_Filename(JSStackFrame *fp)
{
    while (fp && fp->script == NULL)
        fp = fp->down;
    return (fp && fp->script && fp->script->filename)
           ? (char *)fp->script->filename
           : jstv_empty;
}
inline uintN
jstv_Lineno(JSContext *cx, JSStackFrame *fp)
{
    while (fp && fp->regs == NULL)
        fp = fp->down;
    return (fp && fp->regs) ? js_FramePCToLineNumber(cx, fp) : 0;
}


JS_FRIEND_API(void)
js_StoreTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r)
{
    JSStackFrame *fp = cx->fp;

    char *script_file = jstv_Filename(fp);
    JSHashNumber hash = JS_HashString(script_file);

    JSHashEntry **hep = JS_HashTableRawLookup(traceVisScriptTable, hash, script_file);
    
    JSHashEntry *he = *hep;
    if (he) {
        EthogramEventBuffer *p;
        p = (EthogramEventBuffer *) he->value;

        p->push(s, r, script_file, jstv_Lineno(cx, fp));
    }
}

static JSBool
ethogram_construct(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval)
{
    EthogramEventBuffer *p;

    p = (EthogramEventBuffer *) JS_malloc(cx, sizeof(EthogramEventBuffer));

    p->mReadPos = p->mWritePos = 0;
    p->mScripts = NULL;
    p->mFilenames = JS_NewArrayObject(cx, 0, NULL);

#if defined(XP_WIN)
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    unsigned long long raw_us = 0.1 *
        (((unsigned long long) now.dwHighDateTime << 32ULL) |
         (unsigned long long) now.dwLowDateTime);
    unsigned int s = raw_us / 1000000L;
    p->mStartSecond = s;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    p->mStartSecond = tv.tv_sec;
#endif
    jsval filenames = OBJECT_TO_JSVAL(p->filenames());
    if (!JS_DefineProperty(cx, obj, "filenames", filenames,
                           NULL, NULL, JSPROP_READONLY|JSPROP_PERMANENT))
        return JS_FALSE;

    if (!JS_IsConstructing(cx)) {
        obj = JS_NewObject(cx, &ethogram_class, NULL, NULL);
        if (!obj)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(obj);
    }
    JS_SetPrivate(cx, obj, p);
    return JS_TRUE;
}

static void
ethogram_finalize(JSContext *cx, JSObject *obj)
{
    EthogramEventBuffer *p;
    p = (EthogramEventBuffer *) JS_GetInstancePrivate(cx, obj, &ethogram_class, NULL);
    if (!p)
        return;

    p->removeScripts(cx);

    JS_free(cx, p);
}

static JSBool
ethogram_addScript(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    char *filename = NULL;
    if (argc > 0 && JSVAL_IS_STRING(argv[0])) {
        str = JSVAL_TO_STRING(argv[0]);
        filename = js_DeflateString(cx,
                                    str->chars(),
                                    str->length());
    }

    
    if (!filename)
        return JS_TRUE;

    EthogramEventBuffer *p = (EthogramEventBuffer *) JS_GetInstancePrivate(cx, obj, &ethogram_class, argv);

    p->addScript(cx, obj, filename, str);
    JS_CallFunctionName(cx, p->filenames(), "push", 1, argv, rval);
    return JS_TRUE;
}

static JSBool
ethogram_getAllEvents(JSContext *cx, JSObject *obj,
                      uintN argc, jsval *argv, jsval *rval)
{
    EthogramEventBuffer *p;

    p = (EthogramEventBuffer *) JS_GetInstancePrivate(cx, obj, &ethogram_class, argv);
    if (!p)
        return JS_FALSE;

    if (p->isEmpty()) {
        *rval = JSVAL_NULL;
        return JS_TRUE;
    }

    JSObject *rarray = JS_NewArrayObject(cx, 0, NULL);
    if (rarray == NULL) {
        *rval = JSVAL_NULL;
        return JS_TRUE;
    }

    *rval = OBJECT_TO_JSVAL(rarray);

    for (int i = 0; !p->isEmpty(); i++) {

        JSObject *x = JS_NewObject(cx, NULL, NULL, NULL);
        if (x == NULL)
            return JS_FALSE;

        EthogramEvent *e = p->pop();

        jsval state = INT_TO_JSVAL(e->s);
        jsval reason = INT_TO_JSVAL(e->r);
        jsval ts = INT_TO_JSVAL(e->ts);
        jsval tus = INT_TO_JSVAL(e->tus);

        jsval filename = STRING_TO_JSVAL(e->filename);
        jsval lineno = INT_TO_JSVAL(e->lineno);

        if (!JS_SetProperty(cx, x, "state", &state))
            return JS_FALSE;
        if (!JS_SetProperty(cx, x, "reason", &reason))
            return JS_FALSE;
        if (!JS_SetProperty(cx, x, "ts", &ts))
            return JS_FALSE;
        if (!JS_SetProperty(cx, x, "tus", &tus))
            return JS_FALSE;

        if (!JS_SetProperty(cx, x, "filename", &filename))
            return JS_FALSE;
        if (!JS_SetProperty(cx, x, "lineno", &lineno))
            return JS_FALSE;

        jsval element = OBJECT_TO_JSVAL(x);
        JS_SetElement(cx, rarray, i, &element);
    }

    return JS_TRUE;
}

static JSBool
ethogram_getNextEvent(JSContext *cx, JSObject *obj,
                      uintN argc, jsval *argv, jsval *rval)
{
    EthogramEventBuffer *p;

    p = (EthogramEventBuffer *) JS_GetInstancePrivate(cx, obj, &ethogram_class, argv);
    if (!p)
        return JS_FALSE;

    JSObject *x = JS_NewObject(cx, NULL, NULL, NULL);
    if (x == NULL)
        return JS_FALSE;

    if (p->isEmpty()) {
        *rval = JSVAL_NULL;
        return JS_TRUE;
    }

    EthogramEvent *e = p->pop();
    jsval state = INT_TO_JSVAL(e->s);
    jsval reason = INT_TO_JSVAL(e->r);
    jsval ts = INT_TO_JSVAL(e->ts);
    jsval tus = INT_TO_JSVAL(e->tus);

    jsval filename = STRING_TO_JSVAL(e->filename);
    jsval lineno = INT_TO_JSVAL(e->lineno);

    if (!JS_SetProperty(cx, x, "state", &state))
        return JS_FALSE;
    if (!JS_SetProperty(cx, x, "reason", &reason))
        return JS_FALSE;
    if (!JS_SetProperty(cx, x, "ts", &ts))
        return JS_FALSE;
    if (!JS_SetProperty(cx, x, "tus", &tus))
        return JS_FALSE;
    if (!JS_SetProperty(cx, x, "filename", &filename))
        return JS_FALSE;

    if (!JS_SetProperty(cx, x, "lineno", &lineno))
        return JS_FALSE;

    *rval = OBJECT_TO_JSVAL(x);

    return JS_TRUE;
}

static JSFunctionSpec ethogram_methods[] = {
    {"addScript",    ethogram_addScript,    1},
    {"getAllEvents", ethogram_getAllEvents, 0},
    {"getNextEvent", ethogram_getNextEvent, 0},
    {0}
};





JS_FRIEND_API(JSBool)
js_InitEthogram(JSContext *cx, JSObject *obj,
                uintN argc, jsval *argv, jsval *rval)
{
    if (!traceVisScriptTable) {
        traceVisScriptTable = JS_NewHashTable(8, JS_HashString, compare_strings,
                                         NULL, NULL, NULL);
    }

    JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, &ethogram_class,
                 ethogram_construct, 0, NULL, ethogram_methods,
                 NULL, NULL);

    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_ShutdownEthogram(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *rval)
{
    if (traceVisScriptTable)
        JS_HashTableDestroy(traceVisScriptTable);

    return JS_TRUE;
}

#endif 
