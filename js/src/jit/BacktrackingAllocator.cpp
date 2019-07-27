





#include "jit/BacktrackingAllocator.h"
#include "jit/BitSet.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;





static inline bool
SortBefore(UsePosition* a, UsePosition* b)
{
    return a->pos <= b->pos;
}

static inline bool
SortBefore(LiveRange::BundleLink* a, LiveRange::BundleLink* b)
{
    return LiveRange::get(a)->from() <= LiveRange::get(b)->from();
}

static inline bool
SortBefore(LiveRange::RegisterLink* a, LiveRange::RegisterLink* b)
{
    return LiveRange::get(a)->from() <= LiveRange::get(b)->from();
}

template <typename T>
static inline void
InsertSortedList(InlineForwardList<T> &list, T* value)
{
    if (list.empty()) {
        list.pushFront(value);
        return;
    }

    if (SortBefore(list.back(), value)) {
        list.pushBack(value);
        return;
    }

    T* prev = nullptr;
    for (InlineForwardListIterator<T> iter = list.begin(); iter; iter++) {
        if (SortBefore(value, *iter))
            break;
        prev = *iter;
    }

    if (prev)
        list.insertAfter(prev, value);
    else
        list.pushFront(value);
}





void
LiveRange::addUse(UsePosition* use)
{
    MOZ_ASSERT(covers(use->pos));
    InsertSortedList(uses_, use);
}

void
LiveRange::distributeUses(LiveRange* other)
{
    MOZ_ASSERT(other->vreg() == vreg());
    MOZ_ASSERT(this != other);

    
    for (UsePositionIterator iter = usesBegin(); iter; ) {
        UsePosition* use = *iter;
        if (other->covers(use->pos)) {
            uses_.removeAndIncrement(iter);
            other->addUse(use);
        } else {
            iter++;
        }
    }

    
    if (hasDefinition() && from() == other->from())
        other->setHasDefinition();
}

bool
LiveRange::contains(LiveRange* other) const
{
    return from() <= other->from() && to() >= other->to();
}

void
LiveRange::intersect(LiveRange* other, Range* pre, Range* inside, Range* post) const
{
    MOZ_ASSERT(pre->empty() && inside->empty() && post->empty());

    CodePosition innerFrom = from();
    if (from() < other->from()) {
        if (to() < other->from()) {
            *pre = range_;
            return;
        }
        *pre = Range(from(), other->from());
        innerFrom = other->from();
    }

    CodePosition innerTo = to();
    if (to() > other->to()) {
        if (from() >= other->to()) {
            *post = range_;
            return;
        }
        *post = Range(other->to(), to());
        innerTo = other->to();
    }

    if (innerFrom != innerTo)
        *inside = Range(innerFrom, innerTo);
}





#ifdef DEBUG
size_t
LiveBundle::numRanges() const
{
    size_t count = 0;
    for (LiveRange::BundleLinkIterator iter = rangesBegin(); iter; iter++)
        count++;
    return count;
}
#endif 

LiveRange*
LiveBundle::rangeFor(CodePosition pos) const
{
    for (LiveRange::BundleLinkIterator iter = rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        if (range->covers(pos))
            return range;
    }
    return nullptr;
}

void
LiveBundle::addRange(LiveRange* range)
{
    MOZ_ASSERT(!range->bundle());
    range->setBundle(this);
    InsertSortedList(ranges_, &range->bundleLink);
}

bool
LiveBundle::addRange(TempAllocator& alloc, uint32_t vreg, CodePosition from, CodePosition to)
{
    LiveRange* range = LiveRange::New(alloc, vreg, from, to);
    if (!range)
        return false;
    addRange(range);
    return true;
}

bool
LiveBundle::addRangeAndDistributeUses(TempAllocator& alloc, LiveRange* oldRange,
                                      CodePosition from, CodePosition to)
{
    LiveRange* range = LiveRange::New(alloc, oldRange->vreg(), from, to);
    if (!range)
        return false;
    addRange(range);
    oldRange->distributeUses(range);
    return true;
}





bool
VirtualRegister::addInitialRange(TempAllocator& alloc, CodePosition from, CodePosition to)
{
    MOZ_ASSERT(from < to);

    
    

    LiveRange* prev = nullptr;
    LiveRange* merged = nullptr;
    for (LiveRange::RegisterLinkIterator iter(rangesBegin()); iter; ) {
        LiveRange* existing = LiveRange::get(*iter);

        if (from > existing->to()) {
            
            prev = existing;
            iter++;
            continue;
        }

        if (to.next() < existing->from()) {
            
            break;
        }

        if (!merged) {
            
            
            merged = existing;

            if (from < existing->from())
                existing->setFrom(from);
            if (to > existing->to())
                existing->setTo(to);

            
            
            iter++;
            continue;
        }

        
        MOZ_ASSERT(existing->from() >= merged->from());
        if (existing->to() > merged->to())
            merged->setTo(existing->to());

        MOZ_ASSERT(!existing->hasDefinition());
        existing->distributeUses(merged);
        MOZ_ASSERT(!existing->hasUses());

        ranges_.removeAndIncrement(iter);
    }

    if (!merged) {
        
        LiveRange* range = LiveRange::New(alloc, vreg(), from, to);
        if (!range)
            return false;

        if (prev)
            ranges_.insertAfter(&prev->registerLink, &range->registerLink);
        else
            ranges_.pushFront(&range->registerLink);
    }

    return true;
}

void
VirtualRegister::addInitialUse(UsePosition* use)
{
    LiveRange::get(*rangesBegin())->addUse(use);
}

void
VirtualRegister::setInitialDefinition(CodePosition from)
{
    LiveRange* first = LiveRange::get(*rangesBegin());
    MOZ_ASSERT(from >= first->from());
    first->setFrom(from);
    first->setHasDefinition();
}

LiveRange*
VirtualRegister::rangeFor(CodePosition pos) const
{
    for (LiveRange::RegisterLinkIterator iter = rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        if (range->covers(pos))
            return range;
    }
    return nullptr;
}

void
VirtualRegister::addRange(LiveRange* range)
{
    InsertSortedList(ranges_, &range->registerLink);
}

void
VirtualRegister::removeRange(LiveRange* range)
{
    for (LiveRange::RegisterLinkIterator iter = rangesBegin(); iter; iter++) {
        LiveRange* existing = LiveRange::get(*iter);
        if (existing == range) {
            ranges_.removeAt(iter);
            return;
        }
    }
    MOZ_CRASH();
}







bool
BacktrackingAllocator::init()
{
    if (!RegisterAllocator::init())
        return false;

    liveIn = mir->allocate<BitSet>(graph.numBlockIds());
    if (!liveIn)
        return false;

    callRanges = LiveBundle::New(alloc());

    size_t numVregs = graph.numVirtualRegisters();
    if (!vregs.init(mir->alloc(), numVregs))
        return false;
    memset(&vregs[0], 0, sizeof(VirtualRegister) * numVregs);
    for (uint32_t i = 0; i < numVregs; i++)
        new(&vregs[i]) VirtualRegister();

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("Create data structures (main loop)"))
            return false;

        LBlock* block = graph.getBlock(i);
        for (LInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            for (size_t j = 0; j < ins->numDefs(); j++) {
                LDefinition* def = ins->getDef(j);
                if (def->isBogusTemp())
                    continue;
                vreg(def).init(*ins, def,  false);
            }

            for (size_t j = 0; j < ins->numTemps(); j++) {
                LDefinition* def = ins->getTemp(j);
                if (def->isBogusTemp())
                    continue;
                vreg(def).init(*ins, def,  true);
            }
        }
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi* phi = block->getPhi(j);
            LDefinition* def = phi->getDef(0);
            vreg(def).init(phi, def,  false);
        }
    }

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
    }

    hotcode.setAllocator(lifoAlloc);

    
    
    
    

    LBlock* backedge = nullptr;
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        LBlock* block = graph.getBlock(i);

        
        
        
        if (block->mir()->isLoopHeader())
            backedge = block->mir()->backedge()->lir();

        if (block == backedge) {
            LBlock* header = block->mir()->loopHeaderOfBackedge()->lir();
            LiveRange* range = LiveRange::New(alloc(), 0, entryOf(header), exitOf(block).next());
            if (!range || !hotcode.insert(range))
                return false;
        }
    }

    return true;
}

bool
BacktrackingAllocator::addInitialFixedRange(AnyRegister reg, CodePosition from, CodePosition to)
{
    LiveRange* range = LiveRange::New(alloc(), 0, from, to);
    return range && registers[reg.code()].allocations.insert(range);
}

#ifdef DEBUG

static bool
IsInputReused(LInstruction* ins, LUse* use)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        if (ins->getDef(i)->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(ins->getDef(i)->getReusedInput())->toUse() == use)
        {
            return true;
        }
    }

    for (size_t i = 0; i < ins->numTemps(); i++) {
        if (ins->getTemp(i)->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(ins->getTemp(i)->getReusedInput())->toUse() == use)
        {
            return true;
        }
    }

    return false;
}
#endif





















