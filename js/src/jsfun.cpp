










































#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jspropertytree.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/BytecodeEmitter.h"
#include "frontend/TokenStream.h"
#include "vm/Debugger.h"
#include "vm/MethodGuard.h"
#include "vm/ScopeObject.h"
#include "vm/Xdr.h"

#if JS_HAS_GENERATORS
# include "jsiter.h"
#endif

#ifdef JS_METHODJIT
#include "methodjit/MethodJIT.h"
#endif

#include "jsatominlines.h"
#include "jsfuninlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "vm/ArgumentsObject-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/Stack-inl.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;
using namespace js::types;

bool
StackFrame::getValidCalleeObject(JSContext *cx, Value *vp)
{
    if (!isFunctionFrame()) {
        vp->setNull();
        return true;
    }

    JSFunction *fun = this->callee().toFunction();
    vp->setObject(*fun);

    




    const Value &thisv = functionThis();
    if (thisv.isObject() && fun->methodAtom() && !fun->isClonedMethod()) {
        JSObject *thisp = &thisv.toObject();
        JSObject *first_barriered_thisp = NULL;

        do {
            




            if (!thisp->isNative())
                continue;

            const Shape *shape = thisp->nativeLookup(cx, ATOM_TO_JSID(fun->methodAtom()));
            if (shape) {
                









                if (shape->isMethod() && thisp->nativeGetMethod(shape) == fun) {
                    if (!thisp->methodReadBarrier(cx, *shape, vp))
                        return false;
                    overwriteCallee(vp->toObject());
                    return true;
                }

                if (shape->hasSlot()) {
                    Value v = thisp->getSlot(shape->slot());
                    JSFunction *clone;

                    if (IsFunctionObject(v, &clone) &&
                        clone->isInterpreted() &&
                        clone->script() == fun->script() &&
                        clone->methodObj() == thisp) {
                        





                        JS_ASSERT_IF(!clone->hasSingletonType(), clone != fun);
                        *vp = v;
                        overwriteCallee(*clone);
                        return true;
                    }
                }
            }

            if (!first_barriered_thisp)
                first_barriered_thisp = thisp;
        } while ((thisp = thisp->getProto()) != NULL);

        if (!first_barriered_thisp)
            return true;

        










        JSFunction *newfunobj = CloneFunctionObject(cx, fun);
        if (!newfunobj)
            return false;
        newfunobj->setMethodObj(*first_barriered_thisp);
        overwriteCallee(*newfunobj);
        vp->setObject(*newfunobj);
        return true;
    }

    return true;
}

static JSBool
fun_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    while (!obj->isFunction()) {
        obj = obj->getProto();
        if (!obj)
            return true;
    }
    JSFunction *fun = obj->toFunction();

    





    if (fun->isInterpreted()) {
        fun->script()->uninlineable = true;
        MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);
    }

    
    vp->setNull();

    
    StackIter iter(cx);
    for (; !iter.done(); ++iter) {
        if (!iter.isFunctionFrame() || iter.isEvalFrame())
            continue;
        Value callee = iter.calleev();
        if (callee.isNull())
            return false;
        if (&callee.toObject() == fun)
            break;
    }
    if (iter.done())
        return true;

    StackFrame *fp = NULL;
    if (iter.isScript())
        fp = iter.fp();

    if (JSID_IS_ATOM(id, cx->runtime->atomState.argumentsAtom)) {
        
        if (!JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING | JSREPORT_STRICT, js_GetErrorMessage,
                                          NULL, JSMSG_DEPRECATED_USAGE, js_arguments_str)) {
            return false;
        }

        ArgumentsObject *argsobj;
        if (!fp) {
            JSObject &f = iter.callee();
            if (!&f)
                return false;
            
            
            
            argsobj = ArgumentsObject::createPoison(cx, f.toFunction()->nargs, f);
        } else {
            argsobj = ArgumentsObject::createUnexpected(cx, fp);
        }

        if (!argsobj)
            return false;

        *vp = ObjectValue(*argsobj);
        return true;
    }

    StackIter prev(iter);
    ++prev;

