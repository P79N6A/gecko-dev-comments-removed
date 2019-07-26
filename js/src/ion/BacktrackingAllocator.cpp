






#include "BacktrackingAllocator.h"

using namespace js;
using namespace js::ion;

using mozilla::DebugOnly;

bool
BacktrackingAllocator::init()
{
    RegisterSet remainingRegisters(allRegisters_);
    while (!remainingRegisters.empty( false)) {
        AnyRegister reg = AnyRegister(remainingRegisters.takeGeneral());
        registers[reg.code()].allocatable = true;
    }
    while (!remainingRegisters.empty( true)) {
        AnyRegister reg = AnyRegister(remainingRegisters.takeFloat());
        registers[reg.code()].allocatable = true;
    }

    LifoAlloc *alloc = mir->temp().lifoAlloc();
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        registers[i].reg = AnyRegister::FromCode(i);
        registers[i].allocations.setAllocator(alloc);

        LiveInterval *fixed = fixedIntervals[i];
        for (size_t j = 0; j < fixed->numRanges(); j++) {
            AllocatedRange range(fixed, fixed->getRange(j));
            if (!registers[i].allocations.insert(range))
                return false;
        }
    }

    hotcode.setAllocator(alloc);

    
    
    
    

    LiveInterval *hotcodeInterval = new LiveInterval(0);

    LBlock *backedge = NULL;
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock *block = graph.getBlock(i);

        
        
        
        if (block->mir()->isLoopHeader())
            backedge = block->mir()->backedge()->lir();

        if (block == backedge) {
            LBlock *header = block->mir()->loopHeaderOfBackedge()->lir();
            CodePosition from = inputOf(header->firstId());
            CodePosition to = outputOf(block->lastId()).next();
            if (!hotcodeInterval->addRange(from, to))
                return false;
        }
    }

    for (size_t i = 0; i < hotcodeInterval->numRanges(); i++) {
        AllocatedRange range(hotcodeInterval, hotcodeInterval->getRange(i));
        if (!hotcode.insert(range))
            return false;
    }

    return true;
}

static inline const char *
IntervalString(const LiveInterval *interval)
{
#ifdef DEBUG
    if (!interval->numRanges())
        return " empty";

    
    static char buf[1000];

    char *cursor = buf;
    char *end = cursor + sizeof(buf);

    for (size_t i = 0; i < interval->numRanges(); i++) {
        const LiveInterval::Range *range = interval->getRange(i);
        int n = JS_snprintf(cursor, end - cursor, " [%u,%u>", range->from.pos(), range->to.pos());
        if (n < 0)
            return " ???";
        cursor += n;
    }

    return buf;
#else
    return " ???";
#endif
}

bool
BacktrackingAllocator::go()
{
    IonSpew(IonSpew_RegAlloc, "Beginning register allocation");

    IonSpew(IonSpew_RegAlloc, "Beginning liveness analysis");
    if (!buildLivenessInfo())
        return false;
    IonSpew(IonSpew_RegAlloc, "Liveness analysis complete");

    if (IonSpewEnabled(IonSpew_RegAlloc))
        dumpLiveness();

    if (!init())
        return false;

    if (!queuedIntervals.reserve(graph.numVirtualRegisters() * 3 / 2))
        return false;

    if (!groupAndQueueRegisters())
        return false;

    if (IonSpewEnabled(IonSpew_RegAlloc))
        dumpRegisterGroups();

    
    while (!queuedIntervals.empty()) {
        if (mir->shouldCancel("Backtracking Allocation"))
            return false;

        LiveInterval *interval = queuedIntervals.removeHighest().interval;
        if (!processInterval(interval))
            return false;
    }

    if (IonSpewEnabled(IonSpew_RegAlloc))
        dumpAllocations();

    return resolveControlFlow() && reifyAllocations();
}

static bool
LifetimesMightOverlap(BacktrackingVirtualRegister *reg0, BacktrackingVirtualRegister *reg1)
{
    
    CodePosition start0 = reg0->getFirstInterval()->start();
    CodePosition start1 = reg1->getFirstInterval()->start();
    CodePosition end0 = reg0->lastInterval()->end();
    CodePosition end1 = reg1->lastInterval()->end();
    return (end0 > start1) && (end1 > start0);
}

bool
BacktrackingAllocator::canAddToGroup(VirtualRegisterGroup *group, BacktrackingVirtualRegister *reg)
{
    for (size_t i = 0; i < group->registers.length(); i++) {
        if (LifetimesMightOverlap(reg, &vregs[group->registers[i]]))
            return false;
    }
    return true;
}

