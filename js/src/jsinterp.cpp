







































#define __STDC_LIMIT_MACROS




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
#include "jscntxtinlines.h"
#include "jsdtracef.h"
#include "jsobjinlines.h"
#include "jspropertycacheinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsstrinlines.h"
#include "jscntxtinlines.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsautooplen.h"

using namespace js;


#if !JS_LONE_INTERPRET ^ defined jsinvoke_cpp___

#ifdef DEBUG
jsbytecode *const JSStackFrame::sInvalidPC = (jsbytecode *)0xbeef;
#endif

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
        JS_ASSERT_IF(fp->scopeChain->getClass() == &js_BlockClass,
                     fp->scopeChain->getPrivate() != js_FloatingFrameIfGenerator(cx, fp));
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
    AutoObjectRooter tvr(cx, innermostNewChild);

    



    JSObject *newChild = innermostNewChild;
    for (;;) {
        JS_ASSERT(newChild->getProto() == sharedBlock);
        sharedBlock = sharedBlock->getParent();

        
        if (sharedBlock == limitBlock || !sharedBlock)
            break;

        
        JSObject *clone = js_CloneBlockObject(cx, sharedBlock, fp);
        if (!clone)
            return NULL;

        newChild->setParent(NonFunObjTag(*clone));
        newChild = clone;
    }
    newChild->setParent(NonFunObjTag(*fp->scopeChain));


    



    JS_ASSERT_IF(limitBlock &&
                 limitBlock->getClass() == &js_BlockClass &&
                 limitClone->getPrivate() == js_FloatingFrameIfGenerator(cx, fp),
                 sharedBlock);

    
    fp->scopeChain = innermostNewChild;
    return fp->scopeChain;
}

JSBool
js_GetPrimitiveThis(JSContext *cx, Value *vp, Class *clasp, const Value **vpp)
{
    const Value *p = &vp[1];
    if (p->isObject()) {
        JSObject *obj = ComputeThisObjectFromVp(cx, vp);
        if (!InstanceOf(cx, obj, clasp, vp + 2))
            return JS_FALSE;
        *vpp = &obj->getPrimitiveThis();
    } else {
        *vpp = p;
    }
    return JS_TRUE;
}
















JS_STATIC_INTERPRET bool
ComputeGlobalThis(JSContext *cx, Value *argv)
{
    JSObject *thisp;
    if (argv[-2].isPrimitive() || !argv[-2].asObject().getParent())
        thisp = cx->globalObject;
    else
        thisp = argv[-2].asObject().getGlobal();
    return JSObject::thisObject(cx, NonFunObjTag(*thisp), &argv[-1]);
}

static bool
ComputeThis(JSContext *cx, Value *argv)
{
    JS_ASSERT(!argv[-1].isNull());
    if (!argv[-1].isObject())
        return js_PrimitiveToObject(cx, &argv[-1]);

    Value thisv = argv[-1];
    JSObject *thisp = &thisv.asObject();
    if (thisp->getClass() == &js_CallClass || thisp->getClass() == &js_BlockClass)
        return ComputeGlobalThis(cx, argv);

    return JSObject::thisObject(cx, thisv, &argv[-1]);
}

namespace js {

bool
ComputeThisFromArgv(JSContext *cx, Value *argv)
{
    JS_ASSERT(!argv[-1].isMagic());  
    if (argv[-1].isNull())
        return ComputeGlobalThis(cx, argv);
    return ComputeThis(cx, argv);
}

}

#if JS_HAS_NO_SUCH_METHOD

const uint32 JSSLOT_FOUND_FUNCTION  = JSSLOT_PRIVATE;
const uint32 JSSLOT_SAVED_ID        = JSSLOT_PRIVATE + 1;

Class js_NoSuchMethodClass = {
    "NoSuchMethod",
    JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_IS_ANONYMOUS,
    PropertyStub,     PropertyStub,     PropertyStub,      PropertyStub,
    EnumerateStub,    ResolveStub,      ConvertStub,       NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};















JS_STATIC_INTERPRET JSBool
js_OnUnknownMethod(JSContext *cx, Value *vp)
{
    JS_ASSERT(!vp[1].isPrimitive());

    JSObject *obj = &vp[1].asObject();
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.noSuchMethodAtom);
    AutoValueRooter tvr(cx);
    if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, tvr.addr()))
        return false;
    if (tvr.value().isPrimitive()) {
        vp[0] = tvr.value();
    } else {
#if JS_HAS_XML_SUPPORT
        
        if (vp[0].isObject()) {
            obj = &vp[0].asObject();
            if (!js_IsFunctionQName(cx, obj, &id))
                return false;
            if (id != 0)
                vp[0] = IdToValue(id);
        }
#endif
        obj = NewObjectWithGivenProto(cx, &js_NoSuchMethodClass, NULL, NULL);
        if (!obj)
            return false;
        obj->fslots[JSSLOT_FOUND_FUNCTION] = tvr.value();
        obj->fslots[JSSLOT_SAVED_ID] = vp[0];
        vp[0].setNonFunObj(*obj);
    }
    return true;
}

