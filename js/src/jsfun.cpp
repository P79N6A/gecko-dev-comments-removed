









#include "jsfuninlines.h"

#include "mozilla/PodOperations.h"
#include "mozilla/Util.h"

#include <string.h>

#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsproxy.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jswrapper.h"

#include "builtin/Eval.h"
#include "builtin/Object.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#ifdef JS_ION
#include "jit/Ion.h"
#include "jit/IonFrameIterator.h"
#endif
#include "vm/Interpreter.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"
#include "vm/WrapperObject.h"
#include "vm/Xdr.h"

#include "jsscriptinlines.h"

#include "vm/Interpreter-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;
using namespace js::frontend;

using mozilla::ArrayLength;
using mozilla::PodCopy;

static bool
fun_getProperty(JSContext *cx, HandleObject obj_, HandleId id, MutableHandleValue vp)
{
    RootedObject obj(cx, obj_);
    while (!obj->is<JSFunction>()) {
        if (!JSObject::getProto(cx, obj, &obj))
            return false;
        if (!obj)
            return true;
    }
    RootedFunction fun(cx, &obj->as<JSFunction>());

    
    vp.setNull();

    
    NonBuiltinScriptFrameIter iter(cx);
    for (; !iter.done(); ++iter) {
        if (!iter.isFunctionFrame() || iter.isEvalFrame())
            continue;
        if (iter.callee() == fun)
            break;
    }
    if (iter.done())
        return true;

    if (JSID_IS_ATOM(id, cx->names().arguments)) {
        if (fun->hasRest()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_FUNCTION_ARGUMENTS_AND_REST);
            return false;
        }
        
        if (!JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING | JSREPORT_STRICT, js_GetErrorMessage,
                                          NULL, JSMSG_DEPRECATED_USAGE, js_arguments_str)) {
            return false;
        }

        ArgumentsObject *argsobj = ArgumentsObject::createUnexpected(cx, iter);
        if (!argsobj)
            return false;

#ifdef JS_ION
        
        
        
        
        JSScript *script = iter.script();
        jit::ForbidCompilation(cx, script);
#endif

        vp.setObject(*argsobj);
        return true;
    }

    if (JSID_IS_ATOM(id, cx->names().caller)) {
        ++iter;
        if (iter.done() || !iter.isFunctionFrame()) {
            JS_ASSERT(vp.isNull());
            return true;
        }

        
        JSObject &maybeClone = iter.calleev().toObject();
        if (maybeClone.is<JSFunction>() && maybeClone.as<JSFunction>().nonLazyScript()->isCallsiteClone)
            vp.setObject(*maybeClone.as<JSFunction>().nonLazyScript()->originalFunction());
        else
            vp.set(iter.calleev());

        if (!cx->compartment()->wrap(cx, vp))
            return false;

        


        RootedObject caller(cx, &vp.toObject());
        if (caller->is<WrapperObject>() && !Wrapper::wrapperHandler(caller)->isSafeToUnwrap()) {
            vp.setNull();
        } else if (caller->is<JSFunction>()) {
            JSFunction *callerFun = &caller->as<JSFunction>();
            if (callerFun->isInterpreted() && callerFun->strict()) {
                JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                             JSMSG_CALLER_IS_STRICT);
                return false;
            }
        }

        return true;
    }

    MOZ_ASSUME_UNREACHABLE("fun_getProperty");
}





static const uint16_t poisonPillProps[] = {
    NAME_OFFSET(arguments),
    NAME_OFFSET(caller),
};

static bool
fun_enumerate(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<JSFunction>());

    RootedId id(cx);
    bool found;

    if (!obj->isBoundFunction()) {
        id = NameToId(cx->names().classPrototype);
        if (!JSObject::hasProperty(cx, obj, id, &found, 0))
            return false;
    }

    id = NameToId(cx->names().length);
    if (!JSObject::hasProperty(cx, obj, id, &found, 0))
        return false;

    id = NameToId(cx->names().name);
    if (!JSObject::hasProperty(cx, obj, id, &found, 0))
        return false;

    for (unsigned i = 0; i < ArrayLength(poisonPillProps); i++) {
        const uint16_t offset = poisonPillProps[i];
        id = NameToId(OFFSET_TO_NAME(cx->runtime(), offset));
        if (!JSObject::hasProperty(cx, obj, id, &found, 0))
            return false;
    }

    return true;
}

static JSObject *
ResolveInterpretedFunctionPrototype(JSContext *cx, HandleObject obj)
{
#ifdef DEBUG
    JSFunction *fun = &obj->as<JSFunction>();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
#endif

    
    
    
    JS_ASSERT(!IsInternalFunctionObject(obj));
    JS_ASSERT(!obj->isBoundFunction());

    
    
    
    
    bool isStarGenerator = obj->as<JSFunction>().isStarGenerator();
    JSObject *objProto;
    if (isStarGenerator)
        objProto = obj->global().getOrCreateStarGeneratorObjectPrototype(cx);
    else
        objProto = obj->global().getOrCreateObjectPrototype(cx);
    if (!objProto)
        return NULL;
    const Class *clasp = &JSObject::class_;

    RootedObject proto(cx, NewObjectWithGivenProto(cx, clasp, objProto, NULL, SingletonObject));
    if (!proto)
        return NULL;

    
    
    RootedValue protoVal(cx, ObjectValue(*proto));
    if (!JSObject::defineProperty(cx, obj, cx->names().classPrototype,
                                  protoVal, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT))
    {
        return NULL;
    }

    
    
    
    
    if (!isStarGenerator) {
        RootedValue objVal(cx, ObjectValue(*obj));
        if (!JSObject::defineProperty(cx, proto, cx->names().constructor,
                                      objVal, JS_PropertyStub, JS_StrictPropertyStub, 0))
        {
            return NULL;
        }
    }

    return proto;
}