bool
BacktrackingAllocator::tryGroupRegisters(uint32_t vreg0, uint32_t vreg1)
{
    
    
    
    BacktrackingVirtualRegister *reg0 = &vregs[vreg0], *reg1 = &vregs[vreg1];

    if (reg0->isDouble() != reg1->isDouble())
        return true;

    VirtualRegisterGroup *group0 = reg0->group(), *group1 = reg1->group();

    if (!group0 && group1)
        return tryGroupRegisters(vreg1, vreg0);

    if (group0) {
        if (group1) {
            if (group0 == group1) {
                
                return true;
            }
            
            for (size_t i = 0; i < group1->registers.length(); i++) {
                if (!canAddToGroup(group0, &vregs[group1->registers[i]]))
                    return true;
            }
            for (size_t i = 0; i < group1->registers.length(); i++) {
                uint32_t vreg = group1->registers[i];
                if (!group0->registers.append(vreg))
                    return false;
                vregs[vreg].setGroup(group0);
            }
            return true;
        }
        if (!canAddToGroup(group0, reg1))
            return true;
        if (!group0->registers.append(vreg1))
            return false;
        reg1->setGroup(group0);
        return true;
    }

    if (LifetimesMightOverlap(reg0, reg1))
        return true;

    VirtualRegisterGroup *group = new VirtualRegisterGroup();
    if (!group->registers.append(vreg0) || !group->registers.append(vreg1))
        return false;

    reg0->setGroup(group);
    reg1->setGroup(group);
    return true;
}

bool
BacktrackingAllocator::groupAndQueueRegisters()
{
    for (size_t i = 0; i < graph.numVirtualRegisters(); i++) {
        if (mir->shouldCancel("Backtracking Group Registers"))
            return false;

        BacktrackingVirtualRegister &reg = vregs[i];

        
        for (size_t j = 0; j < reg.numIntervals(); j++) {
            LiveInterval *interval = reg.getInterval(j);
            if (interval->numRanges() > 0) {
                size_t priority = computePriority(interval);
                if (!queuedIntervals.insert(QueuedInterval(interval, priority)))
                    return false;
            }
        }

        LDefinition *def = reg.def();
        if (def && def->policy() == LDefinition::MUST_REUSE_INPUT) {
            LUse *use = reg.ins()->getOperand(def->getReusedInput())->toUse();
            VirtualRegister &usedReg = vregs[use->virtualRegister()];
            if (usedReg.intervalFor(outputOf(reg.ins())) || reg.intervalFor(inputOf(reg.ins()))) {
                
                
                
                
                
                
                
                IonSpew(IonSpew_RegAlloc, "Relaxing reuse-input constraint on v%u", i);
                reg.setMustCopyInput();
            } else {
                
                
                
                if (!tryGroupRegisters(use->virtualRegister(), def->virtualRegister()))
                    return false;
            }
        }

        
        for (size_t i = 0; i < graph.numBlocks(); i++) {
            LBlock *block = graph.getBlock(i);
            for (size_t j = 0; j < block->numPhis(); j++) {
                LPhi *phi = block->getPhi(j);
                uint32_t output = phi->getDef(0)->virtualRegister();
                for (size_t k = 0; k < phi->numOperands(); k++) {
                    uint32_t input = phi->getOperand(k)->toUse()->virtualRegister();
                    if (!tryGroupRegisters(input, output))
                        return false;
                }
            }
        }
    }

    return true;
}

static const size_t MAX_ATTEMPTS = 2;

