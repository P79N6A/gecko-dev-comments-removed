





#ifndef jit_BacktrackingAllocator_h
#define jit_BacktrackingAllocator_h

#include "mozilla/Array.h"

#include "ds/PriorityQueue.h"
#include "ds/SplayTree.h"
#include "jit/LiveRangeAllocator.h"






namespace js {
namespace jit {





struct VirtualRegisterGroup : public TempObject
{
    
    Vector<uint32_t, 2, IonAllocPolicy> registers;

    
    LAllocation allocation;

    
    LAllocation spill;

    explicit VirtualRegisterGroup(TempAllocator &alloc)
      : registers(alloc), allocation(LUse(0, LUse::ANY)), spill(LUse(0, LUse::ANY))
    {}

    uint32_t canonicalReg() {
        uint32_t minimum = registers[0];
        for (size_t i = 1; i < registers.length(); i++)
            minimum = Min(minimum, registers[i]);
        return minimum;
    }
};

class BacktrackingVirtualRegister : public VirtualRegister
{
    
    
    bool mustCopyInput_;

    
    LAllocation canonicalSpill_;

    
    
    CodePosition canonicalSpillExclude_;

    
    
    
    VirtualRegisterGroup *group_;

  public:
    explicit BacktrackingVirtualRegister(TempAllocator &alloc)
      : VirtualRegister(alloc)
    {}
    void setMustCopyInput() {
        mustCopyInput_ = true;
    }
    bool mustCopyInput() {
        return mustCopyInput_;
    }

    void setCanonicalSpill(LAllocation alloc) {
        JS_ASSERT(!alloc.isUse());
        canonicalSpill_ = alloc;
    }
    const LAllocation *canonicalSpill() const {
        return canonicalSpill_.isBogus() ? nullptr : &canonicalSpill_;
    }

    void setCanonicalSpillExclude(CodePosition pos) {
        canonicalSpillExclude_ = pos;
    }
    bool hasCanonicalSpillExclude() const {
        return canonicalSpillExclude_.bits() != 0;
    }
    CodePosition canonicalSpillExclude() const {
        JS_ASSERT(hasCanonicalSpillExclude());
        return canonicalSpillExclude_;
    }

