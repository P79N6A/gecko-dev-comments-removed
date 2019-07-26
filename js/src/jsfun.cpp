









#include <string.h>

#include "mozilla/RangedPtr.h"
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
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jspropertytree.h"
#include "jsproxy.h"
#include "jsscript.h"
#include "jsstr.h"

#include "builtin/Eval.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#include "vm/Debugger.h"
#include "vm/ScopeObject.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"
#include "vm/Xdr.h"

#ifdef JS_METHODJIT
#include "methodjit/MethodJIT.h"
#endif

#include "jsatominlines.h"
#include "jsfuninlines.h"
#include "jsinferinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/ArgumentsObject-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/Stack-inl.h"

#ifdef JS_ION
#include "ion/Ion.h"
#include "ion/IonFrameIterator.h"
#include "ion/IonFrameIterator-inl.h"
#endif

using namespace js;
using namespace js::gc;
using namespace js::types;
using namespace js::frontend;

using mozilla::ArrayLength;

static JSBool
fun_getProperty(JSContext *cx, HandleObject obj_, HandleId id, MutableHandleValue vp)
{
    RootedObject obj(cx, obj_);
    while (!obj->isFunction()) {
        if (!JSObject::getProto(cx, obj, &obj))
            return false;
        if (!obj)
            return true;
    }
    RootedFunction fun(cx, obj->toFunction());

    





    if (fun->isInterpreted()) {
        if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
            return false;
        fun->nonLazyScript()->uninlineable = true;
        MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);

        
        if (fun->nonLazyScript()->isCallsiteClone) {
            RootedFunction original(cx, fun->nonLazyScript()->originalFunction());
            original->nonLazyScript()->uninlineable = true;
            MarkTypeObjectFlags(cx, original, OBJECT_FLAG_UNINLINEABLE);
        }
    }

    
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
        
        
        
        
        RawScript script = iter.script();
        ion::ForbidCompilation(cx, script);
#endif

        vp.setObject(*argsobj);
        return true;
    }

#ifdef JS_METHODJIT
    StackFrame *fp = NULL;
    if (iter.isScript() && !iter.isIon())
        fp = iter.interpFrame();

    if (JSID_IS_ATOM(id, cx->names().caller) && fp && fp->prev()) {
        




        InlinedSite *inlined;
        jsbytecode *prevpc = fp->prevpc(&inlined);
        if (inlined) {
            mjit::JITChunk *chunk = fp->prev()->jit()->chunk(prevpc);
            RawFunction fun = chunk->inlineFrames()[inlined->inlineIndex].fun;
            fun->nonLazyScript()->uninlineable = true;
            MarkTypeObjectFlags(cx, fun, OBJECT_FLAG_UNINLINEABLE);
        }
    }
#endif

    if (JSID_IS_ATOM(id, cx->names().caller)) {
        ++iter;
        if (iter.done() || !iter.isFunctionFrame()) {
            JS_ASSERT(vp.isNull());
            return true;
        }

        
        JSObject &maybeClone = iter.calleev().toObject();
        if (maybeClone.isFunction() && maybeClone.toFunction()->nonLazyScript()->isCallsiteClone)
            vp.setObject(*maybeClone.toFunction()->nonLazyScript()->originalFunction());
        else
            vp.set(iter.calleev());

        if (!cx->compartment->wrap(cx, vp))
            return false;

        


        JSObject &caller = vp.toObject();
        if (caller.isWrapper() && !Wrapper::wrapperHandler(&caller)->isSafeToUnwrap()) {
            vp.setNull();
        } else if (caller.isFunction()) {
            JSFunction *callerFun = caller.toFunction();
            if (callerFun->isInterpreted() && callerFun->strict()) {
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
    NAME_OFFSET(arguments),
    NAME_OFFSET(caller),
};

static JSBool
fun_enumerate(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isFunction());

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
        id = NameToId(OFFSET_TO_NAME(cx->runtime, offset));
        if (!JSObject::hasProperty(cx, obj, id, &found, 0))
            return false;
    }

    return true;
}

