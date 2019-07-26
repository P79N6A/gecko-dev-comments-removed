






#if !defined(jsion_ion_h__) && defined(JS_ION)
#define jsion_ion_h__

#include "jscntxt.h"
#include "IonCode.h"
#include "jsinfer.h"
#include "jsinterp.h"

namespace js {
namespace ion {

class TempAllocator;

struct IonOptions
{
    
    
    
    bool gvn;

    
    
    
    
    bool gvnIsOptimistic;

    
    
    
    bool licm;

    
    
    
    bool osr;

    
    
    
    bool limitScriptSize;

    
    
    
    
    bool lsra;

    
    
    
    bool inlining;

    
    
    
    bool edgeCaseAnalysis;

    
    
    
    bool rangeAnalysis;

    
    
    
    bool parallelCompilation;

    
    
    
    
    uint32 usesBeforeCompile;

    
    
    
    
    uint32 usesBeforeCompileNoJaeger;

    
    
    
    
    uint32 usesBeforeInlining;

    
    
    
    uint32 maxStackArgs;

    
    
    
    uint32 maxInlineDepth;

    
    
    
    
    
    
    
    uint32 smallFunctionMaxBytecodeLength;

    
    
    
    
    
    
    
    uint32 smallFunctionUsesBeforeInlining;

    
    
    
    uint32 polyInlineMax;

    
    
    
    uint32 inlineMaxTotalBytecodeLength;

    
    
    
    bool eagerCompilation;

    
    
    
    uint32 slowCallLimit;

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
        lsra(true),
        inlining(true),
        edgeCaseAnalysis(true),
        rangeAnalysis(false),
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
        eagerCompilation(false),
        slowCallLimit(512)
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

MethodStatus CanEnterAtBranch(JSContext *cx, JSScript *script,
                              StackFrame *fp, jsbytecode *pc);
MethodStatus CanEnter(JSContext *cx, JSScript *script, StackFrame *fp, bool newType);

enum IonExecStatus
{
    IonExec_Error,
    IonExec_Ok,
    IonExec_Bailout
};

IonExecStatus Cannon(JSContext *cx, StackFrame *fp);
IonExecStatus SideCannon(JSContext *cx, StackFrame *fp, jsbytecode *pc);


void Invalidate(types::TypeCompartment &types, FreeOp *fop,
                const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
void Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
bool Invalidate(JSContext *cx, JSScript *script, bool resetUses = true);

void MarkFromIon(JSCompartment *comp, Value *vp);

void ToggleBarriers(JSCompartment *comp, bool needs);

class IonBuilder;

bool CompileBackEnd(IonBuilder *builder);
void AttachFinishedCompilations(JSContext *cx);
void FinishOffThreadBuilder(IonBuilder *builder);
bool TestIonCompile(JSContext *cx, JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing);

static inline bool IsEnabled(JSContext *cx)
{
    return cx->hasRunOption(JSOPTION_ION) && cx->typeInferenceEnabled();
}

void ForbidCompilation(JSScript *script);
uint32_t UsesBeforeIonRecompile(JSScript *script, jsbytecode *pc);

} 
} 

#endif 

