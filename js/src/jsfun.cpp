









#include "jsfuninlines.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Range.h"

#include <string.h>

#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jswrapper.h"

#include "builtin/Eval.h"
#include "builtin/Object.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#include "jit/Ion.h"
#include "jit/JitFrameIterator.h"
#include "js/CallNonGenericMethod.h"
#include "js/Proxy.h"
#include "vm/GlobalObject.h"
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
using namespace js::frontend;

using mozilla::ArrayLength;
using mozilla::PodCopy;
using mozilla::RangedPtr;

static bool
fun_enumerate(JSContext* cx, HandleObject obj)
{
    MOZ_ASSERT(obj->is<JSFunction>());

    RootedId id(cx);
    bool found;

    if (!obj->isBoundFunction() && !obj->as<JSFunction>().isArrow()) {
        id = NameToId(cx->names().prototype);
        if (!HasProperty(cx, obj, id, &found))
            return false;
    }

    id = NameToId(cx->names().length);
    if (!HasProperty(cx, obj, id, &found))
        return false;

    id = NameToId(cx->names().name);
    if (!HasProperty(cx, obj, id, &found))
        return false;

    return true;
}

bool
IsFunction(HandleValue v)
{
    return v.isObject() && v.toObject().is<JSFunction>();
}

static bool
AdvanceToActiveCallLinear(JSContext* cx, NonBuiltinScriptFrameIter& iter, HandleFunction fun)
{
    MOZ_ASSERT(!fun->isBuiltin());
    MOZ_ASSERT(!fun->isBoundFunction(), "all bound functions are currently native (ergo builtin)");

    for (; !iter.done(); ++iter) {
        if (!iter.isFunctionFrame() || iter.isEvalFrame())
            continue;
        if (iter.matchCallee(cx, fun))
            return true;
    }
    return false;
}

static void
ThrowTypeErrorBehavior(JSContext* cx)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, GetErrorMessage, nullptr,
                                 JSMSG_THROW_TYPE_ERROR);
}







static bool
ArgumentsRestrictions(JSContext* cx, HandleFunction fun)
{
    
    
    
    if (fun->isBuiltin() ||
        (fun->isInterpreted() && fun->strict()) ||
        fun->isBoundFunction())
    {
        ThrowTypeErrorBehavior(cx);
        return false;
    }

    
    
    if (fun->hasRest()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_FUNCTION_ARGUMENTS_AND_REST);
        return false;
    }

    
    
    if (!JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING | JSREPORT_STRICT, GetErrorMessage,
                                      nullptr, JSMSG_DEPRECATED_USAGE, js_arguments_str))
    {
        return false;
    }

    return true;
}

bool
ArgumentsGetterImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsFunction(args.thisv()));

    RootedFunction fun(cx, &args.thisv().toObject().as<JSFunction>());
    if (!ArgumentsRestrictions(cx, fun))
        return false;

    
    NonBuiltinScriptFrameIter iter(cx);
    if (!AdvanceToActiveCallLinear(cx, iter, fun)) {
        args.rval().setNull();
        return true;
    }

    Rooted<ArgumentsObject*> argsobj(cx, ArgumentsObject::createUnexpected(cx, iter));
    if (!argsobj)
        return false;

    
    
    
    JSScript* script = iter.script();
    jit::ForbidCompilation(cx, script);

    args.rval().setObject(*argsobj);
    return true;
}

static bool
ArgumentsGetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsFunction, ArgumentsGetterImpl>(cx, args);
}

bool
ArgumentsSetterImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsFunction(args.thisv()));

    RootedFunction fun(cx, &args.thisv().toObject().as<JSFunction>());
    if (!ArgumentsRestrictions(cx, fun))
        return false;

    
    args.rval().setUndefined();
    return true;
}

static bool
ArgumentsSetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsFunction, ArgumentsSetterImpl>(cx, args);
}







static bool
CallerRestrictions(JSContext* cx, HandleFunction fun)
{
    
    
    
    if (fun->isBuiltin() ||
        (fun->isInterpreted() && fun->strict()) ||
        fun->isBoundFunction())
    {
        ThrowTypeErrorBehavior(cx);
        return false;
    }

    
    
    if (!JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING | JSREPORT_STRICT, GetErrorMessage,
                                      nullptr, JSMSG_DEPRECATED_USAGE, js_caller_str))
    {
        return false;
    }

    return true;
}

bool
CallerGetterImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsFunction(args.thisv()));

    
    
    
    
    RootedFunction fun(cx, &args.thisv().toObject().as<JSFunction>());
    if (!CallerRestrictions(cx, fun))
        return false;

    
    NonBuiltinScriptFrameIter iter(cx);
    if (!AdvanceToActiveCallLinear(cx, iter, fun)) {
        args.rval().setNull();
        return true;
    }

    ++iter;
    if (iter.done() || !iter.isFunctionFrame()) {
        args.rval().setNull();
        return true;
    }

    RootedObject caller(cx, iter.callee(cx));
    if (!cx->compartment()->wrap(cx, &caller))
        return false;

    
    
    
    {
        JSObject* callerObj = CheckedUnwrap(caller);
        if (!callerObj) {
            args.rval().setNull();
            return true;
        }

        JSFunction* callerFun = &callerObj->as<JSFunction>();
        MOZ_ASSERT(!callerFun->isBuiltin(), "non-builtin iterator returned a builtin?");

        if (callerFun->strict()) {
            JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, GetErrorMessage, nullptr,
                                         JSMSG_CALLER_IS_STRICT);
            return false;
        }
    }

    args.rval().setObject(*caller);
    return true;
}

static bool
CallerGetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsFunction, CallerGetterImpl>(cx, args);
}

bool
CallerSetterImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsFunction(args.thisv()));

    
    
    
    
    RootedFunction fun(cx, &args.thisv().toObject().as<JSFunction>());
    if (!CallerRestrictions(cx, fun))
        return false;

    
    args.rval().setUndefined();

    
    
    
    

    NonBuiltinScriptFrameIter iter(cx);
    if (!AdvanceToActiveCallLinear(cx, iter, fun))
        return true;

    ++iter;
    if (iter.done() || !iter.isFunctionFrame())
        return true;

    RootedObject caller(cx, iter.callee(cx));
    if (!cx->compartment()->wrap(cx, &caller)) {
        cx->clearPendingException();
        return true;
    }

    
    
    JSObject* callerObj = CheckedUnwrap(caller);
    if (!callerObj)
        return true;

    JSFunction* callerFun = &callerObj->as<JSFunction>();
    MOZ_ASSERT(!callerFun->isBuiltin(), "non-builtin iterator returned a builtin?");

    if (callerFun->strict()) {
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, GetErrorMessage, nullptr,
                                     JSMSG_CALLER_IS_STRICT);
        return false;
    }

    return true;
}

static bool
CallerSetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsFunction, CallerSetterImpl>(cx, args);
}

static const JSPropertySpec function_properties[] = {
    JS_PSGS("arguments", ArgumentsGetter, ArgumentsSetter, 0),
    JS_PSGS("caller", CallerGetter, CallerSetter, 0),
    JS_PS_END
};

