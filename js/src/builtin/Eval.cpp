





#include "builtin/Eval.h"

#include "mozilla/HashFunctions.h"

#include "jscntxt.h"
#include "jsonparser.h"

#include "frontend/BytecodeCompiler.h"
#include "vm/GlobalObject.h"

#include "vm/Interpreter-inl.h"

using namespace js;

using mozilla::AddToHash;
using mozilla::HashString;


static void
AssertInnerizedScopeChain(JSContext *cx, JSObject &scopeobj)
{
#ifdef DEBUG
    RootedObject obj(cx);
    for (obj = &scopeobj; obj; obj = obj->enclosingScope())
        JS_ASSERT(GetInnerObject(obj) == obj);
#endif
}

static bool
IsEvalCacheCandidate(JSScript *script)
{
    
    
    
    return script->savedCallerFun() &&
           !script->hasSingletons() &&
           script->objects()->length == 1 &&
           !script->hasRegexps();
}

 HashNumber
EvalCacheHashPolicy::hash(const EvalCacheLookup &l)
{
    return AddToHash(HashString(l.str->chars(), l.str->length()),
                     l.callerScript.get(),
                     l.version,
                     l.pc);
}

 bool
EvalCacheHashPolicy::match(const EvalCacheEntry &cacheEntry, const EvalCacheLookup &l)
{
    JSScript *script = cacheEntry.script;

    JS_ASSERT(IsEvalCacheCandidate(script));

    
    
    JSAtom *keyStr = script->atoms[0];

    return EqualStrings(keyStr, l.str) &&
           cacheEntry.callerScript == l.callerScript &&
           script->getVersion() == l.version &&
           cacheEntry.pc == l.pc;
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
            CallDestroyScriptHook(cx_->runtime()->defaultFreeOp(), script_);
            script_->cacheForEval();
            EvalCacheEntry cacheEntry = {script_, lookup_.callerScript, lookup_.pc};
            lookup_.str = lookupStr_;
            if (lookup_.str && IsEvalCacheCandidate(script_))
                cx_->runtime()->evalCache.relookupOrAdd(p_, lookup_, cacheEntry);
        }
    }

    void lookupInEvalCache(JSLinearString *str, JSScript *callerScript, jsbytecode *pc)
    {
        lookupStr_ = str;
        lookup_.str = str;
        lookup_.callerScript = callerScript;
        lookup_.version = cx_->findVersion();
        lookup_.pc = pc;
        p_ = cx_->runtime()->evalCache.lookupForAdd(lookup_);
        if (p_) {
            script_ = p_->script;
            cx_->runtime()->evalCache.remove(p_);
            CallNewScriptHook(cx_, script_, NullPtr());
            script_->uncacheForEval();
        }
    }

    void setNewScript(JSScript *script) {
        
        JS_ASSERT(!script_ && script);
        script_ = script;
        script_->setActiveEval();
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
            ConstTwoByteChars chars, size_t length, MutableHandleValue rval)
{
    
    
    
    
    
    
    
    
    
    if (length > 2 &&
        ((chars[0] == '[' && chars[length - 1] == ']') ||
        (chars[0] == '(' && chars[length - 1] == ')')) &&
        (!callerScript || !callerScript->strict()))
    {
        
        
        
        
        
        
        for (const jschar *cp = &chars[1], *end = &chars[length - 2]; ; cp++) {
            if (*cp == 0x2028 || *cp == 0x2029)
                break;

            if (cp == end) {
                bool isArray = (chars[0] == '[');
                JSONParser parser(cx, isArray ? chars : chars + 1U, isArray ? length : length - 2,
                                  JSONParser::NoError);
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
           HandleObject scopeobj, jsbytecode *pc)
{
    JS_ASSERT((evalType == INDIRECT_EVAL) == !caller);
    JS_ASSERT((evalType == INDIRECT_EVAL) == !pc);
    JS_ASSERT_IF(evalType == INDIRECT_EVAL, scopeobj->is<GlobalObject>());
    AssertInnerizedScopeChain(cx, *scopeobj);

    Rooted<GlobalObject*> scopeObjGlobal(cx, &scopeobj->global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, scopeObjGlobal)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CSP_BLOCKED_EVAL);
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
        JS_ASSERT_IF(caller.isInterpreterFrame(), !caller.asInterpreterFrame()->runningInJit());
        staticLevel = caller.script()->staticLevel() + 1;

        
        
        
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

    Rooted<JSFlatString*> flatStr(cx, str->ensureFlat(cx));
    if (!flatStr)
        return false;

    size_t length = flatStr->length();
    ConstTwoByteChars chars(flatStr->chars(), length);

    RootedScript callerScript(cx, caller ? caller.script() : nullptr);
    EvalJSONResult ejr = TryEvalJSON(cx, callerScript, chars, length, args.rval());
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    if (evalType == DIRECT_EVAL && caller.isNonEvalFunctionFrame())
        esg.lookupInEvalCache(flatStr, callerScript, pc);

    if (!esg.foundScript()) {
        JSScript *maybeScript;
        unsigned lineno;
        const char *filename;
        JSPrincipals *originPrincipals;
        uint32_t pcOffset;
        DescribeScriptedCallerForCompilation(cx, &maybeScript, &filename, &lineno, &pcOffset,
                                             &originPrincipals,
                                             evalType == DIRECT_EVAL
                                             ? CALLED_FROM_JSOP_EVAL
                                             : NOT_CALLED_FROM_JSOP_EVAL);

        const char *introducerFilename = filename;
        if (maybeScript && maybeScript->scriptSource()->introducerFilename())
            introducerFilename = maybeScript->scriptSource()->introducerFilename();

        CompileOptions options(cx);
        options.setFileAndLine(filename, 1)
               .setCompileAndGo(true)
               .setForEval(true)
               .setNoScriptRval(false)
               .setOriginPrincipals(originPrincipals)
               .setIntroductionInfo(introducerFilename, "eval", lineno, maybeScript, pcOffset);
        SourceBufferHolder srcBuf(chars.get(), length, SourceBufferHolder::NoOwnership);
        JSScript *compiled = frontend::CompileScript(cx, &cx->tempLifoAlloc(),
                                                     scopeobj, callerScript, options,
                                                     srcBuf, flatStr, staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisv, ExecuteType(evalType),
                         NullFramePtr() , args.rval().address());
}

bool
js::DirectEvalStringFromIon(JSContext *cx,
                            HandleObject scopeobj, HandleScript callerScript,
                            HandleValue thisValue, HandleString str,
                            jsbytecode *pc, MutableHandleValue vp)
{
    AssertInnerizedScopeChain(cx, *scopeobj);

    Rooted<GlobalObject*> scopeObjGlobal(cx, &scopeobj->global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, scopeObjGlobal)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CSP_BLOCKED_EVAL);
        return false;
    }

    

    unsigned staticLevel = callerScript->staticLevel() + 1;

    Rooted<JSFlatString*> flatStr(cx, str->ensureFlat(cx));
    if (!flatStr)
        return false;

    size_t length = flatStr->length();
    ConstTwoByteChars chars(flatStr->chars(), length);

    EvalJSONResult ejr = TryEvalJSON(cx, callerScript, chars, length, vp);
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    esg.lookupInEvalCache(flatStr, callerScript, pc);

    if (!esg.foundScript()) {
        JSScript *maybeScript;
        const char *filename;
        unsigned lineno;
        JSPrincipals *originPrincipals;
        uint32_t pcOffset;
        DescribeScriptedCallerForCompilation(cx, &maybeScript, &filename, &lineno, &pcOffset,
                                              &originPrincipals, CALLED_FROM_JSOP_EVAL);

        const char *introducerFilename = filename;
        if (maybeScript && maybeScript->scriptSource()->introducerFilename())
            introducerFilename = maybeScript->scriptSource()->introducerFilename();

        CompileOptions options(cx);
        options.setFileAndLine(filename, 1)
               .setCompileAndGo(true)
               .setForEval(true)
               .setNoScriptRval(false)
               .setOriginPrincipals(originPrincipals)
               .setIntroductionInfo(introducerFilename, "eval", lineno, maybeScript, pcOffset);
        SourceBufferHolder srcBuf(chars.get(), length, SourceBufferHolder::NoOwnership);
        JSScript *compiled = frontend::CompileScript(cx, &cx->tempLifoAlloc(),
                                                     scopeobj, callerScript, options,
                                                     srcBuf, flatStr, staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    
    
    JS_ASSERT(thisValue.isObject() || thisValue.isUndefined() || thisValue.isNull());

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisValue, ExecuteType(DIRECT_EVAL),
                         NullFramePtr() , vp.address());
}

bool
js::DirectEvalValueFromIon(JSContext *cx,
                           HandleObject scopeobj, HandleScript callerScript,
                           HandleValue thisValue, HandleValue evalArg,
                           jsbytecode *pc, MutableHandleValue vp)
{
    
    if (!evalArg.isString()) {
        vp.set(evalArg);
        return true;
    }

    RootedString string(cx, evalArg.toString());
    return DirectEvalStringFromIon(cx, scopeobj, callerScript, thisValue, string, pc, vp);
}

bool
js::IndirectEval(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, &args.callee().global());
    return EvalKernel(cx, args, INDIRECT_EVAL, NullFramePtr(), global, nullptr);
}

bool
js::DirectEval(JSContext *cx, const CallArgs &args)
{
    
    ScriptFrameIter iter(cx);
    AbstractFramePtr caller = iter.abstractFramePtr();

    JS_ASSERT(caller.scopeChain()->global().valueIsEval(args.calleev()));
    JS_ASSERT(JSOp(*iter.pc()) == JSOP_EVAL ||
              JSOp(*iter.pc()) == JSOP_SPREADEVAL);
    JS_ASSERT_IF(caller.isFunctionFrame(),
                 caller.compartment() == caller.callee()->compartment());

    RootedObject scopeChain(cx, caller.scopeChain());
    return EvalKernel(cx, args, DIRECT_EVAL, caller, scopeChain, iter.pc());
}

bool
js::IsAnyBuiltinEval(JSFunction *fun)
{
    return fun->maybeNative() == IndirectEval;
}
