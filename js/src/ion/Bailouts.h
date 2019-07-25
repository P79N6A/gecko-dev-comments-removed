








































#ifndef jsion_bailouts_h__
#define jsion_bailouts_h__

#include "jstypes.h"
#include "vm/Stack.h"
#include "IonFrames.h"

namespace js {
namespace ion {










































































typedef uint32 BailoutId;
static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);



enum BailoutKind
{
    
    
    Bailout_Normal,

    
    
    Bailout_ArgumentCheck,

    
    
    Bailout_TypeBarrier
};

static const uint32 BAILOUT_KIND_BITS = 1;


static const uint32 BAILOUT_TABLE_SIZE = 16;



static const uint32 BAILOUT_RETURN_OK = 0;
static const uint32 BAILOUT_RETURN_FATAL_ERROR = 1;
static const uint32 BAILOUT_RETURN_ARGUMENT_CHECK = 2;
static const uint32 BAILOUT_RETURN_TYPE_BARRIER = 3;



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

class IonCompartment;



class BailoutStack;


FrameRecovery
FrameRecoveryFromBailout(IonCompartment *ion, BailoutStack *sp);


uint32 Bailout(BailoutStack *sp);



JSBool ThunkToInterpreter(Value *vp);

uint32 ReflowTypeInfo(uint32 bailoutResult);






}
}

#endif 

