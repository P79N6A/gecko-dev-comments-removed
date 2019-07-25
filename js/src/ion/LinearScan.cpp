








































#include <limits.h>
#include "BitSet.h"
#include "LinearScan.h"
#include "IonSpewer.h"
#include "LIR-inl.h"

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
        if (alloc.isGeneralReg())
            return uint32(alloc.toGeneralReg()->reg().code()) == use->registerCode();
        if (alloc.isFloatReg())
            return uint32(alloc.toFloatReg()->reg().code()) == use->registerCode();
        return false;
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
        return alloc == *ins->getOperand(def->getReusedInput());
      case LDefinition::PASSTHROUGH:
        return true;
      default:
        JS_NOT_REACHED("Unknown definition policy");
    }
    return false;
}
#endif

int
Requirement::priority() const
{
    switch (kind_) {
      case Requirement::FIXED:
        return 0;

      case Requirement::REGISTER:
        return 1;

      case Requirement::NONE:
        return 2;

      default:
        JS_NOT_REACHED("Unknown requirement kind.");
        return -1;
    }
}

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

bool
LiveInterval::covers(CodePosition pos)
{
    for (size_t i = 0; i < ranges_.length(); i++) {
        if (ranges_[i].to <= pos)
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
        if (ranges_[i].to <= pos) {
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












bool
LiveInterval::splitFrom(CodePosition pos, LiveInterval *after)
{
    JS_ASSERT(pos >= start() && pos < end());
    JS_ASSERT(after->ranges_.empty());

    
    size_t bufferLength = ranges_.length();
    Range *buffer = ranges_.extractRawBuffer();
    if (!buffer)
        return false;
    after->ranges_.replaceRawBuffer(buffer, bufferLength);

    
    for (Range *i = &after->ranges_.back(); i >= after->ranges_.begin(); i--) {
        if (pos >= i->to)
            continue;

        if (pos > i->from) {
            
            Range split(i->from, pos);
            i->from = pos;
            if (!ranges_.append(split))
                return false;
        }
        if (!ranges_.append(i + 1, after->ranges_.end()))
            return false;
        after->ranges_.shrinkBy(after->ranges_.end() - i - 1);
        break;
    }

    
    UsePosition *prev = NULL;
    for (UsePositionIterator usePos(usesBegin()); usePos != usesEnd(); usePos++) {
        if (usePos->pos > pos)
            break;
        prev = *usePos;
    }

    uses_.splitAfter(prev, &after->uses_);
    return true;
}

LiveInterval *
VirtualRegister::intervalFor(CodePosition pos)
{
    for (LiveInterval **i = intervals_.begin(); i != intervals_.end(); i++) {
        if ((*i)->covers(pos))
            return *i;
        if (pos < (*i)->end())
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

void
LiveInterval::addUse(UsePosition *use)
{
    
    
    
    
    UsePosition *prev = NULL;
    for (UsePositionIterator current(usesBegin()); current != usesEnd(); current++) {
        if (current->pos >= use->pos)
            break;
        prev = *current;
    }

    if (prev)
        uses_.insertAfter(prev, use);
    else
        uses_.pushFront(use);
}

UsePosition *
LiveInterval::nextUseAfter(CodePosition after)
{
    for (UsePositionIterator usePos(usesBegin()); usePos != usesEnd(); usePos++) {
        if (usePos->pos >= after && usePos->use->policy() != LUse::KEEPALIVE)
            return *usePos;
    }
    return NULL;
}







CodePosition
LiveInterval::nextUsePosAfter(CodePosition after)
{
    UsePosition *min = nextUseAfter(after);
    return min ? min->pos : CodePosition::MAX;
}







CodePosition
LiveInterval::firstIncompatibleUse(LAllocation alloc)
{
    for (UsePositionIterator usePos(usesBegin()); usePos != usesEnd(); usePos++) {
        if (!UseCompatibleWith(usePos->use, alloc))
            return usePos->pos;
    }
    return CodePosition::MAX;
}

const CodePosition CodePosition::MAX(UINT_MAX);
const CodePosition CodePosition::MIN(0);





bool
LinearScanAllocator::createDataStructures()
{
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
                LDefinition *def = ins->getDef(j);
                if (def->policy() != LDefinition::PASSTHROUGH) {
                    foundRealDef = true;
                    uint32 reg = def->virtualRegister();
                    if (!vregs[reg].init(reg, block, *ins, def,  false))
                        return false;
                }
            }
            if (!foundRealDef) {
                if (!vregs[*ins].init(ins->id(), block, *ins, NULL,  false))
                    return false;
            }
            for (size_t j = 0; j < ins->numTemps(); j++) {
                LDefinition *def = ins->getTemp(j);
                if (def->isBogusTemp())
                    continue;
                if (!vregs[def].init(def->virtualRegister(), block, *ins, def,  true))
                    return false;
            }
        }
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi *phi = block->getPhi(j);
            LDefinition *def = phi->getDef(0);
            if (!vregs[def].init(phi->id(), block, phi, def,  false))
                return false;
        }
    }

    return true;
}





void
LinearScanAllocator::addSpillInterval(LInstruction *ins, const Requirement &req)
{
    LiveInterval *bogus = new LiveInterval(NULL, 0);

    bogus->addRange(inputOf(ins), outputOf(ins));
    bogus->setRequirement(req);

    unhandled.enqueueAtHead(bogus);
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
                                               outputOf(block->lastId()).next());
        }

        
        
        for (LInstructionReverseIterator ins = block->rbegin(); ins != block->rend(); ins++) {
            
            if (ins->isCall()) {
                for (AnyRegisterIterator iter(RegisterSet::All()); iter.more(); iter++)
                    addSpillInterval(*ins, Requirement(LAllocation(*iter)));
            }

            for (size_t i = 0; i < ins->numDefs(); i++) {
                if (ins->getDef(i)->policy() != LDefinition::PASSTHROUGH) {
                    LDefinition *def = ins->getDef(i);
                    CodePosition from(inputOf(*ins));

                    
                    
                    if (vregs[def].getInterval(0)->numRanges() == 0)
                        vregs[def].getInterval(0)->addRange(from, outputOf(*ins));
                    vregs[def].getInterval(0)->setFrom(from);
                    live->remove(def->virtualRegister());
                }
            }

            for (size_t i = 0; i < ins->numTemps(); i++) {
                LDefinition *temp = ins->getTemp(i);
                if (temp->isBogusTemp())
                    continue;
                vregs[temp].getInterval(0)->addRange(inputOf(*ins), outputOf(*ins));
            }

            for (LInstruction::InputIterator alloc(**ins); alloc.more(); alloc.next()) {
                if (alloc->isUse()) {
                    LUse *use = alloc->toUse();

                    
                    JS_ASSERT(inputOf(*ins) > outputOf(block->firstId()));

                    
                    
                    JS_ASSERT_IF(ins->isCall() && !alloc.isSnapshotInput(), use->usedAtStart());

                    CodePosition endPos = use->usedAtStart() ? inputOf(*ins) : outputOf(*ins);
                    LiveInterval *interval = vregs[use].getInterval(0);
                    interval->addRange(inputOf(block->firstId()), endPos);
                    interval->addUse(new UsePosition(use, endPos));

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
                                                    outputOf(block->firstId()));
            }
        }

        if (mblock->isLoopHeader()) {
            
            
            
            
            
            MBasicBlock *loopBlock = mblock->backedge();
            while (true) {
                
                JS_ASSERT(loopBlock->id() >= mblock->id());

                
                for (BitSet::Iterator i(live->begin()); i != live->end(); i++) {
                    vregs[*i].getInterval(0)->addRange(inputOf(loopBlock->lir()->firstId()),
                                                       outputOf(loopBlock->lir()->lastId()).next());
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

                
                do {
                    loopBlock = loopWorkList.popCopy();
                } while (loopBlock->lir() == graph.osrBlock());
            }

            
            loopDone->clear();
        }

        JS_ASSERT_IF(!mblock->numPredecessors(), live->empty());
    }

    return true;
}





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

    
    while ((current = unhandled.dequeue()) != NULL) {
        JS_ASSERT(current->getAllocation()->isUse());
        JS_ASSERT(current->numRanges() > 0);

        CodePosition position = current->start();
        Requirement *req = current->requirement();
        Requirement *hint = current->hint();

        IonSpew(IonSpew_RegAlloc, "Processing %d = [%u, %u] (pri=%d)",
                current->reg() ? current->reg()->reg() : 0, current->start().pos(),
                current->end().pos(), current->requirement()->priority());

        
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

        
        validateIntervals();

        
        if (req->kind() == Requirement::FIXED) {
            if (!assign(req->allocation()))
                return false;
            continue;
        }

        
        if (req->kind() != Requirement::REGISTER && hint->kind() == Requirement::NONE) {
            
            IonSpew(IonSpew_RegAlloc, "  Eagerly spilling virtual register %d",
                    current->reg() ? current->reg()->reg() : 0);
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

        IonSpew(IonSpew_RegAlloc, "  Unable to allocate free register");

        
        if (!current->index() && current->reg() && current->reg()->ins()->isPhi()) {
            IonSpew(IonSpew_RegAlloc, " Can't split at phi, spilling this interval");
            if (!spill())
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

            if (vregs[phi].mustSpillAtDefinition() && !to->isSpill()) {
                
                LMoveGroup *moves = successor->getEntryMoveGroup();
                if (!moves->add(to->getAllocation(), to->reg()->canonicalSpill()))
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
                    if (!moveAfter(outputOf(successor->firstId()), from, to))
                        return false;
                }
            }
        }
    }

    return true;
}

