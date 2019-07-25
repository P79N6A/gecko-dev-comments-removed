








































#include "Ion.h"
#include "IonSpewer.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "IonBuilder.h"
#include "frontend/BytecodeEmitter.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

MIRGenerator::MIRGenerator(JSContext *cx, TempAllocator &temp, MIRGraph &graph, CompileInfo &info)
  : cx(cx),
    info_(info),
    temp_(temp),
    graph_(graph),
    error_(false)
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
#ifdef DEBUG
    numBlocks_++;
#endif
}

void
MIRGraph::insertBlockAfter(MBasicBlock *at, MBasicBlock *block)
{
    block->setId(blockIdGen_++);
    blocks_.insertAfter(at, block);
#ifdef DEBUG
    numBlocks_++;
#endif
}

void
MIRGraph::unmarkBlocks() {
    for (MBasicBlockIterator i(blocks_.begin()); i != blocks_.end(); i++)
        i->unmark();
}

MBasicBlock *
MBasicBlock::New(MIRGraph &graph, CompileInfo &info,
                 MBasicBlock *pred, jsbytecode *entryPc, Kind kind)
{
    MBasicBlock *block = new MBasicBlock(graph, info, entryPc, kind);
    if (!block->init())
        return NULL;

    if (!block->inherit(pred))
        return NULL;

    return block;
}

MBasicBlock *
MBasicBlock::NewPendingLoopHeader(MIRGraph &graph, CompileInfo &info,
                                  MBasicBlock *pred, jsbytecode *entryPc)
{
    return MBasicBlock::New(graph, info, pred, entryPc, PENDING_LOOP_HEADER);
}

MBasicBlock *
MBasicBlock::NewSplitEdge(MIRGraph &graph, CompileInfo &info, MBasicBlock *pred)
{
    return MBasicBlock::New(graph, info, pred, pred->pc(), SPLIT_EDGE);
}

MBasicBlock::MBasicBlock(MIRGraph &graph, CompileInfo &info, jsbytecode *pc, Kind kind)
  : graph_(graph),
    info_(info),
    stackPosition_(info_.firstStackSlot()),
    lastIns_(NULL),
    pc_(pc),
    lir_(NULL),
    start_(NULL),
    successorWithPhis_(NULL),
    positionInPhiSuccessor_(0),
    kind_(kind),
    loopDepth_(0),
    mark_(false),
    immediateDominator_(NULL),
    numDominated_(0),
    loopHeader_(NULL)
#ifdef TRACK_SNAPSHOTS
  , trackedPc_(pc)
#endif
{
}

bool
MBasicBlock::init()
{
    if (!slots_.init(info_.nslots()))
        return false;
    return true;
}

void
MBasicBlock::copySlots(MBasicBlock *from)
{
    JS_ASSERT(stackPosition_ == from->stackPosition_);

    for (uint32 i = 0; i < stackPosition_; i++)
        slots_[i] = from->slots_[i];
}

bool
MBasicBlock::inherit(MBasicBlock *pred)
{
    if (pred) {
        stackPosition_ = pred->stackPosition_;
        if (kind_ != PENDING_LOOP_HEADER)
            copySlots(pred);
    } else {
        uint32_t stackDepth = info().script()->analysis()->getCode(pc()).stackDepth;
        stackPosition_ = info().firstStackSlot() + stackDepth;
    }

    JS_ASSERT(info_.nslots() >= stackPosition_);

    
    MResumePoint *callerResumePoint = pred ? pred->callerResumePoint() : NULL;

    
    entryResumePoint_ = new MResumePoint(this, pc(), callerResumePoint, MResumePoint::ResumeAt);
    if (!entryResumePoint_->init(this))
        return false;

    if (pred) {
        if (!predecessors_.append(pred))
            return false;

        if (kind_ == PENDING_LOOP_HEADER) {
            for (size_t i = 0; i < stackDepth(); i++) {
                MPhi *phi = MPhi::New(i);
                if (!phi->addInput(pred->getSlot(i)))
                    return NULL;
                addPhi(phi);
                setSlot(i, phi);
                entryResumePoint()->initOperand(i, phi);
            }
        } else {
            for (size_t i = 0; i < stackDepth(); i++)
                entryResumePoint()->initOperand(i, getSlot(i));
        }
    }

    return true;
}

