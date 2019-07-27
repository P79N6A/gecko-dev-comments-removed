





#include "jit/BacktrackingAllocator.h"
#include "jit/BitSet.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;

bool
BacktrackingAllocator::init()
{
    LiveRegisterSet remainingRegisters(allRegisters_.asLiveSet());
    while (!remainingRegisters.emptyGeneral()) {
        AnyRegister reg = AnyRegister(remainingRegisters.takeAnyGeneral());
        registers[reg.code()].allocatable = true;
    }
    while (!remainingRegisters.emptyFloat()) {
        AnyRegister reg = AnyRegister(remainingRegisters.takeAnyFloat());
        registers[reg.code()].allocatable = true;
    }

    LifoAlloc* lifoAlloc = mir->alloc().lifoAlloc();
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        registers[i].reg = AnyRegister::FromCode(i);
        registers[i].allocations.setAllocator(lifoAlloc);

        LiveInterval* fixed = fixedIntervals[i];
        for (size_t j = 0; j < fixed->numRanges(); j++) {
            AllocatedRange range(fixed, fixed->getRange(j));
            if (!registers[i].allocations.insert(range))
                return false;
        }
    }

    hotcode.setAllocator(lifoAlloc);

    
    
    
    

    LiveInterval* hotcodeInterval = LiveInterval::New(alloc(), 0);

    LBlock* backedge = nullptr;
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock* block = graph.getBlock(i);

        
        
        
        if (block->mir()->isLoopHeader())
            backedge = block->mir()->backedge()->lir();

        if (block == backedge) {
            LBlock* header = block->mir()->loopHeaderOfBackedge()->lir();
            CodePosition from = entryOf(header);
            CodePosition to = exitOf(block).next();
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

bool
BacktrackingAllocator::go()
{
    JitSpew(JitSpew_RegAlloc, "Beginning register allocation");

    if (!buildLivenessInfo())
        return false;

    if (!init())
        return false;

    if (JitSpewEnabled(JitSpew_RegAlloc))
        dumpFixedRanges();

    if (!allocationQueue.reserve(graph.numVirtualRegisters() * 3 / 2))
        return false;

    JitSpew(JitSpew_RegAlloc, "Beginning grouping and queueing registers");
    if (!groupAndQueueRegisters())
        return false;
    JitSpew(JitSpew_RegAlloc, "Grouping and queueing registers complete");

    if (JitSpewEnabled(JitSpew_RegAlloc))
        dumpRegisterGroups();

    JitSpew(JitSpew_RegAlloc, "Beginning main allocation loop");

    
    while (!allocationQueue.empty()) {
        if (mir->shouldCancel("Backtracking Allocation"))
            return false;

        QueueItem item = allocationQueue.removeHighest();
        if (item.interval ? !processInterval(item.interval) : !processGroup(item.group))
            return false;
    }
    JitSpew(JitSpew_RegAlloc, "Main allocation loop complete");

    if (!pickStackSlots())
        return false;

    if (JitSpewEnabled(JitSpew_RegAlloc))
        dumpAllocations();

    if (!resolveControlFlow())
        return false;

    if (!reifyAllocations())
        return false;

    if (!populateSafepoints())
        return false;

    if (!annotateMoveGroups())
        return false;

    return true;
}

static bool
LifetimesOverlap(BacktrackingVirtualRegister* reg0, BacktrackingVirtualRegister* reg1)
{
    
    
    MOZ_ASSERT(reg0->numIntervals() <= 2 && reg1->numIntervals() <= 2);

    LiveInterval* interval0 = reg0->getInterval(0);
    LiveInterval* interval1 = reg1->getInterval(0);

    
    
    size_t index0 = 0, index1 = 0;
    while (index0 < interval0->numRanges() && index1 < interval1->numRanges()) {
        const LiveInterval::Range
            *range0 = interval0->getRange(index0),
            *range1 = interval1->getRange(index1);
        if (range0->from >= range1->to)
            index0++;
        else if (range1->from >= range0->to)
            index1++;
        else
            return true;
    }

    return false;
}

bool
BacktrackingAllocator::canAddToGroup(VirtualRegisterGroup* group, BacktrackingVirtualRegister* reg)
{
    for (size_t i = 0; i < group->registers.length(); i++) {
        if (LifetimesOverlap(reg, &vregs[group->registers[i]]))
            return false;
    }
    return true;
}

static bool
IsArgumentSlotDefinition(LDefinition* def)
{
    return def->policy() == LDefinition::FIXED && def->output()->isArgument();
}

static bool
IsThisSlotDefinition(LDefinition* def)
{
    return IsArgumentSlotDefinition(def) &&
           def->output()->toArgument()->index() < THIS_FRAME_ARGSLOT + sizeof(Value);
}

bool
BacktrackingAllocator::tryGroupRegisters(uint32_t vreg0, uint32_t vreg1)
{
    
    
    
    BacktrackingVirtualRegister* reg0 = &vregs[vreg0];
    BacktrackingVirtualRegister* reg1 = &vregs[vreg1];

    if (!reg0->isCompatibleVReg(*reg1))
        return true;

    
    
    
    
    if (IsThisSlotDefinition(reg0->def()) || IsThisSlotDefinition(reg1->def())) {
        if (*reg0->def()->output() != *reg1->def()->output())
            return true;
    }

    
    
    
    if (IsArgumentSlotDefinition(reg0->def()) || IsArgumentSlotDefinition(reg1->def())) {
        JSScript* script = graph.mir().entryBlock()->info().script();
        if (script && script->argumentsHasVarBinding()) {
            if (*reg0->def()->output() != *reg1->def()->output())
                return true;
        }
    }

    VirtualRegisterGroup* group0 = reg0->group();
    VirtualRegisterGroup* group1 = reg1->group();

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

    if (LifetimesOverlap(reg0, reg1))
        return true;

    VirtualRegisterGroup* group = new(alloc()) VirtualRegisterGroup(alloc());
    if (!group->registers.append(vreg0) || !group->registers.append(vreg1))
        return false;

    reg0->setGroup(group);
    reg1->setGroup(group);
    return true;
}

bool
BacktrackingAllocator::tryGroupReusedRegister(uint32_t def, uint32_t use)
{
    BacktrackingVirtualRegister& reg = vregs[def];
    BacktrackingVirtualRegister& usedReg = vregs[use];

    
    
    
    
    

    if (reg.intervalFor(inputOf(reg.ins()))) {
        MOZ_ASSERT(reg.isTemp());
        reg.setMustCopyInput();
        return true;
    }

    if (!usedReg.intervalFor(outputOf(reg.ins()))) {
        
        
        
        return tryGroupRegisters(use, def);
    }

    
    
    
    
    
    
    
    

    if (usedReg.numIntervals() != 1 ||
        (usedReg.def()->isFixed() && !usedReg.def()->output()->isRegister())) {
        reg.setMustCopyInput();
        return true;
    }
    LiveInterval* interval = usedReg.getInterval(0);
    LBlock* block = reg.ins()->block();

    
    
    if (interval->end() > exitOf(block)) {
        reg.setMustCopyInput();
        return true;
    }

    for (UsePositionIterator iter = interval->usesBegin(); iter != interval->usesEnd(); iter++) {
        if (iter->pos <= inputOf(reg.ins()))
            continue;

        LUse* use = iter->use;
        if (FindReusingDefinition(insData[iter->pos], use)) {
            reg.setMustCopyInput();
            return true;
        }
        if (use->policy() != LUse::ANY && use->policy() != LUse::KEEPALIVE) {
            reg.setMustCopyInput();
            return true;
        }
    }

    LiveInterval* preInterval = LiveInterval::New(alloc(), interval->vreg(), 0);
    for (size_t i = 0; i < interval->numRanges(); i++) {
        const LiveInterval::Range* range = interval->getRange(i);
        MOZ_ASSERT(range->from <= inputOf(reg.ins()));

        CodePosition to = Min(range->to, outputOf(reg.ins()));
        if (!preInterval->addRange(range->from, to))
            return false;
    }

    
    
    
    LiveInterval* postInterval = LiveInterval::New(alloc(), interval->vreg(), 0);
    if (!postInterval->addRange(inputOf(reg.ins()), interval->end()))
        return false;

    LiveIntervalVector newIntervals;
    if (!newIntervals.append(preInterval) || !newIntervals.append(postInterval))
        return false;

    distributeUses(interval, newIntervals);

    JitSpew(JitSpew_RegAlloc, "  splitting reused input at %u to try to help grouping",
            inputOf(reg.ins()));

    if (!split(interval, newIntervals))
        return false;

    MOZ_ASSERT(usedReg.numIntervals() == 2);

    usedReg.setCanonicalSpillExclude(inputOf(reg.ins()));

    return tryGroupRegisters(use, def);
}

bool
BacktrackingAllocator::groupAndQueueRegisters()
{
    
    
    if (MBasicBlock* osr = graph.mir().osrBlock()) {
        size_t originalVreg = 1;
        for (LInstructionIterator iter = osr->lir()->begin(); iter != osr->lir()->end(); iter++) {
            if (iter->isParameter()) {
                for (size_t i = 0; i < iter->numDefs(); i++) {
                    DebugOnly<bool> found = false;
                    uint32_t paramVreg = iter->getDef(i)->virtualRegister();
                    for (; originalVreg < paramVreg; originalVreg++) {
                        if (*vregs[originalVreg].def()->output() == *iter->getDef(i)->output()) {
                            MOZ_ASSERT(vregs[originalVreg].ins()->isParameter());
                            if (!tryGroupRegisters(originalVreg, paramVreg))
                                return false;
                            MOZ_ASSERT(vregs[originalVreg].group() == vregs[paramVreg].group());
                            found = true;
                            break;
                        }
                    }
                    MOZ_ASSERT(found);
                }
            }
        }
    }

    
    
    MOZ_ASSERT(vregs[0u].numIntervals() == 0);
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        BacktrackingVirtualRegister& reg = vregs[i];
        if (!reg.numIntervals())
            continue;

        if (reg.def()->policy() == LDefinition::MUST_REUSE_INPUT) {
            LUse* use = reg.ins()->getOperand(reg.def()->getReusedInput())->toUse();
            if (!tryGroupReusedRegister(i, use->virtualRegister()))
                return false;
        }
    }

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock* block = graph.getBlock(i);
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi* phi = block->getPhi(j);
            uint32_t output = phi->getDef(0)->virtualRegister();
            for (size_t k = 0, kend = phi->numOperands(); k < kend; k++) {
                uint32_t input = phi->getOperand(k)->toUse()->virtualRegister();
                if (!tryGroupRegisters(input, output))
                    return false;
            }
        }
    }

    
    MOZ_ASSERT(vregs[0u].numIntervals() == 0);
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        if (mir->shouldCancel("Backtracking Enqueue Registers"))
            return false;

        BacktrackingVirtualRegister& reg = vregs[i];
        MOZ_ASSERT(reg.numIntervals() <= 2);
        MOZ_ASSERT(!reg.canonicalSpill());

        if (!reg.numIntervals())
            continue;

        
        
        LDefinition* def = reg.def();
        if (def->policy() == LDefinition::FIXED && !def->output()->isRegister()) {
            MOZ_ASSERT(!def->output()->isStackSlot());
            reg.setCanonicalSpill(*def->output());
            if (reg.group() && reg.group()->spill.isUse())
                reg.group()->spill = *def->output();
        }

        
        
        
        
        
        
        
        size_t start = 0;
        if (VirtualRegisterGroup* group = reg.group()) {
            if (i == group->canonicalReg()) {
                size_t priority = computePriority(group);
                if (!allocationQueue.insert(QueueItem(group, priority)))
                    return false;
            }
            start++;
        }
        for (; start < reg.numIntervals(); start++) {
            LiveInterval* interval = reg.getInterval(start);
            if (interval->numRanges() > 0) {
                size_t priority = computePriority(interval);
                if (!allocationQueue.insert(QueueItem(interval, priority)))
                    return false;
            }
        }
    }

    return true;
}

