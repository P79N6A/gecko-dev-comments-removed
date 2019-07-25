







































#ifndef js_ion_registerallocator_h__
#define js_ion_registerallocator_h__

#include "Ion.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "InlineList.h"
#include "LIR.h"
#include "Lowering.h"
#include "BitSet.h"
#include "StackSlotAllocator.h"

#include "js/Vector.h"

namespace js {
namespace ion {

class VirtualRegister;













class CodePosition
{
  private:
    CodePosition(const uint32 &bits)
      : bits_(bits)
    { }

    static const unsigned int INSTRUCTION_SHIFT = 1;
    static const unsigned int SUBPOSITION_MASK = 1;
    uint32 bits_;

  public:
    static const CodePosition MAX;
    static const CodePosition MIN;

    



    enum SubPosition {
        INPUT,
        OUTPUT
    };

    CodePosition() : bits_(0)
    { }

    CodePosition(uint32 instruction, SubPosition where) {
        JS_ASSERT(instruction < 0x80000000u);
        JS_ASSERT(((uint32)where & SUBPOSITION_MASK) == (uint32)where);
        bits_ = (instruction << INSTRUCTION_SHIFT) | (uint32)where;
    }

    uint32 ins() const {
        return bits_ >> INSTRUCTION_SHIFT;
    }

    uint32 pos() const {
        return bits_;
    }

    SubPosition subpos() const {
        return (SubPosition)(bits_ & SUBPOSITION_MASK);
    }

    bool operator <(const CodePosition &other) const {
        return bits_ < other.bits_;
    }

    bool operator <=(const CodePosition &other) const {
        return bits_ <= other.bits_;
    }

    bool operator !=(const CodePosition &other) const {
        return bits_ != other.bits_;
    }

    bool operator ==(const CodePosition &other) const {
        return bits_ == other.bits_;
    }

    bool operator >(const CodePosition &other) const {
        return bits_ > other.bits_;
    }

    bool operator >=(const CodePosition &other) const {
        return bits_ >= other.bits_;
    }

    CodePosition previous() const {
        JS_ASSERT(*this != MIN);
        return CodePosition(bits_ - 1);
    }
    CodePosition next() const {
        JS_ASSERT(*this != MAX);
        return CodePosition(bits_ + 1);
    }
};

struct UsePosition : public TempObject,
                     public InlineForwardListNode<UsePosition>
{
    LUse *use;
    CodePosition pos;

    UsePosition(LUse *use, CodePosition pos) :
        use(use),
        pos(pos)
    { }
};

class Requirement
{
  public:
    enum Kind {
        NONE,
        REGISTER,
        FIXED,
        SAME_AS_OTHER
    };

    Requirement()
      : kind_(NONE)
    { }

    Requirement(Kind kind)
      : kind_(kind)
    {
        
        JS_ASSERT(kind != FIXED && kind != SAME_AS_OTHER);
    }

    Requirement(Kind kind, CodePosition at)
      : kind_(kind),
        position_(at)
    { }

    Requirement(LAllocation fixed)
      : kind_(FIXED),
        allocation_(fixed)
    { }

    
    
    Requirement(LAllocation fixed, CodePosition at)
      : kind_(FIXED),
        allocation_(fixed),
        position_(at)
    { }

    Requirement(uint32 vreg, CodePosition at)
      : kind_(SAME_AS_OTHER),
        allocation_(LUse(vreg, LUse::ANY)),
        position_(at)
    { }

    Kind kind() const {
        return kind_;
    }

    LAllocation allocation() const {
        JS_ASSERT(!allocation_.isUse());
        return allocation_;
    }

    uint32 virtualRegister() const {
        JS_ASSERT(allocation_.isUse());
        return allocation_.toUse()->virtualRegister();
    }

    CodePosition pos() const {
        return position_;
    }

    int priority() const;

