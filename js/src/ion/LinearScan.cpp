








































#include <limits.h>
#include "BitSet.h"
#include "LinearScan.h"
#include "IonLIR-inl.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

LiveInterval *
LiveInterval::New(VirtualRegister *reg)
{
    LiveInterval *result = new LiveInterval(reg);
    return result;
}

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
RegisterAllocator::createDataStructures()
{
    allowedRegs = RegisterSet::All();

    liveIn = lir->mir()->allocate<BitSet*>(graph.numBlocks());
    freeUntilPos = lir->mir()->allocate<CodePosition>(RegisterCodes::Total);
    nextUsePos = lir->mir()->allocate<CodePosition>(RegisterCodes::Total);
    if (!liveIn || !freeUntilPos || !nextUsePos)
        return false;

    
    vregs = new VirtualRegister[graph.numVirtualRegisters()];

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *b = graph.getBlock(i);
        for (LInstructionIterator ins = b->begin(); ins != b->end(); ins++) {
            if (ins->numDefs()) {
                for (size_t j = 0; j < ins->numDefs(); j++) {
                    if (ins->getDef(j)->policy() != LDefinition::REDEFINED) {
                        uint32 reg = ins->getDef(j)->virtualRegister();
                        if (!vregs[reg].init(reg, b, *ins, ins->getDef(j)))
                            return false;
                    }
                }
            } else {
                if (!vregs[ins->id()].init(ins->id(), b, *ins, NULL))
                    return false;
            }
            if (ins->numTemps()) {
                for (size_t j = 0; j < ins->numTemps(); j++) {
                    uint32 reg = ins->getTemp(j)->virtualRegister();
                    if (!vregs[reg].init(reg, b, *ins, ins->getTemp(j)))
                        return false;
                }
            }
        }
        for (size_t j = 0; j < b->numPhis(); j++) {
            LPhi *phi = b->getPhi(j);
            uint32 reg = phi->getDef(0)->virtualRegister();
            if (!vregs[reg].init(phi->id(), b, phi, phi->getDef(0)))
                return false;
        }
    }

    return true;
}

bool
RegisterAllocator::buildLivenessInfo()
{
    for (size_t i = graph.numBlocks(); i > 0; i--) {
        LBlock *b = graph.getBlock(i - 1);
        MBasicBlock *mb = b->mir();

        BitSet *live = BitSet::New(graph.numVirtualRegisters());
        if (!live)
            return false;

        
        for (size_t i = 0; i < mb->lastIns()->numSuccessors(); i++) {
            MBasicBlock *successor = mb->lastIns()->getSuccessor(i);
            
            if (mb->id() < successor->id())
                live->insertAll(liveIn[successor->id()]);
        }

        
        if (mb->successorWithPhis()) {
            LBlock *phiSuccessor = mb->successorWithPhis()->lir();
            for (unsigned int j = 0; j < phiSuccessor->numPhis(); j++) {
                LPhi *phi = phiSuccessor->getPhi(j);
                LAllocation *use = phi->getOperand(mb->positionInPhiSuccessor());
                uint32 reg = use->toUse()->virtualRegister();
                live->insert(reg);
            }
        }

        
        
        for (BitSet::Iterator i(live->begin()); i != live->end(); i++)
            vregs[*i].getInterval(0)->addRange(outputOf(b->firstId()), inputOf(b->lastId() + 1));

        
        
        for (LInstructionReverseIterator ins = b->rbegin();
             ins != b->rend();
             ins++)
        {
            for (size_t i = 0; i < ins->numDefs(); i++) {
                if (ins->getDef(i)->policy() != LDefinition::REDEFINED) {
                    uint32 reg = ins->getDef(i)->virtualRegister();
                    vregs[reg].getInterval(0)->setFrom(outputOf(*ins));
                    live->remove(reg);
                }
            }
            for (size_t i = 0; i < ins->numTemps(); i++) {
                uint32 reg = ins->getTemp(i)->virtualRegister();
                vregs[reg].getInterval(0)->addRange(outputOf(*ins), outputOf(*ins));
            }
            for (LInstruction::InputIterator alloc(**ins);
                 alloc.more();
                 alloc.next())
            {
                if (alloc->isUse()) {
                    LUse *use = alloc->toUse();
                    uint32 reg = use->virtualRegister();

                    if (use->policy() != LUse::KEEPALIVE)
                        vregs[reg].addUse(LOperand(use, *ins));

                    if (ins->id() == b->firstId())
                        vregs[reg].getInterval(0)->addRange(inputOf(*ins), inputOf(*ins));
                    else
                        vregs[reg].getInterval(0)->addRange(outputOf(b->firstId()), inputOf(*ins));
                    live->insert(reg);
                }
            }
        }

        
        
        
        for (unsigned int i = 0; i < b->numPhis();) {
            if (live->contains(b->getPhi(i)->getDef(0)->virtualRegister())) {
                live->remove(b->getPhi(i)->getDef(0)->virtualRegister());
                i++;
            } else {
                
                
                b->removePhi(i);
            }
        }

        
        
        if (mb->isLoopHeader()) {
            MBasicBlock *backedge = mb->backedge();
            for (BitSet::Iterator i(live->begin()); i != live->end(); i++)
                vregs[*i].getInterval(0)->addRange(outputOf(b->firstId()),
                                                   outputOf(backedge->lir()->lastId()));
        }

        liveIn[mb->id()] = live;
    }

    return true;
}