static JS_REQUIRES_STACK JSBool
NoSuchMethod(JSContext *cx, uintN argc, Value *vp, uint32 flags)
{
    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, 2, args))
        return JS_FALSE;

    JS_ASSERT(vp[0].isObject());
    JS_ASSERT(vp[1].isObject());
    JSObject *obj = &vp[0].asObject();
    JS_ASSERT(obj->getClass() == &js_NoSuchMethodClass);

    Value *invokevp = args.getvp();
    invokevp[0] = obj->fslots[JSSLOT_FOUND_FUNCTION];
    invokevp[1] = vp[1];
    invokevp[2] = obj->fslots[JSSLOT_SAVED_ID];
    JSObject *argsobj = js_NewArrayObject(cx, argc, vp + 2);
    if (!argsobj)
        return JS_FALSE;
    invokevp[3].setNonFunObj(*argsobj);
    JSBool ok = (flags & JSINVOKE_CONSTRUCT)
                ? InvokeConstructor(cx, args, JS_TRUE)
                : Invoke(cx, args, flags);
    vp[0] = invokevp[0];
    return ok;
}

#endif 

namespace js {

const Value::MaskType PrimitiveValue::Masks[PrimitiveValue::THISP_ARRAY_SIZE] = {
    0,                                                          
    JSVAL_STRING_MASK,                                          
    JSVAL_NUMBER_MASK,                                          
    JSVAL_NUMBER_MASK | JSVAL_STRING_MASK,                      
    JSVAL_BOOLEAN_MASK,                                         
    JSVAL_BOOLEAN_MASK | JSVAL_STRING_MASK,                     
    JSVAL_BOOLEAN_MASK | JSVAL_NUMBER_MASK,                     
    JSVAL_BOOLEAN_MASK | JSVAL_NUMBER_MASK | JSVAL_STRING_MASK  
};







JS_REQUIRES_STACK bool
Invoke(JSContext *cx, const InvokeArgsGuard &args, uintN flags)
{
    Value *vp = args.getvp();
    uintN argc = args.getArgc();
    JS_ASSERT(argc <= JS_ARGS_LENGTH_MAX);
    JS_ASSERT(!vp[1].isUndefined());

    if (vp[0].isPrimitive()) {
        js_ReportIsNotFunction(cx, vp, flags & JSINVOKE_FUNFLAGS);
        return false;
    }

    JSObject *funobj = &vp[0].asObject();
    JSObject *parent = funobj->getParent();
    Class *clasp = funobj->getClass();

    
    Native native;
    JSFunction *fun;
    JSScript *script;
    if (clasp != &js_FunctionClass) {
#if JS_HAS_NO_SUCH_METHOD
        if (clasp == &js_NoSuchMethodClass)
            return NoSuchMethod(cx, argc, vp, flags);
#endif

        
        const JSObjectOps *ops = funobj->map->ops;

        fun = NULL;
        script = NULL;

        
        if (flags & JSINVOKE_CONSTRUCT) {
            if (!vp[1].isObjectOrNull()) {
                if (!js_PrimitiveToObject(cx, &vp[1]))
                    return false;
            }
            native = ops->construct;
        } else {
            native = ops->call;
        }
        if (!native) {
            js_ReportIsNotFunction(cx, vp, flags & JSINVOKE_FUNFLAGS);
            return false;
        }
    } else {
        
        fun = GET_FUNCTION_PRIVATE(cx, funobj);
        if (FUN_INTERPRETED(fun)) {
            native = NULL;
            script = fun->u.i.script;
            JS_ASSERT(script);

            if (script->isEmpty()) {
                if (flags & JSINVOKE_CONSTRUCT) {
                    JS_ASSERT(vp[1].isObject());
                    *vp = vp[1];
                } else {
                    vp->setUndefined();
                }
                return true;
            }
        } else {
            native = fun->u.n.native;
            script = NULL;
        }

        if (JSFUN_BOUND_METHOD_TEST(fun->flags)) {
            
            vp[1].setObjectOrNull(parent);
        } else if (vp[1].isPrimitive()) {
            JS_ASSERT(!(flags & JSINVOKE_CONSTRUCT));
            if (PrimitiveValue::test(fun, vp[1]))
                goto start_call;
        }
    }

    if (flags & JSINVOKE_CONSTRUCT) {
        JS_ASSERT(vp[1].isObject());
    } else {
        









        if (native && (!fun || !(fun->flags & JSFUN_FAST_NATIVE))) {
            if (!ComputeThisFromArgv(cx, vp + 2))
                return false;
            flags |= JSFRAME_COMPUTED_THIS;
        }
    }

  start_call:
    if (native && fun && fun->isFastNative()) {
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif
        JSBool ok = ((FastNative) native)(cx, argc, vp);
        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
        return ok;
    }

    
    uintN nmissing;
    uintN nvars;
    if (fun) {
        if (fun->isInterpreted()) {
            uintN minargs = fun->nargs;
            nmissing = minargs > argc ? minargs - argc : 0;
            nvars = fun->u.i.nvars;
        } else if (fun->isFastNative()) {
            nvars = nmissing = 0;
        } else {
            uintN minargs = fun->nargs;
            nmissing = (minargs > argc ? minargs - argc : 0) + fun->u.n.extra;
            nvars = 0;
        }
    } else {
        nvars = nmissing = 0;
    }

    uintN nfixed = script ? script->nslots : 0;

    



    JSFrameRegs regs;
    InvokeFrameGuard frame;
    if (!cx->stack().getInvokeFrame(cx, args, nmissing, nfixed, frame))
        return false;
    JSStackFrame *fp = frame.getFrame();

    
    Value *missing = vp + 2 + argc;
    for (Value *v = missing, *end = missing + nmissing; v != end; ++v)
        v->setUndefined();
    for (Value *v = fp->slots(), *end = v + nvars; v != end; ++v)
        v->setUndefined();

    
    fp->thisv = vp[1];
    fp->callobj = NULL;
    fp->argsobj = NULL;
    fp->script = script;
    fp->fun = fun;
    fp->argc = argc;
    fp->argv = vp + 2;
    if (flags & JSINVOKE_CONSTRUCT)
        fp->rval = fp->thisv;
    else
        fp->rval.setUndefined();
    fp->annotation = NULL;
    fp->scopeChain = NULL;
    fp->blockChain = NULL;
    fp->imacpc = NULL;
    fp->flags = flags;
    fp->displaySave = NULL;

    
    if (script) {
        regs.pc = script->code;
        regs.sp = fp->slots() + script->nfixed;
    } else {
        regs.pc = NULL;
        regs.sp = fp->slots();
    }

    
    cx->stack().pushInvokeFrame(cx, args, frame, regs);

    
    if (native) {
        
        if (JSStackFrame *down = fp->down)
            fp->scopeChain = down->scopeChain;

        
        if (!fp->scopeChain)
            fp->scopeChain = parent;
    } else {
        
        fp->scopeChain = parent;
        if (fun->isHeavyweight() && !js_GetCallObject(cx, fp))
            return false;
    }

    
    JSInterpreterHook hook = cx->debugHooks->callHook;
    void *hookData = NULL;
    if (hook)
        hookData = hook(cx, fp, JS_TRUE, 0, cx->debugHooks->callHookData);

    DTrace::enterJSFun(cx, fp, fun, fp->down, fp->argc, fp->argv);

    
    JSBool ok;
    if (native) {
#ifdef DEBUG_NOT_THROWING
        JSBool alreadyThrowing = cx->throwing;
#endif
        
        JSObject *thisp = &fp->thisv.asObject();
        ok = native(cx, thisp, fp->argc, fp->argv, &fp->rval);
        JS_ASSERT(cx->fp == fp);
        JS_RUNTIME_METER(cx->runtime, nativeCalls);
#ifdef DEBUG_NOT_THROWING
        if (ok && !alreadyThrowing)
            ASSERT_NOT_THROWING(cx);
#endif
    } else {
        JS_ASSERT(script);
        ok = Interpret(cx);
    }

    DTrace::exitJSFun(cx, fp, fun, fp->rval);

    if (hookData) {
        hook = cx->debugHooks->callHook;
        if (hook)
            hook(cx, fp, JS_FALSE, &ok, hookData);
    }

    fp->putActivationObjects(cx);
    *vp = fp->rval;
    return ok;
}

JS_REQUIRES_STACK JS_FRIEND_API(bool)
InvokeFriendAPI(JSContext *cx, const InvokeArgsGuard &args, uintN flags)
{
    return Invoke(cx, args, flags);
}

bool
InternalInvoke(JSContext *cx, JSObject *obj, const Value &fval, uintN flags,
               uintN argc, const Value *argv, Value *rval)
{
    LeaveTrace(cx);

    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, argc, args))
        return JS_FALSE;

    args.getvp()[0] = fval;
    args.getvp()[1].setObjectOrNull(obj);
    memcpy(args.getvp() + 2, argv, argc * sizeof(*argv));

    if (!Invoke(cx, args, flags))
        return JS_FALSE;

    






    *rval = *args.getvp();
    if (rval->isGCThing()) {
        JSLocalRootStack *lrs = JS_THREAD_DATA(cx)->localRootStack;
        if (lrs) {
            if (js_PushLocalRoot(cx, lrs, rval->asGCThing()) < 0)
                return JS_FALSE;
        } else {
            cx->weakRoots.lastInternalResult = rval->asGCThing();
        }
    }
    return JS_TRUE;
}

