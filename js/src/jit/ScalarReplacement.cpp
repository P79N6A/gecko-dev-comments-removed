





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
    MOZ_ASSERT(ins->isNewObject() || ins->isGuardShape());

    
    
    
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

          case MDefinition::Op_Slots: {
#ifdef DEBUG
            
            MSlots *ins = def->toSlots();
            MOZ_ASSERT(ins->object() != 0);
            for (MUseIterator i(ins->usesBegin()); i != ins->usesEnd(); i++) {
                
                
                MDefinition *def = (*i)->consumer()->toDefinition();
                MOZ_ASSERT(def->op() == MDefinition::Op_StoreSlot ||
                           def->op() == MDefinition::Op_LoadSlot);
            }
#endif
            break;
          }

          case MDefinition::Op_GuardShape: {
            MGuardShape *guard = def->toGuardShape();
            MOZ_ASSERT(!ins->isGuardShape());
            if (ins->toNewObject()->templateObject()->lastProperty() != guard->shape())
                return true;
            if (IsObjectEscaped(def->toInstruction()))
                return true;
            break;
          }
          default:
            return true;
        }
    }

    return false;
}

struct ObjectTrait {
    typedef MObjectState BlockState;
    typedef Vector<BlockState *, 8, SystemAllocPolicy> GraphState;
};







