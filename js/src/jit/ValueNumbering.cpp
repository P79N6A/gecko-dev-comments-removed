





#include "jit/ValueNumbering.h"

#include "jit/AliasAnalysis.h"
#include "jit/IonAnalysis.h"
#include "jit/JitSpewer.h"
#include "jit/MIRGenerator.h"

using namespace js;
using namespace js::jit;



























HashNumber
ValueNumberer::VisibleValues::ValueHasher::hash(Lookup ins)
{
    return ins->valueHash();
}


bool
ValueNumberer::VisibleValues::ValueHasher::match(Key k, Lookup l)
{
    
    
    if (k->dependency() != l->dependency())
        return false;

    bool congruent = k->congruentTo(l); 
    MOZ_ASSERT(congruent == l->congruentTo(k), "congruentTo relation is not symmetric");
    return congruent;
}

void
ValueNumberer::VisibleValues::ValueHasher::rekey(Key &k, Key newKey)
{
    k = newKey;
}

ValueNumberer::VisibleValues::VisibleValues(TempAllocator &alloc)
  : set_(alloc)
{}


bool
ValueNumberer::VisibleValues::init()
{
    return set_.init();
}


ValueNumberer::VisibleValues::Ptr
ValueNumberer::VisibleValues::findLeader(const MDefinition *def) const
{
    return set_.lookup(def);
}


ValueNumberer::VisibleValues::AddPtr
ValueNumberer::VisibleValues::findLeaderForAdd(MDefinition *def)
{
    return set_.lookupForAdd(def);
}


bool
ValueNumberer::VisibleValues::add(AddPtr p, MDefinition *def)
{
    return set_.add(p, def);
}


void
ValueNumberer::VisibleValues::overwrite(AddPtr p, MDefinition *def)
{
    set_.rekeyInPlace(p, def);
}


void
ValueNumberer::VisibleValues::forget(const MDefinition *def)
{
    Ptr p = set_.lookup(def);
    if (p && *p == def)
        set_.remove(p);
}


void
ValueNumberer::VisibleValues::clear()
{
    set_.clear();
}

#ifdef DEBUG

bool
ValueNumberer::VisibleValues::has(const MDefinition *def) const
{
    Ptr p = set_.lookup(def);
    return p && *p == def;
}
#endif


static bool
DeadIfUnused(const MDefinition *def)
{
    return !def->isEffectful() && !def->isGuard() && !def->isControlInstruction() &&
           (!def->isInstruction() || !def->toInstruction()->resumePoint());
}



static bool
IsDiscardable(const MDefinition *def)
{
    return !def->hasUses() && (DeadIfUnused(def) || def->block()->isMarked());
}


static void
ReplaceAllUsesWith(MDefinition *from, MDefinition *to)
{
    MOZ_ASSERT(from != to, "GVN shouldn't try to replace a value with itself");
    MOZ_ASSERT(from->type() == to->type(), "Def replacement has different type");

    
    
    from->justReplaceAllUsesWith(to);
}


static bool
HasSuccessor(const MControlInstruction *block, const MBasicBlock *succ)
{
    for (size_t i = 0, e = block->numSuccessors(); i != e; ++i) {
        if (block->getSuccessor(i) == succ)
            return true;
    }
    return false;
}




static MBasicBlock *
ComputeNewDominator(MBasicBlock *block, MBasicBlock *old)
{
    MBasicBlock *now = block->getPredecessor(0);
    for (size_t i = 1, e = block->numPredecessors(); i < e; ++i) {
        MBasicBlock *pred = block->getPredecessor(i);
        
        
        while (!now->dominates(pred)) {
            MBasicBlock *next = now->immediateDominator();
            if (next == old)
                return old;
            if (next == now) {
                MOZ_ASSERT(block == old, "Non-self-dominating block became self-dominating");
                return block;
            }
            now = next;
        }
    }
    MOZ_ASSERT(old != block || old != now, "Missed self-dominating block staying self-dominating");
    return now;
}


