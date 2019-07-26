





#include "jit/MIRGraph.h"

#include "jit/AsmJS.h"
#include "jit/BytecodeAnalysis.h"
#include "jit/Ion.h"
#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"

using namespace js;
using namespace js::jit;

MIRGenerator::MIRGenerator(JSCompartment *compartment,
                           TempAllocator *alloc, MIRGraph *graph, CompileInfo *info)
  : compartment(compartment),
    info_(info),
    alloc_(alloc),
    graph_(graph),
    error_(false),
    cancelBuild_(0),
    maxAsmJSStackArgBytes_(0),
    performsAsmJSCall_(false),
    minAsmJSHeapLength_(AsmJSAllocationGranularity)
{ }

bool
MIRGenerator::abortFmt(const char *message, va_list ap)
{
    IonSpewVA(IonSpew_Abort, message, ap);
    error_ = true;
    return false;
}

bool
MIRGenerator::abort(const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    abortFmt(message, ap);
    va_end(ap);
    return false;
}

void
MIRGraph::addBlock(MBasicBlock *block)
{
    JS_ASSERT(block);
    block->setId(blockIdGen_++);
    blocks_.pushBack(block);
    numBlocks_++;
}

void
MIRGraph::insertBlockAfter(MBasicBlock *at, MBasicBlock *block)
{
    block->setId(blockIdGen_++);
    blocks_.insertAfter(at, block);
    numBlocks_++;
}

void
MIRGraph::removeBlocksAfter(MBasicBlock *start)
{
    MBasicBlockIterator iter(begin());
    iter++;
    while (iter != end()) {
        MBasicBlock *block = *iter;
        iter++;

        if (block->id() <= start->id())
            continue;

        
        
        block->discardAllResumePoints();
        removeBlock(block);
    }
}

void
MIRGraph::removeBlock(MBasicBlock *block)
{
    
    
    

    if (block == osrBlock_)
        osrBlock_ = nullptr;

    if (exitAccumulator_) {
        size_t i = 0;
        while (i < exitAccumulator_->length()) {
            if ((*exitAccumulator_)[i] == block)
                exitAccumulator_->erase(exitAccumulator_->begin() + i);
            else
                i++;
        }
    }

    block->discardAllInstructions();

    
    
    
    
    block->discardAllPhiOperands();

    block->markAsDead();
    blocks_.remove(block);
    numBlocks_--;
}

void
MIRGraph::unmarkBlocks()
{
    for (MBasicBlockIterator i(blocks_.begin()); i != blocks_.end(); i++)
        i->unmark();
}

MDefinition *
MIRGraph::forkJoinSlice()
{
    
    
    
    
    
    
    
    
    
    

    MBasicBlock *entry = entryBlock();
    JS_ASSERT(entry->info().executionMode() == ParallelExecution);

    MInstruction *start = nullptr;
    for (MInstructionIterator ins(entry->begin()); ins != entry->end(); ins++) {
        if (ins->isForkJoinSlice())
            return *ins;
        else if (ins->isStart())
            start = *ins;
    }
    JS_ASSERT(start);

    MForkJoinSlice *slice = MForkJoinSlice::New(alloc());
    entry->insertAfter(start, slice);
    return slice;
}

MBasicBlock *
MBasicBlock::New(MIRGraph &graph, BytecodeAnalysis *analysis, CompileInfo &info,
                 MBasicBlock *pred, jsbytecode *entryPc, Kind kind)
{
    JS_ASSERT(entryPc != nullptr);

    MBasicBlock *block = new(graph.alloc()) MBasicBlock(graph, info, entryPc, kind);
    if (!block->init())
        return nullptr;

    if (!block->inherit(graph.alloc(), analysis, pred, 0))
        return nullptr;

    return block;
}

MBasicBlock *
MBasicBlock::NewPopN(MIRGraph &graph, CompileInfo &info,
                     MBasicBlock *pred, jsbytecode *entryPc, Kind kind, uint32_t popped)
{
    MBasicBlock *block = new(graph.alloc()) MBasicBlock(graph, info, entryPc, kind);
    if (!block->init())
        return nullptr;

    if (!block->inherit(graph.alloc(), nullptr, pred, popped))
        return nullptr;

    return block;
}