static JSObject*
ResolveInterpretedFunctionPrototype(JSContext* cx, HandleObject obj)
{
#ifdef DEBUG
    JSFunction* fun = &obj->as<JSFunction>();
    MOZ_ASSERT(fun->isInterpreted() || fun->isAsmJSNative());
    MOZ_ASSERT(!fun->isFunctionPrototype());
#endif

    
    
    
    MOZ_ASSERT(!IsInternalFunctionObject(obj));
    MOZ_ASSERT(!obj->isBoundFunction());

    
    
    
    
    bool isStarGenerator = obj->as<JSFunction>().isStarGenerator();
    Rooted<GlobalObject*> global(cx, &obj->global());
    RootedObject objProto(cx);
    if (isStarGenerator)
        objProto = GlobalObject::getOrCreateStarGeneratorObjectPrototype(cx, global);
    else
        objProto = obj->global().getOrCreateObjectPrototype(cx);
    if (!objProto)
        return nullptr;

    RootedPlainObject proto(cx, NewObjectWithGivenProto<PlainObject>(cx, objProto,
                                                                     SingletonObject));
    if (!proto)
        return nullptr;

    
    
    RootedValue protoVal(cx, ObjectValue(*proto));
    if (!DefineProperty(cx, obj, cx->names().prototype, protoVal, nullptr, nullptr,
                        JSPROP_PERMANENT))
    {
        return nullptr;
    }

    
    
    
    
    if (!isStarGenerator) {
        RootedValue objVal(cx, ObjectValue(*obj));
        if (!DefineProperty(cx, proto, cx->names().constructor, objVal, nullptr, nullptr, 0))
            return nullptr;
    }

    return proto;
}

bool
js::FunctionHasResolveHook(const JSAtomState& atomState, jsid id)
{
    if (!JSID_IS_ATOM(id))
        return false;

    JSAtom* atom = JSID_TO_ATOM(id);
    return atom == atomState.prototype || atom == atomState.length || atom == atomState.name;
}

bool
js::fun_resolve(JSContext* cx, HandleObject obj, HandleId id, bool* resolvedp)
{
    if (!JSID_IS_ATOM(id))
        return true;

    RootedFunction fun(cx, &obj->as<JSFunction>());

    if (JSID_IS_ATOM(id, cx->names().prototype)) {
        













        if (fun->isBuiltin() || fun->isArrow() || fun->isFunctionPrototype())
            return true;

        if (!ResolveInterpretedFunctionPrototype(cx, fun))
            return false;

        *resolvedp = true;
        return true;
    }

    bool isLength = JSID_IS_ATOM(id, cx->names().length);
    if (isLength || JSID_IS_ATOM(id, cx->names().name)) {
        MOZ_ASSERT(!IsInternalFunctionObject(obj));

        RootedValue v(cx);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (isLength) {
            if (fun->hasResolvedLength())
                return true;

            uint16_t length;
            if (!fun->getLength(cx, &length))
                return false;

            v.setInt32(length);
        } else {
            if (fun->hasResolvedName())
                return true;

            v.setString(fun->atom() == nullptr ? cx->runtime()->emptyString : fun->atom());
        }

        if (!NativeDefineProperty(cx, fun, id, v, nullptr, nullptr, JSPROP_READONLY))
            return false;

        if (isLength)
            fun->setResolvedLength();
        else
            fun->setResolvedName();

        *resolvedp = true;
        return true;
    }

    return true;
}

template<XDRMode mode>
bool
js::XDRInterpretedFunction(XDRState<mode>* xdr, HandleObject enclosingScope, HandleScript enclosingScript,
                           MutableHandleFunction objp)
{
    enum FirstWordFlag {
        HasAtom             = 0x1,
        IsStarGenerator     = 0x2,
        IsLazy              = 0x4,
        HasSingletonType    = 0x8
    };

    
    RootedAtom atom(xdr->cx());
    uint32_t firstword = 0;        
    uint32_t flagsword = 0;        

    JSContext* cx = xdr->cx();
    RootedFunction fun(cx);
    RootedScript script(cx);
    Rooted<LazyScript*> lazy(cx);

    if (mode == XDR_ENCODE) {
        fun = objp;
        if (!fun->isInterpreted()) {
            JSAutoByteString funNameBytes;
            if (const char* name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                                     JSMSG_NOT_SCRIPTED_FUNCTION, name);
            }
            return false;
        }

        if (fun->atom() || fun->hasGuessedAtom())
            firstword |= HasAtom;

        if (fun->isStarGenerator())
            firstword |= IsStarGenerator;

        if (fun->isInterpretedLazy()) {
            
            
            
            MOZ_ASSERT(!fun->lazyScript()->maybeScript());

            
            firstword |= IsLazy;
            lazy = fun->lazyScript();
        } else {
            
            script = fun->nonLazyScript();
        }

        if (fun->isSingleton())
            firstword |= HasSingletonType;

        atom = fun->displayAtom();
        flagsword = (fun->nargs() << 16) | fun->flags();

        
        
        
        MOZ_ASSERT_IF(fun->isSingleton() &&
                      !((lazy && lazy->hasBeenCloned()) || (script && script->hasBeenCloned())),
                      fun->environment() == nullptr);
    }

    if (!xdr->codeUint32(&firstword))
        return false;

    if ((firstword & HasAtom) && !XDRAtom(xdr, &atom))
        return false;
    if (!xdr->codeUint32(&flagsword))
        return false;

    if (mode == XDR_DECODE) {
        RootedObject proto(cx);
        if (firstword & IsStarGenerator) {
            proto = GlobalObject::getOrCreateStarGeneratorFunctionPrototype(cx, cx->global());
            if (!proto)
                return false;
        }

        gc::AllocKind allocKind = JSFunction::FinalizeKind;
        if (uint16_t(flagsword) & JSFunction::EXTENDED)
            allocKind = JSFunction::ExtendedFinalizeKind;
        fun = NewFunctionWithProto(cx, nullptr, 0, JSFunction::INTERPRETED,
                                    NullPtr(), NullPtr(), proto,
                                   allocKind, TenuredObject);
        if (!fun)
            return false;
        script = nullptr;
    }

    if (firstword & IsLazy) {
        if (!XDRLazyScript(xdr, enclosingScope, enclosingScript, fun, &lazy))
            return false;
    } else {
        if (!XDRScript(xdr, enclosingScope, enclosingScript, fun, &script))
            return false;
    }

    if (mode == XDR_DECODE) {
        fun->setArgCount(flagsword >> 16);
        fun->setFlags(uint16_t(flagsword));
        fun->initAtom(atom);
        if (firstword & IsLazy) {
            fun->initLazyScript(lazy);
        } else {
            fun->initScript(script);
            script->setFunction(fun);
            MOZ_ASSERT(fun->nargs() == script->bindings.numArgs());
        }

        bool singleton = firstword & HasSingletonType;
        if (!JSFunction::setTypeForScriptedFunction(cx, fun, singleton))
            return false;
        objp.set(fun);
    }

    return true;
}

template bool
js::XDRInterpretedFunction(XDRState<XDR_ENCODE>*, HandleObject, HandleScript, MutableHandleFunction);

template bool
js::XDRInterpretedFunction(XDRState<XDR_DECODE>*, HandleObject, HandleScript, MutableHandleFunction);