static bool
BlockHasInterestingDefs(MBasicBlock *block)
{
    return !block->phisEmpty() || *block->begin() != block->lastIns();
}



static bool
ScanDominatorsForDefs(MBasicBlock *block)
{
    for (MBasicBlock *i = block;;) {
        if (BlockHasInterestingDefs(block))
            return true;

        MBasicBlock *immediateDominator = i->immediateDominator();
        if (immediateDominator == i)
            break;
        i = immediateDominator;
    }
    return false;
}



static bool
ScanDominatorsForDefs(MBasicBlock *now, MBasicBlock *old)
{
    MOZ_ASSERT(old->dominates(now), "Refined dominator not dominated by old dominator");

    for (MBasicBlock *i = now; i != old; i = i->immediateDominator()) {
        if (BlockHasInterestingDefs(i))
            return true;
    }
    return false;
}




static bool
IsDominatorRefined(MBasicBlock *block)
{
    MBasicBlock *old = block->immediateDominator();
    MBasicBlock *now = ComputeNewDominator(block, old);

    
    
    
    MControlInstruction *control = block->lastIns();
    if (*block->begin() == control && block->phisEmpty() && control->isGoto() &&
        !block->dominates(control->toGoto()->target()))
    {
        return false;
    }

    
    
    if (block == old)
        return block != now && ScanDominatorsForDefs(now);
    MOZ_ASSERT(block != now, "Non-self-dominating block became self-dominating");
    return ScanDominatorsForDefs(now, old);
}



bool
ValueNumberer::handleUseReleased(MDefinition *def, UseRemovedOption useRemovedOption)
{
    if (IsDiscardable(def)) {
        values_.forget(def);
        if (!deadDefs_.append(def))
            return false;
    } else {
        if (useRemovedOption == SetUseRemoved)
            def->setUseRemovedUnchecked();
    }
    return true;
}


bool
ValueNumberer::discardDefsRecursively(MDefinition *def)
{
    MOZ_ASSERT(deadDefs_.empty(), "deadDefs_ not cleared");

    return discardDef(def) && processDeadDefs();
}





bool
ValueNumberer::releaseResumePointOperands(MResumePoint *resume)
{
    for (size_t i = 0, e = resume->numOperands(); i < e; ++i) {
        if (!resume->hasOperand(i))
            continue;
        MDefinition *op = resume->getOperand(i);
        
        
        if (op->isDiscarded())
            continue;
        resume->releaseOperand(i);

        
        
        
        if (!handleUseReleased(op, SetUseRemoved))
            return false;
    }
    return true;
}



bool
ValueNumberer::releaseAndRemovePhiOperands(MPhi *phi)
{
    
    for (int o = phi->numOperands() - 1; o >= 0; --o) {
        MDefinition *op = phi->getOperand(o);
        phi->removeOperand(o);
        if (!handleUseReleased(op, DontSetUseRemoved))
            return false;
    }
    return true;
}



bool
ValueNumberer::releaseOperands(MDefinition *def)
{
    for (size_t o = 0, e = def->numOperands(); o < e; ++o) {
        MDefinition *op = def->getOperand(o);
        def->releaseOperand(o);
        if (!handleUseReleased(op, DontSetUseRemoved))
            return false;
    }
    return true;
}


