










































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

    
    argsobj = js_NewObject(cx, &js_ArgumentsClass, NULL, NULL);
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

JSBool
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
    JS_ASSERT(fp->fun);

    
    if (!parent) {
        funobj = fp->callee;
        if (funobj)
            parent = OBJ_GET_PARENT(cx, funobj);
    }

    
    callobj = js_NewObject(cx, &js_CallClass, NULL, parent);
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

JSBool
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
call_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSStackFrame *fp;
    jsint slot;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->fun);

    slot = JSVAL_TO_INT(id);
    switch (slot) {
      case CALL_ARGUMENTS:
        if (!TEST_OVERRIDE_BIT(fp, slot)) {
            JSObject *argsobj = js_GetArgsObject(cx, fp);
            if (!argsobj)
                return JS_FALSE;
            *vp = OBJECT_TO_JSVAL(argsobj);
        }
        break;

      default:
        if ((uintN)slot < JS_MAX(fp->argc, fp->fun->nargs))
            *vp = fp->argv[slot];
        break;
    }
    return JS_TRUE;
}

static JSBool
call_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSStackFrame *fp;
    jsint slot;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->fun);

    slot = JSVAL_TO_INT(id);
    switch (slot) {
      case CALL_ARGUMENTS:
        SET_OVERRIDE_BIT(fp, slot);
        break;

      default:
        if ((uintN)slot < JS_MAX(fp->argc, fp->fun->nargs))
            fp->argv[slot] = *vp;
        break;
    }
    return JS_TRUE;
}

JSBool
js_GetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSStackFrame *fp;

    JS_ASSERT(JSVAL_IS_INT(id));
    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (fp) {
        
        if ((uintN)JSVAL_TO_INT(id) < fp->nvars)
            *vp = fp->vars[JSVAL_TO_INT(id)];
    }
    return JS_TRUE;
}

JSBool
js_SetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSStackFrame *fp;

    JS_ASSERT(JSVAL_IS_INT(id));
    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (fp) {
        
        jsint slot = JSVAL_TO_INT(id);
        if ((uintN)slot < fp->nvars)
            fp->vars[slot] = *vp;
    }
    return JS_TRUE;
}

static JSBool
call_enumerate(JSContext *cx, JSObject *obj)
{
    JSStackFrame *fp;
    JSObject *funobj, *pobj;
    JSScope *scope;
    JSScopeProperty *sprop, *cprop;
    JSPropertyOp getter;
    jsval *vec;
    jsid id;
    JSProperty *prop;

    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;

    











    funobj = fp->fun->object;
    if (!funobj)
        return JS_TRUE;

    




    scope = OBJ_SCOPE(funobj);
    for (sprop = SCOPE_LAST_PROP(scope); sprop; sprop = sprop->parent) {
        getter = sprop->getter;
        if (getter == js_GetArgument)
            vec = fp->argv;
        else if (getter == js_GetLocalVariable)
            vec = fp->vars;
        else
            continue;

        
        id = JSID_UNHIDE_NAME(sprop->id);
        if (!js_LookupProperty(cx, obj, id, &pobj, &prop))
            return JS_FALSE;

        





        if (!prop || pobj != obj) {
            if (prop)
                OBJ_DROP_PROPERTY(cx, pobj, prop);
            continue;
        }
        cprop = (JSScopeProperty *)prop;
        LOCKED_OBJ_SET_SLOT(obj, cprop->slot, vec[(uint16) sprop->shortid]);
        OBJ_DROP_PROPERTY(cx, obj, prop);
    }

    return JS_TRUE;
}