  private:
    Kind kind_;
    LAllocation allocation_;
    CodePosition position_;
};

typedef InlineForwardListIterator<UsePosition> UsePositionIterator;







class LiveInterval
  : public InlineListNode<LiveInterval>,
    public TempObject
{
  public:
    



    struct Range {
        Range(CodePosition f, CodePosition t)
          : from(f),
            to(t)
        {
            JS_ASSERT(from < to);
        }
        CodePosition from;

        
        CodePosition to;
    };

  private:
    Vector<Range, 1, IonAllocPolicy> ranges_;
    LAllocation alloc_;
    VirtualRegister *reg_;
    uint32 index_;
    Requirement requirement_;
    Requirement hint_;
    InlineForwardList<UsePosition> uses_;
    size_t lastProcessedRange_;

  public:

    LiveInterval(VirtualRegister *reg, uint32 index)
      : reg_(reg),
        index_(index),
        lastProcessedRange_(size_t(-1))
    { }

    bool addRange(CodePosition from, CodePosition to);
    void setFrom(CodePosition from);
    CodePosition intersect(LiveInterval *other);
    bool covers(CodePosition pos);
    CodePosition nextCoveredAfter(CodePosition pos);

    CodePosition start() const {
        JS_ASSERT(!ranges_.empty());
        return ranges_.back().from;
    }

    CodePosition end() const {
        JS_ASSERT(!ranges_.empty());
        return ranges_.begin()->to;
    }

    size_t numRanges() const {
        return ranges_.length();
    }
    const Range *getRange(size_t i) const {
        return &ranges_[i];
    }
    void setLastProcessedRange(size_t range, DebugOnly<CodePosition> pos) {
        
        
        JS_ASSERT(ranges_[range].from <= pos);
        lastProcessedRange_ = range;
    }
    size_t lastProcessedRangeIfValid(CodePosition pos) const {
        if (lastProcessedRange_ < ranges_.length() && ranges_[lastProcessedRange_].from <= pos)
            return lastProcessedRange_;
        return ranges_.length() - 1;
    }

    LAllocation *getAllocation() {
        return &alloc_;
    }
    void setAllocation(LAllocation alloc) {
        alloc_ = alloc;
    }
    VirtualRegister *reg() const {
        return reg_;
    }
    uint32 index() const {
        return index_;
    }
    void setIndex(uint32 index) {
        index_ = index;
    }
    Requirement *requirement() {
        return &requirement_;
    }
    void setRequirement(const Requirement &requirement) {
        
        
        JS_ASSERT(requirement.kind() != Requirement::SAME_AS_OTHER);

        
        
        JS_ASSERT_IF(requirement.kind() == Requirement::FIXED,
                     !requirement.allocation().isRegister());

        requirement_ = requirement;
    }
    Requirement *hint() {
        return &hint_;
    }
    void setHint(const Requirement &hint) {
        hint_ = hint;
    }
    bool isSpill() const {
        return alloc_.isStackSlot();
    }
    bool splitFrom(CodePosition pos, LiveInterval *after);

    void addUse(UsePosition *use);
    UsePosition *nextUseAfter(CodePosition pos);
    CodePosition nextUsePosAfter(CodePosition pos);
    CodePosition firstIncompatibleUse(LAllocation alloc);

    UsePositionIterator usesBegin() const {
        return uses_.begin();
    }

    UsePositionIterator usesEnd() const {
        return uses_.end();
    }
};






class VirtualRegister
{
    uint32 reg_;
    LBlock *block_;
    LInstruction *ins_;
    LDefinition *def_;
    Vector<LiveInterval *, 1, IonAllocPolicy> intervals_;
    LMoveGroup *movesBefore_;
    LMoveGroup *inputMoves_;
    LMoveGroup *movesAfter_;
    LAllocation *canonicalSpill_;
    CodePosition spillPosition_ ;

    bool spillAtDefinition_ : 1;

    
    