bool
js::fun_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                MutableHandleObject objp)
{
    if (!JSID_IS_ATOM(id))
        return true;

    RootedFunction fun(cx, &obj->as<JSFunction>());

    if (JSID_IS_ATOM(id, cx->names().classPrototype)) {
        











        if (fun->isBuiltin() || fun->isFunctionPrototype())
            return true;

        if (!ResolveInterpretedFunctionPrototype(cx, fun))
            return false;
        objp.set(fun);
        return true;
    }

    if (JSID_IS_ATOM(id, cx->names().length) || JSID_IS_ATOM(id, cx->names().name)) {
        JS_ASSERT(!IsInternalFunctionObject(obj));

        RootedValue v(cx);
        if (JSID_IS_ATOM(id, cx->names().length)) {
            if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
                return false;
            uint16_t length = fun->hasScript() ? fun->nonLazyScript()->funLength :
                fun->nargs - fun->hasRest();
            v.setInt32(length);
        } else {
            v.setString(fun->atom() == NULL ?  cx->runtime()->emptyString : fun->atom());
        }

        if (!DefineNativeProperty(cx, fun, id, v, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY, 0, 0)) {
            return false;
        }
        objp.set(fun);
        return true;
    }

    for (unsigned i = 0; i < ArrayLength(poisonPillProps); i++) {
        const uint16_t offset = poisonPillProps[i];

        if (JSID_IS_ATOM(id, OFFSET_TO_NAME(cx->runtime(), offset))) {
            JS_ASSERT(!IsInternalFunctionObject(fun));

            PropertyOp getter;
            StrictPropertyOp setter;
            unsigned attrs = JSPROP_PERMANENT | JSPROP_SHARED;
            if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
                return false;
            if (fun->isInterpreted() ? fun->strict() : fun->isBoundFunction()) {
                JSObject *throwTypeError = fun->global().getThrowTypeError();

                getter = CastAsPropertyOp(throwTypeError);
                setter = CastAsStrictPropertyOp(throwTypeError);
                attrs |= JSPROP_GETTER | JSPROP_SETTER;
            } else {
                getter = fun_getProperty;
                setter = JS_StrictPropertyStub;
            }

            RootedValue value(cx, UndefinedValue());
            if (!DefineNativeProperty(cx, fun, id, value, getter, setter,
                                      attrs, 0, 0)) {
                return false;
            }
            objp.set(fun);
            return true;
        }
    }

    return true;
}

template<XDRMode mode>
bool
js::XDRInterpretedFunction(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript enclosingScript,
                           MutableHandleObject objp)
{
    enum FirstWordFlag {
        HasAtom = 0x1,
        IsStarGenerator = 0x2
    };

    
    RootedAtom atom(xdr->cx());
    uint32_t firstword = 0;        
    uint32_t flagsword = 0;        

    JSContext *cx = xdr->cx();
    RootedFunction fun(cx);
    RootedScript script(cx);
    if (mode == XDR_ENCODE) {
        fun = &objp->as<JSFunction>();
        if (!fun->isInterpreted()) {
            JSAutoByteString funNameBytes;
            if (const char *name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_SCRIPTED_FUNCTION,
                                     name);
            }
            return false;
        }
        if (fun->atom())
            firstword |= HasAtom;
        if (fun->isStarGenerator())
            firstword |= IsStarGenerator;
        script = fun->getOrCreateScript(cx);
        if (!script)
            return false;
        atom = fun->atom();
        flagsword = (fun->nargs << 16) | fun->flags;

        if (!xdr->codeUint32(&firstword))
            return false;
    } else {
        if (!xdr->codeUint32(&firstword))
            return false;

        JSObject *proto = NULL;
        if (firstword & IsStarGenerator) {
            proto = cx->global()->getOrCreateStarGeneratorFunctionPrototype(cx);
            if (!proto)
                return false;
        }
        fun = NewFunctionWithProto(cx, NullPtr(), NULL, 0, JSFunction::INTERPRETED,
                                   NullPtr(), NullPtr(), proto,
                                   JSFunction::FinalizeKind, TenuredObject);
        if (!fun)
            return false;
        atom = NULL;
        script = NULL;
    }

    if ((firstword & HasAtom) && !XDRAtom(xdr, &atom))
        return false;
    if (!xdr->codeUint32(&flagsword))
        return false;

    if (!XDRScript(xdr, enclosingScope, enclosingScript, fun, &script))
        return false;

    if (mode == XDR_DECODE) {
        fun->nargs = flagsword >> 16;
        fun->flags = uint16_t(flagsword);
        fun->initAtom(atom);
        fun->initScript(script);
        script->setFunction(fun);
        if (!JSFunction::setTypeForScriptedFunction(cx, fun))
            return false;
        JS_ASSERT(fun->nargs == fun->nonLazyScript()->bindings.numArgs());
        RootedScript script(cx, fun->nonLazyScript());
        CallNewScriptHook(cx, script, fun);
        objp.set(fun);
    }

    return true;
}

template bool
js::XDRInterpretedFunction(XDRState<XDR_ENCODE> *, HandleObject, HandleScript, MutableHandleObject);

template bool
js::XDRInterpretedFunction(XDRState<XDR_DECODE> *, HandleObject, HandleScript, MutableHandleObject);

JSObject *
js::CloneFunctionAndScript(JSContext *cx, HandleObject enclosingScope, HandleFunction srcFun)
{
    
    JSObject *cloneProto = NULL;
    if (srcFun->isStarGenerator()) {
        cloneProto = cx->global()->getOrCreateStarGeneratorFunctionPrototype(cx);
        if (!cloneProto)
            return NULL;
    }
    RootedFunction clone(cx, NewFunctionWithProto(cx, NullPtr(), NULL, 0, JSFunction::INTERPRETED,
                                                  NullPtr(), NullPtr(), cloneProto,
                                                  JSFunction::FinalizeKind, TenuredObject));
    if (!clone)
        return NULL;

    RootedScript srcScript(cx, srcFun->nonLazyScript());
    RootedScript clonedScript(cx, CloneScript(cx, enclosingScope, clone, srcScript));
    if (!clonedScript)
        return NULL;

    clone->nargs = srcFun->nargs;
    clone->flags = srcFun->flags;
    clone->initAtom(srcFun->displayAtom());
    clone->initScript(clonedScript);
    clonedScript->setFunction(clone);
    if (!JSFunction::setTypeForScriptedFunction(cx, clone))
        return NULL;

    RootedScript cloneScript(cx, clone->nonLazyScript());
    CallNewScriptHook(cx, cloneScript, clone);
    return clone;
}






