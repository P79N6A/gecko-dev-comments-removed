






#include <limits.h>

#include "mozilla/DebugOnly.h"

#include "BitSet.h"
#include "LinearScan.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "LIR-inl.h"

using namespace js;
using namespace js::ion;

using mozilla::DebugOnly;





void
LinearScanAllocator::enqueueVirtualRegisterIntervals()
{
    
    IntervalReverseIterator curr = unhandled.rbegin();

    
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        LiveInterval *live = vregs[i].getInterval(0);
        if (live->numRanges() > 0) {
            setIntervalRequirement(live);

            
            for (; curr != unhandled.rend(); curr++) {
                if (curr->start() > live->start())
                    break;
            }

            
            
            unhandled.enqueueForward(*curr, live);
        }
    }
}





















bool
LinearScanAllocator::allocateRegisters()
{
    
    
    
    
    enqueueVirtualRegisterIntervals();
    unhandled.assertSorted();

    
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        if (fixedIntervals[i]->numRanges() > 0)
            fixed.pushBack(fixedIntervals[i]);
    }

    
    CodePosition prevPosition = CodePosition::MIN;
    while ((current = unhandled.dequeue()) != NULL) {
        JS_ASSERT(current->getAllocation()->isUse());
        JS_ASSERT(current->numRanges() > 0);

        if (mir->shouldCancel("LSRA Allocate Registers (main loop)"))
            return false;

        CodePosition position = current->start();
        const Requirement *req = current->requirement();
        const Requirement *hint = current->hint();

        IonSpew(IonSpew_RegAlloc, "Processing %d = [%u, %u] (pri=%d)",
                current->hasVreg() ? current->vreg() : 0, current->start().pos(),
                current->end().pos(), current->requirement()->priority());

        
        if (position != prevPosition) {
            JS_ASSERT(position > prevPosition);
            prevPosition = position;

            for (IntervalIterator i(active.begin()); i != active.end(); ) {
                LiveInterval *it = *i;
                JS_ASSERT(it->numRanges() > 0);

                if (it->end() <= position) {
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

                if (it->end() <= position) {
                    i = inactive.removeAt(i);
                    finishInterval(it);
                } else if (it->covers(position)) {
                    i = inactive.removeAt(i);
                    active.pushBack(it);
                } else {
                    i++;
                }
            }
        }

        
        validateIntervals();

        
        if (req->kind() == Requirement::FIXED) {
            JS_ASSERT(!req->allocation().isRegister());
            if (!assign(req->allocation()))
                return false;
            continue;
        }

        
        if (req->kind() != Requirement::REGISTER && hint->kind() == Requirement::NONE) {
            IonSpew(IonSpew_RegAlloc, "  Eagerly spilling virtual register %d",
                    current->hasVreg() ? current->vreg() : 0);
            if (!spill())
                return false;
            continue;
        }

        
        IonSpew(IonSpew_RegAlloc, " Attempting free register allocation");
        CodePosition bestFreeUntil;
        AnyRegister::Code bestCode = findBestFreeRegister(&bestFreeUntil);
        if (bestCode != AnyRegister::Invalid) {
            AnyRegister best = AnyRegister::FromCode(bestCode);
            IonSpew(IonSpew_RegAlloc, "  Decided best register was %s", best.name());

            
            if (bestFreeUntil < current->end()) {
                if (!splitInterval(current, bestFreeUntil))
                    return false;
            }
            if (!assign(LAllocation(best)))
                return false;

            continue;
        }

        IonSpew(IonSpew_RegAlloc, " Attempting blocked register allocation");

        
        
        
        CodePosition bestNextUsed;
        bestCode = findBestBlockedRegister(&bestNextUsed);
        if (bestCode != AnyRegister::Invalid &&
            (req->kind() == Requirement::REGISTER || hint->pos() < bestNextUsed))
        {
            AnyRegister best = AnyRegister::FromCode(bestCode);
            IonSpew(IonSpew_RegAlloc, "  Decided best register was %s", best.name());

            if (!splitBlockingIntervals(LAllocation(best)))
                return false;
            if (!assign(LAllocation(best)))
                return false;

            continue;
        }

        IonSpew(IonSpew_RegAlloc, "  No registers available to spill");
        JS_ASSERT(req->kind() == Requirement::NONE);

        if (!spill())
            return false;
    }

    validateAllocations();
    validateVirtualRegisters();

    return true;
}










