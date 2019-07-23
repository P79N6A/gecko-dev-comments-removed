










































#include "jsstddef.h"
#include <string.h>
#include "jstypes.h"
#include "jsbit.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsexn.h"

#if JS_HAS_GENERATORS
# include "jsiter.h"
#endif

#if JS_HAS_XDR
# include "jsxdrapi.h"
#endif


enum {
    CALL_ARGUMENTS  = -1,       
    ARGS_LENGTH     = -2,       
    ARGS_CALLEE     = -3,       
    FUN_ARITY       = -4,       
    FUN_NAME        = -5,       
    FUN_CALLER      = -6        
};

#if JSFRAME_OVERRIDE_BITS < 8
# error "not enough override bits in JSStackFrame.flags!"
#endif

#define TEST_OVERRIDE_BIT(fp, tinyid) \
    ((fp)->flags & JS_BIT(JSFRAME_OVERRIDE_SHIFT - ((tinyid) + 1)))

#define SET_OVERRIDE_BIT(fp, tinyid) \
    ((fp)->flags |= JS_BIT(JSFRAME_OVERRIDE_SHIFT - ((tinyid) + 1)))

JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, jsval *vp)
{
    JSObject *argsobj;

    if (TEST_OVERRIDE_BIT(fp, CALL_ARGUMENTS)) {
        JS_ASSERT(fp->callobj);
        return OBJ_GET_PROPERTY(cx, fp->callobj,
                                ATOM_TO_JSID(cx->runtime->atomState
                                             .argumentsAtom),
                                vp);
    }
    argsobj = js_GetArgsObject(cx, fp);
    if (!argsobj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(argsobj);
    return JS_TRUE;
}

static JSBool
MarkArgDeleted(JSContext *cx, JSStackFrame *fp, uintN slot)
{
    JSObject *argsobj;
    jsval bmapval, bmapint;
    size_t nbits, nbytes;
    jsbitmap *bitmap;

    argsobj = fp->argsobj;
    (void) JS_GetReservedSlot(cx, argsobj, 0, &bmapval);
    nbits = fp->argc;
    JS_ASSERT(slot < nbits);
    if (JSVAL_IS_VOID(bmapval)) {
        if (nbits <= JSVAL_INT_BITS) {
            bmapint = 0;
            bitmap = (jsbitmap *) &bmapint;
        } else {
            nbytes = JS_HOWMANY(nbits, JS_BITS_PER_WORD) * sizeof(jsbitmap);
            bitmap = (jsbitmap *) JS_malloc(cx, nbytes);
            if (!bitmap)
                return JS_FALSE;
            memset(bitmap, 0, nbytes);
            bmapval = PRIVATE_TO_JSVAL(bitmap);
            JS_SetReservedSlot(cx, argsobj, 0, bmapval);
        }
    } else {
        if (nbits <= JSVAL_INT_BITS) {
            bmapint = JSVAL_TO_INT(bmapval);
            bitmap = (jsbitmap *) &bmapint;
        } else {
            bitmap = (jsbitmap *) JSVAL_TO_PRIVATE(bmapval);
        }
    }
    JS_SET_BIT(bitmap, slot);
    if (bitmap == (jsbitmap *) &bmapint) {
        bmapval = INT_TO_JSVAL(bmapint);
        JS_SetReservedSlot(cx, argsobj, 0, bmapval);
    }
    return JS_TRUE;
}


static JSBool
ArgWasDeleted(JSContext *cx, JSStackFrame *fp, uintN slot)
{
    JSObject *argsobj;
    jsval bmapval, bmapint;
    jsbitmap *bitmap;

    argsobj = fp->argsobj;
    (void) JS_GetReservedSlot(cx, argsobj, 0, &bmapval);
    if (JSVAL_IS_VOID(bmapval))
        return JS_FALSE;
    if (fp->argc <= JSVAL_INT_BITS) {
        bmapint = JSVAL_TO_INT(bmapval);
        bitmap = (jsbitmap *) &bmapint;
    } else {
        bitmap = (jsbitmap *) JSVAL_TO_PRIVATE(bmapval);
    }
    return JS_TEST_BIT(bitmap, slot) != 0;
}

JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, jsval *vp)
{
    jsval val;
    JSObject *obj;
    uintN slot;

    if (TEST_OVERRIDE_BIT(fp, CALL_ARGUMENTS)) {
        JS_ASSERT(fp->callobj);
        if (!OBJ_GET_PROPERTY(cx, fp->callobj,
                              ATOM_TO_JSID(cx->runtime->atomState
                                           .argumentsAtom),
                              &val)) {
            return JS_FALSE;
        }
        if (JSVAL_IS_PRIMITIVE(val)) {
            obj = js_ValueToNonNullObject(cx, val);
            if (!obj)
                return JS_FALSE;
        } else {
            obj = JSVAL_TO_OBJECT(val);
        }
        return OBJ_GET_PROPERTY(cx, obj, id, vp);
    }

    *vp = JSVAL_VOID;
    if (JSID_IS_INT(id)) {
        slot = (uintN) JSID_TO_INT(id);
        if (slot < fp->argc) {
            if (fp->argsobj && ArgWasDeleted(cx, fp, slot))
                return OBJ_GET_PROPERTY(cx, fp->argsobj, id, vp);
            *vp = fp->argv[slot];
        } else {
            











            if (fp->argsobj)
                return OBJ_GET_PROPERTY(cx, fp->argsobj, id, vp);
        }
    } else {
        if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
            if (fp->argsobj && TEST_OVERRIDE_BIT(fp, ARGS_LENGTH))
                return OBJ_GET_PROPERTY(cx, fp->argsobj, id, vp);
            *vp = INT_TO_JSVAL((jsint) fp->argc);
        }
    }
    return JS_TRUE;
}

JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *argsobj, *global, *parent;

    



    JS_ASSERT(fp->fun && (!(fp->fun->flags & JSFUN_HEAVYWEIGHT) || fp->varobj));

    
    while (fp->flags & JSFRAME_SPECIAL)
        fp = fp->down;

    
    argsobj = fp->argsobj;
    if (argsobj)
        return argsobj;

    
    argsobj = js_NewObject(cx, &js_ArgumentsClass, NULL, NULL, 0);
    if (!argsobj || !JS_SetPrivate(cx, argsobj, fp)) {
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;
        return NULL;
    }

    










    global = fp->scopeChain;
    while ((parent = OBJ_GET_PARENT(cx, global)) != NULL)
        global = parent;
    STOBJ_SET_PARENT(argsobj, global);
    fp->argsobj = argsobj;
    return argsobj;
}

static JSBool
args_enumerate(JSContext *cx, JSObject *obj);

JS_FRIEND_API(JSBool)
js_PutArgsObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *argsobj;
    jsval bmapval, rval;
    JSBool ok;
    JSRuntime *rt;

    




    argsobj = fp->argsobj;
    ok = args_enumerate(cx, argsobj);

    



    (void) JS_GetReservedSlot(cx, argsobj, 0, &bmapval);
    if (!JSVAL_IS_VOID(bmapval)) {
        JS_SetReservedSlot(cx, argsobj, 0, JSVAL_VOID);
        if (fp->argc > JSVAL_INT_BITS)
            JS_free(cx, JSVAL_TO_PRIVATE(bmapval));
    }

    



    rt = cx->runtime;
    ok &= js_GetProperty(cx, argsobj, ATOM_TO_JSID(rt->atomState.calleeAtom),
                         &rval);
    ok &= js_SetProperty(cx, argsobj, ATOM_TO_JSID(rt->atomState.calleeAtom),
                         &rval);
    ok &= js_GetProperty(cx, argsobj, ATOM_TO_JSID(rt->atomState.lengthAtom),
                         &rval);
    ok &= js_SetProperty(cx, argsobj, ATOM_TO_JSID(rt->atomState.lengthAtom),
                         &rval);

    




    ok &= JS_SetPrivate(cx, argsobj, NULL);
    fp->argsobj = NULL;
    return ok;
}

static JSBool
args_delProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint slot;
    JSStackFrame *fp;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    fp = (JSStackFrame *)
         JS_GetInstancePrivate(cx, obj, &js_ArgumentsClass, NULL);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->argsobj);

    slot = JSVAL_TO_INT(id);
    switch (slot) {
      case ARGS_CALLEE:
      case ARGS_LENGTH:
        SET_OVERRIDE_BIT(fp, slot);
        break;

      default:
        if ((uintN)slot < fp->argc && !MarkArgDeleted(cx, fp, slot))
            return JS_FALSE;
        break;
    }
    return JS_TRUE;
}

