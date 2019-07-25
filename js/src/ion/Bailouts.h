








































#ifndef jsion_bailouts_h__
#define jsion_bailouts_h__

#include "jstypes.h"
#include "vm/Stack.h"
#include "IonFrames.h"

namespace js {
namespace ion {










































































static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);



enum BailoutKind
{
    
    
    Bailout_Normal,

    
    
    Bailout_ArgumentCheck,

    
    
    Bailout_TypeBarrier,

    
    Bailout_RecompileCheck
};

static const uint32 BAILOUT_KIND_BITS = 2;
static const uint32 BAILOUT_RESUME_BITS = 1;


static const uint32 BAILOUT_TABLE_SIZE = 16;



static const uint32 BAILOUT_RETURN_OK = 0;
static const uint32 BAILOUT_RETURN_FATAL_ERROR = 1;
static const uint32 BAILOUT_RETURN_ARGUMENT_CHECK = 2;
static const uint32 BAILOUT_RETURN_TYPE_BARRIER = 3;
static const uint32 BAILOUT_RETURN_RECOMPILE_CHECK = 4;



class BailoutClosure
{
    BailoutFrameGuard bfg_;
    StackFrame *entryfp_;
    jsbytecode *bailoutPc_;

  public:
    BailoutClosure()
      : bailoutPc_(NULL)
    { }
    BailoutFrameGuard *frameGuard() {
        return &bfg_;
    }
    StackFrame *entryfp() const {
        return entryfp_;
    }
    void setEntryFrame(StackFrame *fp) {
        entryfp_ = fp;
    }

    
    
    
    
    void setBailoutPc(jsbytecode *pc) {
        bailoutPc_ = pc;
    }
    jsbytecode *bailoutPc() const {
        return bailoutPc_;
    }
};

class IonCompartment;



class BailoutStack;
class InvalidationBailoutStack;


FrameRecovery
FrameRecoveryFromBailout(IonCompartment *ion, BailoutStack *sp);

FrameRecovery
FrameRecoveryFromInvalidation(IonCompartment *ion, InvalidationBailoutStack *sp);


uint32 Bailout(BailoutStack *sp);


uint32 InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut);



JSBool ThunkToInterpreter(Value *vp);

uint32 ReflowTypeInfo(uint32 bailoutResult);

uint32 RecompileForInlining();






} 
} 

#endif 

