





#include "jit/LiveRangeAllocator.h"

#include "mozilla/DebugOnly.h"

#include "jsprf.h"

#include "jit/BacktrackingAllocator.h"
#include "jit/BitSet.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;

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
        MOZ_CRASH("Unknown requirement kind.");
    }
}

const char*
Requirement::toString() const
{
#ifdef DEBUG
    
    static char buf[1000];

    char* cursor = buf;
    char* end = cursor + sizeof(buf);

    int n = -1;  
    switch (kind()) {
      case NONE:
        return "none";
      case REGISTER:
        n = JS_snprintf(cursor, end - cursor, "r");
        break;
      case FIXED:
        n = JS_snprintf(cursor, end - cursor, "%s", allocation().toString());
        break;
      case MUST_REUSE_INPUT:
        n = JS_snprintf(cursor, end - cursor, "v%u", virtualRegister());
        break;
    }
    if (n < 0)
        return "???";
    cursor += n;

    if (pos() != CodePosition::MIN) {
        n = JS_snprintf(cursor, end - cursor, "@%u", pos().bits());
        if (n < 0)
            return "???";
        cursor += n;
    }

    return buf;
#else
    return " ???";
#endif
}

void
Requirement::dump() const
{
    fprintf(stderr, "%s\n", toString());
}

bool
LiveInterval::Range::contains(const Range* other) const
{
    return from <= other->from && to >= other->to;
}

void
LiveInterval::Range::intersect(const Range* other, Range* pre, Range* inside, Range* post) const
{
    MOZ_ASSERT(pre->empty() && inside->empty() && post->empty());

    CodePosition innerFrom = from;
    if (from < other->from) {
        if (to < other->from) {
            *pre = Range(from, to);
            return;
        }
        *pre = Range(from, other->from);
        innerFrom = other->from;
    }

    CodePosition innerTo = to;
    if (to > other->to) {
        if (from >= other->to) {
            *post = Range(from, to);
            return;
        }
        *post = Range(other->to, to);
        innerTo = other->to;
    }

    if (innerFrom != innerTo)
        *inside = Range(innerFrom, innerTo);
}

const char*
LiveInterval::Range::toString() const
{
#ifdef DEBUG
    
    static char buf[1000];

    char* cursor = buf;
    char* end = cursor + sizeof(buf);

    int n = JS_snprintf(cursor, end - cursor, "[%u,%u)", from.bits(), to.bits());
    if (n < 0)
        return " ???";
    cursor += n;

    return buf;
#else
    return " ???";
#endif
}

void
LiveInterval::Range::dump() const
{
    fprintf(stderr, "%s\n", toString());
}

bool
LiveInterval::addRangeAtHead(CodePosition from, CodePosition to)
{
    MOZ_ASSERT(from < to);
    MOZ_ASSERT(ranges_.empty() || from <= ranges_.back().from);

    Range newRange(from, to);

    if (ranges_.empty())
        return ranges_.append(newRange);

    Range& first = ranges_.back();
    if (to < first.from)
        return ranges_.append(newRange);

    if (to == first.from) {
        first.from = from;
        return true;
    }

    MOZ_ASSERT(from < first.to);
    MOZ_ASSERT(to > first.from);
    if (from < first.from)
        first.from = from;
    if (to > first.to)
        first.to = to;

    return true;
}

bool
LiveInterval::addRange(CodePosition from, CodePosition to)
{
    MOZ_ASSERT(from < to);

    Range newRange(from, to);

    Range* i;
    
    for (i = ranges_.end(); i > ranges_.begin(); i--) {
        if (newRange.from <= i[-1].to) {
            if (i[-1].from < newRange.from)
                newRange.from = i[-1].from;
            break;
        }
    }
    
    Range* coalesceEnd = i;
    for (; i > ranges_.begin(); i--) {
        if (newRange.to < i[-1].from)
            break;
        if (newRange.to < i[-1].to)
            newRange.to = i[-1].to;
    }

    if (i == coalesceEnd)
        return ranges_.insert(i, newRange);

    i[0] = newRange;
    ranges_.erase(i + 1, coalesceEnd);
    return true;
}

