








































#if !defined(jsion_ion_h__) && defined(JS_ION)
#define jsion_ion_h__

#include "jscntxt.h"
#include "IonCode.h"
#include "jsinfer.h"

#ifdef DEBUG

#define TRACK_SNAPSHOTS 1
#endif

namespace js {
namespace ion {

class TempAllocator;

struct IonOptions
{
    
    
    
    bool enabled;

    
    
    
    bool gvn;

    
    
    
    
    bool gvnIsOptimistic;

    
    
    
    bool licm;

    
    
    
    bool osr;

    
    
    
    
    bool lsra;

    
    
    
    bool inlining;

    
    
    
    
    uint32 usesBeforeCompile;

    
    
    
    
    uint32 usesBeforeInlining;

    void setEagerCompilation() {
        usesBeforeCompile = 0;

        
        usesBeforeInlining = usesBeforeCompile;
    }

    IonOptions()
      : enabled(false),
        gvn(true),
        gvnIsOptimistic(false),
        licm(true),
        osr(true),
        lsra(true),
        inlining(true),
        usesBeforeCompile(40),
        usesBeforeInlining(10000)
    { }
};

enum MethodStatus
{
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
MethodStatus Compile(JSContext *cx, JSScript *script,
                     js::StackFrame *fp, jsbytecode *osrPc);

bool Cannon(JSContext *cx, StackFrame *fp);
bool SideCannon(JSContext *cx, StackFrame *fp, jsbytecode *pc);


void Invalidate(JSContext *cx, const Vector<types::RecompileInfo> &invalid, bool resetUses = true);
void InvalidateAll(JSContext *cx, JSCompartment *comp);
void FinishInvalidation(JSContext *cx, JSScript *script);

static inline bool IsEnabled()
{
    return js_IonOptions.enabled;
}

} 
} 

#endif 