bool
RegisterAllocator::allocateRegisters()
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
            JS_ASSERT(it->getAllocation()->isGeneralReg());
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
            JS_ASSERT(it->getAllocation()->isGeneralReg());
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

#ifdef DEBUG
        
        for (IntervalIterator i(active.begin()); i != active.end(); i++) {
            JS_ASSERT(i->getAllocation()->isGeneralReg());
            JS_ASSERT(i->numRanges() > 0);
            JS_ASSERT(i->covers(position));

            for (IntervalIterator j(active.begin()); j != i; j++)
                JS_ASSERT(canCoexist(*i, *j));
        }

        for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
            JS_ASSERT(i->getAllocation()->isGeneralReg());
            JS_ASSERT(i->numRanges() > 0);
            JS_ASSERT(i->end() >= position);
            JS_ASSERT(!i->covers(position));

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
                JS_ASSERT(i->end() < position);
                JS_ASSERT(!i->covers(position));
            }

            for (IntervalIterator j(active.begin()); j != active.end(); j++)
                JS_ASSERT(*i != *j);
            for (IntervalIterator j(inactive.begin()); j != inactive.end(); j++)
                JS_ASSERT(*i != *j);
        }
#endif

        
        bool mustHaveRegister = false;
        if (position == current->reg()->getFirstInterval()->start()) {
            JS_ASSERT(position.subpos() == CodePosition::OUTPUT);

            LDefinition *def = current->reg()->def();
            LDefinition::Policy pol = current->reg()->def()->policy();
            if (pol == LDefinition::PRESET) {
                IonSpew(IonSpew_LSRA, " Definition has preset policy.");
                current->setFlag(LiveInterval::FIXED);
                if (!assign(*def->output()))
                    return false;
                continue;
            }
            if (pol == LDefinition::MUST_REUSE_INPUT) {
                IonSpew(IonSpew_LSRA, " Definition has 'must reuse input' policy.");
                LInstruction *ins = current->reg()->ins();
                JS_ASSERT(ins->numOperands() > 0);
                JS_ASSERT(ins->getOperand(0)->isUse());
                uint32 inputReg = ins->getOperand(0)->toUse()->virtualRegister();
                LiveInterval *inputInterval = vregs[inputReg].intervalFor(inputOf(ins));
                JS_ASSERT(inputInterval);
                JS_ASSERT(inputInterval->getAllocation()->isGeneralReg());
                if (!assign(*inputInterval->getAllocation()))
                    return false;
                continue;
            }
            if (pol == LDefinition::DEFAULT)
                mustHaveRegister = !current->reg()->ins()->isPhi();
            else
                JS_ASSERT(pol == LDefinition::REDEFINED);
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

#ifdef DEBUG
    
    for (IntervalIterator i(handled.begin()); i != handled.end(); i++) {
        for (IntervalIterator j(handled.begin()); j != i; j++) {
            JS_ASSERT(*i != *j);
            JS_ASSERT(canCoexist(*i, *j));
        }
    }

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *b = graph.getBlock(i);
        for (LInstructionIterator ins = b->begin(); ins != b->end(); ins++) {
            if (ins->isMove()) {
                LMove *move = ins->toMove();
                for (size_t j = 0; j < move->numEntries(); j++) {
                    LMove::Entry *entry = move->getEntry(j);
                    JS_ASSERT(!entry->from->isUse());
                    JS_ASSERT(!entry->to->isUse());
                }
            }
        }
    }
#endif

    return true;
}

bool
RegisterAllocator::resolveControlFlow()
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

                    from = vregs[phiInput->toUse()->virtualRegister()].intervalFor(predecessorEnd);
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
RegisterAllocator::reifyAllocations()
{
    
    return true;
}

bool
RegisterAllocator::splitInterval(LiveInterval *interval, CodePosition pos)
{
    
    
    JS_ASSERT(interval->start() <= pos && pos <= interval->end());

    VirtualRegister *reg = interval->reg();

    
    LiveInterval *newInterval = LiveInterval::New(reg);
    if (!interval->splitFrom(pos, newInterval))
        return false;

    JS_ASSERT(interval->numRanges() > 0);
    JS_ASSERT(newInterval->numRanges() > 0);

    if (!reg->addInterval(newInterval))
        return false;

    if (!moveBefore(pos, interval, newInterval))
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
RegisterAllocator::assign(LAllocation allocation)
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
                    handled.insert(it);
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

    if (allocation.isGeneralReg())
        active.insert(current);
    else
        finishInterval(current);

    return true;
}