void
LiveInterval::setFrom(CodePosition from)
{
    while (!ranges_.empty()) {
        if (ranges_.back().to < from) {
            ranges_.popBack();
        } else {
            if (from == ranges_.back().to)
                ranges_.popBack();
            else
                ranges_.back().from = from;
            break;
        }
    }
}

bool
LiveInterval::covers(CodePosition pos)
{
    if (pos < start() || pos >= end())
        return false;

    
    size_t i = lastProcessedRangeIfValid(pos);
    for (; i < ranges_.length(); i--) {
        if (pos < ranges_[i].from)
            return false;
        setLastProcessedRange(i, pos);
        if (pos < ranges_[i].to)
            return true;
    }
    return false;
}

CodePosition
LiveInterval::intersect(LiveInterval* other)
{
    if (start() > other->start())
        return other->intersect(this);

    
    
    size_t i = lastProcessedRangeIfValid(other->start());
    size_t j = other->ranges_.length() - 1;
    if (i >= ranges_.length() || j >= other->ranges_.length())
        return CodePosition::MIN;

    while (true) {
        const Range& r1 = ranges_[i];
        const Range& r2 = other->ranges_[j];

        if (r1.from <= r2.from) {
            if (r1.from <= other->start())
                setLastProcessedRange(i, other->start());
            if (r2.from < r1.to)
                return r2.from;
            if (i == 0 || ranges_[i-1].from > other->end())
                break;
            i--;
        } else {
            if (r1.from < r2.to)
                return r1.from;
            if (j == 0 || other->ranges_[j-1].from > end())
                break;
            j--;
        }
    }

    return CodePosition::MIN;
}