static const size_t MAX_ATTEMPTS = 2;

bool
BacktrackingAllocator::tryAllocateFixed(LiveInterval* interval, bool* success,
                                        bool* pfixed, LiveIntervalVector& conflicting)
{
    
    if (!interval->requirement()->allocation().isRegister()) {
        JitSpew(JitSpew_RegAlloc, "  stack allocation requirement");
        interval->setAllocation(interval->requirement()->allocation());
        *success = true;
        return true;
    }

    AnyRegister reg = interval->requirement()->allocation().toRegister();
    return tryAllocateRegister(registers[reg.code()], interval, success, pfixed, conflicting);
}

bool
BacktrackingAllocator::tryAllocateNonFixed(LiveInterval* interval, bool* success,
                                           bool* pfixed, LiveIntervalVector& conflicting)
{
    
    
    
    
    
    if (interval->hint()->kind() == Requirement::FIXED) {
        AnyRegister reg = interval->hint()->allocation().toRegister();
        if (!tryAllocateRegister(registers[reg.code()], interval, success, pfixed, conflicting))
            return false;
        if (*success)
            return true;
    }

    
    if (interval->requirement()->kind() == Requirement::NONE &&
        interval->hint()->kind() != Requirement::REGISTER)
    {
        spill(interval);
        *success = true;
        return true;
    }

    if (conflicting.empty() || minimalInterval(interval)) {
        
        
        for (size_t i = 0; i < AnyRegister::Total; i++) {
            if (!tryAllocateRegister(registers[i], interval, success, pfixed, conflicting))
                return false;
            if (*success)
                return true;
        }
    }

    
    
    if (interval->requirement()->kind() == Requirement::NONE) {
        spill(interval);
        *success = true;
        return true;
    }

    
    MOZ_ASSERT(!*success);
    return true;
}

bool
BacktrackingAllocator::processInterval(LiveInterval* interval)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "Allocating %s [priority %lu] [weight %lu]",
                interval->toString(), computePriority(interval), computeSpillWeight(interval));
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    bool canAllocate = setIntervalRequirement(interval);

    bool fixed;
    LiveIntervalVector conflicting;
    for (size_t attempt = 0;; attempt++) {
        if (canAllocate) {
            bool success = false;
            fixed = false;
            conflicting.clear();

            
            if (interval->requirement()->kind() == Requirement::FIXED) {
                if (!tryAllocateFixed(interval, &success, &fixed, conflicting))
                    return false;
            } else {
                if (!tryAllocateNonFixed(interval, &success, &fixed, conflicting))
                    return false;
            }

            
            if (success)
                return true;

            
            
            if (attempt < MAX_ATTEMPTS &&
                !fixed &&
                !conflicting.empty() &&
                maximumSpillWeight(conflicting) < computeSpillWeight(interval))
            {
                for (size_t i = 0; i < conflicting.length(); i++) {
                    if (!evictInterval(conflicting[i]))
                        return false;
                }
                continue;
            }
        }

        
        
        
        
        MOZ_ASSERT(!minimalInterval(interval));

        LiveInterval* conflict = conflicting.empty() ? nullptr : conflicting[0];
        return chooseIntervalSplit(interval, canAllocate && fixed, conflict);
    }
}