static JSObject *
ResolveInterpretedFunctionPrototype(JSContext *cx, HandleObject obj)
{
#ifdef DEBUG
    JSFunction *fun = obj->toFunction();
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->isFunctionPrototype());
#endif

    




    JS_ASSERT(!IsInternalFunctionObject(obj));
    JS_ASSERT(!obj->isBoundFunction());

    



    RawObject objProto = obj->global().getOrCreateObjectPrototype(cx);
    if (!objProto)
        return NULL;
    RootedObject proto(cx, NewObjectWithGivenProto(cx, &ObjectClass, objProto, NULL, SingletonObject));
    if (!proto)
        return NULL;

    





    RootedValue protoVal(cx, ObjectValue(*proto));
    RootedValue objVal(cx, ObjectValue(*obj));
    if (!JSObject::defineProperty(cx, obj, cx->names().classPrototype,
                                  protoVal, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT) ||
        !JSObject::defineProperty(cx, proto, cx->names().constructor,
                                  objVal, JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
       return NULL;
    }

    return proto;
}

static JSBool
fun_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
            MutableHandleObject objp)
{
    if (!JSID_IS_ATOM(id))
        return true;

    RootedFunction fun(cx, obj->toFunction());

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
            uint16_t ndefaults = fun->hasScript() ? fun->nonLazyScript()->ndefaults : 0;
            v.setInt32(fun->nargs - ndefaults - fun->hasRest());
        } else {
            v.setString(fun->atom() == NULL ?  cx->runtime->emptyString : fun->atom());
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

        if (JSID_IS_ATOM(id, OFFSET_TO_NAME(cx->runtime, offset))) {
            JS_ASSERT(!IsInternalFunctionObject(fun));

            PropertyOp getter;
            StrictPropertyOp setter;
            unsigned attrs = JSPROP_PERMANENT;
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
    
    RootedAtom atom(xdr->cx());
    uint32_t firstword;           


    uint32_t flagsword;           

    JSContext *cx = xdr->cx();
    RootedFunction fun(cx);
    RootedScript script(cx);
    if (mode == XDR_ENCODE) {
        fun = objp->toFunction();
        if (!fun->isInterpreted()) {
            JSAutoByteString funNameBytes;
            if (const char *name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_SCRIPTED_FUNCTION,
                                     name);
            }
            return false;
        }
        firstword = !!fun->atom();
        flagsword = (fun->nargs << 16) | fun->flags;
        atom = fun->atom();
        script = fun->nonLazyScript();
    } else {
        fun = NewFunction(cx, NullPtr(), NULL, 0, JSFunction::INTERPRETED, NullPtr(), NullPtr());
        if (!fun)
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
js::CloneInterpretedFunction(JSContext *cx, HandleObject enclosingScope, HandleFunction srcFun)
{
    

    RootedFunction clone(cx, NewFunction(cx, NullPtr(), NULL, 0,
                                         JSFunction::INTERPRETED, NullPtr(), NullPtr()));
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






static JSBool
fun_hasInstance(JSContext *cx, HandleObject objArg, MutableHandleValue v, JSBool *bp)
{
    RootedObject obj(cx, objArg);

    while (obj->isFunction() && obj->isBoundFunction())
        obj = obj->toFunction()->getBoundFunctionTarget();

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
        if (hasScript())
            MarkScriptUnbarriered(trc, &u.i.script_, "script");
        if (u.i.env_)
            MarkObjectUnbarriered(trc, &u.i.env_, "fun_callscope");
    }
}

static void
fun_trace(JSTracer *trc, RawObject obj)
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
    fun_hasInstance,
    NULL,                    
    fun_trace
};



static bool
FindBody(JSContext *cx, HandleFunction fun, StableCharPtr chars, size_t length,
         size_t *bodyStart, size_t *bodyEnd)
{
    
    CompileOptions options(cx);
    options.setFileAndLine("internal-findBody", 0)
           .setVersion(fun->nonLazyScript()->getVersion());
    TokenStream ts(cx, options, chars.get(), length, NULL);
    int nest = 0;
    bool onward = true;
    
    do {
        switch (ts.getToken()) {
          case TOK_NAME:
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
    StringBuffer out(cx);
    RootedScript script(cx);

    
    
    if (fun->isBoundFunction()) {
        JSObject *target = fun->getBoundFunctionTarget();
        if (target->isFunction() && target->toFunction()->isArrow()) {
            RootedFunction targetfun(cx, target->toFunction());
            return FunctionToString(cx, targetfun, bodyOnly, lambdaParen);
        }
    }

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
            if (!out.append("function "))
                return NULL;
        }
        if (fun->atom()) {
            if (!out.append(fun->atom()))
                return NULL;
        }
    }
    bool haveSource = fun->isInterpreted() && !fun->isSelfHostedBuiltin();
    if (haveSource && !script->scriptSource()->hasSourceData() &&
        !JSScript::loadSource(cx, script, &haveSource))
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
        
        
        bool funCon = script->sourceStart == 0 && script->scriptSource()->argumentsNotIncluded();

        
        
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
    if (!obj->isFunction()) {
        if (IsFunctionProxy(obj))
            return Proxy::fun_toString(cx, obj, indent);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             "object");
        return NULL;
    }

    RootedFunction fun(cx, obj->toFunction());
    return FunctionToString(cx, fun, false, indent != JS_DONT_PRETTY_PRINT);
}

