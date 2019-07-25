








































#include <limits.h>
#include "BitSet.h"
#include "LinearScan.h"
#include "IonLIR-inl.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

static bool
UseCompatibleWith(const LUse *use, LAllocation alloc)
{
    switch (use->policy()) {
      case LUse::ANY:
      case LUse::KEEPALIVE:
        return alloc.isRegister() || alloc.isMemory();
      case LUse::REGISTER:
        return alloc.isRegister();
      case LUse::FIXED:
        if (!alloc.isGeneralReg())
            return false;
        return alloc.toGeneralReg()->reg().code() == use->registerCode();
      default:
        JS_NOT_REACHED("Unknown use policy");
    }
    return false;
}

#ifdef DEBUG
static bool
DefinitionCompatibleWith(LInstruction *ins, const LDefinition *def, LAllocation alloc)
{
    if (ins->isPhi()) {
        if (def->type() == LDefinition::DOUBLE)
            return alloc.isFloatReg() || alloc.kind() == LAllocation::DOUBLE_SLOT;
        return alloc.isGeneralReg() || alloc.kind() == LAllocation::STACK_SLOT;
    }

    switch (def->policy()) {
      case LDefinition::DEFAULT:
        if (!alloc.isRegister())
            return false;
        return alloc.isFloatReg() == (def->type() == LDefinition::DOUBLE);
      case LDefinition::PRESET:
        return alloc == *def->output();
      case LDefinition::MUST_REUSE_INPUT:
        if (!alloc.isRegister() || !ins->numOperands())
            return false;
        return alloc == *ins->getOperand(0);
      case LDefinition::REDEFINED:
        return true;
      default:
        JS_NOT_REACHED("Unknown definition policy");
    }
    return false;
}
#endif

bool
LiveInterval::addRange(CodePosition from, CodePosition to)
{
    JS_ASSERT(from <= to);

    Range newRange(from, to);

    Range *i;
    
    for (i = ranges_.end() - 1; i >= ranges_.begin(); i--) {
        if (newRange.from <= i->to) {
            if (i->from < newRange.from)
                newRange.from = i->from;
            break;
        }
    }
    
    for (; i >= ranges_.begin(); i--) {
        if (newRange.to < i->from.previous())
            break;
        if (newRange.to < i->to)
            newRange.to = i->to;
        ranges_.erase(i);
    }

    return ranges_.insert(i + 1, newRange);
}

void
LiveInterval::setFrom(CodePosition from)
{
    while (!ranges_.empty()) {
        if (ranges_.back().to < from) {
            ranges_.erase(&ranges_.back());
        } else {
            ranges_.back().from = from;
            break;
        }
    }
}

CodePosition
LiveInterval::start()
{
    JS_ASSERT(!ranges_.empty());
    return ranges_.back().from;
}

CodePosition
LiveInterval::end()
{
    JS_ASSERT(!ranges_.empty());
    return ranges_.begin()->to;
}

bool
LiveInterval::covers(CodePosition pos)
{
    for (size_t i = 0; i < ranges_.length(); i++) {
        if (ranges_[i].to < pos)
            return false;
        if (ranges_[i].from <= pos)
            return true;
    }
    return false;
}

CodePosition
LiveInterval::nextCoveredAfter(CodePosition pos)
{
    for (size_t i = 0; i < ranges_.length(); i++) {
        if (ranges_[i].to < pos) {
            if (i)
                return ranges_[i-1].from;
            break;
        }
        if (ranges_[i].from <= pos)
            return pos;
    }
    return CodePosition::MIN;
}

CodePosition
LiveInterval::intersect(LiveInterval *other)
{
    size_t i = 0;
    size_t j = 0;

    while (i < ranges_.length() && j < other->ranges_.length()) {
        if (ranges_[i].from <= other->ranges_[j].from) {
            if (other->ranges_[j].from < ranges_[i].to)
                return other->ranges_[j].from;
            i++;
        } else if (other->ranges_[j].from <= ranges_[i].from) {
            if (ranges_[i].from < other->ranges_[j].to)
                return ranges_[i].from;
            j++;
        }
    }

    return CodePosition::MIN;
}

size_t
LiveInterval::numRanges()
{
    return ranges_.length();
}

LiveInterval::Range *
LiveInterval::getRange(size_t i)
{
    return &ranges_[i];
}