MBasicBlock *
MBasicBlock::NewWithResumePoint(MIRGraph &graph, CompileInfo &info,
                                MBasicBlock *pred, jsbytecode *entryPc,
                                MResumePoint *resumePoint)
{
    MBasicBlock *block = new(graph.alloc()) MBasicBlock(graph, info, entryPc, NORMAL);

    resumePoint->block_ = block;
    block->entryResumePoint_ = resumePoint;

    if (!block->init())
        return nullptr;

    if (!block->inheritResumePoint(pred))
        return nullptr;

    return block;
}

MBasicBlock *
MBasicBlock::NewPendingLoopHeader(MIRGraph &graph, CompileInfo &info,
                                  MBasicBlock *pred, jsbytecode *entryPc)
{
    return MBasicBlock::New(graph, nullptr, info, pred, entryPc, PENDING_LOOP_HEADER);
}

MBasicBlock *
MBasicBlock::NewSplitEdge(MIRGraph &graph, CompileInfo &info, MBasicBlock *pred)
{
    return pred->pc()
           ? MBasicBlock::New(graph, nullptr, info, pred, pred->pc(), SPLIT_EDGE)
           : MBasicBlock::NewAsmJS(graph, info, pred, SPLIT_EDGE);
}

MBasicBlock *
MBasicBlock::NewAbortPar(MIRGraph &graph, CompileInfo &info,
                         MBasicBlock *pred, jsbytecode *entryPc,
                         MResumePoint *resumePoint)
{
    MBasicBlock *block = new(graph.alloc()) MBasicBlock(graph, info, entryPc, NORMAL);

    resumePoint->block_ = block;
    block->entryResumePoint_ = resumePoint;

    if (!block->init())
        return nullptr;

    if (!block->addPredecessorWithoutPhis(pred))
        return nullptr;

    block->end(MAbortPar::New(graph.alloc()));
    return block;
}

MBasicBlock *
MBasicBlock::NewAsmJS(MIRGraph &graph, CompileInfo &info, MBasicBlock *pred, Kind kind)
{
    MBasicBlock *block = new(graph.alloc()) MBasicBlock(graph, info,  nullptr, kind);
    if (!block->init())
        return nullptr;

    if (pred) {
        block->stackPosition_ = pred->stackPosition_;

        if (block->kind_ == PENDING_LOOP_HEADER) {
            for (size_t i = 0; i < block->stackPosition_; i++) {
                MDefinition *predSlot = pred->getSlot(i);

                JS_ASSERT(predSlot->type() != MIRType_Value);
                MPhi *phi = MPhi::New(graph.alloc(), i, predSlot->type());

                JS_ALWAYS_TRUE(phi->reserveLength(2));
                phi->addInput(predSlot);

                block->addPhi(phi);
                block->setSlot(i, phi);
            }
        } else {
            block->copySlots(pred);
        }

        if (!block->predecessors_.append(pred))
            return nullptr;
    }

    return block;
}

MBasicBlock::MBasicBlock(MIRGraph &graph, CompileInfo &info, jsbytecode *pc, Kind kind)
    : earlyAbort_(false),
    graph_(graph),
    info_(info),
    stackPosition_(info_.firstStackSlot()),
    lastIns_(nullptr),
    pc_(pc),
    lir_(nullptr),
    start_(nullptr),
    entryResumePoint_(nullptr),
    successorWithPhis_(nullptr),
    positionInPhiSuccessor_(0),
    kind_(kind),
    loopDepth_(0),
    mark_(false),
    immediateDominator_(nullptr),
    numDominated_(0),
    loopHeader_(nullptr),
    trackedPc_(pc)
#if defined (JS_ION_PERF)
    , lineno_(0u),
    columnIndex_(0u)
#endif
{
}