JSObject*
js::CloneFunctionAndScript(JSContext* cx, HandleObject enclosingScope, HandleFunction srcFun,
                           PollutedGlobalScopeOption polluted)
{
    
    RootedObject cloneProto(cx);
    if (srcFun->isStarGenerator()) {
        cloneProto = GlobalObject::getOrCreateStarGeneratorFunctionPrototype(cx, cx->global());
        if (!cloneProto)
            return nullptr;
    }

    gc::AllocKind allocKind = JSFunction::FinalizeKind;
    if (srcFun->isExtended())
        allocKind = JSFunction::ExtendedFinalizeKind;
    RootedFunction clone(cx, NewFunctionWithProto(cx, nullptr, 0,
                                                  JSFunction::INTERPRETED, NullPtr(), NullPtr(),
                                                  cloneProto, allocKind, TenuredObject));
    if (!clone)
        return nullptr;

    JSScript::AutoDelazify srcScript(cx, srcFun);
    if (!srcScript)
        return nullptr;
    RootedScript clonedScript(cx, CloneScript(cx, enclosingScope, clone, srcScript, polluted));
    if (!clonedScript)
        return nullptr;

    clone->setArgCount(srcFun->nargs());
    clone->setFlags(srcFun->flags());
    clone->initAtom(srcFun->displayAtom());
    clone->initScript(clonedScript);
    clonedScript->setFunction(clone);
    if (!JSFunction::setTypeForScriptedFunction(cx, clone))
        return nullptr;

    RootedScript cloneScript(cx, clone->nonLazyScript());
    return clone;
}






static bool
fun_hasInstance(JSContext* cx, HandleObject objArg, MutableHandleValue v, bool* bp)
{
    RootedObject obj(cx, objArg);

    while (obj->is<JSFunction>() && obj->isBoundFunction())
        obj = obj->as<JSFunction>().getBoundFunctionTarget();

    RootedValue pval(cx);
    if (!GetProperty(cx, obj, obj, cx->names().prototype, &pval))
        return false;

    if (pval.isPrimitive()) {
        



        RootedValue val(cx, ObjectValue(*obj));
        ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, val, js::NullPtr());
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
JSFunction::trace(JSTracer* trc)
{
    if (isExtended()) {
        TraceRange(trc, ArrayLength(toExtended()->extendedSlots),
                   (HeapValue*)toExtended()->extendedSlots, "nativeReserved");
    }

    if (atom_)
        TraceEdge(trc, &atom_, "atom");

    if (isInterpreted()) {
        
        
        
        if (hasScript() && u.i.s.script_) {
            
            
            
            
            
            
            
            
            
            
            
            JSRuntime* rt = trc->runtime();
            if (trc->isMarkingTracer() &&
                (rt->allowRelazificationForTesting || !compartment()->hasBeenEntered()) &&
                !compartment()->isDebuggee() && !compartment()->isSelfHosting &&
                u.i.s.script_->isRelazifiable() && (!isSelfHostedBuiltin() || isExtended()))
            {
                relazify(trc);
            } else {
                TraceManuallyBarrieredEdge(trc, &u.i.s.script_, "script");
            }
        } else if (isInterpretedLazy() && u.i.s.lazy_) {
            TraceManuallyBarrieredEdge(trc, &u.i.s.lazy_, "lazyScript");
        }
        if (u.i.env_)
            TraceManuallyBarrieredEdge(trc, &u.i.env_, "fun_environment");
    }
}

static void
fun_trace(JSTracer* trc, JSObject* obj)
{
    obj->as<JSFunction>().trace(trc);
}

static bool
ThrowTypeError(JSContext* cx, unsigned argc, Value* vp)
{
    ThrowTypeErrorBehavior(cx);
    return false;
}

static JSObject*
CreateFunctionConstructor(JSContext* cx, JSProtoKey key)
{
    Rooted<GlobalObject*> global(cx, cx->global());
    RootedObject functionProto(cx, &global->getPrototype(JSProto_Function).toObject());

    RootedObject functionCtor(cx,
      NewFunctionWithProto(cx, Function, 1, JSFunction::NATIVE_CTOR,
                           NullPtr(), HandlePropertyName(cx->names().Function),
                           functionProto, JSFunction::FinalizeKind, SingletonObject));
    if (!functionCtor)
        return nullptr;

    return functionCtor;

}

static JSObject*
CreateFunctionPrototype(JSContext* cx, JSProtoKey key)
{
    Rooted<GlobalObject*> self(cx, cx->global());

    RootedObject objectProto(cx, &self->getPrototype(JSProto_Object).toObject());
    



    JSObject* functionProto_ =
        NewFunctionWithProto(cx, nullptr, 0, JSFunction::INTERPRETED,
                             self, NullPtr(), objectProto, JSFunction::FinalizeKind,
                             SingletonObject);
    if (!functionProto_)
        return nullptr;

    RootedFunction functionProto(cx, &functionProto_->as<JSFunction>());
    functionProto->setIsFunctionPrototype();

    const char* rawSource = "() {\n}";
    size_t sourceLen = strlen(rawSource);
    char16_t* source = InflateString(cx, rawSource, &sourceLen);
    if (!source)
        return nullptr;

    ScriptSource* ss =
        cx->new_<ScriptSource>();
    if (!ss) {
        js_free(source);
        return nullptr;
    }
    ScriptSourceHolder ssHolder(ss);
    ss->setSource(source, sourceLen);
    CompileOptions options(cx);
    options.setNoScriptRval(true)
           .setVersion(JSVERSION_DEFAULT);
    RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss));
    if (!sourceObject || !ScriptSourceObject::initFromOptions(cx, sourceObject, options))
        return nullptr;

    RootedScript script(cx, JSScript::Create(cx,
                                              js::NullPtr(),
                                              false,
                                             options,
                                              0,
                                             sourceObject,
                                             0,
                                             ss->length()));
    if (!script || !JSScript::fullyInitTrivial(cx, script))
        return nullptr;

    functionProto->initScript(script);
    ObjectGroup* protoGroup = functionProto->getGroup(cx);
    if (!protoGroup)
        return nullptr;

    protoGroup->setInterpretedFunction(functionProto);
    script->setFunction(functionProto);

    




    if (!JSObject::setNewGroupUnknown(cx, &JSFunction::class_, functionProto))
        return nullptr;

    
    
    
    
    
    
    
    
    
    
    RootedFunction throwTypeError(cx,
      NewFunctionWithProto(cx, ThrowTypeError, 0, JSFunction::NATIVE_FUN,
                           NullPtr(), NullPtr(), functionProto, JSFunction::FinalizeKind,
                           SingletonObject));
    if (!throwTypeError || !PreventExtensions(cx, throwTypeError))
        return nullptr;

    self->setThrowTypeError(throwTypeError);

    return functionProto;
}

const Class JSFunction::class_ = {
    js_Function_str,
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Function),
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    fun_enumerate,
    js::fun_resolve,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    fun_hasInstance,
    nullptr,                 
    fun_trace,
    {
        CreateFunctionConstructor,
        CreateFunctionPrototype,
        nullptr,
        nullptr,
        function_methods,
        function_properties
    }
};

const Class* const js::FunctionClassPtr = &JSFunction::class_;


bool
js::FindBody(JSContext* cx, HandleFunction fun, HandleLinearString src, size_t* bodyStart,
             size_t* bodyEnd)
{
    
    CompileOptions options(cx);
    options.setFileAndLine("internal-findBody", 0);

    
    if (fun->hasScript())
        options.setVersion(fun->nonLazyScript()->getVersion());

    AutoKeepAtoms keepAtoms(cx->perThreadData);

    AutoStableStringChars stableChars(cx);
    if (!stableChars.initTwoByte(cx, src))
        return false;

    const mozilla::Range<const char16_t> srcChars = stableChars.twoByteRange();
    TokenStream ts(cx, options, srcChars.start().get(), srcChars.length(), nullptr);
    int nest = 0;
    bool onward = true;
    
    do {
        TokenKind tt;
        if (!ts.getToken(&tt))
            return false;
        switch (tt) {
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
          default:
            break;
        }
    } while (onward);
    TokenKind tt;
    if (!ts.getToken(&tt))
        return false;
    if (tt == TOK_ARROW) {
        if (!ts.getToken(&tt))
            return false;
    }
    bool braced = tt == TOK_LC;
    MOZ_ASSERT_IF(fun->isExprBody(), !braced);
    *bodyStart = ts.currentToken().pos.begin;
    if (braced)
        *bodyStart += 1;
    mozilla::RangedPtr<const char16_t> end = srcChars.end();
    if (end[-1] == '}') {
        end--;
    } else {
        MOZ_ASSERT(!braced);
        for (; unicode::IsSpaceOrBOM2(end[-1]); end--)
            ;
    }
    *bodyEnd = end - srcChars.start();
    MOZ_ASSERT(*bodyStart <= *bodyEnd);
    return true;
}