#ifdef JS_METHODJIT
    if (JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom) && fp && fp->prev()) {
        




        JSInlinedSite *inlined;
        jsbytecode *prevpc = fp->prev()->pcQuadratic(cx->stack, fp, &inlined);
        if (inlined) {
            mjit::JITChunk *chunk = fp->prev()->jit()->chunk(prevpc);
            JSFunction *fun = chunk->inlineFrames()[inlined->inlineIndex].fun;
            fun->script()->uninlineable = true;
            MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);
        }
    }
#endif

    if (JSID_IS_ATOM(id, cx->runtime->atomState.callerAtom)) {
        if (prev.done())
            return true;

        if (!prev.isFunctionFrame())
            return true;
        *vp = prev.calleev();

        if (!vp->isObject()) {
            JS_ASSERT(vp->isNull());
            return false;
        }

        
        JSObject &caller = vp->toObject();
        if (caller.compartment() != cx->compartment) {
            vp->setNull();
        } else if (caller.isFunction()) {
            JSFunction *callerFun = caller.toFunction();
            if (callerFun->isInterpreted() && callerFun->inStrictMode()) {
                JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                             JSMSG_CALLER_IS_STRICT);
                return false;
            }
        }

        return true;
    }

    JS_NOT_REACHED("fun_getProperty");
    return false;
}





static const uint16_t poisonPillProps[] = {
    ATOM_OFFSET(arguments),
    ATOM_OFFSET(caller),
};

static JSBool
fun_enumerate(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isFunction());

    RootObject root(cx, &obj);

    jsid id;
    bool found;

    if (!obj->isBoundFunction()) {
        id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
        return false;
        
    id = ATOM_TO_JSID(cx->runtime->atomState.nameAtom);
    if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
        return false;

    for (unsigned i = 0; i < ArrayLength(poisonPillProps); i++) {
        const uint16_t offset = poisonPillProps[i];
        id = ATOM_TO_JSID(OFFSET_TO_ATOM(cx->runtime, offset));
        if (!obj->hasProperty(cx, id, &found, JSRESOLVE_QUALIFIED))
            return false;
    }

    return true;
}

static JSObject *
ResolveInterpretedFunctionPrototype(JSContext *cx, JSObject *obj)
{
#ifdef DEBUG
    JSFunction *fun = obj->toFunction();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
#endif

    




    JS_ASSERT(!IsInternalFunctionObject(obj));
    JS_ASSERT(!obj->isBoundFunction());

    



    JSObject *objProto = obj->global().getOrCreateObjectPrototype(cx);
    if (!objProto)
        return NULL;
    JSObject *proto = NewObjectWithGivenProto(cx, &ObjectClass, objProto, NULL);
    if (!proto || !proto->setSingletonType(cx))
        return NULL;

    





    if (!obj->defineProperty(cx, cx->runtime->atomState.classPrototypeAtom,
                             ObjectValue(*proto), JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_PERMANENT) ||
        !proto->defineProperty(cx, cx->runtime->atomState.constructorAtom,
                               ObjectValue(*obj), JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
       return NULL;
    }

    return proto;
}

static JSBool
fun_resolve(JSContext *cx, JSObject *obj, jsid id, unsigned flags,
            JSObject **objp)
{
    if (!JSID_IS_ATOM(id))
        return true;

    RootedVarFunction fun(cx);
    fun = obj->toFunction();

    if (JSID_IS_ATOM(id, cx->runtime->atomState.classPrototypeAtom)) {
        











        if (fun->isNative() || fun->isFunctionPrototype())
            return true;

        if (!ResolveInterpretedFunctionPrototype(cx, fun))
            return false;
        *objp = fun;
        return true;
    }

    if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom) ||
        JSID_IS_ATOM(id, cx->runtime->atomState.nameAtom)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));

        Value v;
        if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom))
            v.setInt32(fun->nargs);
        else
            v.setString(fun->atom ? fun->atom : cx->runtime->emptyString);
        
        if (!DefineNativeProperty(cx, fun, id, v, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY, 0, 0)) {
            return false;
        }
        *objp = fun;
        return true;
    }

    for (unsigned i = 0; i < ArrayLength(poisonPillProps); i++) {
        const uint16_t offset = poisonPillProps[i];

        if (JSID_IS_ATOM(id, OFFSET_TO_ATOM(cx->runtime, offset))) {
            JS_ASSERT(!IsInternalFunctionObject(fun));

            PropertyOp getter;
            StrictPropertyOp setter;
            unsigned attrs = JSPROP_PERMANENT;
            if (fun->isInterpreted() ? fun->inStrictMode() : fun->isBoundFunction()) {
                JSObject *throwTypeError = fun->global().getThrowTypeError();

                getter = CastAsPropertyOp(throwTypeError);
                setter = CastAsStrictPropertyOp(throwTypeError);
                attrs |= JSPROP_GETTER | JSPROP_SETTER;
            } else {
                getter = fun_getProperty;
                setter = JS_StrictPropertyStub;
            }

            if (!DefineNativeProperty(cx, fun, id, UndefinedValue(), getter, setter,
                                      attrs, 0, 0)) {
                return false;
            }
            *objp = fun;
            return true;
        }
    }

    return true;
}