static JSBool
args_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint slot;
    JSStackFrame *fp;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    fp = (JSStackFrame *)
         JS_GetInstancePrivate(cx, obj, &js_ArgumentsClass, NULL);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->argsobj);

    slot = JSVAL_TO_INT(id);
    switch (slot) {
      case ARGS_CALLEE:
        if (!TEST_OVERRIDE_BIT(fp, slot))
            *vp = OBJECT_TO_JSVAL(fp->callee);
        break;

      case ARGS_LENGTH:
        if (!TEST_OVERRIDE_BIT(fp, slot))
            *vp = INT_TO_JSVAL((jsint)fp->argc);
        break;

      default:
        if ((uintN)slot < fp->argc && !ArgWasDeleted(cx, fp, slot))
            *vp = fp->argv[slot];
        break;
    }
    return JS_TRUE;
}

static JSBool
args_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSStackFrame *fp;
    jsint slot;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    fp = (JSStackFrame *)
         JS_GetInstancePrivate(cx, obj, &js_ArgumentsClass, NULL);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->argsobj);

    slot = JSVAL_TO_INT(id);
    switch (slot) {
      case ARGS_CALLEE:
      case ARGS_LENGTH:
        SET_OVERRIDE_BIT(fp, slot);
        break;

      default:
        if (FUN_INTERPRETED(fp->fun) &&
            (uintN)slot < fp->argc &&
            !ArgWasDeleted(cx, fp, slot)) {
            fp->argv[slot] = *vp;
        }
        break;
    }
    return JS_TRUE;
}

static JSBool
args_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
             JSObject **objp)
{
    JSStackFrame *fp;
    uintN slot;
    JSString *str;
    JSAtom *atom;
    intN tinyid;
    jsval value;

    *objp = NULL;
    fp = (JSStackFrame *)
         JS_GetInstancePrivate(cx, obj, &js_ArgumentsClass, NULL);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->argsobj);

    if (JSVAL_IS_INT(id)) {
        slot = JSVAL_TO_INT(id);
        if (slot < fp->argc && !ArgWasDeleted(cx, fp, slot)) {
            
            if (!js_DefineProperty(cx, obj, INT_JSVAL_TO_JSID(id),
                                   fp->argv[slot],
                                   args_getProperty, args_setProperty,
                                   0, NULL)) {
                return JS_FALSE;
            }
            *objp = obj;
        }
    } else {
        str = JSVAL_TO_STRING(id);
        atom = cx->runtime->atomState.lengthAtom;
        if (str == ATOM_TO_STRING(atom)) {
            tinyid = ARGS_LENGTH;
            value = INT_TO_JSVAL(fp->argc);
        } else {
            atom = cx->runtime->atomState.calleeAtom;
            if (str == ATOM_TO_STRING(atom)) {
                tinyid = ARGS_CALLEE;
                value = OBJECT_TO_JSVAL(fp->callee);
            } else {
                atom = NULL;

                
                tinyid = 0;
                value = JSVAL_NULL;
            }
        }

        if (atom && !TEST_OVERRIDE_BIT(fp, tinyid)) {
            if (!js_DefineNativeProperty(cx, obj, ATOM_TO_JSID(atom), value,
                                         args_getProperty, args_setProperty, 0,
                                         SPROP_HAS_SHORTID, tinyid, NULL)) {
                return JS_FALSE;
            }
            *objp = obj;
        }
    }

    return JS_TRUE;
}

static JSBool
args_enumerate(JSContext *cx, JSObject *obj)
{
    JSStackFrame *fp;
    JSObject *pobj;
    JSProperty *prop;
    uintN slot, argc;

    fp = (JSStackFrame *)
         JS_GetInstancePrivate(cx, obj, &js_ArgumentsClass, NULL);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->argsobj);

    






    if (!js_LookupProperty(cx, obj,
                           ATOM_TO_JSID(cx->runtime->atomState.lengthAtom),
                           &pobj, &prop)) {
        return JS_FALSE;
    }
    if (prop)
        OBJ_DROP_PROPERTY(cx, pobj, prop);

    if (!js_LookupProperty(cx, obj,
                           ATOM_TO_JSID(cx->runtime->atomState.calleeAtom),
                           &pobj, &prop)) {
        return JS_FALSE;
    }
    if (prop)
        OBJ_DROP_PROPERTY(cx, pobj, prop);

    argc = fp->argc;
    for (slot = 0; slot < argc; slot++) {
        if (!js_LookupProperty(cx, obj, INT_TO_JSID((jsint)slot), &pobj, &prop))
            return JS_FALSE;
        if (prop)
            OBJ_DROP_PROPERTY(cx, pobj, prop);
    }
    return JS_TRUE;
}

#if JS_HAS_GENERATORS




static void
args_or_call_trace(JSTracer *trc, JSObject *obj)
{
    JSStackFrame *fp;

    fp = (JSStackFrame *) JS_GetPrivate(trc->context, obj);
    if (fp && (fp->flags & JSFRAME_GENERATOR)) {
        JS_CALL_OBJECT_TRACER(trc, FRAME_TO_GENERATOR(fp)->obj,
                              "FRAME_TO_GENERATOR(fp)->obj");
    }
}
#else
# define args_or_call_trace NULL
#endif












JSClass js_ArgumentsClass = {
    js_Object_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,    args_delProperty,
    args_getProperty,   args_setProperty,
    args_enumerate,     (JSResolveOp) args_resolve,
    JS_ConvertStub,     JS_FinalizeStub,
    NULL,               NULL,
    NULL,               NULL,
    NULL,               NULL,
    JS_CLASS_TRACE(args_or_call_trace), NULL
};

JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp, JSObject *parent)
{
    JSObject *callobj, *funobj;

    
    JS_ASSERT(fp->fun);
    callobj = fp->callobj;
    if (callobj)
        return callobj;

    
    if (!parent) {
        funobj = fp->callee;
        if (funobj)
            parent = OBJ_GET_PARENT(cx, funobj);
    }

    
    callobj = js_NewObject(cx, &js_CallClass, NULL, parent, 0);
    if (!callobj || !JS_SetPrivate(cx, callobj, fp)) {
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;
        return NULL;
    }
    fp->callobj = callobj;

    
    JS_ASSERT(fp->scopeChain == parent);
    fp->scopeChain = callobj;
    fp->varobj = callobj;
    return callobj;
}

static JSBool
call_enumerate(JSContext *cx, JSObject *obj);

JS_FRIEND_API(JSBool)
js_PutCallObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *callobj;
    JSBool ok;
    jsid argsid;
    jsval aval;

    



    callobj = fp->callobj;
    if (!callobj)
        return JS_TRUE;
    ok = call_enumerate(cx, callobj);

    


    if (fp->argsobj) {
        if (!TEST_OVERRIDE_BIT(fp, CALL_ARGUMENTS)) {
            argsid = ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom);
            aval = OBJECT_TO_JSVAL(fp->argsobj);
            ok &= js_SetProperty(cx, callobj, argsid, &aval);
        }
        ok &= js_PutArgsObject(cx, fp);
    }

    




    ok &= JS_SetPrivate(cx, callobj, NULL);
    fp->callobj = NULL;
    return ok;
}

static JSBool
call_enumerate(JSContext *cx, JSObject *obj)
{
    JSStackFrame *fp;
    JSFunction *fun;
    uintN n, i, slot;
    void *mark;
    jsuword *names;
    JSBool ok;
    JSAtom *name;
    JSObject *pobj;
    JSProperty *prop;
    jsval v;

    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(GET_FUNCTION_PRIVATE(cx, fp->callee) == fp->fun);

    




    fun = fp->fun;
    n = JS_GET_LOCAL_NAME_COUNT(fun);
    if (n == 0)
        return JS_TRUE;

    mark = JS_ARENA_MARK(&cx->tempPool);

    
    names = js_GetLocalNameArray(cx, fun, &cx->tempPool);
    if (!names) {
        ok = JS_FALSE;
        goto out;
    }

    for (i = 0; i != n; ++i) {
        name = JS_LOCAL_NAME_TO_ATOM(names[i]);
        if (!name)
            continue;

        



        ok = js_LookupProperty(cx, obj, ATOM_TO_JSID(name), &pobj, &prop);
        if (!ok)
            goto out;

        




        JS_ASSERT(prop && pobj == obj);
        slot = ((JSScopeProperty *) prop)->slot;
        OBJ_DROP_PROPERTY(cx, pobj, prop);

        v = (i < fun->nargs) ? fp->argv[i] : fp->vars[i - fun->nargs];
        LOCKED_OBJ_SET_SLOT(obj, slot, v);
    }
    ok = JS_TRUE;

  out:
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;
}

typedef enum JSCallPropertyKind {
    JSCPK_ARGUMENTS,
    JSCPK_ARG,
    JSCPK_VAR
} JSCallPropertyKind;

