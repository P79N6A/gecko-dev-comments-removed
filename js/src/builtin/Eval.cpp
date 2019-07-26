






#include "jscntxt.h"
#include "jsonparser.h"

#include "builtin/Eval.h"
#include "frontend/BytecodeCompiler.h"
#include "mozilla/HashFunctions.h"
#include "vm/GlobalObject.h"

#include "jsinterpinlines.h"

using namespace js;

using mozilla::AddToHash;
using mozilla::HashString;


static void
AssertInnerizedScopeChain(JSContext *cx, JSObject &scopeobj)
{
#ifdef DEBUG
    RootedObject obj(cx);
    for (obj = &scopeobj; obj; obj = obj->enclosingScope()) {
        if (JSObjectOp op = obj->getClass()->ext.innerObject) {
            JS_ASSERT(op(cx, obj) == obj);
        }
    }
#endif
}

static bool
IsEvalCacheCandidate(RawScript script)
{
    
    
    
    return script->savedCallerFun &&
           !script->hasSingletons &&
           script->objects()->length == 1 &&
           !script->hasRegexps();
}

 HashNumber
EvalCacheHashPolicy::hash(const EvalCacheLookup &l)
{
    return AddToHash(HashString(l.str->chars(), l.str->length()),
                     l.caller.get(),
                     l.staticLevel,
                     l.version,
                     l.compartment);
}

 bool
EvalCacheHashPolicy::match(RawScript script, const EvalCacheLookup &l)
{
    JS_ASSERT(IsEvalCacheCandidate(script));

    
    
    JSAtom *keyStr = script->atoms[0];

    return EqualStrings(keyStr, l.str) &&
           script->getCallerFunction() == l.caller &&
           script->staticLevel == l.staticLevel &&
           script->getVersion() == l.version &&
           script->compartment() == l.compartment;
}









class EvalScriptGuard
{
    JSContext *cx_;
    Rooted<JSScript*> script_;

    
    EvalCacheLookup lookup_;
    EvalCache::AddPtr p_;

    Rooted<JSLinearString*> lookupStr_;

  public:
    EvalScriptGuard(JSContext *cx)
        : cx_(cx), script_(cx), lookup_(cx), lookupStr_(cx) {}

    ~EvalScriptGuard() {
        if (script_) {
            CallDestroyScriptHook(cx_->runtime->defaultFreeOp(), script_);
            script_->isActiveEval = false;
            script_->isCachedEval = true;
            lookup_.str = lookupStr_;
            if (lookup_.str && IsEvalCacheCandidate(script_))
                cx_->runtime->evalCache.relookupOrAdd(p_, lookup_, script_);
        }
    }

    void lookupInEvalCache(JSLinearString *str, JSFunction *caller, unsigned staticLevel)
    {
        lookupStr_ = str;
        lookup_.str = str;
        lookup_.caller = caller;
        lookup_.staticLevel = staticLevel;
        lookup_.version = cx_->findVersion();
        lookup_.compartment = cx_->compartment;
        p_ = cx_->runtime->evalCache.lookupForAdd(lookup_);
        if (p_) {
            script_ = *p_;
            cx_->runtime->evalCache.remove(p_);
            CallNewScriptHook(cx_, script_, NullPtr());
            script_->isCachedEval = false;
            script_->isActiveEval = true;
        }
    }

    void setNewScript(RawScript script) {
        
        JS_ASSERT(!script_ && script);
        script_ = script;
        script_->isActiveEval = true;
    }

    bool foundScript() {
        return !!script_;
    }

    HandleScript script() {
        JS_ASSERT(script_);
        return script_;
    }
};

enum EvalJSONResult {
    EvalJSON_Failure,
    EvalJSON_Success,
    EvalJSON_NotJSON
};