bool
LiveInterval::splitFrom(CodePosition pos, LiveInterval* after)
{
    MOZ_ASSERT(pos >= start() && pos < end());
    MOZ_ASSERT(after->ranges_.empty());

    
    size_t bufferLength = ranges_.length();
    Range* buffer = ranges_.extractRawBuffer();
    if (!buffer)
        return false;
    after->ranges_.replaceRawBuffer(buffer, bufferLength);

    
    for (Range* i = &after->ranges_.back(); i >= after->ranges_.begin(); i--) {
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

    
    UsePosition* prev = nullptr;
    for (UsePositionIterator usePos(usesBegin()); usePos != usesEnd(); usePos++) {
        if (usePos->pos > pos)
            break;
        prev = *usePos;
    }

    uses_.splitAfter(prev, &after->uses_);
    return true;
}

void
LiveInterval::addUse(UsePosition* use)
{
    
    
    
    
    UsePosition* prev = nullptr;
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

void
LiveInterval::addUseAtEnd(UsePosition* use)
{
    MOZ_ASSERT(uses_.empty() || use->pos >= uses_.back()->pos);
    uses_.pushBack(use);
}

UsePosition*
LiveInterval::nextUseAfter(CodePosition after)
{
    for (UsePositionIterator usePos(usesBegin()); usePos != usesEnd(); usePos++) {
        if (usePos->pos >= after) {
            LUse::Policy policy = usePos->use->policy();
            MOZ_ASSERT(policy != LUse::RECOVERED_INPUT);
            if (policy != LUse::KEEPALIVE)
                return *usePos;
        }
    }
    return nullptr;
}

UsePosition*
LiveInterval::popUse()
{
    return uses_.popFront();
}







CodePosition
LiveInterval::nextUsePosAfter(CodePosition after)
{
    UsePosition* min = nextUseAfter(after);
    return min ? min->pos : CodePosition::MAX;
}

LiveInterval*
VirtualRegister::intervalFor(CodePosition pos)
{
    
    for (LiveInterval** i = intervals_.begin(); i != intervals_.end(); i++) {
        if ((*i)->covers(pos))
            return *i;
        if (pos < (*i)->start())
            break;
    }
    return nullptr;
}

LiveInterval*
VirtualRegister::getFirstInterval()
{
    MOZ_ASSERT(!intervals_.empty());
    return intervals_[0];
}


template bool LiveRangeAllocator<BacktrackingVirtualRegister>::buildLivenessInfo();
template void LiveRangeAllocator<BacktrackingVirtualRegister>::dumpVregs();

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





template <typename VREG>
bool
LiveRangeAllocator<VREG>::init()
{
    if (!RegisterAllocator::init())
        return false;

    liveIn = mir->allocate<BitSet>(graph.numBlockIds());
    if (!liveIn)
        return false;

    
    for (size_t i = 0; i < AnyRegister::Total; i++) {
        AnyRegister reg = AnyRegister::FromCode(i);
        LiveInterval* interval = LiveInterval::New(alloc(), 0);
        interval->setAllocation(LAllocation(reg));
        fixedIntervals[i] = interval;
    }

    fixedIntervalsUnion = LiveInterval::New(alloc(), 0);

    if (!vregs.init(mir, graph.numVirtualRegisters()))
        return false;

    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (mir->shouldCancel("Create data structures (main loop)"))
            return false;

        LBlock* block = graph.getBlock(i);
        for (LInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            for (size_t j = 0; j < ins->numDefs(); j++) {
                LDefinition* def = ins->getDef(j);
                if (def->isBogusTemp())
                    continue;
                if (!vregs[def].init(alloc(), *ins, def,  false))
                    return false;
            }

            for (size_t j = 0; j < ins->numTemps(); j++) {
                LDefinition* def = ins->getTemp(j);
                if (def->isBogusTemp())
                    continue;
                if (!vregs[def].init(alloc(), *ins, def,  true))
                    return false;
            }
        }
        for (size_t j = 0; j < block->numPhis(); j++) {
            LPhi* phi = block->getPhi(j);
            LDefinition* def = phi->getDef(0);
            if (!vregs[def].init(alloc(), phi, def,  false))
                return false;
        }
    }

    return true;
}





















template <typename VREG>
bool
LiveRangeAllocator<VREG>::buildLivenessInfo()
{
    JitSpew(JitSpew_RegAlloc, "Beginning liveness analysis");

    if (!init())
        return false;

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
            if (!vregs[*liveRegId].getInterval(0)->addRangeAtHead(entryOf(block),
                                                                  exitOf(block).next()))
            {
                return false;
            }
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
                    if (!found && !addFixedRangeAtHead(*iter, outputOf(*ins), outputOf(*ins).next()))
                        return false;
                }
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

                LiveInterval* interval = vregs[def].getInterval(0);
                interval->setFrom(from);

                
                
                if (interval->numRanges() == 0) {
                    if (!interval->addRangeAtHead(from, from.next()))
                        return false;
                }
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
                                if (GetFixedRegister(vregs[use].def(), use) == reg)
                                    from = outputOf(*ins);
                            }
                        }
                    }
                }

                CodePosition to =
                    ins->isCall() ? outputOf(*ins) : outputOf(*ins).next();
                if (!vregs[temp].getInterval(0)->addRangeAtHead(from, to))
                    return false;
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
                            MOZ_ASSERT(vregs[ins->getTemp(i)].isFloatReg() != vregs[use].isFloatReg());
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

                    LiveInterval* interval = vregs[use].getInterval(0);
                    if (!interval->addRangeAtHead(entryOf(block), to.next()))
                        return false;
                    interval->addUse(new(alloc()) UsePosition(use, to));

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
                if (!vregs[def].getInterval(0)->addRangeAtHead(entryPos, entryPos.next()))
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
                    if (!vregs[*liveRegId].getInterval(0)->addRange(from, to))
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

    validateVirtualRegisters();

    
    
    
    if (fixedIntervalsUnion->numRanges() == 0) {
        if (!fixedIntervalsUnion->addRangeAtHead(CodePosition(0, CodePosition::INPUT),
                                                 CodePosition(0, CodePosition::OUTPUT)))
        {
            return false;
        }
    }

    JitSpew(JitSpew_RegAlloc, "Liveness analysis complete");

    if (JitSpewEnabled(JitSpew_RegAlloc)) {
        dumpInstructions();

        fprintf(stderr, "Live ranges by virtual register:\n");
        dumpVregs();
    }

    return true;
}