    bool finished_ : 1;

    
    bool isTemp_ : 1;

  public:
    bool init(uint32 reg, LBlock *block, LInstruction *ins, LDefinition *def, bool isTemp) {
        reg_ = reg;
        block_ = block;
        ins_ = ins;
        def_ = def;
        isTemp_ = isTemp;
        LiveInterval *initial = new LiveInterval(this, 0);
        if (!initial)
            return false;
        return intervals_.append(initial);
    }
    uint32 reg() {
        return reg_;
    }
    LBlock *block() {
        return block_;
    }
    LInstruction *ins() {
        return ins_;
    }
    LDefinition *def() const {
        return def_;
    }
    LDefinition::Type type() const {
        return def()->type();
    }
    bool isTemp() const {
        return isTemp_;
    }
    size_t numIntervals() {
        return intervals_.length();
    }
    LiveInterval *getInterval(size_t i) {
        return intervals_[i];
    }
    bool addInterval(LiveInterval *interval) {
        JS_ASSERT(interval->numRanges());

        
        LiveInterval **found = NULL;
        LiveInterval **i;
        for (i = intervals_.begin(); i != intervals_.end(); i++) {
            if (!found && interval->start() < (*i)->start())
                found = i;
            if (found)
                (*i)->setIndex((*i)->index() + 1);
        }
        if (!found)
            found = intervals_.end();
        return intervals_.insert(found, interval);
    }
    void setMovesBefore(LMoveGroup *moves) {
        movesBefore_ = moves;
    }
    LMoveGroup *movesBefore() const {
        return movesBefore_;
    }
    void setInputMoves(LMoveGroup *moves) {
        inputMoves_ = moves;
    }
    LMoveGroup *inputMoves() const {
        return inputMoves_;
    }
    void setMovesAfter(LMoveGroup *moves) {
        movesAfter_ = moves;
    }
    LMoveGroup *movesAfter() const {
        return movesAfter_;
    }
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
    bool isDouble() const {
        return def_->type() == LDefinition::DOUBLE;
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

    LiveInterval *intervalFor(CodePosition pos);
    LiveInterval *getFirstInterval();
};

class VirtualRegisterMap
{
  private:
    VirtualRegister *vregs_;
    uint32 numVregs_;

  public:
    VirtualRegisterMap()
      : vregs_(NULL),
        numVregs_(0)
    { }

    bool init(MIRGenerator *gen, uint32 numVregs) {
        vregs_ = gen->allocate<VirtualRegister>(numVregs);
        numVregs_ = numVregs;
        if (!vregs_)
            return false;
        memset(vregs_, 0, sizeof(VirtualRegister) * numVregs);
        return true;
    }
    VirtualRegister &operator[](unsigned int index) {
        JS_ASSERT(index < numVregs_);
        return vregs_[index];
    }
    VirtualRegister &operator[](const LAllocation *alloc) {
        JS_ASSERT(alloc->isUse());
        JS_ASSERT(alloc->toUse()->virtualRegister() < numVregs_);
        return vregs_[alloc->toUse()->virtualRegister()];
    }
    VirtualRegister &operator[](const LDefinition *def) {
        JS_ASSERT(def->virtualRegister() < numVregs_);
        return vregs_[def->virtualRegister()];
    }
    VirtualRegister &operator[](const CodePosition &pos) {
        JS_ASSERT(pos.ins() < numVregs_);
        return vregs_[pos.ins()];
    }
    VirtualRegister &operator[](const LInstruction *ins) {
        JS_ASSERT(ins->id() < numVregs_);
        return vregs_[ins->id()];
    }
    uint32 numVirtualRegisters() const {
        return numVregs_;
    }
};

typedef HashMap<uint32,
                LInstruction *,
                DefaultHasher<uint32>,
                IonAllocPolicy> InstructionMap;

typedef HashMap<uint32,
                LiveInterval *,
                DefaultHasher<uint32>,
                IonAllocPolicy> LiveMap;

typedef InlineList<LiveInterval>::iterator IntervalIterator;
typedef InlineList<LiveInterval>::reverse_iterator IntervalReverseIterator;

class LinearScanAllocator
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

    
    LIRGenerator *lir;
    LIRGraph &graph;

    
    BitSet **liveIn;
    VirtualRegisterMap vregs;
    FixedArityList<LiveInterval *, AnyRegister::Total> fixedIntervals;

    
    
