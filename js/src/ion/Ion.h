








































#if !defined(jsion_ion_h__) && defined(JS_ION)
#define jsion_ion_h__

#include "jscntxt.h"
#include "IonCode.h"
#include "jsinfer.h"

namespace js {
namespace ion {

class TempAllocator;

struct IonOptions
{
    
    
    
    bool gvn;

    
    
    
    
    bool gvnIsOptimistic;

    
    
    
    bool licm;

    
    
    
    bool osr;

    
    
    
    
    bool lsra;

    
    
    
    bool inlining;

    
    
    
    bool rangeAnalysis;

    
    
    
    
    uint32 usesBeforeCompile;

    
    
    
    
    uint32 usesBeforeInlining;

    void setEagerCompilation() {
        usesBeforeCompile = 0;

        
        usesBeforeInlining = usesBeforeCompile;
    }

    IonOptions()
      : gvn(true),
        gvnIsOptimistic(true),
        licm(true),
        osr(true),
        lsra(true),
        inlining(true),
        rangeAnalysis(true),
        usesBeforeCompile(40),
        usesBeforeInlining(10240)
    { }
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
    IonContext(JSContext *cx, TempAllocator *temp);
    ~IonContext();

    JSContext *cx;
    TempAllocator *temp;

  private:
    IonContext *prev_;
};

extern IonOptions js_IonOptions;


bool InitializeIon();


IonContext *GetIonContext();
bool SetIonContext(IonContext *ctx);

MethodStatus CanEnterAtBranch(JSContext *cx, JSScript *script,
                              StackFrame *fp, jsbytecode *pc);
MethodStatus CanEnter(JSContext *cx, JSScript *script, StackFrame *fp, bool newType);

bool Cannon(JSContext *cx, StackFrame *fp);
bool SideCannon(JSContext *cx, StackFrame *fp, jsbytecode *pc);


void Invalidate(FreeOp *fop, const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
void InvalidateAll(FreeOp *fop, JSCompartment *comp);
void FinishInvalidation(FreeOp *fop, JSScript *script);
void MarkFromIon(JSCompartment *comp, Value *vp);

static inline bool IsEnabled(JSContext *cx)
{
    return cx->hasRunOption(JSOPTION_ION) && cx->typeInferenceEnabled();
}

} 
} 

#endif 

