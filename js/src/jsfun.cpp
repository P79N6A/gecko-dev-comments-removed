










































#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jspropertytree.h"
#include "jsproxy.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsexn.h"
#include "jsstaticcheck.h"
#include "jstracer.h"

#if JS_HAS_GENERATORS
# include "jsiter.h"
#endif

#if JS_HAS_XDR
# include "jsxdrapi.h"
#endif

#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jsfuninlines.h"
#include "jsobjinlines.h"

using namespace js;

inline JSObject *
JSObject::getThrowTypeError() const
{
    return &getGlobal()->getReservedSlot(JSRESERVED_GLOBAL_THROWTYPEERROR).toObject();
}

JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, Value *vp)
{
    JSObject *argsobj;

    if (fp->flags & JSFRAME_OVERRIDE_ARGS) {
        JS_ASSERT(fp->hasCallObj());
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom);
        return fp->getCallObj()->getProperty(cx, id, vp);
    }
    argsobj = js_GetArgsObject(cx, fp);
    if (!argsobj)
        return JS_FALSE;
    vp->setObject(*argsobj);
    return JS_TRUE;
}

JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, Value *vp)
{
    if (fp->flags & JSFRAME_OVERRIDE_ARGS) {
        JS_ASSERT(fp->hasCallObj());

        jsid argumentsid = ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom);
        Value v;
        if (!fp->getCallObj()->getProperty(cx, argumentsid, &v))
            return false;

        JSObject *obj;
        if (v.isPrimitive()) {
            obj = js_ValueToNonNullObject(cx, v);
            if (!obj)
                return false;
        } else {
            obj = &v.toObject();
        }
        return obj->getProperty(cx, id, vp);
    }

    vp->setUndefined();
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        JSObject *argsobj = fp->maybeArgsObj();
        if (arg < fp->numActualArgs()) {
            if (argsobj) {
                if (argsobj->getArgsElement(arg).isMagic(JS_ARGS_HOLE))
                    return argsobj->getProperty(cx, id, vp);
            }
            *vp = fp->argv[arg];
        } else {
            











            if (argsobj)
                return argsobj->getProperty(cx, id, vp);
        }
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        JSObject *argsobj = fp->maybeArgsObj();
        if (argsobj && argsobj->isArgsLengthOverridden())
            return argsobj->getProperty(cx, id, vp);
        vp->setInt32(fp->numActualArgs());
    }
    return true;
}

static JSObject *
NewArguments(JSContext *cx, JSObject *parent, uint32 argc, JSObject *callee)
{
    JSObject *proto;
    if (!js_GetClassPrototype(cx, parent, JSProto_Object, &proto))
        return NULL;

    JSObject *argsobj = js_NewGCObject(cx);
    if (!argsobj)
        return NULL;

    ArgumentsData *data = (ArgumentsData *)
        cx->malloc(offsetof(ArgumentsData, slots) + argc * sizeof(Value));
    if (!data)
        return NULL;
    SetValueRangeToUndefined(data->slots, argc);

    
    argsobj->init(callee->getFunctionPrivate()->inStrictMode()
                  ? &StrictArgumentsClass
                  : &js_ArgumentsClass,
                  proto, parent, PrivateValue(NULL), cx);

    argsobj->setMap(cx->runtime->emptyArgumentsShape);

    argsobj->setArgsLength(argc);
    argsobj->setArgsData(data);
    data->callee = ObjectValue(*callee);

    return argsobj;
}

static void
PutArguments(JSContext *cx, JSObject *argsobj, Value *args)
{
    JS_ASSERT(argsobj->isNormalArguments());
    uint32 argc = argsobj->getArgsInitialLength();
    ArgumentsData *data = argsobj->getArgsData();
    for (uint32 i = 0; i != argc; ++i) {
        if (!data->slots[i].isMagic(JS_ARGS_HOLE))
            data->slots[i] = args[i];
    }
}

JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp)
{
    



    JS_ASSERT(fp->hasFunction());
    JS_ASSERT_IF(fp->getFunction()->flags & JSFUN_HEAVYWEIGHT,
                 fp->varobj(cx->containingSegment(fp)));

    
    while (fp->flags & JSFRAME_SPECIAL)
        fp = fp->down;

    
    if (fp->hasArgsObj())
        return fp->getArgsObj();

    
    JSObject *global = fp->getScopeChain()->getGlobal();
    JSObject *argsobj = NewArguments(cx, global, fp->numActualArgs(), &fp->argv[-2].toObject());
    if (!argsobj)
        return argsobj;

    








    if (argsobj->isStrictArguments())
        memcpy(argsobj->getArgsData()->slots, fp->argv, fp->numActualArgs() * sizeof(Value));
    else
        argsobj->setPrivate(fp);

    fp->setArgsObj(argsobj);
    return argsobj;
}

void
js_PutArgsObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *argsobj = fp->getArgsObj();
    if (argsobj->isNormalArguments()) {
        JS_ASSERT(argsobj->getPrivate() == fp);
        PutArguments(cx, argsobj, fp->argv);
        argsobj->setPrivate(NULL);
    } else {
        JS_ASSERT(!argsobj->getPrivate());
    }
    fp->setArgsObj(NULL);
}

#ifdef JS_TRACER




JSObject * JS_FASTCALL
js_Arguments(JSContext *cx, JSObject *parent, uint32 argc, JSObject *callee)
{
    JSObject *argsobj = NewArguments(cx, parent, argc, callee);
    if (!argsobj)
        return NULL;

    if (argsobj->isStrictArguments()) {
        



        JS_ASSERT(!argsobj->getPrivate());
    } else {
        argsobj->setPrivate(JS_ARGUMENTS_OBJECT_ON_TRACE);
    }

    return argsobj;
}
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_Arguments, CONTEXT, OBJECT, UINT32, OBJECT,
                     0, nanojit::ACCSET_STORE_ANY)


JSBool JS_FASTCALL
js_PutArguments(JSContext *cx, JSObject *argsobj, Value *args)
{
    JS_ASSERT(argsobj->isNormalArguments());
    JS_ASSERT(argsobj->getPrivate() == JS_ARGUMENTS_OBJECT_ON_TRACE);
    PutArguments(cx, argsobj, args);
    argsobj->setPrivate(NULL);
    return true;
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_PutArguments, CONTEXT, OBJECT, VALUEPTR, 0,
                     nanojit::ACCSET_STORE_ANY)

#endif 

static JSBool
args_delProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(obj->isArguments());

    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsInitialLength())
            obj->setArgsElement(arg, MagicValue(JS_ARGS_HOLE));
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        obj->setArgsLengthOverridden();
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom)) {
        obj->setArgsCallee(MagicValue(JS_ARGS_HOLE));
    }
    return true;
}

static JS_REQUIRES_STACK JSObject *
WrapEscapingClosure(JSContext *cx, JSStackFrame *fp, JSFunction *fun)
{
    JS_ASSERT(fun->optimizedClosure());
    JS_ASSERT(!fun->u.i.wrapper);

    






    JSObject *scopeChain = js_GetScopeChain(cx, fp);
    if (!scopeChain)
        return NULL;

    JSObject *wfunobj = NewFunction(cx, scopeChain);
    if (!wfunobj)
        return NULL;
    AutoObjectRooter tvr(cx, wfunobj);

    JSFunction *wfun = (JSFunction *) wfunobj;
    wfunobj->setPrivate(wfun);
    wfun->nargs = fun->nargs;
    wfun->flags = fun->flags | JSFUN_HEAVYWEIGHT;
    wfun->u.i.nvars = fun->u.i.nvars;
    wfun->u.i.nupvars = fun->u.i.nupvars;
    wfun->u.i.skipmin = fun->u.i.skipmin;
    wfun->u.i.wrapper = true;
    wfun->u.i.script = NULL;
    wfun->u.i.names = fun->u.i.names;
    wfun->atom = fun->atom;

    JSScript *script = fun->u.i.script;
    jssrcnote *snbase = script->notes();
    jssrcnote *sn = snbase;
    while (!SN_IS_TERMINATOR(sn))
        sn = SN_NEXT(sn);
    uintN nsrcnotes = (sn - snbase) + 1;

    
    JSScript *wscript = js_NewScript(cx, script->length, nsrcnotes,
                                     script->atomMap.length,
                                     (script->objectsOffset != 0)
                                     ? script->objects()->length
                                     : 0,
                                     fun->u.i.nupvars,
                                     (script->regexpsOffset != 0)
                                     ? script->regexps()->length
                                     : 0,
                                     (script->trynotesOffset != 0)
                                     ? script->trynotes()->length
                                     : 0,
                                     (script->constOffset != 0)
                                     ? script->consts()->length
                                     : 0);
    if (!wscript)
        return NULL;

    memcpy(wscript->code, script->code, script->length);
    wscript->main = wscript->code + (script->main - script->code);

    memcpy(wscript->notes(), snbase, nsrcnotes * sizeof(jssrcnote));
    memcpy(wscript->atomMap.vector, script->atomMap.vector,
           wscript->atomMap.length * sizeof(JSAtom *));
    if (script->objectsOffset != 0) {
        memcpy(wscript->objects()->vector, script->objects()->vector,
               wscript->objects()->length * sizeof(JSObject *));
    }
    if (script->regexpsOffset != 0) {
        memcpy(wscript->regexps()->vector, script->regexps()->vector,
               wscript->regexps()->length * sizeof(JSObject *));
    }
    if (script->trynotesOffset != 0) {
        memcpy(wscript->trynotes()->vector, script->trynotes()->vector,
               wscript->trynotes()->length * sizeof(JSTryNote));
    }

    if (wfun->u.i.nupvars != 0) {
        JS_ASSERT(wfun->u.i.nupvars == wscript->upvars()->length);
        memcpy(wscript->upvars()->vector, script->upvars()->vector,
               wfun->u.i.nupvars * sizeof(uint32));
    }

    jsbytecode *pc = wscript->code;
    while (*pc != JSOP_STOP) {
        
        JSOp op = js_GetOpcode(cx, wscript, pc);
        const JSCodeSpec *cs = &js_CodeSpec[op];
        ptrdiff_t oplen = cs->length;
        if (oplen < 0)
            oplen = js_GetVariableBytecodeLength(pc);

        





        switch (op) {
          case JSOP_GETUPVAR:       *pc = JSOP_GETUPVAR_DBG; break;
          case JSOP_CALLUPVAR:      *pc = JSOP_CALLUPVAR_DBG; break;
          case JSOP_GETFCSLOT:      *pc = JSOP_GETUPVAR_DBG; break;
          case JSOP_CALLFCSLOT:     *pc = JSOP_CALLUPVAR_DBG; break;
          case JSOP_DEFFUN_FC:      *pc = JSOP_DEFFUN_DBGFC; break;
          case JSOP_DEFLOCALFUN_FC: *pc = JSOP_DEFLOCALFUN_DBGFC; break;
          case JSOP_LAMBDA_FC:      *pc = JSOP_LAMBDA_DBGFC; break;
          default:;
        }
        pc += oplen;
    }

    



    wscript->noScriptRval = script->noScriptRval;
    wscript->savedCallerFun = script->savedCallerFun;
    wscript->hasSharps = script->hasSharps;
    wscript->strictModeCode = script->strictModeCode;
    wscript->version = script->version;
    wscript->nfixed = script->nfixed;
    wscript->filename = script->filename;
    wscript->lineno = script->lineno;
    wscript->nslots = script->nslots;
    wscript->staticLevel = script->staticLevel;
    wscript->principals = script->principals;
    if (wscript->principals)
        JSPRINCIPALS_HOLD(cx, wscript->principals);
#ifdef CHECK_SCRIPT_OWNER
    wscript->owner = script->owner;
#endif

    
    FUN_SET_KIND(wfun, JSFUN_INTERPRETED);
    wfun->u.i.script = wscript;
    return wfunobj;
}

