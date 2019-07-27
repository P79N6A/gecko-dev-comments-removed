





#ifndef jit_BacktrackingAllocator_h
#define jit_BacktrackingAllocator_h

#include "mozilla/Array.h"

#include "ds/PriorityQueue.h"
#include "ds/SplayTree.h"
#include "jit/RegisterAllocator.h"
#include "jit/StackSlotAllocator.h"






namespace js {
namespace jit {

class Requirement
{
  public:
    enum Kind {
        NONE,
        REGISTER,
        FIXED,
        MUST_REUSE_INPUT
    };

    Requirement()
      : kind_(NONE)
    { }

    explicit Requirement(Kind kind)
      : kind_(kind)
    {
        
        MOZ_ASSERT(kind != FIXED && kind != MUST_REUSE_INPUT);
    }

    Requirement(Kind kind, CodePosition at)
      : kind_(kind),
        position_(at)
    {
        
        MOZ_ASSERT(kind != FIXED && kind != MUST_REUSE_INPUT);
    }

    explicit Requirement(LAllocation fixed)
      : kind_(FIXED),
        allocation_(fixed)
    {
        MOZ_ASSERT(!fixed.isBogus() && !fixed.isUse());
    }

    
    
    Requirement(LAllocation fixed, CodePosition at)
      : kind_(FIXED),
        allocation_(fixed),
        position_(at)
    {
        MOZ_ASSERT(!fixed.isBogus() && !fixed.isUse());
    }

    Requirement(uint32_t vreg, CodePosition at)
      : kind_(MUST_REUSE_INPUT),
        allocation_(LUse(vreg, LUse::ANY)),
        position_(at)
    { }

    Kind kind() const {
        return kind_;
    }

    LAllocation allocation() const {
        MOZ_ASSERT(!allocation_.isBogus() && !allocation_.isUse());
        return allocation_;
    }

    uint32_t virtualRegister() const {
        MOZ_ASSERT(allocation_.isUse());
        MOZ_ASSERT(kind() == MUST_REUSE_INPUT);
        return allocation_.toUse()->virtualRegister();
    }

    CodePosition pos() const {
        return position_;
    }

    int priority() const;

    bool merge(const Requirement& newRequirement) {
        
        
        MOZ_ASSERT(newRequirement.kind() != Requirement::MUST_REUSE_INPUT);

        if (newRequirement.kind() == Requirement::FIXED) {
            if (kind() == Requirement::FIXED)
                return newRequirement.allocation() == allocation();
            *this = newRequirement;
            return true;
        }

        MOZ_ASSERT(newRequirement.kind() == Requirement::REGISTER);
        if (kind() == Requirement::FIXED)
            return allocation().isRegister();

        *this = newRequirement;
        return true;
    }

    void dump() const;

  private:
    Kind kind_;
    LAllocation allocation_;
    CodePosition position_;
};

struct UsePosition : public TempObject,
                     public InlineForwardListNode<UsePosition>
{
    LUse* use;
    CodePosition pos;

    UsePosition(LUse* use, CodePosition pos) :
        use(use),
        pos(pos)
    {
        
        
        
        MOZ_ASSERT_IF(!use->isFixedRegister(),
                      pos.subpos() == (use->usedAtStart()
                                       ? CodePosition::INPUT
                                       : CodePosition::OUTPUT));
    }
};

typedef InlineForwardListIterator<UsePosition> UsePositionIterator;























class LiveBundle;

class LiveRange : public TempObject
{
  public:
    
    
    
    class BundleLink : public InlineForwardListNode<BundleLink> {};
    class RegisterLink : public InlineForwardListNode<RegisterLink> {};

    typedef InlineForwardListIterator<BundleLink> BundleLinkIterator;
    typedef InlineForwardListIterator<RegisterLink> RegisterLinkIterator;

    
    BundleLink bundleLink;
    RegisterLink registerLink;

