








































#if !defined(jsion_ion_h__) && defined(JS_ION)
#define jsion_ion_h__

#include "jscntxt.h"
#include "IonCode.h"

namespace js {
namespace ion {

class TempAllocator;

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




class IonContext
{
  public:
    IonContext(JSContext *cx, TempAllocator *temp);
    ~IonContext();

    JSContext *cx;
    TempAllocator *temp;
};


bool InitializeIon();


IonContext *GetIonContext();
bool SetIonContext(IonContext *ctx);

bool Go(JSContext *cx, JSScript *script, js::StackFrame *fp);

extern IonOptions js_IonOptions;

}
}

#endif 