JSString*
js::FunctionToString(JSContext* cx, HandleFunction fun, bool bodyOnly, bool lambdaParen)
{
    if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
        return nullptr;

    if (IsAsmJSModule(fun))
        return AsmJSModuleToString(cx, fun, !lambdaParen);
    if (IsAsmJSFunction(fun))
        return AsmJSFunctionToString(cx, fun);

    StringBuffer out(cx);
    RootedScript script(cx);

    if (fun->hasScript()) {
        script = fun->nonLazyScript();
        if (script->isGeneratorExp()) {
            if ((!bodyOnly && !out.append("function genexp() {")) ||
                !out.append("\n    [generator expression]\n") ||
                (!bodyOnly && !out.append("}")))
            {
                return nullptr;
            }
            return out.finishString();
        }
    }
    
    bool funIsMethodOrNonArrowLambda = (fun->isLambda() && !fun->isArrow()) || fun->isMethod();
    if (!bodyOnly) {
        
        if (fun->isInterpreted() && !lambdaParen && funIsMethodOrNonArrowLambda) {
            if (!out.append("("))
                return nullptr;
        }
        if (!fun->isArrow()) {
            if (!(fun->isStarGenerator() ? out.append("function* ") : out.append("function ")))
                return nullptr;
        }
        if (fun->atom()) {
            if (!out.append(fun->atom()))
                return nullptr;
        }
    }
    bool haveSource = fun->isInterpreted() && !fun->isSelfHostedBuiltin();
    if (haveSource && !script->scriptSource()->hasSourceData() &&
        !JSScript::loadSource(cx, script->scriptSource(), &haveSource))
    {
        return nullptr;
    }
    if (haveSource) {
        Rooted<JSFlatString*> src(cx, script->sourceData(cx));
        if (!src)
            return nullptr;

        bool exprBody = fun->isExprBody();

        
        
        
        
        bool funCon = !fun->isArrow() &&
                      script->sourceStart() == 0 &&
                      script->sourceEnd() == script->scriptSource()->length() &&
                      script->scriptSource()->argumentsNotIncluded();

        
        
        MOZ_ASSERT_IF(funCon, !fun->isArrow());
        MOZ_ASSERT_IF(funCon, !exprBody);
        MOZ_ASSERT_IF(!funCon && !fun->isArrow(),
                      src->length() > 0 && src->latin1OrTwoByteChar(0) == '(');

        
        
        
        
        bool addUseStrict = script->strict() && !script->explicitUseStrict() && !fun->isArrow();

        bool buildBody = funCon && !bodyOnly;
        if (buildBody) {
            
            
            
            
            if (!out.append("("))
                return nullptr;

            
            MOZ_ASSERT(script->bindings.numArgs() == fun->nargs());

            BindingIter bi(script);
            for (unsigned i = 0; i < fun->nargs(); i++, bi++) {
                MOZ_ASSERT(bi.argIndex() == i);
                if (i && !out.append(", "))
                    return nullptr;
                if (i == unsigned(fun->nargs() - 1) && fun->hasRest() && !out.append("..."))
                    return nullptr;
                if (!out.append(bi->name()))
                    return nullptr;
            }
            if (!out.append(") {\n"))
                return nullptr;
        }
        if ((bodyOnly && !funCon) || addUseStrict) {
            
            
            size_t bodyStart = 0, bodyEnd;

            
            
            if (!funCon) {
                MOZ_ASSERT(!buildBody);
                if (!FindBody(cx, fun, src, &bodyStart, &bodyEnd))
                    return nullptr;
            } else {
                bodyEnd = src->length();
            }

            if (addUseStrict) {
                
                if (!out.appendSubstring(src, 0, bodyStart))
                    return nullptr;
                if (exprBody) {
                    
                    
                    
                    if (!out.append("/* use strict */ "))
                        return nullptr;
                } else {
                    if (!out.append("\n\"use strict\";\n"))
                        return nullptr;
                }
            }

            
            
            size_t dependentEnd = bodyOnly ? bodyEnd : src->length();
            if (!out.appendSubstring(src, bodyStart, dependentEnd - bodyStart))
                return nullptr;
        } else {
            if (!out.append(src))
                return nullptr;
        }
        if (buildBody) {
            if (!out.append("\n}"))
                return nullptr;
        }
        if (bodyOnly) {
            
            if (exprBody && !out.append(";"))
                return nullptr;
        } else if (!lambdaParen && funIsMethodOrNonArrowLambda) {
            if (!out.append(")"))
                return nullptr;
        }
    } else if (fun->isInterpreted() && !fun->isSelfHostedBuiltin()) {
        if ((!bodyOnly && !out.append("() {\n    ")) ||
            !out.append("[sourceless code]") ||
            (!bodyOnly && !out.append("\n}")))
            return nullptr;
        if (!lambdaParen && fun->isLambda() && !fun->isArrow() && !out.append(")"))
            return nullptr;
    } else {
        MOZ_ASSERT(!fun->isExprBody());

        if ((!bodyOnly && !out.append("() {\n    "))
            || !out.append("[native code]")
            || (!bodyOnly && !out.append("\n}")))
        {
            return nullptr;
        }
    }
    return out.finishString();
}

JSString*
fun_toStringHelper(JSContext* cx, HandleObject obj, unsigned indent)
{
    if (!obj->is<JSFunction>()) {
        if (obj->is<ProxyObject>())
            return Proxy::fun_toString(cx, obj, indent);
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_INCOMPATIBLE_PROTO,
                             js_Function_str, js_toString_str,
                             "object");
        return nullptr;
    }

    RootedFunction fun(cx, &obj->as<JSFunction>());
    return FunctionToString(cx, fun, false, indent != JS_DONT_PRETTY_PRINT);
}