static JSBool
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
static JSBool
fun_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(IsFunctionObject(args.calleev()));

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedString str(cx, fun_toStringHelper(cx, obj, JS_DONT_PRETTY_PRINT));
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}
#endif

JSBool
js_fun_call(JSContext *cx, unsigned argc, Value *vp)
{
    RootedValue fval(cx, vp[1]);

    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &FunctionClass);
        return false;
    }

    Value *argv = vp + 2;
    RootedValue thisv(cx, UndefinedValue());
    if (argc != 0) {
        thisv = argv[0];

        argc--;
        argv++;
    }

    
    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc, &args))
        return JS_FALSE;

    
    args.setCallee(fval);
    args.setThis(thisv);
    PodCopy(args.array(), argv, argc);

    bool ok = Invoke(cx, args);
    *vp = args.rval();
    return ok;
}


JSBool
js_fun_apply(JSContext *cx, unsigned argc, Value *vp)
{
    
    RootedValue fval(cx, vp[1]);
    if (!js_IsCallable(fval)) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &FunctionClass);
        return false;
    }

    
    if (argc < 2 || vp[3].isNullOrUndefined())
        return js_fun_call(cx, (argc > 0) ? 1 : 0, vp);

    InvokeArgsGuard args;

    




    if (vp[3].isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        





        
        StackFrame *fp = cx->fp();

#ifdef JS_ION
        
        
        
        if (fp->beginsIonActivation()) {
            ion::IonActivationIterator activations(cx);
            ion::IonFrameIterator frame(activations);
            JS_ASSERT(frame.isNative());
            
            ++frame;
            ion::InlineFrameIterator iter(cx, &frame);

            unsigned length = iter.numActualArgs();
            JS_ASSERT(length <= StackSpace::ARGS_LENGTH_MAX);

            if (!cx->stack.pushInvokeArgs(cx, length, &args))
                return false;

            
            args.setCallee(fval);
            args.setThis(vp[2]);

            
            iter.forEachCanonicalActualArg(cx, CopyTo(args.array()), 0, -1);
        } else
#endif
        {
            unsigned length = fp->numActualArgs();
            JS_ASSERT(length <= StackSpace::ARGS_LENGTH_MAX);

            if (!cx->stack.pushInvokeArgs(cx, length, &args))
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

        
        if (length > StackSpace::ARGS_LENGTH_MAX) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_APPLY_ARGS);
            return false;
        }

        if (!cx->stack.pushInvokeArgs(cx, length, &args))
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
    JS_ASSERT(isFunction());

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

bool
JSFunction::initializeLazyScript(JSContext *cx)
{
    JS_ASSERT(isInterpretedLazy());
    JSFunctionSpec *fs = static_cast<JSFunctionSpec *>(getExtendedSlot(0).toPrivate());
    RootedAtom funAtom(cx, Atomize(cx, fs->selfHostedName, strlen(fs->selfHostedName)));
    if (!funAtom)
        return false;
    Rooted<PropertyName *> funName(cx, funAtom->asPropertyName());
    Rooted<JSFunction*> self(cx, this);
    return cx->runtime->cloneSelfHostedFunctionScript(cx, funName, self);
}


