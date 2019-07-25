








































#ifndef jsion_bailouts_h__
#define jsion_bailouts_h__

#include "jstypes.h"

#if defined(JS_CPU_X86)
# include "ion/x86/Bailouts-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/Bailouts-x64.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/Bailouts-arm.h"
#else
# error "CPU!"
#endif

namespace js {
namespace ion {










































































typedef uint32 BailoutId;
static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);


static const uint32 BAILOUT_TABLE_SIZE = 16;


static const uint32 BAILOUT_RETURN_OK = 0;
static const uint32 BAILOUT_RETURN_FATAL_ERROR = 1;



class BailoutClosure
{
    BailoutFrameGuard bfg_;
    StackFrame *entryfp_;

  public:
    BailoutFrameGuard *frameGuard() {
        return &bfg_;
    }
    StackFrame *entryfp() const {
        return entryfp_;
    }
    void setEntryFrame(StackFrame *fp) {
        entryfp_ = fp;
    }
};


uint32 Bailout(void **esp);



JSBool ThunkToInterpreter(IonFramePrefix *top, Value *vp);




uint32 HandleException(IonFramePrefix *top);

}
}

#endif 