static JSBool
CallPropertyOp(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
               JSCallPropertyKind kind, JSBool setter)
{
    JSStackFrame *fp;
    JSFunction *fun;
    uintN i;
    jsval *array;

    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;
    fun = fp->fun;
    JS_ASSERT(fun && FUN_INTERPRETED(fun));

    if (kind == JSCPK_ARGUMENTS) {
        if (setter) {
            SET_OVERRIDE_BIT(fp, CALL_ARGUMENTS);
        } else if (!TEST_OVERRIDE_BIT(fp, CALL_ARGUMENTS)) {
            JSObject *argsobj;

            argsobj = js_GetArgsObject(cx, fp);
            if (!argsobj)
                return JS_FALSE;
            *vp = OBJECT_TO_JSVAL(argsobj);
        }
        return JS_TRUE;
    }

    JS_ASSERT((int16) JSVAL_TO_INT(id) == JSVAL_TO_INT(id));
    i = (uint16) JSVAL_TO_INT(id);
    JS_ASSERT_IF(kind == JSCPK_ARG, i < fun->nargs);
    JS_ASSERT_IF(kind == JSCPK_VAR, i < fun->u.i.nvars);

    JS_ASSERT(fun->u.i.nvars == fp->nvars);
    if (kind == JSCPK_ARG) {
        array = fp->argv;
    } else {
        JS_ASSERT(kind == JSCPK_VAR);
        array = fp->vars;
    }
    if (setter)
        array[i] = *vp;
    else
        *vp = array[i];
    return JS_TRUE;
}

static JSBool
GetCallArguments(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARGUMENTS, JS_FALSE);
}

static JSBool
SetCallArguments(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARGUMENTS, JS_TRUE);
}

JSBool
js_GetCallArg(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARG, JS_FALSE);
}

static JSBool
SetCallArg(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARG, JS_TRUE);
}

JSBool
js_GetCallVar(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_VAR, JS_FALSE);
}

static JSBool
SetCallVar(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_VAR, JS_TRUE);
}

static JSBool
call_resolve(JSContext *cx, JSObject *obj, jsval idval, uintN flags,
             JSObject **objp)
{
    JSStackFrame *fp;
    JSFunction *fun;
    jsid id;
    JSLocalKind localKind;
    JSPropertyOp getter, setter;
    uintN slot, attrs;
    jsval *vp;

    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;
    fun = fp->fun;
    JS_ASSERT(fun);
    JS_ASSERT(GET_FUNCTION_PRIVATE(cx, fp->callee) == fun);

    if (!JSVAL_IS_STRING(idval))
        return JS_TRUE;

    if (!js_ValueToStringId(cx, idval, &id))
        return JS_FALSE;

    localKind = js_LookupLocal(cx, fun, JSID_TO_ATOM(id), &slot);
    if (localKind != JSLOCAL_NONE) {
        JS_ASSERT((uint16) slot == slot);
        if (localKind == JSLOCAL_ARG) {
            JS_ASSERT(slot < fun->nargs);
            vp = fp->argv;
            getter = js_GetCallArg;
            setter = SetCallArg;
            attrs = JSPROP_PERMANENT;
        } else {
            JS_ASSERT(localKind == JSLOCAL_VAR || localKind == JSLOCAL_CONST);
            JS_ASSERT(fun->u.i.nvars == fp->nvars);
            JS_ASSERT(slot < fun->u.i.nvars);
            vp = fp->vars;
            getter = js_GetCallVar;
            setter = SetCallVar;
            attrs = (localKind == JSLOCAL_CONST)
                    ? JSPROP_PERMANENT | JSPROP_READONLY
                    : JSPROP_PERMANENT;
        }
        if (!js_DefineNativeProperty(cx, obj, id, vp[slot], getter, setter,
                                     attrs, SPROP_HAS_SHORTID, (int16) slot,
                                     NULL)) {
            return JS_FALSE;
        }
        *objp = obj;
        return JS_TRUE;
    }

    



    if (id == ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom)) {
        if (!js_DefineNativeProperty(cx, obj, id, JSVAL_VOID,
                                     GetCallArguments, SetCallArguments,
                                     JSPROP_PERMANENT, 0, 0, NULL)) {
            return JS_FALSE;
        }
        *objp = obj;
        return JS_TRUE;
    }
    return JS_TRUE;
}

static JSBool
call_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    JSStackFrame *fp;

    if (type == JSTYPE_FUNCTION) {
        fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
        if (fp) {
            JS_ASSERT(fp->fun);
            *vp = OBJECT_TO_JSVAL(fp->callee);
        }
    }
    return JS_TRUE;
}

JS_FRIEND_DATA(JSClass) js_CallClass = {
    js_Call_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_IS_ANONYMOUS |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Call),
    JS_PropertyStub,    JS_PropertyStub,
    JS_PropertyStub,    JS_PropertyStub,
    call_enumerate,     (JSResolveOp)call_resolve,
    call_convert,       JS_FinalizeStub,
    NULL,               NULL,
    NULL,               NULL,
    NULL,               NULL,
    JS_CLASS_TRACE(args_or_call_trace), NULL,
};















#define LENGTH_PROP_ATTRS (JSPROP_READONLY|JSPROP_PERMANENT|JSPROP_SHARED)

static JSPropertySpec function_props[] = {
    {js_length_str,    ARGS_LENGTH,    LENGTH_PROP_ATTRS, 0,0},
    {0,0,0,0,0}
};

typedef struct LazyFunctionProp {
    uint16      atomOffset;
    int8        tinyid;
    uint8       attrs;
} LazyFunctionProp;


static LazyFunctionProp lazy_function_props[] = {
    {ATOM_OFFSET(arguments), CALL_ARGUMENTS, JSPROP_PERMANENT},
    {ATOM_OFFSET(arity),     FUN_ARITY,      JSPROP_PERMANENT},
    {ATOM_OFFSET(caller),    FUN_CALLER,     JSPROP_PERMANENT},
    {ATOM_OFFSET(name),      FUN_NAME,       JSPROP_PERMANENT},
};

static JSBool
fun_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint slot;
    JSFunction *fun;
    JSStackFrame *fp;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    slot = JSVAL_TO_INT(id);

    



















    while (!(fun = (JSFunction *)
                   JS_GetInstancePrivate(cx, obj, &js_FunctionClass, NULL))) {
        if (slot != ARGS_LENGTH)
            return JS_TRUE;
        obj = OBJ_GET_PROTO(cx, obj);
        if (!obj)
            return JS_TRUE;
    }

    
    for (fp = cx->fp; fp && (fp->fun != fun || (fp->flags & JSFRAME_SPECIAL));
         fp = fp->down) {
        continue;
    }

    switch (slot) {
      case CALL_ARGUMENTS:
        
        if (!JS_ReportErrorFlagsAndNumber(cx,
                                          JSREPORT_WARNING | JSREPORT_STRICT,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_DEPRECATED_USAGE,
                                          js_arguments_str)) {
            return JS_FALSE;
        }
        if (fp) {
            if (!js_GetArgsValue(cx, fp, vp))
                return JS_FALSE;
        } else {
            *vp = JSVAL_NULL;
        }
        break;

      case ARGS_LENGTH:
      case FUN_ARITY:
            *vp = INT_TO_JSVAL((jsint)fun->nargs);
        break;

      case FUN_NAME:
        *vp = fun->atom
              ? ATOM_KEY(fun->atom)
              : STRING_TO_JSVAL(cx->runtime->emptyString);
        break;

      case FUN_CALLER:
        if (fp && fp->down && fp->down->fun)
            *vp = OBJECT_TO_JSVAL(fp->down->callee);
        else
            *vp = JSVAL_NULL;
        if (!JSVAL_IS_PRIMITIVE(*vp) && cx->runtime->checkObjectAccess) {
            id = ATOM_KEY(cx->runtime->atomState.callerAtom);
            if (!cx->runtime->checkObjectAccess(cx, obj, id, JSACC_READ, vp))
                return JS_FALSE;
        }
        break;

      default:
        
        if (fp && fp->fun && (uintN)slot < fp->fun->nargs)
            *vp = fp->argv[slot];
        break;
    }

    return JS_TRUE;
}

static JSBool
fun_enumerate(JSContext *cx, JSObject *obj)
{
    jsid prototypeId;
    JSObject *pobj;
    JSProperty *prop;

    prototypeId = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    if (!OBJ_LOOKUP_PROPERTY(cx, obj, prototypeId, &pobj, &prop))
        return JS_FALSE;
    if (prop)
        OBJ_DROP_PROPERTY(cx, pobj, prop);
    return JS_TRUE;
}