bool
MBasicBlock::inheritNonPredecessor(MBasicBlock *parent)
{
    copySlots(parent);

    
    entryResumePoint_ = MResumePoint::New(this, pc(), callerResumePoint(), MResumePoint::ResumeAt);
    if (!entryResumePoint_)
        return false;
    return true;
}

MDefinition *
MBasicBlock::getSlot(uint32 index)
{
    JS_ASSERT(index < stackPosition_);
    return slots_[index];
}

void
MBasicBlock::initSlot(uint32 slot, MDefinition *ins)
{
    slots_[slot] = ins;
    entryResumePoint()->initOperand(slot, ins);
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

    for (uint32 i = 0; i < stackDepth(); i++) {
        MDefinition *def = slots_[i];
        if (i == info().scopeChainSlot())
            def->toOsrScopeChain()->setResumePoint(res);
        else
            def->toOsrValue()->setResumePoint(res);
    }
}

void
MBasicBlock::setSlot(uint32 slot, MDefinition *ins)
{
    slots_[slot] = ins;
}

void
MBasicBlock::setVariable(uint32 index)
{
    JS_ASSERT(stackPosition_ > info_.firstStackSlot());
    setSlot(index, slots_[stackPosition_ - 1]);
}

void
MBasicBlock::setArg(uint32 arg)
{
    
    setVariable(info_.argSlot(arg));
}

void
MBasicBlock::setLocal(uint32 local)
{
    
    setVariable(info_.localSlot(local));
}

void
MBasicBlock::setSlot(uint32 slot)
{
    setVariable(slot);
}

void
MBasicBlock::rewriteSlot(uint32 slot, MDefinition *ins)
{
    setSlot(slot, ins);
}

void
MBasicBlock::rewriteAtDepth(int32 depth, MDefinition *ins)
{
    JS_ASSERT(depth < 0);
    JS_ASSERT(stackPosition_ + depth >= info_.firstStackSlot());
    rewriteSlot(stackPosition_ + depth, ins);
}

void
MBasicBlock::push(MDefinition *ins)
{
    JS_ASSERT(stackPosition_ < info_.nslots());
    slots_[stackPosition_++] = ins;
}

void
MBasicBlock::pushVariable(uint32 slot)
{
    push(slots_[slot]);
}

void
MBasicBlock::pushArg(uint32 arg)
{
    
    pushVariable(info_.argSlot(arg));
}

void
MBasicBlock::pushLocal(uint32 local)
{
    
    pushVariable(info_.localSlot(local));
}

void
MBasicBlock::pushSlot(uint32 slot)
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
MBasicBlock::pick(int32 depth)
{
    
    
    
    
    
    for (; depth < 0; depth++)
        swapAt(depth);
}

void
MBasicBlock::swapAt(int32 depth)
{
    uint32 lhsDepth = stackPosition_ + depth - 1;
    uint32 rhsDepth = stackPosition_ + depth;

    MDefinition *temp = slots_[lhsDepth];
    slots_[lhsDepth] = slots_[rhsDepth];
    slots_[rhsDepth] = temp;
}

MDefinition *
MBasicBlock::peek(int32 depth)
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
    lastIns_ = NULL;
}

void
MBasicBlock::moveBefore(MInstruction *at, MInstruction *ins)
{
    
    JS_ASSERT(ins->block() == this);
    instructions_.remove(ins);

    
    
    at->block()->insertBefore(at, ins);
}

void
MBasicBlock::discard(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++)
        ins->replaceOperand(i, NULL);

    instructions_.remove(ins);
}

MInstructionIterator
MBasicBlock::discardAt(MInstructionIterator &iter)
{
    for (size_t i = 0; i < iter->numOperands(); i++)
        iter->replaceOperand(i, NULL);

    return instructions_.removeAt(iter);
}