bool
js::fun_toString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(IsFunctionObject(args.calleev()));

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
fun_toSource(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(IsFunctionObject(args.calleev()));

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedString str(cx);
    if (obj->isCallable())
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
js::fun_call(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    HandleValue fval = args.thisv();
    if (!IsCallable(fval)) {
        ReportIncompatibleMethod(cx, args, &JSFunction::class_);
        return false;
    }

    args.setCallee(fval);
    args.setThis(args.get(0));

    if (args.length() > 0) {
        for (size_t i = 0; i < args.length() - 1; i++)
            args[i].set(args[i + 1]);
        args = CallArgsFromVp(args.length() - 1, vp);
    }

    return Invoke(cx, args);
}


bool
js::fun_apply(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    HandleValue fval = args.thisv();
    if (!IsCallable(fval)) {
        ReportIncompatibleMethod(cx, args, &JSFunction::class_);
        return false;
    }

    
    if (args.length() < 2 || args[1].isNullOrUndefined())
        return fun_call(cx, (args.length() > 0) ? 1 : 0, vp);

    InvokeArgs args2(cx);

    
    
    
    
    if (args[1].isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        
        ScriptFrameIter iter(cx);
        MOZ_ASSERT(iter.numActualArgs() <= ARGS_LENGTH_MAX);
        if (!args2.init(iter.numActualArgs()))
            return false;

        args2.setCallee(fval);
        args2.setThis(args[0]);

        
        iter.unaliasedForEachActual(cx, CopyTo(args2.array()));
    } else {
        
        if (!args[1].isObject()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                                 JSMSG_BAD_APPLY_ARGS, js_apply_str);
            return false;
        }

        
        
        RootedObject aobj(cx, &args[1].toObject());
        uint32_t length;
        if (!GetLengthProperty(cx, aobj, &length))
            return false;

        
        if (length > ARGS_LENGTH_MAX) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TOO_MANY_FUN_APPLY_ARGS);
            return false;
        }

        if (!args2.init(length))
            return false;

        
        args2.setCallee(fval);
        args2.setThis(args[0]);

        
        if (!GetElements(cx, aobj, length, args2.array()))
            return false;
    }

    
    if (!Invoke(cx, args2))
        return false;

    args.rval().set(args2.rval());
    return true;
}

static const uint32_t JSSLOT_BOUND_FUNCTION_TARGET     = 0;
static const uint32_t JSSLOT_BOUND_FUNCTION_THIS       = 1;
static const uint32_t JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 2;

static const uint32_t BOUND_FUNCTION_RESERVED_SLOTS = 3;

inline bool
JSFunction::initBoundFunction(JSContext* cx, HandleObject target, HandleValue thisArg,
                              const Value* args, unsigned argslen)
{
    RootedFunction self(cx, this);

    




    if (!self->toDictionaryMode(cx))
        return false;

    if (!self->JSObject::setFlags(cx, BaseShape::BOUND_FUNCTION))
        return false;

    if (!self->setSlotSpan(cx, BOUND_FUNCTION_RESERVED_SLOTS + argslen))
        return false;

    self->setSlot(JSSLOT_BOUND_FUNCTION_TARGET, ObjectValue(*target));
    self->setSlot(JSSLOT_BOUND_FUNCTION_THIS, thisArg);
    self->setSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT, PrivateUint32Value(argslen));

    self->initSlotRange(BOUND_FUNCTION_RESERVED_SLOTS, args, argslen);

    return true;
}

JSObject*
JSFunction::getBoundFunctionTarget() const
{
    MOZ_ASSERT(isBoundFunction());

    return &getSlot(JSSLOT_BOUND_FUNCTION_TARGET).toObject();
}

const js::Value&
JSFunction::getBoundFunctionThis() const
{
    MOZ_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_THIS);
}

const js::Value&
JSFunction::getBoundFunctionArgument(unsigned which) const
{
    MOZ_ASSERT(isBoundFunction());
    MOZ_ASSERT(which < getBoundFunctionArgumentCount());

    return getSlot(BOUND_FUNCTION_RESERVED_SLOTS + which);
}

size_t
JSFunction::getBoundFunctionArgumentCount() const
{
    MOZ_ASSERT(isBoundFunction());

    return getSlot(JSSLOT_BOUND_FUNCTION_ARGS_COUNT).toPrivateUint32();
}

 bool
JSFunction::createScriptForLazilyInterpretedFunction(JSContext* cx, HandleFunction fun)
{
    MOZ_ASSERT(fun->isInterpretedLazy());

    Rooted<LazyScript*> lazy(cx, fun->lazyScriptOrNull());
    if (lazy) {
        
        if (cx->zone()->needsIncrementalBarrier())
            LazyScript::writeBarrierPre(lazy);

        
        
        
        AutoSuppressGC suppressGC(cx);

        RootedScript script(cx, lazy->maybeScript());

        
        
        
        
        
        
        
        bool canRelazify = !lazy->numInnerFunctions() && !lazy->hasDirectEval();

        if (script) {
            fun->setUnlazifiedScript(script);
            
            
            if (canRelazify)
                script->setLazyScript(lazy);
            return true;
        }

        if (fun != lazy->functionNonDelazifying()) {
            if (!lazy->functionDelazifying(cx))
                return false;
            script = lazy->functionNonDelazifying()->nonLazyScript();
            if (!script)
                return false;

            fun->setUnlazifiedScript(script);
            return true;
        }

        
        
        
        
        
        
        
        
        if (canRelazify && !JS::IsIncrementalGCInProgress(cx->runtime())) {
            LazyScriptCache::Lookup lookup(cx, lazy);
            cx->runtime()->lazyScriptCache.lookup(lookup, script.address());
        }

        if (script) {
            RootedObject enclosingScope(cx, lazy->enclosingScope());
            RootedScript clonedScript(cx, CloneScript(cx, enclosingScope, fun, script));
            if (!clonedScript)
                return false;

            clonedScript->setSourceObject(lazy->sourceObject());

            fun->initAtom(script->functionNonDelazifying()->displayAtom());
            clonedScript->setFunction(fun);

            fun->setUnlazifiedScript(clonedScript);

            if (!lazy->maybeScript())
                lazy->initScript(clonedScript);
            return true;
        }

        MOZ_ASSERT(lazy->scriptSource()->hasSourceData());

        
        UncompressedSourceCache::AutoHoldEntry holder;
        const char16_t* chars = lazy->scriptSource()->chars(cx, holder);
        if (!chars)
            return false;

        const char16_t* lazyStart = chars + lazy->begin();
        size_t lazyLength = lazy->end() - lazy->begin();

        if (!frontend::CompileLazyFunction(cx, lazy, lazyStart, lazyLength))
            return false;

        script = fun->nonLazyScript();

        
        
        if (!lazy->maybeScript())
            lazy->initScript(script);

        
        if (canRelazify) {
            
            
            
            script->setColumn(lazy->column());

            LazyScriptCache::Lookup lookup(cx, lazy);
            cx->runtime()->lazyScriptCache.insert(lookup, script);

            
            
            
            script->setLazyScript(lazy);
        }
        return true;
    }

    
    MOZ_ASSERT(fun->isSelfHostedBuiltin());
    RootedAtom funAtom(cx, &fun->getExtendedSlot(0).toString()->asAtom());
    if (!funAtom)
        return false;
    Rooted<PropertyName*> funName(cx, funAtom->asPropertyName());
    return cx->runtime()->cloneSelfHostedFunctionScript(cx, funName, fun);
}

void
JSFunction::relazify(JSTracer* trc)
{
    JSScript* script = nonLazyScript();
    MOZ_ASSERT(script->isRelazifiable());
    MOZ_ASSERT(trc->runtime()->allowRelazificationForTesting || !compartment()->hasBeenEntered());
    MOZ_ASSERT(!compartment()->isDebuggee());

    
    
    
    
    
    
    
    
    
    if (script->functionNonDelazifying()->hasScript())
        TraceManuallyBarrieredEdge(trc, &u.i.s.script_, "script");

    flags_ &= ~INTERPRETED;
    flags_ |= INTERPRETED_LAZY;
    LazyScript* lazy = script->maybeLazyScript();
    u.i.s.lazy_ = lazy;
    if (lazy) {
        MOZ_ASSERT(!isSelfHostedBuiltin());
        
        
        
        if (lazy->maybeScript() == script)
            lazy->resetScript();
        TraceManuallyBarrieredEdge(trc, &u.i.s.lazy_, "lazyScript");
    } else {
        MOZ_ASSERT(isSelfHostedBuiltin());
        MOZ_ASSERT(isExtended());
        MOZ_ASSERT(getExtendedSlot(0).toString()->isAtom());
    }
}