bool
MBasicBlock::init()
{
    return slots_.init(info_.nslots());
}

bool
MBasicBlock::increaseSlots(size_t num)
{
    return slots_.growBy(num);
}

void
MBasicBlock::copySlots(MBasicBlock *from)
{
    JS_ASSERT(stackPosition_ <= from->stackPosition_);

    for (uint32_t i = 0; i < stackPosition_; i++)
        slots_[i] = from->slots_[i];
}

bool
MBasicBlock::inherit(TempAllocator &alloc, BytecodeAnalysis *analysis, MBasicBlock *pred,
                     uint32_t popped)
{
    if (pred) {
        stackPosition_ = pred->stackPosition_;
        JS_ASSERT(stackPosition_ >= popped);
        stackPosition_ -= popped;
        if (kind_ != PENDING_LOOP_HEADER)
            copySlots(pred);
    } else {
        uint32_t stackDepth = analysis->info(pc()).stackDepth;
        stackPosition_ = info().firstStackSlot() + stackDepth;
        JS_ASSERT(stackPosition_ >= popped);
        stackPosition_ -= popped;
    }

    JS_ASSERT(info_.nslots() >= stackPosition_);
    JS_ASSERT(!entryResumePoint_);

    
    MResumePoint *callerResumePoint = pred ? pred->callerResumePoint() : nullptr;

    
    entryResumePoint_ = new(alloc) MResumePoint(this, pc(), callerResumePoint, MResumePoint::ResumeAt);
    if (!entryResumePoint_->init())
        return false;

    if (pred) {
        if (!predecessors_.append(pred))
            return false;

        if (kind_ == PENDING_LOOP_HEADER) {
            for (size_t i = 0; i < stackDepth(); i++) {
                MPhi *phi = MPhi::New(alloc, i);
                if (!phi->addInputSlow(pred->getSlot(i)))
                    return false;
                addPhi(phi);
                setSlot(i, phi);
                entryResumePoint()->setOperand(i, phi);
            }
        } else {
            for (size_t i = 0; i < stackDepth(); i++)
                entryResumePoint()->setOperand(i, getSlot(i));
        }
    } else {
        



        for (size_t i = 0; i < stackDepth(); i++)
            entryResumePoint()->clearOperand(i);
    }

    return true;
}

bool
MBasicBlock::inheritResumePoint(MBasicBlock *pred)
{
    
    stackPosition_ = entryResumePoint_->numOperands();
    for (uint32_t i = 0; i < stackPosition_; i++)
        slots_[i] = entryResumePoint_->getOperand(i);

    JS_ASSERT(info_.nslots() >= stackPosition_);
    JS_ASSERT(kind_ != PENDING_LOOP_HEADER);
    JS_ASSERT(pred != nullptr);

    if (!predecessors_.append(pred))
        return false;

    return true;
}

void
MBasicBlock::inheritSlots(MBasicBlock *parent)
{
    stackPosition_ = parent->stackPosition_;
    copySlots(parent);
}

bool
MBasicBlock::initEntrySlots(TempAllocator &alloc)
{
    
    entryResumePoint_ = MResumePoint::New(alloc, this, pc(), callerResumePoint(),
                                          MResumePoint::ResumeAt);
    if (!entryResumePoint_)
        return false;
    return true;
}

MDefinition *
MBasicBlock::getSlot(uint32_t index)
{
    JS_ASSERT(index < stackPosition_);
    return slots_[index];
}

void
MBasicBlock::initSlot(uint32_t slot, MDefinition *ins)
{
    slots_[slot] = ins;
    if (entryResumePoint())
        entryResumePoint()->setOperand(slot, ins);
}

void
MBasicBlock::shimmySlots(int discardDepth)
{
    
    

    JS_ASSERT(discardDepth < 0);
    JS_ASSERT(stackPosition_ + discardDepth >= info_.firstStackSlot());

    for (int i = discardDepth; i < -1; i++)
        slots_[stackPosition_ + i] = slots_[stackPosition_ + i + 1];

    --stackPosition_;
}