static JSBool
ArgGetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    LeaveTrace(cx);

    if (!InstanceOf(cx, obj, &js_ArgumentsClass, NULL))
        return true;

    if (JSID_IS_INT(id)) {
        



        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsInitialLength()) {
            JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
            if (fp) {
                *vp = fp->argv[arg];
            } else {
                const Value &v = obj->getArgsElement(arg);
                if (!v.isMagic(JS_ARGS_HOLE))
                    *vp = v;
            }
        }
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (!obj->isArgsLengthOverridden())
            vp->setInt32(obj->getArgsInitialLength());
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom));
        const Value &v = obj->getArgsCallee();
        if (!v.isMagic(JS_ARGS_HOLE)) {
            







            if (GET_FUNCTION_PRIVATE(cx, &v.toObject())->needsWrapper()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_OPTIMIZED_CLOSURE_LEAK);
                return false;
            }
            *vp = v;
        }
    }
    return true;
}

static JSBool
ArgSetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
#ifdef JS_TRACER
    
    
    
    
    if (JS_ON_TRACE(cx)) {
        DeepBail(cx);
        return false;
    }
#endif

    if (!InstanceOf(cx, obj, &js_ArgumentsClass, NULL))
        return true;

    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsInitialLength()) {
            JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
            if (fp) {
                fp->argv[arg] = *vp;
                return true;
            }
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom) ||
                  JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom));
    }

    





    AutoValueRooter tvr(cx);
    return js_DeleteProperty(cx, obj, id, tvr.addr()) &&
           js_SetProperty(cx, obj, id, vp);
}

static JSBool
args_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
             JSObject **objp)
{
    JS_ASSERT(obj->isNormalArguments());

    *objp = NULL;
    bool valid = false;
    uintN attrs = JSPROP_SHARED;
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        attrs = JSPROP_ENUMERATE | JSPROP_SHARED;
        if (arg < obj->getArgsInitialLength() && !obj->getArgsElement(arg).isMagic(JS_ARGS_HOLE))
            valid = true;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (!obj->isArgsLengthOverridden())
            valid = true;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom)) {
        if (!obj->getArgsCallee().isMagic(JS_ARGS_HOLE))
            valid = true;
    }

    if (valid) {
        Value tmp = UndefinedValue();
        if (!js_DefineProperty(cx, obj, id, &tmp, ArgGetter, ArgSetter, attrs))
            return JS_FALSE;
        *objp = obj;
    }
    return true;
}

static JSBool
args_enumerate(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNormalArguments());

    



    int argc = int(obj->getArgsInitialLength());
    for (int i = -2; i != argc; i++) {
        jsid id = (i == -2)
                  ? ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)
                  : (i == -1)
                  ? ATOM_TO_JSID(cx->runtime->atomState.calleeAtom)
                  : INT_TO_JSID(i);

        JSObject *pobj;
        JSProperty *prop;
        if (!js_LookupProperty(cx, obj, id, &pobj, &prop))
            return false;

        
        if (prop)
            pobj->dropProperty(cx, prop);
    }
    return true;
}

namespace {

JSBool
StrictArgGetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    LeaveTrace(cx);

    if (!InstanceOf(cx, obj, &StrictArgumentsClass, NULL))
        return true;

    if (JSID_IS_INT(id)) {
        



        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsInitialLength()) {
            const Value &v = obj->getArgsElement(arg);
            if (!v.isMagic(JS_ARGS_HOLE))
                *vp = v;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom));
        if (!obj->isArgsLengthOverridden())
            vp->setInt32(obj->getArgsInitialLength());
    }
    return true;
}

JSBool
StrictArgSetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (!InstanceOf(cx, obj, &StrictArgumentsClass, NULL))
        return true;

    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsInitialLength()) {
            obj->setArgsElement(arg, *vp);
            return true;
        }
    } else {
        JS_ASSERT(JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom));
    }

    





    AutoValueRooter tvr(cx);
    return js_DeleteProperty(cx, obj, id, tvr.addr()) &&
           js_SetProperty(cx, obj, id, vp);
}

JSBool
strictargs_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    JS_ASSERT(obj->isStrictArguments());

    *objp = NULL;
    bool valid = false;
    uintN attrs = JSPROP_SHARED;
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        attrs = JSPROP_SHARED | JSPROP_ENUMERATE;
        if (arg < obj->getArgsInitialLength() && !obj->getArgsElement(arg).isMagic(JS_ARGS_HOLE))
            valid = true;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (!obj->isArgsLengthOverridden())
            valid = true;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.calleeAtom)) {
        Value tmp = UndefinedValue();
        PropertyOp throwTypeError = CastAsPropertyOp(obj->getThrowTypeError());
        uintN attrs = JSPROP_PERMANENT | JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
        if (!js_DefineProperty(cx, obj, id, &tmp, throwTypeError, throwTypeError, attrs))
            return false;

        *objp = obj;
        return true;
    } else if (JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom)) {
        



        PropertyOp throwTypeError = CastAsPropertyOp(obj->getThrowTypeError());
        Value tmp = UndefinedValue();
        if (!js_DefineProperty(cx, obj, id, &tmp, throwTypeError, throwTypeError,
                               JSPROP_PERMANENT | JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED)) {
            return false;
        }

        *objp = obj;
        return true;
    }

    if (valid) {
        Value tmp = UndefinedValue();
        if (!js_DefineProperty(cx, obj, id, &tmp, StrictArgGetter, StrictArgSetter, attrs))
            return false;
        *objp = obj;
    }
    return true;
}

JSBool
strictargs_enumerate(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isStrictArguments());

    



    JSObject *pobj;
    JSProperty *prop;

    
    if (!js_LookupProperty(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom), &pobj, &prop))
        return false;
    if (prop)
        pobj->dropProperty(cx, prop);

    
    if (!js_LookupProperty(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.calleeAtom), &pobj, &prop))
        return false;
    if (prop)
        pobj->dropProperty(cx, prop);

    
    if (!js_LookupProperty(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.callerAtom), &pobj, &prop))
        return false;
    if (prop)
        pobj->dropProperty(cx, prop);

    for (uint32 i = 0, argc = obj->getArgsInitialLength(); i < argc; i++) {
        if (!js_LookupProperty(cx, obj, INT_TO_JSID(i), &pobj, &prop))
            return false;
        if (prop)
            pobj->dropProperty(cx, prop);
    }

    return true;
}

} 

static void
args_finalize(JSContext *cx, JSObject *obj)
{
    cx->free((void *) obj->getArgsData());
}









static inline void
MaybeMarkGenerator(JSTracer *trc, JSObject *obj)
{
#if JS_HAS_GENERATORS
    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp && fp->isFloatingGenerator()) {
        JSObject *genobj = js_FloatingFrameToGenerator(fp)->obj;
        MarkObject(trc, genobj, "generator object");
    }
#endif
}

static void
args_trace(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->isArguments());
    if (obj->getPrivate() == JS_ARGUMENTS_OBJECT_ON_TRACE) {
        JS_ASSERT(!obj->isStrictArguments());
        return;
    }

    ArgumentsData *data = obj->getArgsData();
    if (data->callee.isObject())
        MarkObject(trc, &data->callee.toObject(), js_callee_str);
    MarkValueRange(trc, obj->getArgsInitialLength(), data->slots, js_arguments_str);

    MaybeMarkGenerator(trc, obj);
}













Class js_ArgumentsClass = {
    "Arguments",
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::ARGS_CLASS_RESERVED_SLOTS) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,   
    args_delProperty,
    PropertyStub,   
    PropertyStub,   
    args_enumerate,
    (JSResolveOp) args_resolve,
    ConvertStub,
    args_finalize,  
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    JS_CLASS_TRACE(args_trace)
};

namespace js {






Class StrictArgumentsClass = {
    "Arguments",
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::ARGS_CLASS_RESERVED_SLOTS) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,   
    args_delProperty,
    PropertyStub,   
    PropertyStub,   
    strictargs_enumerate,
    reinterpret_cast<JSResolveOp>(strictargs_resolve),
    ConvertStub,
    args_finalize,  
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    JS_CLASS_TRACE(args_trace)
};

}

const uint32 JSSLOT_CALLEE              = JSSLOT_PRIVATE + 1;
const uint32 JSSLOT_CALL_ARGUMENTS      = JSSLOT_PRIVATE + 2;
const uint32 CALL_CLASS_RESERVED_SLOTS  = 2;





Class js_DeclEnvClass = {
    js_Object_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub
};

