







































#define __STDC_LIMIT_MACROS




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
#include "jsobjinlines.h"
#include "jscntxtinlines.h"

using namespace js;

static inline void
SetArgsPrivateNative(JSObject *argsobj, ArgsPrivateNative *apn)
{
    JS_ASSERT(argsobj->isArguments());
    uintptr_t p = (uintptr_t) apn;
    argsobj->setPrivate((void*) (p | 2));
}

JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, Value *vp)
{
    JSObject *argsobj;

    if (fp->flags & JSFRAME_OVERRIDE_ARGS) {
        JS_ASSERT(fp->callobj);
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom);
        return fp->callobj->getProperty(cx, id, vp);
    }
    argsobj = js_GetArgsObject(cx, fp);
    if (!argsobj)
        return JS_FALSE;
    vp->setNonFunObj(*argsobj);
    return JS_TRUE;
}

JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, Value *vp)
{
    if (fp->flags & JSFRAME_OVERRIDE_ARGS) {
        JS_ASSERT(fp->callobj);

        jsid argumentsid = ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom);
        Value v;
        if (!fp->callobj->getProperty(cx, argumentsid, &v))
            return false;

        if (v.isPrimitive() && !js_ValueToNonNullObject(cx, v, &v))
            return false;
        return v.asObject().getProperty(cx, id, vp);
    }

    vp->setUndefined();
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        JSObject *argsobj = fp->argsobj;
        if (arg < fp->argc) {
            if (argsobj) {
                if (argsobj->getArgsElement(arg).isMagic(JS_ARGS_HOLE))
                    return argsobj->getProperty(cx, id, vp);
            }
            *vp = fp->argv[arg];
        } else {
            











            if (argsobj)
                return argsobj->getProperty(cx, id, vp);
        }
    } else if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
        JSObject *argsobj = fp->argsobj;
        if (argsobj && argsobj->isArgsLengthOverridden())
            return argsobj->getProperty(cx, id, vp);
        vp->setInt32(fp->argc);
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

    
    argsobj->init(&js_ArgumentsClass, NonFunObjTag(*proto), NonFunObjTag(*parent), NullTag());
    argsobj->setArgsCallee(ObjectOrNullTag(callee));
    argsobj->setArgsLength(argc);

    argsobj->map = cx->runtime->emptyArgumentsScope;
    cx->runtime->emptyArgumentsScope->hold();

    
    if (!js_EnsureReservedSlots(cx, argsobj, argc))
        return NULL;
    return argsobj;
}

static void
PutArguments(JSContext *cx, JSObject *argsobj, Value *args)
{
    uint32 argc = argsobj->getArgsLength();
    for (uint32 i = 0; i != argc; ++i) {
        if (!argsobj->getArgsElement(i).isMagic(JS_ARGS_HOLE))
            argsobj->setArgsElement(i, args[i]);
    }
}

JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp)
{
    



    JS_ASSERT(fp->fun);
    JS_ASSERT_IF(fp->fun->flags & JSFUN_HEAVYWEIGHT,
                 fp->varobj(cx->containingCallStack(fp)));

    
    while (fp->flags & JSFRAME_SPECIAL)
        fp = fp->down;

    
    JSObject *argsobj = fp->argsobj;
    if (argsobj)
        return argsobj;

    










    JSObject *global = fp->scopeChain;
    while (JSObject *parent = global->getParent())
        global = parent;

    JS_ASSERT(fp->argv);
    argsobj = NewArguments(cx, global, fp->argc, &fp->argv[-2].asObject());
    if (!argsobj)
        return argsobj;

    
    argsobj->setPrivate(fp);
    fp->argsobj = argsobj;
    return argsobj;
}

void
js_PutArgsObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *argsobj = fp->argsobj;
    JS_ASSERT(argsobj->getPrivate() == fp);
    PutArguments(cx, argsobj, fp->argv);
    argsobj->setPrivate(NULL);
    fp->argsobj = NULL;
}





#ifdef JS_TRACER
JSObject * JS_FASTCALL
js_Arguments(JSContext *cx, JSObject *parent, uint32 argc, JSObject *callee,
             double *argv, ArgsPrivateNative *apn)
{
    JSObject *argsobj = NewArguments(cx, parent, argc, callee);
    if (!argsobj)
        return NULL;
    apn->argv = argv;
    SetArgsPrivateNative(argsobj, apn);
    return argsobj;
}
#endif

JS_DEFINE_CALLINFO_6(extern, OBJECT, js_Arguments, CONTEXT, OBJECT, UINT32, OBJECT,
                     DOUBLEPTR, APNPTR, 0, nanojit::ACC_STORE_ANY)


JSBool JS_FASTCALL
js_PutArguments(JSContext *cx, JSObject *argsobj, Value *args)
{
    JS_ASSERT(GetArgsPrivateNative(argsobj));
    PutArguments(cx, argsobj, args);
    argsobj->setPrivate(NULL);
    return true;
}

JS_DEFINE_CALLINFO_3(extern, BOOL, js_PutArguments, CONTEXT, OBJECT, JSVALPTR, 0,
                     nanojit::ACC_STORE_ANY)

static JSBool
args_delProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(obj->isArguments());

    if (JSID_IS_INT(id)) {
        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsLength())
            obj->setArgsElement(arg, Value(JS_ARGS_HOLE));
    } else if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
        obj->setArgsLengthOverridden();
    } else if (id == ATOM_TO_JSID(cx->runtime->atomState.calleeAtom)) {
        obj->setArgsCallee(Value(JS_ARGS_HOLE));
    }
    return true;
}