bool
InternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, const Value &fval,
                 JSAccessMode mode, uintN argc, const Value *argv,
                 Value *rval)
{
    LeaveTrace(cx);

    



    JS_CHECK_RECURSION(cx, return JS_FALSE);

    return InternalCall(cx, obj, fval, argc, argv, rval);
}

bool
Execute(JSContext *cx, JSObject *chain, JSScript *script,
        JSStackFrame *down, uintN flags, Value *result)
{
    if (script->isEmpty()) {
        if (result)
            result->setUndefined();
        return JS_TRUE;
    }

    LeaveTrace(cx);

    DTrace::ExecutionScope executionScope(script);
    






    JSFrameRegs regs;
    ExecuteFrameGuard frame;
    if (!cx->stack().getExecuteFrame(cx, down, 0, script->nslots, frame))
        return false;
    JSStackFrame *fp = frame.getFrame();

    
    PodZero(fp->slots(), script->nfixed);
#if JS_HAS_SHARP_VARS
    JS_STATIC_ASSERT(SHARP_NSLOTS == 2);
    if (script->hasSharps) {
        JS_ASSERT(script->nfixed >= SHARP_NSLOTS);
        Value *sharps = &fp->slots()[script->nfixed - SHARP_NSLOTS];
        if (down && down->script && down->script->hasSharps) {
            JS_ASSERT(down->script->nfixed >= SHARP_NSLOTS);
            int base = (down->fun && !(down->flags & JSFRAME_SPECIAL))
                       ? down->fun->sharpSlotBase(cx)
                       : down->script->nfixed - SHARP_NSLOTS;
            if (base < 0)
                return false;
            sharps[0] = down->slots()[base];
            sharps[1] = down->slots()[base + 1];
        } else {
            sharps[0].setUndefined();
            sharps[1].setUndefined();
        }
    }
#endif

    
    JSObject *initialVarObj;
    if (down) {
        
        fp->callobj = down->callobj;
        fp->argsobj = down->argsobj;
        fp->fun = (script->staticLevel > 0) ? down->fun : NULL;
        fp->thisv = down->thisv;
        fp->flags = flags | (down->flags & JSFRAME_COMPUTED_THIS);
        fp->argc = down->argc;
        fp->argv = down->argv;
        fp->annotation = down->annotation;
        fp->scopeChain = chain;

        






        initialVarObj = (down == cx->fp)
                        ? down->varobj(cx)
                        : down->varobj(cx->containingCallStack(down));
    } else {
        fp->callobj = NULL;
        fp->argsobj = NULL;
        JSObject *obj = chain;
        if (cx->options & JSOPTION_VAROBJFIX)
            obj = obj->getGlobal();
        fp->fun = NULL;
        JSObject *thisp = chain->thisObject(cx);
        if (!thisp)
            return false;
        fp->thisv.setObject(*thisp);
        fp->flags = flags | JSFRAME_COMPUTED_THIS;
        fp->argc = 0;
        fp->argv = NULL;
        fp->annotation = NULL;
        Innerize(cx, &chain);
        if (!chain)
            return false;
        fp->scopeChain = chain;

        initialVarObj = obj;
    }
    fp->script = script;
    fp->imacpc = NULL;
    fp->rval.setUndefined();
    fp->blockChain = NULL;

    
    regs.pc = script->code;
    regs.sp = fp->base();

    
    cx->stack().pushExecuteFrame(cx, frame, regs, initialVarObj);

    void *hookData = NULL;
    if (JSInterpreterHook hook = cx->debugHooks->executeHook)
        hookData = hook(cx, fp, JS_TRUE, 0, cx->debugHooks->executeHookData);

    JSBool ok = Interpret(cx);
    if (result)
        *result = fp->rval;

    if (hookData) {
        if (JSInterpreterHook hook = cx->debugHooks->executeHook)
            hook(cx, fp, JS_FALSE, &ok, hookData);
    }

    return ok;
}