bool
BacktrackingAllocator::processInterval(LiveInterval *interval)
{
    if (IonSpewEnabled(IonSpew_RegAlloc)) {
        IonSpew(IonSpew_RegAlloc, "Allocating v%u [priority %lu] [weight %lu]: %s",
                interval->vreg(), computePriority(interval), computeSpillWeight(interval),
                IntervalString(interval));
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    bool canAllocate = setIntervalRequirement(interval);

    LiveInterval *conflict;
    for (size_t attempt = 0;; attempt++) {
        if (canAllocate) {
            
            if (interval->requirement()->kind() == Requirement::FIXED &&
                !interval->requirement()->allocation().isRegister())
            {
                IonSpew(IonSpew_RegAlloc, "stack allocation requirement");
                interval->setAllocation(interval->requirement()->allocation());
                return true;
            }

            conflict = NULL;

            
            
            
            
            
            if (interval->hint()->kind() == Requirement::FIXED) {
                AnyRegister reg = interval->hint()->allocation().toRegister();
                bool success;
                if (!tryAllocateRegister(registers[reg.code()], interval, &success, &conflict))
                    return false;
                if (success)
                    return true;
            }

            
            if (interval->requirement()->kind() == Requirement::NONE) {
                spill(interval);
                return true;
            }

            if (!conflict || minimalInterval(interval)) {
                
                
                for (size_t i = 0; i < AnyRegister::Total; i++) {
                    bool success;
                    if (!tryAllocateRegister(registers[i], interval, &success, &conflict))
                        return false;
                    if (success)
                        return true;
                }
            }
        }

        

        if (attempt < MAX_ATTEMPTS &&
            canAllocate &&
            conflict &&
            computeSpillWeight(conflict) < computeSpillWeight(interval))
        {
            if (!evictInterval(conflict))
                return false;
            continue;
        }

        
        
        
        
        JS_ASSERT(!minimalInterval(interval));

        return chooseIntervalSplit(interval);
    }
}

bool
BacktrackingAllocator::setIntervalRequirement(LiveInterval *interval)
{
    
    
    
    interval->setHint(Requirement());
    interval->setRequirement(Requirement());

    BacktrackingVirtualRegister *reg = &vregs[interval->vreg()];

    
    if (VirtualRegisterGroup *group = reg->group()) {
        if (group->allocation.isRegister())
            interval->setHint(Requirement(group->allocation));
    }

    if (interval->index() == 0) {
        
        

        LDefinition::Policy policy = reg->def()->policy();
        if (policy == LDefinition::PRESET) {
            
            interval->setRequirement(Requirement(*reg->def()->output()));
        } else if (reg->ins()->isPhi()) {
            
            
        } else {
            
            interval->setRequirement(Requirement(Requirement::REGISTER));
        }
    }

    
    for (UsePositionIterator iter = interval->usesBegin();
         iter != interval->usesEnd();
         iter++)
    {
        LUse::Policy policy = iter->use->policy();
        if (policy == LUse::FIXED) {
            AnyRegister required = GetFixedRegister(reg->def(), iter->use);

            
            
            
            if (!interval->addRequirement(Requirement(LAllocation(required))))
                return false;
        } else if (policy == LUse::REGISTER) {
            if (!interval->addRequirement(Requirement(Requirement::REGISTER)))
                return false;
        }
    }

    return true;
}

bool
BacktrackingAllocator::tryAllocateRegister(PhysicalRegister &r, LiveInterval *interval,
                                           bool *success, LiveInterval **pconflicting)
{
    *success = false;

    if (!r.allocatable)
        return true;

    BacktrackingVirtualRegister *reg = &vregs[interval->vreg()];
    if (reg->isDouble() != r.reg.isFloat())
        return true;

    if (interval->requirement()->kind() == Requirement::FIXED) {
        if (interval->requirement()->allocation() != LAllocation(r.reg)) {
            IonSpew(IonSpew_RegAlloc, "%s does not match fixed requirement", r.reg.name());
            return true;
        }
    }

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i)), existing;
        if (r.allocations.contains(range, &existing)) {
            if (existing.interval->hasVreg()) {
                if (IonSpewEnabled(IonSpew_RegAlloc)) {
                    IonSpew(IonSpew_RegAlloc, "%s collides with v%u [%u,%u> [weight %lu]",
                            r.reg.name(), existing.interval->vreg(),
                            existing.range->from.pos(), existing.range->to.pos(),
                            computeSpillWeight(existing.interval));
                }
                if (!*pconflicting || computeSpillWeight(existing.interval) < computeSpillWeight(*pconflicting))
                    *pconflicting = existing.interval;
            } else {
                if (IonSpewEnabled(IonSpew_RegAlloc)) {
                    IonSpew(IonSpew_RegAlloc, "%s collides with fixed use [%u,%u>",
                            r.reg.name(), existing.range->from.pos(), existing.range->to.pos());
                }
            }
            return true;
        }
    }

    IonSpew(IonSpew_RegAlloc, "allocated to %s", r.reg.name());

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i));
        if (!r.allocations.insert(range))
            return false;
    }

    
    if (VirtualRegisterGroup *group = reg->group()) {
        if (!group->allocation.isRegister())
            group->allocation = LAllocation(r.reg);
    }

    interval->setAllocation(LAllocation(r.reg));
    *success = true;
    return true;
}

