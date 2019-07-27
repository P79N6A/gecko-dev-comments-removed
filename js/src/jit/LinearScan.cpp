





#include "jit/LinearScan.h"

#include "mozilla/DebugOnly.h"

#include "jit/BitSet.h"
#include "jit/JitSpewer.h"

using namespace js;
using namespace js::jit;

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
    while ((current = unhandled.dequeue()) != nullptr) {
        JS_ASSERT(current->getAllocation()->isBogus());
        JS_ASSERT(current->numRanges() > 0);

        if (mir->shouldCancel("LSRA Allocate Registers (main loop)"))
            return false;

        CodePosition position = current->start();
        const Requirement *req = current->requirement();
        const Requirement *hint = current->hint();

        JitSpew(JitSpew_RegAlloc, "Processing %d = [%u, %u] (pri=%d)",
                current->hasVreg() ? current->vreg() : 0, current->start().bits(),
                current->end().bits(), current->requirement()->priority());

        
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
            JitSpew(JitSpew_RegAlloc, "  Eagerly spilling virtual register %d",
                    current->hasVreg() ? current->vreg() : 0);
            if (!spill())
                return false;
            continue;
        }

        
        JitSpew(JitSpew_RegAlloc, " Attempting free register allocation");
        CodePosition bestFreeUntil;
        AnyRegister::Code bestCode = findBestFreeRegister(&bestFreeUntil);
        if (bestCode != AnyRegister::Invalid) {
            AnyRegister best = AnyRegister::FromCode(bestCode);
            JitSpew(JitSpew_RegAlloc, "  Decided best register was %s", best.name());

            
            if (bestFreeUntil < current->end()) {
                if (!splitInterval(current, bestFreeUntil))
                    return false;
            }
            if (!assign(LAllocation(best)))
                return false;

            continue;
        }

        JitSpew(JitSpew_RegAlloc, " Attempting blocked register allocation");

        
        
        
        CodePosition bestNextUsed;
        bestCode = findBestBlockedRegister(&bestNextUsed);
        if (bestCode != AnyRegister::Invalid &&
            (req->kind() == Requirement::REGISTER || hint->pos() < bestNextUsed))
        {
            AnyRegister best = AnyRegister::FromCode(bestCode);
            JitSpew(JitSpew_RegAlloc, "  Decided best register was %s", best.name());

            if (!splitBlockingIntervals(best))
                return false;
            if (!assign(LAllocation(best)))
                return false;

            continue;
        }

        JitSpew(JitSpew_RegAlloc, "  No registers available to spill");
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
            LDefinition *def = phi->getDef(0);
            LinearScanVirtualRegister *vreg = &vregs[def];
            LiveInterval *to = vreg->intervalFor(entryOf(successor));
            JS_ASSERT(to);

            for (size_t k = 0; k < mSuccessor->numPredecessors(); k++) {
                LBlock *predecessor = mSuccessor->getPredecessor(k)->lir();
                JS_ASSERT(predecessor->mir()->numSuccessors() == 1);

                LAllocation *input = phi->getOperand(k);
                LiveInterval *from = vregs[input].intervalFor(exitOf(predecessor));
                JS_ASSERT(from);

                if (!moveAtExit(predecessor, from, to, def->type()))
                    return false;
            }

            if (vreg->mustSpillAtDefinition() && !to->isSpill()) {
                
                LMoveGroup *moves = successor->getEntryMoveGroup(alloc());
                if (!moves->add(to->getAllocation(), vregs[to->vreg()].canonicalSpill(),
                                def->type()))
                    return false;
            }
        }

        
        BitSet *live = liveIn[mSuccessor->id()];

        for (BitSet::Iterator liveRegId(*live); liveRegId; liveRegId++) {
            LinearScanVirtualRegister *vreg = &vregs[*liveRegId];
            LiveInterval *to = vreg->intervalFor(entryOf(successor));
            JS_ASSERT(to);

            for (size_t j = 0; j < mSuccessor->numPredecessors(); j++) {
                LBlock *predecessor = mSuccessor->getPredecessor(j)->lir();
                LiveInterval *from = vregs[*liveRegId].intervalFor(exitOf(predecessor));
                JS_ASSERT(from);

                if (*from->getAllocation() == *to->getAllocation())
                    continue;

                
                
                if (vreg->mustSpillAtDefinition() && to->getAllocation()->isStackSlot()) {
                    JS_ASSERT(vreg->canonicalSpill());
                    JS_ASSERT(*vreg->canonicalSpill() == *to->getAllocation());
                    continue;
                }

                if (mSuccessor->numPredecessors() > 1) {
                    JS_ASSERT(predecessor->mir()->numSuccessors() == 1);
                    if (!moveAtExit(predecessor, from, to, vreg->type()))
                        return false;
                } else {
                    if (!moveAtEntry(successor, from, to, vreg->type()))
                        return false;
                }
            }
        }
    }

    return true;
}