    static LiveRange* get(BundleLink* link) {
        return reinterpret_cast<LiveRange*>(reinterpret_cast<uint8_t*>(link) -
                                            offsetof(LiveRange, bundleLink));
    }
    static LiveRange* get(RegisterLink* link) {
        return reinterpret_cast<LiveRange*>(reinterpret_cast<uint8_t*>(link) -
                                            offsetof(LiveRange, registerLink));
    }

    struct Range
    {
        
        CodePosition from;

        
        CodePosition to;

        Range() {}

        Range(CodePosition from, CodePosition to)
          : from(from), to(to)
        {
            MOZ_ASSERT(from < to);
        }

        bool empty() {
            return from == to;
        }
    };

  private:
    
    
    uint32_t vreg_;

    
    
    LiveBundle* bundle_;

    
    Range range_;

    
    InlineForwardList<UsePosition> uses_;

    
    bool hasDefinition_;

    LiveRange(uint32_t vreg, Range range)
      : vreg_(vreg), bundle_(nullptr), range_(range), hasDefinition_(false)
    {
        MOZ_ASSERT(!range.empty());
    }

  public:
    static LiveRange* New(TempAllocator& alloc, uint32_t vreg,
                          CodePosition from, CodePosition to) {
        return new(alloc) LiveRange(vreg, Range(from, to));
    }

    uint32_t vreg() const {
        MOZ_ASSERT(hasVreg());
        return vreg_;
    }
    bool hasVreg() const {
        return vreg_ != 0;
    }

    LiveBundle* bundle() const {
        return bundle_;
    }

    CodePosition from() const {
        return range_.from;
    }
    CodePosition to() const {
        return range_.to;
    }
    bool covers(CodePosition pos) const {
        return pos >= from() && pos < to();
    }

    
    bool contains(LiveRange* other) const;

    
    
    void intersect(LiveRange* other, Range* pre, Range* inside, Range* post) const;

    UsePositionIterator usesBegin() const {
        return uses_.begin();
    }
    bool hasUses() const {
        return !!usesBegin();
    }
    UsePosition* popUse() {
        return uses_.popFront();
    }

    bool hasDefinition() const {
        return hasDefinition_;
    }

    void setFrom(CodePosition from) {
        range_.from = from;
    }
    void setTo(CodePosition to) {
        range_.to = to;
    }

    void setBundle(LiveBundle* bundle) {
        bundle_ = bundle;
    }

    void addUse(UsePosition* use);
    void distributeUses(LiveRange* other);

    void setHasDefinition() {
        MOZ_ASSERT(!hasDefinition_);
        hasDefinition_ = true;
    }

    
#ifdef DEBUG
    const char* toString() const;
#else
    const char* toString() const { return "???"; }
#endif

    
    static int compare(LiveRange* v0, LiveRange* v1) {
        
        if (v0->to() <= v1->from())
            return -1;
        if (v0->from() >= v1->to())
            return 1;
        return 0;
    }
};




class LiveBundle : public TempObject
{
    
    InlineForwardList<LiveRange::BundleLink> ranges_;

    
    LAllocation alloc_;

    
    
    LiveBundle* spillParent_;

    LiveBundle()
      : spillParent_(nullptr)
    { }

  public:
    static LiveBundle* New(TempAllocator& alloc) {
        return new(alloc) LiveBundle();
    }

    LiveRange::BundleLinkIterator rangesBegin() const {
        return ranges_.begin();
    }
    LiveRange* firstRange() const {
        return LiveRange::get(*rangesBegin());
    }
    LiveRange* rangeFor(CodePosition pos) const;
    void addRange(LiveRange* range);
    bool addRange(TempAllocator& alloc, uint32_t vreg, CodePosition from, CodePosition to);
    bool addRangeAndDistributeUses(TempAllocator& alloc, LiveRange* oldRange,
                                   CodePosition from, CodePosition to);
#ifdef DEBUG
    size_t numRanges() const;
#endif

    LAllocation allocation() const {
        return alloc_;
    }
    void setAllocation(LAllocation alloc) {
        alloc_ = alloc;
    }

    LiveBundle* spillParent() {
        return spillParent_;
    }
    void setSpillParent(LiveBundle* spill) {
        spillParent_ = spill;
    }