static JSBool
CheckForEscapingClosure(JSContext *cx, JSObject *obj, Value *vp)
{
    JS_ASSERT(obj->isCall() || obj->getClass() == &js_DeclEnvClass);

    const Value &v = *vp;

    JSObject *funobj;
    if (IsFunctionObject(v, &funobj)) {
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

        





        if (fun->needsWrapper()) {
            LeaveTrace(cx);

            JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
            if (fp) {
                JSObject *wrapper = WrapEscapingClosure(cx, fp, fun);
                if (!wrapper)
                    return false;
                vp->setObject(*wrapper);
                return true;
            }

            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_OPTIMIZED_CLOSURE_LEAK);
            return false;
        }
    }
    return true;
}

static JSBool
CalleeGetter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CheckForEscapingClosure(cx, obj, vp);
}

static JSObject *
NewCallObject(JSContext *cx, JSFunction *fun, JSObject *scopeChain)
{
    JSObject *callobj = js_NewGCObject(cx);
    if (!callobj)
        return NULL;

    callobj->init(&js_CallClass, NULL, scopeChain, PrivateValue(NULL), cx);
    callobj->setMap(fun->u.i.names);

    
    if (!callobj->ensureInstanceReservedSlots(cx, fun->countArgsAndVars()))
        return NULL;

#ifdef DEBUG
    for (Shape::Range r = callobj->lastProp; !r.empty(); r.popFront()) {
        const Shape &s = r.front();
        if (s.slot != SHAPE_INVALID_SLOT) {
            JS_ASSERT(s.slot + 1 == callobj->freeslot);
            break;
        }
    }
#endif
    return callobj;
}

static inline JSObject *
NewDeclEnvObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *envobj = js_NewGCObject(cx);
    if (!envobj)
        return NULL;

    envobj->init(&js_DeclEnvClass, NULL, fp->maybeScopeChain(), PrivateValue(fp), cx);
    envobj->setMap(cx->runtime->emptyDeclEnvShape);
    return envobj;
}

JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp)
{
    
    JS_ASSERT(fp->hasFunction());
    if (fp->hasCallObj())
        return fp->getCallObj();

#ifdef DEBUG
    
    Class *clasp = fp->getScopeChain()->getClass();
    if (clasp == &js_WithClass || clasp == &js_BlockClass)
        JS_ASSERT(fp->getScopeChain()->getPrivate() != js_FloatingFrameIfGenerator(cx, fp));
    else if (clasp == &js_CallClass)
        JS_ASSERT(fp->getScopeChain()->getPrivate() != fp);
#endif

    





    JSAtom *lambdaName =
        (fp->getFunction()->flags & JSFUN_LAMBDA) ? fp->getFunction()->atom : NULL;
    if (lambdaName) {
        JSObject *envobj = NewDeclEnvObject(cx, fp);
        if (!envobj)
            return NULL;

        
        fp->setScopeChain(envobj);
        JS_ASSERT(fp->argv);
        if (!js_DefineNativeProperty(cx, fp->getScopeChain(), ATOM_TO_JSID(lambdaName),
                                     fp->calleeValue(),
                                     CalleeGetter, NULL,
                                     JSPROP_PERMANENT | JSPROP_READONLY,
                                     0, 0, NULL)) {
            return NULL;
        }
    }

    JSObject *callobj = NewCallObject(cx, fp->getFunction(), fp->getScopeChain());
    if (!callobj)
        return NULL;

    callobj->setPrivate(fp);
    JS_ASSERT(fp->argv);
    JS_ASSERT(fp->getFunction() == GET_FUNCTION_PRIVATE(cx, fp->callee()));
    callobj->setSlot(JSSLOT_CALLEE, fp->calleeValue());
    fp->setCallObj(callobj);

    



    fp->setScopeChain(callobj);
    return callobj;
}

JSObject * JS_FASTCALL
js_CreateCallObjectOnTrace(JSContext *cx, JSFunction *fun, JSObject *callee, JSObject *scopeChain)
{
    JS_ASSERT(!js_IsNamedLambda(fun));
    JSObject *callobj = NewCallObject(cx, fun, scopeChain);
    if (!callobj)
        return NULL;
    callobj->setSlot(JSSLOT_CALLEE, ObjectValue(*callee));
    return callobj;
}

JS_DEFINE_CALLINFO_4(extern, OBJECT, js_CreateCallObjectOnTrace, CONTEXT, FUNCTION, OBJECT, OBJECT,
                     0, nanojit::ACCSET_STORE_ANY)

JSFunction *
js_GetCallObjectFunction(JSObject *obj)
{
    JS_ASSERT(obj->isCall());
    const Value &v = obj->getSlot(JSSLOT_CALLEE);
    if (v.isUndefined()) {
        
        return NULL;
    }
    JS_ASSERT(v.isObject());
    return GET_FUNCTION_PRIVATE(cx, &v.toObject());
}

inline static void
CopyValuesToCallObject(JSObject *callobj, uintN nargs, Value *argv, uintN nvars, Value *slots)
{
    
    uintN first = JSSLOT_PRIVATE + CALL_CLASS_RESERVED_SLOTS + 1;
    JS_ASSERT(first <= JS_INITIAL_NSLOTS);

    Value *vp = &callobj->fslots[first];
    uintN len = JS_MIN(nargs, JS_INITIAL_NSLOTS - first);

    memcpy(vp, argv, len * sizeof(Value));
    vp += len;

    nargs -= len;
    if (nargs != 0) {
        
        vp = callobj->dslots;
        memcpy(vp, argv + len, nargs * sizeof(Value));
        vp += nargs;
    } else {
        
        first += len;
        len = JS_MIN(nvars, JS_INITIAL_NSLOTS - first);
        memcpy(vp, slots, len * sizeof(Value));
        slots += len;
        nvars -= len;
        vp = callobj->dslots;
    }

    
    memcpy(vp, slots, nvars * sizeof(Value));
}

void
js_PutCallObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *callobj = fp->getCallObj();

    
    if (fp->hasArgsObj()) {
        if (!(fp->flags & JSFRAME_OVERRIDE_ARGS))
            callobj->setSlot(JSSLOT_CALL_ARGUMENTS, ObjectOrNullValue(fp->getArgsObj()));
        js_PutArgsObject(cx, fp);
    }

    JSFunction *fun = fp->getFunction();
    JS_ASSERT(fun == js_GetCallObjectFunction(callobj));
    uintN n = fun->countArgsAndVars();

    



    JS_STATIC_ASSERT(JS_INITIAL_NSLOTS - JSSLOT_PRIVATE ==
                     1 + CALL_CLASS_RESERVED_SLOTS);
    if (n != 0) {
        JS_ASSERT(JSFunction::FIRST_FREE_SLOT + n <= callobj->numSlots());
        CopyValuesToCallObject(callobj, fun->nargs, fp->argv, fun->u.i.nvars, fp->slots());
    }

    
    if (js_IsNamedLambda(fun)) {
        JSObject *env = callobj->getParent();

        JS_ASSERT(env->getClass() == &js_DeclEnvClass);
        JS_ASSERT(env->getPrivate() == fp);
        env->setPrivate(NULL);
    }

    callobj->setPrivate(NULL);
    fp->setCallObj(NULL);
}

JSBool JS_FASTCALL
js_PutCallObjectOnTrace(JSContext *cx, JSObject *scopeChain, uint32 nargs, Value *argv,
                        uint32 nvars, Value *slots)
{
    JS_ASSERT(scopeChain->hasClass(&js_CallClass));
    JS_ASSERT(!scopeChain->getPrivate());

    uintN n = nargs + nvars;
    if (n != 0)
        CopyValuesToCallObject(scopeChain, nargs, argv, nvars, slots);

    return true;
}

JS_DEFINE_CALLINFO_6(extern, BOOL, js_PutCallObjectOnTrace, CONTEXT, OBJECT, UINT32, VALUEPTR,
                     UINT32, VALUEPTR, 0, nanojit::ACCSET_STORE_ANY)

enum JSCallPropertyKind {
    JSCPK_ARGUMENTS,
    JSCPK_ARG,
    JSCPK_VAR,
    JSCPK_UPVAR
};

static JSBool
CallPropertyOp(JSContext *cx, JSObject *obj, jsid id, Value *vp,
               JSCallPropertyKind kind, JSBool setter = false)
{
    JS_ASSERT(obj->isCall());

    uintN i = 0;
    if (kind != JSCPK_ARGUMENTS) {
        JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
        i = (uint16) JSID_TO_INT(id);
    }

    Value *array;
    if (kind == JSCPK_UPVAR) {
        JSObject *callee = &obj->getSlot(JSSLOT_CALLEE).toObject();

#ifdef DEBUG
        JSFunction *callee_fun = (JSFunction *) callee->getPrivate();
        JS_ASSERT(FUN_FLAT_CLOSURE(callee_fun));
        JS_ASSERT(i < callee_fun->u.i.nupvars);
#endif

        array = callee->getFlatClosureUpvars();
    } else {
        JSFunction *fun = js_GetCallObjectFunction(obj);
        JS_ASSERT_IF(kind == JSCPK_ARG, i < fun->nargs);
        JS_ASSERT_IF(kind == JSCPK_VAR, i < fun->u.i.nvars);

        JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();

        if (kind == JSCPK_ARGUMENTS) {
            if (setter) {
                if (fp)
                    fp->flags |= JSFRAME_OVERRIDE_ARGS;
                obj->setSlot(JSSLOT_CALL_ARGUMENTS, *vp);
            } else {
                if (fp && !(fp->flags & JSFRAME_OVERRIDE_ARGS)) {
                    JSObject *argsobj;

                    argsobj = js_GetArgsObject(cx, fp);
                    if (!argsobj)
                        return false;
                    vp->setObject(*argsobj);
                } else {
                    *vp = obj->getSlot(JSSLOT_CALL_ARGUMENTS);
                }
            }
            return true;
        }

        if (!fp) {
            if (kind == JSCPK_VAR)
                i += fun->nargs;
            else
                JS_ASSERT(kind == JSCPK_ARG);

            const uintN first = JSSLOT_PRIVATE + CALL_CLASS_RESERVED_SLOTS + 1;
            JS_ASSERT(first == JSSLOT_FREE(&js_CallClass));
            JS_ASSERT(first <= JS_INITIAL_NSLOTS);

            array = (i < JS_INITIAL_NSLOTS - first) ? obj->fslots : obj->dslots;
        } else if (kind == JSCPK_ARG) {
            array = fp->argv;
        } else {
            JS_ASSERT(kind == JSCPK_VAR);
            array = fp->slots();
        }
    }

    if (setter) {
        GC_POKE(cx, array[i]);
        array[i] = *vp;
    } else {
        *vp = array[i];
    }
    return true;
}