template<XDRMode mode>
bool
js::XDRInterpretedFunction(XDRState<mode> *xdr, JSObject **objp, JSScript *parentScript)
{
    JSFunction *fun;
    JSAtom *atom;
    uint32_t firstword;           


    uint32_t flagsword;           

    JSContext *cx = xdr->cx();
    JSScript *script;
    if (mode == XDR_ENCODE) {
        fun = (*objp)->toFunction();
        if (!fun->isInterpreted()) {
            JSAutoByteString funNameBytes;
            if (const char *name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_SCRIPTED_FUNCTION,
                                     name);
            }
            return false;
        }
        firstword = !!fun->atom;
        flagsword = (fun->nargs << 16) | fun->flags;
        atom = fun->atom;
        script = fun->script();
    } else {
        RootedVarObject parent(cx, NULL);
        fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_INTERPRETED, parent, NULL);
        if (!fun)
            return false;
        if (!fun->clearParent(cx))
            return false;
        if (!fun->clearType(cx))
            return false;
        atom = NULL;
        script = NULL;
    }

    if (!xdr->codeUint32(&firstword))
        return false;
    if ((firstword & 1U) && !XDRAtom(xdr, &atom))
        return false;
    if (!xdr->codeUint32(&flagsword))
        return false;

    if (!XDRScript(xdr, &script, parentScript))
        return false;

    if (mode == XDR_DECODE) {
        fun->nargs = flagsword >> 16;
        JS_ASSERT((flagsword & JSFUN_KINDMASK) >= JSFUN_INTERPRETED);
        fun->flags = uint16_t(flagsword);
        fun->atom.init(atom);
        fun->initScript(script);
        if (!script->typeSetFunction(cx, fun))
            return false;
        JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
        js_CallNewScriptHook(cx, fun->script(), fun);
        *objp = fun;
    }

    return true;
}

template bool
js::XDRInterpretedFunction(XDRState<XDR_ENCODE> *xdr, JSObject **objp, JSScript *parentScript);

template bool
js::XDRInterpretedFunction(XDRState<XDR_DECODE> *xdr, JSObject **objp, JSScript *parentScript);






static JSBool
fun_hasInstance(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp)
{
    while (obj->isFunction()) {
        if (!obj->isBoundFunction())
            break;
        obj = obj->toFunction()->getBoundFunctionTarget();
    }

    Value pval;
    if (!obj->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &pval))
        return JS_FALSE;

    if (pval.isPrimitive()) {
        



        js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, ObjectValue(*obj), NULL);
        return JS_FALSE;
    }

    *bp = js_IsDelegate(cx, &pval.toObject(), *v);
    return JS_TRUE;
}