static JS_REQUIRES_STACK JSObject *
WrapEscapingClosure(JSContext *cx, JSStackFrame *fp, JSObject *funobj, JSFunction *fun)
{
    JS_ASSERT(GET_FUNCTION_PRIVATE(cx, funobj) == fun);
    JS_ASSERT(fun->optimizedClosure());
    JS_ASSERT(!fun->u.i.wrapper);

    






    JSObject *scopeChain = js_GetScopeChain(cx, fp);
    if (!scopeChain)
        return NULL;

    JSObject *wfunobj = NewObjectWithGivenProto(cx, &js_FunctionClass,
                                                funobj, scopeChain);
    if (!wfunobj)
        return NULL;
    AutoObjectRooter tvr(cx, wfunobj);

    JSFunction *wfun = (JSFunction *) wfunobj;
    wfunobj->setPrivate(wfun);
    wfun->nargs = 0;
    wfun->flags = fun->flags | JSFUN_HEAVYWEIGHT;
    wfun->u.i.nvars = 0;
    wfun->u.i.nupvars = 0;
    wfun->u.i.skipmin = fun->u.i.skipmin;
    wfun->u.i.wrapper = true;
    wfun->u.i.script = NULL;
    wfun->u.i.names.taggedAtom = NULL;
    wfun->atom = fun->atom;

    if (fun->hasLocalNames()) {
        void *mark = JS_ARENA_MARK(&cx->tempPool);
        jsuword *names = js_GetLocalNameArray(cx, fun, &cx->tempPool);
        if (!names)
            return NULL;

        JSBool ok = true;
        for (uintN i = 0, n = fun->countLocalNames(); i != n; i++) {
            jsuword name = names[i];
            JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(name);
            JSLocalKind localKind = (i < fun->nargs)
                                    ? JSLOCAL_ARG
                                    : (i < fun->countArgsAndVars())
                                    ? (JS_LOCAL_NAME_IS_CONST(name)
                                       ? JSLOCAL_CONST
                                       : JSLOCAL_VAR)
                                    : JSLOCAL_UPVAR;

            ok = js_AddLocal(cx, wfun, atom, localKind);
            if (!ok)
                break;
        }

        JS_ARENA_RELEASE(&cx->tempPool, mark);
        if (!ok)
            return NULL;
        JS_ASSERT(wfun->nargs == fun->nargs);
        JS_ASSERT(wfun->u.i.nvars == fun->u.i.nvars);
        JS_ASSERT(wfun->u.i.nupvars == fun->u.i.nupvars);
        js_FreezeLocalNames(cx, wfun);
    }

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
                                     : 0,
                                     (script->globalsOffset != 0)
                                     ? script->globals()->length
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
    if (script->globalsOffset != 0) {
        memcpy(wscript->globals()->vector, script->globals()->vector,
               wscript->globals()->length * sizeof(GlobalSlotArray::Entry));
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
          case JSOP_GETDSLOT:       *pc = JSOP_GETUPVAR_DBG; break;
          case JSOP_CALLDSLOT:      *pc = JSOP_CALLUPVAR_DBG; break;
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
    if (!InstanceOf(cx, obj, &js_ArgumentsClass, NULL))
        return true;

    if (JSID_IS_INT(id)) {
        



        uintN arg = uintN(JSID_TO_INT(id));
        if (arg < obj->getArgsLength()) {
#ifdef JS_TRACER
            ArgsPrivateNative *argp = GetArgsPrivateNative(obj);
            if (argp) {
                if (NativeToValue(cx, *vp, argp->typemap()[arg], &argp->argv[arg]))
                    return true;
                LeaveTrace(cx);
                return false;
            }
#endif

            JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
            if (fp) {
                *vp = fp->argv[arg];
            } else {
                const Value &v = obj->getArgsElement(arg);
                if (!v.isMagic(JS_ARGS_HOLE))
                    *vp = v;
            }
        }
    } else if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
        if (!obj->isArgsLengthOverridden())
            vp->setInt32(obj->getArgsLength());
    } else {
        JS_ASSERT(id == ATOM_TO_JSID(cx->runtime->atomState.calleeAtom));
        const Value &v = obj->getArgsCallee();
        if (!v.isMagic(JS_ARGS_HOLE)) {
            







            if (GET_FUNCTION_PRIVATE(cx, &v.asObject())->needsWrapper()) {
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
        if (arg < obj->getArgsLength()) {
            JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
            if (fp) {
                fp->argv[arg] = *vp;
                return true;
            }
        }
    } else {
        JS_ASSERT(id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom) ||
                  id == ATOM_TO_JSID(cx->runtime->atomState.calleeAtom));
    }

    





    AutoValueRooter tvr(cx);
    return js_DeleteProperty(cx, obj, id, tvr.addr()) &&
           js_SetProperty(cx, obj, id, vp);
}

static JSBool
args_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
             JSObject **objp)
{
    JS_ASSERT(obj->isArguments());

    *objp = NULL;
    bool valid = false;
    if (JSID_IS_INT(id)) {
        uint32 arg = uint32(JSID_TO_INT(id));
        if (arg < obj->getArgsLength() && !obj->getArgsElement(arg).isMagic(JS_ARGS_HOLE))
            valid = true;
    } else if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
        if (!obj->isArgsLengthOverridden())
            valid = true;
    } else if (id == ATOM_TO_JSID(cx->runtime->atomState.calleeAtom)) {
        if (!obj->getArgsCallee().isMagic(JS_ARGS_HOLE))
            valid = true;
    }

    if (valid) {
        



        Value tmp = UndefinedTag();
        if (!js_DefineProperty(cx, obj, id, &tmp, ArgGetter, ArgSetter, JSPROP_SHARED))
            return JS_FALSE;
        *objp = obj;
    }
    return true;
}

static JSBool
args_enumerate(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isArguments());

    



    int argc = int(obj->getArgsLength());
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

#if JS_HAS_GENERATORS








static void
args_or_call_trace(JSTracer *trc, JSObject *obj)
{
    if (obj->isArguments()) {
        if (GetArgsPrivateNative(obj))
            return;
    } else {
        JS_ASSERT(obj->getClass() == &js_CallClass);
    }

    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp && fp->isFloatingGenerator()) {
        JSObject *obj = js_FloatingFrameToGenerator(fp)->obj;
        JS_CALL_OBJECT_TRACER(trc, obj, "generator object");
    }
}
#else
# define args_or_call_trace NULL
#endif

static uint32
args_reserveSlots(JSContext *cx, JSObject *obj)
{
    return obj->getArgsLength();
}












Class js_ArgumentsClass = {
    js_Object_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::ARGS_FIXED_RESERVED_SLOTS) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,       args_delProperty,
    PropertyStub,       PropertyStub,
    args_enumerate,     (JSResolveOp) args_resolve,
    ConvertStub,        NULL,
    NULL,               NULL,
    NULL,               NULL,
    NULL,               NULL,
    JS_CLASS_TRACE(args_or_call_trace), args_reserveSlots
};

const uint32 JSSLOT_CALLEE =                    JSSLOT_PRIVATE + 1;
const uint32 JSSLOT_CALL_ARGUMENTS =            JSSLOT_PRIVATE + 2;
const uint32 CALL_CLASS_FIXED_RESERVED_SLOTS =  2;





