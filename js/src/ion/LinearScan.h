







































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

struct LOperand
{
  public:
    LUse *use;
    LInstruction *ins;
    bool snapshot;

    LOperand(LUse *use, LInstruction *ins, bool snapshot) :
        use(use),
        ins(ins),
        snapshot(snapshot)
    { }

};

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

    Kind kind() {
        return kind_;
    }

    LAllocation allocation() {
        JS_ASSERT(!allocation_.isUse());
        return allocation_;
    }

    uint32 virtualRegister() {
        JS_ASSERT(allocation_.isUse());
        return allocation_.toUse()->virtualRegister();
    }

    CodePosition pos() {
        return position_;
    }

    int priority();

private:
    Kind kind_;
    LAllocation allocation_;
    CodePosition position_;
};







class LiveInterval : public InlineListNode<LiveInterval>
{
  public:
    



    struct Range {
        Range(CodePosition f, CodePosition t)
          : from(f),
            to(t)
        { }
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

  public:

    LiveInterval(VirtualRegister *reg, uint32 index)
      : reg_(reg),
        index_(index)
    { }

    bool addRange(CodePosition from, CodePosition to);
    void setFrom(CodePosition from);
    CodePosition start();
    CodePosition end();
    CodePosition intersect(LiveInterval *other);
    bool covers(CodePosition pos);
    CodePosition nextCoveredAfter(CodePosition pos);
    size_t numRanges();
    Range *getRange(size_t i);

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
        requirement_ = requirement;
    }
    Requirement *hint() {
        return &hint_;
    }
    void setHint(const Requirement &hint) {
        hint_ = hint;
    }
    bool isSpilled() const {
        return alloc_.isMemory();
    }

    bool splitFrom(CodePosition pos, LiveInterval *after);
};






class VirtualRegister : public TempObject
{
    uint32 reg_;
    LBlock *block_;
    LInstruction *ins_;
    LDefinition *def_;
    Vector<LiveInterval *, 1, IonAllocPolicy> intervals_;
    Vector<LOperand, 0, IonAllocPolicy> uses_;
    LMoveGroup *inputMoves_;
    LMoveGroup *outputMoves_;
    LAllocation *canonicalSpill_;
    bool spillAtDefinition_;

  public:
    bool init(uint32 reg, LBlock *block, LInstruction *ins, LDefinition *def) {
        reg_ = reg;
        block_ = block;
        ins_ = ins;
        def_ = def;
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
    size_t numUses() {
        return uses_.length();
    }
    LOperand *getUse(size_t i) {
        return &uses_[i];
    }
    bool addUse(LOperand operand) {
        return uses_.append(operand);
    }
    void setInputMoves(LMoveGroup *moves) {
        inputMoves_ = moves;
    }
    LMoveGroup *inputMoves() {
        return inputMoves_;
    }
    void setOutputMoves(LMoveGroup *moves) {
        outputMoves_ = moves;
    }
    LMoveGroup *outputMoves() {
        return outputMoves_;
    }
    void setCanonicalSpill(LAllocation *alloc) {
        canonicalSpill_ = alloc;
    }
    LAllocation *canonicalSpill() {
        return canonicalSpill_;
    }
    bool isDouble() {
        return def_->type() == LDefinition::DOUBLE;
    }
    void setSpillAtDefinition() {
        spillAtDefinition_ = true;
    }
    bool mustSpillAtDefinition() const {
        return spillAtDefinition_;
    }

    LiveInterval *intervalFor(CodePosition pos);
    LOperand *nextUseAfter(CodePosition pos);
    CodePosition nextUsePosAfter(CodePosition pos);
    CodePosition nextIncompatibleUseAfter(CodePosition after, LAllocation alloc);
    LiveInterval *getFirstInterval();
};

class VirtualRegisterMap
{
  private:
    VirtualRegister *vregs_;
#ifdef DEBUG
    uint32 numVregs_;
#endif

  public:
    VirtualRegisterMap()
      : vregs_(NULL)
#ifdef DEBUG
      , numVregs_(0)
#endif
    { }

    bool init(MIRGenerator *gen, uint32 numVregs) {
        vregs_ = gen->allocate<VirtualRegister>(numVregs);
#ifdef DEBUG
        numVregs_ = numVregs;
#endif
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

class LinearScanAllocator
{
  private:
    friend class C1Spewer;
    friend class JSONSpewer;

    
    class UnhandledQueue : private InlineList<LiveInterval>
    {
      public:
        void enqueue(LiveInterval *interval);
        void enqueue(InlineList<LiveInterval> &list) {
            for (IntervalIterator i(list.begin()); i != list.end(); ) {
                LiveInterval *save = *i;
                i = list.removeAt(i);
                enqueue(save);
            }
        }

        LiveInterval *dequeue();
    };

    
    LIRGenerator *lir;
    LIRGraph &graph;

    
    BitSet **liveIn;
    VirtualRegisterMap vregs;

    
    StackSlotAllocator stackSlotAllocator;

    typedef Vector<LiveInterval *, 0, SystemAllocPolicy> SlotList;
    SlotList finishedSlots_;
    SlotList finishedDoubleSlots_;

    
    UnhandledQueue unhandled;
    InlineList<LiveInterval> active;
    InlineList<LiveInterval> inactive;
    InlineList<LiveInterval> handled;
    LiveInterval *current;
    LOperand *firstUse;
    CodePosition firstUsePos;

    bool createDataStructures();
    bool buildLivenessInfo();
    bool allocateRegisters();
    bool resolveControlFlow();
    bool reifyAllocations();

    uint32 allocateSlotFor(const LiveInterval *interval);
    bool splitInterval(LiveInterval *interval, CodePosition pos);
    bool assign(LAllocation allocation);
    bool spill();
    void finishInterval(LiveInterval *interval);
    AnyRegister::Code findBestFreeRegister(CodePosition *freeUntil);
    AnyRegister::Code findBestBlockedRegister(CodePosition *nextUsed);
    bool canCoexist(LiveInterval *a, LiveInterval *b);
    LMoveGroup *getMoveGroupBefore(CodePosition pos);
    LMoveGroup *getOutputSpillMoveGroup(VirtualRegister *vreg);
    bool moveBefore(CodePosition pos, LiveInterval *from, LiveInterval *to);
    void setIntervalRequirement(LiveInterval *interval);
    void addSpillInterval(LInstruction *ins, const Requirement &req);

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

  public:
    LinearScanAllocator(LIRGenerator *lir, LIRGraph &graph)
      : lir(lir),
        graph(graph),
        firstUsePos(CodePosition::MAX)
    { }

    bool go();

};

}
}

#endif