bool
CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                   JSObject **objp, JSProperty **propp)
{
    JSObject *obj2;
    JSProperty *prop;
    uintN oldAttrs, report;
    bool isFunction;
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
            Value value;
            if (!obj->getProperty(cx, id, &value))
                return JS_FALSE;
            isFunction = value.isFunObj();
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
    name = js_ValueToPrintableString(cx, IdToValue(id));
    if (!name)
        return JS_FALSE;
    return JS_ReportErrorFlagsAndNumber(cx, report,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_REDECLARED_VAR,
                                        type, name);
}

static JS_ALWAYS_INLINE bool
EqualObjects(JSContext *cx, JSObject *lobj, JSObject *robj)
{
    return lobj == robj ||
           js_GetWrappedObject(cx, lobj) == js_GetWrappedObject(cx, robj);
}

bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval)
{
    JS_STATIC_ASSERT(JSVAL_NULL_MASK == 0);

    Value::MaskType lmask = lval.mask, rmask = rval.mask;
    Value::MaskType maskor = lmask | rmask;

    if (lmask == rmask) {
        if (Value::isSingleton(lmask))
            return true;
        if (lmask == JSVAL_INT32_MASK)
            return lval.data.i32 == rval.data.i32;
        if (lmask == JSVAL_DOUBLE_MASK)
            return JSDOUBLE_COMPARE(lval.data.dbl, ==, rval.data.dbl, JS_FALSE);
        if (lmask == JSVAL_STRING_MASK)
            return js_EqualStrings(lval.data.str, rval.data.str);
        if (lmask & JSVAL_OBJECT_MASK)
            return EqualObjects(cx, lval.data.obj, rval.data.obj);
        JS_ASSERT(lmask == JSVAL_BOOLEAN_MASK);
        return lval.data.boo == rval.data.boo;
    }

    

    if (maskor == JSVAL_NUMBER_MASK) {
        double ld = lmask == JSVAL_DOUBLE_MASK ? lval.data.dbl : lval.data.i32;
        double rd = rmask == JSVAL_DOUBLE_MASK ? rval.data.dbl : rval.data.i32;
        return JSDOUBLE_COMPARE(ld, ==, rd, JS_FALSE);
    }

    if (maskor == JSVAL_OBJECT_MASK)
        return EqualObjects(cx, lval.data.obj, rval.data.obj);

    return false;
}

static inline bool
IsNegativeZero(const Value &v)
{
    return v.isDouble() && JSDOUBLE_IS_NEGZERO(v.asDouble());
}

static inline bool
IsNaN(const Value &v)
{
    return v.isDouble() && JSDOUBLE_IS_NaN(v.asDouble());
}

bool
SameValue(JSContext *cx, const Value &v1, const Value &v2)
{
    if (IsNegativeZero(v1))
        return IsNegativeZero(v2);
    if (IsNegativeZero(v2))
        return false;
    if (IsNaN(v1) && IsNaN(v2))
        return true;
    return StrictlyEqual(cx, v1, v2);
}