bool
BacktrackingAllocator::evictInterval(LiveInterval *interval)
{
    if (IonSpewEnabled(IonSpew_RegAlloc)) {
        IonSpew(IonSpew_RegAlloc, "Evicting interval v%u: %s",
                interval->vreg(), IntervalString(interval));
    }

    JS_ASSERT(interval->getAllocation()->isRegister());

    AnyRegister reg(interval->getAllocation()->toRegister());
    PhysicalRegister &physical = registers[reg.code()];
    JS_ASSERT(physical.reg == reg && physical.allocatable);

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i));
        physical.allocations.remove(range);
    }

    interval->setAllocation(LAllocation());

    size_t priority = computePriority(interval);
    return queuedIntervals.insert(QueuedInterval(interval, priority));
}

bool
BacktrackingAllocator::splitAndRequeueInterval(LiveInterval *interval,
                                               const LiveIntervalVector &newIntervals)
{
    JS_ASSERT(newIntervals.length() >= 2);

    if (IonSpewEnabled(IonSpew_RegAlloc)) {
        IonSpew(IonSpew_RegAlloc, "splitting interval %s:", IntervalString(interval));
        for (size_t i = 0; i < newIntervals.length(); i++)
            IonSpew(IonSpew_RegAlloc, "    %s", IntervalString(newIntervals[i]));
    }

    
    LiveInterval *first = newIntervals[0];
    for (size_t i = 1; i < newIntervals.length(); i++) {
        if (newIntervals[i]->start() < first->start())
            first = newIntervals[i];
    }

    
    VirtualRegister *reg = &vregs[interval->vreg()];
    reg->replaceInterval(interval, first);
    for (size_t i = 0; i < newIntervals.length(); i++) {
        if (newIntervals[i] != first && !reg->addInterval(newIntervals[i]))
            return false;
    }

    
    
    
    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        CodePosition pos = iter->pos;
        LiveInterval *maxInterval = NULL;
        for (size_t i = 0; i < newIntervals.length(); i++) {
            if (newIntervals[i]->covers(pos)) {
                if (!maxInterval || newIntervals[i]->start() > maxInterval->start())
                    maxInterval = newIntervals[i];
            }
        }
        maxInterval->addUse(new UsePosition(iter->use, iter->pos));
    }

    
    for (size_t i = 0; i < newIntervals.length(); i++) {
        LiveInterval *newInterval = newIntervals[i];
        size_t priority = computePriority(newInterval);
        if (!queuedIntervals.insert(QueuedInterval(newInterval, priority)))
            return false;
    }

    return true;
}

void
BacktrackingAllocator::spill(LiveInterval *interval)
{
    IonSpew(IonSpew_RegAlloc, "Spilling interval");

    JS_ASSERT(interval->requirement()->kind() == Requirement::NONE);

    
    JS_ASSERT(interval->hasVreg());

    BacktrackingVirtualRegister *reg = &vregs[interval->vreg()];

    if (reg->canonicalSpill()) {
        IonSpew(IonSpew_RegAlloc, "  Picked canonical spill location %u",
                reg->canonicalSpill()->toStackSlot()->slot());
        interval->setAllocation(*reg->canonicalSpill());
        return;
    }

    if (reg->group() && reg->group()->spill.isStackSlot()) {
        IonSpew(IonSpew_RegAlloc, "  Reusing group spill location %u",
                reg->group()->spill.toStackSlot()->slot());
        interval->setAllocation(reg->group()->spill);
        reg->setCanonicalSpill(reg->group()->spill);
        return;
    }

    uint32_t stackSlot;
    if (reg->isDouble()) {
        stackSlot = stackSlotAllocator.allocateDoubleSlot();
    } else {
        stackSlot = stackSlotAllocator.allocateSlot();
    }
    JS_ASSERT(stackSlot <= stackSlotAllocator.stackHeight());

    IonSpew(IonSpew_RegAlloc, "  Allocating canonical spill location %u", stackSlot);
    interval->setAllocation(LStackSlot(stackSlot, reg->isDouble()));
    reg->setCanonicalSpill(*interval->getAllocation());

    if (reg->group()) {
        JS_ASSERT(!reg->group()->spill.isStackSlot());
        reg->group()->spill = *interval->getAllocation();
    }
}



