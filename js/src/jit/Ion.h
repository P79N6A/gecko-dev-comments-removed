





#ifndef jit_Ion_h
#define jit_Ion_h

#ifdef JS_ION

#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "jit/CompileInfo.h"

namespace js {
namespace jit {

class TempAllocator;


enum IonRegisterAllocator {
    RegisterAllocator_LSRA,
    RegisterAllocator_Backtracking,
    RegisterAllocator_Stupid
};

struct IonOptions
{
    
    
    
    bool gvn;

    
    
    
    
    bool gvnIsOptimistic;

    
    
    
    bool licm;

    
    
    
    bool osr;

    
    
    
    bool limitScriptSize;

    
    
    
    IonRegisterAllocator registerAllocator;

    
    
    
    bool inlining;

    
    
    
    bool edgeCaseAnalysis;

    
    
    
    bool rangeAnalysis;

    
    
    
    
    bool checkRangeAnalysis;

    
    
    
    bool uce;

    
    
    
    bool eaa;

#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    
    
    bool checkOsiPointRegisters;
#endif

    
    
    
    
    uint32_t baselineUsesBeforeCompile;

    
    
    
    
    uint32_t usesBeforeCompile;

    
    
    
    
    double usesBeforeInliningFactor;

    
    
    
    
    uint32_t osrPcMismatchesBeforeRecompile;

    
    
    
    
    uint32_t frequentBailoutThreshold;

    
    
    
    
    uint32_t exceptionBailoutThreshold;

    
    
    
    bool compileTryCatch;

    
    
    
    uint32_t maxStackArgs;

    
    
    
    uint32_t maxInlineDepth;

    
    
    
    
    
    
    
    
    uint32_t smallFunctionMaxInlineDepth;

    
    
    
    
    
    
    
    uint32_t smallFunctionMaxBytecodeLength;

    
    
    
    uint32_t polyInlineMax;

    
    
    
    uint32_t inlineMaxTotalBytecodeLength;

    
    
    
    
    uint32_t inlineUseCountRatio;

    
    
    
    bool eagerCompilation;

    
    
    
    uint32_t usesBeforeCompilePar;

    void setEagerCompilation() {
        eagerCompilation = true;
        usesBeforeCompile = 0;
        baselineUsesBeforeCompile = 0;
    }

    IonOptions()
      : gvn(true),
        gvnIsOptimistic(true),
        licm(true),
        osr(true),
        limitScriptSize(true),
        registerAllocator(RegisterAllocator_LSRA),
        inlining(true),
        edgeCaseAnalysis(true),
        rangeAnalysis(true),
        checkRangeAnalysis(false),
        uce(true),
        eaa(true),
#ifdef CHECK_OSIPOINT_REGISTERS
        checkOsiPointRegisters(false),
#endif
        baselineUsesBeforeCompile(10),
        usesBeforeCompile(1000),
        usesBeforeInliningFactor(.125),
        osrPcMismatchesBeforeRecompile(6000),
        frequentBailoutThreshold(10),
        exceptionBailoutThreshold(10),
        compileTryCatch(true),
        maxStackArgs(4096),
        maxInlineDepth(3),
        smallFunctionMaxInlineDepth(10),
        smallFunctionMaxBytecodeLength(100),
        polyInlineMax(4),
        inlineMaxTotalBytecodeLength(1000),
        inlineUseCountRatio(128),
        eagerCompilation(false),
        usesBeforeCompilePar(1)
    {
    }

    uint32_t usesBeforeInlining() {
        return usesBeforeCompile * usesBeforeInliningFactor;
    }
};

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
    AbortReason_Disable,
    AbortReason_Error,
    AbortReason_NoAbort
};






class IonContext
{
  public:
    IonContext(JSContext *cx, TempAllocator *temp);
    IonContext(ExclusiveContext *cx, TempAllocator *temp);
    IonContext(JSRuntime *rt, JSCompartment *comp, TempAllocator *temp);
    IonContext(JSRuntime *rt);
    ~IonContext();

    JSRuntime *runtime;
    JSContext *cx;
    JSCompartment *compartment;
    TempAllocator *temp;
    int getNextAssemblerId() {
        return assemblerCount_++;
    }
  private:
    IonContext *prev_;
    int assemblerCount_;
};

extern IonOptions js_IonOptions;


bool InitializeIon();


IonContext *GetIonContext();
IonContext *MaybeGetIonContext();

bool SetIonContext(IonContext *ctx);

bool CanIonCompileScript(JSContext *cx, HandleScript script, bool osr);

MethodStatus CanEnterAtBranch(JSContext *cx, JSScript *script,
                              BaselineFrame *frame, jsbytecode *pc, bool isConstructing);
MethodStatus CanEnter(JSContext *cx, RunState &state);
MethodStatus CompileFunctionForBaseline(JSContext *cx, HandleScript script, BaselineFrame *frame,
                                        bool isConstructing);
MethodStatus CanEnterUsingFastInvoke(JSContext *cx, HandleScript script, uint32_t numActualArgs);

MethodStatus CanEnterInParallel(JSContext *cx, HandleScript script);

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

IonExecStatus Cannon(JSContext *cx, RunState &state);


IonExecStatus FastInvoke(JSContext *cx, HandleFunction fun, CallArgs &args);


void Invalidate(types::TypeCompartment &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
void Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
bool Invalidate(JSContext *cx, JSScript *script, ExecutionMode mode, bool resetUses = true);
bool Invalidate(JSContext *cx, JSScript *script, bool resetUses = true);

void MarkValueFromIon(JSRuntime *rt, Value *vp);
void MarkShapeFromIon(JSRuntime *rt, Shape **shapep);

void ToggleBarriers(JS::Zone *zone, bool needs);

class IonBuilder;
class MIRGenerator;
class LIRGraph;
class CodeGenerator;

bool OptimizeMIR(MIRGenerator *mir);
LIRGraph *GenerateLIR(MIRGenerator *mir);
CodeGenerator *GenerateCode(MIRGenerator *mir, LIRGraph *lir, MacroAssembler *maybeMasm = nullptr);
CodeGenerator *CompileBackEnd(MIRGenerator *mir, MacroAssembler *maybeMasm = nullptr);

void AttachFinishedCompilations(JSContext *cx);
void FinishOffThreadBuilder(IonBuilder *builder);

static inline bool
IsIonEnabled(JSContext *cx)
{
    return cx->options().ion() &&
        cx->options().baseline() &&
        cx->typeInferenceEnabled();
}

inline bool
IsIonInlinablePC(jsbytecode *pc) {
    
    
    
    return IsCallPC(pc) || IsGetPropPC(pc) || IsSetPropPC(pc);
}

void ForbidCompilation(JSContext *cx, JSScript *script);
void ForbidCompilation(JSContext *cx, JSScript *script, ExecutionMode mode);
uint32_t UsesBeforeIonRecompile(JSScript *script, jsbytecode *pc);

void PurgeCaches(JSScript *script, JS::Zone *zone);
size_t SizeOfIonData(JSScript *script, mozilla::MallocSizeOf mallocSizeOf);
void DestroyIonScripts(FreeOp *fop, JSScript *script);
void TraceIonScripts(JSTracer* trc, JSScript *script);

void TriggerOperationCallbackForIonCode(JSRuntime *rt, JSRuntime::OperationCallbackTrigger trigger);

} 
} 

#endif 

#endif 