static JSBool
fun_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
            JSObject **objp)
{
    JSFunction *fun;
    JSAtom *atom;
    uintN i;

    if (!JSVAL_IS_STRING(id))
        return JS_TRUE;

    fun = GET_FUNCTION_PRIVATE(cx, obj);

    











    if (flags & JSRESOLVE_ASSIGNING)
        return JS_TRUE;

    



    atom = cx->runtime->atomState.classPrototypeAtom;
    if (id == ATOM_KEY(atom)) {
        JSObject *proto;

        



        if (fun->atom == CLASS_ATOM(cx, Object))
            return JS_TRUE;

        



        proto = js_NewObject(cx, &js_ObjectClass, NULL, OBJ_GET_PARENT(cx, obj),
                             0);
        if (!proto)
            return JS_FALSE;

        






        if (!js_SetClassPrototype(cx, obj, proto,
                                  JSPROP_ENUMERATE | JSPROP_PERMANENT)) {
            cx->weakRoots.newborn[GCX_OBJECT] = NULL;
            return JS_FALSE;
        }
        *objp = obj;
        return JS_TRUE;
    }

    for (i = 0; i < JS_ARRAY_LENGTH(lazy_function_props); i++) {
        LazyFunctionProp *lfp = &lazy_function_props[i];

        atom = OFFSET_TO_ATOM(cx->runtime, lfp->atomOffset);
        if (id == ATOM_KEY(atom)) {
            if (!js_DefineNativeProperty(cx, obj,
                                         ATOM_TO_JSID(atom), JSVAL_VOID,
                                         NULL, NULL, lfp->attrs,
                                         SPROP_HAS_SHORTID, lfp->tinyid,
                                         NULL)) {
                return JS_FALSE;
            }
            *objp = obj;
            return JS_TRUE;
        }
    }

    return JS_TRUE;
}

static JSBool
fun_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    switch (type) {
      case JSTYPE_FUNCTION:
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
      default:
        return js_TryValueOf(cx, obj, type, vp);
    }
}

#if JS_HAS_XDR


static JSBool
fun_xdrObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    JSFunction *fun;
    uint32 nullAtom;            
    uintN nargs, nvars, n;
    uint32 localsword;          
    uint32 flagsword;           
    JSTempValueRooter tvr;
    JSBool ok;

    cx = xdr->cx;
    if (xdr->mode == JSXDR_ENCODE) {
        fun = GET_FUNCTION_PRIVATE(cx, *objp);
        if (!FUN_INTERPRETED(fun)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NOT_SCRIPTED_FUNCTION,
                                 JS_GetFunctionName(fun));
            return JS_FALSE;
        }
        nullAtom = !fun->atom;
        nargs = fun->nargs;
        nvars = fun->u.i.nvars;
        localsword = (nargs << 16) | nvars;
        flagsword = fun->flags;
    } else {
        fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_INTERPRETED, NULL, NULL);
        if (!fun)
            return JS_FALSE;
        STOBJ_SET_PARENT(FUN_OBJECT(fun), NULL);
        STOBJ_SET_PROTO(FUN_OBJECT(fun), NULL);
#ifdef __GNUC__
        nvars = nargs = 0;   
#endif
    }

    
    JS_PUSH_TEMP_ROOT_OBJECT(cx, FUN_OBJECT(fun), &tvr);
    ok = JS_TRUE;

    if (!JS_XDRUint32(xdr, &nullAtom))
        goto bad;
    if (!nullAtom && !js_XDRStringAtom(xdr, &fun->atom))
        goto bad;
    if (!JS_XDRUint32(xdr, &localsword) ||
        !JS_XDRUint32(xdr, &flagsword)) {
        goto bad;
    }

    if (xdr->mode == JSXDR_DECODE) {
        nargs = localsword >> 16;
        nvars = localsword & JS_BITMASK(16);
        JS_ASSERT(flagsword | JSFUN_INTERPRETED);
        fun->flags = (uint16) flagsword;
    }

    
    n = nargs + nvars;
    if (n != 0) {
        void *mark;
        uintN i;
        uintN bitmapLength;
        uint32 *bitmap;
        jsuword *names;
        JSAtom *name;
        JSLocalKind localKind;

        mark = JS_ARENA_MARK(&xdr->cx->tempPool);

        









        bitmapLength = JS_HOWMANY(n, JS_BITS_PER_UINT32);
        JS_ARENA_ALLOCATE_CAST(bitmap, uint32 *, &xdr->cx->tempPool,
                               bitmapLength * sizeof *bitmap);
        if (!bitmap) {
            js_ReportOutOfScriptQuota(xdr->cx);
            ok = JS_FALSE;
            goto release_mark;
        }
        if (xdr->mode == JSXDR_ENCODE) {
            names = js_GetLocalNameArray(xdr->cx, fun, &xdr->cx->tempPool);
            if (!names) {
                ok = JS_FALSE;
                goto release_mark;
            }
            memset(bitmap, 0, bitmapLength * sizeof *bitmap);
            for (i = 0; i != n; ++i) {
                if (i < fun->nargs
                    ? JS_LOCAL_NAME_TO_ATOM(names[i]) != NULL
                    : JS_LOCAL_NAME_IS_CONST(names[i])) {
                    bitmap[i >> JS_BITS_PER_UINT32_LOG2] |=
                        JS_BIT(i & (JS_BITS_PER_UINT32 - 1));
                }
            }
        }
#ifdef __GNUC__
        else {
            names = NULL;   
        }
#endif
        for (i = 0; i != bitmapLength; ++i) {
            ok = JS_XDRUint32(xdr, &bitmap[i]);
            if (!ok)
                goto release_mark;
        }
        for (i = 0; i != n; ++i) {
            if (i < nargs &&
                !(bitmap[i >> JS_BITS_PER_UINT32_LOG2] &
                  JS_BIT(i & (JS_BITS_PER_UINT32 - 1)))) {
                if (xdr->mode == JSXDR_DECODE) {
                    ok = js_AddLocal(xdr->cx, fun, NULL, JSLOCAL_ARG);
                    if (!ok)
                        goto release_mark;
                } else {
                    JS_ASSERT(!JS_LOCAL_NAME_TO_ATOM(names[i]));
                }
                continue;
            }
            if (xdr->mode == JSXDR_ENCODE)
                name = JS_LOCAL_NAME_TO_ATOM(names[i]);
            ok = js_XDRStringAtom(xdr, &name);
            if (!ok)
                goto release_mark;
            if (xdr->mode == JSXDR_DECODE) {
                localKind = (i < nargs)
                            ? JSLOCAL_ARG
                            : bitmap[i >> JS_BITS_PER_UINT32_LOG2] &
                              JS_BIT(i & (JS_BITS_PER_UINT32 - 1))
                            ? JSLOCAL_CONST
                            : JSLOCAL_VAR;
                ok = js_AddLocal(xdr->cx, fun, name, localKind);
                if (!ok)
                    goto release_mark;
            }
        }
        ok = JS_TRUE;

      release_mark:
        JS_ARENA_RELEASE(&xdr->cx->tempPool, mark);
        if (!ok)
            goto out;

        if (xdr->mode == JSXDR_DECODE)
            js_FreezeLocalNames(cx, fun);
    }

    if (!js_XDRScript(xdr, &fun->u.i.script, NULL))
        goto bad;

    if (xdr->mode == JSXDR_DECODE) {
        *objp = FUN_OBJECT(fun);
#ifdef CHECK_SCRIPT_OWNER
        fun->u.i.script->owner = NULL;
#endif
        js_CallNewScriptHook(cx, fun->u.i.script, fun);
    }

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;

bad:
    ok = JS_FALSE;
    goto out;
}

#else  

#define fun_xdrObject NULL

#endif 






static JSBool
fun_hasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    jsval pval;

    if (!OBJ_GET_PROPERTY(cx, obj,
                          ATOM_TO_JSID(cx->runtime->atomState
                                       .classPrototypeAtom),
                          &pval)) {
        return JS_FALSE;
    }

    if (JSVAL_IS_PRIMITIVE(pval)) {
        



        js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE,
                            -1, OBJECT_TO_JSVAL(obj), NULL);
        return JS_FALSE;
    }

    return js_IsDelegate(cx, JSVAL_TO_OBJECT(pval), v, bp);
}

static void
TraceLocalNames(JSTracer *trc, JSFunction *fun);

static void
DestroyLocalNames(JSContext *cx, JSFunction *fun);

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    JSFunction *fun;

    
    fun = (JSFunction *) JS_GetPrivate(trc->context, obj);
    if (!fun)
        return;

    if (FUN_OBJECT(fun) != obj) {
        
        JS_CALL_TRACER(trc, FUN_OBJECT(fun), JSTRACE_OBJECT, "private");
        return;
    }
    if (fun->atom)
        JS_CALL_STRING_TRACER(trc, ATOM_TO_STRING(fun->atom), "atom");
    if (FUN_INTERPRETED(fun)) {
        if (fun->u.i.script)
            js_TraceScript(trc, fun->u.i.script);
        TraceLocalNames(trc, fun);
    }
}