JSType
TypeOfValue(JSContext *cx, const Value &vref)
{
    Value v = vref;
    if (v.isNumber())
        return JSTYPE_NUMBER;
    if (v.isString())
        return JSTYPE_STRING;
    if (v.isNull())
        return JSTYPE_OBJECT;
    if (v.isUndefined())
        return JSTYPE_VOID;
    if (v.isObject())
        return v.asObject().map->ops->typeOf(cx, &v.asObject());
    JS_ASSERT(v.isBoolean());
    return JSTYPE_BOOLEAN;
}

bool
InstanceOfSlow(JSContext *cx, JSObject *obj, Class *clasp, Value *argv)
{
    JS_ASSERT(!obj || obj->getClass() != clasp);
    if (argv) {
        JSFunction *fun = js_ValueToFunction(cx, &argv[-2], 0);
        if (fun) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_INCOMPATIBLE_PROTO,
                                 clasp->name, JS_GetFunctionName(fun),
                                 obj
                                 ? obj->getClass()->name
                                 : js_null_str);
        }
    }
    return false;
}

JS_REQUIRES_STACK bool
InvokeConstructor(JSContext *cx, const InvokeArgsGuard &args, JSBool clampReturn)
{
    JSFunction *fun = NULL;
    Value *vp = args.getvp();

    
    if (vp->isPrimitive() || vp->isFunObj() ||
        vp->asNonFunObj().map->ops->construct)
    {
        fun = js_ValueToFunction(cx, vp, JSV2F_CONSTRUCT);
        if (!fun)
            return JS_FALSE;
    }

    JSObject *proto, *parent;
    Class *clasp = &js_ObjectClass;
    if (vp->isPrimitive()) {
        proto = NULL;
        parent = NULL;
        fun = NULL;
    } else {
        





        JSObject *obj2 = &vp->asObject();
        if (!obj2->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                               &vp[1])) {
            return JS_FALSE;
        }
        const Value &v = vp[1];
        if (v.isObjectOrNull())
            proto = v.asObjectOrNull();
        else
            proto = NULL;
        parent = obj2->getParent();

        if (obj2->getClass() == &js_FunctionClass) {
            JSFunction *f = GET_FUNCTION_PRIVATE(cx, obj2);
            if (!f->isInterpreted() && f->u.n.clasp)
                clasp = f->u.n.clasp;
        }
    }
    JSObject *obj = NewObject(cx, clasp, proto, parent);
    if (!obj)
        return JS_FALSE;

    
    vp[1].setObject(*obj);
    if (!Invoke(cx, args, JSINVOKE_CONSTRUCT))
        return JS_FALSE;

    
    if (clampReturn && vp->isPrimitive()) {
        if (!fun) {
            
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_NEW_RESULT,
                                 js_ValueToPrintableString(cx, *vp));
            return JS_FALSE;
        }
        vp->setObject(*obj);
    }

    JS_RUNTIME_METER(cx->runtime, constructs);
    return JS_TRUE;
}

Value
BoxedWordToValue(jsboxedword w)
{
    if (JSBOXEDWORD_IS_STRING(w))
        return StringTag(JSBOXEDWORD_TO_STRING(w));
    if (JSBOXEDWORD_IS_INT(w))
        return Int32Tag(JSBOXEDWORD_TO_INT(w));
    if (JSBOXEDWORD_IS_DOUBLE(w))
        return DoubleTag(*JSBOXEDWORD_TO_DOUBLE(w));
    if (JSBOXEDWORD_IS_OBJECT(w))
        return ObjectOrNullTag(JSBOXEDWORD_TO_OBJECT(w));
    if (JSBOXEDWORD_IS_VOID(w))
        return UndefinedTag();
    return BooleanTag(JSBOXEDWORD_TO_BOOLEAN(w));
}

bool
ValueToBoxedWord(JSContext *cx, const Value &v, jsboxedword *wp)
{
    int32_t i;
    if (v.isInt32() &&
        INT32_FITS_IN_JSID((i = v.asInt32()))) {
        *wp = INT_TO_JSBOXEDWORD(i);
        return true;
    }
    if (v.isString()) {
        *wp = STRING_TO_JSBOXEDWORD(v.asString());
        return true;
    }
    if (v.isObjectOrNull()) {
        *wp = OBJECT_TO_JSBOXEDWORD(v.asObjectOrNull());
        return true;
    }
    if (v.isBoolean()) {
        *wp = BOOLEAN_TO_JSBOXEDWORD(v.asBoolean());
        return true;
    }
    if (v.isUndefined()) {
        *wp = JSBOXEDWORD_VOID;
        return true;
    }
    double *dp = js_NewWeaklyRootedDoubleAtom(cx, v.asDouble());
    if (!dp)
        return false;
    *wp = DOUBLE_TO_JSBOXEDWORD(dp);
    return true;
}

Value
IdToValue(jsid id)
{
    return BoxedWordToValue(id);
}

bool
ValueToId(JSContext *cx, const Value &v, jsid *idp)
{
    int32_t i;
    if (ValueFitsInInt32(v, &i) && INT32_FITS_IN_JSID(i)) {
        *idp = INT_TO_JSID(i);
        return true;
    }

#if JS_HAS_XML_SUPPORT
    if (v.isObject()) {
        Class *clasp = v.asObject().getClass();
        if (JS_UNLIKELY(clasp == &js_QNameClass.base ||
                        clasp == &js_AttributeNameClass ||
                        clasp == &js_AnyNameClass)) {
            *idp = OBJECT_TO_JSID(&v.asObject());
            return true;
        }
    }
#endif

    return js_ValueToStringId(cx, v, idp);
}

} 