bool
LinearScanAllocator::resolveControlFlow()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("LSRA Resolve Control Flow (main loop)"))
            return false;

        LBlock *successor = graph.getBlock(i);
        MBasicBlock *mSuccessor = successor->mir();
        if (mSuccessor->numPredecessors() < 1)
            continue;

        
        for (size_t j = 0; j < successor->numPhis(); j++) {
            LPhi *phi = successor->getPhi(j);
            JS_ASSERT(phi->numDefs() == 1);
            LinearScanVirtualRegister *vreg = &vregs[phi->getDef(0)];
            LiveInterval *to = vreg->intervalFor(inputOf(successor->firstId()));
            JS_ASSERT(to);

            for (size_t k = 0; k < mSuccessor->numPredecessors(); k++) {
                LBlock *predecessor = mSuccessor->getPredecessor(k)->lir();
                JS_ASSERT(predecessor->mir()->numSuccessors() == 1);

                LAllocation *input = phi->getOperand(predecessor->mir()->positionInPhiSuccessor());
                LiveInterval *from = vregs[input].intervalFor(outputOf(predecessor->lastId()));
                JS_ASSERT(from);

                LMoveGroup *moves = predecessor->getExitMoveGroup();
                if (!addMove(moves, from, to))
                    return false;
            }

            if (vreg->mustSpillAtDefinition() && !to->isSpill()) {
                
                LMoveGroup *moves = successor->getEntryMoveGroup();
                if (!moves->add(to->getAllocation(), vregs[to->vreg()].canonicalSpill()))
                    return false;
            }
        }

        
        BitSet *live = liveIn[mSuccessor->id()];

        for (BitSet::Iterator liveRegId(*live); liveRegId; liveRegId++) {
            LiveInterval *to = vregs[*liveRegId].intervalFor(inputOf(successor->firstId()));
            JS_ASSERT(to);

            for (size_t j = 0; j < mSuccessor->numPredecessors(); j++) {
                LBlock *predecessor = mSuccessor->getPredecessor(j)->lir();
                LiveInterval *from = vregs[*liveRegId].intervalFor(outputOf(predecessor->lastId()));
                JS_ASSERT(from);

                if (mSuccessor->numPredecessors() > 1) {
                    JS_ASSERT(predecessor->mir()->numSuccessors() == 1);
                    LMoveGroup *moves = predecessor->getExitMoveGroup();
                    if (!addMove(moves, from, to))
                        return false;
                } else {
                    LMoveGroup *moves = successor->getEntryMoveGroup();
                    if (!addMove(moves, from, to))
                        return false;
                }
            }
        }
    }

    return true;
}

bool
LinearScanAllocator::moveInputAlloc(CodePosition pos, LAllocation *from, LAllocation *to)
{
    if (*from == *to)
        return true;
    LMoveGroup *moves = getInputMoveGroup(pos);
    return moves->add(from, to);
}