bool
LiveInterval::splitFrom(CodePosition pos, LiveInterval *after)
{
    JS_ASSERT(pos >= start() && pos <= end());
    JS_ASSERT(after->ranges_.empty());

    
    size_t bufferLength = ranges_.length();
    Range *buffer = ranges_.extractRawBuffer();
    if (!buffer)
        return false;
    after->ranges_.replaceRawBuffer(buffer, bufferLength);

    
    for (Range *i = &after->ranges_.back(); i >= after->ranges_.begin(); i--) {
        if (pos > i->to)
            continue;

        if (pos > i->from) {
            
            Range split(i->from, pos.previous());
            i->from = pos;
            if (!ranges_.append(split))
                return false;
        }
        if (!ranges_.append(i + 1, after->ranges_.end()))
            return false;
        after->ranges_.shrinkBy(after->ranges_.end() - i - 1);
        return true;
    }

    JS_NOT_REACHED("Emptied an interval");
    return false;
}

LiveInterval *
VirtualRegister::intervalFor(CodePosition pos)
{
    for (LiveInterval **i = intervals_.begin(); i != intervals_.end(); i++) {
        if ((*i)->covers(pos))
            return *i;
        if (pos <= (*i)->end())
            break;
    }
    return NULL;
}

LiveInterval *
VirtualRegister::getFirstInterval()
{
    JS_ASSERT(!intervals_.empty());
    return intervals_[0];
}

LOperand *
VirtualRegister::nextUseAfter(CodePosition after)
{
    LOperand *min = NULL;
    CodePosition minPos = CodePosition::MAX;
    for (LOperand *i = uses_.begin(); i != uses_.end(); i++) {
        CodePosition ip(i->ins->id(), CodePosition::INPUT);
        if (i->use->policy() != LUse::KEEPALIVE && ip >= after && ip < minPos) {
            min = i;
            minPos = ip;
        }
    }
    return min;
}







CodePosition
VirtualRegister::nextUsePosAfter(CodePosition after)
{
    LOperand *min = nextUseAfter(after);
    if (min)
        return CodePosition(min->ins->id(), CodePosition::INPUT);
    return CodePosition::MAX;
}







CodePosition
VirtualRegister::nextIncompatibleUseAfter(CodePosition after, LAllocation alloc)
{
    CodePosition min = CodePosition::MAX;
    for (LOperand *i = uses_.begin(); i != uses_.end(); i++) {
        CodePosition ip(i->ins->id(), CodePosition::INPUT);
        if (ip >= after && ip < min && !UseCompatibleWith(i->use, alloc))
            min = ip;
    }
    return min;
}

const CodePosition CodePosition::MAX(UINT_MAX);
const CodePosition CodePosition::MIN(0);





bool
LinearScanAllocator::createDataStructures()
{
    allowedRegs = RegisterSet::All();

    liveIn = lir->mir()->allocate<BitSet*>(graph.numBlockIds());
    if (!liveIn)
        return false;

    if (!vregs.init(lir->mir(), graph.numVirtualRegisters()))
        return false;

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *block = graph.getBlock(i);
        for (LInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            bool foundRealDef = false;
            for (size_t j = 0; j < ins->numDefs(); j++) {
                if (ins->getDef(j)->policy() != LDefinition::REDEFINED) {
                    foundRealDef = true;
                    uint32 reg = ins->getDef(j)->virtualRegister();
                    if (!vregs[reg].init(reg, block, *ins, ins->getDef(j)))
                        return false;
                }
            }
            if (!foundRealDef) {
                if (!vregs[*ins].init(ins->id(), block, *ins, NULL))
                    return false;
            }
            if (ins->numTemps()) {
                for (size_t j = 0; j < ins->numTemps(); j++) {
                    LDefinition *def = ins->getTemp(j);
                    if (!vregs[def].init(def->virtualRegister(), block, *ins, def, true))
                        return false;
                }
            }
        }
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi *phi = block->getPhi(j);
            LDefinition *def = phi->getDef(0);
            if (!vregs[def].init(phi->id(), block, phi, def))
                return false;
        }
    }

    return true;
}





















