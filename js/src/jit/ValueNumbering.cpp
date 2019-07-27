





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
IsDead(const MDefinition *def)
{
    return !def->hasUses() && DeadIfUnused(def);
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

    
    
    MOZ_ASSERT(old->dominates(now), "Refined dominator not dominated by old dominator");
    for (MBasicBlock *i = now; i != old; i = i->immediateDominator()) {
        if (!i->phisEmpty() || *i->begin() != i->lastIns())
            return true;
    }

    return false;
}


bool
ValueNumberer::discardDefsRecursively(MDefinition *def)
{
    MOZ_ASSERT(deadDefs_.empty(), "deadDefs_ not cleared");

    return discardDef(def) && processDeadDefs();
}



bool
ValueNumberer::releasePhiOperands(MPhi *phi, const MBasicBlock *phiBlock,
                                  UseRemovedOption useRemovedOption)
{
    
    for (int o = phi->numOperands() - 1; o >= 0; --o) {
        MDefinition *op = phi->getOperand(o);
        phi->removeOperand(o);
        if (IsDead(op) && !phiBlock->dominates(op->block())) {
            if (!deadDefs_.append(op))
                return false;
        } else {
            if (useRemovedOption == SetUseRemoved)
                op->setUseRemovedUnchecked();
        }
    }
    return true;
}



bool
ValueNumberer::releaseInsOperands(MInstruction *ins,
                                  UseRemovedOption useRemovedOption)
{
    for (size_t o = 0, e = ins->numOperands(); o < e; ++o) {
        MDefinition *op = ins->getOperand(o);
        ins->releaseOperand(o);
        if (IsDead(op)) {
            if (!deadDefs_.append(op))
                return false;
        } else {
            if (useRemovedOption == SetUseRemoved)
                op->setUseRemovedUnchecked();
        }
    }
    return true;
}


bool
ValueNumberer::discardDef(MDefinition *def,
                         UseRemovedOption useRemovedOption)
{
    JitSpew(JitSpew_GVN, "      Discarding %s%u", def->opName(), def->id());
    MOZ_ASSERT(IsDead(def), "Discarding non-dead definition");
    MOZ_ASSERT(!values_.has(def), "Discarding an instruction still in the set");

    if (def->isPhi()) {
        MPhi *phi = def->toPhi();
        MBasicBlock *phiBlock = phi->block();
        if (!releasePhiOperands(phi, phiBlock, useRemovedOption))
             return false;
        MPhiIterator at(phiBlock->phisBegin(phi));
        phiBlock->discardPhiAt(at);
    } else {
        MInstruction *ins = def->toInstruction();
        if (!releaseInsOperands(ins, useRemovedOption))
             return false;
        ins->block()->discardIgnoreOperands(ins);
    }
    return true;
}


bool
ValueNumberer::processDeadDefs()
{
    while (!deadDefs_.empty()) {
        MDefinition *def = deadDefs_.popCopy();

        values_.forget(def);
        if (!discardDef(def))
            return false;
    }
    return true;
}


bool
ValueNumberer::removePredecessor(MBasicBlock *block, MBasicBlock *pred)
{
    bool isUnreachableLoop = false;
    if (block->isLoopHeader()) {
        if (block->loopPredecessor() == pred) {
            
            isUnreachableLoop = true;
            JitSpew(JitSpew_GVN, "      Loop with header block%u is no longer reachable",
                    block->id());
#ifdef DEBUG
        } else if (block->hasUniqueBackedge() && block->backedge() == pred) {
            JitSpew(JitSpew_GVN, "      Loop with header block%u is no longer a loop",
                    block->id());
#endif
        }
    }

    
    
    
    
    block->removePredecessor(pred);
    return block->numPredecessors() == 0 || isUnreachableLoop;
}


bool
ValueNumberer::removeBlocksRecursively(MBasicBlock *start, const MBasicBlock *dominatorRoot)
{
    MOZ_ASSERT(start != graph_.entryBlock(), "Removing normal entry block");
    MOZ_ASSERT(start != graph_.osrBlock(), "Removing OSR entry block");

    
    
    
    MBasicBlock *parent = start->immediateDominator();
    if (parent != start)
        parent->removeImmediatelyDominatedBlock(start);

    if (!unreachableBlocks_.append(start))
        return false;
    do {
        MBasicBlock *block = unreachableBlocks_.popCopy();
        if (block->isDead())
            continue;

        
        for (size_t i = 0, e = block->numSuccessors(); i < e; ++i) {
            MBasicBlock *succ = block->getSuccessor(i);
            if (succ->isDead())
                continue;
            if (removePredecessor(succ, block)) {
                if (!unreachableBlocks_.append(succ))
                    return false;
            } else if (!rerun_) {
                if (!remainingBlocks_.append(succ))
                    return false;
            }
        }

#ifdef DEBUG
        JitSpew(JitSpew_GVN, "    Discarding block%u%s%s%s", block->id(),
                block->isLoopHeader() ? " (loop header)" : "",
                block->isSplitEdge() ? " (split edge)" : "",
                block->immediateDominator() == block ? " (dominator root)" : "");
        for (MDefinitionIterator iter(block); iter; ++iter) {
            MDefinition *def = *iter;
            JitSpew(JitSpew_GVN, "      Discarding %s%u", def->opName(), def->id());
        }
        MControlInstruction *control = block->lastIns();
        JitSpew(JitSpew_GVN, "      Discarding %s%u", control->opName(), control->id());
#endif

        
        if (dominatorRoot->dominates(block))
            ++numBlocksDiscarded_;

        
        
        
        
        graph_.removeBlockIncludingPhis(block);
        blocksRemoved_ = true;
    } while (!unreachableBlocks_.empty());

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
ValueNumberer::loopHasOptimizablePhi(MBasicBlock *backedge) const
{
    
    
    MBasicBlock *header = backedge->loopHeaderOfBackedge();
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
    if (dep != nullptr && dep->block()->isDead()) {
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
                
                
                mozilla::DebugOnly<bool> r = discardDef(def, DontSetUseRemoved);
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
            if (removePredecessor(succ, block)) {
                if (!removeBlocksRecursively(succ, dominatorRoot))
                    return false;
            } else if (!rerun_) {
                if (!remainingBlocks_.append(succ))
                    return false;
            }
        }
    }

    if (!releaseInsOperands(control))
        return false;
    block->discardIgnoreOperands(control);
    block->end(newControl);
    return processDeadDefs();
}