bool
LinearScanAllocator::reifyAllocations()
{
    
    for (size_t j = 1; j < graph.numVirtualRegisters(); j++) {
        LinearScanVirtualRegister *reg = &vregs[j];
        if (mir->shouldCancel("LSRA Reification (main loop)"))
            return false;

    for (size_t k = 0; k < reg->numIntervals(); k++) {
        LiveInterval *interval = reg->getInterval(k);
        JS_ASSERT(reg == &vregs[interval->vreg()]);
        if (!interval->numRanges())
            continue;

        UsePositionIterator usePos(interval->usesBegin());
        for (; usePos != interval->usesEnd(); usePos++) {
            if (usePos->use->isFixedRegister()) {
                LiveInterval *to = fixedIntervals[GetFixedRegister(reg->def(), usePos->use).code()];

                *static_cast<LAllocation *>(usePos->use) = *to->getAllocation();
                if (!moveInput(usePos->pos, interval, to))
                    return false;
            } else {
                JS_ASSERT(UseCompatibleWith(usePos->use, *interval->getAllocation()));
                *static_cast<LAllocation *>(usePos->use) = *interval->getAllocation();
            }
        }

        
        if (interval->index() == 0)
        {
            LDefinition *def = reg->def();
            LAllocation *spillFrom;

            if (def->policy() == LDefinition::PRESET && def->output()->isRegister()) {
                AnyRegister fixedReg = def->output()->toRegister();
                LiveInterval *from = fixedIntervals[fixedReg.code()];
                if (!moveAfter(outputOf(reg->ins()), from, interval))
                    return false;
                spillFrom = from->getAllocation();
            } else {
                if (def->policy() == LDefinition::MUST_REUSE_INPUT) {
                    LAllocation *alloc = reg->ins()->getOperand(def->getReusedInput());
                    LAllocation *origAlloc = LAllocation::New(*alloc);

                    JS_ASSERT(!alloc->isUse());

                    *alloc = *interval->getAllocation();
                    if (!moveInputAlloc(inputOf(reg->ins()), origAlloc, alloc))
                        return false;
                }

                JS_ASSERT(DefinitionCompatibleWith(reg->ins(), def, *interval->getAllocation()));
                def->setOutput(*interval->getAllocation());

                spillFrom = interval->getAllocation();
            }

            if (reg->ins()->recoversInput()) {
                LSnapshot *snapshot = reg->ins()->snapshot();
                for (size_t i = 0; i < snapshot->numEntries(); i++) {
                    LAllocation *entry = snapshot->getEntry(i);
                    if (entry->isUse() && entry->toUse()->policy() == LUse::RECOVERED_INPUT)
                        *entry = *def->output();
                }
            }

            if (reg->mustSpillAtDefinition() && !reg->ins()->isPhi() &&
                (*reg->canonicalSpill() != *spillFrom))
            {
                
                
                
                
                LMoveGroup *moves = getMoveGroupAfter(outputOf(reg->ins()));
                if (!moves->add(spillFrom, reg->canonicalSpill()))
                    return false;
            }
        }
        else if (interval->start() > inputOf(insData[interval->start()].block()->firstId()) &&
                 (!reg->canonicalSpill() ||
                  (reg->canonicalSpill() == interval->getAllocation() &&
                   !reg->mustSpillAtDefinition()) ||
                  *reg->canonicalSpill() != *interval->getAllocation()))
        {
            
            
            
            
            
            
            
            
            
            LiveInterval *prevInterval = reg->getInterval(interval->index() - 1);
            CodePosition start = interval->start();
            InstructionData *data = &insData[start];

            JS_ASSERT(start == inputOf(data->ins()) || start == outputOf(data->ins()));

            if (start.subpos() == CodePosition::INPUT) {
                if (!moveInput(inputOf(data->ins()), prevInterval, interval))
                    return false;
            } else {
                if (!moveAfter(outputOf(data->ins()), prevInterval, interval))
                    return false;
            }

            
            if (reg->canonicalSpill() == interval->getAllocation() &&
                !reg->mustSpillAtDefinition())
            {
                reg->setSpillPosition(interval->start());
            }
        }

        addLiveRegistersForInterval(reg, interval);
    }} 

    
    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());

    return true;
}

inline bool
LinearScanAllocator::isSpilledAt(LiveInterval *interval, CodePosition pos)
{
    LinearScanVirtualRegister *reg = &vregs[interval->vreg()];
    if (!reg->canonicalSpill() || !reg->canonicalSpill()->isStackSlot())
        return false;

    if (reg->mustSpillAtDefinition()) {
        JS_ASSERT(reg->spillPosition() <= pos);
        return true;
    }

    return interval->getAllocation() == reg->canonicalSpill();
}