static JSBool
GetCallArguments(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARGUMENTS);
}

static JSBool
SetCallArguments(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARGUMENTS, true);
}

JSBool
js_GetCallArg(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARG);
}

JSBool
SetCallArg(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_ARG, true);
}

JSBool
GetFlatUpvar(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_UPVAR);
}

JSBool
SetFlatUpvar(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_UPVAR, true);
}

JSBool
js_GetCallVar(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_VAR);
}

JSBool
js_GetCallVarChecked(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (!CallPropertyOp(cx, obj, id, vp, JSCPK_VAR))
        return false;

    return CheckForEscapingClosure(cx, obj, vp);
}

JSBool
SetCallVar(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return CallPropertyOp(cx, obj, id, vp, JSCPK_VAR, true);
}

#if JS_TRACER
JSBool JS_FASTCALL
js_SetCallArg(JSContext *cx, JSObject *obj, jsid slotid, ValueArgType arg)
{
    Value argcopy = ValueArgToConstRef(arg);
    return CallPropertyOp(cx, obj, slotid, &argcopy, JSCPK_ARG, true);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_SetCallArg, CONTEXT, OBJECT, JSID, VALUE, 0,
                     nanojit::ACCSET_STORE_ANY)

JSBool JS_FASTCALL
js_SetCallVar(JSContext *cx, JSObject *obj, jsid slotid, ValueArgType arg)
{
    Value argcopy = ValueArgToConstRef(arg);
    return CallPropertyOp(cx, obj, slotid, &argcopy, JSCPK_VAR, true);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_SetCallVar, CONTEXT, OBJECT, JSID, VALUE, 0,
                     nanojit::ACCSET_STORE_ANY)
#endif

static JSBool
call_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
             JSObject **objp)
{
    JS_ASSERT(obj->isCall());
    JS_ASSERT(!obj->getProto());

    if (!JSID_IS_ATOM(id))
        return JS_TRUE;

    const Value &callee = obj->getSlot(JSSLOT_CALLEE);
    if (callee.isUndefined())
        return JS_TRUE;

#ifdef DEBUG
    JSFunction *fun;
    fun = GET_FUNCTION_PRIVATE(cx, &callee.toObject());
    JS_ASSERT(fun->lookupLocal(cx, JSID_TO_ATOM(id), NULL) == JSLOCAL_NONE);
#endif

    



    if (JSID_IS_ATOM(id, cx->runtime->atomState.argumentsAtom)) {
        if (!js_DefineNativeProperty(cx, obj, id, UndefinedValue(),
                                     GetCallArguments, SetCallArguments,
                                     JSPROP_PERMANENT | JSPROP_SHARED,
                                     0, 0, NULL, JSDNP_DONT_PURGE)) {
            return JS_FALSE;
        }
        *objp = obj;
        return JS_TRUE;
    }

    
    return JS_TRUE;
}

static void
call_trace(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->isCall());
    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        






        uintN first = JSSLOT_PRIVATE + CALL_CLASS_RESERVED_SLOTS + 1;
        JS_ASSERT(first <= JS_INITIAL_NSLOTS);

        uintN count = fp->getFunction()->countArgsAndVars();
        uintN fixed = JS_MIN(count, JS_INITIAL_NSLOTS - first);

        SetValueRangeToUndefined(&obj->fslots[first], fixed);
        SetValueRangeToUndefined(obj->dslots, count - fixed);
    }

    MaybeMarkGenerator(trc, obj);
}

JS_PUBLIC_DATA(Class) js_CallClass = {
    "Call",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(CALL_CLASS_RESERVED_SLOTS) |
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_ANONYMOUS | JSCLASS_MARK_IS_TRACE,
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    JS_EnumerateStub,
    (JSResolveOp)call_resolve,
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    JS_CLASS_TRACE(call_trace)
};

bool
JSStackFrame::getValidCalleeObject(JSContext *cx, Value *vp)
{
    if (!hasFunction()) {
        *vp = argv ? argv[-2] : UndefinedValue();
        return true;
    }

    JSFunction *fun = getFunction();

    





    if (fun->needsWrapper()) {
        JSObject *wrapper = WrapEscapingClosure(cx, this, fun);
        if (!wrapper)
            return false;
        vp->setObject(*wrapper);
        return true;
    }

    JSObject *funobj = &calleeObject();
    vp->setObject(*funobj);

    




    if (getThisValue().isObject()) {
        JS_ASSERT(GET_FUNCTION_PRIVATE(cx, funobj) == fun);

        if (fun == funobj && fun->methodAtom()) {
            JSObject *thisp = &getThisValue().toObject();
            JS_ASSERT(thisp->canHaveMethodBarrier());

            if (thisp->hasMethodBarrier()) {
                const Shape *shape = thisp->nativeLookup(ATOM_TO_JSID(fun->methodAtom()));

                











                if (shape) {
                    if (shape->isMethod() && &shape->methodObject() == funobj) {
                        if (!thisp->methodReadBarrier(cx, *shape, vp))
                            return false;
                        setCalleeObject(vp->toObject());
                        return true;
                    }
                    if (shape->hasSlot()) {
                        Value v = thisp->getSlot(shape->slot);
                        JSObject *clone;

                        if (IsFunctionObject(v, &clone) &&
                            GET_FUNCTION_PRIVATE(cx, clone) == fun &&
                            clone->hasMethodObj(*thisp)) {
                            JS_ASSERT(clone != funobj);
                            *vp = v;
                            setCalleeObject(*clone);
                            return true;
                        }
                    }
                }

                







                funobj = CloneFunctionObject(cx, fun, fun->getParent());
                if (!funobj)
                    return false;
                funobj->setMethodObj(*thisp);
                setCalleeObject(*funobj);
                return true;
            }
        }
    }

    return true;
}


enum {
    FUN_ARGUMENTS   = -1,       
    FUN_LENGTH      = -2,       
    FUN_ARITY       = -3,       
    FUN_NAME        = -4,       
    FUN_CALLER      = -5        
};

static JSBool
fun_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (!JSID_IS_INT(id))
        return true;

    jsint slot = JSID_TO_INT(id);

    




















    JSFunction *fun;
    while (!(fun = (JSFunction *)
                   GetInstancePrivate(cx, obj, &js_FunctionClass, NULL))) {
        if (slot != FUN_LENGTH)
            return true;
        obj = obj->getProto();
        if (!obj)
            return true;
    }

    
    JSStackFrame *fp;
    for (fp = js_GetTopStackFrame(cx);
         fp && (fp->maybeFunction() != fun || (fp->flags & JSFRAME_SPECIAL));
         fp = fp->down) {
        continue;
    }

    switch (slot) {
      case FUN_ARGUMENTS:
        
        if (!JS_ReportErrorFlagsAndNumber(cx,
                                          JSREPORT_WARNING | JSREPORT_STRICT,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_DEPRECATED_USAGE,
                                          js_arguments_str)) {
            return false;
        }
        if (fp) {
            if (!js_GetArgsValue(cx, fp, vp))
                return false;
        } else {
            vp->setNull();
        }
        break;

      case FUN_LENGTH:
      case FUN_ARITY:
        vp->setInt32(fun->nargs);
        break;

      case FUN_NAME:
        vp->setString(fun->atom ? ATOM_TO_STRING(fun->atom)
                                : cx->runtime->emptyString);
        break;

      case FUN_CALLER:
        vp->setNull();
        if (fp && fp->down && !fp->down->getValidCalleeObject(cx, vp))
            return false;

        if (vp->isObject()) {
            JSObject &caller = vp->toObject();

            
            if (caller.getCompartment(cx) != cx->compartment) {
                vp->setNull();
            } else if (caller.isFunction() && caller.getFunctionPrivate()->inStrictMode()) {
                JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                             JSMSG_CALLER_IS_STRICT);
                return false;
            }
        }
        break;

      default:
        
        if (fp && fp->hasFunction() && uint16(slot) < fp->getFunction()->nargs)
            *vp = fp->argv[slot];
        break;
    }

    return true;
}

namespace {

struct LazyFunctionDataProp {
    uint16      atomOffset;
    int8        tinyid;
    uint8       attrs;
};

struct PoisonPillProp {
    uint16       atomOffset;
    int8         tinyid;
};



const LazyFunctionDataProp lazyFunctionDataProps[] = {
    {ATOM_OFFSET(arity),     FUN_ARITY,      JSPROP_PERMANENT},
    {ATOM_OFFSET(name),      FUN_NAME,       JSPROP_PERMANENT},
};


const PoisonPillProp poisonPillProps[] = {
    {ATOM_OFFSET(arguments), FUN_ARGUMENTS },
    {ATOM_OFFSET(caller),    FUN_CALLER    },
};

}