static JSBool
call_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
             JSObject **objp)
{
    JSStackFrame *fp;
    JSObject *funobj;
    JSString *str;
    JSAtom *atom;
    JSObject *obj2;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSPropertyOp getter, setter;
    uintN attrs, slot, nslots, spflags;
    jsval *vp, value;
    intN shortid;

    fp = (JSStackFrame *) JS_GetPrivate(cx, obj);
    if (!fp)
        return JS_TRUE;
    JS_ASSERT(fp->fun);

    if (!JSVAL_IS_STRING(id))
        return JS_TRUE;

    funobj = fp->callee;
    if (!funobj)
        return JS_TRUE;
    JS_ASSERT((JSFunction *) OBJ_GET_PRIVATE(cx, funobj) == fp->fun);

    str = JSVAL_TO_STRING(id);
    atom = js_AtomizeString(cx, str, 0);
    if (!atom)
        return JS_FALSE;
    if (!js_LookupHiddenProperty(cx, funobj, ATOM_TO_JSID(atom), &obj2, &prop))
        return JS_FALSE;

    if (prop) {
        if (!OBJ_IS_NATIVE(obj2)) {
            OBJ_DROP_PROPERTY(cx, obj2, prop);
            return JS_TRUE;
        }

        sprop = (JSScopeProperty *) prop;
        getter = sprop->getter;
        attrs = sprop->attrs & ~JSPROP_SHARED;
        slot = (uintN) sprop->shortid;
        OBJ_DROP_PROPERTY(cx, obj2, prop);

        
        if ((sprop->flags & SPROP_IS_HIDDEN) &&
            (obj2 == funobj ||
             (JSFunction *) OBJ_GET_PRIVATE(cx, obj2) == fp->fun)) {
            if (getter == js_GetArgument) {
                vp = fp->argv;
                nslots = JS_MAX(fp->argc, fp->fun->nargs);
                getter = setter = NULL;
            } else {
                JS_ASSERT(getter == js_GetLocalVariable);
                vp = fp->vars;
                nslots = fp->nvars;
                getter = js_GetCallVariable;
                setter = js_SetCallVariable;
            }
            if (slot < nslots) {
                value = vp[slot];
                spflags = SPROP_HAS_SHORTID;
                shortid = (intN) slot;
            } else {
                value = JSVAL_VOID;
                spflags = 0;
                shortid = 0;
            }
            if (!js_DefineNativeProperty(cx, obj, ATOM_TO_JSID(atom), value,
                                         getter, setter, attrs,
                                         spflags, shortid, NULL)) {
                return JS_FALSE;
            }
            *objp = obj;
        }
        return JS_TRUE;
    }

    if (!(flags & JSRESOLVE_ASSIGNING)) {
        



        atom = cx->runtime->atomState.argumentsAtom;
        if (id == ATOM_KEY(atom)) {
            if (!js_DefineNativeProperty(cx, obj,
                                         ATOM_TO_JSID(atom), JSVAL_VOID,
                                         NULL, NULL, JSPROP_PERMANENT,
                                         SPROP_HAS_SHORTID, CALL_ARGUMENTS,
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

JSClass js_CallClass = {
    js_Call_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_IS_ANONYMOUS |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Call),
    JS_PropertyStub,    JS_PropertyStub,
    call_getProperty,   call_setProperty,
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
        while (fp && (fp->flags & JSFRAME_SKIP_CALLER) && fp->down)
            fp = fp->down;
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

    
    fun = (JSFunction *)JS_GetInstancePrivate(cx, obj, &js_FunctionClass, NULL);
    JS_ASSERT(fun && fun->object);

    



    if (flags & JSRESOLVE_HIDDEN) {
        if (fun->object != obj) {
            JSObject *pobj;
            JSProperty *prop;

            atom = js_AtomizeString(cx, JSVAL_TO_STRING(id), 0);
            if (!atom)
                return JS_FALSE;
            if (!js_LookupHiddenProperty(cx, fun->object, ATOM_TO_JSID(atom),
                                         &pobj, &prop)) {
                return JS_FALSE;
            }
            if (prop) {
                JS_ASSERT(pobj == fun->object);
                *objp = pobj;
                OBJ_DROP_PROPERTY(cx, pobj, prop);
            }
        }
        return JS_TRUE;
    }

    




    if (flags & JSRESOLVE_ASSIGNING)
        return JS_TRUE;

    



    atom = cx->runtime->atomState.classPrototypeAtom;
    if (id == ATOM_KEY(atom)) {
        JSObject *proto, *parentProto;
        jsval pval;

        proto = parentProto = NULL;
        if (fun->object != obj &&
            (!cx->runtime->findObjectPrincipals ||
             cx->runtime->findObjectPrincipals(cx, obj) ==
             cx->runtime->findObjectPrincipals(cx, fun->object))) {
            





            if (!OBJ_GET_PROPERTY(cx, fun->object, ATOM_TO_JSID(atom), &pval))
                return JS_FALSE;
            if (!JSVAL_IS_PRIMITIVE(pval)) {
                




                cx->weakRoots.newborn[GCX_OBJECT] =
                    (JSGCThing *)JSVAL_TO_GCTHING(pval);
                parentProto = JSVAL_TO_OBJECT(pval);
            }
        }

        



        if (!parentProto && fun->atom == CLASS_ATOM(cx, Object))
            return JS_TRUE;

        




        proto = js_NewObject(cx, &js_ObjectClass, parentProto,
                             OBJ_GET_PARENT(cx, obj));
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

#include "jsxdrapi.h"

enum {
    JSXDR_FUNARG = 1,
    JSXDR_FUNVAR = 2,
    JSXDR_FUNCONST = 3
};


static JSBool
fun_xdrObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    JSFunction *fun;
    uint32 nullAtom;            
    JSTempValueRooter tvr;
    uint32 flagsword;           
    uint16 extraUnused;         
    JSAtom *propAtom;
    JSScopeProperty *sprop;
    uint32 userid;              
    uintN i, n, dupflag;
    uint32 type;
    JSBool ok;
#ifdef DEBUG
    uintN nvars = 0, nargs = 0;
#endif

    cx = xdr->cx;
    if (xdr->mode == JSXDR_ENCODE) {
        




        fun = (JSFunction *) OBJ_GET_PRIVATE(cx, *objp);
        if (!fun)
            return JS_TRUE;
        if (!FUN_INTERPRETED(fun)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NOT_SCRIPTED_FUNCTION,
                                 JS_GetFunctionName(fun));
            return JS_FALSE;
        }
        nullAtom = !fun->atom;
        flagsword = fun->flags;
        extraUnused = 0;
    } else {
        fun = js_NewFunction(cx, NULL, NULL, 0, 0, NULL, NULL);
        if (!fun)
            return JS_FALSE;
    }

    
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &tvr);
    ok = JS_TRUE;

    if (!JS_XDRUint32(xdr, &nullAtom))
        goto bad;
    if (!nullAtom && !js_XDRStringAtom(xdr, &fun->atom))
        goto bad;

    if (!JS_XDRUint16(xdr, &fun->nargs) ||
        !JS_XDRUint16(xdr, &extraUnused) ||
        !JS_XDRUint16(xdr, &fun->u.i.nvars) ||
        !JS_XDRUint32(xdr, &flagsword)) {
        goto bad;
    }

    
    JS_ASSERT(extraUnused == 0);

    
    if (fun->object) {
        n = fun->nargs + fun->u.i.nvars;
        if (xdr->mode == JSXDR_ENCODE) {
            JSScope *scope;
            JSScopeProperty **spvec, *auto_spvec[8];
            void *mark;

            if (n <= sizeof auto_spvec / sizeof auto_spvec[0]) {
                spvec = auto_spvec;
                mark = NULL;
            } else {
                mark = JS_ARENA_MARK(&cx->tempPool);
                JS_ARENA_ALLOCATE_CAST(spvec, JSScopeProperty **, &cx->tempPool,
                                       n * sizeof(JSScopeProperty *));
                if (!spvec) {
                    JS_ReportOutOfMemory(cx);
                    goto bad;
                }
            }
            scope = OBJ_SCOPE(fun->object);
            for (sprop = SCOPE_LAST_PROP(scope); sprop;
                 sprop = sprop->parent) {
                if (sprop->getter == js_GetArgument) {
                    JS_ASSERT(nargs++ <= fun->nargs);
                    spvec[sprop->shortid] = sprop;
                } else if (sprop->getter == js_GetLocalVariable) {
                    JS_ASSERT(nvars++ <= fun->u.i.nvars);
                    spvec[fun->nargs + sprop->shortid] = sprop;
                }
            }
            for (i = 0; i < n; i++) {
                sprop = spvec[i];
                JS_ASSERT(sprop->flags & SPROP_HAS_SHORTID);
                type = (i < fun->nargs)
                       ? JSXDR_FUNARG
                       : (sprop->attrs & JSPROP_READONLY)
                       ? JSXDR_FUNCONST
                       : JSXDR_FUNVAR;
                userid = INT_TO_JSVAL(sprop->shortid);

                





                propAtom = JSID_TO_ATOM(JSID_UNHIDE_NAME(sprop->id));
                if (!JS_XDRUint32(xdr, &type) ||
                    !JS_XDRUint32(xdr, &userid) ||
                    !js_XDRStringAtom(xdr, &propAtom)) {
                    if (mark)
                        JS_ARENA_RELEASE(&cx->tempPool, mark);
                    goto bad;
                }
            }
            if (mark)
                JS_ARENA_RELEASE(&cx->tempPool, mark);
        } else {
            JSPropertyOp getter, setter;

            for (i = n; i != 0; i--) {
                uintN attrs = JSPROP_PERMANENT;

                if (!JS_XDRUint32(xdr, &type) ||
                    !JS_XDRUint32(xdr, &userid) ||
                    !js_XDRStringAtom(xdr, &propAtom)) {
                    goto bad;
                }
                JS_ASSERT(type == JSXDR_FUNARG || type == JSXDR_FUNVAR ||
                          type == JSXDR_FUNCONST);
                if (type == JSXDR_FUNARG) {
                    getter = js_GetArgument;
                    setter = js_SetArgument;
                    JS_ASSERT(nargs++ <= fun->nargs);
                } else if (type == JSXDR_FUNVAR || type == JSXDR_FUNCONST) {
                    getter = js_GetLocalVariable;
                    setter = js_SetLocalVariable;
                    if (type == JSXDR_FUNCONST)
                        attrs |= JSPROP_READONLY;
                    JS_ASSERT(nvars++ <= fun->u.i.nvars);
                } else {
                    getter = NULL;
                    setter = NULL;
                }

                
                dupflag = SCOPE_GET_PROPERTY(OBJ_SCOPE(fun->object),
                                             ATOM_TO_JSID(propAtom))
                          ? SPROP_IS_DUPLICATE
                          : 0;

                if (!js_AddHiddenProperty(cx, fun->object,
                                          ATOM_TO_JSID(propAtom),
                                          getter, setter, SPROP_INVALID_SLOT,
                                          attrs | JSPROP_SHARED,
                                          dupflag | SPROP_HAS_SHORTID,
                                          JSVAL_TO_INT(userid))) {
                    goto bad;
                }
            }
        }
    }

    if (!js_XDRScript(xdr, &fun->u.i.script, NULL))
        goto bad;

    if (xdr->mode == JSXDR_DECODE) {
        fun->flags = (uint16) flagsword | JSFUN_INTERPRETED;

        *objp = fun->object;
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
fun_trace(JSTracer *trc, JSObject *obj)
{
    JSFunction *fun;

    fun = (JSFunction *) JS_GetPrivate(trc->context, obj);
    if (fun) {
        JS_CALL_TRACER(trc, fun, JSTRACE_FUNCTION, "private");
        if (fun->object != obj)
            JS_CALL_TRACER(trc, fun->object, JSTRACE_OBJECT, "object");
        if (fun->atom)
            JS_CALL_STRING_TRACER(trc, ATOM_TO_STRING(fun->atom), "atom");
        if (FUN_INTERPRETED(fun) && fun->u.i.script)
            js_TraceScript(trc, fun->u.i.script);
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
    fun_convert,      JS_FinalizeStub,
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

    fval = vp[1];
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
    if (argc != 0 && !js_ValueToECMAUint32(cx, vp[2], &indent))
        return JS_FALSE;

    JS_ASSERT(JS_ObjectIsFunction(cx, obj));
    fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
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
    jsval fval, *argv, *sp, *oldsp;
    JSString *str;
    void *mark;
    uintN i;
    JSStackFrame *fp;
    JSBool ok;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_FUNCTION, &vp[1]))
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

    
    sp = js_AllocStack(cx, 2 + argc, &mark);
    if (!sp)
        return JS_FALSE;

    
    *sp++ = fval;
    *sp++ = OBJECT_TO_JSVAL(obj);
    for (i = 0; i < argc; i++)
        *sp++ = argv[i];

    
    fp = cx->fp;
    oldsp = fp->sp;
    fp->sp = sp;
    ok = js_Invoke(cx, argc,
                   (fp->flags & JSFRAME_IN_FAST_CALL)
                   ? JSINVOKE_INTERNAL
                   : JSINVOKE_INTERNAL | JSINVOKE_SKIP_CALLER);

    
    *vp = fp->sp[-1];
    fp->sp = oldsp;
    js_FreeStack(cx, mark);
    return ok;
}

static JSBool
fun_apply(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj, *aobj;
    jsval fval, *sp, *oldsp;
    JSString *str;
    jsuint length;
    JSBool arraylike, ok;
    void *mark;
    uintN i;
    JSStackFrame *fp;

    if (argc == 0) {
        
        return fun_call(cx, argc, vp);
    }

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_FUNCTION, &vp[1]))
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
    sp = js_AllocStack(cx, 2 + argc, &mark);
    if (!sp)
        return JS_FALSE;

    
    *sp++ = fval;
    *sp++ = OBJECT_TO_JSVAL(obj);
    for (i = 0; i < argc; i++) {
        ok = JS_GetElement(cx, aobj, (jsint)i, sp);
        if (!ok)
            goto out;
        sp++;
    }

    
    fp = cx->fp;
    oldsp = fp->sp;
    fp->sp = sp;
    ok = js_Invoke(cx, argc,
                   (fp->flags & JSFRAME_IN_FAST_CALL)
                   ? JSINVOKE_INTERNAL
                   : JSINVOKE_INTERNAL | JSINVOKE_SKIP_CALLER);

    
    *vp = fp->sp[-1];
    fp->sp = oldsp;
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
    jsval *sp, *newsp, *oldsp;
    JSStackFrame *fp;
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
    newsp = sp = js_AllocStack(cx, 2 + length, &mark);
    if (!sp)
        return JS_FALSE;

    fp = cx->fp;
    oldsp = fp->sp;
    *sp++ = vp[1];
    *sp++ = JSVAL_NULL; 
    for (i = 0; i < length; i++) {
        ok = JS_GetElement(cx, aobj, (jsint)i, sp);
        if (!ok)
            goto out;
        sp++;
    }

    oldsp = fp->sp;
    fp->sp = sp;
    ok = js_InvokeConstructor(cx, newsp, length);

    *vp = fp->sp[-1];
    fp->sp = oldsp;
out:
    js_FreeStack(cx, mark);
    return ok;
}
#endif

static JSFunctionSpec function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0,0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0,0,0),
    JS_FN("apply",           fun_apply,      0,2,0,0),
    JS_FN(call_str,          fun_call,       0,1,0,0),
#ifdef NARCISSUS
    JS_FN("__applyConstructor__", fun_applyConstructor, 0,1,0,0),
#endif
    JS_FS_END
};