static bool
fun_hasInstance(JSContext *cx, HandleObject objArg, MutableHandleValue v, bool *bp)
{
    RootedObject obj(cx, objArg);

    while (obj->is<JSFunction>() && obj->isBoundFunction())
        obj = obj->as<JSFunction>().getBoundFunctionTarget();

    RootedValue pval(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().classPrototype, &pval))
        return false;

    if (pval.isPrimitive()) {
        



        RootedValue val(cx, ObjectValue(*obj));
        js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, val, NullPtr());
        return false;
    }

    RootedObject pobj(cx, &pval.toObject());
    bool isDelegate;
    if (!IsDelegate(cx, pobj, v, &isDelegate))
        return false;
    *bp = isDelegate;
    return true;
}

inline void
JSFunction::trace(JSTracer *trc)
{
    if (isExtended()) {
        MarkValueRange(trc, ArrayLength(toExtended()->extendedSlots),
                       toExtended()->extendedSlots, "nativeReserved");
    }

    if (atom_)
        MarkString(trc, &atom_, "atom");

    if (isInterpreted()) {
        
        
        
        if (hasScript() && u.i.s.script_)
            MarkScriptUnbarriered(trc, &u.i.s.script_, "script");
        else if (isInterpretedLazy() && u.i.s.lazy_)
            MarkLazyScriptUnbarriered(trc, &u.i.s.lazy_, "lazyScript");
        if (u.i.env_)
            MarkObjectUnbarriered(trc, &u.i.env_, "fun_callscope");
    }
}

static void
fun_trace(JSTracer *trc, JSObject *obj)
{
    obj->as<JSFunction>().trace(trc);
}

const Class JSFunction::class_ = {
    js_Function_str,
    JSCLASS_NEW_RESOLVE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    fun_enumerate,
    (JSResolveOp)js::fun_resolve,
    JS_ConvertStub,
    NULL,                    
    NULL,                    
    NULL,                    
    fun_hasInstance,
    NULL,                    
    fun_trace
};

const Class* const js::FunctionClassPtr = &JSFunction::class_;


static bool
FindBody(JSContext *cx, HandleFunction fun, StableCharPtr chars, size_t length,
         size_t *bodyStart, size_t *bodyEnd)
{
    
    CompileOptions options(cx);
    options.setFileAndLine("internal-findBody", 0)
           .setVersion(fun->nonLazyScript()->getVersion());
    AutoKeepAtoms keepAtoms(cx->perThreadData);
    TokenStream ts(cx, options, chars.get(), length, NULL);
    int nest = 0;
    bool onward = true;
    
    do {
        switch (ts.getToken()) {
          case TOK_NAME:
          case TOK_YIELD:
            if (nest == 0)
                onward = false;
            break;
          case TOK_LP:
            nest++;
            break;
          case TOK_RP:
            if (--nest == 0)
                onward = false;
            break;
          case TOK_ERROR:
            
            return false;
          default:
            break;
        }
    } while (onward);
    TokenKind tt = ts.getToken();
    if (tt == TOK_ARROW)
        tt = ts.getToken();
    if (tt == TOK_ERROR)
        return false;
    bool braced = tt == TOK_LC;
    JS_ASSERT_IF(fun->isExprClosure(), !braced);
    *bodyStart = ts.currentToken().pos.begin;
    if (braced)
        *bodyStart += 1;
    StableCharPtr end(chars.get() + length, chars.get(), length);
    if (end[-1] == '}') {
        end--;
    } else {
        JS_ASSERT(!braced);
        for (; unicode::IsSpaceOrBOM2(end[-1]); end--)
            ;
    }
    *bodyEnd = end - chars;
    JS_ASSERT(*bodyStart <= *bodyEnd);
    return true;
}