bool
LinearScanAllocator::buildLivenessInfo()
{
    Vector<MBasicBlock *, 1, SystemAllocPolicy> loopWorkList;
    BitSet *loopDone = BitSet::New(graph.numBlockIds());
    if (!loopDone)
        return false;

    for (size_t i = graph.numBlocks(); i > 0; i--) {
        LBlock *block = graph.getBlock(i - 1);
        MBasicBlock *mblock = block->mir();

        BitSet *live = BitSet::New(graph.numVirtualRegisters());
        if (!live)
            return false;
        liveIn[mblock->id()] = live;

        
        for (size_t i = 0; i < mblock->lastIns()->numSuccessors(); i++) {
            MBasicBlock *successor = mblock->lastIns()->getSuccessor(i);
            
            if (mblock->id() < successor->id())
                live->insertAll(liveIn[successor->id()]);
        }

        
        if (mblock->successorWithPhis()) {
            LBlock *phiSuccessor = mblock->successorWithPhis()->lir();
            for (unsigned int j = 0; j < phiSuccessor->numPhis(); j++) {
                LPhi *phi = phiSuccessor->getPhi(j);
                LAllocation *use = phi->getOperand(mblock->positionInPhiSuccessor());
                uint32 reg = use->toUse()->virtualRegister();
                live->insert(reg);
            }
        }

        
        
        for (BitSet::Iterator i(live->begin()); i != live->end(); i++) {
            vregs[*i].getInterval(0)->addRange(inputOf(block->firstId()),
                                               outputOf(block->lastId()));
        }

        
        
        for (LInstructionReverseIterator ins = block->rbegin(); ins != block->rend(); ins++) {
            for (size_t i = 0; i < ins->numDefs(); i++) {
                if (ins->getDef(i)->policy() != LDefinition::REDEFINED) {
                    LDefinition *def = ins->getDef(i);

                    
                    
                    
                    CodePosition from(inputOf(*ins));
                    if (def->policy() == LDefinition::MUST_REUSE_INPUT)
                        from = outputOf(*ins);

                    
                    
                    if (vregs[def].getInterval(0)->numRanges() == 0)
                        vregs[def].getInterval(0)->addRange(from, outputOf(*ins));
                    vregs[def].getInterval(0)->setFrom(from);
                    live->remove(def->virtualRegister());
                }
            }

            for (size_t i = 0; i < ins->numTemps(); i++)
                vregs[ins->getTemp(i)].getInterval(0)->addRange(inputOf(*ins), outputOf(*ins));

            for (LInstruction::InputIterator alloc(**ins); alloc.more(); alloc.next())
            {
                if (alloc->isUse()) {
                    LUse *use = alloc->toUse();
                    vregs[use].addUse(LOperand(use, *ins, alloc.isSnapshotInput()));

                    if (ins->id() == block->firstId()) {
                        vregs[use].getInterval(0)->addRange(inputOf(*ins), outputOf(*ins));
                    } else {
                        vregs[use].getInterval(0)->addRange(inputOf(block->firstId()),
                                                            outputOf(*ins));
                    }
                    live->insert(use->virtualRegister());
                }
            }
        }

        
        
        
        for (unsigned int i = 0; i < block->numPhis(); i++) {
            LDefinition *def = block->getPhi(i)->getDef(0);
            if (live->contains(def->virtualRegister())) {
                live->remove(def->virtualRegister());
            } else {
                
                
                vregs[def].getInterval(0)->addRange(inputOf(block->firstId()),
                                                    inputOf(*block->begin()).previous());
            }
        }

        if (mblock->isLoopHeader()) {
            
            
            
            
            
            MBasicBlock *loopBlock = mblock->backedge();
            while (true) {
                
                for (BitSet::Iterator i(live->begin()); i != live->end(); i++) {
                    vregs[*i].getInterval(0)->addRange(inputOf(loopBlock->lir()->firstId()),
                                                       outputOf(loopBlock->lir()->lastId()));
                }

                
                liveIn[loopBlock->id()]->insertAll(live);

                
                loopDone->insert(loopBlock->id());

                
                
                
                if (loopBlock != mblock) {
                    for (size_t i = 0; i < loopBlock->numPredecessors(); i++) {
                        MBasicBlock *pred = loopBlock->getPredecessor(i);
                        if (loopDone->contains(pred->id()))
                            continue;
                        if (!loopWorkList.append(pred))
                            return false;
                    }
                }

                
                if (loopWorkList.empty())
                    break;
                loopBlock = loopWorkList.popCopy();
            }

            
            loopDone->clear();
        }

        JS_ASSERT_IF(!mblock->numPredecessors(), live->empty());
    }

    return true;
}





