static EvalJSONResult
TryEvalJSON(JSContext *cx, JSScript *callerScript,
            StableCharPtr chars, size_t length, MutableHandleValue rval)
{
    
    
    
    
    
    
    
    
    
    if (length > 2 &&
        ((chars[0] == '[' && chars[length - 1] == ']') ||
        (chars[0] == '(' && chars[length - 1] == ')')) &&
         (!callerScript || !callerScript->strict))
    {
        
        
        
        
        
        
        for (const jschar *cp = &chars[1], *end = &chars[length - 2]; ; cp++) {
            if (*cp == 0x2028 || *cp == 0x2029)
                break;

            if (cp == end) {
                bool isArray = (chars[0] == '[');
                JSONParser parser(cx, isArray ? chars : chars + 1U, isArray ? length : length - 2,
                                  JSONParser::StrictJSON, JSONParser::NoError);
                RootedValue tmp(cx);
                if (!parser.parse(&tmp))
                    return EvalJSON_Failure;
                if (tmp.isUndefined())
                    return EvalJSON_NotJSON;
                rval.set(tmp);
                return EvalJSON_Success;
            }
        }
    }
    return EvalJSON_NotJSON;
}


enum EvalType { DIRECT_EVAL = EXECUTE_DIRECT_EVAL, INDIRECT_EVAL = EXECUTE_INDIRECT_EVAL };









static bool
EvalKernel(JSContext *cx, const CallArgs &args, EvalType evalType, AbstractFramePtr caller,
           HandleObject scopeobj)
{
    JS_ASSERT((evalType == INDIRECT_EVAL) == !caller);
    JS_ASSERT_IF(evalType == INDIRECT_EVAL, scopeobj->isGlobal());
    AssertInnerizedScopeChain(cx, *scopeobj);

    Rooted<GlobalObject*> scopeObjGlobal(cx, &scopeobj->global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, scopeObjGlobal)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_EVAL);
        return false;
    }

    
    if (args.length() < 1) {
        args.rval().setUndefined();
        return true;
    }
    if (!args[0].isString()) {
        args.rval().set(args[0]);
        return true;
    }
    RootedString str(cx, args[0].toString());

    

    
    
    
    unsigned staticLevel;
    RootedValue thisv(cx);
    if (evalType == DIRECT_EVAL) {
        JS_ASSERT_IF(caller.isStackFrame(), !caller.asStackFrame()->runningInIon());
        staticLevel = caller.script()->staticLevel + 1;

        
        
        
        if (!ComputeThis(cx, caller))
            return false;
        thisv = caller.thisValue();
    } else {
        JS_ASSERT(args.callee().global() == *scopeobj);
        staticLevel = 0;

        
        JSObject *thisobj = JSObject::thisObject(cx, scopeobj);
        if (!thisobj)
            return false;
        thisv = ObjectValue(*thisobj);
    }

    Rooted<JSStableString*> stableStr(cx, str->ensureStable(cx));
    if (!stableStr)
        return false;

    StableCharPtr chars = stableStr->chars();
    size_t length = stableStr->length();

    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    JSScript *callerScript = caller ? caller.script() : NULL;
    EvalJSONResult ejr = TryEvalJSON(cx, callerScript, chars, length, args.rval());
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    if (evalType == DIRECT_EVAL && caller.isNonEvalFunctionFrame())
        esg.lookupInEvalCache(stableStr, caller.fun(), staticLevel);

    if (!esg.foundScript()) {
        unsigned lineno;
        const char *filename;
        JSPrincipals *originPrincipals;
        CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals,
                                    evalType == DIRECT_EVAL ? CALLED_FROM_JSOP_EVAL
                                                            : NOT_CALLED_FROM_JSOP_EVAL);

        CompileOptions options(cx);
        options.setFileAndLine(filename, lineno)
               .setCompileAndGo(true)
               .setNoScriptRval(false)
               .setPrincipals(principals)
               .setOriginPrincipals(originPrincipals);
        RootedScript callerScript(cx, caller ? caller.script() : NULL);
        RawScript compiled = frontend::CompileScript(cx, scopeobj, callerScript, options,
                                                     chars.get(), length, stableStr, staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisv, ExecuteType(evalType),
                         NullFramePtr() , args.rval().address());
}

