








































#ifndef jsion_mirgraph_h__
#define jsion_mirgraph_h__

#include "MIR.h"

namespace js {
namespace ion {

class MIRGraph
{
    Vector<MBasicBlock *, 8, TempAllocPolicy> blocks_;
    uint32 idGen_;

  public:
    MIRGraph(JSContext *cx);

    bool addBlock(MBasicBlock *block);

    size_t numBlocks() const {
        return blocks_.length();
    }
    MBasicBlock *getBlock(size_t i) const {
        return blocks_[i];
    }
    uint32 allocInstructionId() {
        idGen_ += 2;
        return idGen_;
    }
};

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
    void assertUsesAreNotWithin(MOperand *use);

    
    void setSlot(uint32 slot, MInstruction *ins);

    
    void pushCopy(uint32 slot);

    
    void pushVariable(uint32 slot);

    
    
    bool setVariable(uint32 slot);

    
    
    
    MInstruction *getSlot(uint32 index);

  public:
    
    
    

    
    
    static MBasicBlock *New(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);
    static MBasicBlock *NewLoopHeader(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);

    
    
    
    bool initHeader();

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

    
    
    bool add(MInstruction *ins);

    
    
    bool end(MControlInstruction *ins);

    
    bool addPhi(MPhi *phi);

    
    
    
    bool addPredecessor(MBasicBlock *pred);

    
    
    
    bool setBackedge(MBasicBlock *block, MBasicBlock *successor);

    
    
    

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
    size_t numEntrySlots() const {
        return headerSlots_;
    }
    MInstruction *getEntrySlot(size_t i) const {
        JS_ASSERT(i < numEntrySlots());
        return header_[i];
    }
    size_t numInstructions() const {
        return instructions_.length();
    }
    MInstruction *getInstruction(size_t i) const {
        return instructions_[i];
    }
    size_t numPhis() const {
        return phis_.length();
    }
    MPhi *getPhi(size_t i) const {
        return phis_[i];
    }

  private:
    MIRGenerator *gen;
    Vector<MInstruction *, 4, TempAllocPolicy> instructions_;
    Vector<MBasicBlock *, 1, TempAllocPolicy> predecessors_;
    Vector<MPhi *, 2, TempAllocPolicy> phis_;
    StackSlot *slots_;
    uint32 stackPosition_;
    MControlInstruction *lastIns_;
    jsbytecode *pc_;
    uint32 id_;

    
    
    
    MInstruction **header_;
    uint32 headerSlots_;
};

} 
} 

#endif 