bool
BacktrackingAllocator::buildLivenessInfo()
{
    JitSpew(JitSpew_RegAlloc, "Beginning liveness analysis");

    Vector<MBasicBlock*, 1, SystemAllocPolicy> loopWorkList;
    BitSet loopDone(graph.numBlockIds());
    if (!loopDone.init(alloc()))
        return false;

    for (size_t i = graph.numBlocks(); i > 0; i--) {
        if (mir->shouldCancel("Build Liveness Info (main loop)"))
            return false;

        LBlock* block = graph.getBlock(i - 1);
        MBasicBlock* mblock = block->mir();

        BitSet& live = liveIn[mblock->id()];
        new (&live) BitSet(graph.numVirtualRegisters());
        if (!live.init(alloc()))
            return false;

        
        for (size_t i = 0; i < mblock->lastIns()->numSuccessors(); i++) {
            MBasicBlock* successor = mblock->lastIns()->getSuccessor(i);
            
            if (mblock->id() < successor->id())
                live.insertAll(liveIn[successor->id()]);
        }

        
        if (mblock->successorWithPhis()) {
            LBlock* phiSuccessor = mblock->successorWithPhis()->lir();
            for (unsigned int j = 0; j < phiSuccessor->numPhis(); j++) {
                LPhi* phi = phiSuccessor->getPhi(j);
                LAllocation* use = phi->getOperand(mblock->positionInPhiSuccessor());
                uint32_t reg = use->toUse()->virtualRegister();
                live.insert(reg);
            }
        }

        
        
        for (BitSet::Iterator liveRegId(live); liveRegId; ++liveRegId) {
            if (!vregs[*liveRegId].addInitialRange(alloc(), entryOf(block), exitOf(block).next()))
                return false;
        }

        
        
        for (LInstructionReverseIterator ins = block->rbegin(); ins != block->rend(); ins++) {
            
            if (ins->isCall()) {
                for (AnyRegisterIterator iter(allRegisters_.asLiveSet()); iter.more(); iter++) {
                    bool found = false;
                    for (size_t i = 0; i < ins->numDefs(); i++) {
                        if (ins->getDef(i)->isFixed() &&
                            ins->getDef(i)->output()->aliases(LAllocation(*iter))) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        if (!addInitialFixedRange(*iter, outputOf(*ins), outputOf(*ins).next()))
                            return false;
                    }
                }
                if (!callRanges->addRange(alloc(), 0, outputOf(*ins), outputOf(*ins).next()))
                    return false;
            }
            DebugOnly<bool> hasDoubleDef = false;
            DebugOnly<bool> hasFloat32Def = false;
            for (size_t i = 0; i < ins->numDefs(); i++) {
                LDefinition* def = ins->getDef(i);
                if (def->isBogusTemp())
                    continue;
#ifdef DEBUG
                if (def->type() == LDefinition::DOUBLE)
                    hasDoubleDef = true;
                if (def->type() == LDefinition::FLOAT32)
                    hasFloat32Def = true;
#endif
                CodePosition from = outputOf(*ins);

                if (def->policy() == LDefinition::MUST_REUSE_INPUT) {
                    
                    
                    
                    
                    
                    LUse* inputUse = ins->getOperand(def->getReusedInput())->toUse();
                    MOZ_ASSERT(inputUse->policy() == LUse::REGISTER);
                    MOZ_ASSERT(inputUse->usedAtStart());
                    *inputUse = LUse(inputUse->virtualRegister(), LUse::ANY,  true);
                }

                if (!vreg(def).addInitialRange(alloc(), from, from.next()))
                    return false;
                vreg(def).setInitialDefinition(from);
                live.remove(def->virtualRegister());
            }

            for (size_t i = 0; i < ins->numTemps(); i++) {
                LDefinition* temp = ins->getTemp(i);
                if (temp->isBogusTemp())
                    continue;

                
                
                
                
                
                CodePosition from = inputOf(*ins);
                if (temp->policy() == LDefinition::FIXED) {
                    AnyRegister reg = temp->output()->toRegister();
                    for (LInstruction::InputIterator alloc(**ins); alloc.more(); alloc.next()) {
                        if (alloc->isUse()) {
                            LUse* use = alloc->toUse();
                            if (use->isFixedRegister()) {
                                if (GetFixedRegister(vreg(use).def(), use) == reg)
                                    from = outputOf(*ins);
                            }
                        }
                    }
                }

                CodePosition to =
                    ins->isCall() ? outputOf(*ins) : outputOf(*ins).next();

                if (!vreg(temp).addInitialRange(alloc(), from, to))
                    return false;
                vreg(temp).setInitialDefinition(from);
            }

            DebugOnly<bool> hasUseRegister = false;
            DebugOnly<bool> hasUseRegisterAtStart = false;

            for (LInstruction::InputIterator inputAlloc(**ins); inputAlloc.more(); inputAlloc.next()) {
                if (inputAlloc->isUse()) {
                    LUse* use = inputAlloc->toUse();

                    
                    
                    MOZ_ASSERT_IF(ins->isCall() && !inputAlloc.isSnapshotInput(),
                                  use->isFixedRegister() || use->usedAtStart());

#ifdef DEBUG
                    
                    
                    if (ins->isCall() && use->usedAtStart()) {
                        for (size_t i = 0; i < ins->numTemps(); i++)
                            MOZ_ASSERT(vreg(ins->getTemp(i)).type() != vreg(use).type());
                    }

                    
                    
                    
                    if (use->policy() == LUse::REGISTER) {
                        if (use->usedAtStart()) {
                            if (!IsInputReused(*ins, use))
                                hasUseRegisterAtStart = true;
                        } else {
                            hasUseRegister = true;
                        }
                    }
                    MOZ_ASSERT(!(hasUseRegister && hasUseRegisterAtStart));
#endif

                    
                    if (use->policy() == LUse::RECOVERED_INPUT)
                        continue;

                    
                    
                    CodePosition to =
                        (use->usedAtStart() || (ins->isCall() && use->isFixedRegister()))
                        ? inputOf(*ins)
                        : outputOf(*ins);
                    if (use->isFixedRegister()) {
                        LAllocation reg(AnyRegister::FromCode(use->registerCode()));
                        for (size_t i = 0; i < ins->numDefs(); i++) {
                            LDefinition* def = ins->getDef(i);
                            if (def->policy() == LDefinition::FIXED && *def->output() == reg)
                                to = inputOf(*ins);
                        }
                    }

                    if (!vreg(use).addInitialRange(alloc(), entryOf(block), to.next()))
                        return false;
                    UsePosition* usePosition = new(alloc()) UsePosition(use, to);
                    if (!usePosition)
                        return false;
                    vreg(use).addInitialUse(usePosition);
                    live.insert(use->virtualRegister());
                }
            }
        }

        
        
        
        for (unsigned int i = 0; i < block->numPhis(); i++) {
            LDefinition* def = block->getPhi(i)->getDef(0);
            if (live.contains(def->virtualRegister())) {
                live.remove(def->virtualRegister());
            } else {
                
                
                CodePosition entryPos = entryOf(block);
                if (!vreg(def).addInitialRange(alloc(), entryPos, entryPos.next()))
                    return false;
            }
        }

        if (mblock->isLoopHeader()) {
            
            
            
            
            
            MBasicBlock* loopBlock = mblock->backedge();
            while (true) {
                
                MOZ_ASSERT(loopBlock->id() >= mblock->id());

                
                CodePosition from = entryOf(loopBlock->lir());
                CodePosition to = exitOf(loopBlock->lir()).next();

                for (BitSet::Iterator liveRegId(live); liveRegId; ++liveRegId) {
                    if (!vregs[*liveRegId].addInitialRange(alloc(), from, to))
                        return false;
                }

                
                liveIn[loopBlock->id()].insertAll(live);

                
                loopDone.insert(loopBlock->id());

                
                
                
                if (loopBlock != mblock) {
                    for (size_t i = 0; i < loopBlock->numPredecessors(); i++) {
                        MBasicBlock* pred = loopBlock->getPredecessor(i);
                        if (loopDone.contains(pred->id()))
                            continue;
                        if (!loopWorkList.append(pred))
                            return false;
                    }
                }

                
                if (loopWorkList.empty())
                    break;

                
                MBasicBlock* osrBlock = graph.mir().osrBlock();
                while (!loopWorkList.empty()) {
                    loopBlock = loopWorkList.popCopy();
                    if (loopBlock != osrBlock)
                        break;
                }

                
                if (loopBlock == osrBlock) {
                    MOZ_ASSERT(loopWorkList.empty());
                    break;
                }
            }

            
            loopDone.clear();
        }

        MOZ_ASSERT_IF(!mblock->numPredecessors(), live.empty());
    }

    JitSpew(JitSpew_RegAlloc, "Liveness analysis complete");

    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        dumpInstructions();

        fprintf(stderr, "Live ranges by virtual register:\n");
        dumpVregs();
    }

    return true;
}

