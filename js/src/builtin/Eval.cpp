






#include "jscntxt.h"
#include "jsonparser.h"

#include "builtin/Eval.h"
#include "frontend/BytecodeCompiler.h"
#include "mozilla/HashFunctions.h"
#include "vm/GlobalObject.h"

#include "jsinterpinlines.h"

using namespace js;


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
    JSContext *context;
    Rooted<JSScript*> script_;

    
    Rooted<JSLinearString*> str;
    RootedFunction caller;
    unsigned staticLevel;

  public:
    EvalScriptGuard(JSContext *cx)
      : context(cx), script_(cx),
        str(cx), caller(cx), staticLevel(0)
    {}

    ~EvalScriptGuard() {
        if (script_) {
            CallDestroyScriptHook(context->runtime->defaultFreeOp(), script_);
            script_->isActiveEval = false;
            script_->isCachedEval = true;
            if (str && IsEvalCacheCandidate(script_)) {
                EvalCacheLookup key(str, caller, staticLevel,
                                    context->findVersion(), context->compartment);
                EvalCache::AddPtr p = context->runtime->evalCache.lookupForAdd(key);
                if (p)
                    context->runtime->evalCache.add(p, script_);
            }
        }
    }

    void lookupInEvalCache(JSLinearString *str, JSFunction *caller, unsigned staticLevel)
    {
        this->str = str;
        this->caller = caller;
        this->staticLevel = staticLevel;
        EvalCacheLookup key(str, caller, staticLevel,
                            context->findVersion(), context->compartment);
        EvalCache::Ptr p = context->runtime->evalCache.lookupForAdd(key);
        if (p) {
            script_ = *p;
            context->runtime->evalCache.remove(p);
            js_CallNewScriptHook(context, script_, NULL);
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

    JSScript *script() const {
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
        args.rval() = args[0];
        return true;
    }
    JSString *str = args[0].toString();

    

    
    
    
    unsigned staticLevel;
    RootedValue thisv(cx);
    if (evalType == DIRECT_EVAL) {
        staticLevel = caller->script()->staticLevel + 1;

        
        
        
        if (!ComputeThis(cx, caller))
            return false;
        thisv = caller->thisValue();
    } else {
        JS_ASSERT(args.callee().global() == *scopeobj);
        staticLevel = 0;

        
        JSObject *thisobj = scopeobj->thisObject(cx);
        if (!thisobj)
            return false;
        thisv = ObjectValue(*thisobj);
    }

    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;
    const jschar *chars = linearStr->chars();
    size_t length = linearStr->length();

    SkipRoot skip(cx, &chars);

    
    
    
    
    
    
    
    
    
    if (length > 2 &&
        ((chars[0] == '[' && chars[length - 1] == ']') ||
        (chars[0] == '(' && chars[length - 1] == ')')) &&
         (!caller || !caller->script()->strictModeCode))
    {
        
        
        
        
        
        
        for (const jschar *cp = &chars[1], *end = &chars[length - 2]; ; cp++) {
            if (*cp == 0x2028 || *cp == 0x2029)
                break;

            if (cp == end) {
                bool isArray = (chars[0] == '[');
                JSONParser parser(cx, isArray ? chars : chars + 1, isArray ? length : length - 2,
                                  JSONParser::StrictJSON, JSONParser::NoError);
                Value tmp;
                if (!parser.parse(&tmp))
                    return false;
                if (tmp.isUndefined())
                    break;
                args.rval() = tmp;
                return true;
            }
        }
    }

    EvalScriptGuard esg(cx);

    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    if (evalType == DIRECT_EVAL && caller->isNonEvalFunctionFrame())
        esg.lookupInEvalCache(linearStr, caller->fun(), staticLevel);

    if (!esg.foundScript()) {
        unsigned lineno;
        const char *filename;
        JSPrincipals *originPrincipals;
        CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals,
                                    evalType == DIRECT_EVAL ? CALLED_FROM_JSOP_EVAL
                                                            : NOT_CALLED_FROM_JSOP_EVAL);

        bool compileAndGo = true;
        bool noScriptRval = false;
        JSScript *compiled = frontend::CompileScript(cx, scopeobj, caller,
                                                     principals, originPrincipals,
                                                     compileAndGo, noScriptRval,
                                                     chars, length, filename,
                                                     lineno, cx->findVersion(), linearStr,
                                                     staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisv, ExecuteType(evalType),
                         NULL , &args.rval());
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
    JS_ASSERT(caller->isScriptFrame());
    JS_ASSERT(IsBuiltinEvalForScope(caller->scopeChain(), args.calleev()));
    JS_ASSERT(JSOp(*cx->regs().pc) == JSOP_EVAL);

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

    
    
    
    
    
    
    
    
    
    
    
    
    

    return call.callee().principals(cx);
}