bool
LinearScanAllocator::allocateRegisters()
{
    
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        LiveInterval *live = vregs[i].getInterval(0);
        if (live->numRanges() > 0) {
            setIntervalPriority(live);
            unhandled.enqueue(live);
        }
    }

    
    while ((current = unhandled.dequeue()) != NULL) {
        JS_ASSERT(current->getAllocation()->isUse());
        JS_ASSERT(current->numRanges() > 0);

        CodePosition position = current->start();

        IonSpew(IonSpew_LSRA, "Processing %d = [%u, %u] (pri=%d)",
                current->reg()->reg(), current->start().pos(),
                current->end().pos(), current->priority());

        
        for (IntervalIterator i(active.begin()); i != active.end(); ) {
            LiveInterval *it = *i;
            JS_ASSERT(it->numRanges() > 0);

            if (it->end() < position) {
                i = active.removeAt(i);
                finishInterval(it);
            } else if (!it->covers(position)) {
                i = active.removeAt(i);
                inactive.pushBack(it);
            } else {
                i++;
            }
        }

        
        for (IntervalIterator i(inactive.begin()); i != inactive.end(); ) {
            LiveInterval *it = *i;
            JS_ASSERT(it->numRanges() > 0);

            if (it->end() < position) {
                i = inactive.removeAt(i);
                finishInterval(it);
            } else if (it->covers(position)) {
                i = inactive.removeAt(i);
                active.pushBack(it);
            } else {
                i++;
            }
        }

        
        validateIntervals();

        
        bool mustHaveRegister = false;
        bool canSpillOthers = true;
        if (position == current->reg()->getFirstInterval()->start()) {
            LDefinition *def = current->reg()->def();
            LDefinition::Policy policy = def->policy();
            if (policy == LDefinition::PRESET) {
                IonSpew(IonSpew_LSRA, " Definition has preset policy.");
                if (!assign(*def->output()))
                    return false;
                continue;
            }
            if (policy == LDefinition::MUST_REUSE_INPUT) {
                IonSpew(IonSpew_LSRA, " Definition has 'must reuse input' policy.");
                LInstruction *ins = current->reg()->ins();
                JS_ASSERT(ins->numOperands() > 0);
                JS_ASSERT(ins->getOperand(0)->isUse());
                LiveInterval *inputInterval = vregs[ins->getOperand(0)].intervalFor(inputOf(ins));
                JS_ASSERT(inputInterval);
                JS_ASSERT(inputInterval->getAllocation()->isGeneralReg());
                if (!assign(*inputInterval->getAllocation()))
                    return false;
                continue;
            }
            if (policy == LDefinition::DEFAULT) {
                if (current->reg()->ins()->isPhi()) {
                    mustHaveRegister = false;
                    canSpillOthers = false;
                } else {
                    mustHaveRegister = true;
                }
            } else {
                JS_ASSERT(policy == LDefinition::REDEFINED);
            }
        } else if (position.subpos() == CodePosition::INPUT) {
            
            LOperand *fixedOp = NULL;
            for (size_t i = 0; i < current->reg()->numUses(); i++) {
                LOperand *op = current->reg()->getUse(i);
                if (op->ins->id() == position.ins()) {
                    LUse::Policy pol = op->use->policy();
                    if (pol == LUse::FIXED) {
                        IonSpew(IonSpew_LSRA, " Use has fixed policy.");
                        JS_ASSERT(!fixedOp);
                        fixedOp = op;
                    } else if (pol == LUse::REGISTER) {
                        IonSpew(IonSpew_LSRA, " Use has 'must have register' policy.");
                        mustHaveRegister = true;
                    } else {
                        JS_ASSERT(pol == LUse::ANY || pol == LUse::KEEPALIVE);
                    }
                }
            }

            
            if (fixedOp) {
                if (!assign(LGeneralReg(Register::FromCode(fixedOp->use->registerCode()))))
                    return false;
                continue;
            }
        }

        
        firstUse = current->reg()->nextUseAfter(position);
        if (firstUse)
            firstUsePos = inputOf(firstUse->ins);
        else
            firstUsePos = CodePosition::MAX;

        
        if (!mustHaveRegister && !firstUse && current->reg()->canonicalSpill()) {
            if (!spill())
                return false;
            continue;
        }

        
        IonSpew(IonSpew_LSRA, " Attempting free register allocation");

        CodePosition bestFreeUntil;
        Register::Code bestCode = findBestFreeRegister(&bestFreeUntil);
        if (bestCode != Register::Codes::Invalid) {
            Register best = Register::FromCode(bestCode);
            IonSpew(IonSpew_LSRA, "  Decided best register was %s", best.name());

            
            if (bestFreeUntil <= current->end()) {
                if (!splitInterval(current, bestFreeUntil))
                    return false;
            }
            if (!assign(LGeneralReg(best)))
                return false;

            continue;
        }

        IonSpew(IonSpew_LSRA, "  Unable to allocate free register");

        
        if (!canSpillOthers) {
            IonSpew(IonSpew_LSRA, " Can't spill any other intervals, spilling this one");
            if (!spill())
                return false;
            continue;
        }

        IonSpew(IonSpew_LSRA, " Attempting blocked register allocation");

        
        
        
        CodePosition bestNextUsed;
        bestCode = findBestBlockedRegister(&bestNextUsed);
        if (bestCode != Register::Codes::Invalid &&
            (mustHaveRegister || firstUsePos < bestNextUsed))
        {
            Register best = Register::FromCode(bestCode);
            IonSpew(IonSpew_LSRA, "  Decided best register was %s", best.name());

            if (!assign(LGeneralReg(best)))
                return false;

            continue;
        }

        IonSpew(IonSpew_LSRA, "  No registers available to spill");
        JS_ASSERT(!mustHaveRegister);

        if (!spill())
            return false;
    }

    validateAllocations();

    return true;
}