void
MBasicBlock::linkOsrValues(MStart *start)
{
    JS_ASSERT(start->startType() == MStart::StartType_Osr);

    MResumePoint *res = start->resumePoint();

    for (uint32_t i = 0; i < stackDepth(); i++) {
        MDefinition *def = slots_[i];
        if (i == info().scopeChainSlot()) {
            if (def->isOsrScopeChain())
                def->toOsrScopeChain()->setResumePoint(res);
        } else if (i == info().returnValueSlot()) {
            if (def->isOsrReturnValue())
                def->toOsrReturnValue()->setResumePoint(res);
        } else if (info().hasArguments() && i == info().argsObjSlot()) {
            JS_ASSERT(def->isConstant() || def->isOsrArgumentsObject());
            JS_ASSERT_IF(def->isConstant(), def->toConstant()->value() == UndefinedValue());
            if (def->isOsrArgumentsObject())
                def->toOsrArgumentsObject()->setResumePoint(res);
        } else {
            JS_ASSERT(def->isOsrValue() || def->isGetArgumentsObjectArg() || def->isConstant() ||
                      def->isParameter());

            
            
            JS_ASSERT_IF(def->isConstant(), def->toConstant()->value() == UndefinedValue());

            if (def->isOsrValue())
                def->toOsrValue()->setResumePoint(res);
            else if (def->isGetArgumentsObjectArg())
                def->toGetArgumentsObjectArg()->setResumePoint(res);
            else if (def->isParameter())
                def->toParameter()->setResumePoint(res);
        }
    }
}

void
MBasicBlock::setSlot(uint32_t slot, MDefinition *ins)
{
    slots_[slot] = ins;
}

void
MBasicBlock::setVariable(uint32_t index)
{
    JS_ASSERT(stackPosition_ > info_.firstStackSlot());
    setSlot(index, slots_[stackPosition_ - 1]);
}

void
MBasicBlock::setArg(uint32_t arg)
{
    setVariable(info_.argSlot(arg));
}

void
MBasicBlock::setLocal(uint32_t local)
{
    setVariable(info_.localSlot(local));
}

void
MBasicBlock::setSlot(uint32_t slot)
{
    setVariable(slot);
}

void
MBasicBlock::rewriteSlot(uint32_t slot, MDefinition *ins)
{
    setSlot(slot, ins);
}

void
MBasicBlock::rewriteAtDepth(int32_t depth, MDefinition *ins)
{
    JS_ASSERT(depth < 0);
    JS_ASSERT(stackPosition_ + depth >= info_.firstStackSlot());
    rewriteSlot(stackPosition_ + depth, ins);
}

void
MBasicBlock::push(MDefinition *ins)
{
    JS_ASSERT(stackPosition_ < nslots());
    slots_[stackPosition_++] = ins;
}

void
MBasicBlock::pushVariable(uint32_t slot)
{
    push(slots_[slot]);
}

void
MBasicBlock::pushArg(uint32_t arg)
{
    pushVariable(info_.argSlot(arg));
}

void
MBasicBlock::pushLocal(uint32_t local)
{
    pushVariable(info_.localSlot(local));
}

void
MBasicBlock::pushSlot(uint32_t slot)
{
    pushVariable(slot);
}

MDefinition *
MBasicBlock::pop()
{
    JS_ASSERT(stackPosition_ > info_.firstStackSlot());
    return slots_[--stackPosition_];
}

void
MBasicBlock::popn(uint32_t n)
{
    JS_ASSERT(stackPosition_ - n >= info_.firstStackSlot());
    JS_ASSERT(stackPosition_ >= stackPosition_ - n);
    stackPosition_ -= n;
}

MDefinition *
MBasicBlock::scopeChain()
{
    return getSlot(info().scopeChainSlot());
}

MDefinition *
MBasicBlock::argumentsObject()
{
    return getSlot(info().argsObjSlot());
}

void
MBasicBlock::setScopeChain(MDefinition *scopeObj)
{
    setSlot(info().scopeChainSlot(), scopeObj);
}