static JSBool
Function(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSStackFrame *fp, *caller;
    JSFunction *fun;
    JSObject *parent;
    uintN i, n, lineno, dupflag;
    JSAtom *atom;
    const char *filename;
    JSObject *obj2;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSString *str, *arg;
    void *mark;
    JSTokenStream *ts;
    JSPrincipals *principals;
    jschar *collected_args, *cp;
    size_t arg_length, args_length, old_args_length;
    JSTokenType tt;
    JSBool ok;

    fp = cx->fp;
    if (!(fp->flags & JSFRAME_CONSTRUCTING)) {
        obj = js_NewObject(cx, &js_FunctionClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(obj);
    }
    fun = (JSFunction *) JS_GetPrivate(cx, obj);
    if (fun)
        return JS_TRUE;

    









    parent = OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(argv[-2]));

    fun = js_NewFunction(cx, obj, NULL, 0, JSFUN_LAMBDA, parent,
                         cx->runtime->atomState.anonymousAtom);

    if (!fun)
        return JS_FALSE;

    






    JS_ASSERT(!fp->script && fp->fun && fp->fun->u.n.native == Function);
    caller = JS_GetScriptedCaller(cx, fp);
    if (caller) {
        filename = caller->script->filename;
        lineno = js_PCToLineNumber(cx, caller->script, caller->pc);
        principals = JS_EvalFramePrincipals(cx, fp, caller);
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
        









        args_length = 0;
        for (i = 0; i < n; i++) {
            
            arg = js_ValueToString(cx, argv[i]);
            if (!arg)
                return JS_FALSE;
            argv[i] = STRING_TO_JSVAL(arg);

            



            old_args_length = args_length;
            args_length = old_args_length + JSSTRING_LENGTH(arg);
            if (args_length < old_args_length) {
                JS_ReportOutOfMemory(cx);
                return JS_FALSE;
            }
        }

        
        old_args_length = args_length;
        args_length = old_args_length + n - 1;
        if (args_length < old_args_length ||
            args_length >= ~(size_t)0 / sizeof(jschar)) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }

        




        mark = JS_ARENA_MARK(&cx->tempPool);
        JS_ARENA_ALLOCATE_CAST(cp, jschar *, &cx->tempPool,
                               (args_length+1) * sizeof(jschar));
        if (!cp) {
            JS_ReportOutOfMemory(cx);
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

        



        ts = js_NewTokenStream(cx, collected_args, args_length, filename,
                               lineno, principals);
        if (!ts) {
            JS_ARENA_RELEASE(&cx->tempPool, mark);
            return JS_FALSE;
        }

        
        tt = js_GetToken(cx, ts);
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (tt != TOK_NAME)
                    goto bad_formal;

                



                atom = CURRENT_TOKEN(ts).t_atom;
                if (!js_LookupHiddenProperty(cx, obj, ATOM_TO_JSID(atom),
                                             &obj2, &prop)) {
                    goto bad_formal;
                }
                sprop = (JSScopeProperty *) prop;
                dupflag = 0;
                if (sprop) {
                    ok = JS_TRUE;
                    if (obj2 == obj) {
                        const char *name = js_AtomToPrintableString(cx, atom);

                        





                        JS_ASSERT(sprop->getter == js_GetArgument);
                        ok = name &&
                             js_ReportCompileErrorNumber(cx, ts,
                                                         JSREPORT_TS |
                                                         JSREPORT_WARNING |
                                                         JSREPORT_STRICT,
                                                         JSMSG_DUPLICATE_FORMAL,
                                                         name);

                        dupflag = SPROP_IS_DUPLICATE;
                    }
                    OBJ_DROP_PROPERTY(cx, obj2, prop);
                    if (!ok)
                        goto bad_formal;
                    sprop = NULL;
                }
                if (!js_AddHiddenProperty(cx, fun->object, ATOM_TO_JSID(atom),
                                          js_GetArgument, js_SetArgument,
                                          SPROP_INVALID_SLOT,
                                          JSPROP_PERMANENT | JSPROP_SHARED,
                                          dupflag | SPROP_HAS_SHORTID,
                                          fun->nargs)) {
                    goto bad_formal;
                }
                if (fun->nargs == JS_BITMASK(16)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TOO_MANY_FUN_ARGS);
                    goto bad;
                }
                fun->nargs++;

                



                tt = js_GetToken(cx, ts);
                if (tt == TOK_EOF)
                    break;
                if (tt != TOK_COMMA)
                    goto bad_formal;
                tt = js_GetToken(cx, ts);
            }
        }

        
        ok = js_CloseTokenStream(cx, ts);
        JS_ARENA_RELEASE(&cx->tempPool, mark);
        if (!ok)
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

    mark = JS_ARENA_MARK(&cx->tempPool);
    ts = js_NewTokenStream(cx, JSSTRING_CHARS(str), JSSTRING_LENGTH(str),
                           filename, lineno, principals);
    if (!ts) {
        ok = JS_FALSE;
    } else {
        ok = js_CompileFunctionBody(cx, ts, fun) &&
             js_CloseTokenStream(cx, ts);
    }
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;