template <typename VREG>
void
LiveRangeAllocator<VREG>::dumpVregs()
{
#ifdef DEBUG
    
    MOZ_ASSERT(vregs[0u].numIntervals() == 0);
    for (uint32_t i = 1; i < graph.numVirtualRegisters(); i++) {
        fprintf(stderr, "  ");
        VirtualRegister& vreg = vregs[i];
        for (size_t j = 0; j < vreg.numIntervals(); j++) {
            if (j)
                fprintf(stderr, " / ");
            fprintf(stderr, "%s", vreg.getInterval(j)->toString());
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "\n");
#endif
}

#ifdef DEBUG

void
LiveInterval::validateRanges()
{
    Range* prev = nullptr;

    for (size_t i = ranges_.length() - 1; i < ranges_.length(); i--) {
        Range* range = &ranges_[i];

        MOZ_ASSERT(range->from < range->to);
        MOZ_ASSERT_IF(prev, prev->to <= range->from);
        prev = range;
    }
}

#endif 

const char*
LiveInterval::rangesToString() const
{
#ifdef DEBUG
    
    static char buf[2000];

    char* cursor = buf;
    char* end = cursor + sizeof(buf);

    int n;

    for (size_t i = ranges_.length() - 1; i < ranges_.length(); i--) {
        const LiveInterval::Range* range = getRange(i);
        n = JS_snprintf(cursor, end - cursor, " %s", range->toString());
        if (n < 0)
            return " ???";
        cursor += n;
    }

    return buf;
#else
    return " ???";
#endif
}

#ifdef DEBUG
static bool
IsHintInteresting(const Requirement& requirement, const Requirement& hint)
{
    if (hint.kind() == Requirement::NONE)
        return false;

    if (hint.kind() != Requirement::FIXED && hint.kind() != Requirement::REGISTER)
        return true;

    Requirement merge = requirement;
    if (!merge.mergeRequirement(hint))
        return true;

    return merge.kind() != requirement.kind();
}
#endif

const char*
LiveInterval::toString() const
{
#ifdef DEBUG
    
    static char buf[2000];

    char* cursor = buf;
    char* end = cursor + sizeof(buf);

    int n;

    if (hasVreg()) {
        n = JS_snprintf(cursor, end - cursor, "v%u", vreg());
        if (n < 0) return "???";
        cursor += n;
    }

    n = JS_snprintf(cursor, end - cursor, "[%u]", index());
    if (n < 0) return "???";
    cursor += n;

    if (requirement_.kind() != Requirement::NONE || hint_.kind() != Requirement::NONE) {
        n = JS_snprintf(cursor, end - cursor, " req(");
        if (n < 0) return "???";
        cursor += n;

        bool printHint = IsHintInteresting(requirement_, hint_);

        if (requirement_.kind() != Requirement::NONE) {
            n = JS_snprintf(cursor, end - cursor, "%s%s",
                            requirement_.toString(),
                            printHint ? "," : "");
            if (n < 0) return "???";
            cursor += n;
        }
        if (printHint) {
            n = JS_snprintf(cursor, end - cursor, "%s?", hint_.toString());
            if (n < 0) return "???";
            cursor += n;
        }

        n = JS_snprintf(cursor, end - cursor, ")");
        if (n < 0) return "???";
        cursor += n;
    }

    if (!alloc_.isBogus()) {
        n = JS_snprintf(cursor, end - cursor, " has(%s)", alloc_.toString());
        if (n < 0) return "???";
        cursor += n;
    }

    n = JS_snprintf(cursor, end - cursor, "%s", rangesToString());
    if (n < 0) return "???";
    cursor += n;

    for (UsePositionIterator usePos(usesBegin()); usePos != usesEnd(); usePos++) {
        n = JS_snprintf(cursor, end - cursor, " %s@%u",
                        usePos->use->toString(), usePos->pos.bits());
        if (n < 0) return "???";
        cursor += n;
    }

    return buf;
#else
    return "???";
#endif
}

void
LiveInterval::dump() const
{
    fprintf(stderr, "%s\n", toString());
}