bool
js::DirectEvalFromIon(JSContext *cx,
                      HandleObject scopeobj, HandleScript callerScript,
                      HandleValue thisValue, HandleString str,
                      MutableHandleValue vp)
{
    AssertInnerizedScopeChain(cx, *scopeobj);

    Rooted<GlobalObject*> scopeObjGlobal(cx, &scopeobj->global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, scopeObjGlobal)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_EVAL);
        return false;
    }

    

    unsigned staticLevel = callerScript->staticLevel + 1;

    Rooted<JSStableString*> stableStr(cx, str->ensureStable(cx));
    if (!stableStr)
        return false;

    StableCharPtr chars = stableStr->chars();
    size_t length = stableStr->length();

    EvalJSONResult ejr = TryEvalJSON(cx, callerScript, chars, length, vp);
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    
    JSPrincipals *principals = cx->compartment->principals;

    esg.lookupInEvalCache(stableStr, callerScript->function(), staticLevel);

    if (!esg.foundScript()) {
        unsigned lineno;
        const char *filename;
        JSPrincipals *originPrincipals;
        CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals,
                                    CALLED_FROM_JSOP_EVAL);

        CompileOptions options(cx);
        options.setFileAndLine(filename, lineno)
               .setCompileAndGo(true)
               .setNoScriptRval(false)
               .setPrincipals(principals)
               .setOriginPrincipals(originPrincipals);
        RawScript compiled = frontend::CompileScript(cx, scopeobj, callerScript, options,
                                                     chars.get(), length, stableStr, staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    
    
    JS_ASSERT(thisValue.isObject() || thisValue.isUndefined() || thisValue.isNull());

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisValue, ExecuteType(DIRECT_EVAL),
                         NullFramePtr() , vp.address());
}




static inline bool
WarnOnTooManyArgs(JSContext *cx, const CallArgs &args)
{
    if (args.length() > 1) {
        Rooted<JSScript*> script(cx, cx->stack.currentScript());
        if (script && !script->warnedAboutTwoArgumentEval) {
            static const char TWO_ARGUMENT_WARNING[] =
                "Support for eval(code, scopeObject) has been removed. "
                "Use |with (scopeObject) eval(code);| instead.";
            if (!JS_ReportWarning(cx, TWO_ARGUMENT_WARNING))
                return false;
            script->warnedAboutTwoArgumentEval = true;
        } else {
            
            
        }
    }

    return true;
}

JSBool
js::IndirectEval(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!WarnOnTooManyArgs(cx, args))
        return false;

    Rooted<GlobalObject*> global(cx, &args.callee().global());
    return EvalKernel(cx, args, INDIRECT_EVAL, NullFramePtr(), global);
}

bool
js::DirectEval(JSContext *cx, const CallArgs &args)
{
    
    StackFrame *caller = cx->fp();
    JS_ASSERT(IsBuiltinEvalForScope(caller->scopeChain(), args.calleev()));
    JS_ASSERT(JSOp(*cx->regs().pc) == JSOP_EVAL);
    JS_ASSERT_IF(caller->isFunctionFrame(),
                 caller->compartment() == caller->callee().compartment());

    if (!WarnOnTooManyArgs(cx, args))
        return false;

    return EvalKernel(cx, args, DIRECT_EVAL, caller, caller->scopeChain());
}

bool
js::IsBuiltinEvalForScope(JSObject *scopeChain, const Value &v)
{
    return scopeChain->global().getOriginalEval() == v;
}

bool
js::IsAnyBuiltinEval(JSFunction *fun)
{
    return fun->maybeNative() == IndirectEval;
}

JSPrincipals *
js::PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx)
{
    JS_ASSERT(IsAnyBuiltinEval(call.callee().toFunction()) ||
              IsBuiltinFunctionConstructor(call.callee().toFunction()));

    
    
    
    
    
    
    
    
    
    
    
    
    

    return call.callee().compartment()->principals;
}