inline void
JSFunction::trace(JSTracer *trc)
{
    if (isExtended()) {
        MarkValueRange(trc, ArrayLength(toExtended()->extendedSlots),
                       toExtended()->extendedSlots, "nativeReserved");
    }

    if (atom)
        MarkString(trc, &atom, "atom");

    if (isInterpreted()) {
        if (u.i.script_)
            MarkScriptUnbarriered(trc, &u.i.script_, "script");
        if (u.i.env_)
            MarkObjectUnbarriered(trc, &u.i.env_, "fun_callscope");
    }
}

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    obj->toFunction()->trace(trc);
}






JS_FRIEND_DATA(Class) js::FunctionClass = {
    js_Function_str,
    JSCLASS_NEW_RESOLVE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    fun_enumerate,
    (JSResolveOp)fun_resolve,
    JS_ConvertStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    fun_hasInstance,
    fun_trace
};

JSString *
fun_toStringHelper(JSContext *cx, JSObject *obj, unsigned indent)
{
    if (!obj->isFunction()) {
        if (IsFunctionProxy(obj))
            return Proxy::fun_toString(cx, obj, indent);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             "object");
        return NULL;
    }

    JSFunction *fun = obj->toFunction();
    if (!fun)
        return NULL;

    if (!indent && !cx->compartment->toSourceCache.empty()) {
        ToSourceCache::Ptr p = cx->compartment->toSourceCache.ref().lookup(fun);
        if (p)
            return p->value;
    }

    JSString *str = JS_DecompileFunction(cx, fun, indent);
    if (!str)
        return NULL;

    if (!indent) {
        Maybe<ToSourceCache> &lazy = cx->compartment->toSourceCache;

        if (lazy.empty()) {
            lazy.construct();
            if (!lazy.ref().init())
                return NULL;
        }

        if (!lazy.ref().put(fun, str))
            return NULL;
    }

    return str;
}

static JSBool
fun_toString(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ASSERT(IsFunctionObject(vp[0]));
    uint32_t indent = 0;

    if (argc != 0 && !ToUint32(cx, vp[2], &indent))
        return false;

    JSObject *obj = ToObject(cx, &vp[1]);
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
fun_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ASSERT(IsFunctionObject(vp[0]));

    JSObject *obj = ToObject(cx, &vp[1]);
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
js_fun_call(JSContext *cx, unsigned argc, Value *vp)
{
    Value fval = vp[1];

    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &FunctionClass);
        return false;
    }

    Value *argv = vp + 2;
    Value thisv;
    if (argc == 0) {
        thisv.setUndefined();
    } else {
        thisv = argv[0];

        argc--;
        argv++;
    }

    
    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc, &args))
        return JS_FALSE;

    
    args.calleev() = fval;
    args.thisv() = thisv;
    PodCopy(args.array(), argv, argc);

    bool ok = Invoke(cx, args);
    *vp = args.rval();
    return ok;
}


JSBool
js_fun_apply(JSContext *cx, unsigned argc, Value *vp)
{
    
    Value fval = vp[1];
    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &FunctionClass);
        return false;
    }

    
    if (argc < 2 || vp[3].isNullOrUndefined())
        return js_fun_call(cx, (argc > 0) ? 1 : 0, vp);

    InvokeArgsGuard args;
    if (vp[3].isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        





        
        unsigned length = cx->fp()->numActualArgs();
        JS_ASSERT(length <= StackSpace::ARGS_LENGTH_MAX);

        if (!cx->stack.pushInvokeArgs(cx, length, &args))
            return false;

        
        args.calleev() = fval;
        args.thisv() = vp[2];

        
        cx->fp()->forEachCanonicalActualArg(CopyTo(args.array()));
    } else {
        
        if (!vp[3].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS, js_apply_str);
            return false;
        }

        



        JSObject *aobj = &vp[3].toObject();
        uint32_t length;
        if (!js_GetLengthProperty(cx, aobj, &length))
            return false;

        
        if (length > StackSpace::ARGS_LENGTH_MAX) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_APPLY_ARGS);
            return false;
        }

        if (!cx->stack.pushInvokeArgs(cx, length, &args))
            return false;

        
        args.calleev() = fval;
        args.thisv() = vp[2];

        
        if (!GetElements(cx, aobj, length, args.array()))
            return false;
    }

    
    if (!Invoke(cx, args))
        return false;

    *vp = args.rval();
    return true;
}