Class js_DeclEnvClass = {
    js_Object_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,     PropertyStub,     PropertyStub,     PropertyStub,
    EnumerateStub,    ResolveStub,      ConvertStub,      NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
CheckForEscapingClosure(JSContext *cx, JSObject *obj, Value *vp)
{
    JS_ASSERT(obj->getClass() == &js_CallClass ||
              obj->getClass() == &js_DeclEnvClass);

    const Value &v = *vp;

    if (v.isFunObj()) {
        JSObject *funobj = &v.asFunObj();
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

        





        if (fun->needsWrapper()) {
            LeaveTrace(cx);

            JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
            if (fp) {
                JSObject *wrapper = WrapEscapingClosure(cx, fp, funobj, fun);
                if (!wrapper)
                    return false;
                vp->setFunObj(*wrapper);
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
    JSObject *callobj = NewObjectWithGivenProto(cx, &js_CallClass, NULL, scopeChain);
    if (!callobj ||
        !js_EnsureReservedSlots(cx, callobj, fun->countArgsAndVars())) {
        return NULL;
    }
    return callobj;
}

JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *callobj;

    
    JS_ASSERT(fp->fun);
    callobj = fp->callobj;
    if (callobj)
        return callobj;

#ifdef DEBUG
    
    Class *classp = fp->scopeChain->getClass();
    if (classp == &js_WithClass || classp == &js_BlockClass)
        JS_ASSERT(fp->scopeChain->getPrivate() != js_FloatingFrameIfGenerator(cx, fp));
    else if (classp == &js_CallClass)
        JS_ASSERT(fp->scopeChain->getPrivate() != fp);
#endif

    





    JSAtom *lambdaName = (fp->fun->flags & JSFUN_LAMBDA) ? fp->fun->atom : NULL;
    if (lambdaName) {
        JSObject *env = NewObjectWithGivenProto(cx, &js_DeclEnvClass, NULL,
                                                fp->scopeChain);
        if (!env)
            return NULL;
        env->setPrivate(fp);

        
        fp->scopeChain = env;
        JS_ASSERT(fp->argv);
        if (!js_DefineNativeProperty(cx, fp->scopeChain, ATOM_TO_JSID(lambdaName),
                                     fp->calleeValue(),
                                     CalleeGetter, NULL,
                                     JSPROP_PERMANENT | JSPROP_READONLY,
                                     0, 0, NULL)) {
            return NULL;
        }
    }

    callobj = NewCallObject(cx, fp->fun, fp->scopeChain);
    if (!callobj)
        return NULL;

    callobj->setPrivate(fp);
    JS_ASSERT(fp->argv);
    JS_ASSERT(fp->fun == GET_FUNCTION_PRIVATE(cx, fp->callee()));
    callobj->setSlot(JSSLOT_CALLEE, fp->calleeValue());
    fp->callobj = callobj;

    



    fp->scopeChain = callobj;
    return callobj;
}

JSObject * JS_FASTCALL
js_CreateCallObjectOnTrace(JSContext *cx, JSFunction *fun, JSObject *callee, JSObject *scopeChain)
{
    JS_ASSERT(!js_IsNamedLambda(fun));
    JSObject *callobj = NewCallObject(cx, fun, scopeChain);
    if (!callobj)
        return NULL;
    callobj->setSlot(JSSLOT_CALLEE, NonFunObjTag(*callee));
    return callobj;
}

JS_DEFINE_CALLINFO_4(extern, OBJECT, js_CreateCallObjectOnTrace, CONTEXT, FUNCTION, OBJECT, OBJECT,
                     0, nanojit::ACC_STORE_ANY)

JSFunction *
js_GetCallObjectFunction(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &js_CallClass);
    const Value &v = obj->getSlot(JSSLOT_CALLEE);
    if (v.isUndefined()) {
        
        return NULL;
    }
    JS_ASSERT(v.isObject());
    return GET_FUNCTION_PRIVATE(cx, &v.asObject());
}

inline static void
CopyValuesToCallObject(JSObject *callobj, int nargs, Value *argv, int nvars, Value *slots)
{
    memcpy(callobj->dslots, argv, nargs * sizeof(Value));
    memcpy(callobj->dslots + nargs, slots, nvars * sizeof(Value));
}

void
js_PutCallObject(JSContext *cx, JSStackFrame *fp)
{
    JSObject *callobj = fp->callobj;
    JS_ASSERT(callobj);

    
    if (fp->argsobj) {
        if (!(fp->flags & JSFRAME_OVERRIDE_ARGS))
            callobj->setSlot(JSSLOT_CALL_ARGUMENTS, NonFunObjTag(*fp->argsobj));
        js_PutArgsObject(cx, fp);
    }

    JSFunction *fun = fp->fun;
    JS_ASSERT(fun == js_GetCallObjectFunction(callobj));
    uintN n = fun->countArgsAndVars();

    



    JS_STATIC_ASSERT(JS_INITIAL_NSLOTS - JSSLOT_PRIVATE ==
                     1 + CALL_CLASS_FIXED_RESERVED_SLOTS);
    if (n != 0) {
        JS_ASSERT(callobj->numSlots() >= JS_INITIAL_NSLOTS + n);
        n += JS_INITIAL_NSLOTS;
        CopyValuesToCallObject(callobj, fun->nargs, fp->argv, fun->u.i.nvars, fp->slots());
    }

    
    if (js_IsNamedLambda(fun)) {
        JSObject *env = callobj->getParent();

        JS_ASSERT(env->getClass() == &js_DeclEnvClass);
        JS_ASSERT(env->getPrivate() == fp);
        env->setPrivate(NULL);
    }

    callobj->setPrivate(NULL);
    fp->callobj = NULL;
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

JS_DEFINE_CALLINFO_6(extern, BOOL, js_PutCallObjectOnTrace, CONTEXT, OBJECT, UINT32, JSVALPTR,
                     UINT32, JSVALPTR, 0, nanojit::ACC_STORE_ANY)

static JSBool
call_enumerate(JSContext *cx, JSObject *obj)
{
    JSFunction *fun;
    uintN n, i;
    void *mark;
    jsuword *names;
    JSBool ok;
    JSAtom *name;
    JSObject *pobj;
    JSProperty *prop;

    fun = js_GetCallObjectFunction(obj);
    n = fun ? fun->countArgsAndVars() : 0;
    if (n == 0)
        return JS_TRUE;

    mark = JS_ARENA_MARK(&cx->tempPool);

    MUST_FLOW_THROUGH("out");
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

        




        JS_ASSERT(prop);
        JS_ASSERT(pobj == obj);
        pobj->dropProperty(cx, prop);
    }
    ok = JS_TRUE;

  out:
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;
}

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
    JS_ASSERT(obj->getClass() == &js_CallClass);

    uintN i = 0;
    if (kind != JSCPK_ARGUMENTS) {
        JS_ASSERT((int16) JSID_TO_INT(id) == JSID_TO_INT(id));
        i = (uint16) JSID_TO_INT(id);
    }

    Value *array;
    if (kind == JSCPK_UPVAR) {
        JSObject *callee = &obj->getSlot(JSSLOT_CALLEE).asObject();

#ifdef DEBUG
        JSFunction *callee_fun = (JSFunction *) callee->getPrivate();
        JS_ASSERT(FUN_FLAT_CLOSURE(callee_fun));
        JS_ASSERT(i < callee_fun->u.i.nupvars);
#endif

        array = callee->dslots;
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
                    vp->setNonFunObj(*argsobj);
                } else {
                    *vp = obj->getSlot(JSSLOT_CALL_ARGUMENTS);
                }
            }
            return true;
        }

        if (!fp) {
            i += CALL_CLASS_FIXED_RESERVED_SLOTS;
            if (kind == JSCPK_VAR)
                i += fun->nargs;
            else
                JS_ASSERT(kind == JSCPK_ARG);
            return setter
                   ? JS_SetReservedSlot(cx, obj, i, Jsvalify(*vp))
                   : JS_GetReservedSlot(cx, obj, i, Jsvalify(vp));
        }

        if (kind == JSCPK_ARG) {
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
js_SetCallArg(JSContext *cx, JSObject *obj, jsid id, jsval v)
{
    return CallPropertyOp(cx, obj, id, &v, JSCPK_ARG, true);
}

JSBool JS_FASTCALL
js_SetCallVar(JSContext *cx, JSObject *obj, jsid id, jsval v)
{
    return CallPropertyOp(cx, obj, id, &v, JSCPK_VAR, true);
}

JS_DEFINE_CALLINFO_4(extern, BOOL, js_SetCallArg, CONTEXT, OBJECT, JSID, JSVAL, 0,
                     nanojit::ACC_STORE_ANY)
JS_DEFINE_CALLINFO_4(extern, BOOL, js_SetCallVar, CONTEXT, OBJECT, JSID, JSVAL, 0,
                     nanojit::ACC_STORE_ANY)
#endif

static JSBool
call_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
             JSObject **objp)
{
    JSFunction *fun;
    JSLocalKind localKind;
    PropertyOp getter, setter;
    uintN slot, attrs;

    JS_ASSERT(obj->getClass() == &js_CallClass);
    JS_ASSERT(!obj->getProto());

    if (!JSID_IS_ATOM(id))
        return JS_TRUE;

    const Value &callee = obj->getSlot(JSSLOT_CALLEE);
    if (callee.isUndefined())
        return JS_TRUE;
    fun = GET_FUNCTION_PRIVATE(cx, &callee.asFunObj());

    










    localKind = js_LookupLocal(cx, fun, JSID_TO_ATOM(id), &slot);
    if (localKind != JSLOCAL_NONE) {
        JS_ASSERT((uint16) slot == slot);

        



        attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED;
        if (localKind == JSLOCAL_ARG) {
            JS_ASSERT(slot < fun->nargs);
            getter = js_GetCallArg;
            setter = SetCallArg;
        } else {
            JSCallPropertyKind cpkind;
            if (localKind == JSLOCAL_UPVAR) {
                if (!FUN_FLAT_CLOSURE(fun))
                    return JS_TRUE;
                getter = GetFlatUpvar;
                setter = SetFlatUpvar;
                cpkind = JSCPK_UPVAR;
            } else {
                JS_ASSERT(localKind == JSLOCAL_VAR || localKind == JSLOCAL_CONST);
                JS_ASSERT(slot < fun->u.i.nvars);
                getter = js_GetCallVar;
                setter = SetCallVar;
                cpkind = JSCPK_VAR;
                if (localKind == JSLOCAL_CONST)
                    attrs |= JSPROP_READONLY;
            }

            




            Value v;
            if (!CallPropertyOp(cx, obj, INT_TO_JSID((int16)slot), &v, cpkind))
                return JS_FALSE;
            if (v.isFunObj() &&
                GET_FUNCTION_PRIVATE(cx, &v.asFunObj())->needsWrapper()) {
                getter = js_GetCallVarChecked;
            }
        }
        if (!js_DefineNativeProperty(cx, obj, id, Value(UndefinedTag()), getter, setter,
                                     attrs, JSScopeProperty::HAS_SHORTID, (int16) slot,
                                     NULL, JSDNP_DONT_PURGE)) {
            return JS_FALSE;
        }
        *objp = obj;
        return JS_TRUE;
    }

    



    if (id == ATOM_TO_JSID(cx->runtime->atomState.argumentsAtom)) {
        if (!js_DefineNativeProperty(cx, obj, id, Value(UndefinedTag()),
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

static uint32
call_reserveSlots(JSContext *cx, JSObject *obj)
{
    JSFunction *fun;

    fun = js_GetCallObjectFunction(obj);
    return fun->countArgsAndVars();
}

JS_FRIEND_DATA(Class) js_CallClass = {
    "Call",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(CALL_CLASS_FIXED_RESERVED_SLOTS) |
    JSCLASS_NEW_RESOLVE | JSCLASS_IS_ANONYMOUS | JSCLASS_MARK_IS_TRACE,
    PropertyStub,       PropertyStub,
    PropertyStub,       PropertyStub,
    call_enumerate,     (JSResolveOp)call_resolve,
    NULL,               NULL,
    NULL,               NULL,
    NULL,               NULL,
    NULL,               NULL,
    JS_CLASS_TRACE(args_or_call_trace), call_reserveSlots
};


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
    jsint slot;
    JSFunction *fun;
    JSStackFrame *fp;
    JSSecurityCallbacks *callbacks;

    if (!JSID_IS_INT(id))
        return JS_TRUE;
    slot = JSID_TO_INT(id);

    



















    while (!(fun = (JSFunction *)
                   GetInstancePrivate(cx, obj, &js_FunctionClass, NULL))) {
        if (slot != FUN_LENGTH)
            return JS_TRUE;
        obj = obj->getProto();
        if (!obj)
            return JS_TRUE;
    }

    
    for (fp = js_GetTopStackFrame(cx);
         fp && (fp->fun != fun || (fp->flags & JSFRAME_SPECIAL));
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
            return JS_FALSE;
        }
        if (fp) {
            if (!js_GetArgsValue(cx, fp, vp))
                return JS_FALSE;
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
        if (fp && fp->down && fp->down->fun) {
            JSFunction *caller = fp->down->fun;
            





            if (caller->needsWrapper()) {
                JSObject *wrapper = WrapEscapingClosure(cx, fp->down, FUN_OBJECT(caller), caller);
                if (!wrapper)
                    return JS_FALSE;
                vp->setFunObj(*wrapper);
                return JS_TRUE;
            }

            JS_ASSERT(fp->down->argv);
            *vp = fp->down->calleeValue();
        } else {
            vp->setNull();
        }
        if (vp->isObject()) {
            callbacks = JS_GetSecurityCallbacks(cx);
            if (callbacks && callbacks->checkObjectAccess) {
                id = ATOM_TO_JSID(cx->runtime->atomState.callerAtom);
                if (!callbacks->checkObjectAccess(cx, obj, id, JSACC_READ, Jsvalify(vp)))
                    return JS_FALSE;
            }
        }
        break;

      default:
        
        if (fp && fp->fun && (uintN)slot < fp->fun->nargs)
            *vp = fp->argv[slot];
        break;
    }

    return JS_TRUE;
}















#define LENGTH_PROP_ATTRS (JSPROP_READONLY|JSPROP_PERMANENT|JSPROP_SHARED)

static JSPropertySpec function_props[] = {
    {js_length_str,    FUN_LENGTH,    LENGTH_PROP_ATTRS, Jsvalify(fun_getProperty), JS_PropertyStub},
    {0,0,0,0,0}
};

typedef struct LazyFunctionProp {
    uint16      atomOffset;
    int8        tinyid;
    uint8       attrs;
} LazyFunctionProp;


static LazyFunctionProp lazy_function_props[] = {
    {ATOM_OFFSET(arguments), FUN_ARGUMENTS, JSPROP_PERMANENT},
    {ATOM_OFFSET(arity),     FUN_ARITY,      JSPROP_PERMANENT},
    {ATOM_OFFSET(caller),    FUN_CALLER,     JSPROP_PERMANENT},
    {ATOM_OFFSET(name),      FUN_NAME,       JSPROP_PERMANENT},
};

static JSBool
fun_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    JSFunction *fun;
    JSAtom *atom;
    uintN i;

    if (!JSID_IS_ATOM(id))
        return JS_TRUE;

    fun = GET_FUNCTION_PRIVATE(cx, obj);

    




    if (flags & JSRESOLVE_ASSIGNING) {
        JS_ASSERT(!IsInternalFunctionObject(obj));
        return JS_TRUE;
    }

    



    atom = cx->runtime->atomState.classPrototypeAtom;
    if (id == ATOM_TO_JSID(atom)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));

        



        if (fun->atom == CLASS_ATOM(cx, Object))
            return JS_TRUE;

        



        JSObject *proto = NewObject(cx, &js_ObjectClass, NULL, obj->getParent());
        if (!proto)
            return JS_FALSE;

        






        if (!js_SetClassPrototype(cx, obj, proto, JSPROP_PERMANENT))
            return JS_FALSE;

        *objp = obj;
        return JS_TRUE;
    }

    for (i = 0; i < JS_ARRAY_LENGTH(lazy_function_props); i++) {
        LazyFunctionProp *lfp = &lazy_function_props[i];

        atom = OFFSET_TO_ATOM(cx->runtime, lfp->atomOffset);
        if (id == ATOM_TO_JSID(atom)) {
            JS_ASSERT(!IsInternalFunctionObject(obj));

            if (!js_DefineNativeProperty(cx, obj,
                                         ATOM_TO_JSID(atom), Value(UndefinedTag()),
                                         fun_getProperty, PropertyStub,
                                         lfp->attrs, JSScopeProperty::HAS_SHORTID,
                                         lfp->tinyid, NULL)) {
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
    if ((firstword & 1U) && !js_XDRStringAtom(xdr, &fun->atom))
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
            names = js_GetLocalNameArray(xdr->cx, fun, &xdr->cx->tempPool);
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
                    ok = !!js_AddLocal(xdr->cx, fun, NULL, JSLOCAL_ARG);
                    if (!ok)
                        goto release_mark;
                } else {
                    JS_ASSERT(!JS_LOCAL_NAME_TO_ATOM(names[i]));
                }
                continue;
            }
            if (xdr->mode == JSXDR_ENCODE)
                name = JS_LOCAL_NAME_TO_ATOM(names[i]);
            ok = !!js_XDRStringAtom(xdr, &name);
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
                ok = !!js_AddLocal(xdr->cx, fun, name, localKind);
                if (!ok)
                    goto release_mark;
            }
        }

      release_mark:
        JS_ARENA_RELEASE(&xdr->cx->tempPool, mark);
        if (!ok)
            return false;

        if (xdr->mode == JSXDR_DECODE)
            js_FreezeLocalNames(cx, fun);
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
fun_hasInstance(JSContext *cx, JSObject *obj, Value v, JSBool *bp)
{
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    Value pval;
    if (!obj->getProperty(cx, id, &pval))
        return JS_FALSE;

    if (pval.isPrimitive()) {
        



        js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, ObjectTag(*obj), NULL);
        return JS_FALSE;
    }

    *bp = js_IsDelegate(cx, &pval.asObject(), v);
    return JS_TRUE;
}