bool
LinearScanAllocator::moveInputAlloc(CodePosition pos, LAllocation *from, LAllocation *to,
                                    LDefinition::Type type)
{
    if (*from == *to)
        return true;
    LMoveGroup *moves = getInputMoveGroup(pos);
    return moves->add(from, to, type);
}

static inline void
SetOsiPointUses(LiveInterval *interval, CodePosition defEnd, const LAllocation &allocation)
{
    
    
    

    JS_ASSERT(interval->index() == 0);

    for (UsePositionIterator usePos(interval->usesBegin());
         usePos != interval->usesEnd();
         usePos++)
    {
        if (usePos->pos > defEnd)
            break;
        *static_cast<LAllocation *>(usePos->use) = allocation;
    }
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
                if (!moveInput(usePos->pos, interval, to, reg->type()))
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

            
            
            CodePosition defEnd = minimalDefEnd(reg->ins());

            if (def->policy() == LDefinition::FIXED && def->output()->isRegister()) {
                AnyRegister fixedReg = def->output()->toRegister();
                LiveInterval *from = fixedIntervals[fixedReg.code()];

                
                
                SetOsiPointUses(interval, defEnd, LAllocation(fixedReg));

                if (!moveAfter(defEnd, from, interval, def->type()))
                    return false;
                spillFrom = from->getAllocation();
            } else {
                if (def->policy() == LDefinition::MUST_REUSE_INPUT) {
                    LAllocation *inputAlloc = reg->ins()->getOperand(def->getReusedInput());
                    LAllocation *origAlloc = LAllocation::New(alloc(), *inputAlloc);

                    JS_ASSERT(!inputAlloc->isUse());

                    *inputAlloc = *interval->getAllocation();
                    if (!moveInputAlloc(inputOf(reg->ins()), origAlloc, inputAlloc, def->type()))
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
                
                
                SetOsiPointUses(interval, defEnd, *spillFrom);

                
                
                
                LMoveGroup *moves = getMoveGroupAfter(defEnd);
                if (!moves->add(spillFrom, reg->canonicalSpill(), def->type()))
                    return false;
            }
        }
        else if (interval->start() > entryOf(insData[interval->start()].block()) &&
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
                if (!moveInput(inputOf(data->ins()), prevInterval, interval, reg->type()))
                    return false;
            } else {
                if (!moveAfter(outputOf(data->ins()), prevInterval, interval, reg->type()))
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

        if (!reg->def() || (!IsTraceable(reg) && !IsSlotsOrElements(reg) && !IsNunbox(reg)))
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
                             def->type() == LDefinition::GENERAL ||
                             def->type() == LDefinition::INT32 ||
                             def->type() == LDefinition::FLOAT32 ||
                             def->type() == LDefinition::DOUBLE);
                continue;
            }

            LSafepoint *safepoint = ins->safepoint();

            if (IsSlotsOrElements(reg)) {
                LiveInterval *interval = reg->intervalFor(inputOf(ins));
                if (!interval)
                    continue;

                LAllocation *a = interval->getAllocation();
                if (a->isGeneralReg() && !ins->isCall())
                    safepoint->addSlotsOrElementsRegister(a->toGeneralReg()->reg());

                if (isSpilledAt(interval, inputOf(ins))) {
                    if (!safepoint->addSlotsOrElementsSlot(reg->canonicalSpillSlot()))
                        return false;
                }
            } else if (!IsNunbox(reg)) {
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

                
                
                
                
                if (payloadAlloc->isArgument() &&
                    (!payload->canonicalSpill() || payload->canonicalSpill() == payloadAlloc))
                {
                    continue;
                }

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

                    
                    
                    if (payloadAlloc->isGeneralReg() && isSpilledAt(payloadInterval, inputOf(ins))) {
                        if (!safepoint->addNunboxParts(*typeAlloc, *payload->canonicalSpill()))
                            return false;
                    }
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

    
    LiveInterval *newInterval = LiveInterval::New(alloc(), interval->vreg(), interval->index() + 1);
    if (!interval->splitFrom(pos, newInterval))
        return false;

    JS_ASSERT(interval->numRanges() > 0);
    JS_ASSERT(newInterval->numRanges() > 0);

    if (!reg->addInterval(newInterval))
        return false;

    JitSpew(JitSpew_RegAlloc, "  Split interval to %u = [%u, %u]/[%u, %u]",
            interval->vreg(), interval->start().bits(),
            interval->end().bits(), newInterval->start().bits(),
            newInterval->end().bits());

    
    
    
    setIntervalRequirement(newInterval);

    
    
    
    unhandled.enqueueBackward(newInterval);

    return true;
}