static void
fun_finalize(JSContext *cx, JSObject *obj)
{
    JSFunction *fun;

    
    fun = (JSFunction *) JS_GetPrivate(cx, obj);
    if (!fun || FUN_OBJECT(fun) != obj)
        return;

    



    if (FUN_INTERPRETED(fun)) {
        if (fun->u.i.script)
            js_DestroyScript(cx, fun->u.i.script);
        DestroyLocalNames(cx, fun);
    }
}

static uint32
fun_reserveSlots(JSContext *cx, JSObject *obj)
{
    JSFunction *fun;

    




    fun = (JSFunction *) JS_GetPrivate(cx, obj);
    return (fun && FUN_INTERPRETED(fun) &&
            fun->u.i.script && fun->u.i.script->regexpsOffset != 0)
           ? JS_SCRIPT_REGEXPS(fun->u.i.script)->length
           : 0;
}






JS_FRIEND_DATA(JSClass) js_FunctionClass = {
    js_Function_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(2) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    JS_PropertyStub,  JS_PropertyStub,
    fun_getProperty,  JS_PropertyStub,
    fun_enumerate,    (JSResolveOp)fun_resolve,
    fun_convert,      fun_finalize,
    NULL,             NULL,
    NULL,             NULL,
    fun_xdrObject,    fun_hasInstance,
    JS_CLASS_TRACE(fun_trace), fun_reserveSlots
};

static JSBool
fun_toStringHelper(JSContext *cx, uint32 indent, uintN argc, jsval *vp)
{
    jsval fval;
    JSObject *obj;
    JSFunction *fun;
    JSString *str;

    fval = JS_THIS(cx, vp);
    if (JSVAL_IS_NULL(fval))
        return JS_FALSE;

    if (!VALUE_IS_FUNCTION(cx, fval)) {
        



        if (!JSVAL_IS_PRIMITIVE(fval)) {
            obj = JSVAL_TO_OBJECT(fval);
            if (!OBJ_GET_CLASS(cx, obj)->convert(cx, obj, JSTYPE_FUNCTION,
                                                 &fval)) {
                return JS_FALSE;
            }
            vp[1] = fval;
        }
        if (!VALUE_IS_FUNCTION(cx, fval)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_INCOMPATIBLE_PROTO,
                                 js_Function_str, js_toString_str,
                                 JS_GetTypeName(cx, JS_TypeOfValue(cx, fval)));
            return JS_FALSE;
        }
    }

    obj = JSVAL_TO_OBJECT(fval);
    if (argc != 0) {
        indent = js_ValueToECMAUint32(cx, &vp[2]);
        if (JSVAL_IS_NULL(vp[2]))
            return JS_FALSE;
    }

    JS_ASSERT(JS_ObjectIsFunction(cx, obj));
    fun = GET_FUNCTION_PRIVATE(cx, obj);
    if (!fun)
        return JS_TRUE;
    str = JS_DecompileFunction(cx, fun, (uintN)indent);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
fun_toString(JSContext *cx, uintN argc, jsval *vp)
{
    return fun_toStringHelper(cx, 0, argc,  vp);
}

#if JS_HAS_TOSOURCE
static JSBool
fun_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    return fun_toStringHelper(cx, JS_DONT_PRETTY_PRINT, argc, vp);
}
#endif

static const char call_str[] = "call";

static JSBool
fun_call(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval fval, *argv, *invokevp;
    JSString *str;
    void *mark;
    JSBool ok;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_FUNCTION, &vp[1]))
        return JS_FALSE;
    fval = vp[1];

    if (!VALUE_IS_FUNCTION(cx, fval)) {
        str = JS_ValueToString(cx, fval);
        if (str) {
            const char *bytes = js_GetStringBytes(cx, str);

            if (bytes) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INCOMPATIBLE_PROTO,
                                     js_Function_str, call_str,
                                     bytes);
            }
        }
        return JS_FALSE;
    }

    argv = vp + 2;
    if (argc == 0) {
        
        obj = NULL;
    } else {
        
        if (!JSVAL_IS_PRIMITIVE(argv[0]))
            obj = JSVAL_TO_OBJECT(argv[0]);
        else if (!js_ValueToObject(cx, argv[0], &obj))
            return JS_FALSE;
        argc--;
        argv++;
    }

    
    invokevp = js_AllocStack(cx, 2 + argc, &mark);
    if (!invokevp)
        return JS_FALSE;

    
    invokevp[0] = fval;
    invokevp[1] = OBJECT_TO_JSVAL(obj);
    memcpy(invokevp + 2, argv, argc * sizeof *argv);

    ok = js_Invoke(cx, argc, invokevp, JSINVOKE_INTERNAL);
    *vp = *invokevp;
    js_FreeStack(cx, mark);
    return ok;
}

static JSBool
fun_apply(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj, *aobj;
    jsval fval, *invokevp, *sp;
    JSString *str;
    jsuint length;
    JSBool arraylike, ok;
    void *mark;
    uintN i;

    if (argc == 0) {
        
        return fun_call(cx, argc, vp);
    }

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_FUNCTION, &vp[1]))
        return JS_FALSE;
    fval = vp[1];

    if (!VALUE_IS_FUNCTION(cx, fval)) {
        str = JS_ValueToString(cx, fval);
        if (str) {
            const char *bytes = js_GetStringBytes(cx, str);

            if (bytes) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INCOMPATIBLE_PROTO,
                                     js_Function_str, "apply",
                                     bytes);
            }
        }
        return JS_FALSE;
    }

    
    aobj = NULL;
    length = 0;

    if (argc >= 2) {
        
        if (JSVAL_IS_NULL(vp[3]) || JSVAL_IS_VOID(vp[3])) {
            argc = 0;
        } else {
            
            arraylike = JS_FALSE;
            if (!JSVAL_IS_PRIMITIVE(vp[3])) {
                aobj = JSVAL_TO_OBJECT(vp[3]);
                if (!js_IsArrayLike(cx, aobj, &arraylike, &length))
                    return JS_FALSE;
            }
            if (!arraylike) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_APPLY_ARGS, "apply");
                return JS_FALSE;
            }
        }
    }

    
    if (!JSVAL_IS_PRIMITIVE(vp[2]))
        obj = JSVAL_TO_OBJECT(vp[2]);
    else if (!js_ValueToObject(cx, vp[2], &obj))
        return JS_FALSE;

    
    argc = (uintN)JS_MIN(length, ARRAY_INIT_LIMIT - 1);
    invokevp = js_AllocStack(cx, 2 + argc, &mark);
    if (!invokevp)
        return JS_FALSE;

    
    sp = invokevp;
    *sp++ = fval;
    *sp++ = OBJECT_TO_JSVAL(obj);
    for (i = 0; i < argc; i++) {
        ok = JS_GetElement(cx, aobj, (jsint)i, sp);
        if (!ok)
            goto out;
        sp++;
    }

    ok = js_Invoke(cx, argc, invokevp, JSINVOKE_INTERNAL);
    *vp = *invokevp;
out:
    js_FreeStack(cx, mark);
    return ok;
}

#ifdef NARCISSUS
static JSBool
fun_applyConstructor(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *aobj;
    uintN length, i;
    void *mark;
    jsval *invokevp, *sp;
    JSBool ok;

    if (JSVAL_IS_PRIMITIVE(vp[2]) ||
        (aobj = JSVAL_TO_OBJECT(vp[2]),
         OBJ_GET_CLASS(cx, aobj) != &js_ArrayClass &&
         OBJ_GET_CLASS(cx, aobj) != &js_ArgumentsClass)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_APPLY_ARGS, "__applyConstruct__");
        return JS_FALSE;
    }

    if (!js_GetLengthProperty(cx, aobj, &length))
        return JS_FALSE;

    if (length >= ARRAY_INIT_LIMIT)
        length = ARRAY_INIT_LIMIT - 1;
    invokevp = js_AllocStack(cx, 2 + length, &mark);
    if (!invokevp)
        return JS_FALSE;

    sp = invokevp;
    *sp++ = vp[1];
    *sp++ = JSVAL_NULL; 
    for (i = 0; i < length; i++) {
        ok = JS_GetElement(cx, aobj, (jsint)i, sp);
        if (!ok)
            goto out;
        sp++;
    }

    ok = js_InvokeConstructor(cx, invokevp, length);
    *vp = *invokevp;
out:
    js_FreeStack(cx, mark);
    return ok;
}
#endif

static JSFunctionSpec function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0,0),
    JS_FN("apply",           fun_apply,      0,2,0),
    JS_FN(call_str,          fun_call,       0,1,0),
