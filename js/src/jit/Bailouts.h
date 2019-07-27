





#ifndef jit_Bailouts_h
#define jit_Bailouts_h

#include "jstypes.h"

#include "jit/JitFrameIterator.h"
#include "jit/JitFrames.h"
#include "vm/Stack.h"

namespace js {
namespace jit {










































































static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);


static const uint32_t BAILOUT_TABLE_SIZE = 16;



static const uint32_t BAILOUT_RETURN_OK = 0;
static const uint32_t BAILOUT_RETURN_FATAL_ERROR = 1;
static const uint32_t BAILOUT_RETURN_OVERRECURSED = 2;



static uint8_t * const FAKE_JIT_TOP_FOR_BAILOUT = reinterpret_cast<uint8_t*>(0xba1);



class BailoutStack;
class InvalidationBailoutStack;






class BailoutFrameInfo
{
    MachineState machine_;
    uint8_t* framePointer_;
    size_t topFrameSize_;
    IonScript* topIonScript_;
    uint32_t snapshotOffset_;
    JitActivation* activation_;

    void attachOnJitActivation(const JitActivationIterator& activations);

  public:
    BailoutFrameInfo(const JitActivationIterator& activations, BailoutStack* sp);
    BailoutFrameInfo(const JitActivationIterator& activations, InvalidationBailoutStack* sp);
    BailoutFrameInfo(const JitActivationIterator& activations, const JitFrameIterator& frame);
    ~BailoutFrameInfo();

    uint8_t* fp() const {
        return framePointer_;
    }
    SnapshotOffset snapshotOffset() const {
        return snapshotOffset_;
    }
    const MachineState machineState() const {
        return machine_;
    }
    size_t topFrameSize() const {
        return topFrameSize_;
    }
    IonScript* ionScript() const {
        return topIonScript_;
    }
    JitActivation* activation() const {
        return activation_;
    }
};

bool EnsureHasScopeObjects(JSContext* cx, AbstractFramePtr fp);

struct BaselineBailoutInfo;


uint32_t Bailout(BailoutStack* sp, BaselineBailoutInfo** info);


uint32_t InvalidationBailout(InvalidationBailoutStack* sp, size_t* frameSizeOut,
                             BaselineBailoutInfo** info);

class ExceptionBailoutInfo
{
    size_t frameNo_;
    jsbytecode* resumePC_;
    size_t numExprSlots_;

  public:
    ExceptionBailoutInfo(size_t frameNo, jsbytecode* resumePC, size_t numExprSlots)
      : frameNo_(frameNo),
        resumePC_(resumePC),
        numExprSlots_(numExprSlots)
    { }

    ExceptionBailoutInfo()
      : frameNo_(0),
        resumePC_(nullptr),
        numExprSlots_(0)
    { }

    bool catchingException() const {
        return !!resumePC_;
    }
    bool propagatingIonExceptionForDebugMode() const {
        return !resumePC_;
    }

    size_t frameNo() const {
        MOZ_ASSERT(catchingException());
        return frameNo_;
    }
    jsbytecode* resumePC() const {
        MOZ_ASSERT(catchingException());
        return resumePC_;
    }
    size_t numExprSlots() const {
        MOZ_ASSERT(catchingException());
        return numExprSlots_;
    }
};



uint32_t ExceptionHandlerBailout(JSContext* cx, const InlineFrameIterator& frame,
                                 ResumeFromException* rfe,
                                 const ExceptionBailoutInfo& excInfo,
                                 bool* overrecursed);

uint32_t FinishBailoutToBaseline(BaselineBailoutInfo* bailoutInfo);

bool CheckFrequentBailouts(JSContext* cx, JSScript* script);

} 
} 

#endif 