bool
BacktrackingAllocator::go()
{
    JitSpew(JitSpew_RegAlloc, "Beginning register allocation");

    if (!init())
        return false;

    if (!buildLivenessInfo())
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
        if (item.bundle ? !processBundle(item.bundle) : !processGroup(item.group))
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
LifetimesOverlapIgnoreGroupExclude(VirtualRegister* reg0, VirtualRegister* reg1)
{
    LiveRange::RegisterLinkIterator iter0 = reg0->rangesBegin(), iter1 = reg1->rangesBegin();
    while (iter0 && iter1) {
        LiveRange* range0 = LiveRange::get(*iter0);
        LiveRange* range1 = LiveRange::get(*iter1);

        
        
        if (range0 == reg0->groupExclude()) {
            iter0++;
            continue;
        } else if (range1 == reg1->groupExclude()) {
            iter1++;
            continue;
        }

        if (range0->from() >= range1->to())
            iter1++;
        else if (range1->from() >= range0->to())
            iter0++;
        else
            return true;
    }

    return false;
}

bool
BacktrackingAllocator::canAddToGroup(VirtualRegisterGroup* group, VirtualRegister* reg)
{
    for (size_t i = 0; i < group->registers.length(); i++) {
        if (LifetimesOverlapIgnoreGroupExclude(reg, &vregs[group->registers[i]]))
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
    
    
    
    VirtualRegister* reg0 = &vregs[vreg0];
    VirtualRegister* reg1 = &vregs[vreg1];

    if (!reg0->isCompatible(*reg1))
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

    if (LifetimesOverlapIgnoreGroupExclude(reg0, reg1))
        return true;

    VirtualRegisterGroup* group = new(alloc()) VirtualRegisterGroup(alloc());
    if (!group->registers.append(vreg0) || !group->registers.append(vreg1))
        return false;

    reg0->setGroup(group);
    reg1->setGroup(group);
    return true;
}

static inline LDefinition*
FindReusingDefinition(LNode* ins, LAllocation* alloc)
{
    for (size_t i = 0; i < ins->numDefs(); i++) {
        LDefinition* def = ins->getDef(i);
        if (def->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(def->getReusedInput()) == alloc)
            return def;
    }
    for (size_t i = 0; i < ins->numTemps(); i++) {
        LDefinition* def = ins->getTemp(i);
        if (def->policy() == LDefinition::MUST_REUSE_INPUT &&
            ins->getOperand(def->getReusedInput()) == alloc)
            return def;
    }
    return nullptr;
}

bool
BacktrackingAllocator::tryGroupReusedRegister(uint32_t def, uint32_t use)
{
    VirtualRegister& reg = vregs[def];
    VirtualRegister& usedReg = vregs[use];

    
    
    
    
    

    if (reg.rangeFor(inputOf(reg.ins()))) {
        MOZ_ASSERT(reg.isTemp());
        reg.setMustCopyInput();
        return true;
    }

    if (!usedReg.rangeFor(outputOf(reg.ins()))) {
        
        
        
        return tryGroupRegisters(use, def);
    }

    
    
    
    
    
    
    
    

    if (usedReg.groupExclude() ||
        (usedReg.def()->isFixed() && !usedReg.def()->output()->isRegister())) {
        reg.setMustCopyInput();
        return true;
    }
    LBlock* block = reg.ins()->block();

    
    
    LiveRange* lastUsedRange = nullptr;
    for (LiveRange::RegisterLinkIterator iter = usedReg.rangesBegin(); iter; iter++)
        lastUsedRange = LiveRange::get(*iter);
    if (lastUsedRange->to() > exitOf(block)) {
        reg.setMustCopyInput();
        return true;
    }

    for (UsePositionIterator iter = lastUsedRange->usesBegin(); iter; iter++) {
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

    LiveRange* preRange = LiveRange::New(alloc(), use, lastUsedRange->from(), outputOf(reg.ins()));
    if (!preRange)
        return false;

    
    
    
    LiveRange* postRange = LiveRange::New(alloc(), use, inputOf(reg.ins()), lastUsedRange->to());
    if (!postRange)
        return false;

    lastUsedRange->distributeUses(preRange);
    lastUsedRange->distributeUses(postRange);
    MOZ_ASSERT(!lastUsedRange->hasUses());

    JitSpew(JitSpew_RegAlloc, "  splitting reused input at %u to try to help grouping",
            inputOf(reg.ins()));

    usedReg.removeRange(lastUsedRange);
    usedReg.addRange(preRange);
    usedReg.addRange(postRange);

    usedReg.setGroupExclude(postRange);

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

    
    MOZ_ASSERT(!vregs[0u].hasRanges());
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister& reg = vregs[i];
        if (!reg.hasRanges())
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

    MOZ_ASSERT(!vregs[0u].hasRanges());
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        if (mir->shouldCancel("Backtracking Enqueue Registers"))
            return false;

        VirtualRegister& reg = vregs[i];
        MOZ_ASSERT(!reg.canonicalSpill());

        if (!reg.hasRanges())
            continue;

        
        
        LDefinition* def = reg.def();
        if (def->policy() == LDefinition::FIXED && !def->output()->isRegister()) {
            MOZ_ASSERT(!def->output()->isStackSlot());
            reg.setCanonicalSpill(*def->output());
            if (reg.group() && reg.group()->spill.isUse())
                reg.group()->spill = *def->output();
        }

        
        
        if (reg.groupExclude()) {
            LiveBundle* bundle = LiveBundle::New(alloc());
            if (!bundle)
                return false;
            bundle->addRange(reg.groupExclude());
            size_t priority = computePriority(bundle);
            if (!allocationQueue.insert(QueueItem(bundle, priority)))
                return false;
        }

        
        LiveBundle* bundle = LiveBundle::New(alloc());
        if (!bundle)
            return false;
        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            if (range != reg.groupExclude())
                bundle->addRange(range);
        }

        
        
        
        
        
        
        
        if (VirtualRegisterGroup* group = reg.group()) {
            if (i == group->canonicalReg()) {
                size_t priority = computePriority(group);
                if (!allocationQueue.insert(QueueItem(group, priority)))
                    return false;
            }
        } else {
            size_t priority = computePriority(bundle);
            if (!allocationQueue.insert(QueueItem(bundle, priority)))
                return false;
        }
    }

    return true;
}

static const size_t MAX_ATTEMPTS = 2;

bool
BacktrackingAllocator::tryAllocateFixed(LiveBundle* bundle, Requirement requirement,
                                        bool* success, bool* pfixed,
                                        LiveBundleVector& conflicting)
{
    
    if (!requirement.allocation().isRegister()) {
        JitSpew(JitSpew_RegAlloc, "  stack allocation requirement");
        bundle->setAllocation(requirement.allocation());
        *success = true;
        return true;
    }

    AnyRegister reg = requirement.allocation().toRegister();
    return tryAllocateRegister(registers[reg.code()], bundle, success, pfixed, conflicting);
}

bool
BacktrackingAllocator::tryAllocateNonFixed(LiveBundle* bundle,
                                           Requirement requirement, Requirement hint,
                                           bool* success, bool* pfixed,
                                           LiveBundleVector& conflicting)
{
    
    
    
    
    
    if (hint.kind() == Requirement::FIXED) {
        AnyRegister reg = hint.allocation().toRegister();
        if (!tryAllocateRegister(registers[reg.code()], bundle, success, pfixed, conflicting))
            return false;
        if (*success)
            return true;
    }

    
    if (requirement.kind() == Requirement::NONE && hint.kind() != Requirement::REGISTER) {
        spill(bundle);
        *success = true;
        return true;
    }

    if (conflicting.empty() || minimalBundle(bundle)) {
        
        
        for (size_t i = 0; i < AnyRegister::Total; i++) {
            if (!tryAllocateRegister(registers[i], bundle, success, pfixed, conflicting))
                return false;
            if (*success)
                return true;
        }
    }

    
    
    if (requirement.kind() == Requirement::NONE) {
        spill(bundle);
        *success = true;
        return true;
    }

    
    MOZ_ASSERT(!*success);
    return true;
}

bool
BacktrackingAllocator::processBundle(LiveBundle* bundle)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "Allocating %s [priority %lu] [weight %lu]",
                bundle->toString(), computePriority(bundle), computeSpillWeight(bundle));
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    Requirement requirement, hint;
    bool canAllocate = computeRequirement(bundle, &requirement, &hint);

    bool fixed;
    LiveBundleVector conflicting;
    for (size_t attempt = 0;; attempt++) {
        if (canAllocate) {
            bool success = false;
            fixed = false;
            conflicting.clear();

            
            if (requirement.kind() == Requirement::FIXED) {
                if (!tryAllocateFixed(bundle, requirement, &success, &fixed, conflicting))
                    return false;
            } else {
                if (!tryAllocateNonFixed(bundle, requirement, hint, &success, &fixed, conflicting))
                    return false;
            }

            
            if (success)
                return true;

            
            
            if (attempt < MAX_ATTEMPTS &&
                !fixed &&
                !conflicting.empty() &&
                maximumSpillWeight(conflicting) < computeSpillWeight(bundle))
                {
                    for (size_t i = 0; i < conflicting.length(); i++) {
                        if (!evictBundle(conflicting[i]))
                            return false;
                    }
                    continue;
                }
        }

        
        
        
        
        MOZ_ASSERT(!minimalBundle(bundle));

        LiveBundle* conflict = conflicting.empty() ? nullptr : conflicting[0];
        return chooseBundleSplit(bundle, canAllocate && fixed, conflict);
    }
}