namespace js {

JSBool
CallOrConstructBoundFunction(JSContext *cx, unsigned argc, Value *vp);

}

static const uint32_t JSSLOT_BOUND_FUNCTION_THIS       = 0;
static const uint32_t JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 1;

static const uint32_t BOUND_FUNCTION_RESERVED_SLOTS = 2;

inline bool
JSFunction::initBoundFunction(JSContext *cx, const Value &thisArg,
                              const Value *args, unsigned argslen)
{
    JS_ASSERT(isFunction());

    




    if (!toDictionaryMode(cx))
        return false;

    if (!setFlag(cx, BaseShape::BOUND_FUNCTION))
        return false;

    if (!setSlotSpan(cx, BOUND_FUNCTION_RESERVED_SLOTS + argslen))
        return false;

    setSlot(JSSLOT_BOUND_FUNCTION_THIS, thisArg);
    setSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT, PrivateUint32Value(argslen));

    initSlotRange(BOUND_FUNCTION_RESERVED_SLOTS, args, argslen);

    return true;
}

inline JSObject *
JSFunction::getBoundFunctionTarget() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    
    return getParent();
}

inline const js::Value &
JSFunction::getBoundFunctionThis() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_THIS);
}

inline const js::Value &
JSFunction::getBoundFunctionArgument(unsigned which) const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());
    JS_ASSERT(which < getBoundFunctionArgumentCount());

    return getSlot(BOUND_FUNCTION_RESERVED_SLOTS + which);
}

inline size_t
JSFunction::getBoundFunctionArgumentCount() const
{
    JS_ASSERT(isFunction());
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT).toPrivateUint32();
}

namespace js {


JSBool
CallOrConstructBoundFunction(JSContext *cx, unsigned argc, Value *vp)
{
    JSFunction *fun = vp[0].toObject().toFunction();
    JS_ASSERT(fun->isBoundFunction());

    bool constructing = IsConstructing(vp);

    
    unsigned argslen = fun->getBoundFunctionArgumentCount();

    if (argc + argslen > StackSpace::ARGS_LENGTH_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    
    JSObject *target = fun->getBoundFunctionTarget();

    
    const Value &boundThis = fun->getBoundFunctionThis();

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc + argslen, &args))
        return false;

    
    for (unsigned i = 0; i < argslen; i++)
        args[i] = fun->getBoundFunctionArgument(i);
    PodCopy(args.array() + argslen, vp + 2, argc);

    
    args.calleev().setObject(*target);

    if (!constructing)
        args.thisv() = boundThis;

    if (constructing ? !InvokeConstructor(cx, args) : !Invoke(cx, args))
        return false;

    *vp = args.rval();
    return true;
}

}

#if JS_HAS_GENERATORS
static JSBool
fun_isGenerator(JSContext *cx, unsigned argc, Value *vp)
{
    JSFunction *fun;
    if (!IsFunctionObject(vp[1], &fun)) {
        JS_SET_RVAL(cx, vp, BooleanValue(false));
        return true;
    }

    bool result = false;
    if (fun->isInterpreted()) {
        JSScript *script = fun->script();
        JS_ASSERT(script->length != 0);
        result = script->code[0] == JSOP_GENERATOR;
    }

    JS_SET_RVAL(cx, vp, BooleanValue(result));
    return true;
}
#endif


