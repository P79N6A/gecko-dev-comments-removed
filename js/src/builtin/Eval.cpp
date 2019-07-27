





#include "builtin/Eval.h"

#include "mozilla/HashFunctions.h"
#include "mozilla/Range.h"

#include "jscntxt.h"

#include "frontend/BytecodeCompiler.h"
#include "vm/Debugger.h"
#include "vm/GlobalObject.h"
#include "vm/JSONParser.h"

#include "vm/Interpreter-inl.h"

using namespace js;

using mozilla::AddToHash;
using mozilla::HashString;
using mozilla::RangedPtr;

using JS::AutoCheckCannotGC;


static void
AssertInnerizedScopeChain(JSContext* cx, JSObject& scopeobj)
{
#ifdef DEBUG
    RootedObject obj(cx);
    for (obj = &scopeobj; obj; obj = obj->enclosingScope())
        MOZ_ASSERT(GetInnerObject(obj) == obj);
#endif
}

static bool
IsEvalCacheCandidate(JSScript* script)
{
    
    
    
    return script->savedCallerFun() &&
           !script->hasSingletons() &&
           script->objects()->length == 1 &&
           !script->hasRegexps();
}

 HashNumber
EvalCacheHashPolicy::hash(const EvalCacheLookup& l)
{
    AutoCheckCannotGC nogc;
    uint32_t hash = l.str->hasLatin1Chars()
                    ? HashString(l.str->latin1Chars(nogc), l.str->length())
                    : HashString(l.str->twoByteChars(nogc), l.str->length());
    return AddToHash(hash, l.callerScript.get(), l.version, l.pc);
}

 bool
EvalCacheHashPolicy::match(const EvalCacheEntry& cacheEntry, const EvalCacheLookup& l)
{
    JSScript* script = cacheEntry.script;

    MOZ_ASSERT(IsEvalCacheCandidate(script));

    return EqualStrings(cacheEntry.str, l.str) &&
           cacheEntry.callerScript == l.callerScript &&
           script->getVersion() == l.version &&
           cacheEntry.pc == l.pc;
}


class EvalScriptGuard
{
    JSContext* cx_;
    Rooted<JSScript*> script_;

    
    EvalCacheLookup lookup_;
    EvalCache::AddPtr p_;

    RootedLinearString lookupStr_;

  public:
    explicit EvalScriptGuard(JSContext* cx)
        : cx_(cx), script_(cx), lookup_(cx), lookupStr_(cx) {}

    ~EvalScriptGuard() {
        if (script_) {
            script_->cacheForEval();
            EvalCacheEntry cacheEntry = {lookupStr_, script_, lookup_.callerScript, lookup_.pc};
            lookup_.str = lookupStr_;
            if (lookup_.str && IsEvalCacheCandidate(script_))
                cx_->runtime()->evalCache.relookupOrAdd(p_, lookup_, cacheEntry);
        }
    }

    void lookupInEvalCache(JSLinearString* str, JSScript* callerScript, jsbytecode* pc)
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

    void setNewScript(JSScript* script) {
        
        MOZ_ASSERT(!script_ && script);
        script_ = script;
        script_->setActiveEval();
    }

    bool foundScript() {
        return !!script_;
    }

    HandleScript script() {
        MOZ_ASSERT(script_);
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
                char16_t c = *cp;
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
ParseEvalStringAsJSON(JSContext* cx, const mozilla::Range<const CharT> chars, MutableHandleValue rval)
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
TryEvalJSON(JSContext* cx, JSLinearString* str, MutableHandleValue rval)
{
    if (str->hasLatin1Chars()) {
        AutoCheckCannotGC nogc;
        if (!EvalStringMightBeJSON(str->latin1Range(nogc)))
            return EvalJSON_NotJSON;
    } else {
        AutoCheckCannotGC nogc;
        if (!EvalStringMightBeJSON(str->twoByteRange(nogc)))
            return EvalJSON_NotJSON;
    }

    AutoStableStringChars linearChars(cx);
    if (!linearChars.init(cx, str))
        return EvalJSON_Failure;

    return linearChars.isLatin1()
           ? ParseEvalStringAsJSON(cx, linearChars.latin1Range(), rval)
           : ParseEvalStringAsJSON(cx, linearChars.twoByteRange(), rval);
}

static bool
HasPollutedScopeChain(JSObject* scopeChain)
{
    while (scopeChain) {
        if (scopeChain->is<DynamicWithObject>())
            return true;
        scopeChain = scopeChain->enclosingScope();
    }

    return false;
}


enum EvalType { DIRECT_EVAL = EXECUTE_DIRECT_EVAL, INDIRECT_EVAL = EXECUTE_INDIRECT_EVAL };









static bool
EvalKernel(JSContext* cx, const CallArgs& args, EvalType evalType, AbstractFramePtr caller,
           HandleObject scopeobj, jsbytecode* pc)
{
    MOZ_ASSERT((evalType == INDIRECT_EVAL) == !caller);
    MOZ_ASSERT((evalType == INDIRECT_EVAL) == !pc);
    MOZ_ASSERT_IF(evalType == INDIRECT_EVAL, scopeobj->is<GlobalObject>());
    AssertInnerizedScopeChain(cx, *scopeobj);

    Rooted<GlobalObject*> scopeObjGlobal(cx, &scopeobj->global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, scopeObjGlobal)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CSP_BLOCKED_EVAL);
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
        MOZ_ASSERT_IF(caller.isInterpreterFrame(), !caller.asInterpreterFrame()->runningInJit());
        staticLevel = caller.script()->staticLevel() + 1;

        
        
        
        if (!ComputeThis(cx, caller))
            return false;
        thisv = caller.thisValue();
    } else {
        MOZ_ASSERT(args.callee().global() == *scopeobj);
        staticLevel = 0;

        
        JSObject* thisobj = GetThisObject(cx, scopeobj);
        if (!thisobj)
            return false;
        thisv = ObjectValue(*thisobj);
    }