bool
BacktrackingAllocator::processGroup(VirtualRegisterGroup* group)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "Allocating group v%u [priority %lu] [weight %lu]",
                group->registers[0], computePriority(group), computeSpillWeight(group));
    }

    LiveBundle* conflict;
    for (size_t attempt = 0;; attempt++) {
        
        conflict = nullptr;
        for (size_t i = 0; i < AnyRegister::Total; i++) {
            bool success;
            if (!tryAllocateGroupRegister(registers[i], group, &success, &conflict))
                return false;
            if (success) {
                conflict = nullptr;
                break;
            }
        }

        if (attempt < MAX_ATTEMPTS &&
            conflict &&
            computeSpillWeight(conflict) < computeSpillWeight(group))
        {
            if (!evictBundle(conflict))
                return false;
            continue;
        }

        for (size_t i = 0; i < group->registers.length(); i++) {
            VirtualRegister& reg = vregs[group->registers[i]];
            LiveRange* firstRange = LiveRange::get(*reg.rangesBegin());
            MOZ_ASSERT(firstRange != reg.groupExclude());
            if (!processBundle(firstRange->bundle()))
                return false;
        }

        return true;
    }
}

bool
BacktrackingAllocator::computeRequirement(LiveBundle* bundle,
                                          Requirement *requirement, Requirement *hint)
{
    
    
    

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        VirtualRegister &reg = vregs[range->vreg()];

        
        if (VirtualRegisterGroup* group = reg.group()) {
            if (group->allocation.isRegister()) {
                JitSpew(JitSpew_RegAlloc, "  Hint %s, used by group allocation",
                        group->allocation.toString());
                hint->merge(Requirement(group->allocation));
            }
        }

        if (range->hasDefinition()) {
            
            LDefinition::Policy policy = reg.def()->policy();
            if (policy == LDefinition::FIXED) {
                
                JitSpew(JitSpew_RegAlloc, "  Requirement %s, fixed by definition",
                        reg.def()->output()->toString());
                if (!requirement->merge(Requirement(*reg.def()->output())))
                    return false;
            } else if (reg.ins()->isPhi()) {
                
                
            } else {
                
                if (!requirement->merge(Requirement(Requirement::REGISTER)))
                    return false;
            }
        }

        
        for (UsePositionIterator iter = range->usesBegin(); iter; iter++) {
            LUse::Policy policy = iter->use->policy();
            if (policy == LUse::FIXED) {
                AnyRegister required = GetFixedRegister(reg.def(), iter->use);

                JitSpew(JitSpew_RegAlloc, "  Requirement %s, due to use at %u",
                        required.name(), iter->pos.bits());

                
                
                
                if (!requirement->merge(Requirement(LAllocation(required))))
                    return false;
            } else if (policy == LUse::REGISTER) {
                if (!requirement->merge(Requirement(Requirement::REGISTER)))
                    return false;
            } else if (policy == LUse::ANY) {
                
                hint->merge(Requirement(Requirement::REGISTER));
            }
        }
    }

    return true;
}

bool
BacktrackingAllocator::tryAllocateGroupRegister(PhysicalRegister& r, VirtualRegisterGroup* group,
                                                bool* psuccess, LiveBundle** pconflicting)
{
    *psuccess = false;

    if (!r.allocatable)
        return true;

    if (!vregs[group->registers[0]].isCompatible(r.reg))
        return true;

    bool allocatable = true;
    LiveBundle* conflicting = nullptr;

    for (size_t i = 0; i < group->registers.length(); i++) {
        VirtualRegister& reg = vregs[group->registers[i]];
        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            if (range == reg.groupExclude())
                continue;
            LiveRange* existing;
            if (r.allocations.contains(range, &existing)) {
                if (!existing->bundle()) {
                    
                    return true;
                }
                if (conflicting) {
                    if (conflicting != existing->bundle())
                        return true;
                } else {
                    conflicting = existing->bundle();
                }
                allocatable = false;
            }
        }
    }

    if (!allocatable) {
        MOZ_ASSERT(conflicting);
        if (!*pconflicting || computeSpillWeight(conflicting) < computeSpillWeight(*pconflicting))
            *pconflicting = conflicting;
        return true;
    }

    *psuccess = true;

    group->allocation = LAllocation(r.reg);
    return true;
}

bool
BacktrackingAllocator::tryAllocateRegister(PhysicalRegister& r, LiveBundle* bundle,
                                           bool* success, bool* pfixed, LiveBundleVector& conflicting)
{
    *success = false;

    if (!r.allocatable)
        return true;

    LiveBundleVector aliasedConflicting;

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        VirtualRegister &reg = vregs[range->vreg()];

        if (!reg.isCompatible(r.reg))
            return true;

        for (size_t a = 0; a < r.reg.numAliased(); a++) {
            PhysicalRegister& rAlias = registers[r.reg.aliased(a).code()];
            LiveRange* existing;
            if (!rAlias.allocations.contains(range, &existing))
                continue;
            if (existing->hasVreg()) {
                MOZ_ASSERT(existing->bundle()->allocation().toRegister() == rAlias.reg);
                bool duplicate = false;
                for (size_t i = 0; i < aliasedConflicting.length(); i++) {
                    if (aliasedConflicting[i] == existing->bundle()) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate && !aliasedConflicting.append(existing->bundle()))
                    return false;
            } else {
                JitSpew(JitSpew_RegAlloc, "  %s collides with fixed use %s",
                        rAlias.reg.name(), existing->toString());
                *pfixed = true;
                return true;
            }
        }
    }

    if (!aliasedConflicting.empty()) {
        
        
        
        

        if (JitSpewEnabled(JitSpew_RegAlloc)) {
            if (aliasedConflicting.length() == 1) {
                LiveBundle* existing = aliasedConflicting[0];
                JitSpew(JitSpew_RegAlloc, "  %s collides with %s [weight %lu]",
                        r.reg.name(), existing->toString(), computeSpillWeight(existing));
            } else {
                JitSpew(JitSpew_RegAlloc, "  %s collides with the following", r.reg.name());
                for (size_t i = 0; i < aliasedConflicting.length(); i++) {
                    LiveBundle* existing = aliasedConflicting[i];
                    JitSpew(JitSpew_RegAlloc, "      %s [weight %lu]",
                            existing->toString(), computeSpillWeight(existing));
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

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);

        
        if (VirtualRegisterGroup* group = vregs[range->vreg()].group()) {
            if (!group->allocation.isRegister())
                group->allocation = LAllocation(r.reg);
        }

        if (!r.allocations.insert(range))
            return false;
    }

    bundle->setAllocation(LAllocation(r.reg));
    *success = true;
    return true;
}

bool
BacktrackingAllocator::evictBundle(LiveBundle* bundle)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "  Evicting %s [priority %lu] [weight %lu]",
                bundle->toString(), computePriority(bundle), computeSpillWeight(bundle));
    }

    AnyRegister reg(bundle->allocation().toRegister());
    PhysicalRegister& physical = registers[reg.code()];
    MOZ_ASSERT(physical.reg == reg && physical.allocatable);

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        physical.allocations.remove(range);
    }

    bundle->setAllocation(LAllocation());

    size_t priority = computePriority(bundle);
    return allocationQueue.insert(QueueItem(bundle, priority));
}

bool
BacktrackingAllocator::splitAndRequeueBundles(LiveBundle* bundle,
                                              const LiveBundleVector& newBundles)
{
    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        JitSpew(JitSpew_RegAlloc, "    splitting bundle %s into:", bundle->toString());
        for (size_t i = 0; i < newBundles.length(); i++)
            JitSpew(JitSpew_RegAlloc, "      %s", newBundles[i]->toString());
    }

    
    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        MOZ_ASSERT(range != vregs[range->vreg()].groupExclude());
        vregs[range->vreg()].removeRange(range);
    }

    
    MOZ_ASSERT(newBundles.length() >= 2);
    for (size_t i = 0; i < newBundles.length(); i++) {
        LiveBundle* newBundle = newBundles[i];
        for (LiveRange::BundleLinkIterator iter = newBundle->rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            vregs[range->vreg()].addRange(range);
        }
    }

    
    for (size_t i = 0; i < newBundles.length(); i++) {
        LiveBundle* newBundle = newBundles[i];
        size_t priority = computePriority(newBundle);
        if (!allocationQueue.insert(QueueItem(newBundle, priority)))
            return false;
    }

    return true;
}

