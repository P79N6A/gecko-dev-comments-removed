





#ifndef ion_ExecutionModeInlines_h
#define ion_ExecutionModeInlines_h

#ifdef JS_ION

namespace js {
namespace ion {

static inline bool
HasIonScript(JSScript *script, ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return script->hasIonScript();
      case ParallelExecution: return script->hasParallelIonScript();
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
    return false;
}

static inline IonScript *
GetIonScript(JSScript *script, ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return script->maybeIonScript();
      case ParallelExecution: return script->maybeParallelIonScript();
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
    return NULL;
}

static inline void
SetIonScript(JSScript *script, ExecutionMode cmode, IonScript *ionScript)
{
    switch (cmode) {
      case SequentialExecution: script->setIonScript(ionScript); return;
      case ParallelExecution: script->setParallelIonScript(ionScript); return;
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
}

static inline size_t
OffsetOfIonInJSScript(ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return JSScript::offsetOfIonScript();
      case ParallelExecution: return JSScript::offsetOfParallelIonScript();
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
}

static inline bool
CanIonCompile(JSScript *script, ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return script->canIonCompile();
      case ParallelExecution: return script->canParallelIonCompile();
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
    return false;
}

static inline bool
CanIonCompile(JSFunction *fun, ExecutionMode cmode)
{
    return fun->isInterpreted() && CanIonCompile(fun->nonLazyScript(), cmode);
}

static inline bool
CompilingOffThread(JSScript *script, ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return script->isIonCompilingOffThread();
      case ParallelExecution: return script->isParallelIonCompilingOffThread();
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
    return false;
}

static inline bool
CompilingOffThread(HandleScript script, ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return script->isIonCompilingOffThread();
      case ParallelExecution: return script->isParallelIonCompilingOffThread();
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
    return false;
}

static inline types::CompilerOutput::Kind
CompilerOutputKind(ExecutionMode cmode)
{
    switch (cmode) {
      case SequentialExecution: return types::CompilerOutput::Ion;
      case ParallelExecution: return types::CompilerOutput::ParallelIon;
    }
    MOZ_ASSUME_NOT_REACHED("No such execution mode");
    return types::CompilerOutput::Ion;
}

} 
} 

#endif  

#endif 