bool
BacktrackingAllocator::resolveControlFlow()
{
    for (size_t i = 0; i < graph.numVirtualRegisters(); i++) {
        BacktrackingVirtualRegister *reg = &vregs[i];

        if (mir->shouldCancel("Backtracking Resolve Control Flow (vreg loop)"))
            return false;

        for (size_t j = 1; j < reg->numIntervals(); j++) {
            LiveInterval *interval = reg->getInterval(j);
            JS_ASSERT(interval->index() == j);

            bool skip = false;
            for (int k = j - 1; k >= 0; k--) {
                LiveInterval *prevInterval = reg->getInterval(k);
                if (prevInterval->start() != interval->start())
                    break;
                if (*prevInterval->getAllocation() == *interval->getAllocation()) {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            CodePosition start = interval->start();
            InstructionData *data = &insData[start];
            if (interval->start() > inputOf(data->block()->firstId())) {
                JS_ASSERT(start == inputOf(data->ins()) || start == outputOf(data->ins()));

                LiveInterval *prevInterval = reg->intervalFor(start.previous());
                if (start.subpos() == CodePosition::INPUT) {
                    if (!moveInput(inputOf(data->ins()), prevInterval, interval))
                        return false;
                } else {
                    if (!moveAfter(outputOf(data->ins()), prevInterval, interval))
                        return false;
                }
            }
        }
    }

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("Backtracking Resolve Control Flow (block loop)"))
            return false;

        LBlock *successor = graph.getBlock(i);
        MBasicBlock *mSuccessor = successor->mir();
        if (mSuccessor->numPredecessors() < 1)
            continue;

        
        for (size_t j = 0; j < successor->numPhis(); j++) {
            LPhi *phi = successor->getPhi(j);
            JS_ASSERT(phi->numDefs() == 1);
            VirtualRegister *vreg = &vregs[phi->getDef(0)];
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

static LDefinition *
FindReusingDefinition(LInstruction *ins, LAllocation *alloc)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition *def = ins->getDef(i);
        if (def->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(def->getReusedInput()) == alloc)
            return def;
    }
    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition *def = ins->getTemp(i);
        if (def->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(def->getReusedInput()) == alloc)
            return def;
    }
    return NULL;
}

bool
BacktrackingAllocator::isReusedInput(LUse *use, LInstruction *ins, bool considerCopy)
{
    if (LDefinition *def = FindReusingDefinition(ins, use))
        return considerCopy || !vregs[def->virtualRegister()].mustCopyInput();
    return false;
}

bool
BacktrackingAllocator::reifyAllocations()
{
    for (size_t i = 0; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister *reg = &vregs[i];

        if (mir->shouldCancel("Backtracking Reify Allocations (main loop)"))
            return false;

        for (size_t j = 0; j < reg->numIntervals(); j++) {
            LiveInterval *interval = reg->getInterval(j);
            JS_ASSERT(interval->index() == j);

            if (interval->index() == 0) {
                reg->def()->setOutput(*interval->getAllocation());
                if (reg->ins()->recoversInput()) {
                    LSnapshot *snapshot = reg->ins()->snapshot();
                    for (size_t i = 0; i < snapshot->numEntries(); i++) {
                        LAllocation *entry = snapshot->getEntry(i);
                        if (entry->isUse() && entry->toUse()->policy() == LUse::RECOVERED_INPUT)
                            *entry = *reg->def()->output();
                    }
                }
            }

            for (UsePositionIterator iter(interval->usesBegin());
                 iter != interval->usesEnd();
                 iter++)
            {
                LAllocation *alloc = iter->use;
                *alloc = *interval->getAllocation();

                
                
                LInstruction *ins = insData[iter->pos].ins();
                if (LDefinition *def = FindReusingDefinition(ins, alloc)) {
                    LiveInterval *outputInterval =
                        vregs[def->virtualRegister()].intervalFor(outputOf(ins));
                    LAllocation *res = outputInterval->getAllocation();
                    LAllocation *sourceAlloc = interval->getAllocation();

                    if (*res != *alloc) {
                        LMoveGroup *group = getInputMoveGroup(inputOf(ins));
                        if (!group->addAfter(sourceAlloc, res))
                            return false;
                        *alloc = *res;
                    }
                }
            }
        }
    }

    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());
    return true;
}