void
BacktrackingAllocator::spill(LiveBundle* bundle)
{
    JitSpew(JitSpew_RegAlloc, "  Spilling bundle");

    if (LiveBundle* spillParent = bundle->spillParent()) {
        JitSpew(JitSpew_RegAlloc, "    Using existing spill bundle");
        for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            LiveRange* parentRange = spillParent->rangeFor(range->from());
            MOZ_ASSERT(parentRange->from() <= range->from());
            MOZ_ASSERT(parentRange->to() >= range->to());
            MOZ_ASSERT(range->vreg() == parentRange->vreg());
            range->distributeUses(parentRange);
            MOZ_ASSERT(!range->hasUses());
            vregs[range->vreg()].removeRange(range);
        }
        return;
    }

    
    LiveRange* firstRange = LiveRange::get(*bundle->rangesBegin());
    VirtualRegister &reg = vregs[firstRange->vreg()];

#ifdef DEBUG
    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        MOZ_ASSERT(range->vreg() == reg.vreg());
        MOZ_ASSERT_IF(range == reg.groupExclude(), bundle->numRanges() == 1);
    }
#endif

    
    
    bool useCanonical = firstRange != reg.groupExclude();

    if (useCanonical) {
        if (reg.canonicalSpill()) {
            JitSpew(JitSpew_RegAlloc, "    Picked canonical spill location %s",
                    reg.canonicalSpill()->toString());
            bundle->setAllocation(*reg.canonicalSpill());
            return;
        }

        if (reg.group() && !reg.group()->spill.isUse()) {
            JitSpew(JitSpew_RegAlloc, "    Reusing group spill location %s",
                    reg.group()->spill.toString());
            bundle->setAllocation(reg.group()->spill);
            reg.setCanonicalSpill(reg.group()->spill);
            return;
        }
    }

    uint32_t virtualSlot = numVirtualStackSlots++;

    
    LStackSlot alloc(LAllocation::DATA_MASK - virtualSlot);
    bundle->setAllocation(alloc);

    JitSpew(JitSpew_RegAlloc, "    Allocating spill location %s", alloc.toString());

    if (useCanonical) {
        reg.setCanonicalSpill(alloc);
        if (reg.group())
            reg.group()->spill = alloc;
    }
}

bool
BacktrackingAllocator::pickStackSlots()
{
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister& reg = vregs[i];

        if (mir->shouldCancel("Backtracking Pick Stack Slots"))
            return false;

        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            if (!pickStackSlot(range->bundle()))
                return false;
        }
    }

    return true;
}

bool
BacktrackingAllocator::addBundlesUsingAllocation(VirtualRegister &reg, LAllocation alloc,
                                                 LiveBundleVector &bundles)
{
    for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        LiveBundle* bundle = range->bundle();
        if (bundle->allocation() == alloc) {
            bool duplicate = false;
            for (size_t i = 0; i < bundles.length(); i++) {
                if (bundles[i] == bundle) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate && !bundles.append(bundle))
                return false;
        }
    }
    return true;
}

bool
BacktrackingAllocator::pickStackSlot(LiveBundle* bundle)
{
    LAllocation alloc = bundle->allocation();
    MOZ_ASSERT(!alloc.isUse());

    if (!isVirtualStackSlot(alloc))
        return true;

    
    LiveRange* firstRange = LiveRange::get(*bundle->rangesBegin());
    VirtualRegister &reg = vregs[firstRange->vreg()];

#ifdef DEBUG
    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        MOZ_ASSERT(range->vreg() == reg.vreg());
        MOZ_ASSERT_IF(range == reg.groupExclude(), bundle->numRanges() == 1);
    }
#endif

    
    LiveBundleVector commonBundles;

    if (!commonBundles.append(bundle))
        return false;

    if (reg.canonicalSpill() && alloc == *reg.canonicalSpill()) {
        
        if (!addBundlesUsingAllocation(reg, alloc, commonBundles))
            return false;

        
        
        if (reg.group() && alloc == reg.group()->spill) {
            for (size_t i = 0; i < reg.group()->registers.length(); i++) {
                uint32_t nvreg = reg.group()->registers[i];
                if (nvreg == reg.vreg())
                    continue;
                VirtualRegister& nreg = vregs[nvreg];
                if (!addBundlesUsingAllocation(nreg, alloc, commonBundles))
                    return false;
            }
        }
    } else {
        MOZ_ASSERT_IF(reg.group(), alloc != reg.group()->spill);
    }

    if (!reuseOrAllocateStackSlot(commonBundles, reg.type(), &alloc))
        return false;

    MOZ_ASSERT(!isVirtualStackSlot(alloc));

    
    for (size_t i = 0; i < commonBundles.length(); i++)
        commonBundles[i]->setAllocation(alloc);

    return true;
}

bool
BacktrackingAllocator::reuseOrAllocateStackSlot(const LiveBundleVector& bundles,
                                                LDefinition::Type type, LAllocation* palloc)
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
            for (size_t i = 0; i < bundles.length() && success; i++) {
                LiveBundle* bundle = bundles[i];
                for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
                    LiveRange* range = LiveRange::get(*iter);
                    LiveRange* existing;
                    if (spill->allocated.contains(range, &existing)) {
                        success = false;
                        break;
                    }
                }
            }
            if (success) {
                
                
                if (!insertAllRanges(spill->allocated, bundles))
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

    if (!insertAllRanges(spill->allocated, bundles))
        return false;

    *palloc = spill->alloc;

    slotList->pushFront(spill);
    return true;
}