static JSBool
fun_enumerate(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isFunction());

    jsid id;
    bool found;

    if (!obj->getFunctionPrivate()->isBound()) {
        id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
        return false;

    for (uintN i = 0; i < JS_ARRAY_LENGTH(lazyFunctionDataProps); i++) {
        const LazyFunctionDataProp &lfp = lazyFunctionDataProps[i];
        id = ATOM_TO_JSID(OFFSET_TO_ATOM(cx->runtime, lfp.atomOffset));
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    for (uintN i = 0; i < JS_ARRAY_LENGTH(poisonPillProps); i++) {
        const PoisonPillProp &p = poisonPillProps[i];
        id = ATOM_TO_JSID(OFFSET_TO_ATOM(cx->runtime, p.atomOffset));
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    return true;
}

static JSBool
fun_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
            JSObject **objp)
{
    if (!JSID_IS_ATOM(id))
        return JS_TRUE;

    JSFunction *fun = obj->getFunctionPrivate();

    




    if ((flags & JSRESOLVE_ASSIGNING) && !JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));
        return JS_TRUE;
    }

    



    JSAtom *atom = cx->runtime->atomState.classPrototypeAtom;
    if (id == ATOM_TO_JSID(atom)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));

        



        if (fun->atom == CLASS_ATOM(cx, Object))
            return JS_TRUE;

        
        if (fun->isBound())
            return JS_TRUE;

        



        JSObject *parent = obj->getParent();
        JSObject *proto;
        if (!js_GetClassPrototype(cx, parent, JSProto_Object, &proto))
            return JS_FALSE;
        proto = NewNativeClassInstance(cx, &js_ObjectClass, proto, parent);
        if (!proto)
            return JS_FALSE;

        






        if (!js_SetClassPrototype(cx, obj, proto, JSPROP_PERMANENT))
            return JS_FALSE;

        *objp = obj;
        return JS_TRUE;
    }

    atom = cx->runtime->atomState.lengthAtom;
    if (id == ATOM_TO_JSID(atom)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));
        if (!js_DefineNativeProperty(cx, obj, ATOM_TO_JSID(atom), Int32Value(fun->nargs),
                                     PropertyStub, PropertyStub,
                                     JSPROP_PERMANENT | JSPROP_READONLY, 0, 0, NULL)) {
            return JS_FALSE;
        }
        *objp = obj;
        return JS_TRUE;
    }

    for (uintN i = 0; i < JS_ARRAY_LENGTH(lazyFunctionDataProps); i++) {
        const LazyFunctionDataProp *lfp = &lazyFunctionDataProps[i];

        atom = OFFSET_TO_ATOM(cx->runtime, lfp->atomOffset);
        if (id == ATOM_TO_JSID(atom)) {
            JS_ASSERT(!IsInternalFunctionObject(obj));

            if (!js_DefineNativeProperty(cx, obj,
                                         ATOM_TO_JSID(atom), UndefinedValue(),
                                         fun_getProperty, PropertyStub,
                                         lfp->attrs, Shape::HAS_SHORTID,
                                         lfp->tinyid, NULL)) {
                return JS_FALSE;
            }
            *objp = obj;
            return JS_TRUE;
        }
    }

    for (uintN i = 0; i < JS_ARRAY_LENGTH(poisonPillProps); i++) {
        const PoisonPillProp &p = poisonPillProps[i];

        atom = OFFSET_TO_ATOM(cx->runtime, p.atomOffset);
        if (id == ATOM_TO_JSID(atom)) {
            JS_ASSERT(!IsInternalFunctionObject(obj));

            PropertyOp getter, setter;
            uintN attrs = JSPROP_PERMANENT;
            if (fun->inStrictMode() || fun->isBound()) {
                JSObject *throwTypeError = obj->getThrowTypeError();

                getter = CastAsPropertyOp(throwTypeError);
                setter = CastAsPropertyOp(throwTypeError);
                attrs |= JSPROP_GETTER | JSPROP_SETTER;
            } else {
                getter = fun_getProperty;
                setter = PropertyStub;
            }

            if (!js_DefineNativeProperty(cx, obj, ATOM_TO_JSID(atom), UndefinedValue(),
                                         getter, setter,
                                         attrs, Shape::HAS_SHORTID,
                                         p.tinyid, NULL)) {
                return JS_FALSE;
            }
            *objp = obj;
            return JS_TRUE;
        }
    }

    return JS_TRUE;
}

#if JS_HAS_XDR


JSBool
js_XDRFunctionObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    JSFunction *fun;
    uint32 firstword;           


    uintN nargs, nvars, nupvars, n;
    uint32 localsword;          
    uint32 flagsword;           

    cx = xdr->cx;
    if (xdr->mode == JSXDR_ENCODE) {
        fun = GET_FUNCTION_PRIVATE(cx, *objp);
        if (!FUN_INTERPRETED(fun)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NOT_SCRIPTED_FUNCTION,
                                 JS_GetFunctionName(fun));
            return false;
        }
        if (fun->u.i.wrapper) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_XDR_CLOSURE_WRAPPER,
                                 JS_GetFunctionName(fun));
            return false;
        }
        JS_ASSERT((fun->u.i.wrapper & ~1U) == 0);
        firstword = (fun->u.i.skipmin << 2) | (fun->u.i.wrapper << 1) | !!fun->atom;
        nargs = fun->nargs;
        nvars = fun->u.i.nvars;
        nupvars = fun->u.i.nupvars;
        localsword = (nargs << 16) | nvars;
        flagsword = (nupvars << 16) | fun->flags;
    } else {
        fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_INTERPRETED, NULL, NULL);
        if (!fun)
            return false;
        FUN_OBJECT(fun)->clearParent();
        FUN_OBJECT(fun)->clearProto();
#ifdef __GNUC__
        nvars = nargs = nupvars = 0;    
#endif
    }

    AutoObjectRooter tvr(cx, FUN_OBJECT(fun));

    if (!JS_XDRUint32(xdr, &firstword))
        return false;
    if ((firstword & 1U) && !js_XDRAtom(xdr, &fun->atom))
        return false;
    if (!JS_XDRUint32(xdr, &localsword) ||
        !JS_XDRUint32(xdr, &flagsword)) {
        return false;
    }

    if (xdr->mode == JSXDR_DECODE) {
        nargs = localsword >> 16;
        nvars = uint16(localsword);
        JS_ASSERT((flagsword & JSFUN_KINDMASK) >= JSFUN_INTERPRETED);
        nupvars = flagsword >> 16;
        fun->flags = uint16(flagsword);
        fun->u.i.skipmin = uint16(firstword >> 2);
        fun->u.i.wrapper = JSPackedBool((firstword >> 1) & 1);
    }

    
    n = nargs + nvars + nupvars;
    if (n != 0) {
        void *mark;
        uintN i;
        uintN bitmapLength;
        uint32 *bitmap;
        jsuword *names;
        JSAtom *name;
        JSLocalKind localKind;

        bool ok = true;
        mark = JS_ARENA_MARK(&xdr->cx->tempPool);

        









        MUST_FLOW_THROUGH("release_mark");
        bitmapLength = JS_HOWMANY(n, JS_BITS_PER_UINT32);
        JS_ARENA_ALLOCATE_CAST(bitmap, uint32 *, &xdr->cx->tempPool,
                               bitmapLength * sizeof *bitmap);
        if (!bitmap) {
            js_ReportOutOfScriptQuota(xdr->cx);
            ok = false;
            goto release_mark;
        }
        if (xdr->mode == JSXDR_ENCODE) {
            names = fun->getLocalNameArray(xdr->cx, &xdr->cx->tempPool);
            if (!names) {
                ok = false;
                goto release_mark;
            }
            PodZero(bitmap, bitmapLength);
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
            ok = !!JS_XDRUint32(xdr, &bitmap[i]);
            if (!ok)
                goto release_mark;
        }
        for (i = 0; i != n; ++i) {
            if (i < nargs &&
                !(bitmap[i >> JS_BITS_PER_UINT32_LOG2] &
                  JS_BIT(i & (JS_BITS_PER_UINT32 - 1)))) {
                if (xdr->mode == JSXDR_DECODE) {
                    ok = !!fun->addLocal(xdr->cx, NULL, JSLOCAL_ARG);
                    if (!ok)
                        goto release_mark;
                } else {
                    JS_ASSERT(!JS_LOCAL_NAME_TO_ATOM(names[i]));
                }
                continue;
            }
            if (xdr->mode == JSXDR_ENCODE)
                name = JS_LOCAL_NAME_TO_ATOM(names[i]);
            ok = !!js_XDRAtom(xdr, &name);
            if (!ok)
                goto release_mark;
            if (xdr->mode == JSXDR_DECODE) {
                localKind = (i < nargs)
                            ? JSLOCAL_ARG
                            : (i < nargs + nvars)
                            ? (bitmap[i >> JS_BITS_PER_UINT32_LOG2] &
                               JS_BIT(i & (JS_BITS_PER_UINT32 - 1))
                               ? JSLOCAL_CONST
                               : JSLOCAL_VAR)
                            : JSLOCAL_UPVAR;
                ok = !!fun->addLocal(xdr->cx, name, localKind);
                if (!ok)
                    goto release_mark;
            }
        }

      release_mark:
        JS_ARENA_RELEASE(&xdr->cx->tempPool, mark);
        if (!ok)
            return false;

        if (xdr->mode == JSXDR_DECODE)
            fun->freezeLocalNames(cx);
    }

    if (!js_XDRScript(xdr, &fun->u.i.script, false, NULL))
        return false;

    if (xdr->mode == JSXDR_DECODE) {
        *objp = FUN_OBJECT(fun);
        if (fun->u.i.script != JSScript::emptyScript()) {
#ifdef CHECK_SCRIPT_OWNER
            fun->u.i.script->owner = NULL;
#endif
            js_CallNewScriptHook(cx, fun->u.i.script, fun);
        }
    }

    return true;
}

#else  

#define js_XDRFunctionObject NULL

#endif 






static JSBool
fun_hasInstance(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp)
{
    while (obj->isFunction()) {
        if (!obj->getFunctionPrivate()->isBound())
            break;
        obj = obj->getBoundFunctionTarget();
    }

    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    Value pval;
    if (!obj->getProperty(cx, id, &pval))
        return JS_FALSE;

    if (pval.isPrimitive()) {
        



        js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, ObjectValue(*obj), NULL);
        return JS_FALSE;
    }

    *bp = js_IsDelegate(cx, &pval.toObject(), *v);
    return JS_TRUE;
}

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    
    JSFunction *fun = (JSFunction *) obj->getPrivate();
    if (!fun)
        return;

    if (fun != obj) {
        
        MarkObject(trc, fun, "private");

        
        if (FUN_FLAT_CLOSURE(fun) && fun->u.i.nupvars)
            MarkValueRange(trc, fun->u.i.nupvars, obj->getFlatClosureUpvars(), "upvars");
        return;
    }

    if (fun->atom)
        MarkString(trc, ATOM_TO_STRING(fun->atom), "atom");

    if (FUN_INTERPRETED(fun)) {
        if (fun->u.i.script)
            js_TraceScript(trc, fun->u.i.script);
        for (const Shape *shape = fun->u.i.names; shape; shape = shape->previous())
            shape->trace(trc);
    }
}

