








































#ifndef jsion_mirgraph_h__
#define jsion_mirgraph_h__




#include "IonAllocPolicy.h"

namespace js {
namespace ion {

class MBasicBlock;
class MIRGraph;
class MStart;

class MIRGenerator
{
  public:
    MIRGenerator(TempAllocator &temp, JSScript *script, JSFunction *fun, MIRGraph &graph);

    TempAllocator &temp() {
        return temp_;
    }
    JSFunction *fun() const {
        return fun_;
    }
    uint32 nslots() const {
        return nslots_;
    }
    uint32 nargs() const {
        return fun()->nargs;
    }
    uint32 nlocals() const {
        return script->nfixed;
    }
    uint32 calleeSlot() const {
        JS_ASSERT(fun());
        return 0;
    }
    uint32 thisSlot() const {
        JS_ASSERT(fun());
        return 1;
    }
    uint32 firstArgSlot() const {
        JS_ASSERT(fun());
        return 2;
    }
    uint32 argSlot(uint32 i) const {
        return firstArgSlot() + i;
    }
    uint32 firstLocalSlot() const {
        return (fun() ? fun()->nargs + 2 : 0);
    }
    uint32 localSlot(uint32 i) const {
        return firstLocalSlot() + i;
    }
    uint32 firstStackSlot() const {
        return firstLocalSlot() + nlocals();
    }
    uint32 stackSlot(uint32 i) const {
        return firstStackSlot() + i;
    }
    MIRGraph &graph() {
        return graph_;
    }
    bool ensureBallast() {
        return temp().ensureBallast();
    }

    template <typename T>
    T * allocate(size_t count = 1)
    {
        return reinterpret_cast<T *>(temp().allocate(sizeof(T) * count));
    }

    
    
    bool abort(const char *message, ...);

    bool errored() const {
        return error_;
    }

  public:
    JSScript *script;

  protected:
    jsbytecode *pc;
    TempAllocator &temp_;
    JSFunction *fun_;
    uint32 nslots_;
    MIRGraph &graph_;
    bool error_;
};

class MIRGraph
{
    Vector<MBasicBlock *, 8, IonAllocPolicy> blocks_;
    uint32 idGen_;

  public:
    MIRGraph()
      : idGen_(0)
    {  }

    bool addBlock(MBasicBlock *block);

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
    void allocInstructionId(MInstruction *ins) {
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
    MBasicBlock *successorWithPhis_;
    uint32 positionInPhiSuccessor_;

    
    
    MBasicBlock *loopSuccessor_;

    
    bool mark_;

    Vector<MBasicBlock *, 1, IonAllocPolicy> immediatelyDominated_;
    MBasicBlock *immediateDominator_;
    size_t numDominated_;
};

class MDefinitionIterator
{
  private:
    MBasicBlock *block_;
    size_t phiIndex_;
    MInstructionIterator iter_;


    MInstruction *getIns() {
        if (phiIndex_ < block_->numPhis()) 
            return block_->getPhi(phiIndex_);

        return *iter_;
    }

  public:
    MDefinitionIterator(MBasicBlock *block)
      : block_(block), 
        phiIndex_(0), 
        iter_(block->begin())
    { }

    void next() {
        if (phiIndex_ < block_->numPhis())
            phiIndex_++;
        else
            iter_++;
    }

    MInstruction *operator *() {
        return getIns();
    }

    MInstruction *operator ->() {
        return getIns();
    }

    bool more() {
        return phiIndex_ < block_->numPhis() || iter_ != block_->end();
    }
};

} 
} 

#endif 