JS_STATIC_INTERPRET JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex)
{
    JSStackFrame *fp = cx->fp;
    Value *sp = cx->regs->sp;
    JS_ASSERT(stackIndex < 0);
    JS_ASSERT(fp->base() <= sp + stackIndex);

    JSObject *obj;
    if (sp[-1].isObject()) {
        obj = &sp[-1].asObject();
    } else {
        if (!js_ValueToNonNullObject(cx, sp[-1], &sp[-1]))
            return JS_FALSE;
        obj = &sp[-1].asObject();
    }

    JSObject *parent = js_GetScopeChain(cx, fp);
    if (!parent)
        return JS_FALSE;

    Innerize(cx, &obj);
    if (!obj)
        return JS_FALSE;

    JSObject *withobj = js_NewWithObject(cx, obj, parent,
                                         sp + stackIndex - fp->base());
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
    JS_ASSERT(withobj->getPrivate() == js_FloatingFrameIfGenerator(cx, cx->fp));
    JS_ASSERT(OBJ_BLOCK_DEPTH(cx, withobj) >= 0);
    cx->fp->scopeChain = withobj->getParent();
    withobj->setPrivate(NULL);
}

JS_REQUIRES_STACK Class *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth)
{
    Class *clasp;

    clasp = obj->getClass();
    if ((clasp == &js_WithClass || clasp == &js_BlockClass) &&
        obj->getPrivate() == js_FloatingFrameIfGenerator(cx, cx->fp) &&
        OBJ_BLOCK_DEPTH(cx, obj) >= stackDepth) {
        return clasp;
    }
    return NULL;
}





JS_STATIC_INTERPRET JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, jsint stackDepth, JSBool normalUnwind)
{
    JSObject *obj;
    Class *clasp;

    JS_ASSERT(stackDepth >= 0);
    JS_ASSERT(cx->fp->base() + stackDepth <= cx->regs->sp);

    JSStackFrame *fp = cx->fp;
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

    cx->regs->sp = fp->base() + stackDepth;
    return normalUnwind;
}

JS_STATIC_INTERPRET JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, Value *vp, Value *vp2)
{
    if (cs->format & JOF_POST) {
        double d;
        if (!ValueToNumber(cx, *vp, &d))
            return JS_FALSE;
        vp->setDouble(d);
        (cs->format & JOF_INC) ? ++d : --d;
        vp2->setDouble(d);
    }

    double d;
    if (!ValueToNumber(cx, *vp, &d))
        return JS_FALSE;
    (cs->format & JOF_INC) ? ++d : --d;
    vp->setDouble(d);
    vp2->setDouble(d);
    return JS_TRUE;
}