bool
js::CallOrConstructBoundFunction(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedFunction fun(cx, &args.callee().as<JSFunction>());
    MOZ_ASSERT(fun->isBoundFunction());

    
    unsigned argslen = fun->getBoundFunctionArgumentCount();

    if (args.length() + argslen > ARGS_LENGTH_MAX) {
        ReportAllocationOverflow(cx);
        return false;
    }

    
    RootedObject target(cx, fun->getBoundFunctionTarget());

    
    const Value& boundThis = fun->getBoundFunctionThis();

    InvokeArgs invokeArgs(cx);
    if (!invokeArgs.init(args.length() + argslen))
        return false;

    
    for (unsigned i = 0; i < argslen; i++)
        invokeArgs[i].set(fun->getBoundFunctionArgument(i));
    PodCopy(invokeArgs.array() + argslen, vp + 2, args.length());

    
    invokeArgs.setCallee(ObjectValue(*target));

    bool constructing = args.isConstructing();
    if (!constructing)
        invokeArgs.setThis(boundThis);

    if (constructing ? !InvokeConstructor(cx, invokeArgs) : !Invoke(cx, invokeArgs))
        return false;

    args.rval().set(invokeArgs.rval());
    return true;
}

static bool
fun_isGenerator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSFunction* fun;
    if (!IsFunctionObject(args.thisv(), &fun)) {
        args.rval().setBoolean(false);
        return true;
    }

    args.rval().setBoolean(fun->isGenerator());
    return true;
}


bool
js::fun_bind(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedValue thisv(cx, args.thisv());

    
    if (!IsCallable(thisv)) {
        ReportIncompatibleMethod(cx, args, &JSFunction::class_);
        return false;
    }

    
    Value* boundArgs = nullptr;
    unsigned argslen = 0;
    if (args.length() > 1) {
        boundArgs = args.array() + 1;
        argslen = args.length() - 1;
    }

    
    RootedValue thisArg(cx, args.length() >= 1 ? args[0] : UndefinedValue());
    RootedObject target(cx, &thisv.toObject());
    JSObject* boundFunction = fun_bind(cx, target, thisArg, boundArgs, argslen);
    if (!boundFunction)
        return false;

    
    args.rval().setObject(*boundFunction);
    return true;
}

JSObject*
js::fun_bind(JSContext* cx, HandleObject target, HandleValue thisArg,
             Value* boundArgs, unsigned argslen)
{
    double length = 0.0;
    
    if (target->is<JSFunction>() && !target->as<JSFunction>().hasResolvedLength()) {
        uint16_t len;
        if (!target->as<JSFunction>().getLength(cx, &len))
            return nullptr;
        length = Max(0.0, double(len) - argslen);
    } else {
        
        RootedId id(cx, NameToId(cx->names().length));
        bool hasLength;
        if (!HasOwnProperty(cx, target, id, &hasLength))
            return nullptr;

        
        if (hasLength) {
            
            RootedValue targetLen(cx);
            if (!GetProperty(cx, target, target, id, &targetLen))
                return nullptr;
            
            if (targetLen.isNumber())
                length = Max(0.0, JS::ToInteger(targetLen.toNumber()) - argslen);
        }
    }

    RootedString name(cx, cx->names().empty);
    if (target->is<JSFunction>() && !target->as<JSFunction>().hasResolvedName()) {
        if (target->as<JSFunction>().atom())
            name = target->as<JSFunction>().atom();
    } else {
        
        RootedValue targetName(cx);
        if (!GetProperty(cx, target, target, cx->names().name, &targetName))
            return nullptr;

        
        if (targetName.isString())
            name = targetName.toString();
    }

    
    StringBuffer sb(cx);
    
    
    
    if (!sb.append(name))
        return nullptr;

    RootedAtom nameAtom(cx, sb.finishAtom());
    if (!nameAtom)
        return nullptr;

    
    RootedFunction fun(cx, target->isConstructor() ?
      NewNativeConstructor(cx, CallOrConstructBoundFunction, length, nameAtom) :
      NewNativeFunction(cx, CallOrConstructBoundFunction, length, nameAtom));
    if (!fun)
        return nullptr;

    if (!fun->initBoundFunction(cx, target, thisArg, boundArgs, argslen))
        return nullptr;

    
    
    if (length != fun->nargs()) {
        RootedValue lengthVal(cx, NumberValue(length));
        if (!DefineProperty(cx, fun, cx->names().length, lengthVal, nullptr, nullptr,
                            JSPROP_READONLY))
        {
            return nullptr;
        }
    }

    return fun;
}





static bool
OnBadFormal(JSContext* cx)
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_FORMAL);
    return false;
}

const JSFunctionSpec js::function_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   fun_toSource,   0,0),
#endif
    JS_FN(js_toString_str,   fun_toString,   0,0),
    JS_FN(js_apply_str,      fun_apply,      2,0),
    JS_FN(js_call_str,       fun_call,       1,0),
    JS_FN("bind",            fun_bind,       1,0),
    JS_FN("isGenerator",     fun_isGenerator,0,0),
    JS_FS_END
};

static bool
FunctionConstructor(JSContext* cx, unsigned argc, Value* vp, GeneratorKind generatorKind)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString arg(cx);   

    
    Rooted<GlobalObject*> global(cx, &args.callee().global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, global)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CSP_BLOCKED_FUNCTION);
        return false;
    }

    AutoKeepAtoms keepAtoms(cx->perThreadData);
    AutoNameVector formals(cx);

    bool hasRest = false;

    bool isStarGenerator = generatorKind == StarGenerator;
    MOZ_ASSERT(generatorKind != LegacyGenerator);

    RootedScript maybeScript(cx);
    const char* filename;
    unsigned lineno;
    bool mutedErrors;
    uint32_t pcOffset;
    DescribeScriptedCallerForCompilation(cx, &maybeScript, &filename, &lineno, &pcOffset,
                                         &mutedErrors);

    const char* introductionType = "Function";
    if (generatorKind != NotGenerator)
        introductionType = "GeneratorFunction";

    const char* introducerFilename = filename;
    if (maybeScript && maybeScript->scriptSource()->introducerFilename())
        introducerFilename = maybeScript->scriptSource()->introducerFilename();

    CompileOptions options(cx);
    options.setMutedErrors(mutedErrors)
           .setFileAndLine(filename, 1)
           .setNoScriptRval(false)
           .setIntroductionInfo(introducerFilename, introductionType, lineno, maybeScript, pcOffset);

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
                ReportAllocationOverflow(cx);
                return false;
            }
        }

        
        size_t old_args_length = args_length;
        args_length = old_args_length + n - 1;
        if (args_length < old_args_length ||
            args_length >= ~(size_t)0 / sizeof(char16_t)) {
            ReportAllocationOverflow(cx);
            return false;
        }

        




        LifoAllocScope las(&cx->tempLifoAlloc());
        char16_t* cp = cx->tempLifoAlloc().newArray<char16_t>(args_length + 1);
        if (!cp) {
            ReportOutOfMemory(cx);
            return false;
        }
        ConstTwoByteChars collected_args(cp, args_length + 1);

        


        for (unsigned i = 0; i < n; i++) {
            JSLinearString* argLinear = args[i].toString()->ensureLinear(cx);
            if (!argLinear)
                return false;

            CopyChars(cp, *argLinear);
            cp += argLinear->length();

            
            *cp++ = (i + 1 < n) ? ',' : 0;
        }

        






        TokenStream ts(cx, options, collected_args.start().get(), args_length,
                        nullptr);
        bool yieldIsValidName = ts.versionNumber() < JSVERSION_1_7 && !isStarGenerator;

        
        TokenKind tt;
        if (!ts.getToken(&tt))
            return false;
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
                        if (!ts.getToken(&tt))
                            return false;
                        if (tt == TOK_YIELD && yieldIsValidName)
                            tt = TOK_NAME;
                        if (tt != TOK_NAME) {
                            ts.reportError(JSMSG_NO_REST_NAME);
                            return false;
                        }
                    } else {
                        return OnBadFormal(cx);
                    }
                }

                if (!formals.append(ts.currentName()))
                    return false;

                



                if (!ts.getToken(&tt))
                    return false;
                if (tt == TOK_EOF)
                    break;
                if (tt != TOK_COMMA)
                    return OnBadFormal(cx);
                if (!ts.getToken(&tt))
                    return false;
            }
        }
    }

