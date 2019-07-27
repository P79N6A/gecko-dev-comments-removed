





#ifndef jit_BaselineDebugModeOSR_h
#define jit_BaselineDebugModeOSR_h

#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"

namespace js {
namespace jit {






















template <typename T>
class DebugModeOSRVolatileStub
{
    T stub_;
    BaselineFrame *frame_;
    uint32_t pcOffset_;

  public:
    DebugModeOSRVolatileStub(BaselineFrame *frame, ICFallbackStub *stub)
      : stub_(static_cast<T>(stub)),
        frame_(frame),
        pcOffset_(stub->icEntry()->pcOffset())
    { }

    bool invalid() const {
        ICEntry &entry = frame_->script()->baselineScript()->icEntryFromPCOffset(pcOffset_);
        return stub_ != entry.fallbackStub();
    }

    operator const T&() const { MOZ_ASSERT(!invalid()); return stub_; }
    T operator->() const { MOZ_ASSERT(!invalid()); return stub_; }
    T *address() { MOZ_ASSERT(!invalid()); return &stub_; }
    const T *address() const { MOZ_ASSERT(!invalid()); return &stub_; }
    T &get() { MOZ_ASSERT(!invalid()); return stub_; }
    const T &get() const { MOZ_ASSERT(!invalid()); return stub_; }

    bool operator!=(const T &other) const { MOZ_ASSERT(!invalid()); return stub_ != other; }
    bool operator==(const T &other) const { MOZ_ASSERT(!invalid()); return stub_ == other; }
};




struct BaselineDebugModeOSRInfo
{
    uint8_t *resumeAddr;
    jsbytecode *pc;
    PCMappingSlotInfo slotInfo;
    ICEntry::Kind frameKind;

    
    uintptr_t stackAdjust;
    Value valueR0;
    Value valueR1;

    BaselineDebugModeOSRInfo(jsbytecode *pc, ICEntry::Kind kind)
      : resumeAddr(nullptr),
        pc(pc),
        slotInfo(0),
        frameKind(kind),
        stackAdjust(0),
        valueR0(UndefinedValue()),
        valueR1(UndefinedValue())
    { }

    void popValueInto(PCMappingSlotInfo::SlotLocation loc, Value *vp);
};

bool
RecompileOnStackBaselineScriptsForDebugMode(JSContext *cx, JSCompartment *comp);

} 
} 

#endif 