void
MBasicBlock::setArgumentsObject(MDefinition *argsObj)
{
    setSlot(info().argsObjSlot(), argsObj);
}

void
MBasicBlock::pick(int32_t depth)
{
    
    
    
    
    
    for (; depth < 0; depth++)
        swapAt(depth);
}

void
MBasicBlock::swapAt(int32_t depth)
{
    uint32_t lhsDepth = stackPosition_ + depth - 1;
    uint32_t rhsDepth = stackPosition_ + depth;

    MDefinition *temp = slots_[lhsDepth];
    slots_[lhsDepth] = slots_[rhsDepth];
    slots_[rhsDepth] = temp;
}

MDefinition *
MBasicBlock::peek(int32_t depth)
{
    JS_ASSERT(depth < 0);
    JS_ASSERT(stackPosition_ + depth >= info_.firstStackSlot());
    return getSlot(stackPosition_ + depth);
}

void
MBasicBlock::discardLastIns()
{
    JS_ASSERT(lastIns_);
    discard(lastIns_);
    lastIns_ = nullptr;
}

void
MBasicBlock::addFromElsewhere(MInstruction *ins)
{
    JS_ASSERT(ins->block() != this);

    
    ins->block()->instructions_.remove(ins);

    
    add(ins);
}

void
MBasicBlock::moveBefore(MInstruction *at, MInstruction *ins)
{
    
    JS_ASSERT(ins->block() == this);
    instructions_.remove(ins);

    
    
    at->block()->insertBefore(at, ins);
}

static inline void
AssertSafelyDiscardable(MDefinition *def)
{
#ifdef DEBUG
    
    
    JS_ASSERT(!def->hasUses());
#endif
}

void
MBasicBlock::discard(MInstruction *ins)
{
    AssertSafelyDiscardable(ins);
    for (size_t i = 0, e = ins->numOperands(); i < e; i++)
        ins->discardOperand(i);

    instructions_.remove(ins);
}

MInstructionIterator
MBasicBlock::discardAt(MInstructionIterator &iter)
{
    AssertSafelyDiscardable(*iter);
    for (size_t i = 0, e = iter->numOperands(); i < e; i++)
        iter->discardOperand(i);

    return instructions_.removeAt(iter);
}

MInstructionReverseIterator
MBasicBlock::discardAt(MInstructionReverseIterator &iter)
{
    AssertSafelyDiscardable(*iter);
    for (size_t i = 0, e = iter->numOperands(); i < e; i++)
        iter->discardOperand(i);

    return instructions_.removeAt(iter);
}

MDefinitionIterator
MBasicBlock::discardDefAt(MDefinitionIterator &old)
{
    MDefinitionIterator iter(old);

    if (iter.atPhi())
        iter.phiIter_ = iter.block_->discardPhiAt(iter.phiIter_);
    else
        iter.iter_ = iter.block_->discardAt(iter.iter_);

    return iter;
}

void
MBasicBlock::discardAllInstructions()
{
    for (MInstructionIterator iter = begin(); iter != end(); ) {
        for (size_t i = 0, e = iter->numOperands(); i < e; i++)
            iter->discardOperand(i);
        iter = instructions_.removeAt(iter);
    }
    lastIns_ = nullptr;
}

void
MBasicBlock::discardAllPhiOperands()
{
    for (MPhiIterator iter = phisBegin(); iter != phisEnd(); iter++) {
        MPhi *phi = *iter;
        for (size_t i = 0, e = phi->numOperands(); i < e; i++)
            phi->discardOperand(i);
    }

    for (MBasicBlock **pred = predecessors_.begin(); pred != predecessors_.end(); pred++)
        (*pred)->setSuccessorWithPhis(nullptr, 0);
}

void
MBasicBlock::discardAllPhis()
{
    discardAllPhiOperands();
    phis_.clear();
}

