





#ifndef jit_MIRGraph_h
#define jit_MIRGraph_h




#include "jit/FixedList.h"
#include "jit/IonAllocPolicy.h"
#include "jit/MIR.h"

namespace js {
namespace jit {

class BytecodeAnalysis;
class MBasicBlock;
class MIRGraph;
class MStart;

class MDefinitionIterator;

typedef InlineListIterator<MInstruction> MInstructionIterator;
typedef InlineListReverseIterator<MInstruction> MInstructionReverseIterator;
typedef InlineForwardListIterator<MPhi> MPhiIterator;
typedef InlineForwardListIterator<MResumePoint> MResumePointIterator;

class LBlock;

class MBasicBlock : public TempObject, public InlineListNode<MBasicBlock>
{
  public:
    enum Kind {
        NORMAL,
        PENDING_LOOP_HEADER,
        LOOP_HEADER,
        SPLIT_EDGE,
        DEAD
    };

  private:
    MBasicBlock(MIRGraph &graph, CompileInfo &info, const BytecodeSite &site, Kind kind);
    bool init();
    void copySlots(MBasicBlock *from);
    bool inherit(TempAllocator &alloc, BytecodeAnalysis *analysis, MBasicBlock *pred,
                 uint32_t popped, unsigned stackPhiCount = 0);
    bool inheritResumePoint(MBasicBlock *pred);
    void assertUsesAreNotWithin(MUseIterator use, MUseIterator end);

    
    bool unreachable_;

    
    void pushVariable(uint32_t slot);

    
    
    void setVariable(uint32_t slot);

  public:
    
    
    

    
    
    static MBasicBlock *New(MIRGraph &graph, BytecodeAnalysis *analysis, CompileInfo &info,
                            MBasicBlock *pred, const BytecodeSite &site, Kind kind);
    static MBasicBlock *NewPopN(MIRGraph &graph, CompileInfo &info,
                                MBasicBlock *pred, const BytecodeSite &site, Kind kind, uint32_t popn);
    static MBasicBlock *NewWithResumePoint(MIRGraph &graph, CompileInfo &info,
                                           MBasicBlock *pred, const BytecodeSite &site,
                                           MResumePoint *resumePoint);
    static MBasicBlock *NewPendingLoopHeader(MIRGraph &graph, CompileInfo &info,
                                             MBasicBlock *pred, const BytecodeSite &site,
                                             unsigned loopStateSlots);
    static MBasicBlock *NewSplitEdge(MIRGraph &graph, CompileInfo &info, MBasicBlock *pred);
    static MBasicBlock *NewAsmJS(MIRGraph &graph, CompileInfo &info,
                                 MBasicBlock *pred, Kind kind);

    bool dominates(const MBasicBlock *other) const;

    void setId(uint32_t id) {
        id_ = id;
    }

    
    void setUnreachable() {
        JS_ASSERT(!unreachable_);
        setUnreachableUnchecked();
    }
    void setUnreachableUnchecked() {
        unreachable_ = true;
    }
    bool unreachable() const {
        return unreachable_;
    }
    
    void pick(int32_t depth);

    
    void swapAt(int32_t depth);

    
    MDefinition *peek(int32_t depth);

    MDefinition *scopeChain();
    MDefinition *argumentsObject();

    
    bool increaseSlots(size_t num);
    bool ensureHasSlots(size_t num);

    
    
    void initSlot(uint32_t index, MDefinition *ins);

    
    void shimmySlots(int discardDepth);

    
    
    void linkOsrValues(MStart *start);

    
    
    void setLocal(uint32_t local);
    void setArg(uint32_t arg);
    void setSlot(uint32_t slot);
    void setSlot(uint32_t slot, MDefinition *ins);

    
    
    void rewriteSlot(uint32_t slot, MDefinition *ins);

    
    void rewriteAtDepth(int32_t depth, MDefinition *ins);

    
    void push(MDefinition *ins);
    void pushArg(uint32_t arg);
    void pushLocal(uint32_t local);
    void pushSlot(uint32_t slot);
    void setScopeChain(MDefinition *ins);
    void setArgumentsObject(MDefinition *ins);

    
    MDefinition *pop();
    void popn(uint32_t n);

    
    
    void add(MInstruction *ins);

    
    
    void end(MControlInstruction *ins);

    
    void addPhi(MPhi *phi);

    
    void addResumePoint(MResumePoint *resume) {
        resumePoints_.pushFront(resume);
    }

    
    
    
    bool addPredecessor(TempAllocator &alloc, MBasicBlock *pred);
    bool addPredecessorPopN(TempAllocator &alloc, MBasicBlock *pred, uint32_t popped);

    
    bool addPredecessorWithoutPhis(MBasicBlock *pred);
    void inheritSlots(MBasicBlock *parent);
    bool initEntrySlots(TempAllocator &alloc);

    
    
    
    
    
    void replacePredecessor(MBasicBlock *old, MBasicBlock *split);
    void replaceSuccessor(size_t pos, MBasicBlock *split);

    
    
    
    
    
    void removePredecessor(MBasicBlock *pred);

    
    void clearDominatorInfo();

    
    
    
    AbortReason setBackedge(MBasicBlock *block);
    bool setBackedgeAsmJS(MBasicBlock *block);

    
    