#ifdef DEBUG
    for (unsigned i = 0; i < formals.length(); ++i) {
        JSString* str = formals[i];
        MOZ_ASSERT(str->asAtom().asPropertyName() == formals[i]);
    }
#endif

    RootedString str(cx);
    if (!args.length())
        str = cx->runtime()->emptyString;
    else
        str = ToString<CanGC>(cx, args[args.length() - 1]);
    if (!str)
        return false;

    





    RootedAtom anonymousAtom(cx, cx->names().anonymous);
    RootedObject proto(cx);
    if (isStarGenerator) {
        proto = GlobalObject::getOrCreateStarGeneratorFunctionPrototype(cx, global);
        if (!proto)
            return false;
    }
    RootedFunction fun(cx, NewFunctionWithProto(cx, nullptr, 0,
                                                JSFunction::INTERPRETED_LAMBDA, global,
                                                anonymousAtom, proto,
                                                JSFunction::FinalizeKind, TenuredObject));
    if (!fun)
        return false;

    if (!JSFunction::setTypeForScriptedFunction(cx, fun))
        return false;

    if (hasRest)
        fun->setHasRest();

    AutoStableStringChars stableChars(cx);
    if (!stableChars.initTwoByte(cx, str))
        return false;

    mozilla::Range<const char16_t> chars = stableChars.twoByteRange();
    SourceBufferHolder::Ownership ownership = stableChars.maybeGiveOwnershipToCaller()
                                              ? SourceBufferHolder::GiveOwnership
                                              : SourceBufferHolder::NoOwnership;
    bool ok;
    SourceBufferHolder srcBuf(chars.start().get(), chars.length(), ownership);
    if (isStarGenerator)
        ok = frontend::CompileStarGeneratorBody(cx, &fun, options, formals, srcBuf);
    else
        ok = frontend::CompileFunctionBody(cx, &fun, options, formals, srcBuf,
                                            js::NullPtr());
    args.rval().setObject(*fun);
    return ok;
}

bool
js::Function(JSContext* cx, unsigned argc, Value* vp)
{
    return FunctionConstructor(cx, argc, vp, NotGenerator);
}

bool
js::Generator(JSContext* cx, unsigned argc, Value* vp)
{
    return FunctionConstructor(cx, argc, vp, StarGenerator);
}

bool
JSFunction::isBuiltinFunctionConstructor()
{
    return maybeNative() == Function || maybeNative() == Generator;
}

JSFunction*
js::NewNativeFunction(ExclusiveContext* cx, Native native, unsigned nargs, HandleAtom atom,
                      gc::AllocKind allocKind ,
                      NewObjectKind newKind )
{
    return NewFunctionWithProto(cx, native, nargs, JSFunction::NATIVE_FUN,
                                NullPtr(), atom, NullPtr(), allocKind, newKind);
}

JSFunction*
js::NewNativeConstructor(ExclusiveContext* cx, Native native, unsigned nargs, HandleAtom atom,
                         gc::AllocKind allocKind ,
                         NewObjectKind newKind ,
                         JSFunction::Flags flags )
{
    MOZ_ASSERT(flags & JSFunction::NATIVE_CTOR);
    return NewFunctionWithProto(cx, native, nargs, flags, NullPtr(), atom,
                                NullPtr(), allocKind, newKind);
}

JSFunction*
js::NewScriptedFunction(ExclusiveContext* cx, unsigned nargs,
                        JSFunction::Flags flags, HandleAtom atom,
                        gc::AllocKind allocKind ,
                        NewObjectKind newKind ,
                        HandleObject enclosingDynamicScope )
{
    return NewFunctionWithProto(cx, nullptr, nargs, flags,
                                enclosingDynamicScope ? enclosingDynamicScope : cx->global(),
                                atom, NullPtr(), allocKind, newKind);
}

JSFunction*
js::NewFunctionWithProto(ExclusiveContext* cx, Native native,
                         unsigned nargs, JSFunction::Flags flags, HandleObject enclosingDynamicScope,
                         HandleAtom atom, HandleObject proto,
                         gc::AllocKind allocKind ,
                         NewObjectKind newKind )
{
    MOZ_ASSERT(allocKind == JSFunction::FinalizeKind || allocKind == JSFunction::ExtendedFinalizeKind);
    MOZ_ASSERT(sizeof(JSFunction) <= gc::Arena::thingSize(JSFunction::FinalizeKind));
    MOZ_ASSERT(sizeof(FunctionExtended) <= gc::Arena::thingSize(JSFunction::ExtendedFinalizeKind));
    MOZ_ASSERT_IF(native, !enclosingDynamicScope);

    RootedObject funobj(cx);
    
    
    
    if (native && !IsAsmJSModuleNative(native))
        newKind = SingletonObject;
#ifdef DEBUG
    RootedObject nonScopeParent(cx, SkipScopeParent(enclosingDynamicScope));
    
    
    
    
    
    MOZ_ASSERT(!nonScopeParent || nonScopeParent == cx->global() ||
               nonScopeParent->is<DebugScopeObject>() ||
               nonScopeParent->isUnqualifiedVarObj());
#endif
    funobj = NewObjectWithClassProto(cx, &JSFunction::class_, proto, allocKind,
                                     newKind);
    if (!funobj)
        return nullptr;

    RootedFunction fun(cx, &funobj->as<JSFunction>());

    if (allocKind == JSFunction::ExtendedFinalizeKind)
        flags = JSFunction::Flags(flags | JSFunction::EXTENDED);

    
    fun->setArgCount(uint16_t(nargs));
    fun->setFlags(flags);
    if (fun->isInterpreted()) {
        MOZ_ASSERT(!native);
        if (fun->isInterpretedLazy())
            fun->initLazyScript(nullptr);
        else
            fun->initScript(nullptr);
        fun->initEnvironment(enclosingDynamicScope);
    } else {
        MOZ_ASSERT(fun->isNative());
        MOZ_ASSERT(native);
        fun->initNative(native, nullptr);
    }
    if (allocKind == JSFunction::ExtendedFinalizeKind)
        fun->initializeExtended();
    fun->initAtom(atom);

    return fun;
}

bool
js::CloneFunctionObjectUseSameScript(JSCompartment* compartment, HandleFunction fun,
                                     HandleObject newParent)
{
    if (compartment != fun->compartment() ||
        fun->isSingleton() ||
        ObjectGroup::useSingletonForClone(fun))
    {
        return false;
    }

    if (newParent->is<GlobalObject>())
        return true;

    
    
    
    
    
    if (IsSyntacticScope(newParent))
        return true;

    
    
    
    
    return !fun->isInterpreted() ||
           (fun->hasScript() && fun->nonLazyScript()->hasPollutedGlobalScope());
}

