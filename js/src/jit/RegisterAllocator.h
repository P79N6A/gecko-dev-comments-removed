





#ifndef jit_RegisterAllocator_h
#define jit_RegisterAllocator_h

#include "mozilla/Attributes.h"
#include "mozilla/MathAlgorithms.h"

#include "jit/LIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"



namespace js {
namespace jit {

class LIRGenerator;









struct AllocationIntegrityState
{
    explicit AllocationIntegrityState(const LIRGraph &graph)
      : graph(graph)
    {}

    
    
    bool record();

    
    
    
    
    bool check(bool populateSafepoints);

  private:

    const LIRGraph &graph;

    
    
    
    
    

    struct InstructionInfo {
        Vector<LAllocation, 2, SystemAllocPolicy> inputs;
        Vector<LDefinition, 0, SystemAllocPolicy> temps;
        Vector<LDefinition, 1, SystemAllocPolicy> outputs;

        InstructionInfo()
        { }

        InstructionInfo(const InstructionInfo &o)
        {
            inputs.appendAll(o.inputs);
            temps.appendAll(o.temps);
            outputs.appendAll(o.outputs);
        }
    };
    Vector<InstructionInfo, 0, SystemAllocPolicy> instructions;

    struct BlockInfo {
        Vector<InstructionInfo, 5, SystemAllocPolicy> phis;
        BlockInfo() {}
        BlockInfo(const BlockInfo &o) {
            phis.appendAll(o.phis);
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
            hash = mozilla::RotateLeft(hash, 4) ^ item.vreg;
            hash = mozilla::RotateLeft(hash, 4) ^ HashNumber(item.block->mir()->id());
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
    MOZ_CONSTEXPR explicit CodePosition(uint32_t bits)
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

    MOZ_CONSTEXPR CodePosition() : bits_(0)
    { }

    CodePosition(uint32_t instruction, SubPosition where) {
        JS_ASSERT(instruction < 0x80000000u);
        JS_ASSERT(((uint32_t)where & SUBPOSITION_MASK) == (uint32_t)where);
        bits_ = (instruction << INSTRUCTION_SHIFT) | (uint32_t)where;
    }

    uint32_t ins() const {
        return bits_ >> INSTRUCTION_SHIFT;
    }

    uint32_t bits() const {
        return bits_;
    }

    SubPosition subpos() const {
        return (SubPosition)(bits_ & SUBPOSITION_MASK);
    }

    bool operator <(CodePosition other) const {
        return bits_ < other.bits_;
    }

    bool operator <=(CodePosition other) const {
        return bits_ <= other.bits_;
    }

    bool operator !=(CodePosition other) const {
        return bits_ != other.bits_;
    }

    bool operator ==(CodePosition other) const {
        return bits_ == other.bits_;
    }

    bool operator >(CodePosition other) const {
        return bits_ > other.bits_;
    }

    bool operator >=(CodePosition other) const {
        return bits_ >= other.bits_;
    }

    uint32_t operator -(CodePosition other) const {
        JS_ASSERT(bits_ >= other.bits_);
        return bits_ - other.bits_;
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
    FixedList<InstructionData> insData_;

  public:
    InstructionDataMap()
      : insData_()
    { }

    bool init(MIRGenerator *gen, uint32_t numInstructions) {
        if (!insData_.init(gen->alloc(), numInstructions))
            return false;
        memset(&insData_[0], 0, sizeof(InstructionData) * numInstructions);
        return true;
    }

    InstructionData &operator[](CodePosition pos) {
        return operator[](pos.ins());
    }
    const InstructionData &operator[](CodePosition pos) const {
        return operator[](pos.ins());
    }
    InstructionData &operator[](LInstruction *ins) {
        return operator[](ins->id());
    }
    const InstructionData &operator[](LInstruction *ins) const {
        return operator[](ins->id());
    }
    InstructionData &operator[](uint32_t ins) {
        return insData_[ins];
    }
    const InstructionData &operator[](uint32_t ins) const {
        return insData_[ins];
    }
};


class RegisterAllocator
{
    void operator=(const RegisterAllocator &) MOZ_DELETE;
    RegisterAllocator(const RegisterAllocator &) MOZ_DELETE;

  protected:
    
    MIRGenerator *mir;
    LIRGenerator *lir;
    LIRGraph &graph;

    
    RegisterSet allRegisters_;

    
    InstructionDataMap insData;

    RegisterAllocator(MIRGenerator *mir, LIRGenerator *lir, LIRGraph &graph)
      : mir(mir),
        lir(lir),
        graph(graph),
        allRegisters_(RegisterSet::All())
    {
        if (mir->compilingAsmJS()) {
#if defined(JS_CODEGEN_X64)
            allRegisters_.take(AnyRegister(HeapReg));
#elif defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
            allRegisters_.take(AnyRegister(HeapReg));
            allRegisters_.take(AnyRegister(GlobalReg));
#endif
        } else {
            if (FramePointer != InvalidReg && mir->instrumentedProfiling())
                allRegisters_.take(AnyRegister(FramePointer));
        }
    }

    bool init();

    TempAllocator &alloc() const {
        return mir->alloc();
    }

    CodePosition outputOf(uint32_t pos) const {
        
        
        
        if (insData[pos].ins()->isPhi()) {
            while (insData[pos + 1].ins()->isPhi())
                ++pos;
        }
        return CodePosition(pos, CodePosition::OUTPUT);
    }
    CodePosition outputOf(const LInstruction *ins) const {
        return outputOf(ins->id());
    }
    CodePosition inputOf(uint32_t pos) const {
        
        
        
        if (insData[pos].ins()->isPhi()) {
            while (pos > 0 && insData[pos - 1].ins()->isPhi())
                --pos;
        }
        return CodePosition(pos, CodePosition::INPUT);
    }
    CodePosition inputOf(const LInstruction *ins) const {
        return inputOf(ins->id());
    }
    CodePosition entryOf(const LBlock *block) {
        return inputOf(block->firstId());
    }
    CodePosition exitOf(const LBlock *block) {
        return outputOf(block->lastId());
    }

    LMoveGroup *getInputMoveGroup(uint32_t ins);
    LMoveGroup *getMoveGroupAfter(uint32_t ins);

    LMoveGroup *getInputMoveGroup(CodePosition pos) {
        return getInputMoveGroup(pos.ins());
    }
    LMoveGroup *getMoveGroupAfter(CodePosition pos) {
        return getMoveGroupAfter(pos.ins());
    }

    CodePosition minimalDefEnd(LInstruction *ins) {
        
        
        
        
        while (true) {
            LInstruction *next = insData[outputOf(ins).next()].ins();
            if (!next->isNop() && !next->isOsiPoint())
                break;
            ins = next;
        }

        return outputOf(ins);
    }

    void dumpInstructions();
};

static inline AnyRegister
GetFixedRegister(const LDefinition *def, const LUse *use)
{
    return def->isFloatReg()
           ? AnyRegister(FloatRegister::FromCode(use->registerCode()))
           : AnyRegister(Register::FromCode(use->registerCode()));
}

} 
} 

#endif 