bool
BacktrackingAllocator::insertAllRanges(LiveRangeSet& set, const LiveBundleVector& bundles)
{
    for (size_t i = 0; i < bundles.length(); i++) {
        LiveBundle* bundle = bundles[i];
        for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
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

    
    
    MOZ_ASSERT(!vregs[0u].hasRanges());
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister& reg = vregs[i];

        if (mir->shouldCancel("Backtracking Resolve Control Flow (vreg loop)"))
            return false;

        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);

            
            
            if (range->hasDefinition())
                continue;

            
            
            CodePosition start = range->from();
            LNode* ins = insData[start];
            if (start == entryOf(ins->block()))
                continue;

            
            
            
            bool skip = false;
            for (LiveRange::RegisterLinkIterator prevIter = reg.rangesBegin();
                 prevIter != iter;
                 prevIter++)
            {
                LiveRange* prevRange = LiveRange::get(*prevIter);
                if (prevRange->covers(start) &&
                    prevRange->bundle()->allocation() == range->bundle()->allocation())
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            LiveRange* predecessorRange = reg.rangeFor(start.previous());
            if (start.subpos() == CodePosition::INPUT) {
                if (!moveInput(ins->toInstruction(), predecessorRange, range, reg.type()))
                    return false;
            } else {
                if (!moveAfter(ins->toInstruction(), predecessorRange, range, reg.type()))
                    return false;
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
            VirtualRegister& reg = vreg(def);
            LiveRange* to = reg.rangeFor(entryOf(successor));
            MOZ_ASSERT(to);

            for (size_t k = 0; k < mSuccessor->numPredecessors(); k++) {
                LBlock* predecessor = mSuccessor->getPredecessor(k)->lir();
                MOZ_ASSERT(predecessor->mir()->numSuccessors() == 1);

                LAllocation* input = phi->getOperand(k);
                LiveRange* from = vreg(input).rangeFor(exitOf(predecessor));
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

                for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
                    LiveRange* to = LiveRange::get(*iter);
                    if (!to->covers(entryOf(successor)))
                        continue;
                    if (to->covers(exitOf(predecessor)))
                        continue;

                    LiveRange* from = reg.rangeFor(exitOf(predecessor));

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
BacktrackingAllocator::isRegisterDefinition(LiveRange* range)
{
    if (!range->hasDefinition())
        return false;

    VirtualRegister& reg = vregs[range->vreg()];
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

    MOZ_ASSERT(!vregs[0u].hasRanges());
    for (size_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister& reg = vregs[i];

        if (mir->shouldCancel("Backtracking Reify Allocations (main loop)"))
            return false;

        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);

            if (range->hasDefinition()) {
                reg.def()->setOutput(range->bundle()->allocation());
                if (reg.ins()->recoversInput()) {
                    LSnapshot* snapshot = reg.ins()->toInstruction()->snapshot();
                    for (size_t i = 0; i < snapshot->numEntries(); i++) {
                        LAllocation* entry = snapshot->getEntry(i);
                        if (entry->isUse() && entry->toUse()->policy() == LUse::RECOVERED_INPUT)
                            *entry = *reg.def()->output();
                    }
                }
            }

            for (UsePositionIterator iter(range->usesBegin()); iter; iter++) {
                LAllocation* alloc = iter->use;
                *alloc = range->bundle()->allocation();

                
                
                LNode* ins = insData[iter->pos];
                if (LDefinition* def = FindReusingDefinition(ins, alloc)) {
                    LiveRange* outputRange = vreg(def).rangeFor(outputOf(ins));
                    LAllocation res = outputRange->bundle()->allocation();
                    LAllocation sourceAlloc = range->bundle()->allocation();

                    if (res != *alloc) {
                        LMoveGroup* group = getInputMoveGroup(ins->toInstruction());
                        if (!group->addAfter(sourceAlloc, res, reg.type()))
                            return false;
                        *alloc = res;
                    }
                }
            }

            addLiveRegistersForRange(reg, range);
        }
    }

    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());
    return true;
}

size_t
BacktrackingAllocator::findFirstNonCallSafepoint(CodePosition from)
{
    size_t i = 0;
    for (; i < graph.numNonCallSafepoints(); i++) {
        const LInstruction* ins = graph.getNonCallSafepoint(i);
        if (from <= inputOf(ins))
            break;
    }
    return i;
}

void
BacktrackingAllocator::addLiveRegistersForRange(VirtualRegister& reg, LiveRange* range)
{
    
    LAllocation a = range->bundle()->allocation();
    if (!a.isRegister())
        return;

    
    CodePosition start = range->from();
    if (range->hasDefinition() && !reg.isTemp()) {
#ifdef CHECK_OSIPOINT_REGISTERS
        
        
        
        if (reg.ins()->isInstruction()) {
            if (LSafepoint* safepoint = reg.ins()->toInstruction()->safepoint())
                safepoint->addClobberedRegister(a.toRegister());
        }
#endif
        start = start.next();
    }

    size_t i = findFirstNonCallSafepoint(start);
    for (; i < graph.numNonCallSafepoints(); i++) {
        LInstruction* ins = graph.getNonCallSafepoint(i);
        CodePosition pos = inputOf(ins);

        
        
        if (range->to() <= pos)
            break;

        MOZ_ASSERT(range->covers(pos));

        LSafepoint* safepoint = ins->safepoint();
        safepoint->addLiveRegister(a.toRegister());

#ifdef CHECK_OSIPOINT_REGISTERS
        if (reg.isTemp())
            safepoint->addClobberedRegister(a.toRegister());
#endif
    }
}

static inline bool
IsNunbox(VirtualRegister& reg)
{
#ifdef JS_NUNBOX32
    return reg.type() == LDefinition::TYPE ||
           reg.type() == LDefinition::PAYLOAD;
#else
    return false;
#endif
}

static inline bool
IsSlotsOrElements(VirtualRegister& reg)
{
    return reg.type() == LDefinition::SLOTS;
}

static inline bool
IsTraceable(VirtualRegister& reg)
{
    if (reg.type() == LDefinition::OBJECT)
        return true;
#ifdef JS_PUNBOX64
    if (reg.type() == LDefinition::BOX)
        return true;
#endif
    return false;
}

size_t
BacktrackingAllocator::findFirstSafepoint(CodePosition pos, size_t startFrom)
{
    size_t i = startFrom;
    for (; i < graph.numSafepoints(); i++) {
        LInstruction* ins = graph.getSafepoint(i);
        if (pos <= inputOf(ins))
            break;
    }
    return i;
}

bool
BacktrackingAllocator::populateSafepoints()
{
    JitSpew(JitSpew_RegAlloc, "Populating Safepoints");

    size_t firstSafepoint = 0;

    MOZ_ASSERT(!vregs[0u].def());
    for (uint32_t i = 1; i < graph.numVirtualRegisters(); i++) {
        VirtualRegister& reg = vregs[i];

        if (!reg.def() || (!IsTraceable(reg) && !IsSlotsOrElements(reg) && !IsNunbox(reg)))
            continue;

        firstSafepoint = findFirstSafepoint(inputOf(reg.ins()), firstSafepoint);
        if (firstSafepoint >= graph.numSafepoints())
            break;

        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);

            for (size_t j = firstSafepoint; j < graph.numSafepoints(); j++) {
                LInstruction* ins = graph.getSafepoint(j);

                if (!range->covers(inputOf(ins))) {
                    if (inputOf(ins) >= range->to())
                        break;
                    continue;
                }

                
                
                
                if (ins == reg.ins() && !reg.isTemp()) {
                    DebugOnly<LDefinition*> def = reg.def();
                    MOZ_ASSERT_IF(def->policy() == LDefinition::MUST_REUSE_INPUT,
                                  def->type() == LDefinition::GENERAL ||
                                  def->type() == LDefinition::INT32 ||
                                  def->type() == LDefinition::FLOAT32 ||
                                  def->type() == LDefinition::DOUBLE);
                    continue;
                }

                LSafepoint* safepoint = ins->safepoint();

                LAllocation a = range->bundle()->allocation();
                if (a.isGeneralReg() && ins->isCall())
                    continue;

                switch (reg.type()) {
                  case LDefinition::OBJECT:
                    safepoint->addGcPointer(a);
                    break;
                  case LDefinition::SLOTS:
                    safepoint->addSlotsOrElementsPointer(a);
                    break;
#ifdef JS_NUNBOX32
                  case LDefinition::TYPE:
                    safepoint->addNunboxType(i, a);
                    break;
                  case LDefinition::PAYLOAD:
                    safepoint->addNunboxPayload(i, a);
                    break;
#else
                  case LDefinition::BOX:
                    safepoint->addBoxedValue(a);
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
    LiveRange* range = LiveRange::New(alloc(), 0, CodePosition(), CodePosition().next());
    if (!range)
        return false;

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("Backtracking Annotate Move Groups"))
            return false;

        LBlock* block = graph.getBlock(i);
        LInstruction* last = nullptr;
        for (LInstructionIterator iter = block->begin(); iter != block->end(); ++iter) {
            if (iter->isMoveGroup()) {
                CodePosition from = last ? outputOf(last) : entryOf(block);
                range->setFrom(from);
                range->setTo(from.next());

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

                    LiveRange* existing;
                    if (found || reg.allocations.contains(range, &existing))
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

bool
BacktrackingAllocator::addLiveBundle(LiveBundleVector& bundles, uint32_t vreg,
                                     LiveBundle* spillParent,
                                     CodePosition from, CodePosition to)
{
    LiveBundle* bundle = LiveBundle::New(alloc());
    if (!bundle || !bundles.append(bundle))
        return false;
    bundle->setSpillParent(spillParent);
    return bundle->addRange(alloc(), vreg, from, to);
}





#ifdef DEBUG

const char*
LiveRange::toString() const
{
    
    static char buf[2000];

    char* cursor = buf;
    char* end = cursor + sizeof(buf);

    int n = JS_snprintf(cursor, end - cursor, "v%u [%u,%u)",
                        hasVreg() ? vreg() : 0, from().bits(), to().bits());
    if (n < 0) MOZ_CRASH();
    cursor += n;

    if (bundle() && !bundle()->allocation().isBogus()) {
        n = JS_snprintf(cursor, end - cursor, " %s", bundle()->allocation().toString());
        if (n < 0) MOZ_CRASH();
        cursor += n;
    }

    if (hasDefinition()) {
        n = JS_snprintf(cursor, end - cursor, " (def)");
        if (n < 0) MOZ_CRASH();
        cursor += n;
    }

    for (UsePositionIterator iter = usesBegin(); iter; iter++) {
        n = JS_snprintf(cursor, end - cursor, " %s@%u", iter->use->toString(), iter->pos.bits());
        if (n < 0) MOZ_CRASH();
        cursor += n;
    }

    return buf;
}

const char*
LiveBundle::toString() const
{
    
    static char buf[2000];

    char* cursor = buf;
    char* end = cursor + sizeof(buf);

    for (LiveRange::BundleLinkIterator iter = rangesBegin(); iter; iter++) {
        int n = JS_snprintf(cursor, end - cursor, "%s %s",
                            (iter == rangesBegin()) ? "" : " ##",
                            LiveRange::get(*iter)->toString());
        if (n < 0) MOZ_CRASH();
        cursor += n;
    }

    return buf;
}

#endif 

void
BacktrackingAllocator::dumpVregs()
{
#ifdef DEBUG
    MOZ_ASSERT(!vregs[0u].hasRanges());
    for (uint32_t i = 1; i < graph.numVirtualRegisters(); i++) {
        fprintf(stderr, "  ");
        VirtualRegister& reg = vregs[i];
        for (LiveRange::RegisterLinkIterator iter = reg.rangesBegin(); iter; iter++) {
            if (iter != reg.rangesBegin())
                fprintf(stderr, " / ");
            fprintf(stderr, "%s", LiveRange::get(*iter)->toString());
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "\n");
#endif
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
    fprintf(stderr, "Live ranges by physical register: %s\n", callRanges->toString());
#endif 
}

#ifdef DEBUG
struct BacktrackingAllocator::PrintLiveRange
{
    bool& first_;

    explicit PrintLiveRange(bool& first) : first_(first) {}

    void operator()(const LiveRange* range)
    {
        if (first_)
            first_ = false;
        else
            fprintf(stderr, " /");
        fprintf(stderr, " %s", range->toString());
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
            registers[i].allocations.forEach(PrintLiveRange(first));
            fprintf(stderr, "\n");
        }
    }

    fprintf(stderr, "\n");
#endif 
}





size_t
BacktrackingAllocator::computePriority(LiveBundle* bundle)
{
    
    
    
    size_t lifetimeTotal = 0;

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        lifetimeTotal += range->to() - range->from();
    }

    return lifetimeTotal;
}

size_t
BacktrackingAllocator::computePriority(const VirtualRegisterGroup* group)
{
    size_t priority = 0;
    for (size_t j = 0; j < group->registers.length(); j++) {
        uint32_t vreg = group->registers[j];
        priority += computePriority(LiveRange::get(*vregs[vreg].rangesBegin())->bundle());
    }
    return priority;
}

bool
BacktrackingAllocator::minimalDef(LiveRange* range, LNode* ins)
{
    
    return (range->to() <= minimalDefEnd(ins).next()) &&
           ((!ins->isPhi() && range->from() == inputOf(ins)) || range->from() == outputOf(ins));
}

bool
BacktrackingAllocator::minimalUse(LiveRange* range, LNode* ins)
{
    
    return (range->from() == inputOf(ins)) &&
           (range->to() == outputOf(ins) || range->to() == outputOf(ins).next());
}

bool
BacktrackingAllocator::minimalBundle(LiveBundle* bundle, bool* pfixed)
{
    LiveRange::BundleLinkIterator iter = bundle->rangesBegin();
    LiveRange* range = LiveRange::get(*iter);

    if (!range->hasVreg()) {
        *pfixed = true;
        return true;
    }

    
    
    if (++iter)
        return false;

    if (range->hasDefinition()) {
        VirtualRegister& reg = vregs[range->vreg()];
        if (pfixed)
            *pfixed = reg.def()->policy() == LDefinition::FIXED && reg.def()->output()->isRegister();
        return minimalDef(range, reg.ins());
    }

    bool fixed = false, minimal = false, multiple = false;

    for (UsePositionIterator iter = range->usesBegin(); iter; iter++) {
        if (iter != range->usesBegin())
            multiple = true;
        LUse* use = iter->use;

        switch (use->policy()) {
          case LUse::FIXED:
            if (fixed)
                return false;
            fixed = true;
            if (minimalUse(range, insData[iter->pos]))
                minimal = true;
            break;

          case LUse::REGISTER:
            if (minimalUse(range, insData[iter->pos]))
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
BacktrackingAllocator::computeSpillWeight(LiveBundle* bundle)
{
    
    
    bool fixed;
    if (minimalBundle(bundle, &fixed))
        return fixed ? 2000000 : 1000000;

    size_t usesTotal = 0;

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);

        if (range->hasDefinition()) {
            VirtualRegister& reg = vregs[range->vreg()];
            if (reg.def()->policy() == LDefinition::FIXED && reg.def()->output()->isRegister())
                usesTotal += 2000;
            else if (!reg.ins()->isPhi())
                usesTotal += 2000;
        }

        for (UsePositionIterator iter = range->usesBegin(); iter; iter++) {
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

        
        if (vregs[range->vreg()].group())
            usesTotal += 2000;
    }

    
    
    size_t lifetimeTotal = computePriority(bundle);
    return lifetimeTotal ? usesTotal / lifetimeTotal : 0;
}

size_t
BacktrackingAllocator::computeSpillWeight(const VirtualRegisterGroup* group)
{
    size_t maxWeight = 0;
    for (size_t j = 0; j < group->registers.length(); j++) {
        uint32_t vreg = group->registers[j];
        maxWeight = Max(maxWeight, computeSpillWeight(LiveRange::get(*vregs[vreg].rangesBegin())->bundle()));
    }
    return maxWeight;
}

size_t
BacktrackingAllocator::maximumSpillWeight(const LiveBundleVector& bundles)
{
    size_t maxWeight = 0;
    for (size_t i = 0; i < bundles.length(); i++)
        maxWeight = Max(maxWeight, computeSpillWeight(bundles[i]));
    return maxWeight;
}

bool
BacktrackingAllocator::trySplitAcrossHotcode(LiveBundle* bundle, bool* success)
{
    
    

    LiveRange* hotRange = nullptr;

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        if (hotcode.contains(range, &hotRange))
            break;
    }

    
    if (!hotRange) {
        JitSpew(JitSpew_RegAlloc, "  bundle does not contain hot code");
        return true;
    }

    
    bool coldCode = false;
    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        if (!hotRange->contains(range)) {
            coldCode = true;
            break;
        }
    }
    if (!coldCode) {
        JitSpew(JitSpew_RegAlloc, "  bundle does not contain cold code");
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  split across hot range %s", hotRange->toString());

    
    
    
    
    if (compilingAsmJS()) {
        SplitPositionVector splitPositions;
        if (!splitPositions.append(hotRange->from()) || !splitPositions.append(hotRange->to()))
            return false;
        *success = true;
        return splitAt(bundle, splitPositions);
    }

    LiveBundle* hotBundle = LiveBundle::New(alloc());
    if (!hotBundle)
        return false;
    LiveBundle* preBundle = nullptr;
    LiveBundle* postBundle = nullptr;

    
    
    
    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);
        LiveRange::Range hot, coldPre, coldPost;
        range->intersect(hotRange, &coldPre, &hot, &coldPost);

        if (!hot.empty()) {
            if (!hotBundle->addRangeAndDistributeUses(alloc(), range, hot.from, hot.to))
                return false;
        }

        if (!coldPre.empty()) {
            if (!preBundle) {
                preBundle = LiveBundle::New(alloc());
                if (!preBundle)
                    return false;
            }
            if (!preBundle->addRangeAndDistributeUses(alloc(), range, coldPre.from, coldPre.to))
                return false;
        }

        if (!coldPost.empty()) {
            if (!postBundle)
                postBundle = LiveBundle::New(alloc());
            if (!postBundle->addRangeAndDistributeUses(alloc(), range, coldPost.from, coldPost.to))
                return false;
        }
    }

    MOZ_ASSERT(preBundle || postBundle);
    MOZ_ASSERT(hotBundle->numRanges() != 0);

    LiveBundleVector newBundles;
    if (!newBundles.append(hotBundle))
        return false;
    if (preBundle && !newBundles.append(preBundle))
        return false;
    if (postBundle && !newBundles.append(postBundle))
        return false;

    *success = true;
    return splitAndRequeueBundles(bundle, newBundles);
}

bool
BacktrackingAllocator::trySplitAfterLastRegisterUse(LiveBundle* bundle, LiveBundle* conflict,
                                                    bool* success)
{
    
    
    

    CodePosition lastRegisterFrom, lastRegisterTo, lastUse;

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);

        
        
        if (isRegisterDefinition(range)) {
            CodePosition spillStart = minimalDefEnd(insData[range->from()]).next();
            if (!conflict || spillStart < conflict->firstRange()->from()) {
                lastUse = lastRegisterFrom = range->from();
                lastRegisterTo = spillStart;
            }
        }

        for (UsePositionIterator iter(range->usesBegin()); iter; iter++) {
            LUse* use = iter->use;
            LNode* ins = insData[iter->pos];

            
            MOZ_ASSERT(iter->pos >= lastUse);
            lastUse = inputOf(ins);

            if (!conflict || outputOf(ins) < conflict->firstRange()->from()) {
                if (isRegisterUse(use, ins,  true)) {
                    lastRegisterFrom = inputOf(ins);
                    lastRegisterTo = iter->pos.next();
                }
            }
        }
    }

    
    if (!lastRegisterFrom.bits()) {
        JitSpew(JitSpew_RegAlloc, "  bundle has no register uses");
        return true;
    }
    if (lastRegisterFrom == lastUse) {
        JitSpew(JitSpew_RegAlloc, "  bundle's last use is a register use");
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  split after last register use at %u",
            lastRegisterTo.bits());

    SplitPositionVector splitPositions;
    if (!splitPositions.append(lastRegisterTo))
        return false;
    *success = true;
    return splitAt(bundle, splitPositions);
}

bool
BacktrackingAllocator::trySplitBeforeFirstRegisterUse(LiveBundle* bundle, LiveBundle* conflict, bool* success)
{
    
    
    

    if (isRegisterDefinition(bundle->firstRange())) {
        JitSpew(JitSpew_RegAlloc, "  bundle is defined by a register");
        return true;
    }
    if (!bundle->firstRange()->hasDefinition()) {
        JitSpew(JitSpew_RegAlloc, "  bundle does not have definition");
        return true;
    }

    CodePosition firstRegisterFrom;

    CodePosition conflictEnd;
    if (conflict) {
        for (LiveRange::BundleLinkIterator iter = conflict->rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            if (range->to() > conflictEnd)
                conflictEnd = range->to();
        }
    }

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);

        for (UsePositionIterator iter(range->usesBegin()); iter; iter++) {
            LUse* use = iter->use;
            LNode* ins = insData[iter->pos];

            if (!conflict || outputOf(ins) >= conflictEnd) {
                if (isRegisterUse(use, ins,  true)) {
                    firstRegisterFrom = inputOf(ins);
                    break;
                }
            }
        }
    }

    if (!firstRegisterFrom.bits()) {
        
        JitSpew(JitSpew_RegAlloc, "  bundle has no register uses");
        return true;
    }

    JitSpew(JitSpew_RegAlloc, "  split before first register use at %u",
            firstRegisterFrom.bits());

    SplitPositionVector splitPositions;
    if (!splitPositions.append(firstRegisterFrom))
        return false;
    *success = true;
    return splitAt(bundle, splitPositions);
}