void
BacktrackingAllocator::dumpRegisterGroups()
{
    printf("Register groups:\n");
    for (size_t i = 0; i < graph.numVirtualRegisters(); i++) {
        if (VirtualRegisterGroup *group = vregs[i].group()) {
            bool minimum = true;
            for (size_t j = 0; j < group->registers.length(); j++) {
                if (group->registers[j] < i) {
                    minimum = false;
                    break;
                }
            }
            if (minimum) {
                for (size_t j = 0; j < group->registers.length(); j++)
                    printf(" v%u", group->registers[j]);
                printf("\n");
            }
        }
    }
}

void
BacktrackingAllocator::dumpLiveness()
{
#ifdef DEBUG
    printf("Virtual Registers:\n");

    for (size_t blockIndex = 0; blockIndex < graph.numBlocks(); blockIndex++) {
        LBlock *block = graph.getBlock(blockIndex);
        MBasicBlock *mir = block->mir();

        printf("\nBlock %lu", blockIndex);
        for (size_t i = 0; i < mir->numSuccessors(); i++)
            printf(" [successor %u]", mir->getSuccessor(i)->id());
        printf("\n");

        for (size_t i = 0; i < block->numPhis(); i++) {
            LPhi *phi = block->getPhi(i);

            printf("Phi v%u <-", phi->getDef(0)->virtualRegister());
            for (size_t j = 0; j < phi->numOperands(); j++)
                printf(" v%u", phi->getOperand(j)->toUse()->virtualRegister());
            printf("\n");
        }

        for (LInstructionIterator iter = block->begin(); iter != block->end(); iter++) {
            LInstruction *ins = *iter;

            printf("[%u,%u %s]", inputOf(ins).pos(), outputOf(ins).pos(), ins->opName());

            for (size_t i = 0; i < ins->numTemps(); i++) {
                LDefinition *temp = ins->getTemp(i);
                if (!temp->isBogusTemp())
                    printf(" [temp v%u]", temp->virtualRegister());
            }

            for (size_t i = 0; i < ins->numDefs(); i++) {
                LDefinition *def = ins->getDef(i);
                printf(" [def v%u]", def->virtualRegister());
            }

            for (LInstruction::InputIterator alloc(*ins); alloc.more(); alloc.next()) {
                if (alloc->isUse())
                    printf(" [use v%u]", alloc->toUse()->virtualRegister());
            }

            printf("\n");
        }
    }

    printf("\nLive Ranges:\n\n");

    for (size_t i = 0; i < AnyRegister::Total; i++)
        printf("reg %s: %s\n", AnyRegister::FromCode(i).name(), IntervalString(fixedIntervals[i]));

    for (size_t i = 0; i < graph.numVirtualRegisters(); i++) {
        printf("v%lu:", i);
        VirtualRegister &vreg = vregs[i];
        for (size_t j = 0; j < vreg.numIntervals(); j++) {
            if (j)
                printf(" *");
            printf("%s", IntervalString(vreg.getInterval(j)));
        }
        printf("\n");
    }

    printf("\n");
#endif 
}

void
BacktrackingAllocator::dumpAllocations()
{
#ifdef DEBUG
    printf("Allocations:\n");

    for (size_t i = 0; i < graph.numVirtualRegisters(); i++) {
        printf("v%lu:", i);
        VirtualRegister &vreg = vregs[i];
        for (size_t j = 0; j < vreg.numIntervals(); j++) {
            if (j)
                printf(" *");
            LiveInterval *interval = vreg.getInterval(j);
            printf("%s :: %s", IntervalString(interval), interval->getAllocation()->toString());
        }
        printf("\n");
    }

    printf("\n");
#endif 
}

bool
BacktrackingAllocator::addLiveInterval(LiveIntervalVector &intervals, uint32_t vreg,
                                       CodePosition from, CodePosition to)
{
    LiveInterval *interval = new LiveInterval(vreg, 0);
    return interval->addRange(from, to) && intervals.append(interval);
}





size_t
BacktrackingAllocator::computePriority(const LiveInterval *interval)
{
    
    
    
    size_t lifetimeTotal = 0;

    for (size_t i = 0; i < interval->numRanges(); i++) {
        const LiveInterval::Range *range = interval->getRange(i);
        lifetimeTotal += range->to.pos() - range->from.pos();
    }

    return lifetimeTotal;
}

CodePosition
BacktrackingAllocator::minimalDefEnd(LInstruction *ins)
{
    
    
    
    
    
    while (true) {
        LInstruction *next = insData[outputOf(ins).next()].ins();
        if (!next->isNop() && !next->isOsiPoint())
            break;
        ins = next;
    }
    return outputOf(ins);
}

