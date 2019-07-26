






#if !defined(jsion_ion_h__) && defined(JS_ION)
#define jsion_ion_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "IonCode.h"
#include "jsinfer.h"
#include "jsinterp.h"

namespace js {
namespace ion {

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

    
    
    
    bool uce;

    
    
    
    bool parallelCompilation;

    
    
    
    
    uint32_t usesBeforeCompile;

    
    
    
    
    uint32_t usesBeforeCompileNoJaeger;

    
    
    
    
    uint32_t usesBeforeInlining;

    
    
    
    uint32_t maxStackArgs;

    
    
    
    uint32_t maxInlineDepth;

    
    
    
    
    
    
    
    uint32_t smallFunctionMaxBytecodeLength;

    
    
    
    
    
    
    
    uint32_t smallFunctionUsesBeforeInlining;

    
    
    
    uint32_t polyInlineMax;

    
    
    
    uint32_t inlineMaxTotalBytecodeLength;

    
    
    
    
    uint32_t inlineUseCountRatio;

    
    
    
    bool eagerCompilation;

    
    
    
    uint32_t slowCallLimit;

    
    
    
    
    
    uint32_t slowCallIncUseCount;

    void setEagerCompilation() {
        eagerCompilation = true;
        usesBeforeCompile = usesBeforeCompileNoJaeger = 0;

        
        usesBeforeInlining = 0;
        smallFunctionUsesBeforeInlining = 0;

        parallelCompilation = false;
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
        uce(true),
        parallelCompilation(false),
        usesBeforeCompile(10240),
        usesBeforeCompileNoJaeger(40),
        usesBeforeInlining(usesBeforeCompile),
        maxStackArgs(4096),
        maxInlineDepth(3),
        smallFunctionMaxBytecodeLength(100),
        smallFunctionUsesBeforeInlining(usesBeforeInlining / 4),
        polyInlineMax(4),
        inlineMaxTotalBytecodeLength(800),
        inlineUseCountRatio(128),
        eagerCompilation(false),
        slowCallLimit(512),
        slowCallIncUseCount(5)
    {
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
    AbortReason_NoAbort
};






class IonContext
{
  public:
    IonContext(JSContext *cx, JSCompartment *compartment, TempAllocator *temp);
    ~IonContext();

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

bool SetIonContext(IonContext *ctx);

MethodStatus CanEnterAtBranch(JSContext *cx, HandleScript script,
                              AbstractFramePtr fp, jsbytecode *pc, bool isConstructing);
MethodStatus CanEnter(JSContext *cx, HandleScript script, AbstractFramePtr fp,
                      bool isConstructing, bool newType);
MethodStatus CanEnterUsingFastInvoke(JSContext *cx, HandleScript script, uint32_t numActualArgs);

enum IonExecStatus
{
    
    
    IonExec_Aborted,

    
    
    IonExec_Error,

    
    IonExec_Ok,

    
    
    IonExec_Bailout
};

static inline bool
IsErrorStatus(IonExecStatus status)
{
    return status == IonExec_Error || status == IonExec_Aborted;
}

IonExecStatus Cannon(JSContext *cx, StackFrame *fp);
IonExecStatus SideCannon(JSContext *cx, StackFrame *fp, jsbytecode *pc);


IonExecStatus FastInvoke(JSContext *cx, HandleFunction fun, CallArgsList &args);


void Invalidate(types::TypeCompartment &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
void Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
bool Invalidate(JSContext *cx, UnrootedScript script, bool resetUses = true);

void MarkValueFromIon(JSRuntime *rt, Value *vp);
void MarkShapeFromIon(JSRuntime *rt, Shape **shapep);

void ToggleBarriers(JSCompartment *comp, bool needs);

class IonBuilder;
class MIRGenerator;
class CodeGenerator;

CodeGenerator *CompileBackEnd(MIRGenerator *mir);
void AttachFinishedCompilations(JSContext *cx);
void FinishOffThreadBuilder(IonBuilder *builder);
MethodStatus TestIonCompile(JSContext *cx, HandleScript script, HandleFunction fun, jsbytecode *osrPc, bool constructing);

static inline bool IsEnabled(JSContext *cx)
{
    return cx->hasRunOption(JSOPTION_ION) && cx->typeInferenceEnabled();
}

void ForbidCompilation(JSContext *cx, UnrootedScript script);
uint32_t UsesBeforeIonRecompile(UnrootedScript script, jsbytecode *pc);

void PurgeCaches(UnrootedScript script, JSCompartment *c);
size_t MemoryUsed(UnrootedScript script, JSMallocSizeOfFun mallocSizeOf);
void DestroyIonScripts(FreeOp *fop, UnrootedScript script);
void TraceIonScripts(JSTracer* trc, UnrootedScript script);

} 
} 

#endif 