JSFunction*
js::CloneFunctionObject(JSContext* cx, HandleFunction fun, HandleObject parent,
                        gc::AllocKind allocKind,
                        NewObjectKind newKindArg ,
                        HandleObject proto)
{
    MOZ_ASSERT(parent);
    MOZ_ASSERT(!fun->isBoundFunction());

    bool useSameScript = CloneFunctionObjectUseSameScript(cx->compartment(), fun, parent);

    JSScript::AutoDelazify funScript(cx);
    if (!useSameScript && fun->isInterpretedLazy()) {
        funScript = fun;
        if (!funScript)
            return nullptr;
    }

    NewObjectKind newKind = useSameScript ? newKindArg : SingletonObject;
    RootedObject cloneProto(cx, proto);
    if (!cloneProto && fun->isStarGenerator()) {
        cloneProto = GlobalObject::getOrCreateStarGeneratorFunctionPrototype(cx, cx->global());
        if (!cloneProto)
            return nullptr;
    }
#ifdef DEBUG
    RootedObject realParent(cx, SkipScopeParent(parent));
    
    
    
    
    
    MOZ_ASSERT(!realParent || realParent == cx->global() ||
               realParent->is<DebugScopeObject>() ||
               realParent->isUnqualifiedVarObj());
#endif
    JSObject* cloneobj = NewObjectWithClassProto(cx, &JSFunction::class_, cloneProto,
                                                 allocKind, newKind);
    if (!cloneobj)
        return nullptr;
    RootedFunction clone(cx, &cloneobj->as<JSFunction>());

    MOZ_ASSERT(useSameScript || !fun->isInterpretedLazy());

    uint16_t flags = fun->flags() & ~JSFunction::EXTENDED;
    if (allocKind == JSFunction::ExtendedFinalizeKind)
        flags |= JSFunction::EXTENDED;

    clone->setArgCount(fun->nargs());
    clone->setFlags(flags);
    if (fun->hasScript()) {
        clone->initScript(fun->nonLazyScript());
        clone->initEnvironment(parent);
    } else if (fun->isInterpretedLazy()) {
        MOZ_ASSERT(fun->compartment() == clone->compartment());
        MOZ_ASSERT(useSameScript);
        LazyScript* lazy = fun->lazyScriptOrNull();
        clone->initLazyScript(lazy);
        clone->initEnvironment(parent);
    } else {
        clone->initNative(fun->native(), fun->jitInfo());
    }
    clone->initAtom(fun->displayAtom());

    if (allocKind == JSFunction::ExtendedFinalizeKind) {
        if (fun->isExtended() && fun->compartment() == cx->compartment()) {
            for (unsigned i = 0; i < FunctionExtended::NUM_EXTENDED_SLOTS; i++)
                clone->initExtendedSlot(i, fun->getExtendedSlot(i));
        } else {
            clone->initializeExtended();
        }
    }

    if (useSameScript) {
        



        if (fun->getProto() == clone->getProto())
            clone->setGroup(fun->group());
        return clone;
    }

    RootedFunction cloneRoot(cx, clone);

    






    PollutedGlobalScopeOption globalScopeOption = parent->is<GlobalObject>() ?
        HasCleanGlobalScope : HasPollutedGlobalScope;
    if (cloneRoot->isInterpreted() &&
        !CloneFunctionScript(cx, fun, cloneRoot, globalScopeOption, newKindArg))
    {
        return nullptr;
    }

    return cloneRoot;
}










JSAtom*
js::IdToFunctionName(JSContext* cx, HandleId id)
{
    if (JSID_IS_ATOM(id))
        return JSID_TO_ATOM(id);

    if (JSID_IS_SYMBOL(id)) {
        RootedAtom desc(cx, JSID_TO_SYMBOL(id)->description());
        StringBuffer sb(cx);
        if (!sb.append('[') || !sb.append(desc) || !sb.append(']'))
            return nullptr;
        return sb.finishAtom();
    }

    RootedValue idv(cx, IdToValue(id));
    return ToAtom<CanGC>(cx, idv);
}

JSFunction*
js::DefineFunction(JSContext* cx, HandleObject obj, HandleId id, Native native,
                   unsigned nargs, unsigned flags, AllocKind allocKind ,
                   NewObjectKind newKind )
{
    GetterOp gop;
    SetterOp sop;
    if (flags & JSFUN_STUB_GSOPS) {
        





        flags &= ~JSFUN_STUB_GSOPS;
        gop = nullptr;
        sop = nullptr;
    } else {
        gop = obj->getClass()->getProperty;
        sop = obj->getClass()->setProperty;
        MOZ_ASSERT(gop != JS_PropertyStub);
        MOZ_ASSERT(sop != JS_StrictPropertyStub);
    }

    RootedAtom atom(cx, IdToFunctionName(cx, id));
    if (!atom)
        return nullptr;

    RootedFunction fun(cx);
    if (!native)
        fun = NewScriptedFunction(cx, nargs,
                                  JSFunction::INTERPRETED_LAZY, atom,
                                  allocKind, newKind, obj);
    else if (flags & JSFUN_CONSTRUCTOR)
        fun = NewNativeConstructor(cx, native, nargs, atom, allocKind, newKind);
    else
        fun = NewNativeFunction(cx, native, nargs, atom, allocKind, newKind);

    if (!fun)
        return nullptr;

    RootedValue funVal(cx, ObjectValue(*fun));
    if (!DefineProperty(cx, obj, id, funVal, gop, sop, flags & ~JSFUN_FLAGS_MASK))
        return nullptr;

    return fun;
}

void
js::ReportIncompatibleMethod(JSContext* cx, CallReceiver call, const Class* clasp)
{
    RootedValue thisv(cx, call.thisv());

#ifdef DEBUG
    if (thisv.isObject()) {
        MOZ_ASSERT(thisv.toObject().getClass() != clasp ||
                   !thisv.toObject().isNative() ||
                   !thisv.toObject().getProto() ||
                   thisv.toObject().getProto()->getClass() != clasp);
    } else if (thisv.isString()) {
        MOZ_ASSERT(clasp != &StringObject::class_);
    } else if (thisv.isNumber()) {
        MOZ_ASSERT(clasp != &NumberObject::class_);
    } else if (thisv.isBoolean()) {
        MOZ_ASSERT(clasp != &BooleanObject::class_);
    } else if (thisv.isSymbol()) {
        MOZ_ASSERT(clasp != &SymbolObject::class_);
    } else {
        MOZ_ASSERT(thisv.isUndefined() || thisv.isNull());
    }
#endif

    if (JSFunction* fun = ReportIfNotFunction(cx, call.calleev())) {
        JSAutoByteString funNameBytes;
        if (const char* funName = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                                 clasp->name, funName, InformalValueTypeName(thisv));
        }
    }
}

void
js::ReportIncompatible(JSContext* cx, CallReceiver call)
{
    if (JSFunction* fun = ReportIfNotFunction(cx, call.calleev())) {
        JSAutoByteString funNameBytes;
        if (const char* funName = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_METHOD,
                                 funName, "method", InformalValueTypeName(call.thisv()));
        }
    }
}

namespace JS {
namespace detail {

JS_PUBLIC_API(void)
CheckIsValidConstructible(Value calleev)
{
    JSObject* callee = &calleev.toObject();
    if (callee->is<JSFunction>())
        MOZ_ASSERT(callee->as<JSFunction>().isNativeConstructor());
    else
        MOZ_ASSERT(callee->constructHook() != nullptr);
}

} 
} 