static void
fun_finalize(JSContext *cx, JSObject *obj)
{
    
    JSFunction *fun = (JSFunction *) obj->getPrivate();
    if (!fun)
        return;

    
    if (fun != obj) {
        if (FUN_FLAT_CLOSURE(fun) && fun->u.i.nupvars != 0)
            cx->free((void *) obj->getFlatClosureUpvars());
        return;
    }

    



    if (FUN_INTERPRETED(fun) && fun->u.i.script)
        js_DestroyScript(cx, fun->u.i.script);
}

int
JSFunction::sharpSlotBase(JSContext *cx)
{
#if JS_HAS_SHARP_VARS
    JSAtom *name = js_Atomize(cx, "#array", 6, 0);
    if (name) {
        uintN index = uintN(-1);
#ifdef DEBUG
        JSLocalKind kind =
#endif
            lookupLocal(cx, name, &index);
        JS_ASSERT(kind == JSLOCAL_VAR);
        return int(index);
    }
#endif
    return -1;
}

uint32
JSFunction::countUpvarSlots() const
{
    JS_ASSERT(FUN_INTERPRETED(this));

    return (u.i.nupvars == 0) ? 0 : u.i.script->upvars()->length;
}






JS_PUBLIC_DATA(Class) js_FunctionClass = {
    js_Function_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(JSFunction::CLASS_RESERVED_SLOTS) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    fun_enumerate,
    (JSResolveOp)fun_resolve,
    ConvertStub,
    fun_finalize,
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    js_XDRFunctionObject,
    fun_hasInstance,
    JS_CLASS_TRACE(fun_trace)
};

namespace js {

JSString *
fun_toStringHelper(JSContext *cx, JSObject *obj, uintN indent)
{
    if (!obj->isFunction()) {
        if (obj->isFunctionProxy())
            return JSProxy::fun_toString(cx, obj, indent);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             "object");
        return NULL;
    }

    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
    if (!fun)
        return NULL;
    return JS_DecompileFunction(cx, fun, indent);
}

}  

static JSBool
fun_toString(JSContext *cx, uintN argc, Value *vp)
{
    JS_ASSERT(IsFunctionObject(vp[0]));
    uint32_t indent = 0;

    if (argc != 0 && !ValueToECMAUint32(cx, vp[2], &indent))
        return false;

    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return false;

    JSString *str = fun_toStringHelper(cx, obj, indent);
    if (!str)
        return false;

    vp->setString(str);
    return true;
}

#if JS_HAS_TOSOURCE
static JSBool
fun_toSource(JSContext *cx, uintN argc, Value *vp)
{
    JS_ASSERT(IsFunctionObject(vp[0]));

    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return false;

    JSString *str = fun_toStringHelper(cx, obj, JS_DONT_PRETTY_PRINT);
    if (!str)
        return false;

    vp->setString(str);
    return true;
}
#endif

JSBool
js_fun_call(JSContext *cx, uintN argc, Value *vp)
{
    LeaveTrace(cx);

    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    Value fval = vp[1];

    if (!js_IsCallable(fval)) {
        JSString *str = js_ValueToString(cx, fval);
        if (str) {
            const char *bytes = js_GetStringBytes(cx, str);

            if (bytes) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INCOMPATIBLE_PROTO,
                                     js_Function_str, js_call_str,
                                     bytes);
            }
        }
        return JS_FALSE;
    }

    Value *argv = vp + 2;
    if (argc == 0) {
        
        obj = NULL;
    } else {
        
        if (argv[0].isObject())
            obj = &argv[0].toObject();
        else if (!js_ValueToObjectOrNull(cx, argv[0], &obj))
            return JS_FALSE;
        argc--;
        argv++;
    }

    
    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, argc, args))
        return JS_FALSE;

    
    args.callee() = fval;
    args.thisv().setObjectOrNull(obj);
    memcpy(args.argv(), argv, argc * sizeof *argv);

    bool ok = Invoke(cx, args, 0);
    *vp = args.rval();
    return ok;
}


JSBool
js_fun_apply(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return false;

    
    Value fval = vp[1];
    if (!js_IsCallable(fval)) {
        if (JSString *str = js_ValueToString(cx, fval)) {
            if (const char *bytes = js_GetStringBytes(cx, str)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INCOMPATIBLE_PROTO,
                                     js_Function_str, js_apply_str,
                                     bytes);
            }
        }
        return false;
    }

    
    if (argc < 2 || vp[3].isNullOrUndefined())
        return js_fun_call(cx, (argc > 0) ? 1 : 0, vp);

    
    if (!vp[3].isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS, js_apply_str);
        return false;
    }

    



    JSObject *aobj = vp[3].toObject().wrappedObject(cx);
    jsuint length;
    if (aobj->isArray()) {
        length = aobj->getArrayLength();
    } else if (aobj->isArguments() && !aobj->isArgsLengthOverridden()) {
        length = aobj->getArgsInitialLength();
    } else {
        Value &lenval = vp[0];
        if (!aobj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom), &lenval))
            return false;

        if (lenval.isInt32()) {
            length = jsuint(lenval.toInt32()); 
        } else {
            JS_STATIC_ASSERT(sizeof(jsuint) == sizeof(uint32_t));
            if (!ValueToECMAUint32(cx, lenval, (uint32_t *)&length))
                return false;
        }
    }

    
    if (vp[2].isObject())
        obj = &vp[2].toObject();
    else if (!js_ValueToObjectOrNull(cx, vp[2], &obj))
        return JS_FALSE;

    LeaveTrace(cx);

    
    uintN n = uintN(JS_MIN(length, JS_ARGS_LENGTH_MAX));

    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, n, args))
        return false;

    
    args.callee() = fval;
    args.thisv().setObjectOrNull(obj);

    
    if (aobj && aobj->isArguments() && !aobj->isArgsLengthOverridden()) {
        





        JSStackFrame *fp = (JSStackFrame *) aobj->getPrivate();
        Value *argv = args.argv();
        if (fp) {
            memcpy(argv, fp->argv, n * sizeof(Value));
            for (uintN i = 0; i < n; i++) {
                if (aobj->getArgsElement(i).isMagic(JS_ARGS_HOLE)) 
                    argv[i].setUndefined();
            }
        } else {
            for (uintN i = 0; i < n; i++) {
                argv[i] = aobj->getArgsElement(i);
                if (argv[i].isMagic(JS_ARGS_HOLE))
                    argv[i].setUndefined();
            }
        }
    } else {
        Value *argv = args.argv();
        for (uintN i = 0; i < n; i++) {
            if (!aobj->getProperty(cx, INT_TO_JSID(jsint(i)), &argv[i]))
                return JS_FALSE;
        }
    }

    
    if (!Invoke(cx, args, 0))
        return false;
    *vp = args.rval();
    return true;
}

namespace {
Native
FastNativeToNative(FastNative fn)
{
    return reinterpret_cast<Native>(fn);
}

JSBool
CallOrConstructBoundFunction(JSContext *cx, uintN argc, Value *vp);
}

bool
JSFunction::isBound() const
{
    return isFastNative() && u.n.native == FastNativeToNative(CallOrConstructBoundFunction);
}

inline bool
JSObject::initBoundFunction(JSContext *cx, const Value &thisArg,
                            const Value *args, uintN argslen)
{
    JS_ASSERT(isFunction());
    JS_ASSERT(getFunctionPrivate()->isBound());

    fslots[JSSLOT_BOUND_FUNCTION_THIS] = thisArg;
    fslots[JSSLOT_BOUND_FUNCTION_ARGS_COUNT].setPrivateUint32(argslen);
    if (argslen != 0) {
        
        EmptyShape *empty = EmptyShape::create(cx, clasp);
        if (!empty)
            return false;

        empty->slot += argslen;
        map = empty;

        if (!ensureInstanceReservedSlots(cx, argslen))
            return false;

        JS_ASSERT(dslots);
        JS_ASSERT(dslots[-1].toPrivateUint32() >= argslen);
        memcpy(&dslots[0], args, argslen * sizeof(Value));
    }
    return true;
}

inline JSObject *
JSObject::getBoundFunctionTarget() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(getFunctionPrivate()->isBound());

    
    return getParent();
}

inline const js::Value &
JSObject::getBoundFunctionThis() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(getFunctionPrivate()->isBound());

    return fslots[JSSLOT_BOUND_FUNCTION_THIS];
}

inline const js::Value *
JSObject::getBoundFunctionArguments(uintN &argslen) const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(getFunctionPrivate()->isBound());

    argslen = fslots[JSSLOT_BOUND_FUNCTION_ARGS_COUNT].toPrivateUint32();
    JS_ASSERT_IF(argslen > 0, dslots);
    JS_ASSERT_IF(argslen > 0, dslots[-1].toPrivateUint32() >= argslen);
    return &dslots[0];
}

namespace {


JSBool
CallOrConstructBoundFunction(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = &vp[0].toObject();
    JS_ASSERT(obj->isFunction());
    JS_ASSERT(obj->getFunctionPrivate()->isBound());

    LeaveTrace(cx);

    bool constructing = vp[1].isMagic(JS_FAST_CONSTRUCTOR);

    
    uintN argslen;
    const Value *boundArgs = obj->getBoundFunctionArguments(argslen);

    if (argc + argslen > JS_ARGS_LENGTH_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    
    JSObject *target = obj->getBoundFunctionTarget();

    
    const Value &boundThis = obj->getBoundFunctionThis();

    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, argc + argslen, args))
        return false;

    
    memcpy(args.argv(), boundArgs, argslen * sizeof(Value));
    memcpy(args.argv() + argslen, vp + 2, argc * sizeof(Value));

    
    args.callee().setObject(*target);

