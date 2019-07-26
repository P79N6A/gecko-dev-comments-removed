







































#if !defined jsjaeger_methodjit_inl_h__ && defined JS_METHODJIT
#define jsjaeger_methodjit_inl_h__

namespace js {
namespace mjit {

enum CompileRequest
{
    CompileRequest_Interpreter,
    CompileRequest_JIT
};





static const size_t USES_BEFORE_COMPILE = 16;
static const size_t INFER_USES_BEFORE_COMPILE = 43;

static inline bool
IonGetsFirstChance(JSContext *cx, JSScript *script, bool osr)
{
#ifdef JS_ION
    if (!ion::IsEnabled(cx))
        return false;

    
    if (!script->canIonCompile())
        return false;

    
    if (osr && script->isOsrForbidden())
        return false;

    return true;
#endif
    return false;
}

static inline CompileStatus
CanMethodJIT(JSContext *cx, JSScript *script, bool construct, CompileRequest request)
{
    if (!cx->methodJitEnabled)
        return Compile_Abort;
    JITScriptStatus status = script->getJITStatus(construct);
    if (status == JITScript_Invalid)
        return Compile_Abort;
    if (IonGetsFirstChance(cx, script, false))
        return Compile_Skipped;
    if (request == CompileRequest_Interpreter && status == JITScript_None &&
        !cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS) &&
        (cx->typeInferenceEnabled()
         ? script->incUseCount() <= INFER_USES_BEFORE_COMPILE
         : script->incUseCount() <= USES_BEFORE_COMPILE))
    {
        return Compile_Skipped;
    }
    if (status == JITScript_None)
        return TryCompile(cx, script, construct);
    return Compile_Okay;
}





static inline CompileStatus
CanMethodJITAtBranch(JSContext *cx, JSScript *script, StackFrame *fp, jsbytecode *pc)
{
    if (!cx->methodJitEnabled)
        return Compile_Abort;
    JITScriptStatus status = script->getJITStatus(fp->isConstructing());
    if (status == JITScript_Invalid)
        return Compile_Abort;
    if (IonGetsFirstChance(cx, script, true))
        return Compile_Skipped;
    if (status == JITScript_None && !cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS)) {
        if ((cx->typeInferenceEnabled())
             ? script->incUseCount() <= INFER_USES_BEFORE_COMPILE
             : script->incUseCount() <= USES_BEFORE_COMPILE) {
            return Compile_Skipped;
        }
    }
    if (status == JITScript_None)
        return TryCompile(cx, fp->script(), fp->isConstructing());
    return Compile_Okay;
}

}
}

#endif
