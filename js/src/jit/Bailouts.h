





#ifndef jit_Bailouts_h
#define jit_Bailouts_h

#include "jstypes.h"

#include "jit/IonFrameIterator.h"
#include "jit/IonFrames.h"
#include "vm/Stack.h"

namespace js {
namespace jit {










































































static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);

static const uint32_t BAILOUT_KIND_BITS = 3;
static const uint32_t BAILOUT_RESUME_BITS = 1;


static const uint32_t BAILOUT_TABLE_SIZE = 16;



static const uint32_t BAILOUT_RETURN_OK = 0;
static const uint32_t BAILOUT_RETURN_FATAL_ERROR = 1;
static const uint32_t BAILOUT_RETURN_OVERRECURSED = 2;

class JitCompartment;



class BailoutStack;
class InvalidationBailoutStack;






class IonBailoutIterator : public IonFrameIterator
{
    MachineState machine_;
    uint32_t snapshotOffset_;
    size_t topFrameSize_;
    IonScript *topIonScript_;

  public:
    IonBailoutIterator(const JitActivationIterator &activations, BailoutStack *sp);
    IonBailoutIterator(const JitActivationIterator &activations, InvalidationBailoutStack *sp);
    IonBailoutIterator(const JitActivationIterator &activations, const IonFrameIterator &frame);

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

bool EnsureHasScopeObjects(JSContext *cx, AbstractFramePtr fp);

struct BaselineBailoutInfo;


uint32_t Bailout(BailoutStack *sp, BaselineBailoutInfo **info);


uint32_t InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut,
                             BaselineBailoutInfo **info);

struct ExceptionBailoutInfo
{
    size_t frameNo;
    jsbytecode *resumePC;
    size_t numExprSlots;
};



uint32_t ExceptionHandlerBailout(JSContext *cx, const InlineFrameIterator &frame,
                                 const ExceptionBailoutInfo &excInfo,
                                 BaselineBailoutInfo **bailoutInfo);

uint32_t FinishBailoutToBaseline(BaselineBailoutInfo *bailoutInfo);

bool CheckFrequentBailouts(JSContext *cx, JSScript *script);

} 
} 

#endif 