    RootedLinearString linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;

    RootedScript callerScript(cx, caller ? caller.script() : nullptr);
    EvalJSONResult ejr = TryEvalJSON(cx, linearStr, args.rval());
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    if (evalType == DIRECT_EVAL && caller.isNonEvalFunctionFrame())
        esg.lookupInEvalCache(linearStr, callerScript, pc);

    if (!esg.foundScript()) {
        RootedScript maybeScript(cx);
        unsigned lineno;
        const char* filename;
        bool mutedErrors;
        uint32_t pcOffset;
        DescribeScriptedCallerForCompilation(cx, &maybeScript, &filename, &lineno, &pcOffset,
                                             &mutedErrors,
                                             evalType == DIRECT_EVAL
                                             ? CALLED_FROM_JSOP_EVAL
                                             : NOT_CALLED_FROM_JSOP_EVAL);

        const char* introducerFilename = filename;
        if (maybeScript && maybeScript->scriptSource()->introducerFilename())
            introducerFilename = maybeScript->scriptSource()->introducerFilename();

        RootedObject enclosing(cx);
        if (evalType == DIRECT_EVAL)
            enclosing = callerScript->innermostStaticScope(pc);
        Rooted<StaticEvalObject*> staticScope(cx, StaticEvalObject::create(cx, enclosing));
        if (!staticScope)
            return false;

        bool hasPollutedGlobalScope =
            HasPollutedScopeChain(scopeobj) ||
            (evalType == DIRECT_EVAL && callerScript->hasPollutedGlobalScope());

        CompileOptions options(cx);
        options.setFileAndLine(filename, 1)
               .setHasPollutedScope(hasPollutedGlobalScope)
               .setIsRunOnce(true)
               .setForEval(true)
               .setNoScriptRval(false)
               .setMutedErrors(mutedErrors)
               .setIntroductionInfo(introducerFilename, "eval", lineno, maybeScript, pcOffset)
               .maybeMakeStrictMode(evalType == DIRECT_EVAL && IsStrictEvalPC(pc));

        AutoStableStringChars linearChars(cx);
        if (!linearChars.initTwoByte(cx, linearStr))
            return false;

        const char16_t* chars = linearChars.twoByteRange().start().get();
        SourceBufferHolder::Ownership ownership = linearChars.maybeGiveOwnershipToCaller()
                                                  ? SourceBufferHolder::GiveOwnership
                                                  : SourceBufferHolder::NoOwnership;
        SourceBufferHolder srcBuf(chars, linearStr->length(), ownership);
        JSScript* compiled = frontend::CompileScript(cx, &cx->tempLifoAlloc(),
                                                     scopeobj, callerScript, staticScope,
                                                     options, srcBuf, linearStr, staticLevel);
        if (!compiled)
            return false;

        if (compiled->strict())
            staticScope->setStrict();

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisv, ExecuteType(evalType),
                         NullFramePtr() , args.rval().address());
}