bool
LinearScanAllocator::resolveControlFlow()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *successor = graph.getBlock(i);
        MBasicBlock *mSuccessor = successor->mir();
        if (mSuccessor->numPredecessors() < 1)
            continue;

        
        for (size_t j = 0; j < successor->numPhis(); j++) {
            LPhi *phi = successor->getPhi(j);
            LiveInterval *to = vregs[phi].intervalFor(inputOf(successor->firstId()));
            JS_ASSERT(to);

            for (size_t k = 0; k < mSuccessor->numPredecessors(); k++) {
                LBlock *predecessor = mSuccessor->getPredecessor(k)->lir();
                JS_ASSERT(predecessor->mir()->numSuccessors() == 1);

                LAllocation *input = phi->getOperand(predecessor->mir()->positionInPhiSuccessor());
                LiveInterval *from = vregs[input].intervalFor(outputOf(predecessor->lastId()));
                JS_ASSERT(from);

                if (!moveBefore(outputOf(predecessor->lastId()), from, to))
                    return false;
            }
        }

        
        BitSet *live = liveIn[mSuccessor->id()];
        for (BitSet::Iterator liveRegId(live->begin()); liveRegId != live->end(); liveRegId++) {
            LiveInterval *to = vregs[*liveRegId].intervalFor(inputOf(successor->firstId()));
            JS_ASSERT(to);

            for (size_t j = 0; j < mSuccessor->numPredecessors(); j++) {
                LBlock *predecessor = mSuccessor->getPredecessor(j)->lir();
                LiveInterval *from = vregs[*liveRegId].intervalFor(outputOf(predecessor->lastId()));
                JS_ASSERT(from);

                if (mSuccessor->numPredecessors() > 1) {
                    JS_ASSERT(predecessor->mir()->numSuccessors() == 1);
                    if (!moveBefore(outputOf(predecessor->lastId()), from, to))
                        return false;
                } else {
                    if (!moveBefore(inputOf(successor->firstId()), from, to))
                        return false;
                }
            }
        }
    }

    return true;
}






bool
LinearScanAllocator::reifyAllocations()
{
    
    unhandled.enqueue(inactive);
    unhandled.enqueue(active);
    unhandled.enqueue(handled);

    
    LiveInterval *interval;
    while ((interval = unhandled.dequeue()) != NULL) {
        VirtualRegister *reg = interval->reg();

        
        for (size_t i = 0; i < reg->numUses(); i++) {
            LOperand *use = reg->getUse(i);
            CodePosition pos = inputOf(use->ins);
            if (use->snapshot)
                pos = outputOf(use->ins);
            if (interval->covers(pos)) {
                JS_ASSERT(UseCompatibleWith(use->use, *interval->getAllocation()));
                *static_cast<LAllocation *>(use->use) = *interval->getAllocation();
            }
        }

        if (interval->index() == 0)
        {
            
            JS_ASSERT(DefinitionCompatibleWith(reg->ins(), reg->def(), *interval->getAllocation()));
            reg->def()->setOutput(*interval->getAllocation());
        }
        else if (interval->start() != inputOf(vregs[interval->start()].block()->firstId()) &&
                 (!interval->reg()->canonicalSpill() ||
                  interval->reg()->canonicalSpill() == interval->getAllocation() ||
                  *interval->reg()->canonicalSpill() != *interval->getAllocation()))
        {
            
            
            
            if (!moveBefore(interval->start(), reg->getInterval(interval->index() - 1), interval))
                return false;
        }
    }

    
    graph.setLocalSlotCount(stackAssignment.stackHeight());

    return true;
}





bool
LinearScanAllocator::splitInterval(LiveInterval *interval, CodePosition pos)
{
    
    
    JS_ASSERT(interval->start() < pos && pos <= interval->end());

    VirtualRegister *reg = interval->reg();

    
    LiveInterval *newInterval = new LiveInterval(reg, interval->index() + 1);
    if (!interval->splitFrom(pos, newInterval))
        return false;

    JS_ASSERT(interval->numRanges() > 0);
    JS_ASSERT(newInterval->numRanges() > 0);

    if (!reg->addInterval(newInterval))
        return false;

    
    if (!getMoveGroupBefore(pos))
        return false;

    IonSpew(IonSpew_LSRA, "  Split interval to %u = [%u, %u]/[%u, %u]",
            interval->reg()->reg(), interval->start().pos(),
            interval->end().pos(), newInterval->start().pos(),
            newInterval->end().pos());

    
    
    
    setIntervalPriority(newInterval);
    unhandled.enqueue(newInterval);

    return true;
}