static void
TraceLocalNames(JSTracer *trc, JSFunction *fun);

static void
DestroyLocalNames(JSContext *cx, JSFunction *fun);

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    
    JSFunction *fun = (JSFunction *) obj->getPrivate();
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
    
    JSFunction *fun = (JSFunction *) obj->getPrivate();
    if (!fun || FUN_OBJECT(fun) != obj)
        return;

    



    if (FUN_INTERPRETED(fun)) {
        if (fun->u.i.script)
            js_DestroyScript(cx, fun->u.i.script);
        DestroyLocalNames(cx, fun);
    }
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
            js_LookupLocal(cx, this, name, &index);
        JS_ASSERT(kind == JSLOCAL_VAR);
        return int(index);
    }
#endif
    return -1;
}

uint32
JSFunction::countInterpretedReservedSlots() const
{
    JS_ASSERT(FUN_INTERPRETED(this));

    return (u.i.nupvars == 0) ? 0 : u.i.script->upvars()->length;
}

static uint32
fun_reserveSlots(JSContext *cx, JSObject *obj)
{
    




    JSFunction *fun = (JSFunction *) obj->getPrivate();
    return (fun && FUN_INTERPRETED(fun))
           ? fun->countInterpretedReservedSlots()
           : 0;
}






JS_FRIEND_DATA(Class) js_FunctionClass = {
    js_Function_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_HAS_RESERVED_SLOTS(2) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    PropertyStub,     PropertyStub,
    PropertyStub,     PropertyStub,
    EnumerateStub,    (JSResolveOp)fun_resolve,
    ConvertStub,      fun_finalize,
    NULL,             NULL,
    NULL,             NULL,
    js_XDRFunctionObject, fun_hasInstance,
    JS_CLASS_TRACE(fun_trace), fun_reserveSlots
};