bool
BacktrackingAllocator::splitAtAllRegisterUses(LiveBundle* bundle)
{
    
    

    LiveBundleVector newBundles;

    JitSpew(JitSpew_RegAlloc, "  split at all register uses");

    
    
    
    bool spillBundleIsNew = false;
    LiveBundle* spillBundle = bundle->spillParent();
    if (!spillBundle) {
        spillBundle = LiveBundle::New(alloc());
        spillBundleIsNew = true;
    }

    CodePosition spillStart = bundle->firstRange()->from();
    if (isRegisterDefinition(bundle->firstRange())) {
        
        
        CodePosition from = spillStart;
        CodePosition to = minimalDefEnd(insData[from]).next();
        if (!addLiveBundle(newBundles, bundle->firstRange()->vreg(), spillBundle, from, to))
            return false;
        bundle->firstRange()->distributeUses(newBundles.back()->firstRange());
        spillStart = to;
    }

    if (spillBundleIsNew) {
        for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            CodePosition from = Max(range->from(), spillStart);
            if (!spillBundle->addRange(alloc(), range->vreg(), from, range->to()))
                return false;
        }
    }

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);

        LiveRange* lastNewRange = nullptr;
        while (range->hasUses()) {
            UsePosition* use = range->popUse();
            LNode* ins = insData[use->pos];

            
            
            MOZ_ASSERT(use->pos >= spillStart);

            if (isRegisterUse(use->use, ins)) {
                
                
                
                CodePosition from = inputOf(ins);
                CodePosition to = use->use->usedAtStart() ? outputOf(ins) : use->pos.next();

                
                
                if (!lastNewRange ||
                    lastNewRange->to() != to ||
                    lastNewRange->usesBegin()->use->policy() == LUse::FIXED ||
                    use->use->policy() == LUse::FIXED)
                {
                    if (!addLiveBundle(newBundles, range->vreg(), spillBundle, from, to))
                        return false;
                    lastNewRange = newBundles.back()->firstRange();
                }

                lastNewRange->addUse(use);
            } else {
                MOZ_ASSERT(spillBundleIsNew);
                spillBundle->rangeFor(use->pos)->addUse(use);
            }
        }
    }

    if (spillBundleIsNew && !newBundles.append(spillBundle))
        return false;

    return splitAndRequeueBundles(bundle, newBundles);
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
BacktrackingAllocator::splitAt(LiveBundle* bundle, const SplitPositionVector& splitPositions)
{
    
    
    

    
    MOZ_ASSERT(!splitPositions.empty());
    for (size_t i = 1; i < splitPositions.length(); ++i)
        MOZ_ASSERT(splitPositions[i-1] < splitPositions[i]);

    
    CodePosition start = bundle->firstRange()->from();
    CodePosition spillStart = start;
    if (isRegisterDefinition(bundle->firstRange()))
        spillStart = minimalDefEnd(insData[spillStart]).next();

    
    
    bool spillBundleIsNew = false;
    LiveBundle* spillBundle = bundle->spillParent();
    if (!spillBundle) {
        spillBundle = LiveBundle::New(alloc());
        if (!spillBundle)
            return false;
        spillBundleIsNew = true;

        for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
            LiveRange* range = LiveRange::get(*iter);
            CodePosition from = Max(range->from(), spillStart);
            if (!spillBundle->addRange(alloc(), range->vreg(), from, range->to()))
                return false;
        }

        if (bundle->firstRange()->hasDefinition() && !isRegisterDefinition(bundle->firstRange())) {
            MOZ_ASSERT(spillStart == start);
            spillBundle->firstRange()->setHasDefinition();
        }
    }

    LiveBundleVector newBundles;

    
    CodePosition lastRegisterUse;

    
    LiveRange* activeRange = nullptr;

    if (spillStart != start) {
        if (!addLiveBundle(newBundles, bundle->firstRange()->vreg(), spillBundle, start, spillStart))
            return false;
        bundle->firstRange()->distributeUses(newBundles.back()->firstRange());
        activeRange = newBundles.back()->firstRange();
        lastRegisterUse = start.previous();
    }

    size_t activeSplitPosition = NextSplitPosition(0, splitPositions, start);

    
    
    LiveRangeVector originalRanges;

    for (LiveRange::BundleLinkIterator iter = bundle->rangesBegin(); iter; iter++) {
        LiveRange* range = LiveRange::get(*iter);

        while (range->hasUses()) {
            UsePosition* use = range->popUse();
            LNode* ins = insData[use->pos];

            
            
            MOZ_ASSERT(use->pos >= spillStart);

            if (isRegisterUse(use->use, ins)) {
                
                
                if (lastRegisterUse.bits() == 0 ||
                    SplitHere(activeSplitPosition, splitPositions, use->pos))
                {
                    if (!addLiveBundle(newBundles, range->vreg(), spillBundle, inputOf(ins), use->pos.next()))
                        return false;
                    activeSplitPosition = NextSplitPosition(activeSplitPosition,
                                                            splitPositions,
                                                            use->pos);
                    activeRange = newBundles.back()->firstRange();
                } else {
                    if (!originalRanges.empty()) {
                        activeRange->setTo(originalRanges[0]->to());
                        for (size_t i = 1; i < originalRanges.length(); i++) {
                            LiveRange* orange = originalRanges[i];
                            if (!newBundles.back()->addRange(alloc(), orange->vreg(), orange->from(), orange->to()))
                                return false;
                        }
                        activeRange = LiveRange::New(alloc(), range->vreg(), range->from(), use->pos.next());
                        if (!activeRange)
                            return false;
                        newBundles.back()->addRange(activeRange);
                    } else {
                        activeRange->setTo(use->pos.next());
                    }
                    MOZ_ASSERT(range->vreg() == activeRange->vreg());
                }
                activeRange->addUse(use);
                originalRanges.clear();
                lastRegisterUse = use->pos;
            } else {
                MOZ_ASSERT(spillBundleIsNew);
                spillBundle->rangeFor(use->pos)->addUse(use);
            }
        }

        if (!originalRanges.append(range))
            return false;
    }

    if (spillBundleIsNew && !newBundles.append(spillBundle))
        return false;

    return splitAndRequeueBundles(bundle, newBundles);
}