bad_formal:
    



    if (!(ts->flags & TSF_ERROR))
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_FORMAL);

bad:
    



    (void)js_CloseTokenStream(cx, ts);
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return JS_FALSE;
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
    fun = js_NewFunction(cx, proto, NULL, 0, 0, obj, NULL);
    if (!fun)
        goto bad;
    fun->u.i.script = js_NewScript(cx, 1, 0, 0, 0, 0, 0);
    if (!fun->u.i.script)
        goto bad;
    fun->u.i.script->code[0] = JSOP_STOP;
    fun->flags |= JSFUN_INTERPRETED;
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
    JSTempValueRooter tvr;

    
    if (funobj) {
        OBJ_SET_PARENT(cx, funobj, parent);
    } else {
        funobj = js_NewObject(cx, &js_FunctionClass, NULL, parent);
        if (!funobj)
            return NULL;
    }

    
    JS_PUSH_SINGLE_TEMP_ROOT(cx, OBJECT_TO_JSVAL(funobj), &tvr);

    



    fun = (JSFunction *) js_NewGCThing(cx, GCX_FUNCTION, sizeof(JSFunction));
    if (!fun)
        goto out;

    
    fun->object = NULL;
    fun->nargs = nargs;
    fun->flags = flags & JSFUN_FLAGS_MASK;
    fun->u.n.native = native;
    fun->u.n.extra = 0;
    fun->u.n.minargs = 0;
    fun->atom = atom;
    fun->clasp = NULL;

    
    if (!js_LinkFunctionObject(cx, fun, funobj)) {
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;
        fun = NULL;
    }

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return fun;
}