void
MBasicBlock::discardAllResumePoints(bool discardEntry)
{
    for (MResumePointIterator iter = resumePointsBegin(); iter != resumePointsEnd(); ) {
        MResumePoint *rp = *iter;
        if (rp == entryResumePoint() && !discardEntry) {
            iter++;
        } else {
            rp->discardUses();
            iter = resumePoints_.removeAt(iter);
        }
    }
}

void
MBasicBlock::insertBefore(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    graph().allocDefinitionId(ins);
    instructions_.insertBefore(at, ins);
    ins->setTrackedPc(at->trackedPc());
}

void
MBasicBlock::insertAfter(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    graph().allocDefinitionId(ins);
    instructions_.insertAfter(at, ins);
    ins->setTrackedPc(at->trackedPc());
}

void
MBasicBlock::add(MInstruction *ins)
{
    JS_ASSERT(!lastIns_);
    ins->setBlock(this);
    graph().allocDefinitionId(ins);
    instructions_.pushBack(ins);
    ins->setTrackedPc(trackedPc_);
}

void
MBasicBlock::end(MControlInstruction *ins)
{
    JS_ASSERT(!lastIns_); 
    JS_ASSERT(ins);
    add(ins);
    lastIns_ = ins;
}

void
MBasicBlock::addPhi(MPhi *phi)
{
    phis_.pushBack(phi);
    phi->setBlock(this);
    graph().allocDefinitionId(phi);
}

MPhiIterator
MBasicBlock::discardPhiAt(MPhiIterator &at)
{
    JS_ASSERT(!phis_.empty());

    for (size_t i = 0, e = at->numOperands(); i < e; i++)
        at->discardOperand(i);

    MPhiIterator result = phis_.removeAt(at);

    if (phis_.empty()) {
        for (MBasicBlock **pred = predecessors_.begin(); pred != predecessors_.end(); pred++)
            (*pred)->setSuccessorWithPhis(nullptr, 0);
    }
    return result;
}

bool
MBasicBlock::addPredecessor(TempAllocator &alloc, MBasicBlock *pred)
{
    return addPredecessorPopN(alloc, pred, 0);
}

bool
MBasicBlock::addPredecessorPopN(TempAllocator &alloc, MBasicBlock *pred, uint32_t popped)
{
    JS_ASSERT(pred);
    JS_ASSERT(predecessors_.length() > 0);

    
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackPosition_ == stackPosition_ + popped);

    for (uint32_t i = 0; i < stackPosition_; i++) {
        MDefinition *mine = getSlot(i);
        MDefinition *other = pred->getSlot(i);

        if (mine != other) {
            
            
            
            if (mine->isPhi() && mine->block() == this) {
                JS_ASSERT(predecessors_.length());
                if (!mine->toPhi()->addInputSlow(other))
                    return false;
            } else {
                
                MPhi *phi;
                if (mine->type() == other->type())
                    phi = MPhi::New(alloc, i, mine->type());
                else
                    phi = MPhi::New(alloc, i);
                addPhi(phi);

                
                
                if (!phi->reserveLength(predecessors_.length() + 1))
                    return false;

                for (size_t j = 0; j < predecessors_.length(); j++) {
                    JS_ASSERT(predecessors_[j]->getSlot(i) == mine);
                    phi->addInput(mine);
                }
                phi->addInput(other);

                setSlot(i, phi);
                if (entryResumePoint())
                    entryResumePoint()->replaceOperand(i, phi);
            }
        }
    }

    return predecessors_.append(pred);
}

bool
MBasicBlock::addPredecessorWithoutPhis(MBasicBlock *pred)
{
    
    JS_ASSERT(pred && pred->lastIns_);
    return predecessors_.append(pred);
}

bool
MBasicBlock::addImmediatelyDominatedBlock(MBasicBlock *child)
{
    return immediatelyDominated_.append(child);
}

void
MBasicBlock::assertUsesAreNotWithin(MUseIterator use, MUseIterator end)
{
#ifdef DEBUG
    for (; use != end; use++) {
        JS_ASSERT_IF(use->consumer()->isDefinition(),
                     use->consumer()->toDefinition()->block()->id() < id());
    }
#endif
}