bool
BacktrackingAllocator::processGroup(VirtualRegisterGroup* group)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "Allocating group v%u [priority %lu] [weight %lu]",
                group->registers[0], computePriority(group), computeSpillWeight(group));
    }

    bool fixed;
    LiveInterval* conflict;
    for (size_t attempt = 0;; attempt++) {
        
        fixed = false;
        conflict = nullptr;
        for (size_t i = 0; i < AnyRegister::Total; i++) {
            bool success;
            if (!tryAllocateGroupRegister(registers[i], group, &success, &fixed, &conflict))
                return false;
            if (success) {
                conflict = nullptr;
                break;
            }
        }

        if (attempt < MAX_ATTEMPTS &&
            !fixed &&
            conflict &&
            conflict->hasVreg() &&
            computeSpillWeight(conflict) < computeSpillWeight(group))
        {
            if (!evictInterval(conflict))
                return false;
            continue;
        }

        for (size_t i = 0; i < group->registers.length(); i++) {
            VirtualRegister& reg = vregs[group->registers[i]];
            MOZ_ASSERT(reg.numIntervals() <= 2);
            if (!processInterval(reg.getInterval(0)))
                return false;
        }

        return true;
    }
}

bool
BacktrackingAllocator::setIntervalRequirement(LiveInterval* interval)
{
    
    
    
    interval->setHint(Requirement());
    interval->setRequirement(Requirement());

    BacktrackingVirtualRegister* reg = &vregs[interval->vreg()];

    
    if (VirtualRegisterGroup* group = reg->group()) {
        if (group->allocation.isRegister()) {
            if (JitSpewEnabled(JitSpew_RegAlloc)) {
                JitSpew(JitSpew_RegAlloc, "  Hint %s, used by group allocation",
                        group->allocation.toString());
            }
            interval->setHint(Requirement(group->allocation));
        }
    }

    if (interval->index() == 0) {
        
        

        LDefinition::Policy policy = reg->def()->policy();
        if (policy == LDefinition::FIXED) {
            
            if (JitSpewEnabled(JitSpew_RegAlloc)) {
                JitSpew(JitSpew_RegAlloc, "  Requirement %s, fixed by definition",
                        reg->def()->output()->toString());
            }
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

            if (JitSpewEnabled(JitSpew_RegAlloc)) {
                JitSpew(JitSpew_RegAlloc, "  Requirement %s, due to use at %u",
                        required.name(), iter->pos.bits());
            }

            
            
            
            if (!interval->addRequirement(Requirement(LAllocation(required))))
                return false;
        } else if (policy == LUse::REGISTER) {
            if (!interval->addRequirement(Requirement(Requirement::REGISTER)))
                return false;
        } else if (policy == LUse::ANY) {
            
            interval->addHint(Requirement(Requirement::REGISTER));
        }
    }

    return true;
}

bool
BacktrackingAllocator::tryAllocateGroupRegister(PhysicalRegister& r, VirtualRegisterGroup* group,
                                                bool* psuccess, bool* pfixed, LiveInterval** pconflicting)
{
    *psuccess = false;

    if (!r.allocatable)
        return true;

    if (!vregs[group->registers[0]].isCompatibleReg(r.reg))
        return true;

    bool allocatable = true;
    LiveInterval* conflicting = nullptr;

    for (size_t i = 0; i < group->registers.length(); i++) {
        VirtualRegister& reg = vregs[group->registers[i]];
        MOZ_ASSERT(reg.numIntervals() <= 2);
        LiveInterval* interval = reg.getInterval(0);

        for (size_t j = 0; j < interval->numRanges(); j++) {
            AllocatedRange range(interval, interval->getRange(j)), existing;
            if (r.allocations.contains(range, &existing)) {
                if (conflicting) {
                    if (conflicting != existing.interval)
                        return true;
                } else {
                    conflicting = existing.interval;
                }
                allocatable = false;
            }
        }
    }

    if (!allocatable) {
        MOZ_ASSERT(conflicting);
        if (!*pconflicting || computeSpillWeight(conflicting) < computeSpillWeight(*pconflicting))
            *pconflicting = conflicting;
        if (!conflicting->hasVreg())
            *pfixed = true;
        return true;
    }

    *psuccess = true;

    group->allocation = LAllocation(r.reg);
    return true;
}

bool
BacktrackingAllocator::tryAllocateRegister(PhysicalRegister& r, LiveInterval* interval,
                                           bool* success, bool* pfixed, LiveIntervalVector& conflicting)
{
    *success = false;

    if (!r.allocatable)
        return true;

    BacktrackingVirtualRegister* reg = &vregs[interval->vreg()];
    if (!reg->isCompatibleReg(r.reg))
        return true;

    MOZ_ASSERT_IF(interval->requirement()->kind() == Requirement::FIXED,
                  interval->requirement()->allocation() == LAllocation(r.reg));

    LiveIntervalVector aliasedConflicting;

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i)), existing;
        for (size_t a = 0; a < r.reg.numAliased(); a++) {
            PhysicalRegister& rAlias = registers[r.reg.aliased(a).code()];
            if (!rAlias.allocations.contains(range, &existing))
                continue;
            if (existing.interval->hasVreg()) {
                MOZ_ASSERT(existing.interval->getAllocation()->toRegister() == rAlias.reg);
                bool duplicate = false;
                for (size_t i = 0; i < aliasedConflicting.length(); i++) {
                    if (aliasedConflicting[i] == existing.interval) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate && !aliasedConflicting.append(existing.interval))
                    return false;
            } else {
                if (JitSpewEnabled(JitSpew_RegAlloc)) {
                    JitSpew(JitSpew_RegAlloc, "  %s collides with fixed use %s",
                            rAlias.reg.name(), existing.range->toString());
                }
                *pfixed = true;
                return true;
            }
        }
    }

    if (!aliasedConflicting.empty()) {
        
        
        
        

        if (JitSpewEnabled(JitSpew_RegAlloc)) {
            if (aliasedConflicting.length() == 1) {
                LiveInterval* existing = aliasedConflicting[0];
                JitSpew(JitSpew_RegAlloc, "  %s collides with v%u[%u] %s [weight %lu]",
                        r.reg.name(), existing->vreg(), existing->index(),
                        existing->rangesToString(), computeSpillWeight(existing));
            } else {
                JitSpew(JitSpew_RegAlloc, "  %s collides with the following", r.reg.name());
                for (size_t i = 0; i < aliasedConflicting.length(); i++) {
                    LiveInterval* existing = aliasedConflicting[i];
                    JitSpew(JitSpew_RegAlloc, "      v%u[%u] %s [weight %lu]",
                            existing->vreg(), existing->index(),
                            existing->rangesToString(), computeSpillWeight(existing));
                }
            }
        }

        if (conflicting.empty()) {
            if (!conflicting.appendAll(aliasedConflicting))
                return false;
        } else {
            if (maximumSpillWeight(aliasedConflicting) < maximumSpillWeight(conflicting)) {
                conflicting.clear();
                if (!conflicting.appendAll(aliasedConflicting))
                    return false;
            }
        }
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  allocated to %s", r.reg.name());

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i));
        if (!r.allocations.insert(range))
            return false;
    }

    
    if (VirtualRegisterGroup* group = reg->group()) {
        if (!group->allocation.isRegister())
            group->allocation = LAllocation(r.reg);
    }

    interval->setAllocation(LAllocation(r.reg));
    *success = true;
    return true;
}

bool
BacktrackingAllocator::evictInterval(LiveInterval* interval)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "  Evicting %s [priority %lu] [weight %lu]",
                interval->toString(), computePriority(interval), computeSpillWeight(interval));
    }

    MOZ_ASSERT(interval->getAllocation()->isRegister());

    AnyRegister reg(interval->getAllocation()->toRegister());
    PhysicalRegister& physical = registers[reg.code()];
    MOZ_ASSERT(physical.reg == reg && physical.allocatable);

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i));
        physical.allocations.remove(range);
    }

    interval->setAllocation(LAllocation());

    size_t priority = computePriority(interval);
    return allocationQueue.insert(QueueItem(interval, priority));
}

void
BacktrackingAllocator::distributeUses(LiveInterval* interval,
                                      const LiveIntervalVector& newIntervals)
{
    MOZ_ASSERT(newIntervals.length() >= 2);

    
    
    
    
    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        CodePosition pos = iter->pos;
        LiveInterval* addInterval = nullptr;
        for (size_t i = 0; i < newIntervals.length(); i++) {
            LiveInterval* newInterval = newIntervals[i];
            if (newInterval->covers(pos)) {
                if (!addInterval || newInterval->start() < addInterval->start())
                    addInterval = newInterval;
            }
        }
        addInterval->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
    }
}