MInstructionReverseIterator
MBasicBlock::discardAt(MInstructionReverseIterator &iter)
{
    for (size_t i = 0; i < iter->numOperands(); i++)
        iter->replaceOperand(i, NULL);

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
MBasicBlock::insertBefore(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    graph().allocDefinitionId(ins);
    instructions_.insertBefore(at, ins);
#ifdef TRACK_SNAPSHOTS
    ins->setTrackedPc(at->trackedPc());
#endif
}

void
MBasicBlock::insertAfter(MInstruction *at, MInstruction *ins)
{
    ins->setBlock(this);
    graph().allocDefinitionId(ins);
    instructions_.insertAfter(at, ins);
#ifdef TRACK_SNAPSHOTS
    ins->setTrackedPc(at->trackedPc());
#endif
}

void
MBasicBlock::add(MInstruction *ins)
{
    JS_ASSERT(!lastIns_);
    ins->setBlock(this);
    graph().allocDefinitionId(ins);
    instructions_.pushBack(ins);
#ifdef TRACK_SNAPSHOTS
    ins->setTrackedPc(trackedPc_);
#endif
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

    for (size_t i = 0; i < at->numOperands(); i++)
        at->replaceOperand(i, NULL);

    MPhiIterator result = phis_.removeAt(at);

    if (phis_.empty()) {
        for (MBasicBlock **pred = predecessors_.begin(); pred != predecessors_.end(); pred++)
            (*pred)->setSuccessorWithPhis(NULL, 0);
    }
    return result;
}

bool
MBasicBlock::addPredecessor(MBasicBlock *pred)
{
    JS_ASSERT(pred);
    JS_ASSERT(predecessors_.length() > 0);

    
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackPosition_ == stackPosition_);

    for (uint32 i = 0; i < stackPosition_; i++) {
        MDefinition *mine = getSlot(i);
        MDefinition *other = pred->getSlot(i);

        if (mine != other) {
            MPhi *phi;

            
            
            
            if (mine->isPhi() && mine->block() == this) {
                JS_ASSERT(predecessors_.length());
                phi = mine->toPhi();
            } else {
                
                phi = MPhi::New(i);
                addPhi(phi);

                
                
                for (size_t j = 0; j < predecessors_.length(); j++) {
                    JS_ASSERT(predecessors_[j]->getSlot(i) == mine);
                    if (!phi->addInput(mine))
                        return false;
                }

                setSlot(i, phi);
                entryResumePoint()->replaceOperand(i, phi);
            }

            if (!phi->addInput(other))
                return false;
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
        JS_ASSERT_IF(use->node()->isDefinition(),
                     use->node()->toDefinition()->block()->id() < id());
    }
#endif
}

bool
MBasicBlock::setBackedge(MBasicBlock *pred)
{
    
    JS_ASSERT(lastIns_);
    JS_ASSERT(pred->lastIns_);
    JS_ASSERT(pred->stackDepth() == entryResumePoint()->stackDepth());

    
    JS_ASSERT(kind_ == PENDING_LOOP_HEADER);

    
    for (uint32 i = 0; i < pred->stackDepth(); i++) {
        MPhi *entryDef = entryResumePoint()->getOperand(i)->toPhi();
        MDefinition *exitDef = pred->slots_[i];

        
        JS_ASSERT(entryDef->block() == this);

        if (entryDef == exitDef) {
            
            
            
            
            
            
            
            exitDef = entryDef->getOperand(0);
        }

        if (!entryDef->addInput(exitDef))
            return false;

        setSlot(i, entryDef);
    }

    
    kind_ = LOOP_HEADER;

    return predecessors_.append(pred);
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

void
MBasicBlock::replaceSuccessor(size_t pos, MBasicBlock *split)
{
    JS_ASSERT(lastIns());
    lastIns()->replaceSuccessor(pos, split);

    
    JS_ASSERT(!successorWithPhis_);
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
    JS_NOT_REACHED("predecessor was not found");
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
MBasicBlock::dumpStack(FILE *fp)
{
#ifdef DEBUG
    fprintf(fp, " %-3s %-16s %-6s %-10s\n", "#", "name", "copyOf", "first/next");
    fprintf(fp, "-------------------------------------------\n");
    for (uint32 i = 0; i < stackPosition_; i++) {
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
        return NULL;

    MBasicBlock *dom = immediateDominator();
    if (dom != getPredecessor(0))
        return NULL;

    
    MInstruction *ins = dom->lastIns();
    if (ins->isTest()) {
        MTest *test = ins->toTest();

        JS_ASSERT(test->ifTrue() == this || test->ifFalse() == this);
        if (test->ifTrue() == this && test->ifFalse() == this)
            return NULL;

        *pdirection = (test->ifTrue() == this) ? TRUE_BRANCH : FALSE_BRANCH;
        return test;
    }

    return NULL;
}