    bool isSpill() const {
        return alloc_.isStackSlot();
    }

    
#ifdef DEBUG
    const char* toString() const;
#else
    const char* toString() const { return "???"; }
#endif
};





struct VirtualRegisterGroup : public TempObject
{
    
    Vector<uint32_t, 2, JitAllocPolicy> registers;

    
    LAllocation allocation;

    
    LAllocation spill;

    explicit VirtualRegisterGroup(TempAllocator& alloc)
      : registers(alloc), allocation(LUse(0, LUse::ANY)), spill(LUse(0, LUse::ANY))
    {}

    uint32_t canonicalReg() {
        uint32_t minimum = registers[0];
        for (size_t i = 1; i < registers.length(); i++)
            minimum = Min(minimum, registers[i]);
        return minimum;
    }
};


class VirtualRegister
{
    
    LNode* ins_;

    
    LDefinition* def_;

    
    
    InlineForwardList<LiveRange::RegisterLink> ranges_;

    
    bool isTemp_;

    
    
    bool mustCopyInput_;

    
    
    
    VirtualRegisterGroup* group_;

    
    
    
    LiveRange* groupExclude_;

    
    LAllocation canonicalSpill_;

    void operator=(const VirtualRegister&) = delete;
    VirtualRegister(const VirtualRegister&) = delete;

  public:
    explicit VirtualRegister()
    {
        
    }

    void init(LNode* ins, LDefinition* def, bool isTemp) {
        MOZ_ASSERT(!ins_);
        ins_ = ins;
        def_ = def;
        isTemp_ = isTemp;
    }

    LNode* ins() const {
        return ins_;
    }
    LDefinition* def() const {
        return def_;
    }
    LDefinition::Type type() const {
        return def()->type();
    }
    uint32_t vreg() const {
        return def()->virtualRegister();
    }
    bool isCompatible(const AnyRegister& r) const {
        return def_->isCompatibleReg(r);
    }
    bool isCompatible(const VirtualRegister& vr) const {
        return def_->isCompatibleDef(*vr.def_);
    }
    bool isTemp() const {
        return isTemp_;
    }

    void setMustCopyInput() {
        mustCopyInput_ = true;
    }
    bool mustCopyInput() {
        return mustCopyInput_;
    }

    void setCanonicalSpill(LAllocation alloc) {
        MOZ_ASSERT(!alloc.isUse());
        canonicalSpill_ = alloc;
    }
    const LAllocation* canonicalSpill() const {
        return canonicalSpill_.isBogus() ? nullptr : &canonicalSpill_;
    }

    void setGroup(VirtualRegisterGroup* group) {
        group_ = group;
    }
    VirtualRegisterGroup* group() {
        return group_;
    }

    void setGroupExclude(LiveRange* range) {
        groupExclude_ = range;
    }
    LiveRange* groupExclude() {
        return groupExclude_;
    }

    LiveRange::RegisterLinkIterator rangesBegin() const {
        return ranges_.begin();
    }
    bool hasRanges() const {
        return !!rangesBegin();
    }
    LiveRange* rangeFor(CodePosition pos) const;
    void removeRange(LiveRange* range);
    void addRange(LiveRange* range);

    bool addInitialRange(TempAllocator& alloc, CodePosition from, CodePosition to);
    void addInitialUse(UsePosition* use);
    void setInitialDefinition(CodePosition from);
};



typedef js::Vector<CodePosition, 4, SystemAllocPolicy> SplitPositionVector;

class BacktrackingAllocator : protected RegisterAllocator
{
    friend class C1Spewer;
    friend class JSONSpewer;

    BitSet* liveIn;
    FixedList<VirtualRegister> vregs;

    
    LiveBundle* callRanges;

    
    StackSlotAllocator stackSlotAllocator;

    
    
    struct QueueItem
    {
        LiveBundle* bundle;
        VirtualRegisterGroup* group;

        QueueItem(LiveBundle* bundle, size_t priority)
          : bundle(bundle), group(nullptr), priority_(priority)
        {}