bool
BacktrackingAllocator::split(LiveInterval* interval,
                             const LiveIntervalVector& newIntervals)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "    splitting interval %s into:", interval->toString());
        for (size_t i = 0; i < newIntervals.length(); i++) {
            JitSpew(JitSpew_RegAlloc, "      %s", newIntervals[i]->toString());
            MOZ_ASSERT(newIntervals[i]->start() >= interval->start());
            MOZ_ASSERT(newIntervals[i]->end() <= interval->end());
        }
    }

    MOZ_ASSERT(newIntervals.length() >= 2);

    
    LiveInterval* first = newIntervals[0];
    for (size_t i = 1; i < newIntervals.length(); i++) {
        if (newIntervals[i]->start() < first->start())
            first = newIntervals[i];
    }

    
    VirtualRegister* reg = &vregs[interval->vreg()];
    reg->replaceInterval(interval, first);
    for (size_t i = 0; i < newIntervals.length(); i++) {
        if (newIntervals[i] != first && !reg->addInterval(newIntervals[i]))
            return false;
    }

    return true;
}

bool BacktrackingAllocator::requeueIntervals(const LiveIntervalVector& newIntervals)
{
    
    for (size_t i = 0; i < newIntervals.length(); i++) {
        LiveInterval* newInterval = newIntervals[i];
        size_t priority = computePriority(newInterval);
        if (!allocationQueue.insert(QueueItem(newInterval, priority)))
            return false;
    }
    return true;
}

void
BacktrackingAllocator::spill(LiveInterval* interval)
{
    JitSpew(JitSpew_RegAlloc, "  Spilling interval");

    MOZ_ASSERT(interval->requirement()->kind() == Requirement::NONE);
    MOZ_ASSERT(!interval->getAllocation()->isStackSlot());

    
    MOZ_ASSERT(interval->hasVreg());

    BacktrackingVirtualRegister* reg = &vregs[interval->vreg()];

    if (LiveInterval* spillInterval = interval->spillInterval()) {
        JitSpew(JitSpew_RegAlloc, "    Spilling to existing spill interval");
        while (!interval->usesEmpty())
            spillInterval->addUse(interval->popUse());
        reg->removeInterval(interval);
        return;
    }

    bool useCanonical = !reg->hasCanonicalSpillExclude()
        || interval->start() < reg->canonicalSpillExclude();

    if (useCanonical) {
        if (reg->canonicalSpill()) {
            JitSpew(JitSpew_RegAlloc, "    Picked canonical spill location %s",
                    reg->canonicalSpill()->toString());
            interval->setAllocation(*reg->canonicalSpill());
            return;
        }

        if (reg->group() && !reg->group()->spill.isUse()) {
            JitSpew(JitSpew_RegAlloc, "    Reusing group spill location %s",
                    reg->group()->spill.toString());
            interval->setAllocation(reg->group()->spill);
            reg->setCanonicalSpill(reg->group()->spill);
            return;
        }
    }

    uint32_t virtualSlot = numVirtualStackSlots++;

    
    
    LStackSlot alloc(LAllocation::DATA_MASK - virtualSlot);
    interval->setAllocation(alloc);

    JitSpew(JitSpew_RegAlloc, "    Allocating spill location %s", alloc.toString());

    if (useCanonical) {
        reg->setCanonicalSpill(alloc);
        if (reg->group())
            reg->group()->spill = alloc;
    }
}

bool
BacktrackingAllocator::pickStackSlots()
{
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        BacktrackingVirtualRegister* reg = &vregs[i];

        if (mir->shouldCancel("Backtracking Pick Stack Slots"))
            return false;

        for (size_t j = 0; j < reg->numIntervals(); j++) {
            LiveInterval* interval = reg->getInterval(j);
            if (!pickStackSlot(interval))
                return false;
        }
    }

    return true;
}

bool
BacktrackingAllocator::pickStackSlot(LiveInterval* interval)
{
    LAllocation alloc = *interval->getAllocation();
    MOZ_ASSERT(!alloc.isUse());

    if (!isVirtualStackSlot(alloc))
        return true;

    BacktrackingVirtualRegister& reg = vregs[interval->vreg()];

    
    LiveIntervalVector commonIntervals;

    if (!commonIntervals.append(interval))
        return false;

    if (reg.canonicalSpill() && alloc == *reg.canonicalSpill()) {
        
        for (size_t i = 0; i < reg.numIntervals(); i++) {
            LiveInterval* ninterval = reg.getInterval(i);
            if (ninterval != interval && *ninterval->getAllocation() == alloc) {
                if (!commonIntervals.append(ninterval))
                    return false;
            }
        }

        
        
        if (reg.group() && alloc == reg.group()->spill) {
            for (size_t i = 0; i < reg.group()->registers.length(); i++) {
                uint32_t nvreg = reg.group()->registers[i];
                if (nvreg == interval->vreg())
                    continue;
                BacktrackingVirtualRegister& nreg = vregs[nvreg];
                for (size_t j = 0; j < nreg.numIntervals(); j++) {
                    LiveInterval* ninterval = nreg.getInterval(j);
                    if (*ninterval->getAllocation() == alloc) {
                        if (!commonIntervals.append(ninterval))
                            return false;
                    }
                }
            }
        }
    } else {
        MOZ_ASSERT_IF(reg.group(), alloc != reg.group()->spill);
    }

    if (!reuseOrAllocateStackSlot(commonIntervals, reg.type(), &alloc))
        return false;

    MOZ_ASSERT(!isVirtualStackSlot(alloc));

    
    for (size_t i = 0; i < commonIntervals.length(); i++)
        commonIntervals[i]->setAllocation(alloc);

    return true;
}

bool
BacktrackingAllocator::reuseOrAllocateStackSlot(const LiveIntervalVector& intervals, LDefinition::Type type,
                                                LAllocation* palloc)
{
    SpillSlotList* slotList;
    switch (StackSlotAllocator::width(type)) {
      case 4:  slotList = &normalSlots; break;
      case 8:  slotList = &doubleSlots; break;
      case 16: slotList = &quadSlots;   break;
      default:
        MOZ_CRASH("Bad width");
    }

    
    
    static const size_t MAX_SEARCH_COUNT = 10;

    if (!slotList->empty()) {
        size_t searches = 0;
        SpillSlot* stop = nullptr;
        while (true) {
            SpillSlot* spill = *slotList->begin();
            if (!stop) {
                stop = spill;
            } else if (stop == spill) {
                
                break;
            }

            bool success = true;
            for (size_t i = 0; i < intervals.length() && success; i++) {
                LiveInterval* interval = intervals[i];
                for (size_t j = 0; j < interval->numRanges(); j++) {
                    AllocatedRange range(interval, interval->getRange(j)), existing;
                    if (spill->allocated.contains(range, &existing)) {
                        success = false;
                        break;
                    }
                }
            }
            if (success) {
                
                
                if (!insertAllRanges(spill->allocated, intervals))
                    return false;
                *palloc = spill->alloc;
                return true;
            }

            
            
            
            slotList->popFront();
            slotList->pushBack(spill);

            if (++searches == MAX_SEARCH_COUNT)
                break;
        }
    }

    
    uint32_t stackSlot = stackSlotAllocator.allocateSlot(type);

    
    if (isVirtualStackSlot(LStackSlot(stackSlot)))
        return false;

    SpillSlot* spill = new(alloc()) SpillSlot(stackSlot, alloc().lifoAlloc());
    if (!spill)
        return false;

    if (!insertAllRanges(spill->allocated, intervals))
        return false;

    *palloc = spill->alloc;

    slotList->pushFront(spill);
    return true;
}