static JSBool
fun_bind(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    Value &thisv = args.thisv();

    
    if (!js_IsCallable(thisv)) {
        ReportIncompatibleMethod(cx, args, &FunctionClass);
        return false;
    }

    RootedVarObject target(cx);
    target = &thisv.toObject();

    
    Value *boundArgs = NULL;
    unsigned argslen = 0;
    if (args.length() > 1) {
        boundArgs = args.array() + 1;
        argslen = args.length() - 1;
    }

    
    unsigned length = 0;
    if (target->isFunction()) {
        unsigned nargs = target->toFunction()->nargs;
        if (nargs > argslen)
            length = nargs - argslen;
    }

    
    JSAtom *name = target->isFunction() ? target->toFunction()->atom.get() : NULL;

    JSObject *funobj =
        js_NewFunction(cx, NULL, CallOrConstructBoundFunction, length,
                       JSFUN_CONSTRUCTOR, target, name);
    if (!funobj)
        return false;

    
    if (!funobj->setParent(cx, target))
        return false;

    
    Value thisArg = args.length() >= 1 ? args[0] : UndefinedValue();
    if (!funobj->toFunction()->initBoundFunction(cx, thisArg, boundArgs, argslen))
        return false;

    
    

    
    args.rval().setObject(*funobj);
    return true;
}





static bool
OnBadFormal(JSContext *cx, TokenKind tt)
{
    if (tt != TOK_ERROR)
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_FORMAL);
    else
        JS_ASSERT(cx->isExceptionPending());
    return false;
}

namespace js {

JSFunctionSpec function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0),
    JS_FN(js_apply_str,      js_fun_apply,   2,0),
    JS_FN(js_call_str,       js_fun_call,    1,0),
    JS_FN("bind",            fun_bind,       1,0),
#if JS_HAS_GENERATORS
    JS_FN("isGenerator",     fun_isGenerator,0,0),
#endif
    JS_FS_END
};

JSBool
Function(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedVar<GlobalObject*> global(cx);
    global = &args.callee().global();
    if (!global->isRuntimeCodeGenEnabled(cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_FUNCTION);
        return false;
    }

    Bindings bindings(cx);

    const char *filename;
    unsigned lineno;
    JSPrincipals *originPrincipals;
    CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals);
    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    unsigned n = args.length() ? args.length() - 1 : 0;
    if (n > 0) {
        









        size_t args_length = 0;
        for (unsigned i = 0; i < n; i++) {
            
            JSString *arg = ToString(cx, args[i]);
            if (!arg)
                return false;
            args[i].setString(arg);

            



            size_t old_args_length = args_length;
            args_length = old_args_length + arg->length();
            if (args_length < old_args_length) {
                js_ReportAllocationOverflow(cx);
                return false;
            }
        }

        
        size_t old_args_length = args_length;
        args_length = old_args_length + n - 1;
        if (args_length < old_args_length ||
            args_length >= ~(size_t)0 / sizeof(jschar)) {
            js_ReportAllocationOverflow(cx);
            return false;
        }

        




        LifoAllocScope las(&cx->tempLifoAlloc());
        jschar *cp = cx->tempLifoAlloc().newArray<jschar>(args_length + 1);
        if (!cp) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        jschar *collected_args = cp;

        


        for (unsigned i = 0; i < n; i++) {
            JSString *arg = args[i].toString();
            size_t arg_length = arg->length();
            const jschar *arg_chars = arg->getChars(cx);
            if (!arg_chars)
                return false;
            (void) js_strncpy(cp, arg_chars, arg_length);
            cp += arg_length;

            
            *cp++ = (i + 1 < n) ? ',' : 0;
        }

        
        TokenStream ts(cx, principals, originPrincipals);
        if (!ts.init(collected_args, args_length, filename, lineno, cx->findVersion()))
            return false;

        
        TokenKind tt = ts.getToken();
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (tt != TOK_NAME)
                    return OnBadFormal(cx, tt);

                
                PropertyName *name = ts.currentToken().name();
                if (bindings.hasBinding(cx, name)) {
                    JSAutoByteString bytes;
                    if (!js_AtomToPrintableString(cx, name, &bytes))
                        return false;
                    if (!ReportCompileErrorNumber(cx, &ts, NULL,
                                                  JSREPORT_WARNING | JSREPORT_STRICT,
                                                  JSMSG_DUPLICATE_FORMAL, bytes.ptr()))
                    {
                        return false;
                    }
                }

                uint16_t dummy;
                if (!bindings.addArgument(cx, name, &dummy))
                    return false;

                



                tt = ts.getToken();
                if (tt == TOK_EOF)
                    break;
                if (tt != TOK_COMMA)
                    return OnBadFormal(cx, tt);
                tt = ts.getToken();
            }
        }
    }

    JS::Anchor<JSString *> strAnchor(NULL);
    const jschar *chars;
    size_t length;

    if (args.length()) {
        JSString *str = ToString(cx, args[args.length() - 1]);
        if (!str)
            return false;
        strAnchor.set(str);
        chars = str->getChars(cx);
        length = str->length();
    } else {
        chars = cx->runtime->emptyString->chars();
        length = 0;
    }

    





    JSFunction *fun = js_NewFunction(cx, NULL, NULL, 0, JSFUN_LAMBDA | JSFUN_INTERPRETED,
                                     global, cx->runtime->atomState.anonymousAtom);
    if (!fun)
        return false;

    bool ok = frontend::CompileFunctionBody(cx, fun, principals, originPrincipals,
                                            &bindings, chars, length, filename, lineno,
                                            cx->findVersion());
    args.rval().setObject(*fun);
    return ok;
}

