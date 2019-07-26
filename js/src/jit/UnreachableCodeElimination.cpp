





#include "jit/UnreachableCodeElimination.h"

#include "jit/AliasAnalysis.h"
#include "jit/IonAnalysis.h"
#include "jit/MIRGenerator.h"
#include "jit/ValueNumbering.h"

using namespace js;
using namespace jit;

bool
UnreachableCodeElimination::analyze()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    if (!prunePointlessBranchesAndMarkReachableBlocks())
        return false;

    return removeUnmarkedBlocksAndCleanup();
}

bool
UnreachableCodeElimination::removeUnmarkedBlocks(size_t marked)
{
    marked_ = marked;
    return removeUnmarkedBlocksAndCleanup();
}

bool
UnreachableCodeElimination::removeUnmarkedBlocksAndCleanup()
{
    
    JS_ASSERT(marked_ <= graph_.numBlocks());
    if (marked_ == graph_.numBlocks()) {
        graph_.unmarkBlocks();
        return true;
    }

    
    if (!removeUnmarkedBlocksAndClearDominators())
        return false;
    graph_.unmarkBlocks();

    AssertGraphCoherency(graph_);

    IonSpewPass("UCE-mid-point");

    
    BuildDominatorTree(graph_);
    if (redundantPhis_ && !EliminatePhis(mir_, graph_, ConservativeObservability))
        return false;

    
    if (rerunAliasAnalysis_) {
        AliasAnalysis analysis(mir_, graph_);
        if (!analysis.analyze())
            return false;
    }

    
    
    if (rerunAliasAnalysis_ && js_IonOptions.gvn) {
        ValueNumberer gvn(mir_, graph_, js_IonOptions.gvnIsOptimistic);
        if (!gvn.clear() || !gvn.analyze())
            return false;
        IonSpewPass("GVN-after-UCE");
        AssertExtendedGraphCoherency(graph_);

        if (mir_->shouldCancel("GVN-after-UCE"))
            return false;
    }

    return true;
}

bool
UnreachableCodeElimination::enqueue(MBasicBlock *block, BlockList &list)
{
    if (block->isMarked())
        return true;

    block->mark();
    marked_++;
    return list.append(block);
}

MBasicBlock *
UnreachableCodeElimination::optimizableSuccessor(MBasicBlock *block)
{
    
    
    

    MControlInstruction *ins = block->lastIns();
    if (!ins->isTest())
        return NULL;

    MTest *testIns = ins->toTest();
    MDefinition *v = testIns->getOperand(0);
    if (!v->isConstant())
        return NULL;

    const Value &val = v->toConstant()->value();
    BranchDirection bdir = ToBoolean(val) ? TRUE_BRANCH : FALSE_BRANCH;
    return testIns->branchSuccessor(bdir);
}

bool
UnreachableCodeElimination::prunePointlessBranchesAndMarkReachableBlocks()
{
    BlockList worklist, optimizableBlocks;

    
    
    if (!enqueue(graph_.entryBlock(), worklist))
        return false;
    while (!worklist.empty()) {
        if (mir_->shouldCancel("Eliminate Unreachable Code"))
            return false;

        MBasicBlock *block = worklist.popCopy();

        
        
        if (MBasicBlock *succ = optimizableSuccessor(block)) {
            if (!optimizableBlocks.append(block))
                return false;
            if (!enqueue(succ, worklist))
                return false;
        } else {
            
            for (size_t i = 0; i < block->numSuccessors(); i++) {
                MBasicBlock *succ = block->getSuccessor(i);
                if (!enqueue(succ, worklist))
                    return false;
            }
        }
    }

    
    
    
    
    
    
    if (graph_.osrBlock()) {
        MBasicBlock *osrBlock = graph_.osrBlock();
        JS_ASSERT(!osrBlock->isMarked());
        if (!enqueue(osrBlock, worklist))
            return false;
        for (size_t i = 0; i < osrBlock->numSuccessors(); i++) {
            if (!osrBlock->getSuccessor(i)->isMarked()) {
                
                for (MBasicBlockIterator iter(graph_.begin()); iter != graph_.end(); iter++)
                    iter->mark();
                marked_ = graph_.numBlocks();
                return true;
            }
        }
    }

    
    
    for (uint32_t i = 0; i < optimizableBlocks.length(); i++) {
        MBasicBlock *block = optimizableBlocks[i];
        MBasicBlock *succ = optimizableSuccessor(block);
        JS_ASSERT(succ);

        MGoto *gotoIns = MGoto::New(succ);
        block->discardLastIns();
        block->end(gotoIns);
        MBasicBlock *successorWithPhis = block->successorWithPhis();
        if (successorWithPhis && successorWithPhis != succ)
            block->setSuccessorWithPhis(NULL, 0);
    }

    return true;
}

void
UnreachableCodeElimination::checkDependencyAndRemoveUsesFromUnmarkedBlocks(MDefinition *instr)
{
    
    
    if (instr->dependency() && !instr->dependency()->block()->isMarked())
        rerunAliasAnalysis_ = true;

    for (MUseIterator iter(instr->usesBegin()); iter != instr->usesEnd(); ) {
        if (!iter->consumer()->block()->isMarked()) {
            instr->setUseRemovedUnchecked();
            iter = instr->removeUse(iter);
        } else {
            iter++;
        }
    }
}

bool
UnreachableCodeElimination::removeUnmarkedBlocksAndClearDominators()
{
    
    
    
    

    size_t id = marked_;
    for (PostorderIterator iter(graph_.poBegin()); iter != graph_.poEnd();) {
        if (mir_->shouldCancel("Eliminate Unreachable Code"))
            return false;

        MBasicBlock *block = *iter;
        iter++;

        
        
        block->clearDominatorInfo();

        if (block->isMarked()) {
            block->setId(--id);
            for (MPhiIterator iter(block->phisBegin()); iter != block->phisEnd(); iter++)
                checkDependencyAndRemoveUsesFromUnmarkedBlocks(*iter);
            for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++)
                checkDependencyAndRemoveUsesFromUnmarkedBlocks(*iter);
        } else {
            if (block->numPredecessors() > 1) {
                
                
                
                for (size_t i = 0; i < block->numPredecessors(); i++)
                    block->getPredecessor(i)->setSuccessorWithPhis(NULL, 0);
            }

            if (block->isLoopBackedge()) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                block->loopHeaderOfBackedge()->clearLoopHeader();
            }

            for (size_t i = 0, c = block->numSuccessors(); i < c; i++) {
                MBasicBlock *succ = block->getSuccessor(i);
                if (succ->isMarked()) {
                    
                    succ->removePredecessor(block);

                    if (!redundantPhis_) {
                        for (MPhiIterator iter(succ->phisBegin()); iter != succ->phisEnd(); iter++) {
                            if (iter->operandIfRedundant()) {
                                redundantPhis_ = true;
                                break;
                            }
                        }
                    }
                }
            }

            
            
            
            
            for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
                if (iter->isCall()) {
                    MCall *call = iter->toCall();
                    for (size_t i = 0; i < call->numStackArgs(); i++) {
                        JS_ASSERT(call->getArg(i)->isPassArg());
                        JS_ASSERT(call->getArg(i)->hasOneDefUse());
                        MPassArg *arg = call->getArg(i)->toPassArg();
                        arg->replaceAllUsesWith(arg->getArgument());
                    }
                }
            }

            graph_.removeBlock(block);
        }
    }

    JS_ASSERT(id == 0);

    return true;
}