bool
BacktrackingAllocator::minimalDef(const LiveInterval *interval, LInstruction *ins)
{
    
    return (interval->end() <= minimalDefEnd(ins).next()) &&
        (interval->start() == inputOf(ins) || interval->start() == outputOf(ins));
}

bool
BacktrackingAllocator::minimalUse(const LiveInterval *interval, LInstruction *ins)
{
    
    return (interval->start() == inputOf(ins)) &&
        (interval->end() == outputOf(ins) || interval->end() == outputOf(ins).next());
}

bool
BacktrackingAllocator::minimalInterval(const LiveInterval *interval, bool *pfixed)
{
    if (interval->index() == 0) {
        VirtualRegister &reg = vregs[interval->vreg()];
        if (pfixed)
            *pfixed = reg.def()->policy() == LDefinition::PRESET && reg.def()->output()->isRegister();
        return minimalDef(interval, reg.ins());
    }

    for (UsePositionIterator iter = interval->usesBegin(); iter != interval->usesEnd(); iter++) {
        LUse *use = iter->use;

        switch (use->policy()) {
          case LUse::FIXED:
            if (pfixed)
                *pfixed = true;
            return minimalUse(interval, insData[iter->pos].ins());

          case LUse::REGISTER:
            if (pfixed)
                *pfixed = false;
            return minimalUse(interval, insData[iter->pos].ins());

          default:
            break;
        }
    }

    return false;
}

size_t
BacktrackingAllocator::computeSpillWeight(const LiveInterval *interval)
{
    
    
    bool fixed;
    if (minimalInterval(interval, &fixed))
        return fixed ? 2000000 : 1000000;

    size_t usesTotal = 0;

    if (interval->index() == 0) {
        VirtualRegister *reg = &vregs[interval->vreg()];
        if (reg->def()->policy() == LDefinition::PRESET && reg->def()->output()->isRegister())
            usesTotal += 2000;
        else if (!reg->ins()->isPhi())
            usesTotal += 2000;
    }

    for (UsePositionIterator iter = interval->usesBegin(); iter != interval->usesEnd(); iter++) {
        LUse *use = iter->use;

        switch (use->policy()) {
          case LUse::ANY:
            usesTotal += 1000;
            break;

          case LUse::REGISTER:
          case LUse::FIXED:
            usesTotal += 2000;
            break;

          case LUse::KEEPALIVE:
            break;

          default:
            
            JS_NOT_REACHED("Bad use");
        }
    }

    
    if (interval->hint()->kind() != Requirement::NONE)
        usesTotal += 2000;

    
    
    size_t lifetimeTotal = computePriority(interval);
    return lifetimeTotal ? usesTotal / lifetimeTotal : 0;
}

bool
BacktrackingAllocator::trySplitAcrossHotcode(LiveInterval *interval, bool *success)
{
    
    

    const LiveInterval::Range *hotRange = NULL;

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i)), existing;
        if (hotcode.contains(range, &existing)) {
            hotRange = existing.range;
            break;
        }
    }

    
    if (!hotRange)
        return true;

    
    bool coldCode = false;
    for (size_t i = 0; i < interval->numRanges(); i++) {
        if (!hotRange->contains(interval->getRange(i))) {
            coldCode = true;
            break;
        }
    }
    if (!coldCode)
        return true;

    LiveInterval *hotInterval = new LiveInterval(interval->vreg(), 0);
    LiveInterval *preInterval = NULL, *postInterval = NULL;

    
    
    
    Vector<LiveInterval::Range, 1, SystemAllocPolicy> hotList, coldList;
    for (size_t i = 0; i < interval->numRanges(); i++) {
        LiveInterval::Range hot, coldPre, coldPost;
        interval->getRange(i)->intersect(hotRange, &coldPre, &hot, &coldPost);

        if (!hot.empty() && !hotInterval->addRange(hot.from, hot.to))
            return false;

        if (!coldPre.empty()) {
            if (!preInterval)
                preInterval = new LiveInterval(interval->vreg(), 0);
            if (!preInterval->addRange(coldPre.from, coldPre.to))
                return false;
        }

        if (!coldPost.empty()) {
            if (!postInterval)
                postInterval = new LiveInterval(interval->vreg(), 0);
            if (!postInterval->addRange(coldPost.from, coldPost.to))
                return false;
        }
    }

    JS_ASSERT(preInterval || postInterval);
    JS_ASSERT(hotInterval->numRanges());

    LiveIntervalVector newIntervals;
    if (!newIntervals.append(hotInterval))
        return false;
    if (preInterval && !newIntervals.append(preInterval))
        return false;
    if (postInterval && !newIntervals.append(postInterval))
        return false;

    *success = true;
    return splitAndRequeueInterval(interval, newIntervals);
}