bool
MBasicBlock::dominates(MBasicBlock *other)
{
    uint32_t high = domIndex() + numDominated();
    uint32_t low  = domIndex();
    return other->domIndex() >= low && other->domIndex() <= high;
}

AbortReason
MBasicBlock::setBackedge(MBasicBlock *pred)
{
    
    JS_ASSERT(lastIns_);
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackDepth() == entryResumePoint()->stackDepth());

    
    JS_ASSERT(kind_ == PENDING_LOOP_HEADER);

    bool hadTypeChange = false;

    
    for (MPhiIterator phi = phisBegin(); phi != phisEnd(); phi++) {
        MPhi *entryDef = *phi;
        MDefinition *exitDef = pred->slots_[entryDef->slot()];

        
        JS_ASSERT(entryDef->block() == this);

        if (entryDef == exitDef) {
            
            
            
            
            
            
            
            exitDef = entryDef->getOperand(0);
        }

        bool typeChange = false;

        if (!entryDef->addInputSlow(exitDef, &typeChange))
            return AbortReason_Alloc;

        hadTypeChange |= typeChange;

        JS_ASSERT(entryDef->slot() < pred->stackDepth());
        setSlot(entryDef->slot(), entryDef);
    }

    if (hadTypeChange) {
        for (MPhiIterator phi = phisBegin(); phi != phisEnd(); phi++)
            phi->removeOperand(phi->numOperands() - 1);
        return AbortReason_Disable;
    }

    
    kind_ = LOOP_HEADER;

    if (!predecessors_.append(pred))
        return AbortReason_Alloc;

    return AbortReason_NoAbort;
}

bool
MBasicBlock::setBackedgeAsmJS(MBasicBlock *pred)
{
    
    JS_ASSERT(lastIns_);
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(stackDepth() == pred->stackDepth());

    
    JS_ASSERT(kind_ == PENDING_LOOP_HEADER);

    
    for (MPhiIterator phi = phisBegin(); phi != phisEnd(); phi++) {
        MPhi *entryDef = *phi;
        MDefinition *exitDef = pred->getSlot(entryDef->slot());

        
        JS_ASSERT(entryDef->block() == this);

        
        JS_ASSERT(entryDef->type() == exitDef->type());
        JS_ASSERT(entryDef->type() != MIRType_Value);

        if (entryDef == exitDef) {
            
            
            
            
            
            
            
            exitDef = entryDef->getOperand(0);
        }

        
        entryDef->addInput(exitDef);

        JS_ASSERT(entryDef->slot() < pred->stackDepth());
        setSlot(entryDef->slot(), entryDef);
    }

    
    kind_ = LOOP_HEADER;

    return predecessors_.append(pred);
}

void
MBasicBlock::clearLoopHeader()
{
    JS_ASSERT(isLoopHeader());
    kind_ = NORMAL;
}

size_t
MBasicBlock::numSuccessors() const
{
    JS_ASSERT(lastIns());
    return lastIns()->numSuccessors();
}

MBasicBlock *
MBasicBlock::getSuccessor(size_t index) const
{
    JS_ASSERT(lastIns());
    return lastIns()->getSuccessor(index);
}

size_t
MBasicBlock::getSuccessorIndex(MBasicBlock *block) const
{
    JS_ASSERT(lastIns());
    for (size_t i = 0; i < numSuccessors(); i++) {
        if (getSuccessor(i) == block)
            return i;
    }
    MOZ_ASSUME_UNREACHABLE("Invalid successor");
}

void
MBasicBlock::replaceSuccessor(size_t pos, MBasicBlock *split)
{
    JS_ASSERT(lastIns());

    
    
    JS_ASSERT_IF(successorWithPhis_, successorWithPhis_ != getSuccessor(pos));

    lastIns()->replaceSuccessor(pos, split);
}