    void clearLoopHeader();

    
    void inheritPhis(MBasicBlock *header);

    
    bool inheritPhisFromBackedge(MBasicBlock *backedge, bool *hadTypeChange);

    
    bool specializePhis();

    void insertBefore(MInstruction *at, MInstruction *ins);
    void insertAfter(MInstruction *at, MInstruction *ins);

    
    void addFromElsewhere(MInstruction *ins);

    
    void moveBefore(MInstruction *at, MInstruction *ins);

    
    void discard(MInstruction *ins);
    void discardLastIns();
    MInstructionIterator discardAt(MInstructionIterator &iter);
    MInstructionReverseIterator discardAt(MInstructionReverseIterator &iter);
    MDefinitionIterator discardDefAt(MDefinitionIterator &iter);
    void discardAllInstructions();
    void discardAllPhiOperands();
    void discardAllPhis();
    void discardAllResumePoints(bool discardEntry = true);

    
    MPhiIterator discardPhiAt(MPhiIterator &at);

    
    void markAsDead() {
        kind_ = DEAD;
    }

    
    
    

    MIRGraph &graph() {
        return graph_;
    }
    CompileInfo &info() const {
        return info_;
    }
    jsbytecode *pc() const {
        return pc_;
    }
    uint32_t nslots() const {
        return slots_.length();
    }
    uint32_t id() const {
        return id_;
    }
    uint32_t numPredecessors() const {
        return predecessors_.length();
    }

    uint32_t domIndex() const {
        JS_ASSERT(!isDead());
        return domIndex_;
    }
    void setDomIndex(uint32_t d) {
        domIndex_ = d;
    }