bool
LinearScanAllocator::populateSafepoints()
{
    size_t firstSafepoint = 0;

    for (uint32_t i = 0; i < vregs.numVirtualRegisters(); i++) {
        LinearScanVirtualRegister *reg = &vregs[i];

        if (!reg->def() || (!IsTraceable(reg) && !IsNunbox(reg)))
            continue;

        firstSafepoint = findFirstSafepoint(reg->getInterval(0), firstSafepoint);
        if (firstSafepoint >= graph.numSafepoints())
            break;

        
        size_t lastInterval = reg->numIntervals() - 1;
        CodePosition end = reg->getInterval(lastInterval)->end();

        for (size_t j = firstSafepoint; j < graph.numSafepoints(); j++) {
            LInstruction *ins = graph.getSafepoint(j);

            
            
            if (end < inputOf(ins))
                break;

            
            
            
            if (ins == reg->ins() && !reg->isTemp()) {
                DebugOnly<LDefinition*> def = reg->def();
                JS_ASSERT_IF(def->policy() == LDefinition::MUST_REUSE_INPUT,
                             def->type() == LDefinition::GENERAL || def->type() == LDefinition::DOUBLE);
                continue;
            }

            LSafepoint *safepoint = ins->safepoint();

            if (!IsNunbox(reg)) {
                JS_ASSERT(IsTraceable(reg));

                LiveInterval *interval = reg->intervalFor(inputOf(ins));
                if (!interval)
                    continue;

                LAllocation *a = interval->getAllocation();
                if (a->isGeneralReg() && !ins->isCall()) {
#ifdef JS_PUNBOX64
                    if (reg->type() == LDefinition::BOX) {
                        safepoint->addValueRegister(a->toGeneralReg()->reg());
                    } else
#endif
                    {
                        safepoint->addGcRegister(a->toGeneralReg()->reg());
                    }
                }

                if (isSpilledAt(interval, inputOf(ins))) {
#ifdef JS_PUNBOX64
                    if (reg->type() == LDefinition::BOX) {
                        if (!safepoint->addValueSlot(reg->canonicalSpillSlot()))
                            return false;
                    } else
#endif
                    {
                        if (!safepoint->addGcSlot(reg->canonicalSpillSlot()))
                            return false;
                    }
                }
#ifdef JS_NUNBOX32
            } else {
                LinearScanVirtualRegister *other = otherHalfOfNunbox(reg);
                LinearScanVirtualRegister *type = (reg->type() == LDefinition::TYPE) ? reg : other;
                LinearScanVirtualRegister *payload = (reg->type() == LDefinition::PAYLOAD) ? reg : other;
                LiveInterval *typeInterval = type->intervalFor(inputOf(ins));
                LiveInterval *payloadInterval = payload->intervalFor(inputOf(ins));

                if (!typeInterval && !payloadInterval)
                    continue;

                LAllocation *typeAlloc = typeInterval->getAllocation();
                LAllocation *payloadAlloc = payloadInterval->getAllocation();

                
                
                
                if (payloadAlloc->isArgument())
                    continue;

                if (isSpilledAt(typeInterval, inputOf(ins)) &&
                    isSpilledAt(payloadInterval, inputOf(ins)))
                {
                    
                    
                    uint32_t payloadSlot = payload->canonicalSpillSlot();
                    uint32_t slot = BaseOfNunboxSlot(LDefinition::PAYLOAD, payloadSlot);
                    if (!safepoint->addValueSlot(slot))
                        return false;
                }

                if (!ins->isCall() &&
                    (!isSpilledAt(typeInterval, inputOf(ins)) || payloadAlloc->isGeneralReg()))
                {
                    
                    
                    
                    
                    if (!safepoint->addNunboxParts(*typeAlloc, *payloadAlloc))
                        return false;
                }
#endif
            }
        }

#ifdef JS_NUNBOX32
        if (IsNunbox(reg)) {
            
            
            JS_ASSERT(&vregs[reg->def()->virtualRegister() + 1] == otherHalfOfNunbox(reg));
            i++;
        }
#endif
    }

    return true;
}





bool
LinearScanAllocator::splitInterval(LiveInterval *interval, CodePosition pos)
{
    
    
    JS_ASSERT(interval->start() < pos && pos < interval->end());

    LinearScanVirtualRegister *reg = &vregs[interval->vreg()];

    
    JS_ASSERT(reg);

    
    LiveInterval *newInterval = new LiveInterval(interval->vreg(), interval->index() + 1);
    if (!interval->splitFrom(pos, newInterval))
        return false;

    JS_ASSERT(interval->numRanges() > 0);
    JS_ASSERT(newInterval->numRanges() > 0);

    if (!reg->addInterval(newInterval))
        return false;

    IonSpew(IonSpew_RegAlloc, "  Split interval to %u = [%u, %u]/[%u, %u]",
            interval->vreg(), interval->start().pos(),
            interval->end().pos(), newInterval->start().pos(),
            newInterval->end().pos());

    
    
    
    setIntervalRequirement(newInterval);

    
    
    
    unhandled.enqueueBackward(newInterval);

    return true;
}