JSString *
js::FunctionToString(JSContext *cx, HandleFunction fun, bool bodyOnly, bool lambdaParen)
{
    if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
        return NULL;

    
    
    if (fun->isArrow() && fun->isBoundFunction()) {
        JSObject *target = fun->getBoundFunctionTarget();
        RootedFunction targetFun(cx, &target->as<JSFunction>());
        JS_ASSERT(targetFun->isArrow());
        return FunctionToString(cx, targetFun, bodyOnly, lambdaParen);
    }

    StringBuffer out(cx);
    RootedScript script(cx);

    if (fun->hasScript()) {
        script = fun->nonLazyScript();
        if (script->isGeneratorExp) {
            if ((!bodyOnly && !out.append("function genexp() {")) ||
                !out.append("\n    [generator expression]\n") ||
                (!bodyOnly && !out.append("}")))
            {
                return NULL;
            }
            return out.finishString();
        }
    }
    if (!bodyOnly) {
        
        if (fun->isInterpreted() && !lambdaParen && fun->isLambda() && !fun->isArrow()) {
            if (!out.append("("))
                return NULL;
        }
        if (!fun->isArrow()) {
            if (!(fun->isStarGenerator() ? out.append("function* ") : out.append("function ")))
                return NULL;
        }
        if (fun->atom()) {
            if (!out.append(fun->atom()))
                return NULL;
        }
    }
    bool haveSource = fun->isInterpreted() && !fun->isSelfHostedBuiltin();
    if (haveSource && !script->scriptSource()->hasSourceData() &&
        !JSScript::loadSource(cx, script->scriptSource(), &haveSource))
    {
        return NULL;
    }
    if (haveSource) {
        RootedString srcStr(cx, script->sourceData(cx));
        if (!srcStr)
            return NULL;
        Rooted<JSStableString *> src(cx, srcStr->ensureStable(cx));
        if (!src)
            return NULL;

        StableCharPtr chars = src->chars();
        bool exprBody = fun->isExprClosure();

        
        
        
        
        bool funCon = !fun->isArrow() &&
                      script->sourceStart == 0 &&
                      script->sourceEnd == script->scriptSource()->length() &&
                      script->scriptSource()->argumentsNotIncluded();

        
        
        JS_ASSERT_IF(funCon, !fun->isArrow());
        JS_ASSERT_IF(funCon, !exprBody);
        JS_ASSERT_IF(!funCon && !fun->isArrow(), src->length() > 0 && chars[0] == '(');

        
        
        
        
        bool addUseStrict = script->strict && !script->explicitUseStrict && !fun->isArrow();

        bool buildBody = funCon && !bodyOnly;
        if (buildBody) {
            
            
            
            
            if (!out.append("("))
                return NULL;

            
            BindingVector *localNames = cx->new_<BindingVector>(cx);
            ScopedJSDeletePtr<BindingVector> freeNames(localNames);
            if (!FillBindingVector(script, localNames))
                return NULL;
            for (unsigned i = 0; i < fun->nargs; i++) {
                if ((i && !out.append(", ")) ||
                    (i == unsigned(fun->nargs - 1) && fun->hasRest() && !out.append("...")) ||
                    !out.append((*localNames)[i].name())) {
                    return NULL;
                }
            }
            if (!out.append(") {\n"))
                return NULL;
        }
        if ((bodyOnly && !funCon) || addUseStrict) {
            
            
            size_t bodyStart = 0, bodyEnd;

            
            
            if (!funCon) {
                JS_ASSERT(!buildBody);
                if (!FindBody(cx, fun, chars, src->length(), &bodyStart, &bodyEnd))
                    return NULL;
            } else {
                bodyEnd = src->length();
            }

            if (addUseStrict) {
                
                if (!out.append(chars, bodyStart))
                    return NULL;
                if (exprBody) {
                    
                    
                    
                    if (!out.append("/* use strict */ "))
                        return NULL;
                } else {
                    if (!out.append("\n\"use strict\";\n"))
                        return NULL;
                }
            }

            
            
            size_t dependentEnd = bodyOnly ? bodyEnd : src->length();
            if (!out.append(chars + bodyStart, dependentEnd - bodyStart))
                return NULL;
        } else {
            if (!out.append(src))
                return NULL;
        }
        if (buildBody) {
            if (!out.append("\n}"))
                return NULL;
        }
        if (bodyOnly) {
            
            if (exprBody && !out.append(";"))
                return NULL;
        } else if (!lambdaParen && fun->isLambda() && !fun->isArrow()) {
            if (!out.append(")"))
                return NULL;
        }
    } else if (fun->isInterpreted() && !fun->isSelfHostedBuiltin()) {
        if ((!bodyOnly && !out.append("() {\n    ")) ||
            !out.append("[sourceless code]") ||
            (!bodyOnly && !out.append("\n}")))
            return NULL;
        if (!lambdaParen && fun->isLambda() && !fun->isArrow() && !out.append(")"))
            return NULL;
    } else {
        JS_ASSERT(!fun->isExprClosure());
        if ((!bodyOnly && !out.append("() {\n    ")) ||
            !out.append("[native code]") ||
            (!bodyOnly && !out.append("\n}")))
            return NULL;
    }
    return out.finishString();
}

JSString *
fun_toStringHelper(JSContext *cx, HandleObject obj, unsigned indent)
{
    if (!obj->is<JSFunction>()) {
        if (obj->is<FunctionProxyObject>())
            return Proxy::fun_toString(cx, obj, indent);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             "object");
        return NULL;
    }

    RootedFunction fun(cx, &obj->as<JSFunction>());
    return FunctionToString(cx, fun, false, indent != JS_DONT_PRETTY_PRINT);
}

static bool
fun_toString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(IsFunctionObject(args.calleev()));

    uint32_t indent = 0;

    if (args.length() != 0 && !ToUint32(cx, args[0], &indent))
        return false;

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedString str(cx, fun_toStringHelper(cx, obj, indent));
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

#if JS_HAS_TOSOURCE
static bool
fun_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(IsFunctionObject(args.calleev()));

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedString str(cx);
    if (obj->is<JSFunction>() || obj->is<FunctionProxyObject>())
        str = fun_toStringHelper(cx, obj, JS_DONT_PRETTY_PRINT);
    else
        str = ObjectToSource(cx, obj);

    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}
#endif

bool
js_fun_call(JSContext *cx, unsigned argc, Value *vp)
{
    RootedValue fval(cx, vp[1]);

    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &JSFunction::class_);
        return false;
    }

    Value *argv = vp + 2;
    RootedValue thisv(cx, UndefinedValue());
    if (argc != 0) {
        thisv = argv[0];

        argc--;
        argv++;
    }

    
    InvokeArgs args(cx);
    if (!args.init(argc))
        return false;

    
    args.setCallee(fval);
    args.setThis(thisv);
    PodCopy(args.array(), argv, argc);

    bool ok = Invoke(cx, args);
    *vp = args.rval();
    return ok;
}

#ifdef JS_ION
static bool
PushBaselineFunApplyArguments(JSContext *cx, jit::IonFrameIterator &frame, InvokeArgs &args,
                              Value *vp)
{
    unsigned length = frame.numActualArgs();
    JS_ASSERT(length <= ARGS_LENGTH_MAX);

    if (!args.init(length))
        return false;

    
    args.setCallee(vp[1]);
    args.setThis(vp[2]);

    
    frame.forEachCanonicalActualArg(CopyTo(args.array()), 0, -1);
    return true;
}
#endif


