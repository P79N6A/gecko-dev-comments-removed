





#ifndef jit_Ion_h
#define jit_Ion_h

#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jscompartment.h"

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






class JitContext
{
  public:
    JitContext(JSContext *cx, TempAllocator *temp);
    JitContext(ExclusiveContext *cx, TempAllocator *temp);
    JitContext(CompileRuntime *rt, CompileCompartment *comp, TempAllocator *temp);
    explicit JitContext(CompileRuntime *rt);
    ~JitContext();

    
    
    JSContext *cx;

    
    TempAllocator *temp;

    
    
    CompileRuntime *runtime;
    CompileCompartment *compartment;

    int getNextAssemblerId() {
        return assemblerCount_++;
    }
  private:
    JitContext *prev_;
    int assemblerCount_;
};


bool InitializeIon();


JitContext *GetJitContext();
JitContext *MaybeGetJitContext();

void SetJitContext(JitContext *ctx);

bool CanIonCompileScript(JSContext *cx, JSScript *script, bool osr);

MethodStatus CanEnterAtBranch(JSContext *cx, JSScript *script,
                              BaselineFrame *frame, jsbytecode *pc);
MethodStatus CanEnter(JSContext *cx, RunState &state);
MethodStatus CompileFunctionForBaseline(JSContext *cx, HandleScript script, BaselineFrame *frame);
MethodStatus CanEnterUsingFastInvoke(JSContext *cx, HandleScript script, uint32_t numActualArgs);

MethodStatus
Recompile(JSContext *cx, HandleScript script, BaselineFrame *osrFrame, jsbytecode *osrPc,
          bool constructing, bool force);

enum JitExecStatus
{
    
    
    JitExec_Aborted,

    
    
    JitExec_Error,

    
    JitExec_Ok
};

static inline bool
IsErrorStatus(JitExecStatus status)
{
    return status == JitExec_Error || status == JitExec_Aborted;
}

struct EnterJitData;

bool SetEnterJitData(JSContext *cx, EnterJitData &data, RunState &state, AutoValueVector &vals);

JitExecStatus IonCannon(JSContext *cx, RunState &state);


JitExecStatus FastInvoke(JSContext *cx, HandleFunction fun, CallArgs &args);


void Invalidate(TypeZone &types, FreeOp *fop,
                const RecompileInfoVector &invalid, bool resetUses = true,
                bool cancelOffThread = true);
void Invalidate(JSContext *cx, const RecompileInfoVector &invalid, bool resetUses = true,
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
void FinishOffThreadBuilder(JSContext *cx, IonBuilder *builder);
void StopAllOffThreadCompilations(Zone *zone);
void StopAllOffThreadCompilations(JSCompartment *comp);

uint8_t *LazyLinkTopActivation(JSContext *cx);

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

void PurgeCaches(JSScript *script);
size_t SizeOfIonData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf);
void DestroyJitScripts(FreeOp *fop, JSScript *script);
void TraceJitScripts(JSTracer* trc, JSScript *script);

bool JitSupportsFloatingPoint();
bool JitSupportsSimd();

} 
} 

#endif 