    LiveInterval *fixedIntervalsUnion;

    
    StackSlotAllocator stackSlotAllocator;

    typedef Vector<LiveInterval *, 0, SystemAllocPolicy> SlotList;
    SlotList finishedSlots_;
    SlotList finishedDoubleSlots_;

    
    UnhandledQueue unhandled;
    InlineList<LiveInterval> active;
    InlineList<LiveInterval> inactive;
    InlineList<LiveInterval> fixed;
    InlineList<LiveInterval> handled;
    LiveInterval *current;

    bool createDataStructures();
    bool buildLivenessInfo();
    bool allocateRegisters();
    bool resolveControlFlow();
    bool reifyAllocations();
    bool populateSafepoints();

    
    void enqueueVirtualRegisterIntervals();

    uint32 allocateSlotFor(const LiveInterval *interval);
    bool splitInterval(LiveInterval *interval, CodePosition pos);
    bool assign(LAllocation allocation);
    bool spill();
    void freeAllocation(LiveInterval *interval, LAllocation *alloc);
    void finishInterval(LiveInterval *interval);
    AnyRegister::Code findBestFreeRegister(CodePosition *freeUntil);
    AnyRegister::Code findBestBlockedRegister(CodePosition *nextUsed);
    bool canCoexist(LiveInterval *a, LiveInterval *b);
    LMoveGroup *getMoveGroupBefore(CodePosition pos);
    LMoveGroup *getInputMoveGroup(CodePosition pos);
    LMoveGroup *getMoveGroupAfter(CodePosition pos);
    bool addMove(LMoveGroup *moves, LiveInterval *from, LiveInterval *to);
    bool moveBefore(CodePosition pos, LiveInterval *from, LiveInterval *to);
    bool moveInput(CodePosition pos, LiveInterval *from, LiveInterval *to);
    bool moveInputAlloc(CodePosition pos, LAllocation *from, LAllocation *to);
    bool moveAfter(CodePosition pos, LiveInterval *from, LiveInterval *to);
    void setIntervalRequirement(LiveInterval *interval);
    size_t findFirstSafepoint(LiveInterval *interval, size_t firstSafepoint);
    size_t findFirstNonCallSafepoint(CodePosition from);

    void addFixedRange(AnyRegister reg, CodePosition from, CodePosition to) {
        fixedIntervals[reg.code()]->addRange(from, to);
        fixedIntervalsUnion->addRange(from, to);
    }

#ifdef DEBUG
    void validateIntervals();
    void validateAllocations();
#else
    inline void validateIntervals() { };
    inline void validateAllocations() { };
#endif

    CodePosition outputOf(uint32 pos) {
        return CodePosition(pos, CodePosition::OUTPUT);
    }
    CodePosition outputOf(LInstruction *ins) {
        return CodePosition(ins->id(), CodePosition::OUTPUT);
    }
    CodePosition inputOf(uint32 pos) {
        return CodePosition(pos, CodePosition::INPUT);
    }
    CodePosition inputOf(LInstruction *ins) {
        return CodePosition(ins->id(), CodePosition::INPUT);
    }
#ifdef JS_NUNBOX32
    VirtualRegister *otherHalfOfNunbox(VirtualRegister *vreg);
#endif

  public:
    LinearScanAllocator(LIRGenerator *lir, LIRGraph &graph)
      : lir(lir),
        graph(graph)
    { }

    bool go();
};

} 
} 

#endif
