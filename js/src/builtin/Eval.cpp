





#include "builtin/Eval.h"

#include "mozilla/HashFunctions.h"
#include "mozilla/Range.h"

#include "jscntxt.h"
#include "jsonparser.h"

#include "frontend/BytecodeCompiler.h"
#include "vm/GlobalObject.h"

#include "vm/Interpreter-inl.h"

using namespace js;

using mozilla::AddToHash;
using mozilla::HashString;
using mozilla::RangedPtr;

using JS::AutoCheckCannotGC;


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
    AutoCheckCannotGC nogc;
    uint32_t hash = l.str->hasLatin1Chars()
                    ? HashString(l.str->latin1Chars(nogc), l.str->length())
                    : HashString(l.str->twoByteChars(nogc), l.str->length());
    return AddToHash(hash, l.callerScript.get(), l.version, l.pc);
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

    RootedLinearString lookupStr_;

  public:
    explicit EvalScriptGuard(JSContext *cx)
        : cx_(cx), script_(cx), lookup_(cx), lookupStr_(cx) {}

    ~EvalScriptGuard() {
        if (script_) {
            script_->clearTraps(cx_->runtime()->defaultFreeOp());
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

template <typename CharT>
static bool
EvalStringMightBeJSON(const mozilla::Range<const CharT> chars)
{
    
    
    
    
    size_t length = chars.length();
    if (length > 2 &&
        ((chars[0] == '[' && chars[length - 1] == ']') ||
         (chars[0] == '(' && chars[length - 1] == ')')))
    {
        
        
        
        
        
        
        if (sizeof(CharT) > 1) {
            for (RangedPtr<const CharT> cp = chars.start() + 1, end = chars.end() - 1;
                 cp < end;
                 cp++)
            {
                jschar c = *cp;
                if (c == 0x2028 || c == 0x2029)
                    return false;
            }
        }

        return true;
    }
    return false;
}

template <typename CharT>
static EvalJSONResult
ParseEvalStringAsJSON(JSContext *cx, const mozilla::Range<const CharT> chars, MutableHandleValue rval)
{
    size_t len = chars.length();
    MOZ_ASSERT((chars[0] == '(' && chars[len - 1] == ')') ||
               (chars[0] == '[' && chars[len - 1] == ']'));

    auto jsonChars = (chars[0] == '[')
                     ? chars
                     : mozilla::Range<const CharT>(chars.start().get() + 1U, len - 2);

    JSONParser<CharT> parser(cx, jsonChars, JSONParserBase::NoError);
    if (!parser.parse(rval))
        return EvalJSON_Failure;

    return rval.isUndefined() ? EvalJSON_NotJSON : EvalJSON_Success;
}

static EvalJSONResult
TryEvalJSON(JSContext *cx, JSScript *callerScript, JSFlatString *str, MutableHandleValue rval)
{
    
    
    
    
    if (callerScript && callerScript->strict())
        return EvalJSON_NotJSON;

    if (str->hasLatin1Chars()) {
        AutoCheckCannotGC nogc;
        if (!EvalStringMightBeJSON(str->latin1Range(nogc)))
            return EvalJSON_NotJSON;
    } else {
        AutoCheckCannotGC nogc;
        if (!EvalStringMightBeJSON(str->twoByteRange(nogc)))
            return EvalJSON_NotJSON;
    }

    AutoStableStringChars flatChars(cx);
    if (!flatChars.init(cx, str))
        return EvalJSON_Failure;

    return flatChars.isLatin1()
           ? ParseEvalStringAsJSON(cx, flatChars.latin1Range(), rval)
           : ParseEvalStringAsJSON(cx, flatChars.twoByteRange(), rval);
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

    RootedScript callerScript(cx, caller ? caller.script() : nullptr);
    EvalJSONResult ejr = TryEvalJSON(cx, callerScript, flatStr, args.rval());
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    if (evalType == DIRECT_EVAL && caller.isNonEvalFunctionFrame())
        esg.lookupInEvalCache(flatStr, callerScript, pc);

    if (!esg.foundScript()) {
        RootedScript maybeScript(cx);
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

        AutoStableStringChars flatChars(cx);
        if (!flatChars.initTwoByte(cx, flatStr))
            return false;

        const jschar *chars = flatChars.twoByteRange().start().get();
        SourceBufferHolder::Ownership ownership = flatChars.maybeGiveOwnershipToCaller()
                                                  ? SourceBufferHolder::GiveOwnership
                                                  : SourceBufferHolder::NoOwnership;
        SourceBufferHolder srcBuf(chars, flatStr->length(), ownership);
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

    EvalJSONResult ejr = TryEvalJSON(cx, callerScript, flatStr, vp);
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    esg.lookupInEvalCache(flatStr, callerScript, pc);

    if (!esg.foundScript()) {
        RootedScript maybeScript(cx);
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

        AutoStableStringChars flatChars(cx);
        if (!flatChars.initTwoByte(cx, flatStr))
            return false;

        const jschar *chars = flatChars.twoByteRange().start().get();
        SourceBufferHolder::Ownership ownership = flatChars.maybeGiveOwnershipToCaller()
                                                  ? SourceBufferHolder::GiveOwnership
                                                  : SourceBufferHolder::NoOwnership;
        SourceBufferHolder srcBuf(chars, flatStr->length(), ownership);
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

JS_FRIEND_API(bool)
js::ExecuteInGlobalAndReturnScope(JSContext *cx, HandleObject global, HandleScript scriptArg,
                                  MutableHandleObject scopeArg)
{
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, global);
    MOZ_ASSERT(global->is<GlobalObject>());

    RootedScript script(cx, scriptArg);
    if (script->compartment() != cx->compartment()) {
        script = CloneScript(cx, NullPtr(), NullPtr(), script);
        if (!script)
            return false;
    }

    RootedObject scope(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    if (!scope)
        return false;

    if (!scope->setQualifiedVarObj(cx))
        return false;

    if (!scope->setUnqualifiedVarObj(cx))
        return false;

    JSObject *thisobj = JSObject::thisObject(cx, global);
    if (!thisobj)
        return false;

    RootedValue thisv(cx, ObjectValue(*thisobj));
    RootedValue rval(cx);
    if (!ExecuteKernel(cx, script, *scope, thisv, EXECUTE_GLOBAL,
                       NullFramePtr() , rval.address()))
    {
        return false;
    }

    scopeArg.set(scope);
    return true;
}