static JSBool
fun_toStringHelper(JSContext *cx, uint32_t indent, uintN argc, Value *vp)
{
    if (!ComputeThisFromVpInPlace(cx, vp))
        return JS_FALSE;
    Value fval = vp[1];

    if (!fval.isFunObj()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             JS_GetTypeName(cx, TypeOfValue(cx, fval)));
        return JS_FALSE;
    }

    JSObject *obj = &fval.asFunObj();
    if (argc != 0) {
        if (!ValueToECMAUint32(cx, vp[2], &indent))
            return JS_FALSE;
    }

    JS_ASSERT(JS_ObjectIsFunction(cx, obj));
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
    if (!fun)
        return JS_TRUE;
    JSString *str = JS_DecompileFunction(cx, fun, (uintN)indent);
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;
}

static JSBool
fun_toString(JSContext *cx, uintN argc, Value *vp)
{
    return fun_toStringHelper(cx, 0, argc,  vp);
}

#if JS_HAS_TOSOURCE
static JSBool
fun_toSource(JSContext *cx, uintN argc, Value *vp)
{
    return fun_toStringHelper(cx, JS_DONT_PRETTY_PRINT, argc, vp);
}
#endif

JSBool
js_fun_call(JSContext *cx, uintN argc, Value *vp)
{
    LeaveTrace(cx);

    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
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
    Value objv;
    if (argc == 0) {
        
        objv.setNull();
    } else {
        
        if (argv[0].isObject())
            objv = argv[0];
        else if (!js_ValueToObjectOrNull(cx, argv[0], &objv))
            return JS_FALSE;
        argc--;
        argv++;
    }

    
    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, argc, args))
        return JS_FALSE;

    
    args.getvp()[0] = fval;
    args.getvp()[1] = objv;
    memcpy(args.getvp() + 2, argv, argc * sizeof *argv);

    bool ok = Invoke(cx, args, 0);
    *vp = *args.getvp();
    return ok;
}