bool
IsBuiltinFunctionConstructor(JSFunction *fun)
{
    return fun->maybeNative() == Function;
}

const Shape *
LookupInterpretedFunctionPrototype(JSContext *cx, JSObject *funobj)
{
#ifdef DEBUG
    JSFunction *fun = funobj->toFunction();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
    JS_ASSERT(!funobj->isBoundFunction());
#endif

    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    const Shape *shape = funobj->nativeLookup(cx, id);
    if (!shape) {
        if (!ResolveInterpretedFunctionPrototype(cx, funobj))
            return NULL;
        shape = funobj->nativeLookup(cx, id);
    }
    JS_ASSERT(!shape->configurable());
    JS_ASSERT(shape->isDataDescriptor());
    JS_ASSERT(shape->hasSlot());
    JS_ASSERT(!shape->isMethod());
    return shape;
}

} 

JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, Native native, unsigned nargs,
               unsigned flags, HandleObject parent, JSAtom *atom, js::gc::AllocKind kind)
{
    JS_ASSERT(kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT(sizeof(JSFunction) <= gc::Arena::thingSize(JSFunction::FinalizeKind));
    JS_ASSERT(sizeof(FunctionExtended) <= gc::Arena::thingSize(JSFunction::ExtendedFinalizeKind));

    JSFunction *fun;

    if (funobj) {
        JS_ASSERT(funobj->isFunction());
        JS_ASSERT(funobj->getParent() == parent);
    } else {
        funobj = NewObjectWithClassProto(cx, &FunctionClass, NULL, SkipScopeParent(parent), kind);
        if (!funobj)
            return NULL;
    }
    fun = static_cast<JSFunction *>(funobj);

    
    fun->nargs = uint16_t(nargs);
    fun->flags = flags & (JSFUN_FLAGS_MASK | JSFUN_KINDMASK);
    if ((flags & JSFUN_KINDMASK) >= JSFUN_INTERPRETED) {
        JS_ASSERT(!native);
        fun->script().init(NULL);
        fun->initEnvironment(parent);
    } else {
        fun->u.native = native;
        JS_ASSERT(fun->u.native);
    }
    if (kind == JSFunction::ExtendedFinalizeKind) {
        fun->flags |= JSFUN_EXTENDED;
        fun->initializeExtended();
    }
    fun->atom.init(atom);

    if (native && !fun->setSingletonType(cx))
        return NULL;

    return fun;
}

JSFunction * JS_FASTCALL
js_CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                       JSObject *proto, gc::AllocKind kind)
{
    JS_ASSERT(parent);
    JS_ASSERT(proto);

    JSObject *cloneobj = NewObjectWithClassProto(cx, &FunctionClass, NULL, SkipScopeParent(parent), kind);
    if (!cloneobj)
        return NULL;
    JSFunction *clone = static_cast<JSFunction *>(cloneobj);

    clone->nargs = fun->nargs;
    clone->flags = fun->flags & ~JSFUN_EXTENDED;
    if (fun->isInterpreted()) {
        clone->initScript(fun->script());
        clone->initEnvironment(parent);
    } else {
        clone->u.native = fun->native();
    }
    clone->atom.init(fun->atom);

    if (kind == JSFunction::ExtendedFinalizeKind) {
        clone->flags |= JSFUN_EXTENDED;
        clone->initializeExtended();
    }

    if (cx->compartment == fun->compartment()) {
        






        if (fun->getProto() == proto && !fun->hasSingletonType())
            clone->setType(fun->type());
    } else {
        



        if (clone->isInterpreted()) {
            JSScript *script = clone->script();
            JS_ASSERT(script);
            JS_ASSERT(script->compartment() == fun->compartment());
            JS_ASSERT(script->compartment() != cx->compartment);

            clone->script().init(NULL);
            JSScript *cscript = CloneScript(cx, script);
            if (!cscript)
                return NULL;

            cscript->globalObject = &clone->global();
            clone->setScript(cscript);
            if (!cscript->typeSetFunction(cx, clone))
                return NULL;

            js_CallNewScriptHook(cx, clone->script(), clone);
            Debugger::onNewScript(cx, clone->script(), NULL);
        }
    }
    return clone;
}

