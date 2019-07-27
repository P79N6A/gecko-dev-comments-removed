





#include "jit/ScalarReplacement.h"

#include "mozilla/Vector.h"

#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"

namespace js {
namespace jit {



static void
ReplaceResumePointOperands(MResumePoint *resumePoint, MDefinition *object, MDefinition *state)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (MResumePoint *rp = resumePoint; rp; rp = rp->caller()) {
        for (size_t op = 0; op < rp->numOperands(); op++) {
            if (rp->getOperand(op) == object) {
                rp->replaceOperand(op, state);

                
                
                
                
                MOZ_ASSERT_IF(rp != resumePoint, state->block()->dominates(rp->block()));
            }
        }
    }
}






static bool
IsObjectEscaped(MInstruction *ins)
{
    MOZ_ASSERT(ins->type() == MIRType_Object);

    
    
    
    for (MUseIterator i(ins->usesBegin()); i != ins->usesEnd(); i++) {
        MNode *consumer = (*i)->consumer();
        if (!consumer->isDefinition()) {
            
            if (consumer->toResumePoint()->isObservableOperand(*i))
                return true;
            continue;
        }

        MDefinition *def = consumer->toDefinition();
        switch (def->op()) {
          case MDefinition::Op_StoreFixedSlot:
          case MDefinition::Op_LoadFixedSlot:
            
            if (def->indexOf(*i) == 0)
                break;
            return true;
          default:
            return true;
        }
    }

    return false;
}

typedef MObjectState BlockState;
typedef Vector<BlockState *, 8, SystemAllocPolicy> GraphState;







static bool
ScalarReplacementOfObject(MIRGenerator *mir, MIRGraph &graph, GraphState &states,
                          MInstruction *obj)
{
    
    
    if (!states.appendN(nullptr, graph.numBlocks()))
        return false;

    
    MBasicBlock *objBlock = obj->block();
    MConstant *undefinedVal = MConstant::New(graph.alloc(), UndefinedValue());
    objBlock->insertBefore(obj, undefinedVal);
    states[objBlock->id()] = BlockState::New(graph.alloc(), obj, undefinedVal);

    
    for (ReversePostorderIterator block = graph.rpoBegin(obj->block()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Scalar Replacement of Object"))
            return false;

        BlockState *state = states[block->id()];
        if (!state) {
            MOZ_ASSERT(!objBlock->dominates(*block));
            continue;
        }

        
        
        if (*block == objBlock)
            objBlock->insertAfter(obj, state);
        else if (block->numPredecessors() > 1)
            block->insertBefore(*block->begin(), state);
        else
            MOZ_ASSERT(state->block()->dominates(*block));

        
        ReplaceResumePointOperands(block->entryResumePoint(), obj, state);

        for (MDefinitionIterator ins(*block); ins; ) {
            switch (ins->op()) {
              case MDefinition::Op_ObjectState: {
                ins++;
                continue;
              }

              case MDefinition::Op_LoadFixedSlot: {
                MLoadFixedSlot *def = ins->toLoadFixedSlot();

                
                if (def->object() != obj)
                    break;

                
                ins->replaceAllUsesWith(state->getFixedSlot(def->slot()));

                
                ins = block->discardDefAt(ins);
                continue;
              }

              case MDefinition::Op_StoreFixedSlot: {
                MStoreFixedSlot *def = ins->toStoreFixedSlot();

                
                if (def->object() != obj)
                    break;

                
                state = BlockState::Copy(graph.alloc(), state);
                state->setFixedSlot(def->slot(), def->value());
                block->insertBefore(ins->toInstruction(), state);

                
                ins = block->discardDefAt(ins);
                continue;
              }

              default:
                break;
            }

            
            if (ins->isInstruction())
                ReplaceResumePointOperands(ins->toInstruction()->resumePoint(), obj, state);

            ins++;
        }

        
        
        for (size_t s = 0; s < block->numSuccessors(); s++) {
            MBasicBlock *succ = block->getSuccessor(s);
            BlockState *succState = states[succ->id()];

            
            
            if (!succState) {
                
                
                
                
                
                
                if (!objBlock->dominates(succ))
                    continue;

                if (succ->numPredecessors() > 1) {
                    succState = states[succ->id()] = BlockState::Copy(graph.alloc(), state);
                    size_t numPreds = succ->numPredecessors();
                    for (size_t slot = 0; slot < state->numSlots(); slot++) {
                        MPhi *phi = MPhi::New(graph.alloc());
                        if (!phi->reserveLength(numPreds))
                            return false;

                        
                        
                        for (size_t p = 0; p < numPreds; p++)
                            phi->addInput(undefinedVal);

                        
                        succ->addPhi(phi);
                        succState->setSlot(slot, phi);
                    }
                } else {
                    succState = states[succ->id()] = state;
                }
            }

            if (succ->numPredecessors() > 1) {
                
                
                
                
                size_t numPreds = succ->numPredecessors();
                for (size_t p = 0; p < numPreds; p++) {
                    if (succ->getPredecessor(p) != *block)
                        continue;

                    
                    
                    for (size_t slot = 0; slot < state->numSlots(); slot++) {
                        MPhi *phi = succState->getSlot(slot)->toPhi();
                        phi->replaceOperand(p, state->getSlot(slot));
                    }
                }
            }
        }
    }

    MOZ_ASSERT(!obj->hasLiveDefUses());
    obj->setRecoveredOnBailout();
    states.clear();
    return true;
}

bool
ScalarReplacement(MIRGenerator *mir, MIRGraph &graph)
{
    GraphState objectStates;
    for (ReversePostorderIterator block = graph.rpoBegin(); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Scalar Replacement (main loop)"))
            return false;

        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            if (ins->isNewObject() && !IsObjectEscaped(*ins)) {
                if (!ScalarReplacementOfObject(mir, graph, objectStates, *ins))
                    return false;
            }
        }
    }

    return true;
}

} 
} 