#ifdef NARCISSUS
    JS_FN("__applyConstructor__", fun_applyConstructor, 0,1,0),
#endif
    JS_FS_END
};

static JSBool
Function(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSStackFrame *fp, *caller;
    JSFunction *fun;
    JSObject *parent;
    uintN i, n, lineno;
    JSAtom *atom;
    const char *filename;
    JSBool ok;
    JSString *str, *arg;
    JSTokenStream ts;
    JSPrincipals *principals;
    jschar *collected_args, *cp;
    void *mark;
    size_t arg_length, args_length, old_args_length;
    JSTokenType tt;

    fp = cx->fp;
    if (!(fp->flags & JSFRAME_CONSTRUCTING)) {
        obj = js_NewObject(cx, &js_FunctionClass, NULL, NULL, 0);
        if (!obj)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(obj);
    } else {
        



        if (JS_GetPrivate(cx, obj))
            return JS_TRUE;
    }

    









    parent = OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(argv[-2]));

    fun = js_NewFunction(cx, obj, NULL, 0, JSFUN_LAMBDA | JSFUN_INTERPRETED,
                         parent, cx->runtime->atomState.anonymousAtom);

    if (!fun)
        return JS_FALSE;

    






    JS_ASSERT(!fp->script && fp->fun && fp->fun->u.n.native == Function);
    caller = JS_GetScriptedCaller(cx, fp);
    if (caller) {
        principals = JS_EvalFramePrincipals(cx, fp, caller);
        filename = js_ComputeFilename(cx, caller, principals, &lineno);
    } else {
        filename = NULL;
        lineno = 0;
        principals = NULL;
    }

    
    if (!js_CheckPrincipalsAccess(cx, parent, principals,
                                  CLASS_ATOM(cx, Function))) {
        return JS_FALSE;
    }

    n = argc ? argc - 1 : 0;
    if (n > 0) {
        enum { OK, BAD, BAD_FORMAL } state;

        









        state = BAD_FORMAL;
        args_length = 0;
        for (i = 0; i < n; i++) {
            
            arg = js_ValueToString(cx, argv[i]);
            if (!arg)
                return JS_FALSE;
            argv[i] = STRING_TO_JSVAL(arg);

            



            old_args_length = args_length;
            args_length = old_args_length + JSSTRING_LENGTH(arg);
            if (args_length < old_args_length) {
                js_ReportAllocationOverflow(cx);
                return JS_FALSE;
            }
        }

        
        old_args_length = args_length;
        args_length = old_args_length + n - 1;
        if (args_length < old_args_length ||
            args_length >= ~(size_t)0 / sizeof(jschar)) {
            js_ReportAllocationOverflow(cx);
            return JS_FALSE;
        }

        




        mark = JS_ARENA_MARK(&cx->tempPool);
        JS_ARENA_ALLOCATE_CAST(cp, jschar *, &cx->tempPool,
                               (args_length+1) * sizeof(jschar));
        if (!cp) {
            js_ReportOutOfScriptQuota(cx);
            return JS_FALSE;
        }
        collected_args = cp;

        


        for (i = 0; i < n; i++) {
            arg = JSVAL_TO_STRING(argv[i]);
            arg_length = JSSTRING_LENGTH(arg);
            (void) js_strncpy(cp, JSSTRING_CHARS(arg), arg_length);
            cp += arg_length;

            
            *cp++ = (i + 1 < n) ? ',' : 0;
        }

        
        if (!js_InitTokenStream(cx, &ts, collected_args, args_length,
                                NULL, filename, lineno)) {
            JS_ARENA_RELEASE(&cx->tempPool, mark);
            return JS_FALSE;
        }

        
        tt = js_GetToken(cx, &ts);
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (tt != TOK_NAME)
                    goto after_args;

                




                atom = CURRENT_TOKEN(&ts).t_atom;

                
                if (js_LookupLocal(cx, fun, atom, NULL) != JSLOCAL_NONE) {
                    const char *name;

                    name = js_AtomToPrintableString(cx, atom);
                    ok = name &&
                         js_ReportCompileErrorNumber(cx, &ts, NULL,
                                                     JSREPORT_WARNING |
                                                     JSREPORT_STRICT,
                                                     JSMSG_DUPLICATE_FORMAL,
                                                     name);
                    if (!ok)
                        goto after_args;
                }
                if (!js_AddLocal(cx, fun, atom, JSLOCAL_ARG))
                    goto after_args;

                



                tt = js_GetToken(cx, &ts);
                if (tt == TOK_EOF)
                    break;
                if (tt != TOK_COMMA)
                    goto after_args;
                tt = js_GetToken(cx, &ts);
            }
        }

        state = OK;
      after_args:
        if (state == BAD_FORMAL && !(ts.flags & TSF_ERROR)) {
            



            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_FORMAL);
        }
        js_CloseTokenStream(cx, &ts);
        JS_ARENA_RELEASE(&cx->tempPool, mark);
        if (state != OK)
            return JS_FALSE;
    }

    if (argc) {
        str = js_ValueToString(cx, argv[argc-1]);
        if (!str)
            return JS_FALSE;
        argv[argc-1] = STRING_TO_JSVAL(str);
    } else {
        str = cx->runtime->emptyString;
    }

    return js_CompileFunctionBody(cx, fun, principals,
                                  JSSTRING_CHARS(str), JSSTRING_LENGTH(str),
                                  filename, lineno);
}

JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;
    JSAtom *atom;
    JSFunction *fun;

    proto = JS_InitClass(cx, obj, NULL, &js_FunctionClass, Function, 1,
                         function_props, function_methods, NULL, NULL);
    if (!proto)
        return NULL;
    atom = js_Atomize(cx, js_FunctionClass.name, strlen(js_FunctionClass.name),
                      0);
    if (!atom)
        goto bad;
    fun = js_NewFunction(cx, proto, NULL, 0, JSFUN_INTERPRETED, obj, NULL);
    if (!fun)
        goto bad;
    fun->u.i.script = js_NewScript(cx, 1, 0, 0, 0, 0, 0);
    if (!fun->u.i.script)
        goto bad;
    fun->u.i.script->code[0] = JSOP_STOP;
#ifdef CHECK_SCRIPT_OWNER
    fun->u.i.script->owner = NULL;
#endif
    return proto;

bad:
    cx->weakRoots.newborn[GCX_OBJECT] = NULL;
    return NULL;
}

JSObject *
js_InitCallClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    proto = JS_InitClass(cx, obj, NULL, &js_CallClass, NULL, 0,
                         NULL, NULL, NULL, NULL);
    if (!proto)
        return NULL;

    



    OBJ_SET_PROTO(cx, proto, NULL);
    return proto;
}

JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, JSNative native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom)
{
    JSFunction *fun;

    if (funobj) {
        JS_ASSERT(HAS_FUNCTION_CLASS(funobj));
        OBJ_SET_PARENT(cx, funobj, parent);
    } else {
        funobj = js_NewObject(cx, &js_FunctionClass, NULL, parent, 0);
        if (!funobj)
            return NULL;
    }
    JS_ASSERT(funobj->fslots[JSSLOT_PRIVATE] == JSVAL_VOID);
    fun = (JSFunction *) funobj;

    
    fun->nargs = nargs;
    fun->flags = flags & (JSFUN_FLAGS_MASK | JSFUN_INTERPRETED);
    if (flags & JSFUN_INTERPRETED) {
        JS_ASSERT(!native);
        JS_ASSERT(nargs == 0);
        fun->u.i.nvars = 0;
        fun->u.i.spare = 0;
        fun->u.i.script = NULL;
#ifdef DEBUG
        fun->u.i.names.taggedAtom = 0;
#endif
    } else {
        fun->u.n.native = native;
        fun->u.n.extra = 0;
        fun->u.n.minargs = 0;
        fun->u.n.clasp = NULL;
    }
    fun->atom = atom;

    
    FUN_OBJECT(fun)->fslots[JSSLOT_PRIVATE] = PRIVATE_TO_JSVAL(fun);
    return fun;
}

JSObject *
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent)
{
    JSObject *clone;

    



    clone = js_NewObject(cx, &js_FunctionClass, NULL, parent,
                         sizeof(JSObject));
    if (!clone)
        return NULL;
    clone->fslots[JSSLOT_PRIVATE] = PRIVATE_TO_JSVAL(fun);
    return clone;
}

JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, JSNative native,
                  uintN nargs, uintN attrs)
{
    JSFunction *fun;
    JSPropertyOp gsop;

    fun = js_NewFunction(cx, NULL, native, nargs, attrs, obj, atom);
    if (!fun)
        return NULL;
    gsop = (attrs & JSFUN_STUB_GSOPS) ? JS_PropertyStub : NULL;
    if (!OBJ_DEFINE_PROPERTY(cx, obj, ATOM_TO_JSID(atom),
                             OBJECT_TO_JSVAL(FUN_OBJECT(fun)),
                             gsop, gsop,
                             attrs & ~JSFUN_FLAGS_MASK, NULL)) {
        return NULL;
    }
    return fun;
}

