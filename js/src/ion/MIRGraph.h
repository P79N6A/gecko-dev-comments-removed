






#ifndef jsion_mirgraph_h__
#define jsion_mirgraph_h__




#include "IonAllocPolicy.h"
#include "MIRGenerator.h"
#include "FixedList.h"

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
    bool inheritResumePoint(MBasicBlock *pred);
    void assertUsesAreNotWithin(MUseIterator use, MUseIterator end);

    
    bool earlyAbort_;


    
    void setSlot(uint32 slot, MDefinition *ins);

    
    void pushVariable(uint32 slot);

    
    
    void setVariable(uint32 slot);

  public:
    
    
    

    
    
    static MBasicBlock *New(MIRGraph &graph, CompileInfo &info,
                            MBasicBlock *pred, jsbytecode *entryPc, Kind kind);
    static MBasicBlock *NewWithResumePoint(MIRGraph &graph, CompileInfo &info,
                                           MBasicBlock *pred, jsbytecode *entryPc,
                                           MResumePoint *resumePoint);
    static MBasicBlock *NewPendingLoopHeader(MIRGraph &graph, CompileInfo &info,
                                             MBasicBlock *pred, jsbytecode *entryPc);
    static MBasicBlock *NewSplitEdge(MIRGraph &graph, CompileInfo &info, MBasicBlock *pred);

    bool dominates(MBasicBlock *other);

    void setId(uint32 id) {
        id_ = id;
    }
    void setEarlyAbort() {
        earlyAbort_ = true;
    }
    void clearEarlyAbort() {
        earlyAbort_ = false;
    }
    bool earlyAbort() {
        return earlyAbort_;
    }
    
    void pick(int32 depth);

    
    void swapAt(int32 depth);

    
    MDefinition *peek(int32 depth);

    MDefinition *scopeChain();

    
    
    void initSlot(uint32 index, MDefinition *ins);

    
    void shimmySlots(int discardDepth);

    
    
    void linkOsrValues(MStart *start);

    
    
    void setLocal(uint32 local);
    void setArg(uint32 arg);
    void setSlot(uint32 slot);

    
    
    void rewriteSlot(uint32 slot, MDefinition *ins);

    
    void rewriteAtDepth(int32 depth, MDefinition *ins);

    
    void push(MDefinition *ins);
    void pushArg(uint32 arg);
    void pushLocal(uint32 local);
    void pushSlot(uint32 slot);
    void setScopeChain(MDefinition *ins);

    
    MDefinition *pop();

    
    
    void add(MInstruction *ins);

    
    
    void end(MControlInstruction *ins);

    
    void addPhi(MPhi *phi);

    
    
    
    bool addPredecessor(MBasicBlock *pred);

    
    bool addPredecessorWithoutPhis(MBasicBlock *pred);
    void inheritSlots(MBasicBlock *parent);
    bool initEntrySlots();

    
    
    void replacePredecessor(MBasicBlock *old, MBasicBlock *split);
    void replaceSuccessor(size_t pos, MBasicBlock *split);

    
    
    bool setBackedge(MBasicBlock *block);

    
    void inheritPhis(MBasicBlock *header);

    void insertBefore(MInstruction *at, MInstruction *ins);
    void insertAfter(MInstruction *at, MInstruction *ins);

    
    void addFromElsewhere(MInstruction *ins);

    
    void moveBefore(MInstruction *at, MInstruction *ins);

    
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

    uint32 domIndex() const {
        return domIndex_;
    }
    void setDomIndex(uint32 d) {
        domIndex_ = d;
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

    MTest *immediateDominatorBranch(BranchDirection *pdirection);

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

    void setLoopDepth(uint32 loopDepth) {
        loopDepth_ = loopDepth;
    }
    uint32 loopDepth() const {
        return loopDepth_;
    }

    bool strictModeCode() const {
        return info_.script()->strictModeCode;
    }

    void dumpStack(FILE *fp);

    
    
    void updateTrackedPc(jsbytecode *pc) {
        trackedPc_ = pc;
    }

    jsbytecode *trackedPc() {
        return trackedPc_;
    }

  private:
    MIRGraph &graph_;
    CompileInfo &info_; 
    InlineList<MInstruction> instructions_;
    Vector<MBasicBlock *, 1, IonAllocPolicy> predecessors_;
    InlineForwardList<MPhi> phis_;
    FixedList<MDefinition *> slots_;
    uint32 stackPosition_;
    MControlInstruction *lastIns_;
    jsbytecode *pc_;
    uint32 id_;
    uint32 domIndex_; 
    LBlock *lir_;
    MStart *start_;
    MResumePoint *entryResumePoint_;
    MBasicBlock *successorWithPhis_;
    uint32 positionInPhiSuccessor_;
    Kind kind_;
    uint32 loopDepth_;

    
    bool mark_;

    Vector<MBasicBlock *, 1, IonAllocPolicy> immediatelyDominated_;
    MBasicBlock *immediateDominator_;
    size_t numDominated_;
    MBasicBlock *loopHeader_;

    jsbytecode *trackedPc_;
};

typedef InlineListIterator<MBasicBlock> MBasicBlockIterator;
typedef InlineListIterator<MBasicBlock> ReversePostorderIterator;
typedef InlineListReverseIterator<MBasicBlock> PostorderIterator;

typedef Vector<MBasicBlock *, 1, IonAllocPolicy> MIRGraphExits;

class MIRGraph
{
    InlineList<MBasicBlock> blocks_;
    TempAllocator *alloc_;
    MIRGraphExits *exitAccumulator_;
    uint32 blockIdGen_;
    uint32 idGen_;
    MBasicBlock *osrBlock_;
    MStart *osrStart_;

    
    Vector<JSScript *, 4, IonAllocPolicy> scripts_;

#ifdef DEBUG
    size_t numBlocks_;
#endif

  public:
    MIRGraph(TempAllocator *alloc)
      : alloc_(alloc),
        exitAccumulator_(NULL),
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
        return reinterpret_cast<T *>(alloc_->allocate(sizeof(T) * count));
    }

    void addBlock(MBasicBlock *block);
    void insertBlockAfter(MBasicBlock *at, MBasicBlock *block);

    void unmarkBlocks();

    void setExitAccumulator(MIRGraphExits *accum) {
        exitAccumulator_ = accum;
    }
    MIRGraphExits *exitAccumulator() const {
        return exitAccumulator_;
    }

    bool addExit(MBasicBlock *exitBlock) {
        if (!exitAccumulator_)
            return true;

        return exitAccumulator_->append(exitBlock);
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
    MBasicBlockIterator begin(MBasicBlock *at) {
        return blocks_.begin(at);
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
    void moveBlockToEnd(MBasicBlock *block) {
        JS_ASSERT(block->id());
        blocks_.remove(block);
        blocks_.pushBack(block);
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
    bool addScript(JSScript *script) {
        
        for (size_t i = 0; i < scripts_.length(); i++) {
            if (scripts_[i] == script)
                return true;
        }
        return scripts_.append(script);
    }
    size_t numScripts() const {
        return scripts_.length();
    }
    JSScript **scripts() {
        return scripts_.begin();
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