bool
BacktrackingAllocator::insertAllRanges(AllocatedRangeSet& set, const LiveIntervalVector& intervals)
{
    for (size_t i = 0; i < intervals.length(); i++) {
        LiveInterval* interval = intervals[i];
        for (size_t j = 0; j < interval->numRanges(); j++) {
            AllocatedRange range(interval, interval->getRange(j));
            if (!set.insert(range))
                return false;
        }
    }
    return true;
}



bool
BacktrackingAllocator::resolveControlFlow()
{
    JitSpew(JitSpew_RegAlloc, "Resolving control flow (vreg loop)");

    
    MOZ_ASSERT(vregs[0u].numIntervals() == 0);
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        BacktrackingVirtualRegister* reg = &vregs[i];

        if (mir->shouldCancel("Backtracking Resolve Control Flow (vreg loop)"))
            return false;

        for (size_t j = 1; j < reg->numIntervals(); j++) {
            LiveInterval* interval = reg->getInterval(j);
            MOZ_ASSERT(interval->index() == j);

            bool skip = false;
            for (int k = j - 1; k >= 0; k--) {
                LiveInterval* prevInterval = reg->getInterval(k);
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
            LNode* ins = insData[start];
            if (start > entryOf(ins->block())) {
                MOZ_ASSERT(start == inputOf(ins) || start == outputOf(ins));

                LiveInterval* prevInterval = reg->intervalFor(start.previous());
                if (start.subpos() == CodePosition::INPUT) {
                    if (!moveInput(ins->toInstruction(), prevInterval, interval, reg->type()))
                        return false;
                } else {
                    if (!moveAfter(ins->toInstruction(), prevInterval, interval, reg->type()))
                        return false;
                }
            }
        }
    }

    JitSpew(JitSpew_RegAlloc, "Resolving control flow (block loop)");

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("Backtracking Resolve Control Flow (block loop)"))
            return false;

        LBlock* successor = graph.getBlock(i);
        MBasicBlock* mSuccessor = successor->mir();
        if (mSuccessor->numPredecessors() < 1)
            continue;

        
        for (size_t j = 0; j < successor->numPhis(); j++) {
            LPhi* phi = successor->getPhi(j);
            MOZ_ASSERT(phi->numDefs() == 1);
            LDefinition* def = phi->getDef(0);
            VirtualRegister* vreg = &vregs[def];
            LiveInterval* to = vreg->intervalFor(entryOf(successor));
            MOZ_ASSERT(to);

            for (size_t k = 0; k < mSuccessor->numPredecessors(); k++) {
                LBlock* predecessor = mSuccessor->getPredecessor(k)->lir();
                MOZ_ASSERT(predecessor->mir()->numSuccessors() == 1);

                LAllocation* input = phi->getOperand(k);
                LiveInterval* from = vregs[input].intervalFor(exitOf(predecessor));
                MOZ_ASSERT(from);

                if (!moveAtExit(predecessor, from, to, def->type()))
                    return false;
            }
        }

        
        BitSet& live = liveIn[mSuccessor->id()];

        for (BitSet::Iterator liveRegId(live); liveRegId; ++liveRegId) {
            VirtualRegister& reg = vregs[*liveRegId];

            for (size_t j = 0; j < mSuccessor->numPredecessors(); j++) {
                LBlock* predecessor = mSuccessor->getPredecessor(j)->lir();

                for (size_t k = 0; k < reg.numIntervals(); k++) {
                    LiveInterval* to = reg.getInterval(k);
                    if (!to->covers(entryOf(successor)))
                        continue;
                    if (to->covers(exitOf(predecessor)))
                        continue;

                    LiveInterval* from = reg.intervalFor(exitOf(predecessor));

                    if (mSuccessor->numPredecessors() > 1) {
                        MOZ_ASSERT(predecessor->mir()->numSuccessors() == 1);
                        if (!moveAtExit(predecessor, from, to, reg.type()))
                            return false;
                    } else {
                        if (!moveAtEntry(successor, from, to, reg.type()))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

bool
BacktrackingAllocator::isReusedInput(LUse* use, LNode* ins, bool considerCopy)
{
    if (LDefinition* def = FindReusingDefinition(ins, use))
        return considerCopy || !vregs[def->virtualRegister()].mustCopyInput();
    return false;
}

bool
BacktrackingAllocator::isRegisterUse(LUse* use, LNode* ins, bool considerCopy)
{
    switch (use->policy()) {
      case LUse::ANY:
        return isReusedInput(use, ins, considerCopy);

      case LUse::REGISTER:
      case LUse::FIXED:
        return true;

      default:
        return false;
    }
}

bool
BacktrackingAllocator::isRegisterDefinition(LiveInterval* interval)
{
    if (interval->index() != 0)
        return false;

    VirtualRegister& reg = vregs[interval->vreg()];
    if (reg.ins()->isPhi())
        return false;

    if (reg.def()->policy() == LDefinition::FIXED && !reg.def()->output()->isRegister())
        return false;

    return true;
}

bool
BacktrackingAllocator::reifyAllocations()
{
    JitSpew(JitSpew_RegAlloc, "Reifying Allocations");

    
    MOZ_ASSERT(vregs[0u].numIntervals() == 0);
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister* reg = &vregs[i];

        if (mir->shouldCancel("Backtracking Reify Allocations (main loop)"))
            return false;

        for (size_t j = 0; j < reg->numIntervals(); j++) {
            LiveInterval* interval = reg->getInterval(j);
            MOZ_ASSERT(interval->index() == j);

            if (interval->index() == 0) {
                reg->def()->setOutput(*interval->getAllocation());
                if (reg->ins()->recoversInput()) {
                    LSnapshot* snapshot = reg->ins()->toInstruction()->snapshot();
                    for (size_t i = 0; i < snapshot->numEntries(); i++) {
                        LAllocation* entry = snapshot->getEntry(i);
                        if (entry->isUse() && entry->toUse()->policy() == LUse::RECOVERED_INPUT)
                            *entry = *reg->def()->output();
                    }
                }
            }

            for (UsePositionIterator iter(interval->usesBegin());
                 iter != interval->usesEnd();
                 iter++)
            {
                LAllocation* alloc = iter->use;
                *alloc = *interval->getAllocation();

                
                
                LNode* ins = insData[iter->pos];
                if (LDefinition* def = FindReusingDefinition(ins, alloc)) {
                    LiveInterval* outputInterval =
                        vregs[def->virtualRegister()].intervalFor(outputOf(ins));
                    LAllocation* res = outputInterval->getAllocation();
                    LAllocation* sourceAlloc = interval->getAllocation();

                    if (*res != *alloc) {
                        LMoveGroup* group = getInputMoveGroup(ins->toInstruction());
                        if (!group->addAfter(sourceAlloc, res, reg->type()))
                            return false;
                        *alloc = *res;
                    }
                }
            }

            addLiveRegistersForInterval(reg, interval);
        }
    }

    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());
    return true;
}

bool
BacktrackingAllocator::populateSafepoints()
{
    JitSpew(JitSpew_RegAlloc, "Populating Safepoints");

    size_t firstSafepoint = 0;

    
    MOZ_ASSERT(!vregs[0u].def());
    for (uint32_t i = 1; i < vregs.numVirtualRegisters(); i++) {
        BacktrackingVirtualRegister* reg = &vregs[i];

        if (!reg->def() || (!IsTraceable(reg) && !IsSlotsOrElements(reg) && !IsNunbox(reg)))
            continue;

        firstSafepoint = findFirstSafepoint(reg->getInterval(0), firstSafepoint);
        if (firstSafepoint >= graph.numSafepoints())
            break;

        
        
        CodePosition end = reg->getInterval(0)->end();
        for (size_t j = 1; j < reg->numIntervals(); j++)
            end = Max(end, reg->getInterval(j)->end());

        for (size_t j = firstSafepoint; j < graph.numSafepoints(); j++) {
            LInstruction* ins = graph.getSafepoint(j);

            
            
            if (end < outputOf(ins))
                break;

            
            
            
            if (ins == reg->ins() && !reg->isTemp()) {
                DebugOnly<LDefinition*> def = reg->def();
                MOZ_ASSERT_IF(def->policy() == LDefinition::MUST_REUSE_INPUT,
                              def->type() == LDefinition::GENERAL ||
                              def->type() == LDefinition::INT32 ||
                              def->type() == LDefinition::FLOAT32 ||
                              def->type() == LDefinition::DOUBLE);
                continue;
            }

            LSafepoint* safepoint = ins->safepoint();

            for (size_t k = 0; k < reg->numIntervals(); k++) {
                LiveInterval* interval = reg->getInterval(k);
                if (!interval->covers(inputOf(ins)))
                    continue;

                LAllocation* a = interval->getAllocation();
                if (a->isGeneralReg() && ins->isCall())
                    continue;

                switch (reg->type()) {
                  case LDefinition::OBJECT:
                    safepoint->addGcPointer(*a);
                    break;
                  case LDefinition::SLOTS:
                    safepoint->addSlotsOrElementsPointer(*a);
                    break;
#ifdef JS_NUNBOX32
                  case LDefinition::TYPE:
                    safepoint->addNunboxType(i, *a);
                    break;
                  case LDefinition::PAYLOAD:
                    safepoint->addNunboxPayload(i, *a);
                    break;
#else
                  case LDefinition::BOX:
                    safepoint->addBoxedValue(*a);
                    break;
#endif
                  default:
                    MOZ_CRASH("Bad register type");
                }
            }
        }
    }

    return true;
}

bool
BacktrackingAllocator::annotateMoveGroups()
{
    
    
    
    
#ifdef JS_CODEGEN_X86
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("Backtracking Annotate Move Groups"))
            return false;

        LBlock* block = graph.getBlock(i);
        LInstruction* last = nullptr;
        for (LInstructionIterator iter = block->begin(); iter != block->end(); ++iter) {
            if (iter->isMoveGroup()) {
                CodePosition from = last ? outputOf(last) : entryOf(block);
                LiveInterval::Range range(from, from.next());
                AllocatedRange search(nullptr, &range), existing;

                for (size_t i = 0; i < AnyRegister::Total; i++) {
                    PhysicalRegister& reg = registers[i];
                    if (reg.reg.isFloat() || !reg.allocatable)
                        continue;

                    
                    
                    
                    
                    
                    
                    

                    if (iter->toMoveGroup()->uses(reg.reg.gpr()))
                        continue;
                    bool found = false;
                    LInstructionIterator niter(iter);
                    for (niter++; niter != block->end(); niter++) {
                        if (niter->isMoveGroup()) {
                            if (niter->toMoveGroup()->uses(reg.reg.gpr())) {
                                found = true;
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                    if (iter != block->begin()) {
                        LInstructionIterator riter(iter);
                        do {
                            riter--;
                            if (riter->isMoveGroup()) {
                                if (riter->toMoveGroup()->uses(reg.reg.gpr())) {
                                    found = true;
                                    break;
                                }
                            } else {
                                break;
                            }
                        } while (riter != block->begin());
                    }

                    if (found || reg.allocations.contains(search, &existing))
                        continue;

                    iter->toMoveGroup()->setScratchRegister(reg.reg.gpr());
                    break;
                }
            } else {
                last = *iter;
            }
        }
    }
#endif

    return true;
}

void
BacktrackingAllocator::dumpRegisterGroups()
{
#ifdef DEBUG
    bool any = false;

    
    MOZ_ASSERT(!vregs[0u].group());
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegisterGroup* group = vregs[i].group();
        if (group && i == group->canonicalReg()) {
            if (!any) {
                fprintf(stderr, "Register groups:\n");
                any = true;
            }
            fprintf(stderr, " ");
            for (size_t j = 0; j < group->registers.length(); j++)
                fprintf(stderr, " v%u", group->registers[j]);
            fprintf(stderr, "\n");
        }
    }
    if (any)
        fprintf(stderr, "\n");
#endif
}

void
BacktrackingAllocator::dumpFixedRanges()
{
#ifdef DEBUG
    bool any = false;

    for (size_t i = 0; i < AnyRegister::Total; i++) {
        if (registers[i].allocatable && fixedIntervals[i]->numRanges() != 0) {
            if (!any) {
                fprintf(stderr, "Live ranges by physical register:\n");
                any = true;
            }
            fprintf(stderr, "  %s: %s\n", AnyRegister::FromCode(i).name(), fixedIntervals[i]->toString());
        }
    }

    if (any)
        fprintf(stderr, "\n");
#endif 
}

#ifdef DEBUG
struct BacktrackingAllocator::PrintLiveIntervalRange
{
    bool& first_;

    explicit PrintLiveIntervalRange(bool& first) : first_(first) {}

    void operator()(const AllocatedRange& item)
    {
        if (item.range == item.interval->getRange(0)) {
            if (first_)
                first_ = false;
            else
                fprintf(stderr, " /");
            if (item.interval->hasVreg())
                fprintf(stderr, " v%u[%u]", item.interval->vreg(), item.interval->index());
            fprintf(stderr, "%s", item.interval->rangesToString());
        }
    }
};
#endif

void
BacktrackingAllocator::dumpAllocations()
{
#ifdef DEBUG
    fprintf(stderr, "Allocations by virtual register:\n");

    dumpVregs();

    fprintf(stderr, "Allocations by physical register:\n");

    for (size_t i = 0; i < AnyRegister::Total; i++) {
        if (registers[i].allocatable && !registers[i].allocations.empty()) {
            fprintf(stderr, "  %s:", AnyRegister::FromCode(i).name());
            bool first = true;
            registers[i].allocations.forEach(PrintLiveIntervalRange(first));
            fprintf(stderr, "\n");
        }
    }

    fprintf(stderr, "\n");
#endif 
}

bool
BacktrackingAllocator::addLiveInterval(LiveIntervalVector& intervals, uint32_t vreg,
                                       LiveInterval* spillInterval,
                                       CodePosition from, CodePosition to)
{
    LiveInterval* interval = LiveInterval::New(alloc(), vreg, 0);
    interval->setSpillInterval(spillInterval);
    return interval->addRange(from, to) && intervals.append(interval);
}





size_t
BacktrackingAllocator::computePriority(const LiveInterval* interval)
{
    
    
    
    size_t lifetimeTotal = 0;

    for (size_t i = 0; i < interval->numRanges(); i++) {
        const LiveInterval::Range* range = interval->getRange(i);
        lifetimeTotal += range->to - range->from;
    }

    return lifetimeTotal;
}

size_t
BacktrackingAllocator::computePriority(const VirtualRegisterGroup* group)
{
    size_t priority = 0;
    for (size_t j = 0; j < group->registers.length(); j++) {
        uint32_t vreg = group->registers[j];
        priority += computePriority(vregs[vreg].getInterval(0));
    }
    return priority;
}

bool
BacktrackingAllocator::minimalDef(const LiveInterval* interval, LNode* ins)
{
    
    return (interval->end() <= minimalDefEnd(ins).next()) &&
        ((!ins->isPhi() && interval->start() == inputOf(ins)) || interval->start() == outputOf(ins));
}

bool
BacktrackingAllocator::minimalUse(const LiveInterval* interval, LNode* ins)
{
    
    return (interval->start() == inputOf(ins)) &&
        (interval->end() == outputOf(ins) || interval->end() == outputOf(ins).next());
}

bool
BacktrackingAllocator::minimalInterval(const LiveInterval* interval, bool* pfixed)
{
    if (!interval->hasVreg()) {
        *pfixed = true;
        return true;
    }

    if (interval->index() == 0) {
        VirtualRegister& reg = vregs[interval->vreg()];
        if (pfixed)
            *pfixed = reg.def()->policy() == LDefinition::FIXED && reg.def()->output()->isRegister();
        return minimalDef(interval, reg.ins());
    }

    bool fixed = false, minimal = false, multiple = false;

    for (UsePositionIterator iter = interval->usesBegin(); iter != interval->usesEnd(); iter++) {
        if (iter != interval->usesBegin())
            multiple = true;
        LUse* use = iter->use;

        switch (use->policy()) {
          case LUse::FIXED:
            if (fixed)
                return false;
            fixed = true;
            if (minimalUse(interval, insData[iter->pos]))
                minimal = true;
            break;

          case LUse::REGISTER:
            if (minimalUse(interval, insData[iter->pos]))
                minimal = true;
            break;

          default:
            break;
        }
    }

    
    
    if (multiple && fixed)
        minimal = false;

    if (pfixed)
        *pfixed = fixed;
    return minimal;
}

size_t
BacktrackingAllocator::computeSpillWeight(const LiveInterval* interval)
{
    
    
    bool fixed;
    if (minimalInterval(interval, &fixed))
        return fixed ? 2000000 : 1000000;

    size_t usesTotal = 0;

    if (interval->index() == 0) {
        VirtualRegister* reg = &vregs[interval->vreg()];
        if (reg->def()->policy() == LDefinition::FIXED && reg->def()->output()->isRegister())
            usesTotal += 2000;
        else if (!reg->ins()->isPhi())
            usesTotal += 2000;
    }

    for (UsePositionIterator iter = interval->usesBegin(); iter != interval->usesEnd(); iter++) {
        LUse* use = iter->use;

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
            
            MOZ_CRASH("Bad use");
        }
    }

    
    if (interval->hint()->kind() != Requirement::NONE)
        usesTotal += 2000;

    
    
    size_t lifetimeTotal = computePriority(interval);
    return lifetimeTotal ? usesTotal / lifetimeTotal : 0;
}

size_t
BacktrackingAllocator::computeSpillWeight(const VirtualRegisterGroup* group)
{
    size_t maxWeight = 0;
    for (size_t j = 0; j < group->registers.length(); j++) {
        uint32_t vreg = group->registers[j];
        maxWeight = Max(maxWeight, computeSpillWeight(vregs[vreg].getInterval(0)));
    }
    return maxWeight;
}

size_t
BacktrackingAllocator::maximumSpillWeight(const LiveIntervalVector& intervals)
{
    size_t maxWeight = 0;
    for (size_t i = 0; i < intervals.length(); i++)
        maxWeight = Max(maxWeight, computeSpillWeight(intervals[i]));
    return maxWeight;
}

bool
BacktrackingAllocator::trySplitAcrossHotcode(LiveInterval* interval, bool* success)
{
    
    

    const LiveInterval::Range* hotRange = nullptr;

    for (size_t i = 0; i < interval->numRanges(); i++) {
        AllocatedRange range(interval, interval->getRange(i)), existing;
        if (hotcode.contains(range, &existing)) {
            hotRange = existing.range;
            break;
        }
    }

    
    if (!hotRange) {
        JitSpew(JitSpew_RegAlloc, "  interval does not contain hot code");
        return true;
    }

    
    bool coldCode = false;
    for (size_t i = 0; i < interval->numRanges(); i++) {
        if (!hotRange->contains(interval->getRange(i))) {
            coldCode = true;
            break;
        }
    }
    if (!coldCode) {
        JitSpew(JitSpew_RegAlloc, "  interval does not contain cold code");
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  split across hot range %s", hotRange->toString());

    
    
    
    
    if (compilingAsmJS()) {
        SplitPositionVector splitPositions;
        if (!splitPositions.append(hotRange->from) || !splitPositions.append(hotRange->to))
            return false;
        *success = true;
        return splitAt(interval, splitPositions);
    }

    LiveInterval* hotInterval = LiveInterval::New(alloc(), interval->vreg(), 0);
    LiveInterval* preInterval = nullptr;
    LiveInterval* postInterval = nullptr;

    
    
    
    Vector<LiveInterval::Range, 1, SystemAllocPolicy> hotList, coldList;
    for (size_t i = 0; i < interval->numRanges(); i++) {
        LiveInterval::Range hot, coldPre, coldPost;
        interval->getRange(i)->intersect(hotRange, &coldPre, &hot, &coldPost);

        if (!hot.empty() && !hotInterval->addRange(hot.from, hot.to))
            return false;

        if (!coldPre.empty()) {
            if (!preInterval)
                preInterval = LiveInterval::New(alloc(), interval->vreg(), 0);
            if (!preInterval->addRange(coldPre.from, coldPre.to))
                return false;
        }

        if (!coldPost.empty()) {
            if (!postInterval)
                postInterval = LiveInterval::New(alloc(), interval->vreg(), 0);
            if (!postInterval->addRange(coldPost.from, coldPost.to))
                return false;
        }
    }

    MOZ_ASSERT(preInterval || postInterval);
    MOZ_ASSERT(hotInterval->numRanges());

    LiveIntervalVector newIntervals;
    if (!newIntervals.append(hotInterval))
        return false;
    if (preInterval && !newIntervals.append(preInterval))
        return false;
    if (postInterval && !newIntervals.append(postInterval))
        return false;

    distributeUses(interval, newIntervals);

    *success = true;
    return split(interval, newIntervals) && requeueIntervals(newIntervals);
}

bool
BacktrackingAllocator::trySplitAfterLastRegisterUse(LiveInterval* interval, LiveInterval* conflict, bool* success)
{
    
    
    

    CodePosition lastRegisterFrom, lastRegisterTo, lastUse;

    
    
    if (isRegisterDefinition(interval)) {
        CodePosition spillStart = minimalDefEnd(insData[interval->start()]).next();
        if (!conflict || spillStart < conflict->start()) {
            lastUse = lastRegisterFrom = interval->start();
            lastRegisterTo = spillStart;
        }
    }

    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        LUse* use = iter->use;
        LNode* ins = insData[iter->pos];

        
        MOZ_ASSERT(iter->pos >= lastUse);
        lastUse = inputOf(ins);

        if (!conflict || outputOf(ins) < conflict->start()) {
            if (isRegisterUse(use, ins,  true)) {
                lastRegisterFrom = inputOf(ins);
                lastRegisterTo = iter->pos.next();
            }
        }
    }

    
    if (!lastRegisterFrom.bits()) {
        JitSpew(JitSpew_RegAlloc, "  interval has no register uses");
        return true;
    }
    if (lastRegisterFrom == lastUse) {
        JitSpew(JitSpew_RegAlloc, "  interval's last use is a register use");
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  split after last register use at %u",
            lastRegisterTo.bits());

    SplitPositionVector splitPositions;
    if (!splitPositions.append(lastRegisterTo))
        return false;
    *success = true;
    return splitAt(interval, splitPositions);
}

bool
BacktrackingAllocator::trySplitBeforeFirstRegisterUse(LiveInterval* interval, LiveInterval* conflict, bool* success)
{
    
    
    

    if (isRegisterDefinition(interval)) {
        JitSpew(JitSpew_RegAlloc, "  interval is defined by a register");
        return true;
    }
    if (interval->index() != 0) {
        JitSpew(JitSpew_RegAlloc, "  interval is not defined in memory");
        return true;
    }

    CodePosition firstRegisterFrom;

    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        LUse* use = iter->use;
        LNode* ins = insData[iter->pos];

        if (!conflict || outputOf(ins) >= conflict->end()) {
            if (isRegisterUse(use, ins,  true)) {
                firstRegisterFrom = inputOf(ins);
                break;
            }
        }
    }

    if (!firstRegisterFrom.bits()) {
        
        JitSpew(JitSpew_RegAlloc, "  interval has no register uses");
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  split before first register use at %u",
            firstRegisterFrom.bits());

    SplitPositionVector splitPositions;
    if (!splitPositions.append(firstRegisterFrom))
        return false;
    *success = true;
    return splitAt(interval, splitPositions);
}