    if (!constructing) {
        




        JSObject *boundThisObj;
        if (boundThis.isObjectOrNull()) {
            boundThisObj = boundThis.toObjectOrNull();
        } else {
            if (!js_ValueToObjectOrNull(cx, boundThis, &boundThisObj))
                return false;
        }

        args.thisv() = ObjectOrNullValue(boundThisObj);
    }

    if (constructing ? !InvokeConstructor(cx, args) : !Invoke(cx, args, 0))
        return false;

    *vp = args.rval();
    return true;
}


JSBool
fun_bind(JSContext *cx, uintN argc, Value *vp)
{
    
    JSObject *target = ComputeThisFromVp(cx, vp);
    if (!target)
        return false;

    
    if (!target->wrappedObject(cx)->isCallable()) {
        if (JSString *str = js_ValueToString(cx, vp[1])) {
            if (const char *bytes = js_GetStringBytes(cx, str)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INCOMPATIBLE_PROTO,
                                     js_Function_str, "bind", bytes);
            }
        }
        return false;
    }

    
    Value *args = NULL;
    uintN argslen = 0;
    if (argc > 1) {
        args = vp + 3;
        argslen = argc - 1;
    }

    
    uintN length = 0;
    if (target->isFunction()) {
        uintN nargs = target->getFunctionPrivate()->nargs;
        if (nargs > argslen)
            length = nargs - argslen;
    }

    
    JSAtom *name = target->isFunction() ? target->getFunctionPrivate()->atom : NULL;

    
    JSObject *funobj =
        js_NewFunction(cx, NULL, FastNativeToNative(CallOrConstructBoundFunction), length,
                       JSFUN_FAST_NATIVE | JSFUN_FAST_NATIVE_CTOR, target, name);
    if (!funobj)
        return false;

    
    Value thisArg = argc >= 1 ? vp[2] : UndefinedValue();
    if (!funobj->initBoundFunction(cx, thisArg, args, argslen))
        return false;

    
    

    
    vp->setObject(*funobj);
    return true;
}

}

static JSFunctionSpec function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0),
    JS_FN(js_apply_str,      js_fun_apply,   2,0),
    JS_FN(js_call_str,       js_fun_call,    1,0),
    JS_FN("bind",            fun_bind,       1,0),
    JS_FS_END
};

static JSBool
Function(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
{
    JSFunction *fun;
    JSObject *parent;
    JSStackFrame *fp, *caller;
    uintN i, n, lineno;
    JSAtom *atom;
    const char *filename;
    JSBool ok;
    JSString *str, *arg;
    TokenStream ts(cx);
    JSPrincipals *principals;
    jschar *collected_args, *cp;
    void *mark;
    size_t arg_length, args_length, old_args_length;
    TokenKind tt;

    if (!JS_IsConstructing(cx)) {
        obj = NewFunction(cx, NULL);
        if (!obj)
            return JS_FALSE;
        rval->setObject(*obj);
    } else {
        



        if (obj->getPrivate())
            return JS_TRUE;
    }

    









    parent = argv[-2].toObject().getParent();

    fun = js_NewFunction(cx, obj, NULL, 0, JSFUN_LAMBDA | JSFUN_INTERPRETED,
                         parent, cx->runtime->atomState.anonymousAtom);

    if (!fun)
        return JS_FALSE;

    






    fp = js_GetTopStackFrame(cx);
    JS_ASSERT(!fp->hasScript() && fp->hasFunction() &&
              fp->getFunction()->u.n.native == Function);
    caller = js_GetScriptedCaller(cx, fp);
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

    




    if (!js_CheckContentSecurityPolicy(cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_FUNCTION);
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
            argv[i].setString(arg);

            



            old_args_length = args_length;
            args_length = old_args_length + arg->length();
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
            arg = argv[i].toString();
            arg_length = arg->length();
            (void) js_strncpy(cp, arg->chars(), arg_length);
            cp += arg_length;

            
            *cp++ = (i + 1 < n) ? ',' : 0;
        }

        
        if (!ts.init(collected_args, args_length, NULL, filename, lineno)) {
            JS_ARENA_RELEASE(&cx->tempPool, mark);
            return JS_FALSE;
        }

        
        tt = ts.getToken();
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (tt != TOK_NAME)
                    goto after_args;

                




                atom = ts.currentToken().t_atom;

                
                if (fun->lookupLocal(cx, atom, NULL) != JSLOCAL_NONE) {
                    const char *name;

                    name = js_AtomToPrintableString(cx, atom);
                    ok = name && ReportCompileErrorNumber(cx, &ts, NULL,
                                                          JSREPORT_WARNING | JSREPORT_STRICT,
                                                          JSMSG_DUPLICATE_FORMAL, name);
                    if (!ok)
                        goto after_args;
                }
                if (!fun->addLocal(cx, atom, JSLOCAL_ARG))
                    goto after_args;

                



                tt = ts.getToken();
                if (tt == TOK_EOF)
                    break;
                if (tt != TOK_COMMA)
                    goto after_args;
                tt = ts.getToken();
            }
        }

        state = OK;
      after_args:
        if (state == BAD_FORMAL && !ts.isError()) {
            



            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_FORMAL);
        }
        ts.close();
        JS_ARENA_RELEASE(&cx->tempPool, mark);
        if (state != OK)
            return JS_FALSE;
    }

    if (argc) {
        str = js_ValueToString(cx, argv[argc-1]);
        if (!str)
            return JS_FALSE;
        argv[argc-1].setString(str);
    } else {
        str = cx->runtime->emptyString;
    }

    return Compiler::compileFunctionBody(cx, fun, principals,
                                         str->chars(), str->length(),
                                         filename, lineno);
}

namespace {

JSBool
ThrowTypeError(JSContext *cx, uintN argc, Value *vp)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                 JSMSG_THROW_TYPE_ERROR);
    return false;
}

}

JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto = js_InitClass(cx, obj, NULL, &js_FunctionClass, Function, 1,
                                   NULL, function_methods, NULL, NULL);
    if (!proto)
        return NULL;

    JSFunction *fun = js_NewFunction(cx, proto, NULL, 0, JSFUN_INTERPRETED, obj, NULL);
    if (!fun)
        return NULL;
    fun->u.i.script = JSScript::emptyScript();

    if (obj->getClass()->flags & JSCLASS_IS_GLOBAL) {
        
        JSObject *throwTypeError =
            js_NewFunction(cx, NULL, reinterpret_cast<Native>(ThrowTypeError), 0,
                           JSFUN_FAST_NATIVE, obj, NULL);
        if (!throwTypeError)
            return NULL;

        JS_ALWAYS_TRUE(js_SetReservedSlot(cx, obj, JSRESERVED_GLOBAL_THROWTYPEERROR,
                                          ObjectValue(*throwTypeError)));
    }

    return proto;
}

JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, Native native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom)
{
    JSFunction *fun;

    if (funobj) {
        JS_ASSERT(funobj->isFunction());
        funobj->setParent(parent);
    } else {
        funobj = NewFunction(cx, parent);
        if (!funobj)
            return NULL;
    }
    JS_ASSERT(!funobj->getPrivate());
    fun = (JSFunction *) funobj;

    
    fun->nargs = uint16(nargs);
    fun->flags = flags & (JSFUN_FLAGS_MASK | JSFUN_KINDMASK |
                          JSFUN_TRCINFO | JSFUN_FAST_NATIVE_CTOR);
    if ((flags & JSFUN_KINDMASK) >= JSFUN_INTERPRETED) {
        JS_ASSERT(!native);
        JS_ASSERT(nargs == 0);
        fun->u.i.nvars = 0;
        fun->u.i.nupvars = 0;
        fun->u.i.skipmin = 0;
        fun->u.i.wrapper = false;
        fun->u.i.script = NULL;
        fun->u.i.names = cx->runtime->emptyCallShape;
    } else {
        fun->u.n.extra = 0;
        fun->u.n.spare = 0;
        fun->u.n.clasp = NULL;
        if (flags & JSFUN_TRCINFO) {
#ifdef JS_TRACER
            JSNativeTraceInfo *trcinfo =
                JS_FUNC_TO_DATA_PTR(JSNativeTraceInfo *, native);
            fun->u.n.native = (js::Native) trcinfo->native;
            fun->u.n.trcinfo = trcinfo;
#else
            fun->u.n.trcinfo = NULL;
#endif
        } else {
            fun->u.n.native = native;
            fun->u.n.trcinfo = NULL;
        }
        JS_ASSERT(fun->u.n.native);
    }
    fun->atom = atom;

    
    FUN_OBJECT(fun)->setPrivate(fun);
    return fun;
}

JSObject * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                       JSObject *proto)
{
    JS_ASSERT(parent);
    JS_ASSERT(proto);

    



    JSObject *clone = NewNativeClassInstance(cx, &js_FunctionClass, proto, parent);
    if (!clone)
        return NULL;
    clone->setPrivate(fun);
    return clone;
}

#ifdef JS_TRACER
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_CloneFunctionObject, CONTEXT, FUNCTION, OBJECT, OBJECT, 0,
                     nanojit::ACCSET_STORE_ANY)
#endif






JSObject * JS_FASTCALL
js_AllocFlatClosure(JSContext *cx, JSFunction *fun, JSObject *scopeChain)
{
    JS_ASSERT(FUN_FLAT_CLOSURE(fun));
    JS_ASSERT((fun->u.i.script->upvarsOffset
               ? fun->u.i.script->upvars()->length
               : 0) == fun->u.i.nupvars);

    JSObject *closure = CloneFunctionObject(cx, fun, scopeChain);
    if (!closure)
        return closure;

    uint32 nslots = fun->countUpvarSlots();
    if (nslots == 0)
        return closure;

    Value *upvars = (Value *) cx->malloc(nslots * sizeof(Value));
    if (!upvars)
        return NULL;

    closure->setFlatClosureUpvars(upvars);
    return closure;
}

JS_DEFINE_CALLINFO_3(extern, OBJECT, js_AllocFlatClosure,
                     CONTEXT, FUNCTION, OBJECT, 0, nanojit::ACCSET_STORE_ANY)