        QueueItem(VirtualRegisterGroup* group, size_t priority)
          : bundle(nullptr), group(group), priority_(priority)
        {}

        static size_t priority(const QueueItem& v) {
            return v.priority_;
        }

      private:
        size_t priority_;
    };

    PriorityQueue<QueueItem, QueueItem, 0, SystemAllocPolicy> allocationQueue;

    typedef SplayTree<LiveRange*, LiveRange> LiveRangeSet;

    
    
    struct PhysicalRegister {
        bool allocatable;
        AnyRegister reg;
        LiveRangeSet allocations;

        PhysicalRegister() : allocatable(false) {}
    };
    mozilla::Array<PhysicalRegister, AnyRegister::Total> registers;

    
    
    LiveRangeSet hotcode;

    
    
    size_t numVirtualStackSlots;

    
    struct SpillSlot : public TempObject, public InlineForwardListNode<SpillSlot> {
        LStackSlot alloc;
        LiveRangeSet allocated;

        SpillSlot(uint32_t slot, LifoAlloc* alloc)
          : alloc(slot), allocated(alloc)
        {}
    };
    typedef InlineForwardList<SpillSlot> SpillSlotList;

    
    SpillSlotList normalSlots, doubleSlots, quadSlots;

  public:
    BacktrackingAllocator(MIRGenerator* mir, LIRGenerator* lir, LIRGraph& graph)
      : RegisterAllocator(mir, lir, graph),
        liveIn(nullptr),
        callRanges(nullptr),
        numVirtualStackSlots(0)
    { }

    bool go();

  private:

    typedef Vector<LiveRange*, 4, SystemAllocPolicy> LiveRangeVector;
    typedef Vector<LiveBundle*, 4, SystemAllocPolicy> LiveBundleVector;

    
    bool init();
    bool buildLivenessInfo();

    bool addInitialFixedRange(AnyRegister reg, CodePosition from, CodePosition to);

    VirtualRegister& vreg(const LDefinition* def) {
        return vregs[def->virtualRegister()];
    }
    VirtualRegister& vreg(const LAllocation* alloc) {
        MOZ_ASSERT(alloc->isUse());
        return vregs[alloc->toUse()->virtualRegister()];
    }

    
    bool canAddToGroup(VirtualRegisterGroup* group, VirtualRegister* reg);
    bool tryGroupRegisters(uint32_t vreg0, uint32_t vreg1);
    bool tryGroupReusedRegister(uint32_t def, uint32_t use);
    bool groupAndQueueRegisters();
    bool tryAllocateFixed(LiveBundle* bundle, Requirement requirement,
                          bool* success, bool* pfixed, LiveBundleVector& conflicting);
    bool tryAllocateNonFixed(LiveBundle* bundle, Requirement requirement, Requirement hint,
                             bool* success, bool* pfixed, LiveBundleVector& conflicting);
    bool processBundle(LiveBundle* bundle);
    bool processGroup(VirtualRegisterGroup* group);
    bool computeRequirement(LiveBundle* bundle, Requirement *prequirement, Requirement *phint);
    bool tryAllocateRegister(PhysicalRegister& r, LiveBundle* bundle,
                             bool* success, bool* pfixed, LiveBundleVector& conflicting);
    bool tryAllocateGroupRegister(PhysicalRegister& r, VirtualRegisterGroup* group,
                                  bool* psuccess, LiveBundle** pconflicting);
    bool evictBundle(LiveBundle* bundle);
    bool splitAndRequeueBundles(LiveBundle* bundle, const LiveBundleVector& newBundles);
    void spill(LiveBundle* bundle);