bool
BacktrackingAllocator::splitAtAllRegisterUses(LiveInterval* interval)
{
    
    

    LiveIntervalVector newIntervals;
    uint32_t vreg = interval->vreg();

    JitSpew(JitSpew_RegAlloc, "  split at all register uses");

    
    
    
    bool spillIntervalIsNew = false;
    LiveInterval* spillInterval = interval->spillInterval();
    if (!spillInterval) {
        spillInterval = LiveInterval::New(alloc(), vreg, 0);
        spillIntervalIsNew = true;
    }

    CodePosition spillStart = interval->start();
    if (isRegisterDefinition(interval)) {
        
        
        CodePosition from = interval->start();
        CodePosition to = minimalDefEnd(insData[from]).next();
        if (!addLiveInterval(newIntervals, vreg, spillInterval, from, to))
            return false;
        spillStart = to;
    }

    if (spillIntervalIsNew) {
        for (size_t i = 0; i < interval->numRanges(); i++) {
            const LiveInterval::Range* range = interval->getRange(i);
            CodePosition from = Max(range->from, spillStart);
            if (!spillInterval->addRange(from, range->to))
                return false;
        }
    }

    for (UsePositionIterator iter(interval->usesBegin());
         iter != interval->usesEnd();
         iter++)
    {
        LNode* ins = insData[iter->pos];
        if (iter->pos < spillStart) {
            newIntervals.back()->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
        } else if (isRegisterUse(iter->use, ins)) {
            
            
            
            CodePosition from = inputOf(ins);
            CodePosition to = iter->use->usedAtStart() ? outputOf(ins) : iter->pos.next();

            
            
            if (newIntervals.empty() ||
                newIntervals.back()->end() != to ||
                newIntervals.back()->usesBegin()->use->policy() == LUse::FIXED ||
                iter->use->policy() == LUse::FIXED)
            {
                if (!addLiveInterval(newIntervals, vreg, spillInterval, from, to))
                    return false;
            }

            newIntervals.back()->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
        } else {
            MOZ_ASSERT(spillIntervalIsNew);
            spillInterval->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
        }
    }

    if (spillIntervalIsNew && !newIntervals.append(spillInterval))
        return false;

    return split(interval, newIntervals) && requeueIntervals(newIntervals);
}


