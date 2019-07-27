





#ifndef jit_MIRGraph_h
#define jit_MIRGraph_h




#include "jit/FixedList.h"
#include "jit/JitAllocPolicy.h"
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
typedef InlineListIterator<MPhi> MPhiIterator;

#ifdef DEBUG
typedef InlineForwardListIterator<MResumePoint> MResumePointIterator;
#endif

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
    MBasicBlock(MIRGraph& graph, CompileInfo& info, BytecodeSite* site, Kind kind);
    bool init();
    void copySlots(MBasicBlock* from);
    bool inherit(TempAllocator& alloc, BytecodeAnalysis* analysis, MBasicBlock* pred,
                 uint32_t popped, unsigned stackPhiCount = 0);
    bool inheritResumePoint(MBasicBlock* pred);
    void assertUsesAreNotWithin(MUseIterator use, MUseIterator end);

    
    bool unreachable_;

    MResumePoint* callerResumePoint_;

    
    void pushVariable(uint32_t slot);

    
    
    void setVariable(uint32_t slot);

    enum ReferencesType {
        RefType_None = 0,

        
        RefType_AssertNoUses = 1 << 0,

        
        
        RefType_DiscardOperands = 1 << 1,
        RefType_DiscardResumePoint = 1 << 2,
        RefType_DiscardInstruction = 1 << 3,

        
        RefType_DefaultNoAssert = RefType_DiscardOperands |
                                  RefType_DiscardResumePoint |
                                  RefType_DiscardInstruction,

        
        RefType_Default = RefType_AssertNoUses | RefType_DefaultNoAssert,

        
        
        RefType_IgnoreOperands = RefType_AssertNoUses |
                                 RefType_DiscardOperands |
                                 RefType_DiscardResumePoint
    };

    void discardResumePoint(MResumePoint* rp, ReferencesType refType = RefType_Default);

    
    
    
    
    void prepareForDiscard(MInstruction* ins, ReferencesType refType = RefType_Default);

  public:
    
    
    

    
    
    static MBasicBlock* New(MIRGraph& graph, BytecodeAnalysis* analysis, CompileInfo& info,
                            MBasicBlock* pred, BytecodeSite* site, Kind kind);
    static MBasicBlock* NewPopN(MIRGraph& graph, CompileInfo& info,
                                MBasicBlock* pred, BytecodeSite* site, Kind kind, uint32_t popn);
    static MBasicBlock* NewWithResumePoint(MIRGraph& graph, CompileInfo& info,
                                           MBasicBlock* pred, BytecodeSite* site,
                                           MResumePoint* resumePoint);
    static MBasicBlock* NewPendingLoopHeader(MIRGraph& graph, CompileInfo& info,
                                             MBasicBlock* pred, BytecodeSite* site,
                                             unsigned loopStateSlots);
    static MBasicBlock* NewSplitEdge(MIRGraph& graph, CompileInfo& info, MBasicBlock* pred);
    static MBasicBlock* NewAsmJS(MIRGraph& graph, CompileInfo& info,
                                 MBasicBlock* pred, Kind kind);

    bool dominates(const MBasicBlock* other) const {
        return other->domIndex() - domIndex() < numDominated();
    }

    void setId(uint32_t id) {
        id_ = id;
    }

    
    void setUnreachable() {
        MOZ_ASSERT(!unreachable_);
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

    
    MDefinition* peek(int32_t depth);

    MDefinition* scopeChain();
    MDefinition* argumentsObject();

    
    bool increaseSlots(size_t num);
    bool ensureHasSlots(size_t num);

    
    
    void initSlot(uint32_t index, MDefinition* ins);

    
    void shimmySlots(int discardDepth);

    
    
    bool linkOsrValues(MStart* start);

    
    
    void setLocal(uint32_t local);
    void setArg(uint32_t arg);
    void setSlot(uint32_t slot);
    void setSlot(uint32_t slot, MDefinition* ins);

    
    
    void rewriteSlot(uint32_t slot, MDefinition* ins);

    
    void rewriteAtDepth(int32_t depth, MDefinition* ins);

    
    void push(MDefinition* ins);
    void pushArg(uint32_t arg);
    void pushLocal(uint32_t local);
    void pushSlot(uint32_t slot);
    void setScopeChain(MDefinition* ins);
    void setArgumentsObject(MDefinition* ins);

    
    MDefinition* pop();
    void popn(uint32_t n);

    
    void add(MInstruction* ins);

    
    
    void end(MControlInstruction* ins);

    
    void addPhi(MPhi* phi);

    
    void addResumePoint(MResumePoint* resume) {
#ifdef DEBUG
        resumePoints_.pushFront(resume);
#endif
    }

    
    void discardPreAllocatedResumePoint(MResumePoint* resume) {
        MOZ_ASSERT(!resume->instruction());
        discardResumePoint(resume);
    }

    
    
    
    bool addPredecessor(TempAllocator& alloc, MBasicBlock* pred);
    bool addPredecessorPopN(TempAllocator& alloc, MBasicBlock* pred, uint32_t popped);

    
    
    void addPredecessorSameInputsAs(MBasicBlock* pred, MBasicBlock* existingPred);

    
    bool addPredecessorWithoutPhis(MBasicBlock* pred);
    void inheritSlots(MBasicBlock* parent);
    bool initEntrySlots(TempAllocator& alloc);

    
    
    
    
    void replacePredecessor(MBasicBlock* old, MBasicBlock* split);
    void replaceSuccessor(size_t pos, MBasicBlock* split);

    
    
    
    
    void removePredecessor(MBasicBlock* pred);

    
    
    void removePredecessorWithoutPhiOperands(MBasicBlock* pred, size_t predIndex);

    
    void clearDominatorInfo();

    
    
    
    AbortReason setBackedge(MBasicBlock* block);
    bool setBackedgeAsmJS(MBasicBlock* block);

    
    
    void clearLoopHeader();

    
    
    
    void setLoopHeader(MBasicBlock* newBackedge);

    
    void inheritPhis(MBasicBlock* header);

    
    bool inheritPhisFromBackedge(MBasicBlock* backedge, bool* hadTypeChange);

    
    bool specializePhis();

    void insertBefore(MInstruction* at, MInstruction* ins);
    void insertAfter(MInstruction* at, MInstruction* ins);

    void insertAtEnd(MInstruction* ins);

    
    void addFromElsewhere(MInstruction* ins);

    
    void moveBefore(MInstruction* at, MInstruction* ins);

    enum IgnoreTop {
        IgnoreNone = 0,
        IgnoreRecover = 1 << 0
    };

    
    
    MInstruction* safeInsertTop(MDefinition* ins = nullptr, IgnoreTop ignore = IgnoreNone);

    
    void discard(MInstruction* ins);
    void discardLastIns();
    void discardDef(MDefinition* def);
    void discardAllInstructions();
    void discardAllInstructionsStartingAt(MInstructionIterator iter);
    void discardAllPhiOperands();
    void discardAllPhis();
    void discardAllResumePoints(bool discardEntry = true);

    
    
    void discardIgnoreOperands(MInstruction* ins);

    
    void discardPhi(MPhi* phi);

    
    
    
    
    
    void flagOperandsOfPrunedBranches(MInstruction* ins);

    
    void markAsDead() {
        MOZ_ASSERT(kind_ != DEAD);
        kind_ = DEAD;
    }

    
    
    

    MIRGraph& graph() {
        return graph_;
    }
    CompileInfo& info() const {
        return info_;
    }
    jsbytecode* pc() const {
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
        MOZ_ASSERT(!isDead());
        return domIndex_;
    }
    void setDomIndex(uint32_t d) {
        domIndex_ = d;
    }

    MBasicBlock* getPredecessor(uint32_t i) const {
        return predecessors_[i];
    }
    size_t indexForPredecessor(MBasicBlock* block) const {
        
        MOZ_ASSERT(!block->successorWithPhis());

        for (size_t i = 0; i < predecessors_.length(); i++) {
            if (predecessors_[i] == block)
                return i;
        }
        MOZ_CRASH();
    }
    bool hasLastIns() const {
        return !instructions_.empty() && instructions_.rbegin()->isControlInstruction();
    }
    MControlInstruction* lastIns() const {
        MOZ_ASSERT(hasLastIns());
        return instructions_.rbegin()->toControlInstruction();
    }
    
    MConstant* optimizedOutConstant(TempAllocator& alloc);
    MPhiIterator phisBegin() const {
        return phis_.begin();
    }
    MPhiIterator phisBegin(MPhi* at) const {
        return phis_.begin(at);
    }
    MPhiIterator phisEnd() const {
        return phis_.end();
    }
    bool phisEmpty() const {
        return phis_.empty();
    }
#ifdef DEBUG
    MResumePointIterator resumePointsBegin() const {
        return resumePoints_.begin();
    }
    MResumePointIterator resumePointsEnd() const {
        return resumePoints_.end();
    }
    bool resumePointsEmpty() const {
        return resumePoints_.empty();
    }
#endif
    MInstructionIterator begin() {
        return instructions_.begin();
    }
    MInstructionIterator begin(MInstruction* at) {
        MOZ_ASSERT(at->block() == this);
        return instructions_.begin(at);
    }
    MInstructionIterator end() {
        return instructions_.end();
    }
    MInstructionReverseIterator rbegin() {
        return instructions_.rbegin();
    }
    MInstructionReverseIterator rbegin(MInstruction* at) {
        MOZ_ASSERT(at->block() == this);
        return instructions_.rbegin(at);
    }
    MInstructionReverseIterator rend() {
        return instructions_.rend();
    }
    bool isLoopHeader() const {
        return kind_ == LOOP_HEADER;
    }
    bool hasUniqueBackedge() const {
        MOZ_ASSERT(isLoopHeader());
        MOZ_ASSERT(numPredecessors() >= 2);
        return numPredecessors() == 2;
    }
    MBasicBlock* backedge() const {
        MOZ_ASSERT(hasUniqueBackedge());
        return getPredecessor(numPredecessors() - 1);
    }
    MBasicBlock* loopHeaderOfBackedge() const {
        MOZ_ASSERT(isLoopBackedge());
        return getSuccessor(numSuccessors() - 1);
    }
    MBasicBlock* loopPredecessor() const {
        MOZ_ASSERT(isLoopHeader());
        return getPredecessor(0);
    }
    bool isLoopBackedge() const {
        if (!numSuccessors())
            return false;
        MBasicBlock* lastSuccessor = getSuccessor(numSuccessors() - 1);
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

    MBasicBlock* immediateDominator() const {
        return immediateDominator_;
    }

    void setImmediateDominator(MBasicBlock* dom) {
        immediateDominator_ = dom;
    }

    MTest* immediateDominatorBranch(BranchDirection* pdirection);

    size_t numImmediatelyDominatedBlocks() const {
        return immediatelyDominated_.length();
    }

    MBasicBlock* getImmediatelyDominatedBlock(size_t i) const {
        return immediatelyDominated_[i];
    }

    MBasicBlock** immediatelyDominatedBlocksBegin() {
        return immediatelyDominated_.begin();
    }

    MBasicBlock** immediatelyDominatedBlocksEnd() {
        return immediatelyDominated_.end();
    }

    
    
    size_t numDominated() const {
        MOZ_ASSERT(numDominated_ != 0);
        return numDominated_;
    }

    void addNumDominated(size_t n) {
        numDominated_ += n;
    }

    
    bool addImmediatelyDominatedBlock(MBasicBlock* child);

    
    void removeImmediatelyDominatedBlock(MBasicBlock* child);

    
    
    
    MDefinition* getSlot(uint32_t index);

    MResumePoint* entryResumePoint() const {
        return entryResumePoint_;
    }
    void setEntryResumePoint(MResumePoint* rp) {
        entryResumePoint_ = rp;
    }
    void clearEntryResumePoint() {
        discardResumePoint(entryResumePoint_);
        entryResumePoint_ = nullptr;
    }
    MResumePoint* outerResumePoint() const {
        return outerResumePoint_;
    }
    void setOuterResumePoint(MResumePoint* outer) {
        MOZ_ASSERT(!outerResumePoint_);
        outerResumePoint_ = outer;
    }
    void clearOuterResumePoint() {
        discardResumePoint(outerResumePoint_);
        outerResumePoint_ = nullptr;
    }
    MResumePoint* callerResumePoint() const {
        return callerResumePoint_;
    }
    void setCallerResumePoint(MResumePoint* caller) {
        callerResumePoint_ = caller;
    }
    size_t numEntrySlots() const {
        return entryResumePoint()->stackDepth();
    }
    MDefinition* getEntrySlot(size_t i) const {
        MOZ_ASSERT(i < numEntrySlots());
        return entryResumePoint()->getOperand(i);
    }

    LBlock* lir() const {
        return lir_;
    }
    void assignLir(LBlock* lir) {
        MOZ_ASSERT(!lir_);
        lir_ = lir;
    }

    MBasicBlock* successorWithPhis() const {
        return successorWithPhis_;
    }
    uint32_t positionInPhiSuccessor() const {
        MOZ_ASSERT(successorWithPhis());
        return positionInPhiSuccessor_;
    }
    void setSuccessorWithPhis(MBasicBlock* successor, uint32_t id) {
        successorWithPhis_ = successor;
        positionInPhiSuccessor_ = id;
    }
    void clearSuccessorWithPhis() {
        successorWithPhis_ = nullptr;
    }
    size_t numSuccessors() const;
    MBasicBlock* getSuccessor(size_t index) const;
    size_t getSuccessorIndex(MBasicBlock*) const;
    size_t getPredecessorIndex(MBasicBlock*) const;

    void setLoopDepth(uint32_t loopDepth) {
        loopDepth_ = loopDepth;
    }
    uint32_t loopDepth() const {
        return loopDepth_;
    }

    bool strict() const {
        return info_.script()->strict();
    }

    void dumpStack(FILE* fp);

    void dump(FILE* fp);
    void dump();

    
    
    
    void updateTrackedSite(BytecodeSite* site) {
        MOZ_ASSERT(site->tree() == trackedSite_->tree());
        trackedSite_ = site;
    }
    BytecodeSite* trackedSite() const {
        return trackedSite_;
    }
    jsbytecode* trackedPc() const {
        return trackedSite_ ? trackedSite_->pc() : nullptr;
    }
    InlineScriptTree* trackedTree() const {
        return trackedSite_ ? trackedSite_->tree() : nullptr;
    }

  private:
    MIRGraph& graph_;
    CompileInfo& info_; 
    InlineList<MInstruction> instructions_;
    Vector<MBasicBlock*, 1, JitAllocPolicy> predecessors_;
    InlineList<MPhi> phis_;
    FixedList<MDefinition*> slots_;
    uint32_t stackPosition_;
    uint32_t id_;
    uint32_t domIndex_; 
    uint32_t numDominated_;
    jsbytecode* pc_;
    LBlock* lir_;

    
    
    MResumePoint* entryResumePoint_;

    
    
    MResumePoint* outerResumePoint_;

#ifdef DEBUG
    
    
    InlineForwardList<MResumePoint> resumePoints_;
#endif

    MBasicBlock* successorWithPhis_;
    uint32_t positionInPhiSuccessor_;
    uint32_t loopDepth_;
    Kind kind_ : 8;

    
    bool mark_;

    Vector<MBasicBlock*, 1, JitAllocPolicy> immediatelyDominated_;
    MBasicBlock* immediateDominator_;

    BytecodeSite* trackedSite_;

#if defined(JS_ION_PERF) || defined(DEBUG)
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

typedef Vector<MBasicBlock*, 1, JitAllocPolicy> MIRGraphReturns;

class MIRGraph
{
    InlineList<MBasicBlock> blocks_;
    TempAllocator* alloc_;
    MIRGraphReturns* returnAccumulator_;
    uint32_t blockIdGen_;
    uint32_t idGen_;
    MBasicBlock* osrBlock_;

    size_t numBlocks_;
    bool hasTryBlock_;

  public:
    explicit MIRGraph(TempAllocator* alloc)
      : alloc_(alloc),
        returnAccumulator_(nullptr),
        blockIdGen_(0),
        idGen_(0),
        osrBlock_(nullptr),
        numBlocks_(0),
        hasTryBlock_(false)
    { }

    TempAllocator& alloc() const {
        return *alloc_;
    }

    void addBlock(MBasicBlock* block);
    void insertBlockAfter(MBasicBlock* at, MBasicBlock* block);
    void insertBlockBefore(MBasicBlock* at, MBasicBlock* block);

    void renumberBlocksAfter(MBasicBlock* at);

    void unmarkBlocks();

    void setReturnAccumulator(MIRGraphReturns* accum) {
        returnAccumulator_ = accum;
    }
    MIRGraphReturns* returnAccumulator() const {
        return returnAccumulator_;
    }

    bool addReturn(MBasicBlock* returnBlock) {
        if (!returnAccumulator_)
            return true;

        return returnAccumulator_->append(returnBlock);
    }

    MBasicBlock* entryBlock() {
        return *blocks_.begin();
    }
    MBasicBlockIterator begin() {
        return blocks_.begin();
    }
    MBasicBlockIterator begin(MBasicBlock* at) {
        return blocks_.begin(at);
    }
    MBasicBlockIterator end() {
        return blocks_.end();
    }
    PostorderIterator poBegin() {
        return blocks_.rbegin();
    }
    PostorderIterator poBegin(MBasicBlock* at) {
        return blocks_.rbegin(at);
    }
    PostorderIterator poEnd() {
        return blocks_.rend();
    }
    ReversePostorderIterator rpoBegin() {
        return blocks_.begin();
    }
    ReversePostorderIterator rpoBegin(MBasicBlock* at) {
        return blocks_.begin(at);
    }
    ReversePostorderIterator rpoEnd() {
        return blocks_.end();
    }
    void removeBlocksAfter(MBasicBlock* block);
    void removeBlock(MBasicBlock* block);
    void removeBlockIncludingPhis(MBasicBlock* block);
    void moveBlockToEnd(MBasicBlock* block) {
        MOZ_ASSERT(block->id());
        blocks_.remove(block);
        blocks_.pushBack(block);
    }
    void moveBlockBefore(MBasicBlock* at, MBasicBlock* block) {
        MOZ_ASSERT(block->id());
        blocks_.remove(block);
        blocks_.insertBefore(at, block);
    }
    size_t numBlocks() const {
        return numBlocks_;
    }
    uint32_t numBlockIds() const {
        return blockIdGen_;
    }
    void allocDefinitionId(MDefinition* ins) {
        ins->setId(idGen_++);
    }
    uint32_t getNumInstructionIds() {
        return idGen_;
    }
    MResumePoint* entryResumePoint() {
        return entryBlock()->entryResumePoint();
    }

    void copyIds(const MIRGraph& other) {
        idGen_ = other.idGen_;
        blockIdGen_ = other.blockIdGen_;
        numBlocks_ = other.numBlocks_;
    }

    void setOsrBlock(MBasicBlock* osrBlock) {
        MOZ_ASSERT(!osrBlock_);
        osrBlock_ = osrBlock;
    }
    MBasicBlock* osrBlock() {
        return osrBlock_;
    }

    bool hasTryBlock() const {
        return hasTryBlock_;
    }
    void setHasTryBlock() {
        hasTryBlock_ = true;
    }

    void dump(FILE* fp);
    void dump();
};

class MDefinitionIterator
{
  friend class MBasicBlock;
  friend class MNodeIterator;

  private:
    MBasicBlock* block_;
    MPhiIterator phiIter_;
    MInstructionIterator iter_;

    bool atPhi() const {
        return phiIter_ != block_->phisEnd();
    }

    MDefinition* getIns() {
        if (atPhi())
            return *phiIter_;
        return *iter_;
    }

    bool more() const {
        return atPhi() || (*iter_) != block_->lastIns();
    }

  public:
    explicit MDefinitionIterator(MBasicBlock* block)
      : block_(block),
        phiIter_(block->phisBegin()),
        iter_(block->begin())
    { }

    MDefinitionIterator operator ++() {
        MOZ_ASSERT(more());
        if (atPhi())
            ++phiIter_;
        else
            ++iter_;
        return *this;
    }

    MDefinitionIterator operator ++(int) {
        MDefinitionIterator old(*this);
        operator++ ();
        return old;
    }

    explicit operator bool() const {
        return more();
    }

    MDefinition* operator*() {
        return getIns();
    }

    MDefinition* operator ->() {
        return getIns();
    }
};




class MNodeIterator
{
  private:
    
    
    
    MInstruction* last_;

    
    
    
    MDefinitionIterator defIter_;

    MBasicBlock* block() const {
        return defIter_.block_;
    }

    bool atResumePoint() const {
        return last_ && !last_->isDiscarded();
    }

    MNode* getNode() {
        if (!atResumePoint())
            return *defIter_;

        
        
        
        
        if (last_ != block()->lastIns())
            return last_->resumePoint();
        return block()->entryResumePoint();
    }

    void next() {
        if (!atResumePoint()) {
            if (defIter_->isInstruction() && defIter_->toInstruction()->resumePoint()) {
                
                MOZ_ASSERT(*defIter_ != block()->lastIns());
                last_ = defIter_->toInstruction();
            }

            defIter_++;
        } else {
            last_ = nullptr;
        }
    }

    bool more() const {
        return defIter_ || atResumePoint();
    }

  public:
    explicit MNodeIterator(MBasicBlock* block)
      : last_(block->entryResumePoint() ? block->lastIns() : nullptr),
        defIter_(block)
    {
        MOZ_ASSERT(bool(block->entryResumePoint()) == atResumePoint());

        
        
        
        MOZ_ASSERT(!block->lastIns()->resumePoint());
    }

    MNodeIterator operator ++(int) {
        MNodeIterator old(*this);
        if (more())
            next();
        return old;
    }

    explicit operator bool() const {
        return more();
    }

    MNode* operator*() {
        return getNode();
    }

    MNode* operator ->() {
        return getNode();
    }

};

} 
} 

#endif 