JSBool
js::CallOrConstructBoundFunction(JSContext *cx, unsigned argc, Value *vp)
{
    RootedFunction fun(cx, vp[0].toObject().toFunction());
    JS_ASSERT(fun->isBoundFunction());

    bool constructing = IsConstructing(vp);

    
    unsigned argslen = fun->getBoundFunctionArgumentCount();

    if (argc + argslen > StackSpace::ARGS_LENGTH_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    
    RootedObject target(cx, fun->getBoundFunctionTarget());

    
    const Value &boundThis = fun->getBoundFunctionThis();

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, argc + argslen, &args))
        return false;

    
    for (unsigned i = 0; i < argslen; i++)
        args[i] = fun->getBoundFunctionArgument(i);
    PodCopy(args.array() + argslen, vp + 2, argc);

    
    args.setCallee(ObjectValue(*target));

    if (!constructing)
        args.setThis(boundThis);

    if (constructing ? !InvokeConstructor(cx, args) : !Invoke(cx, args))
        return false;

    *vp = args.rval();
    return true;
}

#if JS_HAS_GENERATORS
static JSBool
fun_isGenerator(JSContext *cx, unsigned argc, Value *vp)
{
    RawFunction fun;
    if (!IsFunctionObject(vp[1], &fun)) {
        JS_SET_RVAL(cx, vp, BooleanValue(false));
        return true;
    }

    bool result = false;
    if (fun->hasScript()) {
        RawScript script = fun->nonLazyScript();
        JS_ASSERT(script->length != 0);
        result = script->isGenerator;
    }

    JS_SET_RVAL(cx, vp, BooleanValue(result));
    return true;
}
#endif