bool
LinearScanAllocator::moveBeforeAlloc(CodePosition pos, LAllocation *from, LAllocation *to)
{
    LMoveGroup *moves = getMoveGroupBefore(pos);
    if (*from == *to)
        return true;
    return moves->add(from, to);
}






bool
LinearScanAllocator::reifyAllocations()
{
    
    for (size_t j = 1; j < graph.numVirtualRegisters(); j++) {
        VirtualRegister *reg = &vregs[j];
    for (size_t k = 0; k < reg->numIntervals(); k++) {
        LiveInterval *interval = reg->getInterval(k);
        JS_ASSERT(reg == interval->reg());
        if (!interval->numRanges())
            continue;

        UsePositionIterator usePos(interval->usesBegin());
        for (; usePos != interval->usesEnd(); usePos++) {
            JS_ASSERT(UseCompatibleWith(usePos->use, *interval->getAllocation()));
            *static_cast<LAllocation *>(usePos->use) = *interval->getAllocation();
        }

        
        if (interval->index() == 0)
        {
            LDefinition *def = reg->def();
            if (def->policy() == LDefinition::MUST_REUSE_INPUT) {
                LAllocation *alloc = reg->ins()->getOperand(def->getReusedInput());
                LAllocation *origAlloc = LAllocation::New(*alloc);

                *alloc = *interval->getAllocation();
                if (!moveBeforeAlloc(inputOf(reg->ins()), origAlloc, alloc))
                    return false;
            }

            JS_ASSERT(DefinitionCompatibleWith(reg->ins(), def, *interval->getAllocation()));
            def->setOutput(*interval->getAllocation());

            if (reg->mustSpillAtDefinition() && !reg->ins()->isPhi() &&
                (*reg->canonicalSpill() != *interval->getAllocation()))
            {
                
                
                
                
                LMoveGroup *moves = getMoveGroupAfter(outputOf(reg->ins()));
                if (!moves->add(interval->getAllocation(), reg->canonicalSpill()))
                    return false;
            }
        }
        else if (interval->start() > inputOf(vregs[interval->start()].block()->firstId()) &&
                 (!reg->canonicalSpill() ||
                  (reg->canonicalSpill() == interval->getAllocation() &&
                   !reg->mustSpillAtDefinition()) ||
                  *reg->canonicalSpill() != *interval->getAllocation()))
        {
            
            
            
            
            
            
            
            
            
            CodePosition start = interval->start();
            VirtualRegister *vreg = &vregs[start];

            if (start.subpos() == CodePosition::INPUT || start <= inputOf(vreg->ins())) {
                if (!moveBefore(start, reg->getInterval(interval->index() - 1), interval))
                    return false;
            } else {
                if (!moveAfter(start, reg->getInterval(interval->index() - 1), interval))
                    return false;
            }

            
            if (reg->canonicalSpill() == interval->getAllocation() &&
                !reg->mustSpillAtDefinition())
            {
                reg->setSpillPosition(interval->start());
            }
        }

        
        LAllocation *a = interval->getAllocation();
        if (a->isRegister()) {
            for (size_t i = 0; i < graph.numNonCallSafepoints(); i++) {
                LInstruction *ins = graph.getNonCallSafepoint(i);
                CodePosition pos = inputOf(ins);

                
                
                if (interval->end() < pos)
                    break;

                if (!interval->covers(pos)) {
                    if (pos < interval->start())
                        break;
                    continue;
                }

                LSafepoint *safepoint = ins->safepoint();
                safepoint->addLiveRegister(a->toRegister());
            }
        }
    }} 

    
    graph.setLocalSlotCount(stackSlotAllocator.stackHeight());

    return true;
}

