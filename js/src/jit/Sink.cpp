





#include "jit/Sink.h"

#include "mozilla/Vector.h"

#include "jit/IonAnalysis.h"
#include "jit/JitSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"

namespace js {
namespace jit {





static MBasicBlock *
CommonDominator(MBasicBlock *commonDominator, MBasicBlock *defBlock)
{
    
    
    if (!commonDominator)
        return defBlock;

    
    
    while (!commonDominator->dominates(defBlock)) {
        MBasicBlock *nextBlock = commonDominator->immediateDominator();
        
        
        MOZ_ASSERT(commonDominator != nextBlock);
        commonDominator = nextBlock;
    }

    return commonDominator;
}

bool
Sink(MIRGenerator *mir, MIRGraph &graph)
{
    TempAllocator &alloc = graph.alloc();
    bool sinkEnabled = mir->optimizationInfo().sinkEnabled();

    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Sink"))
            return false;

        for (MInstructionReverseIterator iter = block->rbegin(); iter != block->rend(); ) {
            MInstruction *ins = *iter++;

            
            
            if (ins->isGuard() || ins->isGuardRangeBailouts() ||
                ins->isRecoveredOnBailout() || !ins->canRecoverOnBailout())
            {
                continue;
            }

            
            
            bool hasLiveUses = false;
            bool hasUses = false;
            MBasicBlock *usesDominator = nullptr;
            for (MUseIterator i(ins->usesBegin()), e(ins->usesEnd()); i != e; i++) {
                hasUses = true;
                MNode *consumerNode = (*i)->consumer();
                if (consumerNode->isResumePoint())
                    continue;

                MDefinition *consumer = consumerNode->toDefinition();
                if (consumer->isRecoveredOnBailout())
                    continue;

                hasLiveUses = true;

                
                
                MBasicBlock *consumerBlock = consumer->block();
                if (consumer->isPhi())
                    consumerBlock = consumerBlock->getPredecessor(consumer->indexOf(*i));

                usesDominator = CommonDominator(usesDominator, consumerBlock);
                if (usesDominator == *block)
                    break;
            }

            
            if (!hasUses)
                continue;

            
            
            if (!hasLiveUses) {
                MOZ_ASSERT(!usesDominator);
                ins->setRecoveredOnBailout();
                JitSpewDef(JitSpew_Sink, "  No live uses, recover the instruction on bailout\n", ins);
                continue;
            }

            
            
            
            
            if (!sinkEnabled)
                continue;

            
            
            
            if (ins->isEffectful())
                continue;

            
            
            
            
            while (block->loopDepth() < usesDominator->loopDepth()) {
                MOZ_ASSERT(usesDominator != usesDominator->immediateDominator());
                usesDominator = usesDominator->immediateDominator();
            }

            
            
            
            
            MBasicBlock *lastJoin = usesDominator;
            while (*block != lastJoin && lastJoin->numPredecessors() == 1) {
                MOZ_ASSERT(lastJoin != lastJoin->immediateDominator());
                MBasicBlock *next = lastJoin->immediateDominator();
                if (next->numSuccessors() > 1)
                    break;
                lastJoin = next;
            }
            if (*block == lastJoin)
                continue;

            
            
            
            if (!usesDominator || usesDominator == *block)
                continue;

            
            
            

            
            
            
            if (!ins->canClone())
                continue;

            
            
            
            
            if (!usesDominator->entryResumePoint() && usesDominator->numPredecessors() != 1)
                continue;

            JitSpewDef(JitSpew_Sink, "  Can Clone & Recover, sink instruction\n", ins);
            JitSpew(JitSpew_Sink, "  into Block %u", usesDominator->id());

            
            MDefinitionVector operands(alloc);
            for (size_t i = 0, end = ins->numOperands(); i < end; i++) {
                if (!operands.append(ins->getOperand(i)))
                    return false;
            }

            MInstruction *clone = ins->clone(alloc, operands);
            ins->block()->insertBefore(ins, clone);
            clone->setRecoveredOnBailout();

            
            
            
            MResumePoint *entry = usesDominator->entryResumePoint();

            
            
            
            for (MUseIterator i(ins->usesBegin()), e(ins->usesEnd()); i != e; ) {
                MUse *use = *i++;
                MNode *consumer = use->consumer();

                
                
                
                MBasicBlock *consumerBlock = consumer->block();
                if (consumer->isDefinition() && consumer->toDefinition()->isPhi()) {
                    consumerBlock = consumerBlock->getPredecessor(
                        consumer->toDefinition()->toPhi()->indexOf(use));
                }

                
                
                
                if (usesDominator->dominates(consumerBlock) &&
                    (!consumer->isResumePoint() || consumer->toResumePoint() != entry))
                {
                    continue;
                }

                use->replaceProducer(clone);
            }

            
            
            
            if (ins->resumePoint())
                ins->clearResumePoint();

            
            
            
            MInstruction *at = usesDominator->safeInsertTop(nullptr, MBasicBlock::IgnoreRecover);
            block->moveBefore(at, ins);
        }
    }

    return true;
}

}
}
