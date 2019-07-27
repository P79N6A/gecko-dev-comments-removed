





#ifndef jit_Ion_h
#define jit_Ion_h

#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "jit/CompileInfo.h"
#include "jit/CompileWrappers.h"
#include "jit/JitOptions.h"

namespace js {
namespace jit {

class TempAllocator;

enum MethodStatus
{
    Method_Error,
    Method_CantCompile,
    Method_Skipped,
    Method_Compiled
};

enum AbortReason {
    AbortReason_Alloc,
    AbortReason_Inlining,
    AbortReason_NewScriptProperties,
    AbortReason_Disable,
    AbortReason_Error,
    AbortReason_NoAbort
};






class IonContext
{
  public:
    IonContext(JSContext *cx, TempAllocator *temp);
    IonContext(ExclusiveContext *cx, TempAllocator *temp);
    IonContext(CompileRuntime *rt, CompileCompartment *comp, TempAllocator *temp);
    explicit IonContext(CompileRuntime *rt);
    ~IonContext();

    
    
    JSContext *cx;

    
    TempAllocator *temp;

    
    
    CompileRuntime *runtime;
    CompileCompartment *compartment;

    int getNextAssemblerId() {
        return assemblerCount_++;
    }
  private:
    IonContext *prev_;
    int assemblerCount_;
};


bool InitializeIon();


IonContext *GetIonContext();
IonContext *MaybeGetIonContext();

void SetIonContext(IonContext *ctx);

bool CanIonCompileScript(JSContext *cx, JSScript *script, bool osr);

MethodStatus CanEnterAtBranch(JSContext *cx, JSScript *script,
                              BaselineFrame *frame, jsbytecode *pc);
MethodStatus CanEnter(JSContext *cx, RunState &state);
MethodStatus CompileFunctionForBaseline(JSContext *cx, HandleScript script, BaselineFrame *frame);
MethodStatus CanEnterUsingFastInvoke(JSContext *cx, HandleScript script, uint32_t numActualArgs);

MethodStatus CanEnterInParallel(JSContext *cx, HandleScript script);

MethodStatus
Recompile(JSContext *cx, HandleScript script, BaselineFrame *osrFrame, jsbytecode *osrPc,
          bool constructing);

enum IonExecStatus
{
    
    
    IonExec_Aborted,

    
    
    IonExec_Error,

    
    IonExec_Ok
};

static inline bool
IsErrorStatus(IonExecStatus status)
{
    return status == IonExec_Error || status == IonExec_Aborted;
}

struct EnterJitData;

bool SetEnterJitData(JSContext *cx, EnterJitData &data, RunState &state, AutoValueVector &vals);

IonExecStatus IonCannon(JSContext *cx, RunState &state);


IonExecStatus FastInvoke(JSContext *cx, HandleFunction fun, CallArgs &args);


void Invalidate(types::TypeZone &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses = true,
                bool cancelOffThread = true);
void Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses = true,
                bool cancelOffThread = true);
bool Invalidate(JSContext *cx, JSScript *script, ExecutionMode mode, bool resetUses = true,
                bool cancelOffThread = true);
bool Invalidate(JSContext *cx, JSScript *script, bool resetUses = true,
                bool cancelOffThread = true);

void ToggleBarriers(JS::Zone *zone, bool needs);

class IonBuilder;
class MIRGenerator;
class LIRGraph;
class CodeGenerator;

bool OptimizeMIR(MIRGenerator *mir);
LIRGraph *GenerateLIR(MIRGenerator *mir);
CodeGenerator *GenerateCode(MIRGenerator *mir, LIRGraph *lir);
CodeGenerator *CompileBackEnd(MIRGenerator *mir);

void AttachFinishedCompilations(JSContext *cx);
void FinishOffThreadBuilder(IonBuilder *builder);
void StopAllOffThreadCompilations(JSCompartment *comp);

static inline bool
IsIonEnabled(JSContext *cx)
{
#ifdef JS_CODEGEN_NONE
    return false;
#else
    return cx->runtime()->options().ion() &&
           cx->runtime()->options().baseline() &&
           cx->runtime()->jitSupportsFloatingPoint;
#endif
}

inline bool
IsIonInlinablePC(jsbytecode *pc) {
    
    
    
    return IsCallPC(pc) || IsGetPropPC(pc) || IsSetPropPC(pc);
}

inline bool
TooManyActualArguments(unsigned nargs)
{
    return nargs > js_JitOptions.maxStackArgs;
}

inline bool
TooManyFormalArguments(unsigned nargs)
{
    return nargs >= SNAPSHOT_MAX_NARGS || TooManyActualArguments(nargs);
}

inline size_t
NumLocalsAndArgs(JSScript *script)
{
    size_t num = 1  + script->nfixed();
    if (JSFunction *fun = script->functionNonDelazifying())
        num += fun->nargs();
    return num;
}

void ForbidCompilation(JSContext *cx, JSScript *script);
void ForbidCompilation(JSContext *cx, JSScript *script, ExecutionMode mode);

void PurgeCaches(JSScript *script);
size_t SizeOfIonData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf);
void DestroyIonScripts(FreeOp *fop, JSScript *script);
void TraceIonScripts(JSTracer* trc, JSScript *script);

void RequestInterruptForIonCode(JSRuntime *rt, JSRuntime::InterruptMode mode);

bool RematerializeAllFrames(JSContext *cx, JSCompartment *comp);
bool UpdateForDebugMode(JSContext *maybecx, JSCompartment *comp,
                        AutoDebugModeInvalidation &invalidate);

bool JitSupportsFloatingPoint();
bool JitSupportsSimd();

} 
} 

#endif 