#if (JSV2F_CONSTRUCT & JSV2F_SEARCH_STACK)
# error "JSINVOKE_CONSTRUCT and JSV2F_SEARCH_STACK are not disjoint!"
#endif

JSFunction *
js_ValueToFunction(JSContext *cx, jsval *vp, uintN flags)
{
    jsval v;
    JSObject *obj;

    v = *vp;
    obj = NULL;
    if (JSVAL_IS_OBJECT(v)) {
        obj = JSVAL_TO_OBJECT(v);
        if (obj && OBJ_GET_CLASS(cx, obj) != &js_FunctionClass) {
            if (!OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_FUNCTION, &v))
                return NULL;
            obj = VALUE_IS_FUNCTION(cx, v) ? JSVAL_TO_OBJECT(v) : NULL;
        }
    }
    if (!obj) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }
    return GET_FUNCTION_PRIVATE(cx, obj);
}

JSObject *
js_ValueToFunctionObject(JSContext *cx, jsval *vp, uintN flags)
{
    JSFunction *fun;
    JSStackFrame *caller;
    JSPrincipals *principals;

    if (VALUE_IS_FUNCTION(cx, *vp))
        return JSVAL_TO_OBJECT(*vp);

    fun = js_ValueToFunction(cx, vp, flags);
    if (!fun)
        return NULL;
    *vp = OBJECT_TO_JSVAL(FUN_OBJECT(fun));

    caller = JS_GetScriptedCaller(cx, cx->fp);
    if (caller) {
        principals = JS_StackFramePrincipals(cx, caller);
    } else {
        
        principals = NULL;
    }

    if (!js_CheckPrincipalsAccess(cx, FUN_OBJECT(fun), principals,
                                  fun->atom
                                  ? fun->atom
                                  : cx->runtime->atomState.anonymousAtom)) {
        return NULL;
    }
    return FUN_OBJECT(fun);
}

JSObject *
js_ValueToCallableObject(JSContext *cx, jsval *vp, uintN flags)
{
    JSObject *callable;

    callable = JSVAL_IS_PRIMITIVE(*vp) ? NULL : JSVAL_TO_OBJECT(*vp);
    if (callable &&
        ((callable->map->ops == &js_ObjectOps)
         ? OBJ_GET_CLASS(cx, callable)->call
         : callable->map->ops->call)) {
        *vp = OBJECT_TO_JSVAL(callable);
    } else {
        callable = js_ValueToFunctionObject(cx, vp, flags);
    }
    return callable;
}

void
js_ReportIsNotFunction(JSContext *cx, jsval *vp, uintN flags)
{
    JSStackFrame *fp;
    uintN error;
    const char *name, *source;
    JSTempValueRooter tvr;

    for (fp = cx->fp; fp && !fp->regs; fp = fp->down)
        continue;
    name = source = NULL;
    JS_PUSH_TEMP_ROOT_STRING(cx, NULL, &tvr);
    if (flags & JSV2F_ITERATOR) {
        error = JSMSG_BAD_ITERATOR;
        name = js_iterator_str;
        tvr.u.string = js_ValueToSource(cx, *vp);
        if (!tvr.u.string)
            goto out;
        tvr.u.string = js_QuoteString(cx, tvr.u.string, 0);
        if (!tvr.u.string)
            goto out;
        source = js_GetStringBytes(cx, tvr.u.string);
        if (!source)
            goto out;
    } else if (flags & JSV2F_CONSTRUCT) {
        error = JSMSG_NOT_CONSTRUCTOR;
    } else {
        error = JSMSG_NOT_FUNCTION;
    }

    js_ReportValueError3(cx, error,
                         (fp && fp->regs &&
                          fp->spbase <= vp && vp < fp->regs->sp)
                         ? vp - fp->regs->sp
                         : (flags & JSV2F_SEARCH_STACK)
                         ? JSDVG_SEARCH_STACK
                         : JSDVG_IGNORE_STACK,
                         *vp, NULL,
                         name, source);

  out:
    JS_POP_TEMP_ROOT(cx, &tvr);
}





#define MAX_ARRAY_LOCALS 8

JS_STATIC_ASSERT(2 <= MAX_ARRAY_LOCALS);
JS_STATIC_ASSERT(MAX_ARRAY_LOCALS < JS_BITMASK(16));





JS_STATIC_ASSERT((JSVAL_STRING & 1) == 0);






typedef struct JSNameIndexPair JSNameIndexPair;

struct JSNameIndexPair {
    JSAtom          *name;
    uint16          index;
    JSNameIndexPair *link;
};

struct JSLocalNameMap {
    JSDHashTable    names;
    JSNameIndexPair *lastdup;
};

typedef struct JSLocalNameHashEntry {
    JSDHashEntryHdr hdr;
    JSAtom          *name;
    uint16          index;
    uint8           localKind;
} JSLocalNameHashEntry;

static void
FreeLocalNameHash(JSContext *cx, JSLocalNameMap *map)
{
    JSNameIndexPair *dup, *next;

    for (dup = map->lastdup; dup; dup = next) {
        next = dup->link;
        JS_free(cx, dup);
    }
    JS_DHashTableFinish(&map->names);
    JS_free(cx, map);
}

static JSBool
HashLocalName(JSContext *cx, JSLocalNameMap *map, JSAtom *name,
              JSLocalKind localKind, uintN index)
{
    JSLocalNameHashEntry *entry;
    JSNameIndexPair *dup;

    JS_ASSERT(index <= JS_BITMASK(16));
#if JS_HAS_DESTRUCTURING
    if (!name) {
        
        JS_ASSERT(localKind == JSLOCAL_ARG);
        return JS_TRUE;
    }
#endif
    JS_ASSERT(ATOM_IS_STRING(name));
    entry = (JSLocalNameHashEntry *)
            JS_DHashTableOperate(&map->names, name, JS_DHASH_ADD);
    if (!entry) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    if (entry->name) {
        JS_ASSERT(entry->name == name);
        JS_ASSERT(entry->localKind == JSLOCAL_ARG);
        dup = (JSNameIndexPair *) JS_malloc(cx, sizeof *dup);
        if (!dup)
            return JS_FALSE;
        dup->name = entry->name;
        dup->index = entry->index;
        dup->link = map->lastdup;
        map->lastdup = dup;
    }
    entry->name = name;
    entry->index = (uint16) index;
    entry->localKind = (uint8) localKind;
    return JS_TRUE;
}

JSBool
js_AddLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, JSLocalKind kind)
{
    jsuword taggedAtom;
    uint16 *indexp;
    uintN n, i;
    jsuword *array;
    JSLocalNameMap *map;

    JS_ASSERT(FUN_INTERPRETED(fun));
    JS_ASSERT(!fun->u.i.script);
    JS_ASSERT(((jsuword) atom & 1) == 0);
    taggedAtom = (jsuword) atom;
    if (kind == JSLOCAL_ARG) {
        indexp = &fun->nargs;
    } else {
        indexp = &fun->u.i.nvars;
        if (kind == JSLOCAL_CONST)
            taggedAtom |= 1;
        else
            JS_ASSERT(kind == JSLOCAL_VAR);
    }
    n = JS_GET_LOCAL_NAME_COUNT(fun);
    if (n == 0) {
        JS_ASSERT(fun->u.i.names.taggedAtom == 0);
        fun->u.i.names.taggedAtom = taggedAtom;
    } else if (n < MAX_ARRAY_LOCALS) {
        if (n > 1) {
            array = fun->u.i.names.array;
        } else {
            array = (jsuword *) JS_malloc(cx, MAX_ARRAY_LOCALS * sizeof *array);
            if (!array)
                return JS_FALSE;
            array[0] = fun->u.i.names.taggedAtom;
            fun->u.i.names.array = array;
        }
        if (kind == JSLOCAL_ARG) {
            



#if JS_HAS_DESTRUCTURING
            if (fun->u.i.nvars != 0) {
                memmove(array + fun->nargs + 1, array + fun->nargs,
                        fun->u.i.nvars * sizeof *array);
            }
#else
            JS_ASSERT(fun->u.i.nvars == 0);
#endif
            array[fun->nargs] = taggedAtom;
        } else {
            array[n] = taggedAtom;
        }
    } else if (n == MAX_ARRAY_LOCALS) {
        array = fun->u.i.names.array;
        map = (JSLocalNameMap *) JS_malloc(cx, sizeof *map);
        if (!map)
            return JS_FALSE;
        if (!JS_DHashTableInit(&map->names, JS_DHashGetStubOps(),
                               NULL, sizeof(JSLocalNameHashEntry),
                               JS_DHASH_DEFAULT_CAPACITY(MAX_ARRAY_LOCALS
                                                         * 2))) {
            JS_ReportOutOfMemory(cx);
            JS_free(cx, map);
            return JS_FALSE;
        }

        map->lastdup = NULL;
        for (i = 0; i != MAX_ARRAY_LOCALS; ++i) {
            taggedAtom = array[i];
            if (!HashLocalName(cx, map, (JSAtom *) (taggedAtom & ~1),
                               (i < fun->nargs)
                               ? JSLOCAL_ARG
                               : (taggedAtom & 1) ? JSLOCAL_CONST : JSLOCAL_VAR,
                               (i < fun->nargs) ? i : i - fun->nargs)) {
                FreeLocalNameHash(cx, map);
                return JS_FALSE;
            }
        }
        if (!HashLocalName(cx, map, atom, kind, *indexp)) {
            FreeLocalNameHash(cx, map);
            return JS_FALSE;
        }

        



        fun->u.i.names.map = map;
        JS_free(cx, array);
    } else {
        if (*indexp == JS_BITMASK(16)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 (kind == JSLOCAL_ARG)
                                 ? JSMSG_TOO_MANY_FUN_ARGS
                                 : JSMSG_TOO_MANY_FUN_VARS);
            return JS_FALSE;
        }
        if (!HashLocalName(cx, fun->u.i.names.map, atom, kind, *indexp))
            return JS_FALSE;
    }

    
    ++*indexp;
    return JS_TRUE;
}