    MBasicBlock *getPredecessor(uint32_t i) const {
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
    MResumePointIterator resumePointsBegin() const {
        return resumePoints_.begin();
    }
    MResumePointIterator resumePointsEnd() const {
        return resumePoints_.end();
    }
    bool resumePointsEmpty() const {
        return resumePoints_.empty();
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
    bool hasUniqueBackedge() const {
        JS_ASSERT(isLoopHeader());
        JS_ASSERT(numPredecessors() >= 2);
        return numPredecessors() == 2;
    }
    MBasicBlock *backedge() const {
        JS_ASSERT(hasUniqueBackedge());
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
        return lastSuccessor->isLoopHeader() &&
               lastSuccessor->hasUniqueBackedge() &&
               lastSuccessor->backedge() == this;
    }
    bool isSplitEdge() const {
        return kind_ == SPLIT_EDGE;
    }
    bool isDead() const {
        return kind_ == DEAD;
    }

    uint32_t stackDepth() const {
        return stackPosition_;
    }
    void setStackDepth(uint32_t depth) {
        stackPosition_ = depth;
    }
    bool isMarked() const {
        return mark_;
    }
    void mark() {
        MOZ_ASSERT(!mark_, "Marking already-marked block");
        markUnchecked();
    }
    void markUnchecked() {
        mark_ = true;
    }
    void unmark() {
        MOZ_ASSERT(mark_, "Unarking unmarked block");
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

    MBasicBlock **immediatelyDominatedBlocksBegin() {
        return immediatelyDominated_.begin();
    }

    MBasicBlock **immediatelyDominatedBlocksEnd() {
        return immediatelyDominated_.end();
    }

    
    
    size_t numDominated() const {
        JS_ASSERT(numDominated_ != 0);
        return numDominated_;
    }

    void addNumDominated(size_t n) {
        numDominated_ += n;
    }

    bool addImmediatelyDominatedBlock(MBasicBlock *child);

    
    
    
    MDefinition *getSlot(uint32_t index);

    MResumePoint *entryResumePoint() const {
        return entryResumePoint_;
    }
    void clearEntryResumePoint() {
        entryResumePoint_ = nullptr;
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
    uint32_t positionInPhiSuccessor() const {
        return positionInPhiSuccessor_;
    }
    void setSuccessorWithPhis(MBasicBlock *successor, uint32_t id) {
        successorWithPhis_ = successor;
        positionInPhiSuccessor_ = id;
    }
    size_t numSuccessors() const;
    MBasicBlock *getSuccessor(size_t index) const;
    size_t getSuccessorIndex(MBasicBlock *) const;

    void setLoopDepth(uint32_t loopDepth) {
        loopDepth_ = loopDepth;
    }
    uint32_t loopDepth() const {
        return loopDepth_;
    }

    bool strict() const {
        return info_.script()->strict();
    }

    void dumpStack(FILE *fp);

    void dump(FILE *fp);
    void dump();

    
    
    void updateTrackedSite(const BytecodeSite &site) {
        JS_ASSERT(site.tree() == trackedSite_.tree());
        trackedSite_ = site;
    }
    const BytecodeSite &trackedSite() const {
        return trackedSite_;
    }
    jsbytecode *trackedPc() const {
        return trackedSite_.pc();
    }
    InlineScriptTree *trackedTree() const {
        return trackedSite_.tree();
    }

  private:
    MIRGraph &graph_;
    CompileInfo &info_; 
    InlineList<MInstruction> instructions_;
    Vector<MBasicBlock *, 1, IonAllocPolicy> predecessors_;
    InlineForwardList<MPhi> phis_;
    InlineForwardList<MResumePoint> resumePoints_;
    FixedList<MDefinition *> slots_;
    uint32_t stackPosition_;
    MControlInstruction *lastIns_;
    jsbytecode *pc_;
    uint32_t id_;
    uint32_t domIndex_; 
    LBlock *lir_;
    MStart *start_;
    MResumePoint *entryResumePoint_;
    MBasicBlock *successorWithPhis_;
    uint32_t positionInPhiSuccessor_;
    Kind kind_;
    uint32_t loopDepth_;

    
    bool mark_;

    Vector<MBasicBlock *, 1, IonAllocPolicy> immediatelyDominated_;
    MBasicBlock *immediateDominator_;
    size_t numDominated_;

    BytecodeSite trackedSite_;

#if defined (JS_ION_PERF)
    unsigned lineno_;
    unsigned columnIndex_;

  public:
    void setLineno(unsigned l) { lineno_ = l; }
    unsigned lineno() const { return lineno_; }
    void setColumnIndex(unsigned c) { columnIndex_ = c; }
    unsigned columnIndex() const { return columnIndex_; }
#endif
};

typedef InlineListIterator<MBasicBlock> MBasicBlockIterator;
typedef InlineListIterator<MBasicBlock> ReversePostorderIterator;
typedef InlineListReverseIterator<MBasicBlock> PostorderIterator;

typedef Vector<MBasicBlock *, 1, IonAllocPolicy> MIRGraphReturns;

class MIRGraph
{
    InlineList<MBasicBlock> blocks_;
    TempAllocator *alloc_;
    MIRGraphReturns *returnAccumulator_;
    uint32_t blockIdGen_;
    uint32_t idGen_;
    MBasicBlock *osrBlock_;
    MStart *osrStart_;

    size_t numBlocks_;
    bool hasTryBlock_;

  public:
    explicit MIRGraph(TempAllocator *alloc)
      : alloc_(alloc),
        returnAccumulator_(nullptr),
        blockIdGen_(0),
        idGen_(1),
        osrBlock_(nullptr),
        osrStart_(nullptr),
        numBlocks_(0),
        hasTryBlock_(false)
    { }

    TempAllocator &alloc() const {
        return *alloc_;
    }

    void addBlock(MBasicBlock *block);
    void insertBlockAfter(MBasicBlock *at, MBasicBlock *block);

    void unmarkBlocks();

    void setReturnAccumulator(MIRGraphReturns *accum) {
        returnAccumulator_ = accum;
    }
    MIRGraphReturns *returnAccumulator() const {
        return returnAccumulator_;
    }

    bool addReturn(MBasicBlock *returnBlock) {
        if (!returnAccumulator_)
            return true;

        return returnAccumulator_->append(returnBlock);
    }

    MBasicBlock *entryBlock() {
        return *blocks_.begin();
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
    PostorderIterator poBegin(MBasicBlock *at) {
        return blocks_.rbegin(at);
    }
    PostorderIterator poEnd() {
        return blocks_.rend();
    }
    ReversePostorderIterator rpoBegin() {
        return blocks_.begin();
    }
    ReversePostorderIterator rpoBegin(MBasicBlock *at) {
        return blocks_.begin(at);
    }
    ReversePostorderIterator rpoEnd() {
        return blocks_.end();
    }
    void removeBlocksAfter(MBasicBlock *block);
    void removeBlock(MBasicBlock *block);
    void moveBlockToEnd(MBasicBlock *block) {
        JS_ASSERT(block->id());
        blocks_.remove(block);
        blocks_.pushBack(block);
    }
    void moveBlockBefore(MBasicBlock *at, MBasicBlock *block) {
        JS_ASSERT(block->id());
        blocks_.remove(block);
        blocks_.insertBefore(at, block);
    }
    size_t numBlocks() const {
        return numBlocks_;
    }
    uint32_t numBlockIds() const {
        return blockIdGen_;
    }
    void allocDefinitionId(MDefinition *ins) {
        ins->setId(idGen_++);
    }
    uint32_t getNumInstructionIds() {
        return idGen_;
    }
    MResumePoint *entryResumePoint() {
        return entryBlock()->entryResumePoint();
    }

    void copyIds(const MIRGraph &other) {
        idGen_ = other.idGen_;
        blockIdGen_ = other.blockIdGen_;
        numBlocks_ = other.numBlocks_;
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

    bool hasTryBlock() const {
        return hasTryBlock_;
    }
    void setHasTryBlock() {
        hasTryBlock_ = true;
    }

    
    
    
    
    MDefinition *forkJoinContext();

    void dump(FILE *fp);
    void dump();
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
    explicit MDefinitionIterator(MBasicBlock *block)
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