bool
LinearScanAllocator::splitBlockingIntervals(AnyRegister allocatedReg)
{

    
    LiveInterval *fixed = fixedIntervals[allocatedReg.code()];
    if (fixed->numRanges() > 0) {
        CodePosition fixedPos = current->intersect(fixed);
        if (fixedPos != CodePosition::MIN) {
            JS_ASSERT(fixedPos > current->start());
            JS_ASSERT(fixedPos < current->end());
            if (!splitInterval(current, fixedPos))
                return false;
        }
    }

    

    for (IntervalIterator i(active.begin()); i != active.end();) {
        if (i->getAllocation()->isRegister() &&
            i->getAllocation()->toRegister().aliases(allocatedReg))
        {
            JitSpew(JitSpew_RegAlloc, " Splitting active interval %u = [%u, %u]",
                    vregs[i->vreg()].ins()->id(), i->start().bits(), i->end().bits());

            JS_ASSERT(i->start() != current->start());
            JS_ASSERT(i->covers(current->start()));
            JS_ASSERT(i->start() != current->start());

            if (!splitInterval(*i, current->start()))
                return false;

            LiveInterval *it = *i;
            i = active.removeAt(i);
            finishInterval(it);
            if (allocatedReg.numAliased() == 1)
                break;
        } else {
            JitSpew(JitSpew_RegAlloc, " Not touching active interval %u = [%u, %u]",
                    vregs[i->vreg()].ins()->id(), i->start().bits(), i->end().bits());
            i++;
        }
    }
    
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); ) {
        if (i->getAllocation()->isRegister() &&
            i->getAllocation()->toRegister().aliases(allocatedReg))
        {
            JitSpew(JitSpew_RegAlloc, " Splitting inactive interval %u = [%u, %u]",
                    vregs[i->vreg()].ins()->id(), i->start().bits(), i->end().bits());

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
        JitSpew(JitSpew_RegAlloc, "Assigning register %s", allocation.toRegister().name());
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

    bool useAsCanonicalSpillSlot = allocation.isMemory();
    
    
    if (mir->modifiesFrameArguments())
        useAsCanonicalSpillSlot = allocation.isStackSlot();

    if (reg && useAsCanonicalSpillSlot) {
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
    if (reg->type() == LDefinition::DOUBLE)
        freed = &finishedDoubleSlots_;
#if JS_BITS_PER_WORD == 64
    else if (reg->type() == LDefinition::GENERAL ||
             reg->type() == LDefinition::OBJECT ||
             reg->type() == LDefinition::SLOTS)
        freed = &finishedDoubleSlots_;
#endif
#ifdef JS_PUNBOX64
    else if (reg->type() == LDefinition::BOX)
        freed = &finishedDoubleSlots_;
#endif
#ifdef JS_NUNBOX32
    else if (IsNunbox(reg))
        freed = &finishedNunboxSlots_;
#endif
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

    return stackSlotAllocator.allocateSlot(reg->type());
}

bool
LinearScanAllocator::spill()
{
    JitSpew(JitSpew_RegAlloc, "  Decided to spill current interval");

    
    JS_ASSERT(current->hasVreg());

    LinearScanVirtualRegister *reg = &vregs[current->vreg()];

    if (reg->canonicalSpill()) {
        JitSpew(JitSpew_RegAlloc, "  Allocating canonical spill location");

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

    return assign(LStackSlot(stackSlot));
}

void
LinearScanAllocator::freeAllocation(LiveInterval *interval, LAllocation *alloc)
{
    LinearScanVirtualRegister *mine = &vregs[interval->vreg()];
    if (!IsNunbox(mine)) {
        if (alloc->isStackSlot()) {
            if (mine->type() == LDefinition::DOUBLE)
                finishedDoubleSlots_.append(interval);
#if JS_BITS_PER_WORD == 64
            else if (mine->type() == LDefinition::GENERAL ||
                     mine->type() == LDefinition::OBJECT ||
                     mine->type() == LDefinition::SLOTS)
                finishedDoubleSlots_.append(interval);
#endif
#ifdef JS_PUNBOX64
            else if (mine->type() == LDefinition::BOX)
                finishedDoubleSlots_.append(interval);
#endif
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

        finishedNunboxSlots_.append(candidate->lastInterval());
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
    JitSpew(JitSpew_RegAlloc, "  Computing freeUntilPos");

    
    CodePosition freeUntilPos[AnyRegister::Total];
    bool needFloat = vregs[current->vreg()].isFloatReg();
    for (RegisterSet regs(allRegisters_); !regs.empty(needFloat); ) {
        
        
        AnyRegister reg = regs.takeAny(needFloat);
        freeUntilPos[reg.code()] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        LAllocation *alloc = i->getAllocation();
        if (alloc->isRegister(needFloat)) {
            AnyRegister reg = alloc->toRegister();
            for (size_t a = 0; a < reg.numAliased(); a++) {
                JitSpew(JitSpew_RegAlloc, "   Register %s not free", reg.aliased(a).name());
                freeUntilPos[reg.aliased(a).code()] = CodePosition::MIN;
            }
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        LAllocation *alloc = i->getAllocation();
        if (alloc->isRegister(needFloat)) {
            AnyRegister reg = alloc->toRegister();
            CodePosition pos = current->intersect(*i);
            for (size_t a = 0; a < reg.numAliased(); a++) {
                if (pos != CodePosition::MIN && pos < freeUntilPos[reg.aliased(a).code()]) {
                    freeUntilPos[reg.aliased(a).code()] = pos;
                    JitSpew(JitSpew_RegAlloc, "   Register %s free until %u", reg.aliased(a).name(), pos.bits());
                }
            }
        }
    }

    CodePosition fixedPos = fixedIntervalsUnion->intersect(current);
    if (fixedPos != CodePosition::MIN) {
        for (IntervalIterator i(fixed.begin()); i != fixed.end(); i++) {
            AnyRegister reg = i->getAllocation()->toRegister();
            for (size_t a = 0; a < reg.numAliased(); a++) {
                AnyRegister areg = reg.aliased(a);
                if (freeUntilPos[areg.code()] != CodePosition::MIN) {
                    CodePosition pos = current->intersect(*i);
                    if (pos != CodePosition::MIN && pos < freeUntilPos[areg.code()]) {
                        freeUntilPos[areg.code()] = (pos == current->start()) ? CodePosition::MIN : pos;
                        JitSpew(JitSpew_RegAlloc, "   Register %s free until %u", areg.name(), pos.bits());
                    }
                }
            }
        }
    }

    AnyRegister::Code bestCode = AnyRegister::Invalid;
    if (current->index()) {
        
        
        LiveInterval *previous = vregs[current->vreg()].getInterval(current->index() - 1);
        LAllocation *alloc = previous->getAllocation();
        if (alloc->isRegister(needFloat)) {
            AnyRegister prevReg = alloc->toRegister();
            bool useit = true;
            for (size_t a = 0; a < prevReg.numAliased(); a++) {
                AnyRegister aprevReg = prevReg.aliased(a);
                if (freeUntilPos[aprevReg.code()] == CodePosition::MIN) {
                    useit = false;
                    break;
                }
            }
            if (useit)
                bestCode = prevReg.code();
        }
    }

    
    const Requirement *hint = current->hint();
    if (hint->kind() == Requirement::FIXED && hint->allocation().isRegister()) {
        AnyRegister hintReg = hint->allocation().toRegister();
        bool useit = true;
        for (size_t a = 0; a < hintReg.numAliased(); a++) {
            if (freeUntilPos[hintReg.aliased(a).code()] <= hint->pos()) {
                useit = false;
                break;
            }
        }
        if (useit)
            bestCode = hintReg.code();

    } else if (hint->kind() == Requirement::MUST_REUSE_INPUT) {
        LiveInterval *other = vregs[hint->virtualRegister()].intervalFor(hint->pos());
        if (other && other->getAllocation()->isRegister()) {
            AnyRegister hintReg = other->getAllocation()->toRegister();
            bool useit = true;
            for (size_t a = 0; a < hintReg.numAliased(); a++) {
                if (freeUntilPos[hintReg.aliased(a).code()] <= hint->pos()) {
                    useit = false;
                    break;
                }
            }
            if (useit)
                bestCode = hintReg.code();
        }
    }

    if (bestCode == AnyRegister::Invalid) {
        
        for (uint32_t i = 0; i < AnyRegister::Total; i++) {
            if (freeUntilPos[i] == CodePosition::MIN)
                continue;
            AnyRegister cur = AnyRegister::FromCode(i);
            if (!vregs[current->vreg()].isCompatibleReg(cur))
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
    JitSpew(JitSpew_RegAlloc, "  Computing nextUsePos");

    
    CodePosition nextUsePos[AnyRegister::Total];
    bool needFloat = vregs[current->vreg()].isFloatReg();
    for (RegisterSet regs(allRegisters_); !regs.empty(needFloat); ) {
        AnyRegister reg = regs.takeAny(needFloat);
        nextUsePos[reg.code()] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        LAllocation *alloc = i->getAllocation();
        if (alloc->isRegister(needFloat)) {
            AnyRegister fullreg = alloc->toRegister();
            for (size_t a = 0; a < fullreg.numAliased(); a++) {
                AnyRegister reg = fullreg.aliased(a);
                if (i->start() == current->start()) {
                    nextUsePos[reg.code()] = CodePosition::MIN;
                    JitSpew(JitSpew_RegAlloc, "   Disqualifying %s due to recency", reg.name());
                } else if (nextUsePos[reg.code()] != CodePosition::MIN) {
                    nextUsePos[reg.code()] = i->nextUsePosAfter(current->start());
                    JitSpew(JitSpew_RegAlloc, "   Register %s next used %u", reg.name(),
                            nextUsePos[reg.code()].bits());
                }
            }
        }
    }
    for (IntervalIterator i(inactive.begin()); i != inactive.end(); i++) {
        LAllocation *alloc = i->getAllocation();
        if (alloc->isRegister(needFloat)) {
            AnyRegister reg = alloc->toRegister();
            CodePosition pos = i->nextUsePosAfter(current->start());
            for (size_t a = 0; a < reg.numAliased(); a++) {
                if (pos < nextUsePos[reg.aliased(a).code()]) {
                    nextUsePos[reg.aliased(a).code()] = pos;
                    JitSpew(JitSpew_RegAlloc, "   Register %s next used %u", reg.aliased(a).name(), pos.bits());
                }
            }
        }
    }

    CodePosition fixedPos = fixedIntervalsUnion->intersect(current);
    if (fixedPos != CodePosition::MIN) {
        for (IntervalIterator i(fixed.begin()); i != fixed.end(); i++) {
            AnyRegister fullreg = i->getAllocation()->toRegister();
            for (size_t a = 0; a < fullreg.numAliased(); a++) {
                AnyRegister reg = fullreg.aliased(a);
                if (nextUsePos[reg.code()] != CodePosition::MIN) {
                    CodePosition pos = i->intersect(current);
                    if (pos != CodePosition::MIN && pos < nextUsePos[reg.code()]) {
                        nextUsePos[reg.code()] = (pos == current->start()) ? CodePosition::MIN : pos;
                        JitSpew(JitSpew_RegAlloc, "   Register %s next used %u (fixed)", reg.name(), pos.bits());
                    }
                }
            }
        }
    }

    
    AnyRegister::Code bestCode = AnyRegister::Invalid;
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        if (nextUsePos[i] == CodePosition::MIN)
            continue;
        AnyRegister cur = AnyRegister::FromCode(i);
        if (!vregs[current->vreg()].isCompatibleReg(cur))
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
    if (aa->isRegister() && ba->isRegister() && aa->toRegister().aliases(ba->toRegister()))
        return a->intersect(b) == CodePosition::MIN;
    return true;
}

#ifdef DEBUG






void
LinearScanAllocator::validateIntervals()
{
    if (!js_JitOptions.checkGraphConsistency)
        return;

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
    if (!js_JitOptions.checkGraphConsistency)
        return;

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
    JitSpew(JitSpew_RegAlloc, "Beginning register allocation");

    if (!buildLivenessInfo())
        return false;

    if (mir->shouldCancel("LSRA Liveness"))
        return false;

    JitSpew(JitSpew_RegAlloc, "Beginning preliminary register allocation");
    if (!allocateRegisters())
        return false;
    JitSpew(JitSpew_RegAlloc, "Preliminary register allocation complete");

    if (mir->shouldCancel("LSRA Preliminary Regalloc"))
        return false;

    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        fprintf(stderr, "Allocations by virtual register:\n");
        dumpVregs();
    }

    JitSpew(JitSpew_RegAlloc, "Beginning control flow resolution");
    if (!resolveControlFlow())
        return false;
    JitSpew(JitSpew_RegAlloc, "Control flow resolution complete");

    if (mir->shouldCancel("LSRA Control Flow"))
        return false;

    JitSpew(JitSpew_RegAlloc, "Beginning register allocation reification");
    if (!reifyAllocations())
        return false;
    JitSpew(JitSpew_RegAlloc, "Register allocation reification complete");

    if (mir->shouldCancel("LSRA Reification"))
        return false;

    JitSpew(JitSpew_RegAlloc, "Beginning safepoint population.");
    if (!populateSafepoints())
        return false;
    JitSpew(JitSpew_RegAlloc, "Safepoint population complete.");

    if (mir->shouldCancel("LSRA Safepoints"))
        return false;

    JitSpew(JitSpew_RegAlloc, "Register allocation complete");

    return true;
}

void
LinearScanAllocator::setIntervalRequirement(LiveInterval *interval)
{
    JS_ASSERT(interval->requirement()->kind() == Requirement::NONE);
    JS_ASSERT(interval->hint()->kind() == Requirement::NONE);

    
    
    LinearScanVirtualRegister *reg = &vregs[interval->vreg()];

    if (interval->index() == 0) {
        
        

        if (reg->def()->policy() == LDefinition::FIXED) {
            
            if (reg->def()->output()->isRegister())
                interval->setHint(Requirement(*reg->def()->output()));
            else
                interval->setRequirement(Requirement(*reg->def()->output()));
        } else if (reg->def()->policy() == LDefinition::MUST_REUSE_INPUT) {
            
            LUse *use = reg->ins()->getOperand(reg->def()->getReusedInput())->toUse();
            interval->setRequirement(Requirement(Requirement::REGISTER));
            interval->setHint(Requirement(use->virtualRegister(), interval->start().previous()));
        } else if (reg->ins()->isPhi()) {
            
            
            
            if (reg->ins()->toPhi()->numOperands() != 0) {
                LUse *use = reg->ins()->toPhi()->getOperand(0)->toUse();
                LBlock *predecessor = reg->block()->mir()->getPredecessor(0)->lir();
                CodePosition predEnd = exitOf(predecessor);
                interval->setHint(Requirement(use->virtualRegister(), predEnd));
            }
        } else {
            
            interval->setRequirement(Requirement(Requirement::REGISTER));
        }
    }

    UsePosition *fixedOp = nullptr;
    UsePosition *registerOp = nullptr;

    
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
LinearScanAllocator::UnhandledQueue::assertSorted()
{
#ifdef DEBUG
    LiveInterval *prev = nullptr;
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
        return nullptr;

    LiveInterval *result = *rbegin();
    remove(result);
    return result;
}
