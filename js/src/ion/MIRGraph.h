








































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

typedef InlineListIterator<MInstruction> MInstructionIterator;
typedef InlineListReverseIterator<MInstruction> MInstructionReverseIterator;
typedef InlineForwardListIterator<MPhi> MPhiIterator;

class LBlock;

class MBasicBlock : public TempObject, public InlineListNode<MBasicBlock>
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
    MBasicBlock(MIRGraph &graph, CompileInfo &info, jsbytecode *pc, Kind kind);
    bool init();
    void copySlots(MBasicBlock *from);
    bool inherit(MBasicBlock *pred);
    void assertUsesAreNotWithin(MUseIterator use, MUseIterator end);

    
    void setSlot(uint32 slot, MDefinition *ins);

    
    void pushVariable(uint32 slot);

    
    
    void setVariable(uint32 slot);

  public:
    
    
    

    
    
    static MBasicBlock *New(MIRGraph &graph, CompileInfo &info,
                            MBasicBlock *pred, jsbytecode *entryPc, Kind kind);
    static MBasicBlock *NewPendingLoopHeader(MIRGraph &graph, CompileInfo &info,
                                             MBasicBlock *pred, jsbytecode *entryPc);
    static MBasicBlock *NewSplitEdge(MIRGraph &graph, CompileInfo &info, MBasicBlock *pred);

    void setId(uint32 id) {
        id_ = id;
    }

    
    MDefinition *peek(int32 depth);

    
    
    void initSlot(uint32 index, MDefinition *ins);

    
    
    void linkOsrValues(MStart *start);

    
    
    void setLocal(uint32 local);
    void setArg(uint32 arg);
    void setSlot(uint32 slot);

    
    
    void rewriteSlot(uint32 slot, MDefinition *ins);

    
    void push(MDefinition *ins);
    void pushArg(uint32 arg);
    void pushLocal(uint32 local);
    void pushSlot(uint32 slot);

    
    MDefinition *pop();

    
    
    void add(MInstruction *ins);

    
    
    void end(MControlInstruction *ins);

    
    void addPhi(MPhi *phi);

    
    
    
    bool addPredecessor(MBasicBlock *pred);

    
    bool addPredecessorWithoutPhis(MBasicBlock *pred);
    bool inheritNonPredecessor(MBasicBlock *parent);

    
    
    void replacePredecessor(MBasicBlock *old, MBasicBlock *split);
    void replaceSuccessor(size_t pos, MBasicBlock *split);

    
    
    bool setBackedge(MBasicBlock *block);

    
    void inheritPhis(MBasicBlock *header);

    void insertBefore(MInstruction *at, MInstruction *ins);
    void insertAfter(MInstruction *at, MInstruction *ins);

    
    
    void remove(MInstruction *ins);

    
    void discard(MInstruction *ins);
    void discardLastIns();
    MInstructionIterator discardAt(MInstructionIterator &iter);
    MInstructionReverseIterator discardAt(MInstructionReverseIterator &iter);
    MDefinitionIterator discardDefAt(MDefinitionIterator &iter);

    
    MPhiIterator discardPhiAt(MPhiIterator &at);

    
    
    

    MIRGraph &graph() {
        return graph_;
    }
    CompileInfo &info() const {
        return info_;
    }
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
    MPhiIterator phisBegin() const {
        return phis_.begin();
    }
    MPhiIterator phisEnd() const {
        return phis_.end();
    }
    bool phisEmpty() const {
        return phis_.empty();
    }
    MInstructionIterator begin() {
        return instructions_.begin();
    }
    MInstructionIterator begin(MInstruction *at) {
        JS_ASSERT(at->block() == this);
        return instructions_.begin(at);
    }
    MInstructionIterator end() {
        return instructions_.end();
    }
    MInstructionReverseIterator rbegin() {
        return instructions_.rbegin();
    }
    MInstructionReverseIterator rbegin(MInstruction *at) {
        JS_ASSERT(at->block() == this);
        return instructions_.rbegin(at);
    }
    MInstructionReverseIterator rend() {
        return instructions_.rend();
    }
    bool isLoopHeader() const {
        return kind_ == LOOP_HEADER;
    }
    MBasicBlock *backedge() const {
        JS_ASSERT(isLoopHeader());
        JS_ASSERT(numPredecessors() == 1 || numPredecessors() == 2);
        return getPredecessor(numPredecessors() - 1);
    }
    MBasicBlock *loopHeaderOfBackedge() const {
        JS_ASSERT(isLoopBackedge());
        return getSuccessor(numSuccessors() - 1);
    }
    MBasicBlock *loopPredecessor() const {
        JS_ASSERT(isLoopHeader());
        return getPredecessor(0);
    }
    bool isLoopBackedge() const {
        if (!numSuccessors())
            return false;
        MBasicBlock *lastSuccessor = getSuccessor(numSuccessors() - 1);
        return lastSuccessor->isLoopHeader() && lastSuccessor->backedge() == this;
    }
    bool isSplitEdge() const {
        return kind_ == SPLIT_EDGE;
    }

    uint32 stackDepth() const {
        return stackPosition_;
    }
    void setStackDepth(uint32 depth) {
        stackPosition_ = depth;
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

    MResumePoint *entryResumePoint() const {
        return entryResumePoint_;
    }
    MResumePoint *callerResumePoint() {
        return entryResumePoint()->caller();
    }
    void setCallerResumePoint(MResumePoint *caller) {
        entryResumePoint()->setCaller(caller);
    }
    size_t numEntrySlots() const {
        return entryResumePoint()->numOperands();
    }
    MDefinition *getEntrySlot(size_t i) const {
        JS_ASSERT(i < numEntrySlots());
        return entryResumePoint()->getOperand(i);
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

    
    void setLoopHeader(MBasicBlock *loop) {
        JS_ASSERT(loop->isLoopHeader());
        loopHeader_ = loop;
    }
    MBasicBlock *loopHeader() const {
        return loopHeader_;
    }

    
    
    size_t numContainedInLoop() const {
        JS_ASSERT(isLoopHeader());
        return containedInLoop_.length();
    }
    MBasicBlock *getContainedInLoop(size_t i) const {
        JS_ASSERT(isLoopHeader());
        return containedInLoop_[i];
    }
    bool addContainedInLoop(MBasicBlock *block) {
        JS_ASSERT(isLoopHeader());
        return containedInLoop_.append(block);
    }

    void setLoopDepth(uint32 loopDepth) {
        loopDepth_ = loopDepth;
    }
    uint32 loopDepth() const {
        return loopDepth_;
    }

    void dumpStack(FILE *fp);

  private:
    MIRGraph &graph_;
    CompileInfo &info_; 
    InlineList<MInstruction> instructions_;
    Vector<MBasicBlock *, 1, IonAllocPolicy> predecessors_;
    InlineForwardList<MPhi> phis_;
    StackSlot *slots_;
    uint32 stackPosition_;
    MControlInstruction *lastIns_;
    jsbytecode *pc_;
    uint32 id_;
    LBlock *lir_;
    MStart *start_;
    MResumePoint *entryResumePoint_;
    MBasicBlock *successorWithPhis_;
    uint32 positionInPhiSuccessor_;
    Kind kind_;
    uint32 loopDepth_;

    
    bool mark_;

    Vector<MBasicBlock *, 1, IonAllocPolicy> immediatelyDominated_;
    Vector<MBasicBlock *, 1, IonAllocPolicy> containedInLoop_;
    MBasicBlock *immediateDominator_;
    size_t numDominated_;
    MBasicBlock *loopHeader_;
};

typedef InlineListIterator<MBasicBlock> MBasicBlockIterator;
typedef InlineListIterator<MBasicBlock> ReversePostorderIterator;
typedef InlineListReverseIterator<MBasicBlock> PostorderIterator;

typedef Vector<MBasicBlock *, 1, IonAllocPolicy> MIRGraphExits;

class MIRGraph
{
    InlineList<MBasicBlock> blocks_;
    TempAllocator &alloc_;
    MIRGraphExits *exitAccumulator;
    uint32 blockIdGen_;
    uint32 idGen_;
    MBasicBlock *osrBlock_;
    MStart *osrStart_;
#ifdef DEBUG
    size_t numBlocks_;
#endif

  public:
    MIRGraph(TempAllocator &alloc)
      : alloc_(alloc),
        exitAccumulator(NULL),
        blockIdGen_(0),
        idGen_(0),
        osrBlock_(NULL),
        osrStart_(NULL)
#ifdef DEBUG
        , numBlocks_(0)
#endif
    { }

    template <typename T>
    T * allocate(size_t count = 1) {
        return reinterpret_cast<T *>(alloc_.allocate(sizeof(T) * count));
    }

    void addBlock(MBasicBlock *block);
    void unmarkBlocks();

    void setExitAccumulator(MIRGraphExits *accum) {
        exitAccumulator = accum;
    }

    MIRGraphExits &getExitAccumulator() {
        JS_ASSERT(exitAccumulator);
        return *exitAccumulator;
    }

    bool addExit(MBasicBlock *exitBlock) {
        if (!exitAccumulator)
            return true;

        return exitAccumulator->append(exitBlock);
    }

    MBasicBlock *entryBlock() {
        return *blocks_.begin();
    }

    void clearBlockList() {
        blocks_.clear();
        blockIdGen_ = 0;
#ifdef DEBUG
        numBlocks_ = 0;
#endif
    }
    void resetInstructionNumber() {
        idGen_ = 0;
    }
    MBasicBlockIterator begin() {
        return blocks_.begin();
    }
    MBasicBlockIterator end() {
        return blocks_.end();
    }
    PostorderIterator poBegin() {
        return blocks_.rbegin();
    }
    PostorderIterator poEnd() {
        return blocks_.rend();
    }
    ReversePostorderIterator rpoBegin() {
        return blocks_.begin();
    }
    ReversePostorderIterator rpoEnd() {
        return blocks_.end();
    }
    void removeBlock(MBasicBlock *block) {
        blocks_.remove(block);
#ifdef DEBUG
        numBlocks_--;
#endif
    }
#ifdef DEBUG
    size_t numBlocks() const {
        return numBlocks_;
    }
#endif
    uint32 numBlockIds() const {
        return blockIdGen_;
    }
    void allocDefinitionId(MDefinition *ins) {
        
        
        idGen_ += 2;
        ins->setId(idGen_);
    }
    uint32 getMaxInstructionId() {
        return idGen_;
    }
    MResumePoint *entryResumePoint() {
        return blocks_.begin()->entryResumePoint();
    }

    void copyIds(const MIRGraph &other) {
        idGen_ = other.idGen_;
        blockIdGen_ = other.blockIdGen_;
#ifdef DEBUG
        numBlocks_ = other.numBlocks_;
#endif
    }

    void setOsrBlock(MBasicBlock *osrBlock) {
        JS_ASSERT(!osrBlock_);
        osrBlock_ = osrBlock;
    }
    MBasicBlock *osrBlock() {
        return osrBlock_;
    }
    void setOsrStart(MStart *osrStart) {
        osrStart_ = osrStart;
    }
    MStart *osrStart() {
        return osrStart_;
    }
};

class MDefinitionIterator
{

  friend class MBasicBlock;

  private:
    MBasicBlock *block_;
    MPhiIterator phiIter_;
    MInstructionIterator iter_;

    bool atPhi() const {
        return phiIter_ != block_->phisEnd();
    }

    MDefinition *getIns() {
        if (atPhi())
            return *phiIter_;
        return *iter_;
    }

    void next() {
        if (atPhi())
            phiIter_++;
        else
            iter_++;
    }

    bool more() const {
        return atPhi() || (*iter_) != block_->lastIns();
    }

  public:
    MDefinitionIterator(MBasicBlock *block)
      : block_(block),
        phiIter_(block->phisBegin()),
        iter_(block->begin())
    { }

    MDefinitionIterator operator ++(int) {
        MDefinitionIterator old(*this);
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

