








































#if !defined(jsion_ion_h__) && defined(JS_ION)
#define jsion_ion_h__

#include "jscntxt.h"
#include "IonCode.h"

namespace js {
namespace ion {

struct TempAllocator;

struct IonOptions
{
    
    
    
    bool enabled;

    
    
    
    bool gvn;

    
    
    
    
    bool gvnIsOptimistic;

    
    
    
    bool licm;

    
    
    
    
    bool lsra;

    IonOptions()
      : enabled(false),
        gvn(true),
        gvnIsOptimistic(true),
        licm(true),
        lsra(false)
    { }
};

enum MethodStatus
{
    Method_CantCompile,
    Method_Compiled
};




class IonContext
{
  public:
    IonContext(JSContext *cx, TempAllocator *temp);
    ~IonContext();

    JSContext *cx;
    TempAllocator *temp;
};

extern IonOptions js_IonOptions;


bool InitializeIon();


IonContext *GetIonContext();
bool SetIonContext(IonContext *ctx);

MethodStatus Compile(JSContext *cx, JSScript *script, js::StackFrame *fp);
bool Cannon(JSContext *cx, StackFrame *fp);

static inline bool IsEnabled()
{
    return js_IonOptions.enabled;
}

}
}

#endif 

