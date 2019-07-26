






#ifndef js_ion_registerallocator_h__
#define js_ion_registerallocator_h__

#include "Ion.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "InlineList.h"
#include "LIR.h"
#include "Lowering.h"



namespace js {
namespace ion {









struct AllocationIntegrityState
{
    AllocationIntegrityState(LIRGraph &graph)
      : graph(graph)
    {}

    
    
    bool record();

    
    
    
    
    bool check(bool populateSafepoints);

  private:

    LIRGraph &graph;

    
    
    
    
    

    struct InstructionInfo {
        Vector<LAllocation, 2, SystemAllocPolicy> inputs;
        Vector<LDefinition, 0, SystemAllocPolicy> temps;
        Vector<LDefinition, 1, SystemAllocPolicy> outputs;

        InstructionInfo()
        { }

        InstructionInfo(const InstructionInfo &o)
        {
            for (size_t i = 0; i < o.inputs.length(); i++)
                inputs.append(o.inputs[i]);
            for (size_t i = 0; i < o.temps.length(); i++)
                temps.append(o.temps[i]);
            for (size_t i = 0; i < o.outputs.length(); i++)
                outputs.append(o.outputs[i]);
        }
    };
    Vector<InstructionInfo, 0, SystemAllocPolicy> instructions;

    struct BlockInfo {
        Vector<InstructionInfo, 5, SystemAllocPolicy> phis;
        BlockInfo() {}
        BlockInfo(const BlockInfo &o) {
            for (size_t i = 0; i < o.phis.length(); i++)
                phis.append(o.phis[i]);
        }
    };
    Vector<BlockInfo, 0, SystemAllocPolicy> blocks;

    Vector<LDefinition*, 20, SystemAllocPolicy> virtualRegisters;

    
    
    
    struct IntegrityItem
    {
        LBlock *block;
        uint32_t vreg;
        LAllocation alloc;

        
        uint32_t index;

        typedef IntegrityItem Lookup;
        static HashNumber hash(const IntegrityItem &item) {
            HashNumber hash = item.alloc.hash();
            hash = JS_ROTATE_LEFT32(hash, 4) ^ item.vreg;
            hash = JS_ROTATE_LEFT32(hash, 4) ^ HashNumber(item.block->mir()->id());
            return hash;
        }
        static bool match(const IntegrityItem &one, const IntegrityItem &two) {
            return one.block == two.block
                && one.vreg == two.vreg
                && one.alloc == two.alloc;
        }
    };

    
    Vector<IntegrityItem, 10, SystemAllocPolicy> worklist;

    
    typedef HashSet<IntegrityItem, IntegrityItem, SystemAllocPolicy> IntegrityItemSet;
    IntegrityItemSet seen;

    bool checkIntegrity(LBlock *block, LInstruction *ins, uint32_t vreg, LAllocation alloc,
                        bool populateSafepoints);
    bool checkSafepointAllocation(LInstruction *ins, uint32_t vreg, LAllocation alloc,
                                  bool populateSafepoints);
    bool addPredecessor(LBlock *block, uint32_t vreg, LAllocation alloc);

    void dump();
};











class CodePosition
{
  private:
    CodePosition(const uint32_t &bits)
      : bits_(bits)
    { }

    static const unsigned int INSTRUCTION_SHIFT = 1;
    static const unsigned int SUBPOSITION_MASK = 1;
    uint32_t bits_;

  public:
    static const CodePosition MAX;
    static const CodePosition MIN;

    
    
    enum SubPosition {
        INPUT,
        OUTPUT
    };

    CodePosition() : bits_(0)
    { }

    CodePosition(uint32_t instruction, SubPosition where) {
        JS_ASSERT(instruction < 0x80000000u);
        JS_ASSERT(((uint32_t)where & SUBPOSITION_MASK) == (uint32_t)where);
        bits_ = (instruction << INSTRUCTION_SHIFT) | (uint32_t)where;
    }

    uint32_t ins() const {
        return bits_ >> INSTRUCTION_SHIFT;
    }

    uint32_t pos() const {
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


class InstructionData
{
    LInstruction *ins_;
    LBlock *block_;
    LMoveGroup *inputMoves_;
    LMoveGroup *movesAfter_;

  public:
    void init(LInstruction *ins, LBlock *block) {
        JS_ASSERT(!ins_);
        JS_ASSERT(!block_);
        ins_ = ins;
        block_ = block;
    }
    LInstruction *ins() const {
        return ins_;
    }
    LBlock *block() const {
        return block_;
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
};


class InstructionDataMap
{
    InstructionData *insData_;
    uint32_t numIns_;

  public:
    InstructionDataMap()
      : insData_(NULL),
        numIns_(0)
    { }

    bool init(MIRGenerator *gen, uint32_t numInstructions) {
        insData_ = gen->allocate<InstructionData>(numInstructions);
        numIns_ = numInstructions;
        if (!insData_)
            return false;
        memset(insData_, 0, sizeof(InstructionData) * numInstructions);
        return true;
    }

    InstructionData &operator[](const CodePosition &pos) {
        JS_ASSERT(pos.ins() < numIns_);
        return insData_[pos.ins()];
    }
    InstructionData &operator[](LInstruction *ins) {
        JS_ASSERT(ins->id() < numIns_);
        return insData_[ins->id()];
    }
    InstructionData &operator[](uint32_t ins) {
        JS_ASSERT(ins < numIns_);
        return insData_[ins];
    }
};


class RegisterAllocator
{
  protected:
    
    MIRGenerator *mir;
    LIRGenerator *lir;
    LIRGraph &graph;

    
    RegisterSet allRegisters_;

    
    InstructionDataMap insData;

  public:
    RegisterAllocator(MIRGenerator *mir, LIRGenerator *lir, LIRGraph &graph)
      : mir(mir),
        lir(lir),
        graph(graph),
        allRegisters_(RegisterSet::All())
    {
        if (FramePointer != InvalidReg && lir->mir()->instrumentedProfiling())
            allRegisters_.take(AnyRegister(FramePointer));
#ifdef JS_CPU_X64
        if (mir->compilingAsmJS())
            allRegisters_.take(AnyRegister(HeapReg));
#endif
    }

  protected:
    bool init();

    CodePosition outputOf(uint32_t pos) {
        return CodePosition(pos, CodePosition::OUTPUT);
    }
    CodePosition outputOf(LInstruction *ins) {
        return CodePosition(ins->id(), CodePosition::OUTPUT);
    }
    CodePosition inputOf(uint32_t pos) {
        return CodePosition(pos, CodePosition::INPUT);
    }
    CodePosition inputOf(LInstruction *ins) {
        return CodePosition(ins->id(), CodePosition::INPUT);
    }

    LMoveGroup *getInputMoveGroup(uint32_t ins);
    LMoveGroup *getMoveGroupAfter(uint32_t ins);

    LMoveGroup *getInputMoveGroup(CodePosition pos) {
        return getInputMoveGroup(pos.ins());
    }
    LMoveGroup *getMoveGroupAfter(CodePosition pos) {
        return getMoveGroupAfter(pos.ins());
    }

    size_t findFirstNonCallSafepoint(CodePosition from)
    {
        size_t i = 0;
        for (; i < graph.numNonCallSafepoints(); i++) {
            LInstruction *ins = graph.getNonCallSafepoint(i);
            if (from <= inputOf(ins))
                break;
        }
        return i;
    }
};

} 
} 

#endif