bool
BacktrackingAllocator::trySplitAfterLastRegisterUse(LiveInterval *interval, bool *success)
{
    
    

    CodePosition lastRegisterFrom, lastRegisterTo, lastUse;

    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        LUse *use = iter->use;
        LInstruction *ins = insData[iter->pos].ins();

        
        JS_ASSERT(iter->pos >= lastUse);
        lastUse = inputOf(ins);

        switch (use->policy()) {
          case LUse::ANY:
            if (isReusedInput(iter->use, ins,  true)) {
                lastRegisterFrom = inputOf(ins);
                lastRegisterTo = iter->pos.next();
            }
            break;

          case LUse::REGISTER:
          case LUse::FIXED:
            lastRegisterFrom = inputOf(ins);
            lastRegisterTo = iter->pos.next();
            break;

          default:
            break;
        }
    }

    if (!lastRegisterFrom.pos() || lastRegisterFrom == lastUse) {
        
        return true;
    }

    LiveInterval *preInterval = new LiveInterval(interval->vreg(), 0);
    LiveInterval *postInterval = new LiveInterval(interval->vreg(), 0);

    for (size_t i = 0; i < interval->numRanges(); i++) {
        const LiveInterval::Range *range = interval->getRange(i);

        if (range->from < lastRegisterTo) {
            CodePosition to = (range->to <= lastRegisterTo) ? range->to : lastRegisterTo;
            if (!preInterval->addRange(range->from, to))
                return false;
        }

        if (lastRegisterFrom < range->to) {
            CodePosition from = (lastRegisterFrom <= range->from) ? range->from : lastRegisterFrom;
            if (!postInterval->addRange(from, range->to))
                return false;
        }
    }

    LiveIntervalVector newIntervals;
    if (!newIntervals.append(preInterval) || !newIntervals.append(postInterval))
        return false;

    *success = true;
    return splitAndRequeueInterval(interval, newIntervals);
}

bool
BacktrackingAllocator::splitAtAllRegisterUses(LiveInterval *interval)
{
    
    

    Vector<LiveInterval::Range, 4, SystemAllocPolicy> registerUses;
    uint32_t vreg = interval->vreg();

    CodePosition spillStart = interval->start();
    if (interval->index() == 0) {
        
        
        
        
        
        
        
        VirtualRegister &reg = vregs[vreg];
        if (!reg.ins()->isPhi() &&
            (reg.def()->policy() != LDefinition::PRESET ||
             reg.def()->output()->isRegister()))
        {
            CodePosition from = interval->start();
            CodePosition to = minimalDefEnd(reg.ins()).next();
            if (!registerUses.append(LiveInterval::Range(from, to)))
                return false;
            spillStart = to;
        }
    }

    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        LUse *use = iter->use;
        LInstruction *ins = insData[iter->pos].ins();

        bool isRegister = false;
        switch (use->policy()) {
          case LUse::ANY:
            isRegister = isReusedInput(iter->use, ins);
            break;

          case LUse::REGISTER:
          case LUse::FIXED:
            isRegister = true;
            break;

          default:
            break;
        }

        if (isRegister) {
            
            
            
            if (!registerUses.append(LiveInterval::Range(inputOf(ins), iter->pos.next())))
                return false;
        }
    }

    LiveIntervalVector newIntervals;

    for (size_t i = 0; i < registerUses.length(); i++) {
        
        if (i > 0 && registerUses[i].from == registerUses[i - 1].from)
            continue;
        if (!addLiveInterval(newIntervals, vreg, registerUses[i].from, registerUses[i].to))
            return false;
    }

    if (!addLiveInterval(newIntervals, vreg, spillStart, interval->end()))
        return false;

    return splitAndRequeueInterval(interval, newIntervals);
}

bool
BacktrackingAllocator::chooseIntervalSplit(LiveInterval *interval)
{
    bool success = false;

    if (!trySplitAcrossHotcode(interval, &success))
        return false;
    if (success)
        return true;

    if (!trySplitAfterLastRegisterUse(interval, &success))
        return false;
    if (success)
        return true;

    return splitAtAllRegisterUses(interval);
}