void
js_FinalizeFunction(JSContext *cx, JSFunction *fun)
{
    JSScript *script;

    



    if (FUN_INTERPRETED(fun) && fun->u.i.script) {
        script = fun->u.i.script;
        fun->u.i.script = NULL;
        js_DestroyScript(cx, script);
    }
}

JSObject *
js_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent)
{
    JSObject *newfunobj;
    JSFunction *fun;

    JS_ASSERT(OBJ_GET_CLASS(cx, funobj) == &js_FunctionClass);
    newfunobj = js_NewObject(cx, &js_FunctionClass, NULL, parent);
    if (!newfunobj)
        return NULL;
    fun = (JSFunction *) JS_GetPrivate(cx, funobj);
    if (!js_LinkFunctionObject(cx, fun, newfunobj)) {
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;
        return NULL;
    }
    return newfunobj;
}

JSBool
js_LinkFunctionObject(JSContext *cx, JSFunction *fun, JSObject *funobj)
{
    if (!fun->object)
        fun->object = funobj;
    return JS_SetPrivate(cx, funobj, fun);
}

JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, JSNative native,
                  uintN nargs, uintN attrs)
{
    JSFunction *fun;

    fun = js_NewFunction(cx, NULL, native, nargs, attrs, obj, atom);
    if (!fun)
        return NULL;
    if (!OBJ_DEFINE_PROPERTY(cx, obj, ATOM_TO_JSID(atom),
                             OBJECT_TO_JSVAL(fun->object),
                             NULL, NULL,
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
    return (JSFunction *) JS_GetPrivate(cx, obj);
}

JSObject *
js_ValueToFunctionObject(JSContext *cx, jsval *vp, uintN flags)
{
    JSFunction *fun;
    JSObject *funobj;
    JSStackFrame *caller;
    JSPrincipals *principals;

    if (VALUE_IS_FUNCTION(cx, *vp))
        return JSVAL_TO_OBJECT(*vp);

    fun = js_ValueToFunction(cx, vp, flags);
    if (!fun)
        return NULL;
    funobj = fun->object;
    *vp = OBJECT_TO_JSVAL(funobj);

    caller = JS_GetScriptedCaller(cx, cx->fp);
    if (caller) {
        principals = caller->script->principals;
    } else {
        
        principals = NULL;
    }

    if (!js_CheckPrincipalsAccess(cx, funobj, principals,
                                  fun->atom
                                  ? fun->atom
                                  : cx->runtime->atomState.anonymousAtom)) {
        return NULL;
    }
    return funobj;
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

    for (fp = cx->fp; fp && !fp->spbase; fp = fp->down)
        continue;
    name = NULL;
    source = NULL;
    if (flags & JSV2F_ITERATOR) {
        error = JSMSG_BAD_ITERATOR;
        name = js_iterator_str;
        source = js_ValueToPrintableSource(cx, *vp);
        if (!source)
            return;
    } else if (flags & JSV2F_CONSTRUCT) {
        error = JSMSG_NOT_CONSTRUCTOR;
    } else {
        error = JSMSG_NOT_FUNCTION;
    }

    js_ReportValueError3(cx, error,
                         (fp &&
                          !(fp->flags & JSFRAME_IN_FAST_CALL) &&
                          fp->spbase <= vp && vp < fp->sp)
                         ? vp - fp->sp
                         : (flags & JSV2F_SEARCH_STACK)
                         ? JSDVG_SEARCH_STACK
                         : JSDVG_IGNORE_STACK,
                         *vp, NULL,
                         name, source);
}