bool
BacktrackingAllocator::splitAcrossCalls(LiveBundle* bundle)
{
    
    

    
    SplitPositionVector callPositions;
    for (LiveRange::BundleLinkIterator iter = callRanges->rangesBegin(); iter; iter++) {
        LiveRange* callRange = LiveRange::get(*iter);
        if (bundle->rangeFor(callRange->from()) && bundle->rangeFor(callRange->from().previous())) {
            if (!callPositions.append(callRange->from()))
                return false;
        }
    }
    MOZ_ASSERT(callPositions.length());

#ifdef DEBUG
    JitSpewStart(JitSpew_RegAlloc, "  split across calls at ");
    for (size_t i = 0; i < callPositions.length(); ++i)
        JitSpewCont(JitSpew_RegAlloc, "%s%u", i != 0 ? ", " : "", callPositions[i].bits());
    JitSpewFin(JitSpew_RegAlloc);
#endif

    return splitAt(bundle, callPositions);
}

bool
BacktrackingAllocator::chooseBundleSplit(LiveBundle* bundle, bool fixed, LiveBundle* conflict)
{
    bool success = false;

    if (!trySplitAcrossHotcode(bundle, &success))
        return false;
    if (success)
        return true;

    if (fixed)
        return splitAcrossCalls(bundle);

    if (!trySplitBeforeFirstRegisterUse(bundle, conflict, &success))
        return false;
    if (success)
        return true;

    if (!trySplitAfterLastRegisterUse(bundle, conflict, &success))
        return false;
    if (success)
        return true;

    return splitAtAllRegisterUses(bundle);
}