const Value &
js_GetUpvar(JSContext *cx, uintN level, uintN cookie)
{
    level -= UPVAR_FRAME_SKIP(cookie);
    JS_ASSERT(level < JS_DISPLAY_SIZE);

    JSStackFrame *fp = cx->display[level];
    JS_ASSERT(fp->script);

    uintN slot = UPVAR_FRAME_SLOT(cookie);
    Value *vp;

    if (!fp->fun || (fp->flags & JSFRAME_EVAL)) {
        vp = fp->slots() + fp->script->nfixed;
    } else if (slot < fp->fun->nargs) {
        vp = fp->argv;
    } else if (slot == CALLEE_UPVAR_SLOT) {
        vp = &fp->argv[-2];
        slot = 0;
    } else {
        slot -= fp->fun->nargs;
        JS_ASSERT(slot < fp->script->nslots);
        vp = fp->slots();
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
    JSString *str;
    JSOp op;

    tracefp = (FILE *) cx->tracefp;
    JS_ASSERT(tracefp);
    fp = cx->fp;
    regs = cx->regs;

    



    if (cx->tracePrevPc && regs->pc >= fp->script->main) {
        JSOp tracePrevOp = JSOp(*cx->tracePrevPc);
        ndefs = js_GetStackDefs(cx, &js_CodeSpec[tracePrevOp], tracePrevOp,
                                fp->script, cx->tracePrevPc);

        




        if (ndefs != 0 &&
            ndefs < regs->sp - fp->slots()) {
            for (n = -ndefs; n < 0; n++) {
                char *bytes = js_DecompileValueGenerator(cx, n, regs->sp[n], NULL);
                if (bytes) {
                    fprintf(tracefp, "%s %s",
                            (n == -ndefs) ? "  output:" : ",",
                            bytes);
                    cx->free(bytes);
                } else {
                    JS_ClearPendingException(cx);
                }
            }
            fprintf(tracefp, " @ %u\n", (uintN) (regs->sp - fp->base()));
        }
        fprintf(tracefp, "  stack: ");
        for (Value *siter = fp->base(); siter < regs->sp; siter++) {
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
            char *bytes = js_DecompileValueGenerator(cx, n, regs->sp[n], NULL);
            if (bytes) {
                fprintf(tracefp, "%s %s",
                        (n == -nuses) ? "  inputs:" : ",",
                        bytes);
                cx->free(bytes);
            } else {
                JS_ClearPendingException(cx);
            }
        }
        fprintf(tracefp, " @ %u\n", (uintN) (regs->sp - fp->base()));
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

#define PUSH_COPY(v)             *regs.sp++ = v
#define PUSH_NULL()              regs.sp++->setNull()
#define PUSH_UNDEFINED()         regs.sp++->setUndefined()
#define PUSH_BOOLEAN(b)          regs.sp++->setBoolean(b)
#define PUSH_DOUBLE(d)           regs.sp++->setDouble(d)
#define PUSH_INT32(i)            regs.sp++->setInt32(i)
#define PUSH_STRING(s)           regs.sp++->setString(s)
#define PUSH_NONFUNOBJ(obj)      regs.sp++->setNonFunObj(obj)
#define PUSH_FUNOBJ(obj)         regs.sp++->setFunObj(obj)
#define PUSH_OBJECT(obj)         regs.sp++->setObject(obj)
#define PUSH_OBJECT_OR_NULL(obj) regs.sp++->setObject(obj)
#define PUSH_HOLE()              regs.sp++->setMagic(JS_ARRAY_HOLE)
#define POP_COPY_TO(v)           v = *--regs.sp






#define STORE_NUMBER(cx, n, d)                                                \
    JS_BEGIN_MACRO                                                            \
        int32_t i_;                                                           \
        if (JSDOUBLE_IS_INT32(d, i_))                                         \
            regs.sp[n].setInt32(i_);                                          \
        else                                                                  \
            regs.sp[n].setDouble(d);                                          \
    JS_END_MACRO

#define POP_BOOLEAN(cx, vp, b)                                                \
    JS_BEGIN_MACRO                                                            \
        vp = &regs.sp[-1];                                                    \
        if (vp->isNull()) {                                                   \
            b = false;                                                        \
        } else if (vp->isBoolean()) {                                         \
            b = vp->asBoolean();                                              \
        } else {                                                              \
            b = js_ValueToBoolean(*vp);                                       \
        }                                                                     \
        regs.sp--;                                                            \
    JS_END_MACRO

#define VALUE_TO_OBJECT(cx, vp, obj)                                          \
    JS_BEGIN_MACRO                                                            \
        if ((vp)->isObject()) {                                               \
            obj = &(vp)->asObject();                                          \
        } else {                                                              \
            if (!js_ValueToNonNullObject(cx, *(vp), (vp)))                    \
                goto error;                                                   \
            obj = &(vp)->asObject();                                          \
        }                                                                     \
    JS_END_MACRO

#define FETCH_OBJECT(cx, n, obj)                                              \
    JS_BEGIN_MACRO                                                            \
        Value *vp_ = &regs.sp[n];                                             \
        VALUE_TO_OBJECT(cx, vp_, obj);                                        \
    JS_END_MACRO


static JS_ALWAYS_INLINE bool
CanIncDecWithoutOverflow(const Value &v)
{
    return v.isInt32() &&
           ((uint32_t(v.asInt32()) + 1) - uint32_t(JSVAL_INT_MIN)) > 1;
}





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
                     &sprop->methodFunObj() == &pobj->lockedGetSlot(sprop->slot).asFunObj());
    } else {
        Value v;
        JS_ASSERT(entry->vword.isFunObj());
        JS_ASSERT(!entry->vword.isNull());
        JS_ASSERT(pobj->scope()->brandedOrHasMethodBarrier());
        JS_ASSERT(sprop->hasDefaultGetterOrIsMethod());
        JS_ASSERT(SPROP_HAS_VALID_SLOT(sprop, pobj->scope()));
        v = pobj->lockedGetSlot(sprop->slot);
        JS_ASSERT(&entry->vword.toFunObj() == &v.asFunObj());

        if (sprop->isMethod()) {
            JS_ASSERT(js_CodeSpec[*regs.pc].format & JOF_CALLOP);
            JS_ASSERT(&sprop->methodFunObj() == &v.asFunObj());
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
IteratorMore(JSContext *cx, JSObject *iterobj, bool *cond, Value *rval)
{
    if (iterobj->getClass() == &js_IteratorClass.base) {
        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();
        *cond = (ni->props_cursor < ni->props_end);
    } else {
        if (!js_IteratorMore(cx, iterobj, rval))
            return false;
        *cond = rval->isTrue();
    }
    return true;
}

static inline bool
IteratorNext(JSContext *cx, JSObject *iterobj, Value *rval)
{
    if (iterobj->getClass() == &js_IteratorClass.base) {
        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();
        JS_ASSERT(ni->props_cursor < ni->props_end);
        *rval = BoxedWordToValue(*ni->props_cursor);
        if (rval->isString() || (ni->flags & JSITER_FOREACH)) {
            ni->props_cursor++;
            return true;
        }
        
    }
    return js_IteratorNext(cx, iterobj, rval);
}


namespace js {

JS_REQUIRES_STACK bool
Interpret(JSContext *cx)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_INTERP);
#endif
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

    JSRuntime *const rt = cx->runtime;

    
    JSStackFrame *fp = cx->fp;
    JSScript *script = fp->script;
    JS_ASSERT(!script->isEmpty());
    JS_ASSERT(script->length > 1);

    
    uintN inlineCallCount = 0;

    






    JSAtom **atoms = script->atomMap.vector;

#define LOAD_ATOM(PCOFF, atom)                                                \
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

#define LOAD_OBJECT(PCOFF, obj)                                               \
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
        JS_ASSERT(cx->regs == &regs);                                         \
        if (cx->throwing)                                                     \
            goto error;                                                       \
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

    








    JSVersion currentVersion = (JSVersion) script->version;
    JSVersion originalVersion = (JSVersion) cx->version;
    if (currentVersion != originalVersion)
        js_SetVersion(cx, currentVersion);

    
    if (script->staticLevel < JS_DISPLAY_SIZE) {
        JSStackFrame **disp = &cx->display[script->staticLevel];
        fp->displaySave = *disp;
        *disp = fp;
    }

#define CHECK_INTERRUPT_HANDLER()                                             \
    JS_BEGIN_MACRO                                                            \
        if (cx->debugHooks->interruptHook)                                    \
            ENABLE_INTERRUPTS();                                              \
    JS_END_MACRO

    





    CHECK_INTERRUPT_HANDLER();

    



    JS_ASSERT(cx->regs);
    JSFrameRegs *const prevContextRegs = cx->regs;
    JSFrameRegs regs = *cx->regs;
    cx->setCurrentRegs(&regs);

#if JS_HAS_GENERATORS
    if (JS_UNLIKELY(fp->isGenerator())) {
        JS_ASSERT(prevContextRegs == &cx->generatorFor(fp)->savedRegs);
        JS_ASSERT((size_t) (regs.pc - script->code) <= script->length);
        JS_ASSERT((size_t) (regs.sp - fp->base()) <= StackDepth(script));

        



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

    
    JSBool interpReturnOK;
    JSAtom *atomNotDefined;

    





    JSOp op;
#ifdef JS_THREADED_INTERP
    {
        jsint len = 0;
        DO_NEXT_OP(len);
    }
#else
    
    jsint len = 0;
    DO_NEXT_OP(len);
#endif

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
    JS_ASSERT(cx->regs == &regs);
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
        
        interpReturnOK = JS_FALSE;
    } else {
        JSThrowHook handler;
        JSTryNote *tn, *tnlimit;
        uint32 offset;

        
        handler = cx->debugHooks->throwHook;
        if (handler) {
            Value rval;
            switch (handler(cx, script, regs.pc, Jsvalify(&rval),
                            cx->debugHooks->throwHookData)) {
              case JSTRAP_ERROR:
                cx->throwing = JS_FALSE;
                goto error;
              case JSTRAP_RETURN:
                cx->throwing = JS_FALSE;
                fp->rval = rval;
                interpReturnOK = JS_TRUE;
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

            


















            if (tn->stackDepth > regs.sp - fp->base())
                continue;

            




            regs.pc = (script)->main + tn->start + tn->length;

            JSBool ok = js_UnwindScope(cx, tn->stackDepth, JS_TRUE);
            JS_ASSERT(regs.sp == fp->base() + tn->stackDepth);
            if (!ok) {
                



                goto error;
            }

            switch (tn->kind) {
              case JSTRY_CATCH:
                JS_ASSERT(js_GetOpcode(cx, fp->script, regs.pc) == JSOP_ENTERBLOCK);

#if JS_HAS_GENERATORS
                
                if (JS_UNLIKELY(cx->exception.isMagic(JS_GENERATOR_CLOSING)))
                    break;
#endif

                




                {
                    jsint len = 0;
                    DO_NEXT_OP(len);
                }

              case JSTRY_FINALLY:
                



                PUSH_BOOLEAN(true);
                PUSH_COPY(cx->exception);
                cx->throwing = JS_FALSE;
                {
                    jsint len = 0;
                    DO_NEXT_OP(len);
                }

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
        



        interpReturnOK = JS_FALSE;
#if JS_HAS_GENERATORS
        if (JS_UNLIKELY(cx->throwing &&
                        cx->exception.isMagic(JS_GENERATOR_CLOSING))) {
            cx->throwing = JS_FALSE;
            interpReturnOK = JS_TRUE;
            fp->rval.setUndefined();
        }
#endif
    }

  forced_return:
    






    interpReturnOK &= js_UnwindScope(cx, 0, interpReturnOK || cx->throwing);
    JS_ASSERT(regs.sp == fp->base());

#ifdef DEBUG
    cx->tracePrevPc = NULL;
#endif

    if (inlineCallCount)
        goto inline_return;

  exit:
    










    JS_ASSERT(inlineCallCount == 0);
    JS_ASSERT(cx->regs == &regs);
    *prevContextRegs = regs;
    cx->setCurrentRegs(prevContextRegs);

#ifdef JS_TRACER
    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "recording out of Interpret");
#endif

    JS_ASSERT_IF(!fp->isGenerator(), !fp->blockChain);
    JS_ASSERT_IF(!fp->isGenerator(), !js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));

    
    if (script->staticLevel < JS_DISPLAY_SIZE)
        cx->display[script->staticLevel] = fp->displaySave;
    if (cx->version == currentVersion && currentVersion != originalVersion)
        js_SetVersion(cx, originalVersion);
    --cx->interpLevel;

    return interpReturnOK;

  atom_not_defined:
    {
        const char *printable;

        printable = js_AtomToPrintableString(cx, atomNotDefined);
        if (printable)
            js_ReportIsNotDefined(cx, printable);
        goto error;
    }
}

} 

#endif 