bool
js_fun_apply(JSContext *cx, unsigned argc, Value *vp)
{
    
    RootedValue fval(cx, vp[1]);
    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &JSFunction::class_);
        return false;
    }

    
    if (argc < 2 || vp[3].isNullOrUndefined())
        return js_fun_call(cx, (argc > 0) ? 1 : 0, vp);

    InvokeArgs args(cx);

    




    if (vp[3].isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        



        

#ifdef JS_ION
        
        
        
        if (cx->currentlyRunningInJit()) {
            jit::JitActivationIterator activations(cx->runtime());
            jit::IonFrameIterator frame(activations);
            if (frame.isNative()) {
                
                ++frame;
                if (frame.isOptimizedJS()) {
                    jit::InlineFrameIterator iter(cx, &frame);

                    unsigned length = iter.numActualArgs();
                    JS_ASSERT(length <= ARGS_LENGTH_MAX);

                    if (!args.init(length))
                        return false;

                    
                    args.setCallee(fval);
                    args.setThis(vp[2]);

                    
                    iter.forEachCanonicalActualArg(cx, CopyTo(args.array()), 0, -1);
                } else {
                    JS_ASSERT(frame.isBaselineStub());

                    ++frame;
                    JS_ASSERT(frame.isBaselineJS());

                    if (!PushBaselineFunApplyArguments(cx, frame, args, vp))
                        return false;
                }
            } else {
                JS_ASSERT(frame.type() == jit::IonFrame_Exit);

                ++frame;
                JS_ASSERT(frame.isBaselineStub());

                ++frame;
                JS_ASSERT(frame.isBaselineJS());

                if (!PushBaselineFunApplyArguments(cx, frame, args, vp))
                    return false;
            }
        } else
#endif
        {
            StackFrame *fp = cx->interpreterFrame();
            unsigned length = fp->numActualArgs();
            JS_ASSERT(length <= ARGS_LENGTH_MAX);

            if (!args.init(length))
                return false;

            
            args.setCallee(fval);
            args.setThis(vp[2]);

            
            fp->forEachUnaliasedActual(CopyTo(args.array()));
        }
    } else {
        
        if (!vp[3].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS, js_apply_str);
            return false;
        }

        



        RootedObject aobj(cx, &vp[3].toObject());
        uint32_t length;
        if (!GetLengthProperty(cx, aobj, &length))
            return false;

        
        if (length > ARGS_LENGTH_MAX) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_APPLY_ARGS);
            return false;
        }

        if (!args.init(length))
            return false;

        
        args.setCallee(fval);
        args.setThis(vp[2]);

        
        if (!GetElements(cx, aobj, length, args.array()))
            return false;
    }

    
    if (!Invoke(cx, args))
        return false;

    *vp = args.rval();
    return true;
}

static const uint32_t JSSLOT_BOUND_FUNCTION_THIS       = 0;
static const uint32_t JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 1;

static const uint32_t BOUND_FUNCTION_RESERVED_SLOTS = 2;

inline bool
JSFunction::initBoundFunction(JSContext *cx, HandleValue thisArg,
                              const Value *args, unsigned argslen)
{
    RootedFunction self(cx, this);

    




    if (!self->toDictionaryMode(cx))
        return false;

    if (!self->setFlag(cx, BaseShape::BOUND_FUNCTION))
        return false;

    if (!JSObject::setSlotSpan(cx, self, BOUND_FUNCTION_RESERVED_SLOTS + argslen))
        return false;

    self->setSlot(JSSLOT_BOUND_FUNCTION_THIS, thisArg);
    self->setSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT, PrivateUint32Value(argslen));

    self->initSlotRange(BOUND_FUNCTION_RESERVED_SLOTS, args, argslen);

    return true;
}

inline const js::Value &
JSFunction::getBoundFunctionThis() const
{
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_THIS);
}

inline const js::Value &
JSFunction::getBoundFunctionArgument(unsigned which) const
{
    JS_ASSERT(isBoundFunction());
    JS_ASSERT(which < getBoundFunctionArgumentCount());

    return getSlot(BOUND_FUNCTION_RESERVED_SLOTS + which);
}

inline size_t
JSFunction::getBoundFunctionArgumentCount() const
{
    JS_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT).toPrivateUint32();
}

 bool
JSFunction::createScriptForLazilyInterpretedFunction(JSContext *cx, HandleFunction fun)
{
    JS_ASSERT(fun->isInterpretedLazy());

    LazyScript *lazy = fun->lazyScriptOrNull();
    if (lazy) {
        
        if (cx->zone()->needsBarrier())
            LazyScript::writeBarrierPre(lazy);

        
        
        AutoSuppressGC suppressGC(cx);

        fun->flags &= ~INTERPRETED_LAZY;
        fun->flags |= INTERPRETED;

        JSScript *script = lazy->maybeScript();

        if (script) {
            fun->initScript(script);
            return true;
        }

        fun->initScript(NULL);

        if (fun != lazy->function()) {
            script = lazy->function()->getOrCreateScript(cx);
            if (!script) {
                fun->initLazyScript(lazy);
                return false;
            }
            fun->initScript(script);
            return true;
        }

        
        
        
        
        
        
        
        
        if (!lazy->numInnerFunctions() && !JS::IsIncrementalGCInProgress(cx->runtime())) {
            LazyScriptCache::Lookup lookup(cx, lazy);
            cx->runtime()->lazyScriptCache.lookup(lookup, &script);
        }

        if (script) {
            RootedObject enclosingScope(cx, lazy->enclosingScope());
            RootedScript scriptRoot(cx, script);
            RootedScript clonedScript(cx, CloneScript(cx, enclosingScope, fun, scriptRoot));
            if (!clonedScript) {
                fun->initLazyScript(lazy);
                return false;
            }

            clonedScript->setSourceObject(lazy->sourceObject());

            fun->initAtom(script->function()->displayAtom());
            fun->initScript(clonedScript);
            clonedScript->setFunction(fun);

            CallNewScriptHook(cx, clonedScript, fun);

            lazy->initScript(clonedScript);
            return true;
        }

        JS_ASSERT(lazy->source()->hasSourceData());

        
        const jschar *chars = lazy->source()->chars(cx);
        if (!chars) {
            fun->initLazyScript(lazy);
            return false;
        }

        const jschar *lazyStart = chars + lazy->begin();
        size_t lazyLength = lazy->end() - lazy->begin();

        if (!frontend::CompileLazyFunction(cx, lazy, lazyStart, lazyLength)) {
            fun->initLazyScript(lazy);
            return false;
        }

        script = fun->nonLazyScript();

        
        if (!lazy->numInnerFunctions()) {
            
            
            
            script->column = lazy->column();

            LazyScriptCache::Lookup lookup(cx, lazy);
            cx->runtime()->lazyScriptCache.insert(lookup, script);
        }

        
        
        lazy->initScript(script);
        return true;
    }

    
    JSFunctionSpec *fs = static_cast<JSFunctionSpec *>(fun->getExtendedSlot(0).toPrivate());
    RootedAtom funAtom(cx, Atomize(cx, fs->selfHostedName, strlen(fs->selfHostedName)));
    if (!funAtom)
        return false;
    Rooted<PropertyName *> funName(cx, funAtom->asPropertyName());
    return cx->runtime()->cloneSelfHostedFunctionScript(cx, funName, fun);
}