    void setGroup(VirtualRegisterGroup *group) {
        group_ = group;
    }
    VirtualRegisterGroup *group() {
        return group_;
    }
};



typedef js::Vector<CodePosition, 4, SystemAllocPolicy> SplitPositionVector;

class BacktrackingAllocator
  : private LiveRangeAllocator<BacktrackingVirtualRegister,  false>
{
    
    
    struct QueueItem
    {
        LiveInterval *interval;
        VirtualRegisterGroup *group;

        QueueItem(LiveInterval *interval, size_t priority)
          : interval(interval), group(nullptr), priority_(priority)
        {}

        QueueItem(VirtualRegisterGroup *group, size_t priority)
          : interval(nullptr), group(group), priority_(priority)
        {}

        static size_t priority(const QueueItem &v) {
            return v.priority_;
        }

      private:
        size_t priority_;
    };

    PriorityQueue<QueueItem, QueueItem, 0, SystemAllocPolicy> allocationQueue;

    
    struct AllocatedRange {
        LiveInterval *interval;
        const LiveInterval::Range *range;

        AllocatedRange()
          : interval(nullptr), range(nullptr)
        {}

        AllocatedRange(LiveInterval *interval, const LiveInterval::Range *range)
          : interval(interval), range(range)
        {}

        static int compare(const AllocatedRange &v0, const AllocatedRange &v1) {
            
            if (v0.range->to <= v1.range->from)
                return -1;
            if (v0.range->from >= v1.range->to)
                return 1;
            return 0;
        }
    };

    typedef SplayTree<AllocatedRange, AllocatedRange> AllocatedRangeSet;

    
    
    struct PhysicalRegister {
        bool allocatable;
        AnyRegister reg;
        AllocatedRangeSet allocations;

        PhysicalRegister() : allocatable(false) {}
    };
    mozilla::Array<PhysicalRegister, AnyRegister::Total> registers;

    
    
    AllocatedRangeSet hotcode;

  public:
    BacktrackingAllocator(MIRGenerator *mir, LIRGenerator *lir, LIRGraph &graph)
      : LiveRangeAllocator<BacktrackingVirtualRegister,  false>(mir, lir, graph)
    { }

    bool go();

  private:

    typedef Vector<LiveInterval *, 4, SystemAllocPolicy> LiveIntervalVector;

    bool init();
    bool canAddToGroup(VirtualRegisterGroup *group, BacktrackingVirtualRegister *reg);
    bool tryGroupRegisters(uint32_t vreg0, uint32_t vreg1);
    bool tryGroupReusedRegister(uint32_t def, uint32_t use);
    bool groupAndQueueRegisters();
    bool tryAllocateFixed(LiveInterval *interval, bool *success, bool *pfixed, LiveInterval **pconflicting);
    bool tryAllocateNonFixed(LiveInterval *interval, bool *success, bool *pfixed, LiveInterval **pconflicting);
    bool processInterval(LiveInterval *interval);
    bool processGroup(VirtualRegisterGroup *group);
    bool setIntervalRequirement(LiveInterval *interval);
    bool tryAllocateRegister(PhysicalRegister &r, LiveInterval *interval,
                             bool *success, bool *pfixed, LiveInterval **pconflicting);
    bool tryAllocateGroupRegister(PhysicalRegister &r, VirtualRegisterGroup *group,
                                  bool *psuccess, bool *pfixed, LiveInterval **pconflicting);
    bool evictInterval(LiveInterval *interval);
    void distributeUses(LiveInterval *interval, const LiveIntervalVector &newIntervals);
    bool split(LiveInterval *interval, const LiveIntervalVector &newIntervals);
    bool requeueIntervals(const LiveIntervalVector &newIntervals);
    void spill(LiveInterval *interval);

    bool isReusedInput(LUse *use, LInstruction *ins, bool considerCopy);
    bool isRegisterUse(LUse *use, LInstruction *ins, bool considerCopy = false);
    bool isRegisterDefinition(LiveInterval *interval);
    bool addLiveInterval(LiveIntervalVector &intervals, uint32_t vreg,
                         LiveInterval *spillInterval,
                         CodePosition from, CodePosition to);

    bool resolveControlFlow();
    bool reifyAllocations();
    bool populateSafepoints();

    void dumpRegisterGroups();
    void dumpFixedRanges();
    void dumpAllocations();

    struct PrintLiveIntervalRange;

    bool minimalDef(const LiveInterval *interval, LInstruction *ins);
    bool minimalUse(const LiveInterval *interval, LInstruction *ins);
    bool minimalInterval(const LiveInterval *interval, bool *pfixed = nullptr);

    

    size_t computePriority(const LiveInterval *interval);
    size_t computeSpillWeight(const LiveInterval *interval);

    size_t computePriority(const VirtualRegisterGroup *group);
    size_t computeSpillWeight(const VirtualRegisterGroup *group);

    bool chooseIntervalSplit(LiveInterval *interval, LiveInterval *conflict);

    bool splitAt(LiveInterval *interval,
                 const SplitPositionVector &splitPositions);
    bool trySplitAcrossHotcode(LiveInterval *interval, bool *success);
    bool trySplitAfterLastRegisterUse(LiveInterval *interval, LiveInterval *conflict, bool *success);
    bool trySplitBeforeFirstRegisterUse(LiveInterval *interval, LiveInterval *conflict, bool *success);
    bool splitAtAllRegisterUses(LiveInterval *interval);
    bool splitAcrossCalls(LiveInterval *interval);
};

} 
} 

#endif 