bool
js::DirectEvalStringFromIon(JSContext* cx,
                            HandleObject scopeobj, HandleScript callerScript,
                            HandleValue thisValue, HandleString str,
                            jsbytecode* pc, MutableHandleValue vp)
{
    AssertInnerizedScopeChain(cx, *scopeobj);

    Rooted<GlobalObject*> scopeObjGlobal(cx, &scopeobj->global());
    if (!GlobalObject::isRuntimeCodeGenEnabled(cx, scopeObjGlobal)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CSP_BLOCKED_EVAL);
        return false;
    }

    

    unsigned staticLevel = callerScript->staticLevel() + 1;

    RootedLinearString linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;

    EvalJSONResult ejr = TryEvalJSON(cx, linearStr, vp);
    if (ejr != EvalJSON_NotJSON)
        return ejr == EvalJSON_Success;

    EvalScriptGuard esg(cx);

    esg.lookupInEvalCache(linearStr, callerScript, pc);

    if (!esg.foundScript()) {
        RootedScript maybeScript(cx);
        const char* filename;
        unsigned lineno;
        bool mutedErrors;
        uint32_t pcOffset;
        DescribeScriptedCallerForCompilation(cx, &maybeScript, &filename, &lineno, &pcOffset,
                                              &mutedErrors, CALLED_FROM_JSOP_EVAL);

        const char* introducerFilename = filename;
        if (maybeScript && maybeScript->scriptSource()->introducerFilename())
            introducerFilename = maybeScript->scriptSource()->introducerFilename();

        RootedObject enclosing(cx, callerScript->innermostStaticScope(pc));
        Rooted<StaticEvalObject*> staticScope(cx, StaticEvalObject::create(cx, enclosing));
        if (!staticScope)
            return false;

        CompileOptions options(cx);
        options.setFileAndLine(filename, 1)
               .setHasPollutedScope(HasPollutedScopeChain(scopeobj) ||
                                    callerScript->hasPollutedGlobalScope())
               .setIsRunOnce(true)
               .setForEval(true)
               .setNoScriptRval(false)
               .setMutedErrors(mutedErrors)
               .setIntroductionInfo(introducerFilename, "eval", lineno, maybeScript, pcOffset)
               .maybeMakeStrictMode(IsStrictEvalPC(pc));

        AutoStableStringChars linearChars(cx);
        if (!linearChars.initTwoByte(cx, linearStr))
            return false;

        const char16_t* chars = linearChars.twoByteRange().start().get();
        SourceBufferHolder::Ownership ownership = linearChars.maybeGiveOwnershipToCaller()
                                                  ? SourceBufferHolder::GiveOwnership
                                                  : SourceBufferHolder::NoOwnership;
        SourceBufferHolder srcBuf(chars, linearStr->length(), ownership);
        JSScript* compiled = frontend::CompileScript(cx, &cx->tempLifoAlloc(),
                                                     scopeobj, callerScript, staticScope,
                                                     options, srcBuf, linearStr, staticLevel);
        if (!compiled)
            return false;

        if (compiled->strict())
            staticScope->setStrict();

        esg.setNewScript(compiled);
    }

    
    
    MOZ_ASSERT(thisValue.isObject() || thisValue.isUndefined() || thisValue.isNull());

    
    
    
    
    RootedValue nthisValue(cx, thisValue);
    if (!callerScript->strict() && esg.script()->strict() && !thisValue.isObject()) {
        JSObject* obj = BoxNonStrictThis(cx, thisValue);
        if (!obj)
            return false;
        nthisValue = ObjectValue(*obj);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, nthisValue, ExecuteType(DIRECT_EVAL),
                         NullFramePtr() , vp.address());
}

bool
js::DirectEvalValueFromIon(JSContext* cx,
                           HandleObject scopeobj, HandleScript callerScript,
                           HandleValue thisValue, HandleValue evalArg,
                           jsbytecode* pc, MutableHandleValue vp)
{
    
    if (!evalArg.isString()) {
        vp.set(evalArg);
        return true;
    }

    RootedString string(cx, evalArg.toString());
    return DirectEvalStringFromIon(cx, scopeobj, callerScript, thisValue, string, pc, vp);
}

bool
js::IndirectEval(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, &args.callee().global());
    return EvalKernel(cx, args, INDIRECT_EVAL, NullFramePtr(), global, nullptr);
}

bool
js::DirectEval(JSContext* cx, const CallArgs& args)
{
    
    ScriptFrameIter iter(cx);
    AbstractFramePtr caller = iter.abstractFramePtr();

    MOZ_ASSERT(caller.scopeChain()->global().valueIsEval(args.calleev()));
    MOZ_ASSERT(JSOp(*iter.pc()) == JSOP_EVAL ||
               JSOp(*iter.pc()) == JSOP_STRICTEVAL ||
               JSOp(*iter.pc()) == JSOP_SPREADEVAL ||
               JSOp(*iter.pc()) == JSOP_STRICTSPREADEVAL);
    MOZ_ASSERT_IF(caller.isFunctionFrame(),
                  caller.compartment() == caller.callee()->compartment());

    RootedObject scopeChain(cx, caller.scopeChain());
    return EvalKernel(cx, args, DIRECT_EVAL, caller, scopeChain, iter.pc());
}

bool
js::IsAnyBuiltinEval(JSFunction* fun)
{
    return fun->maybeNative() == IndirectEval;
}

JS_FRIEND_API(bool)
js::ExecuteInGlobalAndReturnScope(JSContext* cx, HandleObject global, HandleScript scriptArg,
                                  MutableHandleObject scopeArg)
{
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, global);
    MOZ_ASSERT(global->is<GlobalObject>());
    MOZ_RELEASE_ASSERT(scriptArg->hasPollutedGlobalScope());

    RootedScript script(cx, scriptArg);
    if (script->compartment() != cx->compartment()) {
        script = CloneScript(cx, NullPtr(), NullPtr(), script);
        if (!script)
            return false;

        Debugger::onNewScript(cx, script);
    }

    RootedObject scope(cx, JS_NewPlainObject(cx));
    if (!scope)
        return false;

    if (!scope->setQualifiedVarObj(cx))
        return false;

    if (!scope->setUnqualifiedVarObj(cx))
        return false;

    JSObject* thisobj = GetThisObject(cx, global);
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