bool
js::CallOrConstructBoundFunction(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedFunction fun(cx, &args.callee().as<JSFunction>());
    JS_ASSERT(fun->isBoundFunction());

    bool constructing = args.isConstructing();
    if (constructing && fun->isArrow()) {
        




        return ReportIsNotFunction(cx, ObjectValue(*fun), -1, CONSTRUCT);
    }

    
    unsigned argslen = fun->getBoundFunctionArgumentCount();

    if (argc + argslen > ARGS_LENGTH_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    
    RootedObject target(cx, fun->getBoundFunctionTarget());

    
    const Value &boundThis = fun->getBoundFunctionThis();

    InvokeArgs invokeArgs(cx);
    if (!invokeArgs.init(argc + argslen))
        return false;

    
    for (unsigned i = 0; i < argslen; i++)
        invokeArgs[i].set(fun->getBoundFunctionArgument(i));
    PodCopy(invokeArgs.array() + argslen, vp + 2, argc);

    
    invokeArgs.setCallee(ObjectValue(*target));

    if (!constructing)
        invokeArgs.setThis(boundThis);

    if (constructing ? !InvokeConstructor(cx, invokeArgs) : !Invoke(cx, invokeArgs))
        return false;

    *vp = invokeArgs.rval();
    return true;
}

static bool
fun_isGenerator(JSContext *cx, unsigned argc, Value *vp)
{
    JSFunction *fun;
    if (!IsFunctionObject(vp[1], &fun)) {
        JS_SET_RVAL(cx, vp, BooleanValue(false));
        return true;
    }

    JS_SET_RVAL(cx, vp, BooleanValue(fun->isGenerator()));
    return true;
}


static bool
fun_bind(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    Value thisv = args.thisv();

    
    if (!js_IsCallable(thisv)) {
        ReportIncompatibleMethod(cx, args, &JSFunction::class_);
        return false;
    }

    
    Value *boundArgs = NULL;
    unsigned argslen = 0;
    if (args.length() > 1) {
        boundArgs = args.array() + 1;
        argslen = args.length() - 1;
    }

    
    RootedValue thisArg(cx, args.length() >= 1 ? args[0] : UndefinedValue());
    RootedObject target(cx, &thisv.toObject());
    JSObject *boundFunction = js_fun_bind(cx, target, thisArg, boundArgs, argslen);
    if (!boundFunction)
        return false;

    
    args.rval().setObject(*boundFunction);
    return true;
}

JSObject*
js_fun_bind(JSContext *cx, HandleObject target, HandleValue thisArg,
            Value *boundArgs, unsigned argslen)
{
    
    unsigned length = 0;
    if (target->is<JSFunction>()) {
        unsigned nargs = target->as<JSFunction>().nargs;
        if (nargs > argslen)
            length = nargs - argslen;
    }

    
    RootedAtom name(cx, target->is<JSFunction>() ? target->as<JSFunction>().atom() : NULL);

    RootedObject funobj(cx, NewFunction(cx, NullPtr(), CallOrConstructBoundFunction, length,
                                        JSFunction::NATIVE_CTOR, target, name));
    if (!funobj)
        return NULL;

    
    if (!JSObject::setParent(cx, funobj, target))
        return NULL;

    if (!funobj->as<JSFunction>().initBoundFunction(cx, thisArg, boundArgs, argslen))
        return NULL;

    
    
    return funobj;
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

const JSFunctionSpec js::function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0),
    JS_FN(js_apply_str,      js_fun_apply,   2,0),
    JS_FN(js_call_str,       js_fun_call,    1,0),
    JS_FN("bind",            fun_bind,       1,0),
    JS_FN("isGenerator",     fun_isGenerator,0,0),
    JS_FS_END
};