JS_REQUIRES_STACK JSObject *
js_NewFlatClosure(JSContext *cx, JSFunction *fun)
{
    



    JSObject *scopeChain = js_GetScopeChain(cx, cx->fp());
    if (!scopeChain)
        return NULL;

    JSObject *closure = js_AllocFlatClosure(cx, fun, scopeChain);
    if (!closure || fun->u.i.nupvars == 0)
        return closure;

    Value *upvars = closure->getFlatClosureUpvars();
    uintN level = fun->u.i.script->staticLevel;
    JSUpvarArray *uva = fun->u.i.script->upvars();

    for (uint32 i = 0, n = uva->length; i < n; i++)
        upvars[i] = GetUpvar(cx, level, uva->vector[i]);

    return closure;
}

JSObject *
js_NewDebuggableFlatClosure(JSContext *cx, JSFunction *fun)
{
    JS_ASSERT(cx->fp()->getFunction()->flags & JSFUN_HEAVYWEIGHT);
    JS_ASSERT(!cx->fp()->getFunction()->optimizedClosure());
    JS_ASSERT(FUN_FLAT_CLOSURE(fun));

    return WrapEscapingClosure(cx, cx->fp(), fun);
}

JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, Native native,
                  uintN nargs, uintN attrs)
{
    PropertyOp gsop;
    JSFunction *fun;

    if (attrs & JSFUN_STUB_GSOPS) {
        





        attrs &= ~JSFUN_STUB_GSOPS;
        gsop = PropertyStub;
    } else {
        gsop = NULL;
    }
    fun = js_NewFunction(cx, NULL, native, nargs,
                         attrs & (JSFUN_FLAGS_MASK | JSFUN_TRCINFO), obj, atom);
    if (!fun)
        return NULL;
    if (!obj->defineProperty(cx, ATOM_TO_JSID(atom), ObjectValue(*fun),
                             gsop, gsop, attrs & ~JSFUN_FLAGS_MASK)) {
        return NULL;
    }
    return fun;
}

#if (JSV2F_CONSTRUCT & JSV2F_SEARCH_STACK)
# error "JSINVOKE_CONSTRUCT and JSV2F_SEARCH_STACK are not disjoint!"
#endif

JSFunction *
js_ValueToFunction(JSContext *cx, const Value *vp, uintN flags)
{
    JSObject *funobj;
    if (!IsFunctionObject(*vp, &funobj)) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }
    return GET_FUNCTION_PRIVATE(cx, funobj);
}

JSObject *
js_ValueToFunctionObject(JSContext *cx, Value *vp, uintN flags)
{
    JSObject *funobj;
    if (!IsFunctionObject(*vp, &funobj)) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }

    return funobj;
}

JSObject *
js_ValueToCallableObject(JSContext *cx, Value *vp, uintN flags)
{
    if (vp->isObject()) {
        JSObject *callable = &vp->toObject();
        if (callable->isCallable())
            return callable;
    }

    js_ReportIsNotFunction(cx, vp, flags);
    return NULL;
}

void
js_ReportIsNotFunction(JSContext *cx, const Value *vp, uintN flags)
{
    const char *name = NULL, *source = NULL;
    AutoValueRooter tvr(cx);
    uintN error = (flags & JSV2F_CONSTRUCT) ? JSMSG_NOT_CONSTRUCTOR : JSMSG_NOT_FUNCTION;
    LeaveTrace(cx);

    









    ptrdiff_t spindex = 0;

    FrameRegsIter i(cx);
    while (!i.done() && !i.pc())
        ++i;

    if (!i.done()) {
        uintN depth = js_ReconstructStackDepth(cx, i.fp()->getScript(), i.pc());
        Value *simsp = i.fp()->base() + depth;
        JS_ASSERT(simsp <= i.sp());
        if (i.fp()->base() <= vp && vp < simsp)
            spindex = vp - simsp;
    }

    if (!spindex)
        spindex = ((flags & JSV2F_SEARCH_STACK) ? JSDVG_SEARCH_STACK : JSDVG_IGNORE_STACK);

    js_ReportValueError3(cx, error, spindex, *vp, NULL, name, source);
}

const Shape *
JSFunction::lastArg() const
{
    const Shape *shape = lastVar();
    if (u.i.nvars != 0) {
        while (shape->previous() && shape->getter() != js_GetCallArg)
            shape = shape->previous();
    }
    return shape;
}

const Shape *
JSFunction::lastVar() const
{
    const Shape *shape = u.i.names;
    if (u.i.nupvars != 0) {
        while (shape->getter() == GetFlatUpvar)
            shape = shape->previous();
    }
    return shape;
}

bool
JSFunction::addLocal(JSContext *cx, JSAtom *atom, JSLocalKind kind)
{
    JS_ASSERT(FUN_INTERPRETED(this));
    JS_ASSERT(!u.i.script);

    




    uintN attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED;
    uint16 *indexp;
    PropertyOp getter, setter;
    uint32 slot = JSSLOT_START(&js_CallClass) + CALL_CLASS_RESERVED_SLOTS;

    if (kind == JSLOCAL_ARG) {
        JS_ASSERT(u.i.nupvars == 0);

        indexp = &nargs;
        getter = js_GetCallArg;
        setter = SetCallArg;
        slot += nargs;
    } else if (kind == JSLOCAL_UPVAR) {
        indexp = &u.i.nupvars;
        getter = GetFlatUpvar;
        setter = SetFlatUpvar;
        slot = SHAPE_INVALID_SLOT;
    } else {
        JS_ASSERT(u.i.nupvars == 0);

        indexp = &u.i.nvars;
        getter = js_GetCallVar;
        setter = SetCallVar;
        if (kind == JSLOCAL_CONST)
            attrs |= JSPROP_READONLY;
        else
            JS_ASSERT(kind == JSLOCAL_VAR);
        slot += nargs + u.i.nvars;
    }

    if (*indexp == JS_BITMASK(16)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             (kind == JSLOCAL_ARG)
                             ? JSMSG_TOO_MANY_FUN_ARGS
                             : JSMSG_TOO_MANY_LOCALS);
        return false;
    }

    Shape **listp = &u.i.names;
    Shape *parent = *listp;
    jsid id;

    







    bool findArgInsertionPoint = false;
    if (!atom) {
        JS_ASSERT(kind == JSLOCAL_ARG);
        if (u.i.nvars != 0) {
            



            if (!parent->inDictionary() && !(parent = Shape::newDictionaryList(cx, listp)))
                return false;
            findArgInsertionPoint = true;
        }
        id = INT_TO_JSID(nargs);
    } else {
        if (kind == JSLOCAL_ARG && parent->inDictionary())
            findArgInsertionPoint = true;
        id = ATOM_TO_JSID(atom);
    }

    if (findArgInsertionPoint) {
        while (parent->parent && parent->getter() != js_GetCallArg) {
            ++parent->slot;
            listp = &parent->parent;
            parent = *listp;
        }
    }

    Shape child(id, getter, setter, slot, attrs, Shape::HAS_SHORTID, *indexp);

    Shape *shape = parent->getChild(cx, child, listp);
    if (!shape)
        return false;

    JS_ASSERT_IF(!shape->inDictionary(), u.i.names == shape);
    ++*indexp;
    return true;
}

JSLocalKind
JSFunction::lookupLocal(JSContext *cx, JSAtom *atom, uintN *indexp)
{
    JS_ASSERT(FUN_INTERPRETED(this));

    Shape *shape = SHAPE_FETCH(Shape::search(&u.i.names, ATOM_TO_JSID(atom)));
    if (shape) {
        JSLocalKind localKind;

        if (shape->getter() == js_GetCallArg)
            localKind = JSLOCAL_ARG;
        else if (shape->getter() == GetFlatUpvar)
            localKind = JSLOCAL_UPVAR;
        else if (!shape->writable())
            localKind = JSLOCAL_CONST;
        else
            localKind = JSLOCAL_VAR;

        if (indexp)
            *indexp = shape->shortid;
        return localKind;
    }
    return JSLOCAL_NONE;
}

jsuword *
JSFunction::getLocalNameArray(JSContext *cx, JSArenaPool *pool)
{
    JS_ASSERT(hasLocalNames());

    uintN n = countLocalNames();
    jsuword *names;

    



    JS_ARENA_ALLOCATE_CAST(names, jsuword *, pool, size_t(n) * sizeof *names);
    if (!names) {
        js_ReportOutOfScriptQuota(cx);
        return NULL;
    }

#ifdef DEBUG
    for (uintN i = 0; i != n; i++)
        names[i] = 0xdeadbeef;
#endif

    for (Shape::Range r = u.i.names; !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
        uintN index = uint16(shape.shortid);
        jsuword constFlag = 0;

        if (shape.getter() == js_GetCallArg) {
            JS_ASSERT(index < nargs);
        } else if (shape.getter() == GetFlatUpvar) {
            JS_ASSERT(index < u.i.nupvars);
            index += nargs + u.i.nvars;
        } else {
            JS_ASSERT(index < u.i.nvars);
            index += nargs;
            if (!shape.writable())
                constFlag = 1;
        }

        JSAtom *atom;
        if (JSID_IS_ATOM(shape.id)) {
            atom = JSID_TO_ATOM(shape.id);
        } else {
            JS_ASSERT(JSID_IS_INT(shape.id));
            JS_ASSERT(shape.getter() == js_GetCallArg);
            atom = NULL;
        }

        names[index] = jsuword(atom);
    }

#ifdef DEBUG
    for (uintN i = 0; i != n; i++)
        JS_ASSERT(names[i] != 0xdeadbeef);
#endif
    return names;
}

void
JSFunction::freezeLocalNames(JSContext *cx)
{
    JS_ASSERT(FUN_INTERPRETED(this));

    Shape *shape = u.i.names;
    if (shape->inDictionary()) {
        do {
            JS_ASSERT(!shape->frozen());
            shape->setFrozen();
        } while ((shape = shape->parent) != NULL);
    }
}






JSAtom *
JSFunction::findDuplicateFormal() const
{
    JS_ASSERT(isInterpreted());

    if (nargs <= 1)
        return NULL;

    for (Shape::Range r = lastArg(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
        for (Shape::Range r2 = shape.previous(); !r2.empty(); r2.popFront()) {
            if (r2.front().id == shape.id)
                return JSID_TO_ATOM(shape.id);
        }
    }
    return NULL;
}
