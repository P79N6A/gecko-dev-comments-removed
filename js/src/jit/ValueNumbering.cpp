





#include "jit/ValueNumbering.h"

#include "jit/AliasAnalysis.h"
#include "jit/IonAnalysis.h"
#include "jit/IonSpewer.h"
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

    return k->congruentTo(l); 
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
ValueNumberer::VisibleValues::insert(AddPtr p, MDefinition *def)
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




static bool
WillBecomeDead(const MDefinition *def)
{
    return def->hasOneUse() && DeadIfUnused(def);
}


static void
ReplaceAllUsesWith(MDefinition *from, MDefinition *to)
{
    MOZ_ASSERT(from != to, "GVN shouldn't try to replace a value with itself");
    MOZ_ASSERT(from->type() == to->type(), "Def replacement has different type");

    from->replaceAllUsesWith(to);
}


static bool
HasSuccessor(const MControlInstruction *newControl, const MBasicBlock *succ)
{
    for (size_t i = 0, e = newControl->numSuccessors(); i != e; ++i) {
        if (newControl->getSuccessor(i) == succ)
            return true;
    }
    return false;
}




static MBasicBlock *
ComputeNewDominator(MBasicBlock *block, MBasicBlock *old)
{
    MBasicBlock *now = block->getPredecessor(0);
    for (size_t i = 1, e = block->numPredecessors(); i != e; ++i) {
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

    
    
    MOZ_ASSERT(old->dominates(now), "Refined dominator not dominated by old dominator");
    for (MBasicBlock *i = now; i != old; i = i->immediateDominator()) {
        if (!i->phisEmpty() || *i->begin() != i->lastIns())
            return true;
    }

    return false;
}



bool
ValueNumberer::deleteDefsRecursively(MDefinition *def)
{
    return deleteDef(def) && processDeadDefs();
}



bool
ValueNumberer::pushDeadPhiOperands(MPhi *phi, const MBasicBlock *phiBlock)
{
    for (size_t o = 0, e = phi->numOperands(); o != e; ++o) {
        MDefinition *op = phi->getOperand(o);
        if (WillBecomeDead(op) && !op->isInWorklist() &&
            !phiBlock->dominates(phiBlock->getPredecessor(o)))
        {
            op->setInWorklist();
            if (!deadDefs_.append(op))
                return false;
        } else {
           op->setUseRemovedUnchecked();
        }
    }
    return true;
}


bool
ValueNumberer::pushDeadInsOperands(MInstruction *ins)
{
    for (size_t o = 0, e = ins->numOperands(); o != e; ++o) {
        MDefinition *op = ins->getOperand(o);
        if (WillBecomeDead(op) && !op->isInWorklist()) {
            op->setInWorklist();
            if (!deadDefs_.append(op))
                return false;
        } else {
           op->setUseRemovedUnchecked();
        }
    }
    return true;
}

bool
ValueNumberer::deleteDef(MDefinition *def)
{
    IonSpew(IonSpew_GVN, "    Deleting %s%u", def->opName(), def->id());
    MOZ_ASSERT(IsDead(def), "Deleting non-dead definition");
    MOZ_ASSERT(!values_.has(def), "Deleting an instruction still in the set");

    if (def->isPhi()) {
        MPhi *phi = def->toPhi();
        MBasicBlock *phiBlock = phi->block();
        if (!pushDeadPhiOperands(phi, phiBlock))
             return false;
        MPhiIterator at(phiBlock->phisBegin(phi));
        phiBlock->discardPhiAt(at);
    } else {
        MInstruction *ins = def->toInstruction();
        if (!pushDeadInsOperands(ins))
             return false;
        ins->block()->discard(ins);
    }
    return true;
}


bool
ValueNumberer::processDeadDefs()
{
    while (!deadDefs_.empty()) {
        MDefinition *def = deadDefs_.popCopy();
        MOZ_ASSERT(def->isInWorklist(), "Deleting value not on the worklist");

        values_.forget(def);
        if (!deleteDef(def))
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
            IonSpew(IonSpew_GVN, "    Loop with header block%u is no longer reachable", block->id());
#ifdef DEBUG
        } else if (block->hasUniqueBackedge() && block->backedge() == pred) {
            IonSpew(IonSpew_GVN, "    Loop with header block%u is no longer a loop", block->id());
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

        
        for (size_t i = 0, e = block->numSuccessors(); i != e; ++i) {
            MBasicBlock *succ = block->getSuccessor(i);
            if (!succ->isDead()) {
                if (removePredecessor(succ, block)) {
                    if (!unreachableBlocks_.append(succ))
                        return false;
                } else if (!rerun_) {
                    if (!remainingBlocks_.append(succ))
                        return false;
                }
            }
        }

#ifdef DEBUG
        IonSpew(IonSpew_GVN, "    Deleting block%u%s%s%s", block->id(),
                block->isLoopHeader() ? " (loop header)" : "",
                block->isSplitEdge() ? " (split edge)" : "",
                block->immediateDominator() == block ? " (dominator root)" : "");
        for (MDefinitionIterator iter(block); iter; iter++) {
            MDefinition *def = *iter;
            IonSpew(IonSpew_GVN, "      Deleting %s%u", def->opName(), def->id());
        }
        MControlInstruction *control = block->lastIns();
        IonSpew(IonSpew_GVN, "      Deleting %s%u", control->opName(), control->id());
#endif

        
        if (dominatorRoot->dominates(block))
            ++numBlocksDeleted_;

        
        
        
        
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
                
                MOZ_ASSERT(!rep->isInWorklist(), "Dead value in set");
                return rep;
            }

            
            
            values_.overwrite(p, def);
        } else {
            
            if (!values_.insert(p, def))
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
    
    MDefinition *sim = simplified(def);
    if (sim != def) {
        if (sim == nullptr)
            return false;

        
        if (sim->block() == nullptr)
            def->block()->insertAfter(def->toInstruction(), sim->toInstruction());

        IonSpew(IonSpew_GVN, "    Folded %s%u to %s%u",
                def->opName(), def->id(), sim->opName(), sim->id());
        ReplaceAllUsesWith(def, sim);

        
        
        
        def->setNotGuardUnchecked();

        if (IsDead(def) && !deleteDefsRecursively(def))
            return false;
        def = sim;
    }

    
    MDefinition *rep = leader(def);
    if (rep != def) {
        if (rep == nullptr)
            return false;
        if (rep->updateForReplacement(def)) {
            IonSpew(IonSpew_GVN,
                    "    Replacing %s%u with %s%u",
                    def->opName(), def->id(), rep->opName(), rep->id());
            ReplaceAllUsesWith(def, rep);

            
            
            
            def->setNotGuardUnchecked();

            if (IsDead(def) && !deleteDefsRecursively(def))
                return false;
            def = rep;
        }
    }

    
    
    if (updateAliasAnalysis_ && !dependenciesBroken_) {
        const MDefinition *dep = def->dependency();
        if (dep != nullptr && dep->block()->isDead()) {
            IonSpew(IonSpew_GVN, "    AliasAnalysis invalidated; will recompute!");
            dependenciesBroken_ = true;
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
    IonSpew(IonSpew_GVN, "    Folded control instruction %s%u to %s%u",
            control->opName(), control->id(), newControl->opName(), graph_.getNumInstructionIds());

    
    
    size_t oldNumSuccs = control->numSuccessors();
    size_t newNumSuccs = newControl->numSuccessors();
    if (newNumSuccs != oldNumSuccs) {
        MOZ_ASSERT(newNumSuccs < oldNumSuccs, "New control instruction has too many successors");
        for (size_t i = 0; i != oldNumSuccs; ++i) {
            MBasicBlock *succ = control->getSuccessor(i);
            if (!HasSuccessor(newControl, succ)) {
                if (removePredecessor(succ, block)) {
                    if (!removeBlocksRecursively(succ, dominatorRoot))
                        return false;
                } else if (!rerun_) {
                    if (!remainingBlocks_.append(succ))
                        return false;
                }
            }
        }
    }

    if (!pushDeadInsOperands(control))
        return false;
    block->discardLastIns();
    block->end(newControl);
    return processDeadDefs();
}


bool
ValueNumberer::visitBlock(MBasicBlock *block, const MBasicBlock *dominatorRoot)
{
    MOZ_ASSERT(!block->unreachable(), "Blocks marked unreachable during GVN");
    MOZ_ASSERT(!block->isDead(), "Block to visit is already dead");

    
    for (MDefinitionIterator iter(block); iter; ) {
        MDefinition *def = *iter++;

        
        if (IsDead(def)) {
            if (!deleteDefsRecursively(def))
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
    IonSpew(IonSpew_GVN, "  Visiting dominator tree (with %llu blocks) rooted at block%u%s",
            uint64_t(dominatorRoot->numDominated()), dominatorRoot->id(),
            dominatorRoot == graph_.entryBlock() ? " (normal entry block)" :
            dominatorRoot == graph_.osrBlock() ? " (OSR entry block)" :
            " (normal entry and OSR entry merge point)");
    MOZ_ASSERT(numBlocksDeleted_ == 0, "numBlocksDeleted_ wasn't reset");
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
            IonSpew(IonSpew_GVN, "    Loop phi in block%u can now be optimized; will re-run GVN!",
                    block->id());
            rerun_ = true;
            remainingBlocks_.clear();
        }
        ++numVisited;
        MOZ_ASSERT(numVisited <= dominatorRoot->numDominated() - numBlocksDeleted_,
                   "Visited blocks too many times");
        if (numVisited >= dominatorRoot->numDominated() - numBlocksDeleted_)
            break;
    }

    *totalNumVisited += numVisited;
    values_.clear();
    numBlocksDeleted_ = 0;
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
    numBlocksDeleted_(0),
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

    IonSpew(IonSpew_GVN, "Running GVN on graph (with %llu blocks)",
            uint64_t(graph_.numBlocks()));

    
    
    
    int runs = 0;
    for (;;) {
        if (!visitGraph())
            return false;

        
        
        while (!remainingBlocks_.empty()) {
            MBasicBlock *block = remainingBlocks_.popCopy();
            if (!block->isDead() && IsDominatorRefined(block)) {
                IonSpew(IonSpew_GVN, "  Dominator for block%u can now be refined; will re-run GVN!",
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

        IonSpew(IonSpew_GVN, "Re-running GVN on graph (run %d, now with %llu blocks)",
                runs, uint64_t(graph_.numBlocks()));
        rerun_ = false;

        
        
        
        
        
        ++runs;
        if (runs == 6) {
            IonSpew(IonSpew_GVN, "Re-run cutoff reached. Terminating GVN!");
            break;
        }
    }

    return true;
}