bool
LinearScanAllocator::assign(LAllocation allocation)
{
    if (allocation.isGeneralReg())
        IonSpew(IonSpew_LSRA, "Assigning register %s", allocation.toGeneralReg()->reg().name());
    current->setAllocation(allocation);

    
    CodePosition toSplit = current->reg()->nextIncompatibleUseAfter(current->start(), allocation);
    if (toSplit <= current->end()) {
        if (!splitInterval(current, toSplit))
            return false;
    }

    if (allocation.isGeneralReg()) {
        
        for (IntervalIterator i(active.begin()); i != active.end(); i++) {
            if (i->getAllocation()->isGeneralReg() &&
                i->getAllocation()->toGeneralReg()->reg() == allocation.toGeneralReg()->reg())
            {
                IonSpew(IonSpew_LSRA, " Splitting active interval %u = [%u, %u]",
                        i->reg()->ins()->id(), i->start().pos(), i->end().pos());

                JS_ASSERT(i->start() != current->start());
                JS_ASSERT(i->covers(current->start()));
                JS_ASSERT(i->start() != current->start());

                if (!splitInterval(*i, current->start()))
                    return false;

                LiveInterval *it = *i;
                active.removeAt(i);
                finishInterval(it);
                break;
            }
        }

        
        for (IntervalIterator i(inactive.begin()); i != inactive.end(); ) {
            if (i->getAllocation()->isGeneralReg() &&
                i->getAllocation()->toGeneralReg()->reg() == allocation.toGeneralReg()->reg())
            {
                IonSpew(IonSpew_LSRA, " Splitting inactive interval %u = [%u, %u]",
                        i->reg()->ins()->id(), i->start().pos(), i->end().pos());

                LiveInterval *it = *i;
                CodePosition nextActive = it->nextCoveredAfter(current->start());
                JS_ASSERT(nextActive != CodePosition::MIN);

                if (!splitInterval(it, nextActive))
                    return false;

                i = inactive.removeAt(i);
                finishInterval(it);
            } else {
                i++;
            }
        }
    } else if (allocation.isMemory()) {
        if (current->reg()->canonicalSpill())
            JS_ASSERT(allocation == *current->reg()->canonicalSpill());
        else
            current->reg()->setCanonicalSpill(current->getAllocation());
    }

    active.pushBack(current);

    return true;
}

bool
LinearScanAllocator::spill()
{
    IonSpew(IonSpew_LSRA, "  Decided to spill current interval");

    if (current->reg()->canonicalSpill()) {
        IonSpew(IonSpew_LSRA, "  Allocating canonical spill location");

        return assign(*current->reg()->canonicalSpill());
    }

    uint32 stackSlot;
    if (!stackAssignment.allocateSlot(&stackSlot))
        return false;

    IonSpew(IonSpew_LSRA, "  Allocated spill slot %u", stackSlot);

    return assign(LStackSlot(stackSlot));
}

void
LinearScanAllocator::finishInterval(LiveInterval *interval)
{
    LAllocation *alloc = interval->getAllocation();
    JS_ASSERT(!alloc->isUse());

    bool lastInterval = interval->index() == (interval->reg()->numIntervals() - 1);
    if (alloc->isStackSlot() && (alloc != interval->reg()->canonicalSpill() || lastInterval)) {
        if (alloc->toStackSlot()->isDouble())
            stackAssignment.freeDoubleSlot(alloc->toStackSlot()->slot());
        else
            stackAssignment.freeSlot(alloc->toStackSlot()->slot());
    }

    handled.pushBack(interval);
}