static size_t NextSplitPosition(size_t activeSplitPosition,
                                const SplitPositionVector& splitPositions,
                                CodePosition currentPos)
{
    while (activeSplitPosition < splitPositions.length() &&
           splitPositions[activeSplitPosition] <= currentPos)
    {
        ++activeSplitPosition;
    }
    return activeSplitPosition;
}


static bool SplitHere(size_t activeSplitPosition,
                      const SplitPositionVector& splitPositions,
                      CodePosition currentPos)
{
    return activeSplitPosition < splitPositions.length() &&
           currentPos >= splitPositions[activeSplitPosition];
}

bool
BacktrackingAllocator::splitAt(LiveInterval* interval,
                               const SplitPositionVector& splitPositions)
{
    
    
    

    
    MOZ_ASSERT(!splitPositions.empty());
    for (size_t i = 1; i < splitPositions.length(); ++i)
        MOZ_ASSERT(splitPositions[i-1] < splitPositions[i]);

    
    CodePosition spillStart = interval->start();
    if (isRegisterDefinition(interval))
        spillStart = minimalDefEnd(insData[interval->start()]).next();

    uint32_t vreg = interval->vreg();

    
    
    
    bool spillIntervalIsNew = false;
    LiveInterval* spillInterval = interval->spillInterval();
    if (!spillInterval) {
        spillInterval = LiveInterval::New(alloc(), vreg, 0);
        spillIntervalIsNew = true;

        for (size_t i = 0; i < interval->numRanges(); i++) {
            const LiveInterval::Range* range = interval->getRange(i);
            CodePosition from = Max(range->from, spillStart);
            if (!spillInterval->addRange(from, range->to))
                return false;
        }
    }

    LiveIntervalVector newIntervals;

    CodePosition lastRegisterUse;
    if (spillStart != interval->start()) {
        LiveInterval* newInterval = LiveInterval::New(alloc(), vreg, 0);
        newInterval->setSpillInterval(spillInterval);
        if (!newIntervals.append(newInterval))
            return false;
        lastRegisterUse = interval->start();
    }

    size_t activeSplitPosition = NextSplitPosition(0, splitPositions, interval->start());
    for (UsePositionIterator iter(interval->usesBegin()); iter != interval->usesEnd(); iter++) {
        LNode* ins = insData[iter->pos];
        if (iter->pos < spillStart) {
            newIntervals.back()->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
            activeSplitPosition = NextSplitPosition(activeSplitPosition, splitPositions, iter->pos);
        } else if (isRegisterUse(iter->use, ins)) {
            if (lastRegisterUse.bits() == 0 ||
                SplitHere(activeSplitPosition, splitPositions, iter->pos))
            {
                
                
                LiveInterval* newInterval = LiveInterval::New(alloc(), vreg, 0);
                newInterval->setSpillInterval(spillInterval);
                if (!newIntervals.append(newInterval))
                    return false;
                activeSplitPosition = NextSplitPosition(activeSplitPosition,
                                                        splitPositions,
                                                        iter->pos);
            }
            newIntervals.back()->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
            lastRegisterUse = iter->pos;
        } else {
            MOZ_ASSERT(spillIntervalIsNew);
            spillInterval->addUseAtEnd(new(alloc()) UsePosition(iter->use, iter->pos));
        }
    }

    
    size_t activeRange = interval->numRanges();
    for (size_t i = 0; i < newIntervals.length(); i++) {
        LiveInterval* newInterval = newIntervals[i];
        CodePosition start, end;
        if (i == 0 && spillStart != interval->start()) {
            start = interval->start();
            if (newInterval->usesEmpty())
                end = spillStart;
            else
                end = newInterval->usesBack()->pos.next();
        } else {
            start = inputOf(insData[newInterval->usesBegin()->pos]);
            end = newInterval->usesBack()->pos.next();
        }
        for (; activeRange > 0; --activeRange) {
            const LiveInterval::Range* range = interval->getRange(activeRange - 1);
            if (range->to <= start)
                continue;
            if (range->from >= end)
                break;
            if (!newInterval->addRange(Max(range->from, start),
                                       Min(range->to, end)))
                return false;
            if (range->to >= end)
                break;
        }
    }

    if (spillIntervalIsNew && !newIntervals.append(spillInterval))
        return false;

    return split(interval, newIntervals) && requeueIntervals(newIntervals);
}

