







































#if !defined jsjaeger_methodjit_inl_h__ && defined JS_METHODJIT
#define jsjaeger_methodjit_inl_h__

namespace js {
namespace mjit {

enum CompileRequest
{
    CompileRequest_Interpreter,
    CompileRequest_JIT
};






static const size_t USES_BEFORE_COMPILE       = 16;
static const size_t INFER_USES_BEFORE_COMPILE = 40;

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
        (cx->typeInferenceEnabled()
         ? script->incUseCount() <= INFER_USES_BEFORE_COMPILE
         : script->incUseCount() <= USES_BEFORE_COMPILE))
    {
        return Compile_Skipped;
    }
    if (status == JITScript_None)
        return TryCompile(cx, fp);
    return Compile_Okay;
}

static inline bool
RecursiveMethodJIT(JSContext *cx, StackFrame *fp)
{
    if (!cx->compartment->hasJaegerCompartment())
        return false;

    





    static const unsigned RECURSIVE_METHODJIT_LIMIT = 10;
    VMFrame *f = cx->compartment->jaegerCompartment()->activeFrame();
    for (unsigned i = 0; i < RECURSIVE_METHODJIT_LIMIT; i++) {
        if (!f || f->entryfp != fp)
            return false;
        f = f->previous;
    }
    return true;
}





static inline CompileStatus
CanMethodJITAtBranch(JSContext *cx, JSScript *script, StackFrame *fp, jsbytecode *pc)
{
    if (!cx->methodJitEnabled || RecursiveMethodJIT(cx, fp))
        return Compile_Abort;
    JITScriptStatus status = script->getJITStatus(fp->isConstructing());
    if (status == JITScript_Invalid)
        return Compile_Abort;
    if (status == JITScript_None && !cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS)) {
        







        if (cx->typeInferenceEnabled()) {
            if (script->incUseCount() <= INFER_USES_BEFORE_COMPILE)
                return Compile_Skipped;
        } else {
            if (cx->compartment->incBackEdgeCount(pc) <= USES_BEFORE_COMPILE)
                return Compile_Skipped;
        }
    }
    if (status == JITScript_None)
        return TryCompile(cx, fp);
    return Compile_Okay;
}

}
}

#endif