bool
ValueNumberer::discardDef(MDefinition *def)
{
    JitSpew(JitSpew_GVN, "      Discarding %s %s%u",
            def->block()->isMarked() ? "unreachable" : "dead",
            def->opName(), def->id());

#ifdef DEBUG
    MOZ_ASSERT(def != nextDef_, "Invalidating the MDefinition iterator");
    if (def->block()->isMarked()) {
        MOZ_ASSERT(!def->hasUses(), "Discarding def that still has uses");
    } else {
        MOZ_ASSERT(IsDiscardable(def), "Discarding non-discardable definition");
        MOZ_ASSERT(!values_.has(def), "Discarding a definition still in the set");
    }
#endif

    MBasicBlock *block = def->block();
    if (def->isPhi()) {
        MPhi *phi = def->toPhi();
        if (!releaseAndRemovePhiOperands(phi))
             return false;
        MPhiIterator at(block->phisBegin(phi));
        block->discardPhiAt(at);
    } else {
        MInstruction *ins = def->toInstruction();
        if (MResumePoint *resume = ins->resumePoint()) {
            if (!releaseResumePointOperands(resume))
                return false;
        }
        if (!releaseOperands(ins))
             return false;
        block->discardIgnoreOperands(ins);
    }

    
    
    if (block->phisEmpty() && block->begin() == block->end()) {
        MOZ_ASSERT(block->isMarked(), "Reachable block lacks at least a control instruction");

        
        
        
        if (block->immediateDominator() != block) {
            JitSpew(JitSpew_GVN, "      Block block%u is now empty; discarding", block->id());
            graph_.removeBlock(block);
            blocksRemoved_ = true;
        } else {
            JitSpew(JitSpew_GVN, "      Dominator root block%u is now empty; will discard later",
                    block->id());
        }
    }

    return true;
}


bool
ValueNumberer::processDeadDefs()
{
    MDefinition *nextDef = nextDef_;
    while (!deadDefs_.empty()) {
        MDefinition *def = deadDefs_.popCopy();

        
        
        if (def == nextDef)
            continue;

        if (!discardDef(def))
            return false;
    }
    return true;
}



static bool
hasNonDominatingPredecessor(MBasicBlock *block, MBasicBlock *loopPred)
{
    MOZ_ASSERT(block->isLoopHeader());
    MOZ_ASSERT(block->loopPredecessor() == loopPred);

    for (uint32_t i = 0, e = block->numPredecessors(); i < e; ++i) {
        MBasicBlock *pred = block->getPredecessor(i);
        if (pred != loopPred && !block->dominates(pred))
            return true;
    }
    return false;
}



bool
ValueNumberer::fixupOSROnlyLoop(MBasicBlock *block, MBasicBlock *backedge)
{
    
    
    
    
    
    
    MBasicBlock *fake = MBasicBlock::NewAsmJS(graph_, block->info(),
                                              nullptr, MBasicBlock::NORMAL);
    if (fake == nullptr)
        return false;

    graph_.insertBlockBefore(block, fake);
    fake->setImmediateDominator(fake);
    fake->addNumDominated(1);

    
    
    
    for (MPhiIterator iter(block->phisBegin()), end(block->phisEnd()); iter != end; ++iter) {
        MPhi *phi = *iter;
        MPhi *fakePhi = MPhi::New(graph_.alloc(), phi->type());
        fake->addPhi(fakePhi);
        if (!phi->addInputSlow(fakePhi))
            return false;
    }

    fake->end(MGoto::New(graph_.alloc(), block));

    if (!block->addPredecessorWithoutPhis(fake))
        return false;

    
    block->clearLoopHeader();
    block->setLoopHeader(backedge);

    JitSpew(JitSpew_GVN, "        Created fake block%u", fake->id());
    return true;
}



bool
ValueNumberer::removePredecessorAndDoDCE(MBasicBlock *block, MBasicBlock *pred)
{
    MOZ_ASSERT(!block->isMarked(),
               "Block marked unreachable should have predecessors removed already");

    
    
    if (!block->phisEmpty()) {
        uint32_t index = pred->positionInPhiSuccessor();
        for (MPhiIterator iter(block->phisBegin()), end(block->phisEnd()); iter != end; ++iter) {
            MPhi *phi = *iter;
            MOZ_ASSERT(!values_.has(phi), "Visited phi in block having predecessor removed");

            MDefinition *op = phi->getOperand(index);
            if (op == phi)
                continue;

            
            
            phi->replaceOperand(index, phi);

            if (!handleUseReleased(op, DontSetUseRemoved) || !processDeadDefs())
                return false;
        }
    }

    block->removePredecessor(pred);
    return true;
}





