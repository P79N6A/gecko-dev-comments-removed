







































#if !defined jsjaeger_methodjit_inl_h__ && defined JS_METHODJIT
#define jsjaeger_methodjit_inl_h__

namespace js {
namespace mjit {

enum CompileRequest
{
    CompileRequest_Interpreter,
    CompileRequest_JIT
};


static const size_t CALLS_BEFORE_COMPILE = 16;


static const size_t BACKEDGES_BEFORE_COMPILE = 16;

static inline CompileStatus
CanMethodJIT(JSContext *cx, JSScript *script, StackFrame *fp, CompileRequest request)
{
    if (!cx->methodJitEnabled)
        return Compile_Abort;
    JITScriptStatus status = script->getJITStatus(fp->isConstructing());
    if (status == JITScript_Invalid)
        return Compile_Abort;
    if (request == CompileRequest_Interpreter &&
        status == JITScript_None &&
        !cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS) &&
        script->incCallCount() <= CALLS_BEFORE_COMPILE)
    {
        return Compile_Skipped;
    }
    if (status == JITScript_None)
        return TryCompile(cx, fp);
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
    if (status == JITScript_None &&
        !cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS) &&
        cx->compartment->incBackEdgeCount(pc) <= BACKEDGES_BEFORE_COMPILE)
    {
        return Compile_Skipped;
    }
    if (status == JITScript_None)
        return TryCompile(cx, fp);
    return Compile_Okay;
}

}
}

#endif