bool
LinearScanAllocator::splitBlockingIntervals(LAllocation allocation)
{
    JS_ASSERT(allocation.isRegister());

    
    LiveInterval *fixed = fixedIntervals[allocation.toRegister().code()];
    if (fixed->numRanges() > 0) {
        CodePosition fixedPos = current->intersect(fixed);
        if (fixedPos != CodePosition::MIN) {
            JS_ASSERT(fixedPos < current->end());
            if (!splitInterval(current, fixedPos))
                return false;
        }
    }

    
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isRegister() && *i->getAllocation() == allocation) {
            IonSpew(IonSpew_RegAlloc, " Splitting active interval %u = [%u, %u]",
                    vregs[i->vreg()].ins()->id(), i->start().pos(), i->end().pos());

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
        if (i->getAllocation()->isRegister() && *i->getAllocation() == allocation) {
            IonSpew(IonSpew_RegAlloc, " Splitting inactive interval %u = [%u, %u]",
                    vregs[i->vreg()].ins()->id(), i->start().pos(), i->end().pos());

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

    return true;
}





bool
LinearScanAllocator::assign(LAllocation allocation)
{
    if (allocation.isRegister())
        IonSpew(IonSpew_RegAlloc, "Assigning register %s", allocation.toRegister().name());
    current->setAllocation(allocation);

    
    LinearScanVirtualRegister *reg = &vregs[current->vreg()];
    if (reg) {
        CodePosition splitPos = current->firstIncompatibleUse(allocation);
        if (splitPos != CodePosition::MAX) {
            
            
            
            splitPos = splitPos.previous();
            JS_ASSERT (splitPos < current->end());
            if (!splitInterval(current, splitPos))
                return false;
        }
    }

    if (reg && allocation.isMemory()) {
        if (reg->canonicalSpill()) {
            JS_ASSERT(allocation == *reg->canonicalSpill());

            
            
            reg->setSpillAtDefinition(outputOf(reg->ins()));
        } else {
            reg->setCanonicalSpill(current->getAllocation());

            
            
            InstructionData *other = &insData[current->start()];
            uint32_t loopDepthAtDef = reg->block()->mir()->loopDepth();
            uint32_t loopDepthAtSpill = other->block()->mir()->loopDepth();
            if (loopDepthAtSpill > loopDepthAtDef)
                reg->setSpillAtDefinition(outputOf(reg->ins()));
        }
    }

    active.pushBack(current);

    return true;
}

uint32_t
LinearScanAllocator::allocateSlotFor(const LiveInterval *interval)
{
    LinearScanVirtualRegister *reg = &vregs[interval->vreg()];

    SlotList *freed;
    if (reg->type() == LDefinition::DOUBLE || IsNunbox(reg))
        freed = &finishedDoubleSlots_;
    else
        freed = &finishedSlots_;

    if (!freed->empty()) {
        LiveInterval *maybeDead = freed->back();
        if (maybeDead->end() < reg->getInterval(0)->start()) {
            
            
            
            
            
            
            
            
            
            freed->popBack();
            LinearScanVirtualRegister *dead = &vregs[maybeDead->vreg()];
#ifdef JS_NUNBOX32
            if (IsNunbox(dead))
                return BaseOfNunboxSlot(dead->type(), dead->canonicalSpillSlot());
#endif
            return dead->canonicalSpillSlot();
        }
    }

    if (IsNunbox(reg))
        return stackSlotAllocator.allocateValueSlot();
    if (reg->isDouble())
        return stackSlotAllocator.allocateDoubleSlot();
    return stackSlotAllocator.allocateSlot();
}

bool
LinearScanAllocator::spill()
{
    IonSpew(IonSpew_RegAlloc, "  Decided to spill current interval");

    
    JS_ASSERT(current->hasVreg());

    LinearScanVirtualRegister *reg = &vregs[current->vreg()];

    if (reg->canonicalSpill()) {
        IonSpew(IonSpew_RegAlloc, "  Allocating canonical spill location");

        return assign(*reg->canonicalSpill());
    }

    uint32_t stackSlot;
#if defined JS_NUNBOX32
    if (IsNunbox(reg)) {
        LinearScanVirtualRegister *other = otherHalfOfNunbox(reg);

        if (other->canonicalSpill()) {
            
            
            
            JS_ASSERT(other->canonicalSpill()->isStackSlot());
            stackSlot = BaseOfNunboxSlot(other->type(), other->canonicalSpillSlot());
        } else {
            
            
            stackSlot = allocateSlotFor(current);
        }
        stackSlot -= OffsetOfNunboxSlot(reg->type());
    } else
#endif
    {
        stackSlot = allocateSlotFor(current);
    }
    JS_ASSERT(stackSlot <= stackSlotAllocator.stackHeight());

    return assign(LStackSlot(stackSlot, reg->isDouble()));
}

void
LinearScanAllocator::freeAllocation(LiveInterval *interval, LAllocation *alloc)
{
    LinearScanVirtualRegister *mine = &vregs[interval->vreg()];
    if (!IsNunbox(mine)) {
        if (alloc->isStackSlot()) {
            if (alloc->toStackSlot()->isDouble())
                finishedDoubleSlots_.append(interval);
            else
                finishedSlots_.append(interval);
        }
        return;
    }

#ifdef JS_NUNBOX32
    
    
    LinearScanVirtualRegister *other = otherHalfOfNunbox(mine);
    if (other->finished()) {
        if (!mine->canonicalSpill() && !other->canonicalSpill())
            return;

        JS_ASSERT_IF(mine->canonicalSpill() && other->canonicalSpill(),
                     mine->canonicalSpill()->isStackSlot() == other->canonicalSpill()->isStackSlot());

        LinearScanVirtualRegister *candidate = mine->canonicalSpill() ? mine : other;
        if (!candidate->canonicalSpill()->isStackSlot())
            return;

        finishedDoubleSlots_.append(candidate->lastInterval());
    }
#endif
}

void
LinearScanAllocator::finishInterval(LiveInterval *interval)
{
    LAllocation *alloc = interval->getAllocation();
    JS_ASSERT(!alloc->isUse());

    
    if (!interval->hasVreg())
        return;

    LinearScanVirtualRegister *reg = &vregs[interval->vreg()];

    
    JS_ASSERT_IF(alloc->isStackSlot(), *alloc == *reg->canonicalSpill());

    bool lastInterval = interval->index() == (reg->numIntervals() - 1);
    if (lastInterval) {
        freeAllocation(interval, alloc);
        reg->setFinished();
    }

    handled.pushBack(interval);
}






AnyRegister::Code
LinearScanAllocator::findBestFreeRegister(CodePosition *freeUntil)
{
    IonSpew(IonSpew_RegAlloc, "  Computing freeUntilPos");

    
    CodePosition freeUntilPos[AnyRegister::Total];
    bool needFloat = vregs[current->vreg()].isDouble();
    for (AnyRegisterIterator regs(allRegisters_); regs.more(); regs++) {
        AnyRegister reg = *regs;
        if (reg.isFloat() == needFloat)
            freeUntilPos[reg.code()] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isRegister()) {
            AnyRegister reg = i->getAllocation()->toRegister();
            IonSpew(IonSpew_RegAlloc, "   Register %s not free", reg.name());
            freeUntilPos[reg.code()] = CodePosition::MIN;
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        if (i->getAllocation()->isRegister()) {
            AnyRegister reg = i->getAllocation()->toRegister();
            CodePosition pos = current->intersect(*i);
            if (pos != CodePosition::MIN && pos < freeUntilPos[reg.code()]) {
                freeUntilPos[reg.code()] = pos;
                IonSpew(IonSpew_RegAlloc, "   Register %s free until %u", reg.name(), pos.pos());
            }
        }
    }

    CodePosition fixedPos = fixedIntervalsUnion->intersect(current);
    if (fixedPos != CodePosition::MIN) {
        for (IntervalIterator i(fixed.begin()); i != fixed.end(); i++) {
            AnyRegister reg = i->getAllocation()->toRegister();
            if (freeUntilPos[reg.code()] != CodePosition::MIN) {
                CodePosition pos = current->intersect(*i);
                if (pos != CodePosition::MIN && pos < freeUntilPos[reg.code()]) {
                    freeUntilPos[reg.code()] = (pos == current->start()) ? CodePosition::MIN : pos;
                    IonSpew(IonSpew_RegAlloc, "   Register %s free until %u", reg.name(), pos.pos());
                }
            }
        }
    }

    AnyRegister::Code bestCode = AnyRegister::Invalid;
    if (current->index()) {
        
        
        LiveInterval *previous = vregs[current->vreg()].getInterval(current->index() - 1);
        if (previous->getAllocation()->isRegister()) {
            AnyRegister prevReg = previous->getAllocation()->toRegister();
            if (freeUntilPos[prevReg.code()] != CodePosition::MIN)
                bestCode = prevReg.code();
        }
    }

    
    const Requirement *hint = current->hint();
    if (hint->kind() == Requirement::FIXED && hint->allocation().isRegister()) {
        AnyRegister hintReg = hint->allocation().toRegister();
        if (freeUntilPos[hintReg.code()] > hint->pos())
            bestCode = hintReg.code();
    } else if (hint->kind() == Requirement::SAME_AS_OTHER) {
        LiveInterval *other = vregs[hint->virtualRegister()].intervalFor(hint->pos());
        if (other && other->getAllocation()->isRegister()) {
            AnyRegister hintReg = other->getAllocation()->toRegister();
            if (freeUntilPos[hintReg.code()] > hint->pos())
                bestCode = hintReg.code();
        }
    }

    if (bestCode == AnyRegister::Invalid) {
        
        for (uint32_t i = 0; i < AnyRegister::Total; i++) {
            if (freeUntilPos[i] == CodePosition::MIN)
                continue;
            if (bestCode == AnyRegister::Invalid || freeUntilPos[i] > freeUntilPos[bestCode])
                bestCode = AnyRegister::Code(i);
        }
    }

    if (bestCode != AnyRegister::Invalid)
        *freeUntil = freeUntilPos[bestCode];
    return bestCode;
}







AnyRegister::Code
LinearScanAllocator::findBestBlockedRegister(CodePosition *nextUsed)
{
    IonSpew(IonSpew_RegAlloc, "  Computing nextUsePos");

    
    CodePosition nextUsePos[AnyRegister::Total];
    bool needFloat = vregs[current->vreg()].isDouble();
    for (AnyRegisterIterator regs(allRegisters_); regs.more(); regs++) {
        AnyRegister reg = *regs;
        if (reg.isFloat() == needFloat)
            nextUsePos[reg.code()] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        if (i->getAllocation()->isRegister()) {
            AnyRegister reg = i->getAllocation()->toRegister();
            if (i->start().ins() == current->start().ins()) {
                nextUsePos[reg.code()] = CodePosition::MIN;
                IonSpew(IonSpew_RegAlloc, "   Disqualifying %s due to recency", reg.name());
            } else if (nextUsePos[reg.code()] != CodePosition::MIN) {
                nextUsePos[reg.code()] = i->nextUsePosAfter(current->start());
                IonSpew(IonSpew_RegAlloc, "   Register %s next used %u", reg.name(),
                        nextUsePos[reg.code()].pos());
            }
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        if (i->getAllocation()->isRegister()) {
            AnyRegister reg = i->getAllocation()->toRegister();
            CodePosition pos = i->nextUsePosAfter(current->start());
            if (pos < nextUsePos[reg.code()]) {
                nextUsePos[reg.code()] = pos;
                IonSpew(IonSpew_RegAlloc, "   Register %s next used %u", reg.name(), pos.pos());
            }
        }
    }

    CodePosition fixedPos = fixedIntervalsUnion->intersect(current);
    if (fixedPos != CodePosition::MIN) {
        for (IntervalIterator i(fixed.begin()); i != fixed.end(); i++) {
            AnyRegister reg = i->getAllocation()->toRegister();
            if (nextUsePos[reg.code()] != CodePosition::MIN) {
                CodePosition pos = i->intersect(current);
                if (pos != CodePosition::MIN && pos < nextUsePos[reg.code()]) {
                    nextUsePos[reg.code()] = pos;
                    IonSpew(IonSpew_RegAlloc, "   Register %s next used %u (fixed)", reg.name(), pos.pos());
                }
            }
        }
    }

    
    AnyRegister::Code bestCode = AnyRegister::Invalid;
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        if (nextUsePos[i] == CodePosition::MIN)
            continue;
        if (bestCode == AnyRegister::Invalid || nextUsePos[i] > nextUsePos[bestCode])
            bestCode = AnyRegister::Code(i);
    }

    if (bestCode != AnyRegister::Invalid)
        *nextUsed = nextUsePos[bestCode];
    return bestCode;
}












bool
LinearScanAllocator::canCoexist(LiveInterval *a, LiveInterval *b)
{
    LAllocation *aa = a->getAllocation();
    LAllocation *ba = b->getAllocation();
    if (aa->isRegister() && ba->isRegister() && aa->toRegister() == ba->toRegister())
        return a->intersect(b) == CodePosition::MIN;
    return true;
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
        if (i->getAllocation()->isRegister()) {
            JS_ASSERT(i->end() <= current->start());
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
        LinearScanVirtualRegister *reg = &vregs[i->vreg()];
        bool found = false;
        for (size_t j = 0; j < reg->numIntervals(); j++) {
            if (reg->getInterval(j) == *i) {
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
    IonSpew(IonSpew_RegAlloc, "Beginning register allocation");

    IonSpew(IonSpew_RegAlloc, "Beginning liveness analysis");
    if (!buildLivenessInfo())
        return false;
    IonSpew(IonSpew_RegAlloc, "Liveness analysis complete");

    if (mir->shouldCancel("LSRA Liveness"))
        return false;

    IonSpew(IonSpew_RegAlloc, "Beginning preliminary register allocation");
    if (!allocateRegisters())
        return false;
    IonSpew(IonSpew_RegAlloc, "Preliminary register allocation complete");

    if (mir->shouldCancel("LSRA Preliminary Regalloc"))
        return false;

    IonSpew(IonSpew_RegAlloc, "Beginning control flow resolution");
    if (!resolveControlFlow())
        return false;
    IonSpew(IonSpew_RegAlloc, "Control flow resolution complete");

    if (mir->shouldCancel("LSRA Control Flow"))
        return false;

    IonSpew(IonSpew_RegAlloc, "Beginning register allocation reification");
    if (!reifyAllocations())
        return false;
    IonSpew(IonSpew_RegAlloc, "Register allocation reification complete");

    if (mir->shouldCancel("LSRA Reification"))
        return false;

    IonSpew(IonSpew_RegAlloc, "Beginning safepoint population.");
    if (!populateSafepoints())
        return false;
    IonSpew(IonSpew_RegAlloc, "Safepoint population complete.");

    if (mir->shouldCancel("LSRA Safepoints"))
        return false;

    IonSpew(IonSpew_RegAlloc, "Register allocation complete");

    return true;
}

void
LinearScanAllocator::setIntervalRequirement(LiveInterval *interval)
{
    JS_ASSERT(interval->requirement()->kind() == Requirement::NONE);
    JS_ASSERT(interval->hint()->kind() == Requirement::NONE);

    
    
    LinearScanVirtualRegister *reg = &vregs[interval->vreg()];

    if (interval->index() == 0) {
        
        

        if (reg->def()->policy() == LDefinition::PRESET) {
            
            if (reg->def()->output()->isRegister())
                interval->setHint(Requirement(*reg->def()->output()));
            else
                interval->setRequirement(Requirement(*reg->def()->output()));
        } else if (reg->def()->policy() == LDefinition::MUST_REUSE_INPUT) {
            
            LUse *use = reg->ins()->getOperand(reg->def()->getReusedInput())->toUse();
            interval->setRequirement(Requirement(Requirement::REGISTER));
            interval->setHint(Requirement(use->virtualRegister(), interval->start().previous()));
        } else if (reg->ins()->isPhi()) {
            
            
            
            LUse *use = reg->ins()->getOperand(0)->toUse();
            LBlock *predecessor = reg->block()->mir()->getPredecessor(0)->lir();
            CodePosition predEnd = outputOf(predecessor->lastId());
            interval->setHint(Requirement(use->virtualRegister(), predEnd));
        } else {
            
            interval->setRequirement(Requirement(Requirement::REGISTER));
        }
    }

    UsePosition *fixedOp = NULL;
    UsePosition *registerOp = NULL;

    
    UsePositionIterator usePos(interval->usesBegin());
    for (; usePos != interval->usesEnd(); usePos++) {
        if (interval->start().next() < usePos->pos)
            break;

        LUse::Policy policy = usePos->use->policy();
        if (policy == LUse::FIXED) {
            fixedOp = *usePos;
            interval->setRequirement(Requirement(Requirement::REGISTER));
            break;
        } else if (policy == LUse::REGISTER) {
            
            interval->setRequirement(Requirement(Requirement::REGISTER));
        }
    }

    
    
    
    if (!fixedOp && !vregs[interval->vreg()].canonicalSpill()) {
        for (; usePos != interval->usesEnd(); usePos++) {
            LUse::Policy policy = usePos->use->policy();
            if (policy == LUse::FIXED) {
                fixedOp = *usePos;
                break;
            } else if (policy == LUse::REGISTER) {
                if (!registerOp)
                    registerOp = *usePos;
            }
        }
    }

    if (fixedOp) {
        
        AnyRegister required = GetFixedRegister(reg->def(), fixedOp->use);
        interval->setHint(Requirement(LAllocation(required), fixedOp->pos));
    } else if (registerOp) {
        
        
        
        if (interval->hint()->kind() == Requirement::NONE)
            interval->setHint(Requirement(Requirement::REGISTER, registerOp->pos));
    }
}











void
LinearScanAllocator::UnhandledQueue::enqueueBackward(LiveInterval *interval)
{
    IntervalReverseIterator i(rbegin()); 

    for (; i != rend(); i++) {
        if (i->start() > interval->start())
            break;
        if (i->start() == interval->start() &&
            i->requirement()->priority() >= interval->requirement()->priority())
        {
            break;
        }
    }
    insertAfter(*i, interval);
}





void
LinearScanAllocator::UnhandledQueue::enqueueForward(LiveInterval *after, LiveInterval *interval)
{
    IntervalIterator i(begin(after));
    i++; 

    for (; i != end(); i++) {
        if (i->start() < interval->start())
            break;
        if (i->start() == interval->start() &&
            i->requirement()->priority() < interval->requirement()->priority())
        {
            break;
        }
    }
    insertBefore(*i, interval);
}




void
LinearScanAllocator::UnhandledQueue::enqueueAtHead(LiveInterval *interval)
{
#ifdef DEBUG
    
    
    if (!empty()) {
        LiveInterval *back = peekBack();
        JS_ASSERT(back->start() >= interval->start());
        JS_ASSERT_IF(back->start() == interval->start(),
                     back->requirement()->priority() >= interval->requirement()->priority());
    }
#endif

    pushBack(interval);
}

void
LinearScanAllocator::UnhandledQueue::assertSorted()
{
#ifdef DEBUG
    LiveInterval *prev = NULL;
    for (IntervalIterator i(begin()); i != end(); i++) {
        if (prev) {
            JS_ASSERT(prev->start() >= i->start());
            JS_ASSERT_IF(prev->start() == i->start(),
                         prev->requirement()->priority() >= i->requirement()->priority());
        }
        prev = *i;
    }
#endif
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