bool
RegisterAllocator::spill()
{
    IonSpew(IonSpew_LSRA, "  Decided to spill current interval");

    uint32 stackSlot;
    if (!stackAssignment.allocateSlot(&stackSlot))
        return false;

    IonSpew(IonSpew_LSRA, "  Allocated spill slot %u", stackSlot);

    return assign(LStackSlot(stackSlot));
}

void
RegisterAllocator::finishInterval(LiveInterval *interval)
{
    LAllocation *alloc = interval->getAllocation();
    JS_ASSERT(!alloc->isUse());

    if (alloc->isStackSlot()) {
        if (alloc->toStackSlot()->isDouble())
            stackAssignment.freeDoubleSlot(alloc->toStackSlot()->slot());
        else
            stackAssignment.freeSlot(alloc->toStackSlot()->slot());
    }

    handled.insert(interval);
}

Register
RegisterAllocator::findBestFreeRegister()
{
    
    IonSpew(IonSpew_LSRA, "  Computing freeUntilPos");
    for (size_t i = 0; i < RegisterCodes::Total; i++) {
        if (allowedRegs.has(Register::FromCode(i)))
            freeUntilPos[i] = CodePosition::MAX;
        else
            freeUntilPos[i] = CodePosition::MIN;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        Register reg = i->getAllocation()->toGeneralReg()->reg();
        freeUntilPos[reg.code()] = CodePosition::MIN;
        IonSpew(IonSpew_LSRA, "   Register %u not free", reg.code());
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        Register reg = i->getAllocation()->toGeneralReg()->reg();
        CodePosition pos = current->intersect(*i);
        if (pos != CodePosition::MIN && pos < freeUntilPos[reg.code()]) {
            freeUntilPos[reg.code()] = pos;
            IonSpew(IonSpew_LSRA, "   Register %u free until %u", reg.code(), pos.pos());
        }
    }

    
    Register best = Register::FromCode(0);
    for (uint32 i = 0; i < RegisterCodes::Total; i++) {
        if (freeUntilPos[i] > freeUntilPos[best.code()])
            best = Register::FromCode(i);
    }

    return best;
}

Register
RegisterAllocator::findBestBlockedRegister()
{
    
    IonSpew(IonSpew_LSRA, "  Computing nextUsePos");
    for (size_t i = 0; i < RegisterCodes::Total; i++) {
        if (allowedRegs.has(Register::FromCode(i)))
            nextUsePos[i] = CodePosition::MAX;
        else
            nextUsePos[i] = CodePosition::MIN;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
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
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        Register reg = i->getAllocation()->toGeneralReg()->reg();
        CodePosition pos = i->reg()->nextUseAfter(current->start());
        JS_ASSERT(i->covers(pos) || i->end() == pos);
        if (pos < nextUsePos[reg.code()]) {
            nextUsePos[reg.code()] = pos;
            IonSpew(IonSpew_LSRA, "   Register %u next used %u", reg.code(), pos.pos());
        }
    }

    
    Register best = Register::FromCode(0);
    for (size_t i = 0; i < RegisterCodes::Total; i++) {
        if (nextUsePos[i] > nextUsePos[best.code()])
            best = Register::FromCode(i);
    }
    IonSpew(IonSpew_LSRA, "  Decided best register was %u", best.code());
    return best;
}


bool
RegisterAllocator::canCoexist(LiveInterval *a, LiveInterval *b)
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

bool
RegisterAllocator::moveBefore(CodePosition pos, LiveInterval *from, LiveInterval *to)
{
    VirtualRegister *vreg = &vregs[pos.ins()];
    JS_ASSERT(vreg->ins());

    LMove *move = NULL;
    if (vreg->ins()->isPhi()) {
        move = new LMove;
        vreg->block()->insertBefore(*vreg->block()->begin(), move);
    } else {
        LInstructionReverseIterator riter(vreg->ins());
        riter++;
        if (riter != vreg->block()->rend() && riter->isMove()) {
            move = riter->toMove();
        } else {
            move = new LMove;
            vreg->block()->insertBefore(vreg->ins(), move);
        }
    }

    return move->add(from->getAllocation(), to->getAllocation());
}

bool
RegisterAllocator::go()
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
RegisterAllocator::UnhandledQueue::enqueue(LiveInterval *interval)
{
    IntervalIterator i(begin());
    for (; i != end(); i++) {
        if (i->start() < interval->start())
            break;
    }
    insertBefore(*i, interval);
}

LiveInterval *
RegisterAllocator::UnhandledQueue::dequeue()
{
    if (rbegin() == rend())
        return NULL;

    LiveInterval *result = *rbegin();
    remove(result);
    return result;
}