bool
ValueNumberer::visitBlock(MBasicBlock *block, const MBasicBlock *dominatorRoot)
{
    MOZ_ASSERT(!block->unreachable(), "Blocks marked unreachable during GVN");
    MOZ_ASSERT(!block->isDead(), "Block to visit is already dead");

    JitSpew(JitSpew_GVN, "    Visiting block%u", block->id());

    
    for (MDefinitionIterator iter(block); iter; ) {
        MDefinition *def = *iter++;

        
        if (IsDead(def)) {
            if (!discardDefsRecursively(def))
                return false;
            continue;
        }

        if (!visitDefinition(def))
            return false;
    }

    return visitControlInstruction(block, dominatorRoot);
}


bool
ValueNumberer::visitDominatorTree(MBasicBlock *dominatorRoot, size_t *totalNumVisited)
{
    JitSpew(JitSpew_GVN, "  Visiting dominator tree (with %llu blocks) rooted at block%u%s",
            uint64_t(dominatorRoot->numDominated()), dominatorRoot->id(),
            dominatorRoot == graph_.entryBlock() ? " (normal entry block)" :
            dominatorRoot == graph_.osrBlock() ? " (OSR entry block)" :
            " (normal entry and OSR entry merge point)");
    MOZ_ASSERT(numBlocksDiscarded_ == 0, "numBlocksDiscarded_ wasn't reset");
    MOZ_ASSERT(dominatorRoot->immediateDominator() == dominatorRoot,
            "root is not a dominator tree root");

    
    
    
    
    size_t numVisited = 0;
    for (ReversePostorderIterator iter(graph_.rpoBegin(dominatorRoot)); ; ++iter) {
        MOZ_ASSERT(iter != graph_.rpoEnd(), "Inconsistent dominator information");
        MBasicBlock *block = *iter;
        
        if (!dominatorRoot->dominates(block))
            continue;
        
        if (!visitBlock(block, dominatorRoot))
            return false;
        
        if (!rerun_ && block->isLoopBackedge() && loopHasOptimizablePhi(block)) {
            JitSpew(JitSpew_GVN, "    Loop phi in block%u can now be optimized; will re-run GVN!",
                    block->loopHeaderOfBackedge()->id());
            rerun_ = true;
            remainingBlocks_.clear();
        }
        ++numVisited;
        MOZ_ASSERT(numVisited <= dominatorRoot->numDominated() - numBlocksDiscarded_,
                   "Visited blocks too many times");
        if (numVisited >= dominatorRoot->numDominated() - numBlocksDiscarded_)
            break;
    }

    *totalNumVisited += numVisited;
    values_.clear();
    numBlocksDiscarded_ = 0;
    return true;
}


bool
ValueNumberer::visitGraph()
{
    
    
    
    
    
    size_t totalNumVisited = 0;
    for (ReversePostorderIterator iter(graph_.rpoBegin()); ; ++iter) {
         MBasicBlock *block = *iter;
         if (block->immediateDominator() == block) {
             if (!visitDominatorTree(block, &totalNumVisited))
                 return false;
             MOZ_ASSERT(totalNumVisited <= graph_.numBlocks(), "Visited blocks too many times");
             if (totalNumVisited >= graph_.numBlocks())
                 break;
         }
         MOZ_ASSERT(iter != graph_.rpoEnd(), "Inconsistent dominator information");
    }
    return true;
}

ValueNumberer::ValueNumberer(MIRGenerator *mir, MIRGraph &graph)
  : mir_(mir), graph_(graph),
    values_(graph.alloc()),
    deadDefs_(graph.alloc()),
    unreachableBlocks_(graph.alloc()),
    remainingBlocks_(graph.alloc()),
    numBlocksDiscarded_(0),
    rerun_(false),
    blocksRemoved_(false),
    updateAliasAnalysis_(false),
    dependenciesBroken_(false)
{}

bool
ValueNumberer::run(UpdateAliasAnalysisFlag updateAliasAnalysis)
{
    updateAliasAnalysis_ = updateAliasAnalysis == UpdateAliasAnalysis;

    
    
    
    
    
    
    if (!values_.init())
        return false;

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

        JitSpew(JitSpew_GVN, "Re-running GVN on graph (run %d, now with %llu blocks)",
                runs, uint64_t(graph_.numBlocks()));
        rerun_ = false;

        
        
        
        
        
        ++runs;
        if (runs == 6) {
            JitSpew(JitSpew_GVN, "Re-run cutoff reached. Terminating GVN!");
            break;
        }
    }

    return true;
}
