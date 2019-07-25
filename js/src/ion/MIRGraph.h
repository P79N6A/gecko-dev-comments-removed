








































#ifndef jsion_mirgraph_h__
#define jsion_mirgraph_h__




#include "IonAllocPolicy.h"
#include "MIRGenerator.h"

namespace js {
namespace ion {

class MBasicBlock;
class MIRGraph;
class MStart;

class MDefinitionIterator;

class MIRGraph
{
    Vector<MBasicBlock *, 8, IonAllocPolicy> blocks_;
    uint32 idGen_;

  public:
    MIRGraph()
      : idGen_(0)
    {  }

    bool addBlock(MBasicBlock *block);
    void unmarkBlocks();

    void clearBlockList() {
        blocks_.clear();
    }
    void resetInstructionNumber() {
        idGen_ = 0;
    }

    size_t numBlocks() const {
        return blocks_.length();
    }
    MBasicBlock *getBlock(size_t i) const {
        return blocks_[i];
    }
    void allocDefinitionId(MDefinition *ins) {
        
        
        idGen_ += 2;
        ins->setId(idGen_);
    }
    uint32 getMaxInstructionId() {
        return idGen_;
    }
};

typedef InlineList<MInstruction>::iterator MInstructionIterator;

class LBlock;

class MBasicBlock : public TempObject
{
    static const uint32 NotACopy = uint32(-1);

    struct StackSlot {
        MDefinition *def;
        uint32 copyOf;
        union {
            uint32 firstCopy; 
            uint32 nextCopy;  
        };

        void set(MDefinition *def) {
            this->def = def;
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

  public:
    enum Kind {
        NORMAL,
        PENDING_LOOP_HEADER,
        LOOP_HEADER,
        SPLIT_EDGE
    };

  private:
    MBasicBlock(MIRGenerator *gen, jsbytecode *pc, Kind kind);
    bool init();
    void copySlots(MBasicBlock *from);
    bool inherit(MBasicBlock *pred);
    void assertUsesAreNotWithin(MUseIterator use, MUseIterator end);

    
    void setSlot(uint32 slot, MDefinition *ins);

    
    void pushCopy(uint32 slot);

    
    void pushVariable(uint32 slot);

    
    
    bool setVariable(uint32 slot);

  public:
    
    
    

    
    
    static MBasicBlock *New(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc, Kind kind);
    static MBasicBlock *NewPendingLoopHeader(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);
    static MBasicBlock *NewSplitEdge(MIRGenerator *gen, MBasicBlock *pred);

    void setId(uint32 id) {
        id_ = id;
    }

    
    MDefinition *peek(int32 depth);

    
    
    void initSlot(uint32 index, MDefinition *ins);

    
    
    bool setLocal(uint32 local);
    bool setArg(uint32 arg);

    
    void push(MDefinition *ins);
    void pushArg(uint32 arg);
    void pushLocal(uint32 local);

    
    MDefinition *pop();

    
    
    void add(MInstruction *ins);

    
    
    void end(MControlInstruction *ins);

    
    bool addPhi(MPhi *phi);

    
    
    
    bool addPredecessor(MBasicBlock *pred);

    
    
    void replacePredecessor(MBasicBlock *old, MBasicBlock *split);
    void replaceSuccessor(size_t pos, MBasicBlock *split);

    
    
    
    bool setBackedge(MBasicBlock *block, MBasicBlock *successor);

    void insertBefore(MInstruction *at, MInstruction *ins);
    void insertAfter(MInstruction *at, MInstruction *ins);
    void remove(MInstruction *ins);
    MInstructionIterator removeAt(MInstructionIterator &iter);

    MDefinitionIterator removeDefAt(MDefinitionIterator &iter);
    
    
    

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
        return kind_ == LOOP_HEADER;
    }
    MBasicBlock *backedge() const {
        JS_ASSERT(isLoopHeader());
        JS_ASSERT(numPredecessors() == 1 || numPredecessors() == 2);
        return getPredecessor(numPredecessors() - 1);
    }
    bool isLoopBackedge() const {
        if (!numSuccessors())
            return false;
        MBasicBlock *lastSuccessor = getSuccessor(numSuccessors() - 1);
        return lastSuccessor->isLoopHeader() && lastSuccessor->backedge() == this;
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
    void makeStart(MStart *start) {
        add(start);
        start_ = start;
    }
    MStart *start() const {
        return start_;
    }

    MBasicBlock *immediateDominator() const {
        return immediateDominator_;
    }

    void setImmediateDominator(MBasicBlock *dom) {
        immediateDominator_ = dom;
    }

    size_t numImmediatelyDominatedBlocks() const {
        return immediatelyDominated_.length();
    }

    MBasicBlock *getImmediatelyDominatedBlock(size_t i) const {
        return immediatelyDominated_[i];
    }

    size_t numDominated() const {
        return numDominated_;
    }

    void addNumDominated(size_t n) {
        numDominated_ += n;
    }

    bool addImmediatelyDominatedBlock(MBasicBlock *child);

    
    
    
    MDefinition *getSlot(uint32 index);

    MSnapshot *entrySnapshot() const {
        return entrySnapshot_;
    }
    size_t numEntrySlots() const {
        return entrySnapshot()->numOperands();
    }
    MDefinition *getEntrySlot(size_t i) const {
        JS_ASSERT(i < numEntrySlots());
        return entrySnapshot()->getOperand(i);
    }

    LBlock *lir() const {
        return lir_;
    }
    void assignLir(LBlock *lir) {
        JS_ASSERT(!lir_);
        lir_ = lir;
    }

    MBasicBlock *successorWithPhis() const {
        return successorWithPhis_;
    }
    uint32 positionInPhiSuccessor() const {
        return positionInPhiSuccessor_;
    }
    void setSuccessorWithPhis(MBasicBlock *successor, uint32 id) {
        successorWithPhis_ = successor;
        positionInPhiSuccessor_ = id;
    }
    size_t numSuccessors() const;
    MBasicBlock *getSuccessor(size_t index) const;

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
    LBlock *lir_;
    MStart *start_;
    MSnapshot *entrySnapshot_;
    MBasicBlock *successorWithPhis_;
    uint32 positionInPhiSuccessor_;
    Kind kind_;

    
    bool mark_;

    Vector<MBasicBlock *, 1, IonAllocPolicy> immediatelyDominated_;
    MBasicBlock *immediateDominator_;
    size_t numDominated_;
};

class MDefinitionIterator
{

  friend class MBasicBlock;

  private:
    MBasicBlock *block_;
    size_t phiIndex_;
    MInstructionIterator iter_;


    MDefinition *getIns() {
        if (phiIndex_ < block_->numPhis())
            return block_->getPhi(phiIndex_);

        return *iter_;
    }

    MDefinitionIterator(const MDefinitionIterator *old)
      : block_(old->block_),
        phiIndex_(old->phiIndex_),
        iter_(old->iter_)
    { }

    void next() {
        if (phiIndex_ < block_->numPhis())
            phiIndex_++;
        else
            iter_++;
    }

    bool more() const {
        return phiIndex_ < block_->numPhis() || (*iter_) != block_->lastIns();
    }

  public:
    MDefinitionIterator(MBasicBlock *block)
      : block_(block),
        phiIndex_(0),
        iter_(block->begin())
    { }

    MDefinitionIterator operator ++(int) {
        MDefinitionIterator old(this);
        if (more())
            next();
        return old;
    }

    operator bool() const {
        return more();
    }

    MDefinition *operator *() {
        return getIns();
    }

    MDefinition *operator ->() {
        return getIns();
    }

};

} 
} 

#endif 

