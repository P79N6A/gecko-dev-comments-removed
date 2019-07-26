






#ifndef jsion_bailouts_h__
#define jsion_bailouts_h__

#include "jstypes.h"
#include "vm/Stack.h"
#include "IonFrameIterator.h"
#include "IonFrames.h"

namespace js {
namespace ion {










































































static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);

static const uint32_t BAILOUT_KIND_BITS = 3;
static const uint32_t BAILOUT_RESUME_BITS = 1;


static const uint32_t BAILOUT_TABLE_SIZE = 16;



static const uint32_t BAILOUT_RETURN_OK = 0;
static const uint32_t BAILOUT_RETURN_FATAL_ERROR = 1;
static const uint32_t BAILOUT_RETURN_ARGUMENT_CHECK = 2;
static const uint32_t BAILOUT_RETURN_TYPE_BARRIER = 3;
static const uint32_t BAILOUT_RETURN_MONITOR = 4;
static const uint32_t BAILOUT_RETURN_BOUNDS_CHECK = 5;
static const uint32_t BAILOUT_RETURN_SHAPE_GUARD = 6;
static const uint32_t BAILOUT_RETURN_OVERRECURSED = 7;
static const uint32_t BAILOUT_RETURN_CACHED_SHAPE_GUARD = 8;



class BailoutClosure
{
    
    
    
    
    struct Guards {
        InvokeArgsGuard iag;
        BailoutFrameGuard bfg;
    };

    mozilla::Maybe<Guards> guards_;

    StackFrame *entryfp_;
    jsbytecode *bailoutPc_;

  public:
    BailoutClosure()
      : bailoutPc_(NULL)
    { }

    void constructFrame() {
        guards_.construct();
    }
    InvokeArgsGuard *argsGuard() {
        return &guards_.ref().iag;
    }
    BailoutFrameGuard *frameGuard() {
        return &guards_.ref().bfg;
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






class IonBailoutIterator : public IonFrameIterator
{
    MachineState machine_;
    uint32_t snapshotOffset_;
    size_t topFrameSize_;
    IonScript *topIonScript_;

  public:
    IonBailoutIterator(const IonActivationIterator &activations, BailoutStack *sp);
    IonBailoutIterator(const IonActivationIterator &activations, InvalidationBailoutStack *sp);

    SnapshotOffset snapshotOffset() const {
        JS_ASSERT(topIonScript_);
        return snapshotOffset_;
    }
    const MachineState &machineState() const {
        return machine_;
    }
    size_t topFrameSize() const {
        JS_ASSERT(topIonScript_);
        return topFrameSize_;
    }
    IonScript *ionScript() const {
        if (topIonScript_)
            return topIonScript_;
        return IonFrameIterator::ionScript();
    }

    void dump() const;
};

bool EnsureHasScopeObjects(JSContext *cx, StackFrame *fp);


uint32_t Bailout(BailoutStack *sp);


uint32_t InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut);



uint32_t ThunkToInterpreter(Value *vp);

uint32_t ReflowTypeInfo(uint32_t bailoutResult);

uint32_t BoundsCheckFailure();

uint32_t ShapeGuardFailure();

uint32_t CachedShapeGuardFailure();

} 
} 

#endif 

