






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
    for (JSObject *o = &scopeobj; o; o = o->enclosingScope()) {
        if (JSObjectOp op = o->getClass()->ext.innerObject) {
            Rooted<JSObject*> obj(cx, o);
            JS_ASSERT(op(cx, obj) == o);
        }
    }
#endif
}

static bool
IsEvalCacheCandidate(JSScript *script)
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
                     l.caller,
                     l.staticLevel,
                     l.version,
                     l.compartment);
}

 bool
EvalCacheHashPolicy::match(JSScript *script, const EvalCacheLookup &l)
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
      : cx_(cx), script_(cx), lookupStr_(cx)
    {
        lookup_.str = NULL;
    }

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
            js_CallNewScriptHook(cx_, script_, NULL);
            script_->isCachedEval = false;
            script_->isActiveEval = true;
        }
    }

    void setNewScript(JSScript *script) {
        
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


enum EvalType { DIRECT_EVAL = EXECUTE_DIRECT_EVAL, INDIRECT_EVAL = EXECUTE_INDIRECT_EVAL };









static bool
EvalKernel(JSContext *cx, const CallArgs &args, EvalType evalType, StackFrame *caller,
           HandleObject scopeobj)
{
    JS_ASSERT((evalType == INDIRECT_EVAL) == (caller == NULL));
    JS_ASSERT_IF(evalType == INDIRECT_EVAL, scopeobj->isGlobal());
    AssertInnerizedScopeChain(cx, *scopeobj);

    if (!scopeobj->global().isRuntimeCodeGenEnabled(cx)) {
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
        JS_ASSERT(!caller->runningInIon());
        staticLevel = caller->script()->staticLevel + 1;

        
        
        
        if (!ComputeThis(cx, caller))
            return false;
        thisv = caller->thisValue();
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

    
    
    
    
    
    
    
    
    
    if (length > 2 &&
        ((chars[0] == '[' && chars[length - 1] == ']') ||
        (chars[0] == '(' && chars[length - 1] == ')')) &&
         (!caller || !caller->script()->strict))
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
                    return false;
                if (tmp.isUndefined())
                    break;
                args.rval().set(tmp);
                return true;
            }
        }
    }

    EvalScriptGuard esg(cx);

    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    if (evalType == DIRECT_EVAL && caller->isNonEvalFunctionFrame())
        esg.lookupInEvalCache(stableStr, caller->fun(), staticLevel);

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
        JSScript *compiled = frontend::CompileScript(cx, scopeobj, caller, options,
                                                     chars, length, stableStr, staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisv, ExecuteType(evalType),
                         NULL , args.rval().address());
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
    return EvalKernel(cx, args, INDIRECT_EVAL, NULL, global);
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
