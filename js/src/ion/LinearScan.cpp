








































#include <limits.h>
#include "BitSet.h"
#include "LinearScan.h"
#include "IonLIR-inl.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

bool
LiveInterval::addRange(CodePosition from, CodePosition to)
{
    JS_ASSERT(from <= to);

    Range newRange(from, to);

    Range *i;
    
    for (i = ranges_.end() - 1; i >= ranges_.begin(); i--) {
        if (newRange.from <= i->from)
            break;
    }
    
    for (; i >= ranges_.begin(); i--) {
        if (newRange.to < i->from)
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

    for (Range *i = ranges_.begin(); i != ranges_.end(); ranges_.erase(i)) {
        if (pos > i->from) {
            if (pos <= i->to) {
                
                Range split(pos, i->to);
                i->to = pos.previous();
                if (!after->ranges_.append(split))
                    return false;
            }
            return true;
        } else if (!after->ranges_.append(*i)) {
            return false;
        }
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







CodePosition
VirtualRegister::nextUseAfter(CodePosition after)
{
    CodePosition min = CodePosition::MAX;
    for (LOperand *i = uses_.begin(); i != uses_.end(); i++) {
        CodePosition ip(i->ins->id(), CodePosition::INPUT);
        if (i->use->policy() != LUse::KEEPALIVE && ip >= after && ip < min)
            min = ip;
    }
    return min;
}







CodePosition
VirtualRegister::nextIncompatibleUseAfter(CodePosition after, LAllocation alloc)
{
    CodePosition min = CodePosition::MAX;
    for (LOperand *i = uses_.begin(); i != uses_.end(); i++) {
        CodePosition ip(i->ins->id(), CodePosition::INPUT);
        if (i->use->policy() == LUse::ANY && (alloc.isStackSlot() ||
                                              alloc.isGeneralReg() ||
                                              alloc.isArgument()))
        {
            continue;
        }
        if (i->use->policy() == LUse::REGISTER && alloc.isGeneralReg())
            continue;
        if (i->use->isFixedRegister() && alloc.isGeneralReg() &&
            alloc.toGeneralReg()->reg().code() == i->use->registerCode())
        {
            continue;
        }
        if (i->use->policy() == LUse::KEEPALIVE)
            continue;
        if (ip >= after && ip < min)
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

    liveIn = lir->mir()->allocate<BitSet*>(graph.numBlocks());
    freeUntilPos = lir->mir()->allocate<CodePosition>(Registers::Total);
    nextUsePos = lir->mir()->allocate<CodePosition>(Registers::Total);
    if (!liveIn || !freeUntilPos || !nextUsePos)
        return false;

    if (!vregs.init(lir->mir(), graph.numVirtualRegisters()))
        return false;

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *block = graph.getBlock(i);
        for (LInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            if (ins->numDefs()) {
                for (size_t j = 0; j < ins->numDefs(); j++) {
                    if (ins->getDef(j)->policy() != LDefinition::REDEFINED) {
                        uint32 reg = ins->getDef(j)->virtualRegister();
                        if (!vregs[reg].init(reg, block, *ins, ins->getDef(j)))
                            return false;
                    }
                }
            } else {
                if (!vregs[*ins].init(ins->id(), block, *ins, NULL))
                    return false;
            }
            if (ins->numTemps()) {
                for (size_t j = 0; j < ins->numTemps(); j++) {
                    LDefinition *def = ins->getTemp(j);
                    if (!vregs[def].init(def->virtualRegister(), block, *ins, def))
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
    for (size_t i = graph.numBlocks(); i > 0; i--) {
        LBlock *block = graph.getBlock(i - 1);
        MBasicBlock *mblock = block->mir();

        BitSet *live = BitSet::New(graph.numVirtualRegisters());
        if (!live)
            return false;

        
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
            vregs[*i].getInterval(0)->addRange(outputOf(block->firstId()),
                                               inputOf(block->lastId() + 1));
        }

        
        
        for (LInstructionReverseIterator ins = block->rbegin(); ins != block->rend(); ins++) {
            for (size_t i = 0; i < ins->numDefs(); i++) {
                if (ins->getDef(i)->policy() != LDefinition::REDEFINED) {
                    LDefinition *def = ins->getDef(i);
                    
                    
                    if (vregs[def].getInterval(0)->numRanges() == 0)
                        vregs[def].getInterval(0)->addRange(outputOf(*ins), outputOf(*ins));
                    vregs[def].getInterval(0)->setFrom(outputOf(*ins));
                    live->remove(def->virtualRegister());
                }
            }

            for (size_t i = 0; i < ins->numTemps(); i++)
                vregs[ins->getTemp(i)].getInterval(0)->addRange(outputOf(*ins), outputOf(*ins));

            for (LInstruction::InputIterator alloc(**ins);
                 alloc.more();
                 alloc.next())
            {
                if (alloc->isUse()) {
                    LUse *use = alloc->toUse();
                    vregs[use].addUse(LOperand(use, *ins));

                    if (ins->id() == block->firstId()) {
                        vregs[use].getInterval(0)->addRange(inputOf(*ins), inputOf(*ins));
                    } else {
                        vregs[use].getInterval(0)->addRange(outputOf(block->firstId()),
                                                            inputOf(*ins));
                    }
                    live->insert(use->virtualRegister());
                }
            }
        }

        
        
        
        for (unsigned int i = 0; i < block->numPhis();) {
            if (live->contains(block->getPhi(i)->getDef(0)->virtualRegister())) {
                live->remove(block->getPhi(i)->getDef(0)->virtualRegister());
                i++;
            } else {
                
                
                block->removePhi(i);
            }
        }

        
        
        if (mblock->isLoopHeader()) {
            MBasicBlock *backedge = mblock->backedge();
            for (BitSet::Iterator i(live->begin()); i != live->end(); i++) {
                vregs[*i].getInterval(0)->addRange(outputOf(block->firstId()),
                                                   outputOf(backedge->lir()->lastId()));
            }
        }

        liveIn[mblock->id()] = live;
    }

    return true;
}





















bool
LinearScanAllocator::allocateRegisters()
{
    
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        LiveInterval *live = vregs[i].getInterval(0);
        if (live->numRanges() > 0)
            unhandled.enqueue(live);
    }

    
    while ((current = unhandled.dequeue()) != NULL) {
        JS_ASSERT(current->getAllocation()->isUse());
        JS_ASSERT(current->numRanges() > 0);

        CodePosition position = current->start();

        IonSpew(IonSpew_LSRA, "Processing %d = [%u, %u]",
                current->reg()->reg(), current->start().pos(),
                current->end().pos());

        
        for (IntervalIterator i(active.begin()); i != active.end(); ) {
            LiveInterval *it = *i;
            JS_ASSERT(it->numRanges() > 0);

            if (it->end() < position) {
                i = active.removeAt(i);
                finishInterval(it);
            } else if (!it->covers(position)) {
                i = active.removeAt(i);
                inactive.insert(it);
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
                active.insert(it);
            } else {
                i++;
            }
        }

        
        validateIntervals();

        
        bool mustHaveRegister = false;
        if (position == current->reg()->getFirstInterval()->start()) {
            JS_ASSERT(position.subpos() == CodePosition::OUTPUT);

            LDefinition *def = current->reg()->def();
            LDefinition::Policy policy = def->policy();
            if (policy == LDefinition::PRESET) {
                IonSpew(IonSpew_LSRA, " Definition has preset policy.");
                current->setFlag(LiveInterval::FIXED);
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
            if (policy == LDefinition::DEFAULT)
                mustHaveRegister = !current->reg()->ins()->isPhi();
            else
                JS_ASSERT(policy == LDefinition::REDEFINED);
        } else {
            
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
                current->setFlag(LiveInterval::FIXED);
                if (!assign(LGeneralReg(Register::FromCode(fixedOp->use->registerCode()))))
                    return false;
                continue;
            }
        }

        
        IonSpew(IonSpew_LSRA, " Attempting free register allocation");

        
        Register best = findBestFreeRegister();
        IonSpew(IonSpew_LSRA, "  Decided best register was %u, free until %u", best.code(),
                freeUntilPos[best.code()].pos());

        
        if (freeUntilPos[best.code()] != CodePosition::MIN) {
            
            if (freeUntilPos[best.code()] <= current->end()) {
                if (!splitInterval(current, freeUntilPos[best.code()]))
                    return false;
            }
            if (!assign(LGeneralReg(best)))
                return false;
            continue;
        }

        
        IonSpew(IonSpew_LSRA, "  Unable to allocate free register");
        IonSpew(IonSpew_LSRA, " Attempting blocked register allocation");

        
        best = findBestBlockedRegister();

        
        CodePosition firstUse = current->reg()->nextUseAfter(position);
        IonSpew(IonSpew_LSRA, "  Current interval next used at %u", firstUse.pos());

        
        if (mustHaveRegister) {
            IonSpew(IonSpew_LSRA, "   But this interval needs a register.");
            firstUse = position;
        }

        
        
        if (firstUse >= nextUsePos[best.code()]) {
            if (!spill())
                return false;
        } else {
            if (!assign(LGeneralReg(best)))
                return false;
        }
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
        if (mSuccessor->numPredecessors() <= 1)
            continue;

        IonSpew(IonSpew_LSRA, " Resolving control flow into block %d", i);

        
        
        BitSet *live = liveIn[i];
        for (size_t j = 0; j < successor->numPhis(); j++)
            live->insert(successor->getPhi(j)->getDef(0)->virtualRegister());

        for (BitSet::Iterator liveRegId(live->begin()); liveRegId != live->end(); liveRegId++) {
            IonSpew(IonSpew_LSRA, "  Inspecting live register %d", *liveRegId);
            VirtualRegister *reg = &vregs[*liveRegId];

            
            LPhi *phi = NULL;
            LiveInterval *to;
            if (reg->ins()->isPhi() &&
                reg->ins()->id() >= successor->firstId() &&
                reg->ins()->id() <= successor->lastId())
            {
                IonSpew(IonSpew_LSRA, "   Defined by phi");
                phi = reg->ins()->toPhi();
                to = reg->intervalFor(outputOf(successor->firstId()));
            } else {
                IonSpew(IonSpew_LSRA, "   Present at input");
                to = reg->intervalFor(inputOf(successor->firstId()));
            }
            JS_ASSERT(to);

            
            for (size_t j = 0; j < mSuccessor->numPredecessors(); j++) {
                MBasicBlock *mPredecessor = mSuccessor->getPredecessor(j);
                LBlock *predecessor = mPredecessor->lir();
                CodePosition predecessorEnd = outputOf(predecessor->lastId());
                LiveInterval *from = NULL;

                
                
                
                
                
                
                JS_ASSERT(mPredecessor->numSuccessors() == 1);

                
                if (phi) {
                    JS_ASSERT(mPredecessor->successorWithPhis() == successor->mir());

                    LAllocation *phiInput = phi->getOperand(mPredecessor->
                                                            positionInPhiSuccessor());
                    JS_ASSERT(phiInput->isUse());

                    IonSpew(IonSpew_LSRA, "   Known as register %u at phi input",
                            phiInput->toUse()->virtualRegister());

                    from = vregs[phiInput].intervalFor(predecessorEnd);
                } else {
                    from = reg->intervalFor(predecessorEnd);
                }

                
                JS_ASSERT(from);
                if (*from->getAllocation() != *to->getAllocation()) {
                    IonSpew(IonSpew_LSRA, "    Inserting move");
                    if (!moveBefore(predecessorEnd, from, to))
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
    
    JS_ASSERT(inactive.empty());
    for (IntervalIterator i(active.begin()); i != active.end(); ) {
        LiveInterval *interval = *i;
        i = active.removeAt(i);
        unhandled.enqueue(interval);
    }
    for (IntervalIterator i(handled.begin()); i != handled.end(); ) {
        LiveInterval *interval = *i;
        i = handled.removeAt(i);
        unhandled.enqueue(interval);
    }

    
    uint32 last = 1;
    RegisterSet freeRegs(RegisterSet::All());
    LiveInterval *interval;
    while ((interval = unhandled.dequeue()) != NULL) {
        VirtualRegister *reg = interval->reg();

        IonSpew(IonSpew_LSRA, " Reifying interval %u = [%u,%u]", reg->reg(), interval->start(),
                interval->end());

        
        for (uint32 i = last; i <= interval->start().ins(); i++) {
            if (vregs[i].inputMoves())
                vregs[i].inputMoves()->setFreeRegisters(freeRegs);
            if (vregs[i].outputMoves())
                vregs[i].outputMoves()->setFreeRegisters(freeRegs);
        }
        last = interval->start().ins() + 1;

        
        for (IntervalIterator i(active.begin()); i != active.end(); ) {
            if (i->end() < interval->start()) {
                if (i->getAllocation()->isRegister())
                    freeRegs.add(i->getAllocation()->toRegister());
                i = active.removeAt(i);
            } else if (i->covers(interval->start())) {
                i++;
            } else {
                if (i->getAllocation()->isRegister())
                    freeRegs.add(i->getAllocation()->toRegister());
                LiveInterval *save = *i;
                i = active.removeAt(i);
                inactive.insert(save);
            }
        }
        for (IntervalIterator i(inactive.begin()); i != inactive.end(); ) {
            if (i->end() < interval->start()) {
                i = active.removeAt(i);
                if (i->getAllocation()->isRegister())
                    freeRegs.add(i->getAllocation()->toRegister());
            } else if (i->covers(interval->start())) {
                if (i->getAllocation()->isRegister())
                    freeRegs.take(i->getAllocation()->toRegister());
                LiveInterval *save = *i;
                i = inactive.removeAt(i);
                active.insert(save);
            } else {
                i++;
            }
        }
        active.insert(interval);
        if (interval->getAllocation()->isRegister())
            freeRegs.take(interval->getAllocation()->toRegister());

        
        for (size_t i = 0; i < reg->numUses(); i++) {
            LOperand *use = reg->getUse(i);
            LAllocation *alloc = use->use;
            CodePosition pos = inputOf(use->ins);
            if (interval->covers(pos))
                *alloc = *interval->getAllocation();
        }

        if (interval->index() == 0) {
            
            reg->def()->setOutput(*interval->getAllocation());
        } else {
            
            LiveInterval *from = reg->getInterval(interval->index() - 1);
            if (!moveBefore(interval->start(), from, interval))
                return false;
        }
    }

    
    for (size_t i = 0; i < graph.numBlocks(); i++)
        graph.getBlock(i)->clearPhis();

    
    graph.setLocalSlotCount(stackAssignment.stackHeight());

    return true;
}





bool
LinearScanAllocator::splitInterval(LiveInterval *interval, CodePosition pos)
{
    
    
    JS_ASSERT(interval->start() <= pos && pos <= interval->end());

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

    IonSpew(IonSpew_LSRA, "  Split interval to %u = [%u, %u]",
            interval->reg()->reg(), interval->start().pos(),
            interval->end().pos());
    IonSpew(IonSpew_LSRA, "  Created new interval %u = [%u, %u]",
            newInterval->reg()->reg(), newInterval->start().pos(),
            newInterval->end().pos());

    
    
    
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
        JS_ASSERT(toSplit != current->start());
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

                LiveInterval *it = *i;
                if (it->start() == current->start()) {
                    
                    
                    
                    
                    
                    
                    JS_ASSERT(!it->hasFlag(LiveInterval::FIXED));

                    it->setAllocation(LUse(it->reg()->reg(), LUse::ANY));
                    active.removeAt(i);
                    unhandled.enqueue(it);
                    break;
                } else {
                    JS_ASSERT(it->covers(current->start()));
                    JS_ASSERT(it->start() != current->start());

                    if (!splitInterval(it, current->start()))
                        return false;

                    active.removeAt(i);
                    finishInterval(it);
                    break;
                }
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
    }

    active.insert(current);

    return true;
}

bool
LinearScanAllocator::spill()
{
    IonSpew(IonSpew_LSRA, "  Decided to spill current interval");

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

    if (alloc->isStackSlot()) {
        if (alloc->toStackSlot()->isDouble())
            stackAssignment.freeDoubleSlot(alloc->toStackSlot()->slot());
        else
            stackAssignment.freeSlot(alloc->toStackSlot()->slot());
    }

#ifdef DEBUG
    handled.insert(interval);
#endif
}







Register
LinearScanAllocator::findBestFreeRegister()
{
    
    IonSpew(IonSpew_LSRA, "  Computing freeUntilPos");
    for (size_t i = 0; i < Registers::Total; i++) {
        if (allowedRegs.has(Register::FromCode(i)))
            freeUntilPos[i] = CodePosition::MAX;
        else
            freeUntilPos[i] = CodePosition::MIN;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            freeUntilPos[reg.code()] = CodePosition::MIN;
            IonSpew(IonSpew_LSRA, "   Register %u not free", reg.code());
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            CodePosition pos = current->intersect(*i);
            if (pos != CodePosition::MIN && pos < freeUntilPos[reg.code()]) {
                freeUntilPos[reg.code()] = pos;
                IonSpew(IonSpew_LSRA, "   Register %u free until %u", reg.code(), pos.pos());
            }
        }
    }

    
    freeRegs.clear();
    Register best = Register::FromCode(0);
    for (uint32 i = 0; i < Registers::Total; i++) {
        Register reg = Register::FromCode(i);
        freeRegs.add(reg);
        if (freeUntilPos[i] > freeUntilPos[best.code()])
            best = reg;
    }

    return best;
}







Register
LinearScanAllocator::findBestBlockedRegister()
{
    
    IonSpew(IonSpew_LSRA, "  Computing nextUsePos");
    for (size_t i = 0; i < Registers::Total; i++) {
        if (allowedRegs.has(Register::FromCode(i)))
            nextUsePos[i] = CodePosition::MAX;
        else
            nextUsePos[i] = CodePosition::MIN;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            CodePosition nextUse = i->reg()->nextUseAfter(current->start());
            IonSpew(IonSpew_LSRA, "   Register %u next used %u", reg.code(), nextUse.pos());

            if (i->start() == current->start()) {
                IonSpew(IonSpew_LSRA, "    Disqualifying due to recency.");
                nextUsePos[reg.code()] = current->start();
            } else {
                nextUsePos[reg.code()] = nextUse;
            }
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        if (i->getAllocation()->isGeneralReg()) {
            Register reg = i->getAllocation()->toGeneralReg()->reg();
            CodePosition pos = i->reg()->nextUseAfter(current->start());
            JS_ASSERT(i->covers(pos) || i->end() == pos);
            if (pos < nextUsePos[reg.code()]) {
                nextUsePos[reg.code()] = pos;
                IonSpew(IonSpew_LSRA, "   Register %u next used %u", reg.code(), pos.pos());
            }
        }
    }

    
    Register best = Register::FromCode(0);
    for (size_t i = 0; i < Registers::Total; i++) {
        if (nextUsePos[i] > nextUsePos[best.code()])
            best = Register::FromCode(i);
    }
    IonSpew(IonSpew_LSRA, "  Decided best register was %u", best.code());
    return best;
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
LinearScanAllocator::UnhandledQueue::enqueue(LiveInterval *interval)
{
    IntervalIterator i(begin());
    for (; i != end(); i++) {
        if (i->start() < interval->start())
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