JSBool
js_fun_apply(JSContext *cx, uintN argc, Value *vp)
{
    if (argc == 0) {
        
        return js_fun_call(cx, argc, vp);
    }

    LeaveTrace(cx);

    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    Value fval;
    fval = vp[1];

    if (!js_IsCallable(fval)) {
        JSString *str = js_ValueToString(cx, fval);
        if (str) {
            const char *bytes = js_GetStringBytes(cx, str);

            if (bytes) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INCOMPATIBLE_PROTO,
                                     js_Function_str, js_apply_str,
                                     bytes);
            }
        }
        return JS_FALSE;
    }

    
    JSObject *aobj = NULL;
    jsuint length = 0;

    if (argc >= 2) {
        
        if (vp[3].isNullOrUndefined()) {
            argc = 0;
        } else {
            
            JSBool arraylike = JS_FALSE;
            if (vp[3].isObject()) {
                aobj = &vp[3].asObject();
                if (!js_IsArrayLike(cx, aobj, &arraylike, &length))
                    return JS_FALSE;
            }
            if (!arraylike) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_APPLY_ARGS, js_apply_str);
                return JS_FALSE;
            }
        }
    }

    
    Value objv;
    if (vp[2].isObject())
        objv = vp[2];
    else if (!js_ValueToObjectOrNull(cx, vp[2], &objv))
        return JS_FALSE;

    
    argc = (uintN)JS_MIN(length, JS_ARGS_LENGTH_MAX);

    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, argc, args))
        return JS_FALSE;

    
    Value *sp = args.getvp();
    *sp++ = fval;
    *sp++ = objv;
    if (aobj && aobj->isArguments() && !aobj->isArgsLengthOverridden()) {
        





        JSStackFrame *fp = (JSStackFrame *) aobj->getPrivate();
        if (fp) {
            memcpy(sp, fp->argv, argc * sizeof(Value));
            for (uintN i = 0; i < argc; i++) {
                if (aobj->getArgsElement(i).isMagic(JS_ARGS_HOLE)) 
                    sp[i].setUndefined();
            }
        } else {
            for (uintN i = 0; i < argc; i++) {
                sp[i] = aobj->getArgsElement(i);
                if (sp[i].isMagic(JS_ARGS_HOLE))
                    sp[i].setUndefined();
            }
        }
    } else {
        for (uintN i = 0; i < argc; i++) {
            if (!aobj->getProperty(cx, INT_TO_JSID(jsint(i)), sp))
                return JS_FALSE;
            sp++;
        }
    }

    bool ok = Invoke(cx, args, 0);
    *vp = *args.getvp();
    return ok;
}

#ifdef NARCISSUS
static JS_REQUIRES_STACK JSBool
fun_applyConstructor(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *aobj;
    uintN length, i;
    jsval *sp;

    if (JSVAL_IS_PRIMITIVE(vp[2]) ||
        (aobj = JSVAL_TO_OBJECT(vp[2]),
         !aobj->isArray() &&
         !aobj->isArguments())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_APPLY_ARGS, "__applyConstruct__");
        return JS_FALSE;
    }

    if (!js_GetLengthProperty(cx, aobj, &length))
        return JS_FALSE;

    if (length > JS_ARGS_LENGTH_MAX)
        length = JS_ARGS_LENGTH_MAX;
        return JS_FALSE;

    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, length, args))
        return JS_FALSE;

    jsval *sp = args.getvp();
    *sp++ = vp[1];
    *sp++ = JSVAL_NULL; 
    for (i = 0; i < length; i++) {
        if (!aobj->getProperty(cx, INT_TO_JSID(jsint(i)), sp))
            return JS_FALSE;
        sp++;
    }

    JSBool ok = js_InvokeConstructor(cx, args, JS_TRUE);
    *vp = *args.getvp();
    return ok;
}
#endif

static JSFunctionSpec function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0),
    JS_FN(js_apply_str,      js_fun_apply,   2,0),
    JS_FN(js_call_str,       js_fun_call,    1,0),
#ifdef NARCISSUS
    JS_FN("__applyConstructor__", fun_applyConstructor, 1,0),
#endif
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
        obj = NewObject(cx, &js_FunctionClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;
        rval->setFunObj(*obj);
    } else {
        



        if (obj->getPrivate())
            return JS_TRUE;
    }

    









    parent = argv[-2].asObject().getParent();

    fun = js_NewFunction(cx, obj, NULL, 0, JSFUN_LAMBDA | JSFUN_INTERPRETED,
                         parent, cx->runtime->atomState.anonymousAtom);

    if (!fun)
        return JS_FALSE;

    






    fp = js_GetTopStackFrame(cx);
    JS_ASSERT(!fp->script && fp->fun && fp->fun->u.n.native == Function);
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
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, 
                             JSMSG_CSP_BLOCKED_FUNCTION);
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
            arg = argv[i].asString();
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

                
                if (js_LookupLocal(cx, fun, atom, NULL) != JSLOCAL_NONE) {
                    const char *name;

                    name = js_AtomToPrintableString(cx, atom);
                    ok = name && ReportCompileErrorNumber(cx, &ts, NULL,
                                                          JSREPORT_WARNING | JSREPORT_STRICT,
                                                          JSMSG_DUPLICATE_FORMAL, name);
                    if (!ok)
                        goto after_args;
                }
                if (!js_AddLocal(cx, fun, atom, JSLOCAL_ARG))
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

JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;
    JSFunction *fun;

    proto = js_InitClass(cx, obj, NULL, &js_FunctionClass, Function, 1,
                         function_props, function_methods, NULL, NULL);
    if (!proto)
        return NULL;
    fun = js_NewFunction(cx, proto, NULL, 0, JSFUN_INTERPRETED, obj, NULL);
    if (!fun)
        return NULL;
    fun->u.i.script = JSScript::emptyScript();
    return proto;
}

JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, Native native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom)
{
    JSFunction *fun;

    if (funobj) {
        JS_ASSERT(funobj->isFunction());
        funobj->setParent(ObjectOrNullTag(parent));
    } else {
        funobj = NewObject(cx, &js_FunctionClass, NULL, parent);
        if (!funobj)
            return NULL;
    }
    JS_ASSERT(!funobj->getPrivate());
    fun = (JSFunction *) funobj;

    
    fun->nargs = uint16(nargs);
    fun->flags = flags & (JSFUN_FLAGS_MASK | JSFUN_KINDMASK | JSFUN_TRCINFO);
    if ((flags & JSFUN_KINDMASK) >= JSFUN_INTERPRETED) {
        JS_ASSERT(!native);
        JS_ASSERT(nargs == 0);
        fun->u.i.nvars = 0;
        fun->u.i.nupvars = 0;
        fun->u.i.skipmin = 0;
        fun->u.i.wrapper = false;
        fun->u.i.script = NULL;
#ifdef DEBUG
        fun->u.i.names.taggedAtom = 0;
#endif
    } else {
        fun->u.n.extra = 0;
        fun->u.n.spare = 0;
        fun->u.n.clasp = NULL;
        if (flags & JSFUN_TRCINFO) {
#ifdef JS_TRACER
            JSNativeTraceInfo *trcinfo =
                JS_FUNC_TO_DATA_PTR(JSNativeTraceInfo *, native);
            fun->u.n.native = (JSNative) trcinfo->native;
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

    



    JSObject *clone = NewObjectWithGivenProto(cx, &js_FunctionClass, proto,
                                              parent, sizeof(JSObject));
    if (!clone)
        return NULL;
    clone->setPrivate(fun);
    return clone;
}

#ifdef JS_TRACER
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_CloneFunctionObject, CONTEXT, FUNCTION, OBJECT, OBJECT, 0,
                     nanojit::ACC_STORE_ANY)
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

    uint32 nslots = fun->countInterpretedReservedSlots();
    if (!nslots)
        return closure;
    if (!js_EnsureReservedSlots(cx, closure, nslots))
        return NULL;

    return closure;
}

JS_DEFINE_CALLINFO_3(extern, OBJECT, js_AllocFlatClosure,
                     CONTEXT, FUNCTION, OBJECT, 0, nanojit::ACC_STORE_ANY)

JS_REQUIRES_STACK JSObject *
js_NewFlatClosure(JSContext *cx, JSFunction *fun)
{
    



    JSObject *scopeChain = js_GetScopeChain(cx, cx->fp);
    if (!scopeChain)
        return NULL;

    JSObject *closure = js_AllocFlatClosure(cx, fun, scopeChain);
    if (!closure || fun->u.i.nupvars == 0)
        return closure;

    JSUpvarArray *uva = fun->u.i.script->upvars();
    JS_ASSERT(uva->length <= closure->dslots[-1].asPrivateUint32());

    uintN level = fun->u.i.script->staticLevel;
    for (uint32 i = 0, n = uva->length; i < n; i++)
        closure->dslots[i] = js_GetUpvar(cx, level, uva->vector[i]);

    return closure;
}

