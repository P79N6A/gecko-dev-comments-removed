






#include "UnreachableCodeElimination.h"
#include "IonAnalysis.h"
#include "AliasAnalysis.h"

using namespace js;
using namespace ion;

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

    IonSpewPass("UCEMidPoint");

    
    BuildDominatorTree(graph_);
    if (redundantPhis_ && !EliminatePhis(mir_, graph_, ConservativeObservability))
        return false;

    
    if (rerunAliasAnalysis_) {
        AliasAnalysis analysis(mir_, graph_);
        if (!analysis.analyze())
            return false;
    }

    return true;
}

bool
UnreachableCodeElimination::prunePointlessBranchesAndMarkReachableBlocks()
{
    Vector<MBasicBlock *, 16, SystemAllocPolicy> worklist;

    
    MBasicBlock *entries[] = { graph_.entryBlock(), graph_.osrBlock() };
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        if (entries[i]) {
            entries[i]->mark();
            marked_++;
            if (!worklist.append(entries[i]))
                return false;
        }
    }

    
    while (!worklist.empty()) {
        if (mir_->shouldCancel("Eliminate Unreachable Code"))
            return false;

        MBasicBlock *block = worklist.popCopy();
        MControlInstruction *ins = block->lastIns();

        
        if (ins->isTest()) {
            MTest *testIns = ins->toTest();
            MDefinition *v = testIns->getOperand(0);
            if (v->isConstant()) {
                const Value &val = v->toConstant()->value();
                BranchDirection bdir = ToBoolean(val) ? TRUE_BRANCH : FALSE_BRANCH;
                MBasicBlock *succ = testIns->branchSuccessor(bdir);
                MGoto *gotoIns = MGoto::New(succ);
                block->discardLastIns();
                block->end(gotoIns);
                MBasicBlock *successorWithPhis = block->successorWithPhis();
                if (successorWithPhis && successorWithPhis != succ)
                    block->setSuccessorWithPhis(NULL, 0);
            }
        }

        for (size_t i = 0; i < block->numSuccessors(); i++) {
            MBasicBlock *succ = block->getSuccessor(i);
            if (!succ->isMarked()) {
                succ->mark();
                marked_++;
                if (!worklist.append(succ))
                    return false;
            }
        }
    }
    return true;
}

void
UnreachableCodeElimination::checkDependencyAndRemoveUsesFromUnmarkedBlocks(MDefinition *instr)
{
    
    
    if (instr->dependency() && !instr->dependency()->block()->isMarked())
        rerunAliasAnalysis_ = true;

    for (MUseIterator iter(instr->usesBegin()); iter != instr->usesEnd(); ) {
        if (!iter->consumer()->block()->isMarked())
            iter = instr->removeUse(iter);
        else
            iter++;
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

            graph_.removeBlock(block);
        }
    }

    JS_ASSERT(id == 0);

    return true;
}