static bool
FunctionConstructor(JSContext *cx, unsigned argc, Value *vp, GeneratorKind generatorKind)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString arg(cx);   

    
    Rooted<GlobalObject*> global(cx, &args.callee().global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, global)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_FUNCTION);
        return false;
    }

    AutoKeepAtoms keepAtoms(cx->perThreadData);
    AutoNameVector formals(cx);

    bool hasRest = false;

    bool isStarGenerator = generatorKind == StarGenerator;
    JS_ASSERT(generatorKind != LegacyGenerator);

    const char *filename;
    unsigned lineno;
    JSPrincipals *originPrincipals;
    CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals);
    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setOriginPrincipals(originPrincipals)
           .setFileAndLine(filename, lineno)
           .setNoScriptRval(false)
           .setCompileAndGo(true);

    unsigned n = args.length() ? args.length() - 1 : 0;
    if (n > 0) {
        









        size_t args_length = 0;
        for (unsigned i = 0; i < n; i++) {
            
            arg = ToString<CanGC>(cx, args[i]);
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
        StableCharPtr collected_args(cp, args_length + 1);

        


        for (unsigned i = 0; i < n; i++) {
            arg = args[i].toString();
            size_t arg_length = arg->length();
            const jschar *arg_chars = arg->getChars(cx);
            if (!arg_chars)
                return false;
            (void) js_strncpy(cp, arg_chars, arg_length);
            cp += arg_length;

            
            *cp++ = (i + 1 < n) ? ',' : 0;
        }

        






        TokenStream ts(cx, options, collected_args.get(), args_length,
                        NULL);
        bool yieldIsValidName = ts.versionNumber() < JSVERSION_1_7 && !isStarGenerator;

        
        TokenKind tt = ts.getToken();
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (hasRest) {
                    ts.reportError(JSMSG_PARAMETER_AFTER_REST);
                    return false;
                }

                if (tt == TOK_YIELD && yieldIsValidName)
                    tt = TOK_NAME;

                if (tt != TOK_NAME) {
                    if (tt == TOK_TRIPLEDOT) {
                        hasRest = true;
                        tt = ts.getToken();
                        if (tt == TOK_YIELD && yieldIsValidName)
                            tt = TOK_NAME;
                        if (tt != TOK_NAME) {
                            if (tt != TOK_ERROR)
                                ts.reportError(JSMSG_NO_REST_NAME);
                            return false;
                        }
                    } else {
                        return OnBadFormal(cx, tt);
                    }
                }

                if (!formals.append(ts.currentName()))
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

#ifdef DEBUG
    for (unsigned i = 0; i < formals.length(); ++i) {
        JSString *str = formals[i];
        JS_ASSERT(str->asAtom().asPropertyName() == formals[i]);
    }
#endif

    RootedString str(cx);
    if (!args.length())
        str = cx->runtime()->emptyString;
    else
        str = ToString<CanGC>(cx, args[args.length() - 1]);
    if (!str)
        return false;
    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return false;

    JS::Anchor<JSString *> strAnchor(str);
    const jschar *chars = linear->chars();
    size_t length = linear->length();

    
    SkipRoot skip(cx, &chars);

    





    RootedAtom anonymousAtom(cx, cx->names().anonymous);
    JSObject *proto = NULL;
    if (isStarGenerator) {
        proto = global->getOrCreateStarGeneratorFunctionPrototype(cx);
        if (!proto)
            return false;
    }
    RootedFunction fun(cx, NewFunctionWithProto(cx, NullPtr(), NULL, 0,
                                                JSFunction::INTERPRETED_LAMBDA, global,
                                                anonymousAtom, proto,
                                                JSFunction::FinalizeKind, TenuredObject));
    if (!fun)
        return false;

    if (hasRest)
        fun->setHasRest();

    bool ok;
    if (isStarGenerator)
        ok = frontend::CompileStarGeneratorBody(cx, &fun, options, formals, chars, length);
    else
        ok = frontend::CompileFunctionBody(cx, &fun, options, formals, chars, length);
    args.rval().setObject(*fun);
    return ok;
}

bool
js::Function(JSContext *cx, unsigned argc, Value *vp)
{
    return FunctionConstructor(cx, argc, vp, NotGenerator);
}

bool
js::Generator(JSContext *cx, unsigned argc, Value *vp)
{
    return FunctionConstructor(cx, argc, vp, StarGenerator);
}

bool
JSFunction::isBuiltinFunctionConstructor()
{
    return maybeNative() == Function || maybeNative() == Generator;
}

JSFunction *
js::NewFunction(ExclusiveContext *cx, HandleObject funobjArg, Native native, unsigned nargs,
                JSFunction::Flags flags, HandleObject parent, HandleAtom atom,
                gc::AllocKind allocKind ,
                NewObjectKind newKind )
{
    return NewFunctionWithProto(cx, funobjArg, native, nargs, flags, parent, atom, NULL,
                                allocKind, newKind);
}

JSFunction *
js::NewFunctionWithProto(ExclusiveContext *cx, HandleObject funobjArg, Native native,
                         unsigned nargs, JSFunction::Flags flags, HandleObject parent,
                         HandleAtom atom, JSObject *proto,
                         gc::AllocKind allocKind ,
                         NewObjectKind newKind )
{
    JS_ASSERT(allocKind == JSFunction::FinalizeKind || allocKind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT(sizeof(JSFunction) <= gc::Arena::thingSize(JSFunction::FinalizeKind));
    JS_ASSERT(sizeof(FunctionExtended) <= gc::Arena::thingSize(JSFunction::ExtendedFinalizeKind));

    RootedObject funobj(cx, funobjArg);
    if (funobj) {
        JS_ASSERT(funobj->is<JSFunction>());
        JS_ASSERT(funobj->getParent() == parent);
        JS_ASSERT_IF(native && cx->typeInferenceEnabled(), funobj->hasSingletonType());
    } else {
        
        
        
        if (native && !IsAsmJSModuleNative(native))
            newKind = SingletonObject;
        funobj = NewObjectWithClassProto(cx, &JSFunction::class_, proto,
                                         SkipScopeParent(parent), allocKind, newKind);
        if (!funobj)
            return NULL;
    }
    RootedFunction fun(cx, &funobj->as<JSFunction>());

    
    fun->nargs = uint16_t(nargs);
    fun->flags = flags;
    if (fun->isInterpreted()) {
        JS_ASSERT(!native);
        fun->mutableScript().init(NULL);
        fun->initEnvironment(parent);
    } else {
        JS_ASSERT(fun->isNative());
        JS_ASSERT(native);
        fun->initNative(native, NULL);
    }
    if (allocKind == JSFunction::ExtendedFinalizeKind) {
        fun->flags |= JSFunction::EXTENDED;
        fun->initializeExtended();
    }
    fun->initAtom(atom);

    return fun;
}

JSFunction *
js::CloneFunctionObject(JSContext *cx, HandleFunction fun, HandleObject parent, gc::AllocKind allocKind,
                        NewObjectKind newKindArg )
{
    JS_ASSERT(parent);
    JS_ASSERT(!fun->isBoundFunction());

    bool useSameScript = cx->compartment() == fun->compartment() &&
                         !fun->hasSingletonType() &&
                         !types::UseNewTypeForClone(fun);

    if (!useSameScript && fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
        return NULL;

    NewObjectKind newKind = useSameScript ? newKindArg : SingletonObject;
    JSObject *cloneProto = NULL;
    if (fun->isStarGenerator()) {
        cloneProto = cx->global()->getOrCreateStarGeneratorFunctionPrototype(cx);
        if (!cloneProto)
            return NULL;
    }
    JSObject *cloneobj = NewObjectWithClassProto(cx, &JSFunction::class_, cloneProto,
                                                 SkipScopeParent(parent), allocKind, newKind);
    if (!cloneobj)
        return NULL;
    RootedFunction clone(cx, &cloneobj->as<JSFunction>());

    clone->nargs = fun->nargs;
    clone->flags = fun->flags & ~JSFunction::EXTENDED;
    if (fun->hasScript()) {
        clone->initScript(fun->nonLazyScript());
        clone->initEnvironment(parent);
    } else if (fun->isInterpretedLazy()) {
        LazyScript *lazy = fun->lazyScriptOrNull();
        clone->initLazyScript(lazy);
        clone->initEnvironment(parent);
    } else {
        clone->initNative(fun->native(), fun->jitInfo());
    }
    clone->initAtom(fun->displayAtom());

    if (allocKind == JSFunction::ExtendedFinalizeKind) {
        clone->flags |= JSFunction::EXTENDED;
        if (fun->isExtended() && fun->compartment() == cx->compartment()) {
            for (unsigned i = 0; i < FunctionExtended::NUM_EXTENDED_SLOTS; i++)
                clone->initExtendedSlot(i, fun->getExtendedSlot(i));
        } else {
            clone->initializeExtended();
        }
    }

    if (useSameScript) {
        



        if (fun->getProto() == clone->getProto())
            clone->setType(fun->type());
        return clone;
    }

    RootedFunction cloneRoot(cx, clone);

    





    if (cloneRoot->isInterpreted() && !CloneFunctionScript(cx, fun, cloneRoot, newKindArg))
        return NULL;

    return cloneRoot;
}

JSFunction *
js::DefineFunction(JSContext *cx, HandleObject obj, HandleId id, Native native,
                   unsigned nargs, unsigned flags, AllocKind allocKind ,
                   NewObjectKind newKind )
{
    PropertyOp gop;
    StrictPropertyOp sop;

    RootedFunction fun(cx);

    if (flags & JSFUN_STUB_GSOPS) {
        





        flags &= ~JSFUN_STUB_GSOPS;
        gop = JS_PropertyStub;
        sop = JS_StrictPropertyStub;
    } else {
        gop = NULL;
        sop = NULL;
    }

    JSFunction::Flags funFlags;
    if (!native)
        funFlags = JSFunction::INTERPRETED_LAZY;
    else
        funFlags = JSAPIToJSFunctionFlags(flags);
    RootedAtom atom(cx, JSID_IS_ATOM(id) ? JSID_TO_ATOM(id) : NULL);
    fun = NewFunction(cx, NullPtr(), native, nargs, funFlags, obj, atom, allocKind, newKind);
    if (!fun)
        return NULL;

    RootedValue funVal(cx, ObjectValue(*fun));
    if (!JSObject::defineGeneric(cx, obj, id, funVal, gop, sop, flags & ~JSFUN_FLAGS_MASK))
        return NULL;

    return fun;
}

bool
js::IsConstructor(const Value &v)
{
    
    if (!v.isObject())
        return false;

    
    
    JSObject &obj = v.toObject();
    if (obj.is<JSFunction>()) {
        JSFunction &fun = obj.as<JSFunction>();
        return fun.isNativeConstructor() || fun.isInterpretedConstructor();
    }
    return obj.getClass()->construct != NULL;
}

void
js::ReportIncompatibleMethod(JSContext *cx, CallReceiver call, const Class *clasp)
{
    RootedValue thisv(cx, call.thisv());

#ifdef DEBUG
    if (thisv.isObject()) {
        JS_ASSERT(thisv.toObject().getClass() != clasp ||
                  !thisv.toObject().isNative() ||
                  !thisv.toObject().getProto() ||
                  thisv.toObject().getProto()->getClass() != clasp);
    } else if (thisv.isString()) {
        JS_ASSERT(clasp != &StringObject::class_);
    } else if (thisv.isNumber()) {
        JS_ASSERT(clasp != &NumberObject::class_);
    } else if (thisv.isBoolean()) {
        JS_ASSERT(clasp != &BooleanObject::class_);
    } else {
        JS_ASSERT(thisv.isUndefined() || thisv.isNull());
    }
#endif

    if (JSFunction *fun = ReportIfNotFunction(cx, call.calleev())) {
        JSAutoByteString funNameBytes;
        if (const char *funName = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                                 clasp->name, funName, InformalValueTypeName(thisv));
        }
    }
}

void
js::ReportIncompatible(JSContext *cx, CallReceiver call)
{
    if (JSFunction *fun = ReportIfNotFunction(cx, call.calleev())) {
        JSAutoByteString funNameBytes;
        if (const char *funName = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_METHOD,
                                 funName, "method", InformalValueTypeName(call.thisv()));
        }
    }
}

bool
JSObject::hasIdempotentProtoChain() const
{
    
    
    JSObject *obj = const_cast<JSObject *>(this);
    while (true) {
        if (!obj->isNative())
            return false;

        JSResolveOp resolve = obj->getClass()->resolve;
        if (resolve != JS_ResolveStub && resolve != (JSResolveOp) js::fun_resolve)
            return false;

        if (obj->getOps()->lookupProperty || obj->getOps()->lookupGeneric || obj->getOps()->lookupElement)
            return false;

        obj = obj->getProto();
        if (!obj)
            return true;
    }

    MOZ_ASSUME_UNREACHABLE("Should not get here");
}

namespace JS {
namespace detail {

JS_PUBLIC_API(void)
CheckIsValidConstructible(Value calleev)
{
    JSObject *callee = &calleev.toObject();
    if (callee->is<JSFunction>())
        JS_ASSERT(callee->as<JSFunction>().isNativeConstructor());
    else
        JS_ASSERT(callee->getClass()->construct != NULL);
}

} 
} 