bool
ValueNumberer::removePredecessorAndCleanUp(MBasicBlock *block, MBasicBlock *pred)
{
    MOZ_ASSERT(!block->isMarked(), "Removing predecessor on block already marked unreachable");

    
    
    for (MPhiIterator iter(block->phisBegin()), end(block->phisEnd()); iter != end; ++iter)
        values_.forget(*iter);

    
    
    bool isUnreachableLoop = false;
    MBasicBlock *origBackedgeForOSRFixup = nullptr;
    if (block->isLoopHeader()) {
        if (block->loopPredecessor() == pred) {
            if (MOZ_UNLIKELY(hasNonDominatingPredecessor(block, pred))) {
                JitSpew(JitSpew_GVN, "      "
                        "Loop with header block%u is now only reachable through an "
                        "OSR entry into the middle of the loop!!", block->id());
                origBackedgeForOSRFixup = block->backedge();
            } else {
                
                isUnreachableLoop = true;
                JitSpew(JitSpew_GVN, "      "
                        "Loop with header block%u is no longer reachable",
                        block->id());
            }
#ifdef DEBUG
        } else if (block->hasUniqueBackedge() && block->backedge() == pred) {
            JitSpew(JitSpew_GVN, "      Loop with header block%u is no longer a loop",
                    block->id());
#endif
        }
    }

    
    if (!removePredecessorAndDoDCE(block, pred))
        return false;

    
    if (block->numPredecessors() == 0 || isUnreachableLoop) {
        JitSpew(JitSpew_GVN, "      Disconnecting block%u", block->id());

        
        
        
        MBasicBlock *parent = block->immediateDominator();
        if (parent != block)
            parent->removeImmediatelyDominatedBlock(block);

        
        
        
        
        
        if (block->isLoopHeader())
            block->clearLoopHeader();
        for (size_t i = 0, e = block->numPredecessors(); i < e; ++i) {
            if (!removePredecessorAndDoDCE(block, block->getPredecessor(i)))
                return false;
        }

        
        
        if (MResumePoint *resume = block->entryResumePoint()) {
            if (!releaseResumePointOperands(resume) || !processDeadDefs())
                return false;
            if (MResumePoint *outer = block->outerResumePoint()) {
                if (!releaseResumePointOperands(outer) || !processDeadDefs())
                    return false;
            }
            for (MInstructionIterator iter(block->begin()), end(block->end()); iter != end; ) {
                MInstruction *ins = *iter++;
                nextDef_ = *iter;
                if (MResumePoint *resume = ins->resumePoint()) {
                    if (!releaseResumePointOperands(resume) || !processDeadDefs())
                        return false;
                }
            }
        } else {
#ifdef DEBUG
            MOZ_ASSERT(block->outerResumePoint() == nullptr,
                       "Outer resume point in block without an entry resume point");
            for (MInstructionIterator iter(block->begin()), end(block->end());
                 iter != end;
                 ++iter)
            {
                MOZ_ASSERT(iter->resumePoint() == nullptr,
                           "Instruction with resume point in block without entry resume point");
            }
#endif
        }

        
        
        block->mark();
    } else if (MOZ_UNLIKELY(origBackedgeForOSRFixup != nullptr)) {
        
        
        if (!fixupOSROnlyLoop(block, origBackedgeForOSRFixup))
            return false;
    }

    return true;
}


MDefinition *
ValueNumberer::simplified(MDefinition *def) const
{
    return def->foldsTo(graph_.alloc());
}



MDefinition *
ValueNumberer::leader(MDefinition *def)
{
    
    
    
    
    if (!def->isEffectful() && def->congruentTo(def)) {
        
        VisibleValues::AddPtr p = values_.findLeaderForAdd(def);
        if (p) {
            MDefinition *rep = *p;
            if (rep->block()->dominates(def->block())) {
                
                return rep;
            }

            
            
            values_.overwrite(p, def);
        } else {
            
            if (!values_.add(p, def))
                return nullptr;
        }
    }

    return def;
}


bool
ValueNumberer::hasLeader(const MPhi *phi, const MBasicBlock *phiBlock) const
{
    if (VisibleValues::Ptr p = values_.findLeader(phi)) {
        const MDefinition *rep = *p;
        return rep != phi && rep->block()->dominates(phiBlock);
    }
    return false;
}





