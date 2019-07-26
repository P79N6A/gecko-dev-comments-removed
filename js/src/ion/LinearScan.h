






#ifndef js_ion_linearscan_h__
#define js_ion_linearscan_h__

#include "LiveRangeAllocator.h"
#include "BitSet.h"

#include "js/Vector.h"

namespace js {
namespace ion {

class LinearScanVirtualRegister : public VirtualRegister
{
  private:
    LAllocation *canonicalSpill_;
    CodePosition spillPosition_ ;

    bool spillAtDefinition_ : 1;

    
    
    bool finished_ : 1;

  public:
    void setCanonicalSpill(LAllocation *alloc) {
        canonicalSpill_ = alloc;
    }
    LAllocation *canonicalSpill() const {
        return canonicalSpill_;
    }
    unsigned canonicalSpillSlot() const {
        return canonicalSpill_->toStackSlot()->slot();
    }
    void setFinished() {
        finished_ = true;
    }
    bool finished() const {
        return finished_;
    }
    void setSpillAtDefinition(CodePosition pos) {
        spillAtDefinition_ = true;
        setSpillPosition(pos);
    }
    bool mustSpillAtDefinition() const {
        return spillAtDefinition_;
    }
    CodePosition spillPosition() const {
        return spillPosition_;
    }
    void setSpillPosition(CodePosition pos) {
        spillPosition_ = pos;
    }
};

class LinearScanAllocator : public LiveRangeAllocator<LinearScanVirtualRegister>
{
    friend class C1Spewer;
    friend class JSONSpewer;

    
    
    class UnhandledQueue : public InlineList<LiveInterval>
    {
      public:
        void enqueueForward(LiveInterval *after, LiveInterval *interval);
        void enqueueBackward(LiveInterval *interval);
        void enqueueAtHead(LiveInterval *interval);

        void assertSorted();

        LiveInterval *dequeue();
    };

    typedef Vector<LiveInterval *, 0, SystemAllocPolicy> SlotList;
    SlotList finishedSlots_;
    SlotList finishedDoubleSlots_;

    
    UnhandledQueue unhandled;
    InlineList<LiveInterval> active;
    InlineList<LiveInterval> inactive;
    InlineList<LiveInterval> fixed;
    InlineList<LiveInterval> handled;
    LiveInterval *current;

    bool allocateRegisters();
    bool resolveControlFlow();
    bool reifyAllocations();
    bool populateSafepoints();

    
    void enqueueVirtualRegisterIntervals();

    uint32_t allocateSlotFor(const LiveInterval *interval);
    bool splitInterval(LiveInterval *interval, CodePosition pos);
    bool splitBlockingIntervals(LAllocation allocation);
    bool assign(LAllocation allocation);
    bool spill();
    void freeAllocation(LiveInterval *interval, LAllocation *alloc);
    void finishInterval(LiveInterval *interval);
    AnyRegister::Code findBestFreeRegister(CodePosition *freeUntil);
    AnyRegister::Code findBestBlockedRegister(CodePosition *nextUsed);
    bool canCoexist(LiveInterval *a, LiveInterval *b);
    bool moveInputAlloc(CodePosition pos, LAllocation *from, LAllocation *to);
    void setIntervalRequirement(LiveInterval *interval);
    bool isSpilledAt(LiveInterval *interval, CodePosition pos);

#ifdef DEBUG
    void validateIntervals();
    void validateAllocations();
#else
    inline void validateIntervals() { }
    inline void validateAllocations() { }
#endif

  public:
    LinearScanAllocator(MIRGenerator *mir, LIRGenerator *lir, LIRGraph &graph)
      : LiveRangeAllocator<LinearScanVirtualRegister>(mir, lir, graph,  true)
    {
    }

    bool go();
};

} 
} 

#endif