Register::Code
LinearScanAllocator::findBestFreeRegister(CodePosition *freeUntil)
{
    IonSpew(IonSpew_LSRA, "  Computing freeUntilPos");

    
    CodePosition freeUntilPos[Registers::Total];
    for (size_t i = 0; i < Registers::Total; i++) {
        if (allowedRegs.has(Register::FromCode(i)))
            freeUntilPos[i] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            IonSpew(IonSpew_LSRA, "   Register %s not free", reg.name());
            freeUntilPos[reg.code()] = CodePosition::MIN;
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            CodePosition pos = current->intersect(*i);
            if (pos != CodePosition::MIN && pos < freeUntilPos[reg.code()]) {
                freeUntilPos[reg.code()] = pos;
                IonSpew(IonSpew_LSRA, "   Register %s free until %u", reg.name(), pos.pos());
            }
        }
    }

    Register::Code bestCode = Registers::Invalid;
    if (current->index()) {
        
        
        LiveInterval *previous = current->reg()->getInterval(current->index() - 1);
        if (previous->getAllocation()->isGeneralReg()) {
            Register prevReg = previous->getAllocation()->toGeneralReg()->reg();
            if (freeUntilPos[prevReg.code()] != CodePosition::MIN)
                bestCode = prevReg.code();
        }
    }
    if (bestCode == Registers::Invalid && firstUse && firstUse->use->policy() == LUse::FIXED) {
        
        uint32 fixedReg = firstUse->use->registerCode();
        if (freeUntilPos[fixedReg] != CodePosition::MIN)
            bestCode = Register::Code(fixedReg);
    }
    if (bestCode == Registers::Invalid) {
        
        for (uint32 i = 0; i < Registers::Total; i++) {
            if (freeUntilPos[i] == CodePosition::MIN)
                continue;
            if (bestCode == Registers::Invalid || freeUntilPos[i] > freeUntilPos[bestCode])
                bestCode = Register::Code(i);
        }
    }

    if (bestCode != Registers::Invalid)
        *freeUntil = freeUntilPos[bestCode];
    return bestCode;
}







Register::Code
LinearScanAllocator::findBestBlockedRegister(CodePosition *nextUsed)
{
    IonSpew(IonSpew_LSRA, "  Computing nextUsePos");

    
    CodePosition nextUsePos[Registers::Total];
    for (size_t i = 0; i < Registers::Total; i++) {
        if (allowedRegs.has(Register::FromCode(i)))
            nextUsePos[i] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            if (i->start() == current->start()) {
                nextUsePos[reg.code()] = CodePosition::MIN;
                IonSpew(IonSpew_LSRA, "   Disqualifying %s due to recency", reg.name());
            } else {
                nextUsePos[reg.code()] = i->reg()->nextUsePosAfter(current->start());
                IonSpew(IonSpew_LSRA, "   Register %s next used %u", reg.name(),
                        nextUsePos[reg.code()].pos());
            }
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            CodePosition pos = i->reg()->nextUsePosAfter(current->start());
            JS_ASSERT(i->covers(pos));
            if (pos < nextUsePos[reg.code()]) {
                nextUsePos[reg.code()] = pos;
                IonSpew(IonSpew_LSRA, "   Register %s next used %u", reg.name(), pos.pos());
            }
        }
    }

    
    Register::Code bestCode = Register::Codes::Invalid;
    for (size_t i = 0; i < Registers::Total; i++) {
        if (nextUsePos[i] == CodePosition::MIN)
            continue;
        if (bestCode == Register::Codes::Invalid || nextUsePos[i] > nextUsePos[bestCode])
            bestCode = Register::Code(i);
    }

    if (bestCode != Register::Codes::Invalid)
        *nextUsed = nextUsePos[bestCode];
    return bestCode;
}












bool
LinearScanAllocator::canCoexist(LiveInterval *a, LiveInterval *b)
{
    LAllocation *aa = a->getAllocation();
    LAllocation *ba = b->getAllocation();
    if (aa->isGeneralReg() && ba->isGeneralReg() &&
        aa->toGeneralReg()->reg() == ba->toGeneralReg()->reg())
    {
        return a->intersect(b) == CodePosition::MIN;
    }
    return true;
}

LMoveGroup *
LinearScanAllocator::getMoveGroupBefore(CodePosition pos)
{
    VirtualRegister *vreg = &vregs[pos];
    JS_ASSERT(vreg->ins());
    JS_ASSERT(!vreg->ins()->isPhi());

    LMoveGroup *moves;
    switch (pos.subpos()) {
      case CodePosition::INPUT:
        if (vreg->inputMoves())
            return vreg->inputMoves();

        moves = new LMoveGroup;
        vreg->setInputMoves(moves);
        if (vreg->outputMoves())
            vreg->block()->insertBefore(vreg->outputMoves(), moves);
        else
            vreg->block()->insertBefore(vreg->ins(), moves);
        return moves;

      case CodePosition::OUTPUT:
        if (vreg->outputMoves())
            return vreg->outputMoves();

        moves = new LMoveGroup;
        vreg->setOutputMoves(moves);
        vreg->block()->insertBefore(vreg->ins(), moves);
        return moves;

      default:
        JS_NOT_REACHED("Unknown subposition");
        return NULL;
    }
}

bool
LinearScanAllocator::moveBefore(CodePosition pos, LiveInterval *from, LiveInterval *to)
{
    LMoveGroup *moves = getMoveGroupBefore(pos);
    if (*from->getAllocation() == *to->getAllocation())
        return true;
    return moves->add(from->getAllocation(), to->getAllocation());
}

#ifdef DEBUG






