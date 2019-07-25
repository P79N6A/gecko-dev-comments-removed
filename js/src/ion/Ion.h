








































#ifndef jsion_ion_h__
#define jsion_ion_h__

#include "jscntxt.h"

namespace js {
namespace ion {

class TempAllocator;

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

bool
Go(JSContext *cx, JSScript *script, js::StackFrame *fp);

}
}

#endif 