void
MBasicBlock::replacePredecessor(MBasicBlock *old, MBasicBlock *split)
{
    for (size_t i = 0; i < numPredecessors(); i++) {
        if (getPredecessor(i) == old) {
            predecessors_[i] = split;

#ifdef DEBUG
            
            for (size_t j = i; j < numPredecessors(); j++)
                JS_ASSERT(predecessors_[j] != old);
#endif

            return;
        }
    }

    MOZ_ASSUME_UNREACHABLE("predecessor was not found");
}

void
MBasicBlock::clearDominatorInfo()
{
    setImmediateDominator(nullptr);
    immediatelyDominated_.clear();
    numDominated_ = 0;
}

void
MBasicBlock::removePredecessor(MBasicBlock *pred)
{
    JS_ASSERT(numPredecessors() >= 2);

    for (size_t i = 0; i < numPredecessors(); i++) {
        if (getPredecessor(i) != pred)
            continue;

        
        
        if (!phisEmpty()) {
            JS_ASSERT(pred->successorWithPhis());
            JS_ASSERT(pred->positionInPhiSuccessor() == i);
            for (MPhiIterator iter = phisBegin(); iter != phisEnd(); iter++)
                iter->removeOperand(i);
            for (size_t j = i+1; j < numPredecessors(); j++)
                getPredecessor(j)->setSuccessorWithPhis(this, j - 1);
        }

        
        MBasicBlock **ptr = predecessors_.begin() + i;
        predecessors_.erase(ptr);
        return;
    }

    MOZ_ASSUME_UNREACHABLE("predecessor was not found");
}

void
MBasicBlock::inheritPhis(MBasicBlock *header)
{
    for (MPhiIterator iter = header->phisBegin(); iter != header->phisEnd(); iter++) {
        MPhi *phi = *iter;
        JS_ASSERT(phi->numOperands() == 2);

        
        MDefinition *entryDef = phi->getOperand(0);
        MDefinition *exitDef = getSlot(phi->slot());

        if (entryDef != exitDef)
            continue;

        
        
        
        setSlot(phi->slot(), phi);
    }
}

void
MBasicBlock::specializePhis()
{
    for (MPhiIterator iter = phisBegin(); iter != phisEnd(); iter++) {
        MPhi *phi = *iter;
        phi->specializeType();
    }
}

void
MBasicBlock::dumpStack(FILE *fp)
{
#ifdef DEBUG
    fprintf(fp, " %-3s %-16s %-6s %-10s\n", "#", "name", "copyOf", "first/next");
    fprintf(fp, "-------------------------------------------\n");
    for (uint32_t i = 0; i < stackPosition_; i++) {
        fprintf(fp, " %-3d", i);
        fprintf(fp, " %-16p\n", (void *)slots_[i]);
    }
#endif
}

MTest *
MBasicBlock::immediateDominatorBranch(BranchDirection *pdirection)
{
    *pdirection = FALSE_BRANCH;

    if (numPredecessors() != 1)
        return nullptr;

    MBasicBlock *dom = immediateDominator();
    if (dom != getPredecessor(0))
        return nullptr;

    
    MInstruction *ins = dom->lastIns();
    if (ins->isTest()) {
        MTest *test = ins->toTest();

        JS_ASSERT(test->ifTrue() == this || test->ifFalse() == this);
        if (test->ifTrue() == this && test->ifFalse() == this)
            return nullptr;

        *pdirection = (test->ifTrue() == this) ? TRUE_BRANCH : FALSE_BRANCH;
        return test;
    }

    return nullptr;
}

void
MIRGraph::dump(FILE *fp)
{
#ifdef DEBUG
    for (MBasicBlockIterator iter(begin()); iter != end(); iter++) {
        fprintf(fp, "block%d:\n", iter->id());
        iter->dump(fp);
        fprintf(fp, "\n");
    }
#endif
}

void
MBasicBlock::dump(FILE *fp)
{
#ifdef DEBUG
    for (MPhiIterator iter(phisBegin()); iter != phisEnd(); iter++) {
        iter->dump(fp);
    }
    for (MInstructionIterator iter(begin()); iter != end(); iter++) {
        iter->dump(fp);
    }
#endif
}