bool
ValueNumberer::loopHasOptimizablePhi(MBasicBlock *header) const
{
    
    if (header->isMarked())
        return false;

    
    
    for (MPhiIterator iter(header->phisBegin()), end(header->phisEnd()); iter != end; ++iter) {
        MPhi *phi = *iter;
        MOZ_ASSERT(phi->hasUses(), "Missed an unused phi");

        if (phi->operandIfRedundant() || hasLeader(phi, header))
            return true; 
    }
    return false;
}


bool
ValueNumberer::visitDefinition(MDefinition *def)
{
    
    
    const MDefinition *dep = def->dependency();
    if (dep != nullptr && (dep->isDiscarded() || dep->block()->isDead())) {
        JitSpew(JitSpew_GVN, "      AliasAnalysis invalidated");
        if (updateAliasAnalysis_ && !dependenciesBroken_) {
            
            
            JitSpew(JitSpew_GVN, "        Will recompute!");
            dependenciesBroken_ = true;
        }
        
        def->setDependency(def->toInstruction());
    }

    
    MDefinition *sim = simplified(def);
    if (sim != def) {
        if (sim == nullptr)
            return false;

        
        if (sim->block() == nullptr)
            def->block()->insertAfter(def->toInstruction(), sim->toInstruction());

        JitSpew(JitSpew_GVN, "      Folded %s%u to %s%u",
                def->opName(), def->id(), sim->opName(), sim->id());
        ReplaceAllUsesWith(def, sim);

        
        
        
        def->setNotGuardUnchecked();

        if (DeadIfUnused(def)) {
            if (!discardDefsRecursively(def))
                return false;
        }
        def = sim;
    }

    
    MDefinition *rep = leader(def);
    if (rep != def) {
        if (rep == nullptr)
            return false;
        if (rep->updateForReplacement(def)) {
            JitSpew(JitSpew_GVN,
                    "      Replacing %s%u with %s%u",
                    def->opName(), def->id(), rep->opName(), rep->id());
            ReplaceAllUsesWith(def, rep);

            
            
            
            def->setNotGuardUnchecked();

            if (DeadIfUnused(def)) {
                
                
                mozilla::DebugOnly<bool> r = discardDef(def);
                MOZ_ASSERT(r, "discardDef shouldn't have tried to add anything to the worklist, "
                              "so it shouldn't have failed");
                MOZ_ASSERT(deadDefs_.empty(),
                           "discardDef shouldn't have added anything to the worklist");
            }
            def = rep;
        }
    }

    return true;
}


bool
ValueNumberer::visitControlInstruction(MBasicBlock *block, const MBasicBlock *dominatorRoot)
{
    
    MControlInstruction *control = block->lastIns();
    MDefinition *rep = simplified(control);
    if (rep == control)
        return true;

    if (rep == nullptr)
        return false;

    MControlInstruction *newControl = rep->toControlInstruction();
    MOZ_ASSERT(!newControl->block(),
               "Control instruction replacement shouldn't already be in a block");
    JitSpew(JitSpew_GVN, "      Folded control instruction %s%u to %s%u",
            control->opName(), control->id(), newControl->opName(), graph_.getNumInstructionIds());

    
    
    size_t oldNumSuccs = control->numSuccessors();
    size_t newNumSuccs = newControl->numSuccessors();
    if (newNumSuccs != oldNumSuccs) {
        MOZ_ASSERT(newNumSuccs < oldNumSuccs, "New control instruction has too many successors");
        for (size_t i = 0; i != oldNumSuccs; ++i) {
            MBasicBlock *succ = control->getSuccessor(i);
            if (HasSuccessor(newControl, succ))
                continue;
            if (succ->isMarked())
                continue;
            if (!removePredecessorAndCleanUp(succ, block))
                return false;
            if (succ->isMarked())
                continue;
            if (!rerun_) {
                if (!remainingBlocks_.append(succ))
                    return false;
            }
        }
    }

    if (!releaseOperands(control))
        return false;
    block->discardIgnoreOperands(control);
    block->end(newControl);
    return processDeadDefs();
}