void
LinearScanAllocator::validateIntervals()
{
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        JS_ASSERT(i->numRanges() > 0);
        JS_ASSERT(i->covers(current->start()));

        for (IntervalIterator j(active.begin()); j != i; j++)
            JS_ASSERT(canCoexist(*i, *j));
    }

    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        JS_ASSERT(i->numRanges() > 0);
        JS_ASSERT(i->end() >= current->start());
        JS_ASSERT(!i->covers(current->start()));

        for (IntervalIterator j(active.begin()); j != active.end(); j++) {
            JS_ASSERT(*i != *j);
            JS_ASSERT(canCoexist(*i, *j));
        }
        for (IntervalIterator j(inactive.begin()); j != i; j++)
            JS_ASSERT(canCoexist(*i, *j));
    }

    for (IntervalIterator i(handled.begin()); i != handled.end(); i++) {
        JS_ASSERT(!i->getAllocation()->isUse());
        JS_ASSERT(i->numRanges() > 0);
        if (i->getAllocation()->isGeneralReg()) {
            JS_ASSERT(i->end() < current->start());
            JS_ASSERT(!i->covers(current->start()));
        }

        for (IntervalIterator j(active.begin()); j != active.end(); j++)
            JS_ASSERT(*i != *j);
        for (IntervalIterator j(inactive.begin()); j != inactive.end(); j++)
            JS_ASSERT(*i != *j);
    }
}





void
LinearScanAllocator::validateAllocations()
{
    for (IntervalIterator i(handled.begin()); i != handled.end(); i++) {
        for (IntervalIterator j(handled.begin()); j != i; j++) {
            JS_ASSERT(*i != *j);
            JS_ASSERT(canCoexist(*i, *j));
        }
        bool found = false;
        for (size_t j = 0; j < i->reg()->numIntervals(); j++) {
            if (i->reg()->getInterval(j) == *i) {
                JS_ASSERT(j == i->index());
                found = true;
                break;
            }
        }
        JS_ASSERT(found);
    }
}
#endif

bool
LinearScanAllocator::go()
{
    IonSpew(IonSpew_LSRA, "Beginning register allocation");

    IonSpew(IonSpew_LSRA, "Beginning creation of initial data structures");
    if (!createDataStructures())
        return false;
    IonSpew(IonSpew_LSRA, "Creation of initial data structures completed");

    IonSpew(IonSpew_LSRA, "Beginning liveness analysis");
    if (!buildLivenessInfo())
        return false;
    IonSpew(IonSpew_LSRA, "Liveness analysis complete");

    IonSpew(IonSpew_LSRA, "Beginning preliminary register allocation");
    if (!allocateRegisters())
        return false;
    IonSpew(IonSpew_LSRA, "Preliminary register allocation complete");

    IonSpew(IonSpew_LSRA, "Beginning control flow resolution");
    if (!resolveControlFlow())
        return false;
    IonSpew(IonSpew_LSRA, "Control flow resolution complete");

    IonSpew(IonSpew_LSRA, "Beginning register allocation reification");
    if (!reifyAllocations())
        return false;
    IonSpew(IonSpew_LSRA, "Register allocation reification complete");

    IonSpew(IonSpew_LSRA, "Register allocation complete");

    return true;
}









void
LinearScanAllocator::setIntervalPriority(LiveInterval *interval)
{
    int priority = 2;

    if (interval == interval->reg()->getFirstInterval()) {
        if (interval->reg()->def()->policy() == LDefinition::PRESET) {
            
            priority = 0;
        } else if (interval->reg()->def()->policy() == LDefinition::MUST_REUSE_INPUT) {
            
            priority = 0;
        } else if (!interval->reg()->ins()->isPhi()) {
            
            priority = 1;
        }
        
    } else {
        for (size_t i = 0; i < interval->reg()->numUses(); i++) {
            LOperand *op = interval->reg()->getUse(i);
            if (interval->start() == inputOf(op->ins)) {
                if (op->use->policy() == LUse::FIXED) {
                    
                    priority = 0;
                    break;
                } else if (op->use->policy() == LUse::REGISTER) {
                    
                    priority = priority < 1 ? priority : 1;
                }
            }
        }
    }

    interval->setPriority(priority);
}

void
LinearScanAllocator::UnhandledQueue::enqueue(LiveInterval *interval)
{
    IntervalIterator i(begin());
    for (; i != end(); i++) {
        if (i->start() < interval->start())
            break;
        if (i->start() == interval->start() && i->priority() < interval->priority())
            break;
    }
    insertBefore(*i, interval);
}

LiveInterval *
LinearScanAllocator::UnhandledQueue::dequeue()
{
    if (rbegin() == rend())
        return NULL;

    LiveInterval *result = *rbegin();
    remove(result);
    return result;
}