JSObject *
js_NewDebuggableFlatClosure(JSContext *cx, JSFunction *fun)
{
    JS_ASSERT(cx->fp->fun->flags & JSFUN_HEAVYWEIGHT);
    JS_ASSERT(!cx->fp->fun->optimizedClosure());
    JS_ASSERT(FUN_FLAT_CLOSURE(fun));

    return WrapEscapingClosure(cx, cx->fp, FUN_OBJECT(fun), fun);
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
    fun = js_NewFunction(cx, NULL, native, nargs, attrs, obj, atom);
    if (!fun)
        return NULL;
    if (!obj->defineProperty(cx, ATOM_TO_JSID(atom), fun->funObjVal(),
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
    if (!vp->isFunObj()) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }
    return GET_FUNCTION_PRIVATE(cx, &vp->asObject());
}

JSObject *
js_ValueToFunctionObject(JSContext *cx, Value *vp, uintN flags)
{
    JSFunction *fun;
    JSStackFrame *caller;
    JSPrincipals *principals;

    if (vp->isFunObj())
        return &vp->asFunObj();

    fun = js_ValueToFunction(cx, vp, flags);
    if (!fun)
        return NULL;
    vp->setFunObj(*FUN_OBJECT(fun));

    caller = js_GetScriptedCaller(cx, NULL);
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
js_ValueToCallableObject(JSContext *cx, Value *vp, uintN flags)
{
    if (vp->isObject()) {
        JSObject *callable = &vp->asObject();
        if (callable->isCallable())
            return callable;
    }
    return js_ValueToFunctionObject(cx, vp, flags);
}

void
js_ReportIsNotFunction(JSContext *cx, const Value *vp, uintN flags)
{
    uintN error;
    const char *name = NULL, *source = NULL;
    AutoStringRooter tvr(cx);
    if (flags & JSV2F_ITERATOR) {
        error = JSMSG_BAD_ITERATOR;
        name = js_iterator_str;
        JSString *src = js_ValueToSource(cx, *vp);
        if (!src)
            return;
        tvr.setString(src);
        JSString *qsrc = js_QuoteString(cx, src, 0);
        if (!qsrc)
            return;
        tvr.setString(qsrc);
        source = js_GetStringBytes(cx, qsrc);
        if (!source)
            return;
    } else if (flags & JSV2F_CONSTRUCT) {
        error = JSMSG_NOT_CONSTRUCTOR;
    } else {
        error = JSMSG_NOT_FUNCTION;
    }

    LeaveTrace(cx);
    FrameRegsIter i(cx);
    while (!i.done() && !i.pc())
        ++i;

    ptrdiff_t spindex =
        !i.done() && i.fp()->base() <= vp && vp < i.sp()
            ? vp - i.sp()
            : flags & JSV2F_SEARCH_STACK ? JSDVG_SEARCH_STACK
                                         : JSDVG_IGNORE_STACK;

    js_ReportValueError3(cx, error, spindex, *vp, NULL, name, source);
}





#define MAX_ARRAY_LOCALS 8

JS_STATIC_ASSERT(2 <= MAX_ARRAY_LOCALS);
JS_STATIC_ASSERT(MAX_ARRAY_LOCALS < JS_BITMASK(16));






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
        cx->free(dup);
    }
    JS_DHashTableFinish(&map->names);
    cx->free(map);
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
    entry = (JSLocalNameHashEntry *)
            JS_DHashTableOperate(&map->names, name, JS_DHASH_ADD);
    if (!entry) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    if (entry->name) {
        JS_ASSERT(entry->name == name);
        JS_ASSERT(entry->localKind == JSLOCAL_ARG && localKind == JSLOCAL_ARG);
        dup = (JSNameIndexPair *) cx->malloc(sizeof *dup);
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
    } else if (kind == JSLOCAL_UPVAR) {
        indexp = &fun->u.i.nupvars;
    } else {
        indexp = &fun->u.i.nvars;
        if (kind == JSLOCAL_CONST)
            taggedAtom |= 1;
        else
            JS_ASSERT(kind == JSLOCAL_VAR);
    }
    n = fun->countLocalNames();
    if (n == 0) {
        JS_ASSERT(fun->u.i.names.taggedAtom == 0);
        fun->u.i.names.taggedAtom = taggedAtom;
    } else if (n < MAX_ARRAY_LOCALS) {
        if (n > 1) {
            array = fun->u.i.names.array;
        } else {
            array = (jsuword *) cx->malloc(MAX_ARRAY_LOCALS * sizeof *array);
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
        map = (JSLocalNameMap *) cx->malloc(sizeof *map);
        if (!map)
            return JS_FALSE;
        if (!JS_DHashTableInit(&map->names, JS_DHashGetStubOps(),
                               NULL, sizeof(JSLocalNameHashEntry),
                               JS_DHASH_DEFAULT_CAPACITY(MAX_ARRAY_LOCALS
                                                         * 2))) {
            JS_ReportOutOfMemory(cx);
            cx->free(map);
            return JS_FALSE;
        }

        map->lastdup = NULL;
        for (i = 0; i != MAX_ARRAY_LOCALS; ++i) {
            taggedAtom = array[i];
            uintN j = i;
            JSLocalKind k = JSLOCAL_ARG;
            if (j >= fun->nargs) {
                j -= fun->nargs;
                if (j < fun->u.i.nvars) {
                    k = (taggedAtom & 1) ? JSLOCAL_CONST : JSLOCAL_VAR;
                } else {
                    j -= fun->u.i.nvars;
                    k = JSLOCAL_UPVAR;
                }
            }
            if (!HashLocalName(cx, map, (JSAtom *) (taggedAtom & ~1), k, j)) {
                FreeLocalNameHash(cx, map);
                return JS_FALSE;
            }
        }
        if (!HashLocalName(cx, map, atom, kind, *indexp)) {
            FreeLocalNameHash(cx, map);
            return JS_FALSE;
        }

        



        fun->u.i.names.map = map;
        cx->free(array);
    } else {
        if (*indexp == JS_BITMASK(16)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 (kind == JSLOCAL_ARG)
                                 ? JSMSG_TOO_MANY_FUN_ARGS
                                 : JSMSG_TOO_MANY_LOCALS);
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
    uintN n, i, upvar_base;
    jsuword *array;
    JSLocalNameHashEntry *entry;

    JS_ASSERT(FUN_INTERPRETED(fun));
    n = fun->countLocalNames();
    if (n == 0)
        return JSLOCAL_NONE;
    if (n <= MAX_ARRAY_LOCALS) {
        array = (n == 1) ? &fun->u.i.names.taggedAtom : fun->u.i.names.array;

        
        i = n;
        upvar_base = fun->countArgsAndVars();
        do {
            --i;
            if (atom == JS_LOCAL_NAME_TO_ATOM(array[i])) {
                if (i < fun->nargs) {
                    if (indexp)
                        *indexp = i;
                    return JSLOCAL_ARG;
                }
                if (i >= upvar_base) {
                    if (indexp)
                        *indexp = i - upvar_base;
                    return JSLOCAL_UPVAR;
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

static JSDHashOperator
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
                  entry->localKind == JSLOCAL_CONST ||
                  entry->localKind == JSLOCAL_UPVAR);
        JS_ASSERT(entry->index < args->fun->u.i.nvars + args->fun->u.i.nupvars);
        JS_ASSERT(args->nCopiedVars++ < unsigned(args->fun->u.i.nvars + args->fun->u.i.nupvars));
        i = args->fun->nargs;
        if (entry->localKind == JSLOCAL_UPVAR)
           i += args->fun->u.i.nvars;
        i += entry->index;
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

    JS_ASSERT(fun->hasLocalNames());
    n = fun->countLocalNames();

    if (n <= MAX_ARRAY_LOCALS)
        return (n == 1) ? &fun->u.i.names.taggedAtom : fun->u.i.names.array;

    



    JS_ARENA_ALLOCATE_CAST(names, jsuword *, pool, (size_t) n * sizeof *names);
    if (!names) {
        js_ReportOutOfScriptQuota(cx);
        return NULL;
    }

#if JS_HAS_DESTRUCTURING
    
    PodZero(names, fun->nargs);
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
    JS_ASSERT(args.nCopiedVars == fun->u.i.nvars + fun->u.i.nupvars);

    return names;
}

static JSDHashOperator
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
    MarkRaw(trc, ATOM_TO_STRING(entry->name), JSTRACE_STRING);
    return JS_DHASH_NEXT;
}

static void
TraceLocalNames(JSTracer *trc, JSFunction *fun)
{
    uintN n, i;
    JSAtom *atom;
    jsuword *array;

    JS_ASSERT(FUN_INTERPRETED(fun));
    n = fun->countLocalNames();
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
                MarkRaw(trc, ATOM_TO_STRING(atom), JSTRACE_STRING);
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

    n = fun->countLocalNames();
    if (n <= 1)
        return;
    if (n <= MAX_ARRAY_LOCALS)
        cx->free(fun->u.i.names.array);
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
    n = fun->nargs + fun->u.i.nvars + fun->u.i.nupvars;
    if (2 <= n && n < MAX_ARRAY_LOCALS) {
        
        array = (jsuword *) cx->realloc(fun->u.i.names.array,
                                        n * sizeof *array);
        if (array)
            fun->u.i.names.array = array;
    }
#ifdef DEBUG
    if (n > MAX_ARRAY_LOCALS)
        JS_DHashMarkTableImmutable(&fun->u.i.names.map->names);
#endif
}

JSAtom *
JSFunction::findDuplicateFormal() const
{
    if (nargs <= 1)
        return NULL;

    
    unsigned n = nargs + u.i.nvars + u.i.nupvars;
    if (n <= MAX_ARRAY_LOCALS) {
        jsuword *array = u.i.names.array;

        
        for (unsigned i = 0; i < nargs; i++) {
            for (unsigned j = i + 1; j < nargs; j++) {
                if (array[i] == array[j])
                    return JS_LOCAL_NAME_TO_ATOM(array[i]);
            }
        }
        return NULL;
    }

    




    JSNameIndexPair *dup = u.i.names.map->lastdup;
    return dup ? dup->name : NULL;
}