bool
ValueNumberer::visitUnreachableBlock(MBasicBlock *block)
{
    JitSpew(JitSpew_GVN, "    Visiting unreachable block%u%s%s%s", block->id(),
            block->isLoopHeader() ? " (loop header)" : "",
            block->isSplitEdge() ? " (split edge)" : "",
            block->immediateDominator() == block ? " (dominator root)" : "");

    MOZ_ASSERT(block->isMarked(), "Visiting unmarked (and therefore reachable?) block");
    MOZ_ASSERT(block->numPredecessors() == 0, "Block marked unreachable still has predecessors");
    MOZ_ASSERT(block != graph_.entryBlock(), "Removing normal entry block");
    MOZ_ASSERT(block != graph_.osrBlock(), "Removing OSR entry block");
    MOZ_ASSERT(deadDefs_.empty(), "deadDefs_ not cleared");

    
    for (size_t i = 0, e = block->numSuccessors(); i < e; ++i) {
        MBasicBlock *succ = block->getSuccessor(i);
        if (succ->isDead() || succ->isMarked())
            continue;
        if (!removePredecessorAndCleanUp(succ, block))
            return false;
        if (succ->isMarked())
            continue;
        
        
        if (!rerun_) {
            if (!remainingBlocks_.append(succ))
                return false;
        }
    }

    
    
    for (MDefinitionIterator iter(block); iter; ) {
        MDefinition *def = *iter++;
        if (def->hasUses())
            continue;
        nextDef_ = *iter;
        if (!discardDefsRecursively(def))
            return false;
    }

    nextDef_ = nullptr;
    MControlInstruction *control = block->lastIns();
    return discardDefsRecursively(control);
}


bool
ValueNumberer::visitBlock(MBasicBlock *block, const MBasicBlock *dominatorRoot)
{
    MOZ_ASSERT(!block->isMarked(), "Blocks marked unreachable during GVN");
    MOZ_ASSERT(!block->isDead(), "Block to visit is already dead");

    JitSpew(JitSpew_GVN, "    Visiting block%u", block->id());

    
    for (MDefinitionIterator iter(block); iter; ) {
        MDefinition *def = *iter++;

        
        nextDef_ = *iter;

        
        if (IsDiscardable(def)) {
            if (!discardDefsRecursively(def))
                return false;
            continue;
        }

        if (!visitDefinition(def))
            return false;
    }
    nextDef_ = nullptr;

    return visitControlInstruction(block, dominatorRoot);
}


bool
ValueNumberer::visitDominatorTree(MBasicBlock *dominatorRoot)
{
    JitSpew(JitSpew_GVN, "  Visiting dominator tree (with %llu blocks) rooted at block%u%s",
            uint64_t(dominatorRoot->numDominated()), dominatorRoot->id(),
            dominatorRoot == graph_.entryBlock() ? " (normal entry block)" :
            dominatorRoot == graph_.osrBlock() ? " (OSR entry block)" :
            dominatorRoot->numPredecessors() == 0 ? " (odd unreachable block)" :
            " (merge point from normal entry and OSR entry)");
    MOZ_ASSERT(dominatorRoot->immediateDominator() == dominatorRoot,
            "root is not a dominator tree root");

    
    
    
    
    size_t numVisited = 0;
    size_t numDiscarded = 0;
    for (ReversePostorderIterator iter(graph_.rpoBegin(dominatorRoot)); ; ) {
        MOZ_ASSERT(iter != graph_.rpoEnd(), "Inconsistent dominator information");
        MBasicBlock *block = *iter++;
        
        if (!dominatorRoot->dominates(block))
            continue;

        
        
        MBasicBlock *header = block->isLoopBackedge() ? block->loopHeaderOfBackedge() : nullptr;

        if (block->isMarked()) {
            
            if (!visitUnreachableBlock(block))
                return false;
            ++numDiscarded;
        } else {
            
            if (!visitBlock(block, dominatorRoot))
                return false;
            ++numVisited;
        }

        
        
        if (!rerun_ && header && loopHasOptimizablePhi(header)) {
            JitSpew(JitSpew_GVN, "    Loop phi in block%u can now be optimized; will re-run GVN!",
                    header->id());
            rerun_ = true;
            remainingBlocks_.clear();
        }

        MOZ_ASSERT(numVisited <= dominatorRoot->numDominated() - numDiscarded,
                   "Visited blocks too many times");
        if (numVisited >= dominatorRoot->numDominated() - numDiscarded)
            break;
    }

    totalNumVisited_ += numVisited;
    values_.clear();
    return true;
}