bool
BacktrackingAllocator::splitAcrossCalls(LiveInterval* interval)
{
    
    

    
    
    
    
    SplitPositionVector callPositions;
    for (size_t i = fixedIntervalsUnion->numRanges(); i > 0; i--) {
        const LiveInterval::Range* range = fixedIntervalsUnion->getRange(i - 1);
        if (interval->covers(range->from) && interval->covers(range->from.previous())) {
            if (!callPositions.append(range->from))
                return false;
        }
    }
    MOZ_ASSERT(callPositions.length());

#ifdef DEBUG
    JitSpewStart(JitSpew_RegAlloc, "  split across calls at ");
    for (size_t i = 0; i < callPositions.length(); ++i) {
        JitSpewCont(JitSpew_RegAlloc, "%s%u", i != 0 ? ", " : "", callPositions[i].bits());
    }
    JitSpewFin(JitSpew_RegAlloc);
#endif

    return splitAt(interval, callPositions);
}

bool
BacktrackingAllocator::chooseIntervalSplit(LiveInterval* interval, bool fixed, LiveInterval* conflict)
{
    bool success = false;

    if (!trySplitAcrossHotcode(interval, &success))
        return false;
    if (success)
        return true;

    if (fixed)
        return splitAcrossCalls(interval);

    if (!trySplitBeforeFirstRegisterUse(interval, conflict, &success))
        return false;
    if (success)
        return true;

    if (!trySplitAfterLastRegisterUse(interval, conflict, &success))
        return false;
    if (success)
        return true;

    return splitAtAllRegisterUses(interval);
}