static JSBool
fun_bind(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    Value thisv = args.thisv();

    
    if (!js_IsCallable(thisv)) {
        ReportIncompatibleMethod(cx, args, &FunctionClass);
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
    RawObject boundFunction = js_fun_bind(cx, target, thisArg, boundArgs, argslen);
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
    if (target->isFunction()) {
        unsigned nargs = target->toFunction()->nargs;
        if (nargs > argslen)
            length = nargs - argslen;
    }

    
    RootedAtom name(cx, target->isFunction() ? target->toFunction()->atom() : NULL);

    RootedObject funobj(cx, NewFunction(cx, NullPtr(), CallOrConstructBoundFunction, length,
                                        JSFunction::NATIVE_CTOR, target, name));
    if (!funobj)
        return NULL;

    
    if (!JSObject::setParent(cx, funobj, target))
        return NULL;

    if (!funobj->toFunction()->initBoundFunction(cx, thisArg, boundArgs, argslen))
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

JSFunctionSpec js::function_methods[] = {
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
js::Function(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString arg(cx);   

    
    Rooted<GlobalObject*> global(cx, &args.callee().global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, global)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_FUNCTION);
        return false;
    }

    AutoKeepAtoms keepAtoms(cx->runtime);
    AutoNameVector formals(cx);

    bool hasRest = false;

    const char *filename;
    unsigned lineno;
    JSPrincipals *originPrincipals;
    CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals);
    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setOriginPrincipals(originPrincipals)
           .setFileAndLine(filename, lineno);

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

        






        TokenStream ts(cx, options, collected_args.get(), args_length,  NULL);

        
        TokenKind tt = ts.getToken();
        if (tt != TOK_EOF) {
            for (;;) {
                



                if (hasRest) {
                    ts.reportError(JSMSG_PARAMETER_AFTER_REST);
                    return false;
                }

                if (tt != TOK_NAME) {
                    if (tt == TOK_TRIPLEDOT) {
                        hasRest = true;
                        tt = ts.getToken();
                        if (tt != TOK_NAME) {
                            if (tt != TOK_ERROR)
                                ts.reportError(JSMSG_NO_REST_NAME);
                            return false;
                        }
                    } else {
                        return OnBadFormal(cx, tt);
                    }
                }

                if (!formals.append(ts.currentToken().name()))
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
        RawString str = formals[i];
        JS_ASSERT(str->asAtom().asPropertyName() == formals[i]);
    }
#endif

    RootedString str(cx);
    if (!args.length())
        str = cx->runtime->emptyString;
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

    





    RootedAtom anonymousAtom(cx, cx->names().anonymous);
    RootedFunction fun(cx, NewFunction(cx, NullPtr(), NULL, 0, JSFunction::INTERPRETED_LAMBDA,
                                       global, anonymousAtom));
    if (!fun)
        return false;

    if (hasRest)
        fun->setHasRest();

    bool ok = frontend::CompileFunctionBody(cx, fun, options, formals, chars, length);
    args.rval().setObject(*fun);
    return ok;
}

bool
js::IsBuiltinFunctionConstructor(JSFunction *fun)
{
    return fun->maybeNative() == Function;
}

JSFunction *
js::NewFunction(JSContext *cx, HandleObject funobjArg, Native native, unsigned nargs,
                JSFunction::Flags flags, HandleObject parent, HandleAtom atom,
                gc::AllocKind allocKind ,
                NewObjectKind newKind )
{
    JS_ASSERT(allocKind == JSFunction::FinalizeKind || allocKind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT(sizeof(JSFunction) <= gc::Arena::thingSize(JSFunction::FinalizeKind));
    JS_ASSERT(sizeof(FunctionExtended) <= gc::Arena::thingSize(JSFunction::ExtendedFinalizeKind));

    RootedObject funobj(cx, funobjArg);
    if (funobj) {
        JS_ASSERT(funobj->isFunction());
        JS_ASSERT(funobj->getParent() == parent);
        JS_ASSERT_IF(native && cx->typeInferenceEnabled(), funobj->hasSingletonType());
    } else {
        if (native)
            newKind = SingletonObject;
        funobj = NewObjectWithClassProto(cx, &FunctionClass, NULL, SkipScopeParent(parent), allocKind, newKind);
        if (!funobj)
            return NULL;
    }
    RootedFunction fun(cx, funobj->toFunction());

    
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
js::CloneFunctionObject(JSContext *cx, HandleFunction fun, HandleObject parent, gc::AllocKind allocKind)
{
    JS_ASSERT(parent);
    JS_ASSERT(!fun->isBoundFunction());

    bool useSameScript = cx->compartment == fun->compartment() &&
                         !fun->hasSingletonType() &&
                         !types::UseNewTypeForClone(fun);
    NewObjectKind newKind = useSameScript ? GenericObject : SingletonObject;
    JSObject *cloneobj = NewObjectWithClassProto(cx, &FunctionClass, NULL, SkipScopeParent(parent),
                                                 allocKind, newKind);
    if (!cloneobj)
        return NULL;
    RootedFunction clone(cx, cloneobj->toFunction());

    clone->nargs = fun->nargs;
    clone->flags = fun->flags & ~JSFunction::EXTENDED;
    if (fun->isInterpreted()) {
        if (fun->isInterpretedLazy()) {
            RootedFunction cloneRoot(cx, clone);
            AutoCompartment ac(cx, fun);
            if (!fun->getOrCreateScript(cx))
                return NULL;
            clone = cloneRoot;
        }
        clone->initScript(fun->nonLazyScript());
        clone->initEnvironment(parent);
    } else {
        clone->initNative(fun->native(), fun->jitInfo());
    }
    clone->initAtom(fun->displayAtom());

    if (allocKind == JSFunction::ExtendedFinalizeKind) {
        clone->flags |= JSFunction::EXTENDED;
        clone->initializeExtended();
    }

    if (useSameScript) {
        



        if (fun->getProto() == clone->getProto())
            clone->setType(fun->type());
        return clone;
    }

    RootedFunction cloneRoot(cx, clone);

    





    if (cloneRoot->isInterpreted() && !CloneFunctionScript(cx, fun, cloneRoot))
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

void
js::ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp)
{
    RootedValue thisv(cx, call.thisv());

#ifdef DEBUG
    if (thisv.isObject()) {
        JS_ASSERT(thisv.toObject().getClass() != clasp ||
                  !thisv.toObject().isNative() ||
                  !thisv.toObject().getProto() ||
                  thisv.toObject().getProto()->getClass() != clasp);
    } else if (thisv.isString()) {
        JS_ASSERT(clasp != &StringClass);
    } else if (thisv.isNumber()) {
        JS_ASSERT(clasp != &NumberClass);
    } else if (thisv.isBoolean()) {
        JS_ASSERT(clasp != &BooleanClass);
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
    
    
    RawObject obj = const_cast<RawObject>(this);
    while (true) {
        if (!obj->isNative())
            return false;

        JSResolveOp resolve = obj->getClass()->resolve;
        if (resolve != JS_ResolveStub && resolve != (JSResolveOp) fun_resolve)
            return false;

        if (obj->getOps()->lookupProperty || obj->getOps()->lookupGeneric || obj->getOps()->lookupElement)
            return false;

        obj = obj->getProto();
        if (!obj)
            return true;
    }

    JS_NOT_REACHED("Should not get here");
    return false;
}