bool
ValueNumberer::visitGraph()
{
    
    
    
    
    
    for (ReversePostorderIterator iter(graph_.rpoBegin()); ; ) {
        MOZ_ASSERT(iter != graph_.rpoEnd(), "Inconsistent dominator information");
        MBasicBlock *block = *iter;
        if (block->immediateDominator() == block) {
            if (!visitDominatorTree(block))
                return false;

            
            
            
            
            
            ++iter;
            if (block->isMarked()) {
                JitSpew(JitSpew_GVN, "      Discarding dominator root block%u",
                        block->id());
                MOZ_ASSERT(block->begin() == block->end(),
                           "Unreachable dominator tree root has instructions after tree walk");
                MOZ_ASSERT(block->phisEmpty(),
                           "Unreachable dominator tree root has phis after tree walk");
                graph_.removeBlock(block);
                blocksRemoved_ = true;
            }

            MOZ_ASSERT(totalNumVisited_ <= graph_.numBlocks(), "Visited blocks too many times");
            if (totalNumVisited_ >= graph_.numBlocks())
                break;
        } else {
            
            ++iter;
        }
    }
    totalNumVisited_ = 0;
    return true;
}

ValueNumberer::ValueNumberer(MIRGenerator *mir, MIRGraph &graph)
  : mir_(mir), graph_(graph),
    values_(graph.alloc()),
    deadDefs_(graph.alloc()),
    remainingBlocks_(graph.alloc()),
    nextDef_(nullptr),
    totalNumVisited_(0),
    rerun_(false),
    blocksRemoved_(false),
    updateAliasAnalysis_(false),
    dependenciesBroken_(false)
{}

bool
ValueNumberer::init()
{
    
    
    
    
    
    
    return values_.init();
}

bool
ValueNumberer::run(UpdateAliasAnalysisFlag updateAliasAnalysis)
{
    updateAliasAnalysis_ = updateAliasAnalysis == UpdateAliasAnalysis;

    JitSpew(JitSpew_GVN, "Running GVN on graph (with %llu blocks)",
            uint64_t(graph_.numBlocks()));

    
    
    
    
    int runs = 0;
    for (;;) {
        if (!visitGraph())
            return false;

        
        
        while (!remainingBlocks_.empty()) {
            MBasicBlock *block = remainingBlocks_.popCopy();
            if (!block->isDead() && IsDominatorRefined(block)) {
                JitSpew(JitSpew_GVN, "  Dominator for block%u can now be refined; will re-run GVN!",
                        block->id());
                rerun_ = true;
                remainingBlocks_.clear();
                break;
            }
        }

        if (blocksRemoved_) {
            if (!AccountForCFGChanges(mir_, graph_, dependenciesBroken_))
                return false;

            blocksRemoved_ = false;
            dependenciesBroken_ = false;
        }

        if (mir_->shouldCancel("GVN (outer loop)"))
            return false;

        
        if (!rerun_)
            break;

        rerun_ = false;

        
        
        
        
        
        ++runs;
        if (runs == 6) {
            JitSpew(JitSpew_GVN, "Re-run cutoff of %d reached. Terminating GVN!", runs);
            break;
        }

        JitSpew(JitSpew_GVN, "Re-running GVN on graph (run %d, now with %llu blocks)",
                runs, uint64_t(graph_.numBlocks()));
    }

    return true;
}
