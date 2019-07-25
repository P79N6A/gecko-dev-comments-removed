







































#ifndef js_ion_registerallocator_h__
#define js_ion_registerallocator_h__

#include "Ion.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "InlineList.h"
#include "IonLIR.h"
#include "IonLowering.h"

#include "jsvector.h"

namespace js {
namespace ion {

struct LOperand
{
public:
    LUse *use;
    LInstruction *ins;

    LOperand(LUse *use, LInstruction *ins) :
        use(use),
        ins(ins)
    { }

};

class VirtualRegister;

class CodePosition
{
private:
    CodePosition(const uint32 &bits) :
      bits_(bits)
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

class LiveInterval : public InlineListNode<LiveInterval>
{
public:
    struct Range {
        Range(CodePosition f, CodePosition t) :
          from(f),
          to(t)
        { }
        CodePosition from;
        CodePosition to;
    };

    enum Flag {
        FIXED
    };

private:
    Vector<Range, 1, IonAllocPolicy> ranges_;
    LAllocation alloc_;
    VirtualRegister *reg_;
    uint32 flags_;

public:

    LiveInterval(VirtualRegister *reg)
      : reg_(reg),
        flags_(0)
    { }

    static LiveInterval *New(VirtualRegister *reg);

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

    VirtualRegister *reg() {
        return reg_;
    }

    bool hasFlag(Flag flag) const {
        return !!(flags_ & (1 << (uint32)flag));
    }

    void setFlag(Flag flag) {
        flags_ |= (1 << (uint32)flag);
    }

    bool splitFrom(CodePosition pos, LiveInterval *after);
};

class VirtualRegister : public TempObject
{
private:
    uint32 reg_;
    LBlock *block_;
    LInstruction *ins_;
    LDefinition *def_;
    Vector<LiveInterval *, 1, IonAllocPolicy> intervals_;
    Vector<LOperand, 0, IonAllocPolicy> uses_;

public:
    VirtualRegister() :
      reg_(0),
      block_(NULL),
      ins_(NULL),
      intervals_()
    { }

    bool init(uint32 reg, LBlock *block, LInstruction *ins, LDefinition *def) {
        reg_ = reg;
        block_ = block;
        ins_ = ins;
        def_ = def;
        LiveInterval *initial = LiveInterval::New(this);
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

    LDefinition *def() {
        return def_;
    }

    size_t numIntervals() {
        return intervals_.length();
    }

    LiveInterval *getInterval(size_t i) {
        return intervals_[i];
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

    LiveInterval *intervalFor(CodePosition pos);

    CodePosition nextUseAfter(CodePosition pos);

    CodePosition nextIncompatibleUseAfter(CodePosition after, LAllocation alloc);

    LiveInterval *splitIntervalAfter(VirtualRegister *ins, CodePosition pos);

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

class RegisterAllocator
{
private:
    friend class C1Spewer;
    friend class JSONSpewer;

    
    class UnhandledQueue : private InlineList<LiveInterval>
    {
      public:
        void enqueue(LiveInterval *interval);

        LiveInterval *dequeue();
    };

    
    LIRGenerator *lir;
    LIRGraph &graph;

    
    StackAssignment stackAssignment;

    RegisterSet allowedRegs;
    VirtualRegister *vregs;
    UnhandledQueue unhandled;
    InlineList<LiveInterval> active;
    InlineList<LiveInterval> inactive;
    InlineList<LiveInterval> handled;
    CodePosition *freeUntilPos;
    CodePosition *nextUsePos;
    LiveInterval *current;

    bool createDataStructures();
    bool buildLivenessInfo();
    bool allocateRegisters();
    bool resolveControlFlow();
    bool reifyAllocations();

    bool splitInterval(LiveInterval *interval, CodePosition pos);
    bool assign(LAllocation allocation);
    bool spill();
    void finishInterval(LiveInterval *interval);
    Register findBestFreeRegister();
    Register findBestBlockedRegister();
    bool canCoexist(LiveInterval *a, LiveInterval *b);

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
    RegisterAllocator(LIRGenerator *lir, LIRGraph &graph)
      : lir(lir),
        graph(graph)
    { }

    bool go();

};

}
}

#endif