    bool isReusedInput(LUse* use, LNode* ins, bool considerCopy);
    bool isRegisterUse(LUse* use, LNode* ins, bool considerCopy = false);
    bool isRegisterDefinition(LiveRange* range);
    bool addLiveBundle(LiveBundleVector& bundles, uint32_t vreg, LiveBundle* spillParent,
                       CodePosition from, CodePosition to);
    bool pickStackSlot(LiveBundle* bundle);
    bool addBundlesUsingAllocation(VirtualRegister &reg, LAllocation alloc,
                                   LiveBundleVector &bundles);
    bool reuseOrAllocateStackSlot(const LiveBundleVector& bundles, LDefinition::Type type,
                                  LAllocation* palloc);
    bool insertAllRanges(LiveRangeSet& set, const LiveBundleVector& bundles);

    
    bool pickStackSlots();
    bool resolveControlFlow();
    bool reifyAllocations();
    bool populateSafepoints();
    bool annotateMoveGroups();
    size_t findFirstNonCallSafepoint(CodePosition from);
    size_t findFirstSafepoint(CodePosition pos, size_t startFrom);
    void addLiveRegistersForRange(VirtualRegister& reg, LiveRange* range);

    bool addMove(LMoveGroup* moves, LiveRange* from, LiveRange* to, LDefinition::Type type) {
        LAllocation fromAlloc = from->bundle()->allocation();
        LAllocation toAlloc = to->bundle()->allocation();
        MOZ_ASSERT(fromAlloc != toAlloc);
        return moves->add(fromAlloc, toAlloc, type);
    }

    bool moveInput(LInstruction* ins, LiveRange* from, LiveRange* to, LDefinition::Type type) {
        if (from->bundle()->allocation() == to->bundle()->allocation())
            return true;
        LMoveGroup* moves = getInputMoveGroup(ins);
        return addMove(moves, from, to, type);
    }

    bool moveAfter(LInstruction* ins, LiveRange* from, LiveRange* to, LDefinition::Type type) {
        if (from->bundle()->allocation() == to->bundle()->allocation())
            return true;
        LMoveGroup* moves = getMoveGroupAfter(ins);
        return addMove(moves, from, to, type);
    }

    bool moveAtExit(LBlock* block, LiveRange* from, LiveRange* to, LDefinition::Type type) {
        if (from->bundle()->allocation() == to->bundle()->allocation())
            return true;
        LMoveGroup* moves = block->getExitMoveGroup(alloc());
        return addMove(moves, from, to, type);
    }

    bool moveAtEntry(LBlock* block, LiveRange* from, LiveRange* to, LDefinition::Type type) {
        if (from->bundle()->allocation() == to->bundle()->allocation())
            return true;
        LMoveGroup* moves = block->getEntryMoveGroup(alloc());
        return addMove(moves, from, to, type);
    }

    
    void dumpRegisterGroups();
    void dumpFixedRanges();
    void dumpAllocations();

    struct PrintLiveRange;

    bool minimalDef(LiveRange* range, LNode* ins);
    bool minimalUse(LiveRange* range, LNode* ins);
    bool minimalBundle(LiveBundle* bundle, bool* pfixed = nullptr);

    

    size_t computePriority(LiveBundle* bundle);
    size_t computeSpillWeight(LiveBundle* bundle);

    size_t computePriority(const VirtualRegisterGroup* group);
    size_t computeSpillWeight(const VirtualRegisterGroup* group);

    size_t maximumSpillWeight(const LiveBundleVector& bundles);

    bool chooseBundleSplit(LiveBundle* bundle, bool fixed, LiveBundle* conflict);

    bool splitAt(LiveBundle* bundle,
                 const SplitPositionVector& splitPositions);
    bool trySplitAcrossHotcode(LiveBundle* bundle, bool* success);
    bool trySplitAfterLastRegisterUse(LiveBundle* bundle, LiveBundle* conflict, bool* success);
    bool trySplitBeforeFirstRegisterUse(LiveBundle* bundle, LiveBundle* conflict, bool* success);
    bool splitAtAllRegisterUses(LiveBundle* bundle);
    bool splitAcrossCalls(LiveBundle* bundle);

    bool compilingAsmJS() {
        return mir->info().compilingAsmJS();
    }

    bool isVirtualStackSlot(LAllocation alloc) {
        return alloc.isStackSlot() &&
               LAllocation::DATA_MASK - alloc.toStackSlot()->slot() < numVirtualStackSlots;
    }

    void dumpVregs();
};

} 
} 

#endif 