JSLocalKind
js_LookupLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, uintN *indexp)
{
    uintN n, i;
    jsuword *array;
    JSLocalNameHashEntry *entry;

    JS_ASSERT(FUN_INTERPRETED(fun));
    n = JS_GET_LOCAL_NAME_COUNT(fun);
    if (n == 0)
        return JSLOCAL_NONE;
    if (n <= MAX_ARRAY_LOCALS) {
        array = (n == 1) ? &fun->u.i.names.taggedAtom : fun->u.i.names.array;

        
        i = n;
        do {
            --i;
            if (atom == JS_LOCAL_NAME_TO_ATOM(array[i])) {
                if (i < fun->nargs) {
                    if (indexp)
                        *indexp = i;
                    return JSLOCAL_ARG;
                }
                if (indexp)
                    *indexp = i - fun->nargs;
                return JS_LOCAL_NAME_IS_CONST(array[i])
                       ? JSLOCAL_CONST
                       : JSLOCAL_VAR;
            }
        } while (i != 0);
    } else {
        entry = (JSLocalNameHashEntry *)
                JS_DHashTableOperate(&fun->u.i.names.map->names, atom,
                                     JS_DHASH_LOOKUP);
        if (JS_DHASH_ENTRY_IS_BUSY(&entry->hdr)) {
            JS_ASSERT(entry->localKind != JSLOCAL_NONE);
            if (indexp)
                *indexp = entry->index;
            return (JSLocalKind) entry->localKind;
        }
    }
    return JSLOCAL_NONE;
}

typedef struct JSLocalNameEnumeratorArgs {
    JSFunction      *fun;
    jsuword         *names;
#ifdef DEBUG
    uintN           nCopiedArgs;
    uintN           nCopiedVars;
#endif
} JSLocalNameEnumeratorArgs;

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
get_local_names_enumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                           uint32 number, void *arg)
{
    JSLocalNameHashEntry *entry;
    JSLocalNameEnumeratorArgs *args;
    uint i;
    jsuword constFlag;

    entry = (JSLocalNameHashEntry *) hdr;
    args = (JSLocalNameEnumeratorArgs *) arg;
    JS_ASSERT(entry->name);
    if (entry->localKind == JSLOCAL_ARG) {
        JS_ASSERT(entry->index < args->fun->nargs);
        JS_ASSERT(args->nCopiedArgs++ < args->fun->nargs);
        i = entry->index;
        constFlag = 0;
    } else {
        JS_ASSERT(entry->localKind == JSLOCAL_VAR ||
                  entry->localKind == JSLOCAL_CONST);
        JS_ASSERT(entry->index < args->fun->u.i.nvars);
        JS_ASSERT(args->nCopiedVars++ < args->fun->u.i.nvars);
        i = args->fun->nargs + entry->index;
        constFlag = (entry->localKind == JSLOCAL_CONST);
    }
    args->names[i] = (jsuword) entry->name | constFlag;
    return JS_DHASH_NEXT;
}

jsuword *
js_GetLocalNameArray(JSContext *cx, JSFunction *fun, JSArenaPool *pool)
{
    uintN n;
    jsuword *names;
    JSLocalNameMap *map;
    JSLocalNameEnumeratorArgs args;
    JSNameIndexPair *dup;

    JS_ASSERT(FUN_INTERPRETED(fun));
    n = JS_GET_LOCAL_NAME_COUNT(fun);
    JS_ASSERT(n != 0);

    if (n <= MAX_ARRAY_LOCALS)
        return (n == 1) ? &fun->u.i.names.taggedAtom : fun->u.i.names.array;

    



    JS_ARENA_ALLOCATE_CAST(names, jsuword *, pool, (size_t) n * sizeof *names);
    if (!names) {
        js_ReportOutOfScriptQuota(cx);
        return NULL;
    }

#if JS_HAS_DESTRUCTURING
    
    memset(names, 0, fun->nargs * sizeof *names);
#endif
    map = fun->u.i.names.map;
    args.fun = fun;
    args.names = names;
#ifdef DEBUG
    args.nCopiedArgs = 0;
    args.nCopiedVars = 0;
#endif
    JS_DHashTableEnumerate(&map->names, get_local_names_enumerator, &args);
    for (dup = map->lastdup; dup; dup = dup->link) {
        JS_ASSERT(dup->index < fun->nargs);
        JS_ASSERT(args.nCopiedArgs++ < fun->nargs);
        names[dup->index] = (jsuword) dup->name;
    }
#if !JS_HAS_DESTRUCTURING
    JS_ASSERT(args.nCopiedArgs == fun->nargs);
#endif
    JS_ASSERT(args.nCopiedVars == fun->u.i.nvars);

    return names;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
trace_local_names_enumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                             uint32 number, void *arg)
{
    JSLocalNameHashEntry *entry;
    JSTracer *trc;

    entry = (JSLocalNameHashEntry *) hdr;
    JS_ASSERT(entry->name);
    trc = (JSTracer *) arg;
    JS_SET_TRACING_INDEX(trc,
                         entry->localKind == JSLOCAL_ARG ? "arg" : "var",
                         entry->index);
    JS_CallTracer(trc, ATOM_TO_STRING(entry->name), JSTRACE_STRING);
    return JS_DHASH_NEXT;
}

static void
TraceLocalNames(JSTracer *trc, JSFunction *fun)
{
    uintN n, i;
    JSAtom *atom;
    jsuword *array;

    JS_ASSERT(FUN_INTERPRETED(fun));
    n = JS_GET_LOCAL_NAME_COUNT(fun);
    if (n == 0)
        return;
    if (n <= MAX_ARRAY_LOCALS) {
        array = (n == 1) ? &fun->u.i.names.taggedAtom : fun->u.i.names.array;
        i = n;
        do {
            --i;
            atom = (JSAtom *) (array[i] & ~1);
            if (atom) {
                JS_SET_TRACING_INDEX(trc,
                                     i < fun->nargs ? "arg" : "var",
                                     i < fun->nargs ? i : i - fun->nargs);
                JS_CallTracer(trc, ATOM_TO_STRING(atom), JSTRACE_STRING);
            }
        } while (i != 0);
    } else {
        JS_DHashTableEnumerate(&fun->u.i.names.map->names,
                               trace_local_names_enumerator, trc);

        



    }
}

void
DestroyLocalNames(JSContext *cx, JSFunction *fun)
{
    uintN n;

    n = fun->nargs + fun->u.i.nvars;
    if (n <= 1)
        return;
    if (n <= MAX_ARRAY_LOCALS)
        JS_free(cx, fun->u.i.names.array);
    else
        FreeLocalNameHash(cx, fun->u.i.names.map);
}

void
js_FreezeLocalNames(JSContext *cx, JSFunction *fun)
{
    uintN n;
    jsuword *array;

    JS_ASSERT(FUN_INTERPRETED(fun));
    JS_ASSERT(!fun->u.i.script);
    n = fun->nargs + fun->u.i.nvars;
    if (2 <= n && n < MAX_ARRAY_LOCALS) {
        
        array = (jsuword *) JS_realloc(cx, fun->u.i.names.array,
                                       n * sizeof *array);
        if (array)
            fun->u.i.names.array = array;
    }
}