static bool
ScalarReplacementOfObject(MIRGenerator *mir, MIRGraph &graph,
                          ObjectTrait::GraphState &states,
                          MInstruction *obj)
{
    typedef ObjectTrait::BlockState BlockState;

    
    
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

              case MDefinition::Op_GuardShape: {
                MGuardShape *def = ins->toGuardShape();

                
                if (def->obj() != obj)
                    break;

                
                ins->replaceAllUsesWith(obj);

                
                ins = block->discardDefAt(ins);
                continue;
              }

              case MDefinition::Op_LoadSlot: {
                MLoadSlot *def = ins->toLoadSlot();

                
                MSlots *slots = def->slots()->toSlots();
                if (slots->object() != obj) {
                    
                    MOZ_ASSERT(!slots->object()->isGuardShape() || slots->object()->toGuardShape()->obj() != obj);
                    break;
                }

                
                ins->replaceAllUsesWith(state->getDynamicSlot(def->slot()));

                
                ins = block->discardDefAt(ins);
                if (!slots->hasLiveDefUses())
                    slots->block()->discard(slots);
                continue;
              }

              case MDefinition::Op_StoreSlot: {
                MStoreSlot *def = ins->toStoreSlot();

                
                MSlots *slots = def->slots()->toSlots();
                if (slots->object() != obj) {
                    
                    MOZ_ASSERT(!slots->object()->isGuardShape() || slots->object()->toGuardShape()->obj() != obj);
                    break;
                }

                
                state = BlockState::Copy(graph.alloc(), state);
                state->setDynamicSlot(def->slot(), def->value());
                block->insertBefore(ins->toInstruction(), state);

                
                ins = block->discardDefAt(ins);
                if (!slots->hasLiveDefUses())
                    slots->block()->discard(slots);
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

static bool
IndexOf(MDefinition *ins, int32_t *res)
{
    MOZ_ASSERT(ins->isLoadElement() || ins->isStoreElement());
    MDefinition *indexDef = ins->getOperand(1); 
    if (indexDef->isBoundsCheck())
        indexDef = indexDef->toBoundsCheck()->index();
    if (indexDef->isToInt32())
        indexDef = indexDef->toToInt32()->getOperand(0);
    if (!indexDef->isConstant())
        return false;

    Value index = indexDef->toConstant()->value();
    if (!index.isInt32())
        return false;
    *res = index.toInt32();
    return true;
}







static bool
IsArrayEscaped(MInstruction *ins)
{
    MOZ_ASSERT(ins->type() == MIRType_Object);
    MOZ_ASSERT(ins->isNewArray());
    uint32_t count = ins->toNewArray()->count();

    
    
    if (!ins->toNewArray()->isAllocating() || count >= 16)
        return true;

    
    
    
    for (MUseIterator i(ins->usesBegin()); i != ins->usesEnd(); i++) {
        MNode *consumer = (*i)->consumer();
        if (!consumer->isDefinition()) {
            
            if (consumer->toResumePoint()->isObservableOperand(*i))
                return true;
            continue;
        }

        MDefinition *def = consumer->toDefinition();
        switch (def->op()) {
          case MDefinition::Op_Elements: {
            MOZ_ASSERT(def->toElements()->object() == ins);
            for (MUseIterator i(def->usesBegin()); i != def->usesEnd(); i++) {
                
                
                MDefinition *access = (*i)->consumer()->toDefinition();

                switch (access->op()) {
                  case MDefinition::Op_LoadElement: {
                    MOZ_ASSERT(access->toLoadElement()->elements() == def);

                    
                    
                    
                    
                    
                    if (access->toLoadElement()->needsHoleCheck())
                        return true;

                    
                    
                    int32_t index;
                    if (!IndexOf(access, &index))
                        return true;
                    if (index < 0 || count <= index)
                        return true;
                    break;
                  }

                  case MDefinition::Op_StoreElement: {
                    MOZ_ASSERT(access->toStoreElement()->elements() == def);

                    
                    
                    
                    
                    
                    if (access->toStoreElement()->needsHoleCheck())
                        return true;

                    
                    
                    int32_t index;
                    if (!IndexOf(access, &index))
                        return true;
                    if (index < 0 || count <= index)
                        return true;

                    
                    if (access->toStoreElement()->value()->type() == MIRType_MagicHole)
                        return true;
                    break;
                  }

                  case MDefinition::Op_SetInitializedLength:
                    MOZ_ASSERT(access->toSetInitializedLength()->elements() == def);
                    break;

                  case MDefinition::Op_InitializedLength:
                    MOZ_ASSERT(access->toInitializedLength()->elements() == def);
                    break;

                  case MDefinition::Op_ArrayLength:
                    MOZ_ASSERT(access->toArrayLength()->elements() == def);
                    break;

                  default:
                    return true;
                }
            }

            break;
          }

          default:
            return true;
        }
    }

    return false;
}

struct ArrayTrait {
    typedef MArrayState BlockState;
    typedef Vector<BlockState *, 8, SystemAllocPolicy> GraphState;
};







static bool
ScalarReplacementOfArray(MIRGenerator *mir, MIRGraph &graph,
                         ArrayTrait::GraphState &states,
                         MInstruction *arr)
{
    typedef ArrayTrait::BlockState BlockState;

    
    
    if (!states.appendN(nullptr, graph.numBlocks()))
        return false;

    
    MBasicBlock *arrBlock = arr->block();
    MConstant *undefinedVal = MConstant::New(graph.alloc(), UndefinedValue());
    MConstant *initLength = MConstant::New(graph.alloc(), Int32Value(0));
    MConstant *length = nullptr;
    arrBlock->insertBefore(arr, undefinedVal);
    arrBlock->insertBefore(arr, initLength);
    states[arrBlock->id()] = BlockState::New(graph.alloc(), arr, undefinedVal, initLength);

    
    for (ReversePostorderIterator block = graph.rpoBegin(arr->block()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Scalar Replacement of Array"))
            return false;

        BlockState *state = states[block->id()];
        if (!state) {
            MOZ_ASSERT(!arrBlock->dominates(*block));
            continue;
        }

        
        
        if (*block == arrBlock)
            arrBlock->insertAfter(arr, state);
        else if (block->numPredecessors() > 1)
            block->insertBefore(*block->begin(), state);
        else
            MOZ_ASSERT(state->block()->dominates(*block));

        
        ReplaceResumePointOperands(block->entryResumePoint(), arr, state);

        for (MDefinitionIterator ins(*block); ins; ) {
            switch (ins->op()) {
              case MDefinition::Op_ArrayState: {
                ins++;
                continue;
              }

              case MDefinition::Op_LoadElement: {
                MLoadElement *def = ins->toLoadElement();

                
                MDefinition *elements = def->elements();
                if (!elements->isElements())
                    break;
                if (elements->toElements()->object() != arr)
                    break;

                
                int32_t index;
                MOZ_ALWAYS_TRUE(IndexOf(def, &index));
                ins->replaceAllUsesWith(state->getElement(index));

                
                ins = block->discardDefAt(ins);
                if (!elements->hasLiveDefUses())
                    elements->block()->discard(elements->toInstruction());
                continue;
              }

              case MDefinition::Op_StoreElement: {
                MStoreElement *def = ins->toStoreElement();

                
                MDefinition *elements = def->elements();
                if (!elements->isElements())
                    break;
                if (elements->toElements()->object() != arr)
                    break;

                
                int32_t index;
                MOZ_ALWAYS_TRUE(IndexOf(def, &index));
                state = BlockState::Copy(graph.alloc(), state);
                state->setElement(index, def->value());
                block->insertBefore(ins->toInstruction(), state);

                
                ins = block->discardDefAt(ins);
                if (!elements->hasLiveDefUses())
                    elements->block()->discard(elements->toInstruction());
                continue;
              }

              case MDefinition::Op_SetInitializedLength: {
                MSetInitializedLength *def = ins->toSetInitializedLength();

                
                MDefinition *elements = def->elements();
                if (!elements->isElements())
                    break;
                if (elements->toElements()->object() != arr)
                    break;

                
                
                
                
                
                state = BlockState::Copy(graph.alloc(), state);
                int32_t initLengthValue = def->index()->toConstant()->value().toInt32() + 1;
                MConstant *initLength = MConstant::New(graph.alloc(), Int32Value(initLengthValue));
                block->insertBefore(ins->toInstruction(), initLength);
                block->insertBefore(ins->toInstruction(), state);
                state->setInitializedLength(initLength);

                
                ins = block->discardDefAt(ins);
                if (!elements->hasLiveDefUses())
                    elements->block()->discard(elements->toInstruction());
                continue;
              }

              case MDefinition::Op_InitializedLength: {
                MInitializedLength *def = ins->toInitializedLength();

                
                MDefinition *elements = def->elements();
                if (!elements->isElements())
                    break;
                if (elements->toElements()->object() != arr)
                    break;

                
                ins->replaceAllUsesWith(state->initializedLength());

                
                ins = block->discardDefAt(ins);
                if (!elements->hasLiveDefUses())
                    elements->block()->discard(elements->toInstruction());
                continue;
              }

              case MDefinition::Op_ArrayLength: {
                MArrayLength *def = ins->toArrayLength();

                
                MDefinition *elements = def->elements();
                if (!elements->isElements())
                    break;
                if (elements->toElements()->object() != arr)
                    break;

                
                if (!length) {
                    length = MConstant::New(graph.alloc(), Int32Value(state->numElements()));
                    arrBlock->insertBefore(arr, length);
                }
                ins->replaceAllUsesWith(length);

                
                ins = block->discardDefAt(ins);
                if (!elements->hasLiveDefUses())
                    elements->block()->discard(elements->toInstruction());
                continue;
              }

              default:
                break;
            }

            
            if (ins->isInstruction())
                ReplaceResumePointOperands(ins->toInstruction()->resumePoint(), arr, state);

            ins++;
        }

        
        
        for (size_t s = 0; s < block->numSuccessors(); s++) {
            MBasicBlock *succ = block->getSuccessor(s);
            BlockState *succState = states[succ->id()];

            
            
            if (!succState) {
                
                
                
                
                
                
                if (!arrBlock->dominates(succ))
                    continue;

                if (succ->numPredecessors() > 1) {
                    succState = states[succ->id()] = BlockState::Copy(graph.alloc(), state);
                    size_t numPreds = succ->numPredecessors();
                    for (size_t index = 0; index < state->numElements(); index++) {
                        MPhi *phi = MPhi::New(graph.alloc());
                        if (!phi->reserveLength(numPreds))
                            return false;

                        
                        
                        for (size_t p = 0; p < numPreds; p++)
                            phi->addInput(undefinedVal);

                        
                        succ->addPhi(phi);
                        succState->setElement(index, phi);
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

                    
                    
                    for (size_t index = 0; index < state->numElements(); index++) {
                        MPhi *phi = succState->getElement(index)->toPhi();
                        phi->replaceOperand(p, state->getElement(index));
                    }
                }
            }
        }
    }

    MOZ_ASSERT(!arr->hasLiveDefUses());
    
    
    states.clear();
    return true;
}


bool
ScalarReplacement(MIRGenerator *mir, MIRGraph &graph)
{
    ObjectTrait::GraphState objectStates;
    ArrayTrait::GraphState arrayStates;

    for (ReversePostorderIterator block = graph.rpoBegin(); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Scalar Replacement (main loop)"))
            return false;

        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            if (ins->isNewObject() && !IsObjectEscaped(*ins)) {
                if (!ScalarReplacementOfObject(mir, graph, objectStates, *ins))
                    return false;
                continue;
            }

            if (ins->isNewArray() && !IsArrayEscaped(*ins)) {
                if (!ScalarReplacementOfArray(mir, graph, arrayStates, *ins))
                    return false;
                continue;
            }
        }
    }

    return true;
}

} 
} 