static inline bool
IsNunbox(VirtualRegister *vreg)
{
#ifdef JS_NUNBOX32
    return (vreg->type() == LDefinition::TYPE ||
            vreg->type() == LDefinition::PAYLOAD);
#else
    return false;
#endif
}

static inline bool
IsTraceable(VirtualRegister *reg)
{
    if (reg->type() == LDefinition::OBJECT)
        return true;
#ifdef JS_PUNBOX64
    if (reg->type() == LDefinition::BOX)
        return true;
#endif
    return false;
}


size_t
LinearScanAllocator::findFirstSafepoint(LiveInterval *interval, size_t startFrom)
{
    size_t i = startFrom;
    for (; i < graph.numSafepoints(); i++) {
        LInstruction *ins = graph.getSafepoint(i);
        if (interval->start() <= inputOf(ins))
            break;
    }
    return i;
}

static inline bool
IsSpilledAt(VirtualRegister *reg, CodePosition pos)
{
    if (!reg->canonicalSpill() || !reg->canonicalSpill()->isStackSlot())
        return false;

    return reg->spillPosition() <= pos;
}

bool
LinearScanAllocator::populateSafepoints()
{
    size_t firstSafepoint = 0;

    for (uint32 i = 0; i < vregs.numVirtualRegisters(); i++) {
        VirtualRegister *reg = &vregs[i];

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
                if (a->isGeneralReg() && !ins->isCall())
                    safepoint->addGcRegister(a->toGeneralReg()->reg());

                if (IsSpilledAt(reg, inputOf(ins))) {
                    if (!safepoint->addGcSlot(reg->canonicalSpillSlot()))
                        return false;
                }
#ifdef JS_NUNBOX32
            } else {
                VirtualRegister *other = otherHalfOfNunbox(reg);
                VirtualRegister *type = (reg->type() == LDefinition::TYPE) ? reg : other;
                VirtualRegister *payload = (reg->type() == LDefinition::PAYLOAD) ? reg : other;
                LiveInterval *typeInterval = type->intervalFor(inputOf(ins));
                LiveInterval *payloadInterval = payload->intervalFor(inputOf(ins));

                if (!typeInterval && !payloadInterval)
                    continue;
                JS_ASSERT(typeInterval && payloadInterval);

                if (IsSpilledAt(type, inputOf(ins)) && IsSpilledAt(payload, inputOf(ins))) {
                    
                    
                    uint32 payloadSlot = payload->canonicalSpillSlot();
                    uint32 slot = BaseOfNunboxSlot(LDefinition::PAYLOAD, payloadSlot);
                    if (!safepoint->addValueSlot(slot))
                        return false;
                }

                LAllocation *typeAlloc = typeInterval->getAllocation();
                LAllocation *payloadAlloc = payloadInterval->getAllocation();

                if (!ins->isCall() &&
                    (!IsSpilledAt(type, inputOf(ins)) || payloadAlloc->isGeneralReg()))
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

    VirtualRegister *reg = interval->reg();

    
    JS_ASSERT(reg);

    
    LiveInterval *newInterval = new LiveInterval(reg, interval->index() + 1);
    if (!interval->splitFrom(pos, newInterval))
        return false;

    JS_ASSERT(interval->numRanges() > 0);
    JS_ASSERT(newInterval->numRanges() > 0);

    if (!reg->addInterval(newInterval))
        return false;

    IonSpew(IonSpew_RegAlloc, "  Split interval to %u = [%u, %u]/[%u, %u]",
            interval->reg()->reg(), interval->start().pos(),
            interval->end().pos(), newInterval->start().pos(),
            newInterval->end().pos());

    
    
    
    setIntervalRequirement(newInterval);

    
    
    
    unhandled.enqueueBackward(newInterval);

    return true;
}





bool
LinearScanAllocator::assign(LAllocation allocation)
{
    if (allocation.isRegister())
        IonSpew(IonSpew_RegAlloc, "Assigning register %s", allocation.toRegister().name());
    current->setAllocation(allocation);

    
    VirtualRegister *reg = current->reg();
    if (reg) {
        CodePosition splitPos = current->firstIncompatibleUse(allocation);
        if (splitPos != CodePosition::MAX) {
            
            
            
            splitPos = splitPos.previous();

            JS_ASSERT (splitPos < current->end());
            if (!splitInterval(current, splitPos))
                return false;
        }
    }

    if (allocation.isRegister()) {
        
        for (IntervalIterator i(active.begin()); i != active.end(); i++) {
            if (i->getAllocation()->isRegister() && *i->getAllocation() == allocation) {
                
                
                if (i->isSpillInterval() && !i->requireSpill(current))
                    continue;
                if (current->isSpillInterval() && !current->requireSpill(*i))
                    continue;

                IonSpew(IonSpew_RegAlloc, " Splitting active interval %u = [%u, %u]",
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
            if (i->getAllocation()->isRegister() && *i->getAllocation() == allocation) {
                IonSpew(IonSpew_RegAlloc, " Splitting inactive interval %u = [%u, %u]",
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
    } else if (reg && allocation.isMemory()) {
        if (reg->canonicalSpill()) {
            JS_ASSERT(allocation == *current->reg()->canonicalSpill());

            
            
            reg->setSpillAtDefinition(outputOf(reg->ins()));
        } else {
            reg->setCanonicalSpill(current->getAllocation());

            
            
            VirtualRegister *other = &vregs[current->start()];
            uint32 loopDepthAtDef = reg->block()->mir()->loopDepth();
            uint32 loopDepthAtSpill = other->block()->mir()->loopDepth();
            if (loopDepthAtSpill > loopDepthAtDef)
                reg->setSpillAtDefinition(outputOf(reg->ins()));
        }
    }

    active.pushBack(current);

    return true;
}

#ifdef JS_NUNBOX32
VirtualRegister *
LinearScanAllocator::otherHalfOfNunbox(VirtualRegister *vreg)
{
    signed offset = OffsetToOtherHalfOfNunbox(vreg->type());
    VirtualRegister *other = &vregs[vreg->def()->virtualRegister() + offset];
    AssertTypesFormANunbox(vreg->type(), other->type());
    return other;
}
#endif


uint32
LinearScanAllocator::allocateSlotFor(const LiveInterval *interval)
{
    SlotList *freed;
    if (interval->reg()->type() == LDefinition::DOUBLE || IsNunbox(interval->reg()))
        freed = &finishedDoubleSlots_;
    else
        freed = &finishedSlots_;

    if (!freed->empty()) {
        LiveInterval *maybeDead = freed->back();
        if (maybeDead->end() <= interval->reg()->getInterval(0)->start()) {
            
            
            
            
            
            freed->popBack();
            VirtualRegister *dead = maybeDead->reg();
#ifdef JS_NUNBOX32
            if (IsNunbox(dead))
                return BaseOfNunboxSlot(dead->type(), dead->canonicalSpillSlot());
#endif
            return dead->canonicalSpillSlot();
        }
    }

    if (IsNunbox(interval->reg()))
        return stackSlotAllocator.allocateValueSlot();
    if (interval->reg()->isDouble())
        return stackSlotAllocator.allocateDoubleSlot();
    return stackSlotAllocator.allocateSlot();
}

bool
LinearScanAllocator::spill()
{
    IonSpew(IonSpew_RegAlloc, "  Decided to spill current interval");

    
    JS_ASSERT(current->reg());

    if (current->reg()->canonicalSpill()) {
        IonSpew(IonSpew_RegAlloc, "  Allocating canonical spill location");

        return assign(*current->reg()->canonicalSpill());
    }

    uint32 stackSlot;
#if defined JS_NUNBOX32
    if (IsNunbox(current->reg())) {
        VirtualRegister *other = otherHalfOfNunbox(current->reg());

        if (other->canonicalSpill()) {
            
            
            
            JS_ASSERT(other->canonicalSpill()->isStackSlot());
            stackSlot = BaseOfNunboxSlot(other->type(), other->canonicalSpillSlot());
        } else {
            
            
            stackSlot = allocateSlotFor(current);
        }
        stackSlot -= OffsetOfNunboxSlot(current->reg()->type());
    } else
#endif
    {
        stackSlot = allocateSlotFor(current);
    }
    JS_ASSERT(stackSlot <= stackSlotAllocator.stackHeight());

    return assign(LStackSlot(stackSlot, current->reg()->isDouble()));
}

void
LinearScanAllocator::freeAllocation(LiveInterval *interval, LAllocation *alloc)
{
    VirtualRegister *mine = interval->reg();
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
    
    
    VirtualRegister *other = otherHalfOfNunbox(mine);
    if (other->finished()) {
        if (!mine->canonicalSpill() && !other->canonicalSpill())
            return;

        JS_ASSERT_IF(mine->canonicalSpill() && other->canonicalSpill(),
                     mine->canonicalSpill()->isStackSlot() == other->canonicalSpill()->isStackSlot());

        VirtualRegister *candidate = alloc ? mine : other;
        if (!candidate->canonicalSpill()->isStackSlot())
            return;

        finishedDoubleSlots_.append(interval);
    }
#endif
}

void
LinearScanAllocator::finishInterval(LiveInterval *interval)
{
    LAllocation *alloc = interval->getAllocation();
    JS_ASSERT(!alloc->isUse());

    
    if (!interval->reg())
        return;

    
    JS_ASSERT_IF(alloc->isStackSlot(), *alloc == *interval->reg()->canonicalSpill());

    bool lastInterval = interval->index() == (interval->reg()->numIntervals() - 1);
    if (lastInterval) {
        freeAllocation(interval, alloc);
        interval->reg()->setFinished();
    }

    handled.pushBack(interval);
}






AnyRegister::Code
LinearScanAllocator::findBestFreeRegister(CodePosition *freeUntil)
{
    IonSpew(IonSpew_RegAlloc, "  Computing freeUntilPos");

    
    CodePosition freeUntilPos[AnyRegister::Total];
    bool needFloat = current->reg()->isDouble();
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        AnyRegister reg = AnyRegister::FromCode(i);
        if (reg.allocatable() && reg.isFloat() == needFloat)
            freeUntilPos[i] = CodePosition::MAX;
    }
    for (IntervalIterator i(active.begin()); i != active.end(); i++) {
        
        
        if (i->isSpillInterval() && !i->requireSpill(current)) {
            IonSpew(IonSpew_RegAlloc, "   Register %s free (used by spill interval)",
                    i->getAllocation()->toRegister().name());
            continue;
        }

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

    AnyRegister::Code bestCode = AnyRegister::Invalid;
    if (current->index()) {
        
        
        LiveInterval *previous = current->reg()->getInterval(current->index() - 1);
        if (previous->getAllocation()->isRegister()) {
            AnyRegister prevReg = previous->getAllocation()->toRegister();
            if (freeUntilPos[prevReg.code()] != CodePosition::MIN)
                bestCode = prevReg.code();
        }
    }

    
    Requirement *hint = current->hint();
    if (hint->kind() == Requirement::FIXED && hint->allocation().isRegister()) {
        AnyRegister hintReg = hint->allocation().toRegister();
        if (freeUntilPos[hintReg.code()] > hint->pos())
            bestCode = hintReg.code();
    } else if (hint->kind() == Requirement::SAME_AS_OTHER) {
        LiveInterval *other = vregs[hint->virtualRegister()].intervalFor(hint->pos());
        JS_ASSERT(other);
        if (other->getAllocation()->isRegister()) {
            AnyRegister hintReg = other->getAllocation()->toRegister();
            if (freeUntilPos[hintReg.code()] > hint->pos())
                bestCode = hintReg.code();
        }
    }

    if (bestCode == AnyRegister::Invalid) {
        
        for (uint32 i = 0; i < AnyRegister::Total; i++) {
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
    bool needFloat = current->reg()->isDouble();
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        AnyRegister reg = AnyRegister::FromCode(i);
        if (reg.allocatable() && reg.isFloat() == needFloat)
            nextUsePos[i] = CodePosition::MAX;
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
    if (aa->isRegister() && ba->isRegister() && aa->toRegister() == ba->toRegister()) {
        if (a->isSpillInterval() && !a->requireSpill(b))
            return true;
        if (b->isSpillInterval() && !b->requireSpill(a))
            return true;
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
    JS_ASSERT(!vreg->ins()->isLabel());;

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

LMoveGroup *
LinearScanAllocator::getMoveGroupAfter(CodePosition pos)
{
    VirtualRegister *vreg = &vregs[pos];
    JS_ASSERT(vreg->ins());
    JS_ASSERT(!vreg->ins()->isPhi());

    if (vreg->afterMoves())
        return vreg->afterMoves();

    LMoveGroup *moves = new LMoveGroup;
    vreg->setAfterMoves(moves);
    vreg->block()->insertAfter(vreg->ins(), moves);

    return moves;
}

bool
LinearScanAllocator::addMove(LMoveGroup *moves, LiveInterval *from, LiveInterval *to)
{
    if (*from->getAllocation() == *to->getAllocation())
        return true;
    return moves->add(from->getAllocation(), to->getAllocation());
}

bool
LinearScanAllocator::moveBefore(CodePosition pos, LiveInterval *from, LiveInterval *to)
{
    LMoveGroup *moves = getMoveGroupBefore(pos);
    return addMove(moves, from, to);
}

bool
LinearScanAllocator::moveAfter(CodePosition pos, LiveInterval *from, LiveInterval *to)
{
    JS_ASSERT(pos.subpos() == CodePosition::OUTPUT);
    LMoveGroup *moves = getMoveGroupAfter(pos);
    return addMove(moves, from, to);
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
    IonSpew(IonSpew_RegAlloc, "Beginning register allocation");

    IonSpew(IonSpew_RegAlloc, "Beginning creation of initial data structures");
    if (!createDataStructures())
        return false;
    IonSpew(IonSpew_RegAlloc, "Creation of initial data structures completed");

    IonSpew(IonSpew_RegAlloc, "Beginning liveness analysis");
    if (!buildLivenessInfo())
        return false;
    IonSpew(IonSpew_RegAlloc, "Liveness analysis complete");

    IonSpew(IonSpew_RegAlloc, "Beginning preliminary register allocation");
    if (!allocateRegisters())
        return false;
    IonSpew(IonSpew_RegAlloc, "Preliminary register allocation complete");

    IonSpew(IonSpew_RegAlloc, "Beginning control flow resolution");
    if (!resolveControlFlow())
        return false;
    IonSpew(IonSpew_RegAlloc, "Control flow resolution complete");

    IonSpew(IonSpew_RegAlloc, "Beginning register allocation reification");
    if (!reifyAllocations())
        return false;
    IonSpew(IonSpew_RegAlloc, "Register allocation reification complete");

    IonSpew(IonSpew_RegAlloc, "Beginning safepoint population.");
    if (!populateSafepoints())
        return false;
    IonSpew(IonSpew_RegAlloc, "Safepoint population complete.");

    IonSpew(IonSpew_RegAlloc, "Register allocation complete");

    return true;
}

void
LinearScanAllocator::setIntervalRequirement(LiveInterval *interval)
{
    JS_ASSERT(interval->requirement()->kind() == Requirement::NONE);
    JS_ASSERT(interval->hint()->kind() == Requirement::NONE);

    
    
    VirtualRegister *reg = interval->reg();
    JS_ASSERT(reg);

    if (interval->index() == 0) {
        
        

        if (reg->def()->policy() == LDefinition::PRESET) {
            
            interval->setRequirement(Requirement(*reg->def()->output()));
        } else if (reg->def()->policy() == LDefinition::MUST_REUSE_INPUT) {
            
            LUse *use = reg->ins()->getOperand(reg->def()->getReusedInput())->toUse();
            if (use->isFixedRegister()) {
                if (reg->isDouble())
                    interval->setRequirement(LFloatReg(FloatRegister::FromCode(use->registerCode())));
                else
                    interval->setRequirement(LGeneralReg(Register::FromCode(use->registerCode())));
            } else {
                interval->setRequirement(Requirement(Requirement::REGISTER));
                interval->setHint(Requirement(use->virtualRegister(), interval->start().previous()));
            }
        } else if (reg->ins()->isPhi()) {
            
            
            
            LUse *use = reg->ins()->getOperand(0)->toUse();
            LBlock *predecessor = reg->block()->mir()->getPredecessor(0)->lir();
            CodePosition predEnd = outputOf(predecessor->lastId());
            interval->setHint(Requirement(use->virtualRegister(), predEnd));
        } else {
            
            interval->setRequirement(Requirement(Requirement::REGISTER));
        }
    }

    
    bool fixedOpIsHint = false;
    UsePosition *fixedOp = NULL;
    UsePosition *registerOp = NULL;

    for (UsePositionIterator usePos(interval->usesBegin()); usePos != interval->usesEnd(); usePos++) {
        if (interval->start() == usePos->pos || interval->start() == usePos->pos.previous()) {
            if (usePos->use->policy() == LUse::FIXED) {
                fixedOpIsHint = false;
                fixedOp = *usePos;
                break;
            } else if (usePos->use->policy() == LUse::REGISTER) {
                
                interval->setRequirement(Requirement(Requirement::REGISTER));
            }
        } else {
            if (usePos->use->policy() == LUse::FIXED) {
                if (!fixedOp || usePos->pos.ins() < fixedOp->pos.ins()) {
                    fixedOpIsHint = true;
                    fixedOp = *usePos;
                }
            } else if (usePos->use->policy() == LUse::REGISTER) {
                if (!registerOp || usePos->pos.ins() < registerOp->pos.ins())
                    registerOp = *usePos;
            }
        }
    }

    if (fixedOp) {
        
        AnyRegister required;
        if (reg->isDouble())
            required = AnyRegister(FloatRegister::FromCode(fixedOp->use->registerCode()));
        else
            required = AnyRegister(Register::FromCode(fixedOp->use->registerCode()));

        if (fixedOpIsHint)
            interval->setHint(Requirement(LAllocation(required), fixedOp->pos));
        else
            interval->setRequirement(Requirement(LAllocation(required)));
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