JSFunction *
js_DefineFunction(JSContext *cx, HandleObject obj, jsid id, Native native,
                  unsigned nargs, unsigned attrs, AllocKind kind)
{
    RootId idRoot(cx, &id);

    PropertyOp gop;
    StrictPropertyOp sop;

    RootedVarFunction fun(cx);

    if (attrs & JSFUN_STUB_GSOPS) {
        





        attrs &= ~JSFUN_STUB_GSOPS;
        gop = JS_PropertyStub;
        sop = JS_StrictPropertyStub;
    } else {
        gop = NULL;
        sop = NULL;
    }

    fun = js_NewFunction(cx, NULL, native, nargs,
                         attrs & (JSFUN_FLAGS_MASK),
                         obj,
                         JSID_IS_ATOM(id) ? JSID_TO_ATOM(id) : NULL,
                         kind);
    if (!fun)
        return NULL;

    if (!obj->defineGeneric(cx, id, ObjectValue(*fun), gop, sop, attrs & ~JSFUN_FLAGS_MASK))
        return NULL;

    return fun;
}

JS_STATIC_ASSERT((JSV2F_CONSTRUCT & JSV2F_SEARCH_STACK) == 0);

JSFunction *
js_ValueToFunction(JSContext *cx, const Value *vp, unsigned flags)
{
    JSFunction *fun;
    if (!IsFunctionObject(*vp, &fun)) {
        js_ReportIsNotFunction(cx, vp, flags);
        return NULL;
    }
    return fun;
}

JSObject *
js_ValueToCallableObject(JSContext *cx, Value *vp, unsigned flags)
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
js_ReportIsNotFunction(JSContext *cx, const Value *vp, unsigned flags)
{
    const char *name = NULL, *source = NULL;
    AutoValueRooter tvr(cx);
    unsigned error = (flags & JSV2F_CONSTRUCT) ? JSMSG_NOT_CONSTRUCTOR : JSMSG_NOT_FUNCTION;

    











    ptrdiff_t spindex = 0;

    FrameRegsIter i(cx);
    if (!i.done()) {
        unsigned depth = js_ReconstructStackDepth(cx, i.fp()->script(), i.pc());
        Value *simsp = i.fp()->base() + depth;
        if (i.fp()->base() <= vp && vp < Min(simsp, i.sp()))
            spindex = vp - simsp;
    }

    if (!spindex)
        spindex = ((flags & JSV2F_SEARCH_STACK) ? JSDVG_SEARCH_STACK : JSDVG_IGNORE_STACK);

    js_ReportValueError3(cx, error, spindex, *vp, NULL, name, source);
}
