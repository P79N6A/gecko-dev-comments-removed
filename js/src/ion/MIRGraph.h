








































#ifndef jsion_mirgraph_h__
#define jsion_mirgraph_h__

#include "MIR.h"

namespace js {
namespace ion {

typedef HashMap<uint32,
                MInstruction *,
                DefaultHasher<uint32>,
                IonAllocPolicy> InstructionMap;

class MIRGraph
{
    Vector<MBasicBlock *, 8, IonAllocPolicy> blocks_;
    uint32 idGen_;

  public:
    MIRGraph()
      : idGen_(0)
    {  }

    bool addBlock(MBasicBlock *block);

    void reset();

    size_t numBlocks() const {
        return blocks_.length();
    }
    MBasicBlock *getBlock(size_t i) const {
        return blocks_[i];
    }
    void allocInstructionId(MInstruction *ins) {
        idGen_ += 2;
        ins->setId(idGen_);
    }
    uint32 getMaxInstructionId() {
        return idGen_;
    }
};

typedef InlineList<MInstruction>::iterator MInstructionIterator;

class MBasicBlock : public TempObject
{
    static const uint32 NotACopy = uint32(-1);

    struct StackSlot {
        MInstruction *ins;
        uint32 copyOf;
        union {
            uint32 firstCopy; 
            uint32 nextCopy;  
        };

        void set(MInstruction *ins) {
            this->ins = ins;
            copyOf = NotACopy;
            firstCopy = NotACopy;
        }
        bool isCopy() const {
            return copyOf != NotACopy;
        }
        bool isCopied() const {
            if (isCopy())
                return false;
            return firstCopy != NotACopy;
        }
    };

  private:
    MBasicBlock(MIRGenerator *gen, jsbytecode *pc);
    bool init();
    void copySlots(MBasicBlock *from);
    bool inherit(MBasicBlock *pred);
    void assertUsesAreNotWithin(MUse *use);

    
    void setSlot(uint32 slot, MInstruction *ins);

    
    void pushCopy(uint32 slot);

    
    void pushVariable(uint32 slot);

    
    
    bool setVariable(uint32 slot);

    bool hasHeader() const {
        return instructions_.begin() != instructions_.end();
    }

  public:
    
    
    

    
    
    static MBasicBlock *New(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);
    static MBasicBlock *NewLoopHeader(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);

    void setId(uint32 id) {
        id_ = id;
    }

    
    MInstruction *peek(int32 depth);

    
    
    void initSlot(uint32 index, MInstruction *ins);

    
    
    bool setLocal(uint32 local);
    bool setArg(uint32 arg);

    
    void push(MInstruction *ins);
    void pushArg(uint32 arg);
    void pushLocal(uint32 local);

    
    MInstruction *pop();

    
    
    void add(MInstruction *ins);

    
    
    void end(MControlInstruction *ins);

    
    bool addPhi(MPhi *phi);

    
    
    
    bool addPredecessor(MBasicBlock *pred);

    
    
    
    bool setBackedge(MBasicBlock *block, MBasicBlock *successor);

    void insertBefore(MInstruction *at, MInstruction *ins);
    void insertAfter(MInstruction *at, MInstruction *ins);
    void remove(MInstruction *ins);
    MInstructionIterator removeAt(MInstructionIterator &iter);

    
    
    

    jsbytecode *pc() const {
        return pc_;
    }
    uint32 id() const {
        return id_;
    }
    uint32 numPredecessors() const {
        return predecessors_.length();
    }
    MBasicBlock *getPredecessor(uint32 i) const {
        return predecessors_[i];
    }
    MControlInstruction *lastIns() const {
        return lastIns_;
    }
    size_t numPhis() const {
        return phis_.length();
    }
    MPhi *getPhi(size_t i) const {
        return phis_[i];
    }
    MInstructionIterator begin() {
        return instructions_.begin();
    }
    MInstructionIterator end() {
        return instructions_.end();
    }
    bool isLoopHeader() const {
        return loopSuccessor_ != NULL;
    }
    MBasicBlock *getLoopSuccessor() const {
        JS_ASSERT(isLoopHeader());
        return loopSuccessor_;
    }
    MIRGenerator *gen() {
        return gen_;
    }
    uint32 stackDepth() const {
        return stackPosition_;
    }
    bool isMarked() const {
        return mark_;
    }
    void mark() {
        mark_ = true;
    }
    void unmark() {
        mark_ = false;
    }

    
    
    
    MInstruction *getSlot(uint32 index);

    MSnapshot *entrySnapshot() const {
        return instructions_.begin()->toSnapshot();
    }
    size_t numEntrySlots() const {
        return entrySnapshot()->numOperands();
    }
    MInstruction *getEntrySlot(size_t i) const {
        JS_ASSERT(i < numEntrySlots());
        return entrySnapshot()->getInput(i);
    }

  private:
    MIRGenerator *gen_;
    InlineList<MInstruction> instructions_;
    Vector<MBasicBlock *, 1, IonAllocPolicy> predecessors_;
    Vector<MPhi *, 2, IonAllocPolicy> phis_;
    StackSlot *slots_;
    uint32 stackPosition_;
    MControlInstruction *lastIns_;
    jsbytecode *pc_;
    uint32 id_;

    
    
    MBasicBlock *loopSuccessor_;

    
    bool mark_;
};

} 
} 

#endif 

