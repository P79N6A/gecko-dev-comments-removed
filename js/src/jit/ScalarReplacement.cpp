





#include "jit/ScalarReplacement.h"

#include "mozilla/Vector.h"

#include "jit/IonAnalysis.h"
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

template <typename MemoryView>
class EmulateStateOf
{
  private:
    typedef typename MemoryView::BlockState BlockState;

    MIRGenerator *mir_;
    MIRGraph &graph_;

    
    Vector<BlockState *, 8, SystemAllocPolicy> states_;

  public:
    EmulateStateOf(MIRGenerator *mir, MIRGraph &graph)
      : mir_(mir),
        graph_(graph)
    {
    }

    bool run(MemoryView &view);
};

template <typename MemoryView>
bool
EmulateStateOf<MemoryView>::run(MemoryView &view)
{
    
    if (!states_.appendN(nullptr, graph_.numBlocks()))
        return false;

    
    MBasicBlock *startBlock = view.startingBlock();
    if (!view.initStartingState(&states_[startBlock->id()]))
        return false;

    
    
    for (ReversePostorderIterator block = graph_.rpoBegin(startBlock); block != graph_.rpoEnd(); block++) {
        if (mir_->shouldCancel(MemoryView::phaseName))
            return false;

        
        
        
        BlockState *state = states_[block->id()];
        if (!state)
            continue;
        view.setEntryBlockState(state);

        
        for (MNodeIterator iter(*block); iter; ) {
            
            
            MNode *ins = *iter++;
            if (ins->isDefinition()) {
                if (!ins->toDefinition()->accept(&view))
                    return false;
            } else if (!view.visitResumePoint(ins->toResumePoint())) {
                return false;
            }
        }

        
        
        for (size_t s = 0; s < block->numSuccessors(); s++) {
            MBasicBlock *succ = block->getSuccessor(s);
            if (!view.mergeIntoSuccessorState(*block, succ, &states_[succ->id()]))
                return false;
        }
    }

    states_.clear();
    return true;
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

class ObjectMemoryView : public MDefinitionVisitorDefaultNoop
{
  public:
    typedef MObjectState BlockState;
    static const char *phaseName;

  private:
    TempAllocator &alloc_;
    MConstant *undefinedVal_;
    MInstruction *obj_;
    MBasicBlock *startBlock_;
    BlockState *state_;

  public:
    ObjectMemoryView(TempAllocator &alloc, MInstruction *obj);

    MBasicBlock *startingBlock();
    bool initStartingState(BlockState **pState);

    void setEntryBlockState(BlockState *state);
    bool mergeIntoSuccessorState(MBasicBlock *curr, MBasicBlock *succ, BlockState **pSuccState);

#ifdef DEBUG
    void assertSuccess();
#else
    void assertSuccess() {}
#endif

  public:
    bool visitResumePoint(MResumePoint *rp);
    bool visitStoreFixedSlot(MStoreFixedSlot *ins);
    bool visitLoadFixedSlot(MLoadFixedSlot *ins);
    bool visitStoreSlot(MStoreSlot *ins);
    bool visitLoadSlot(MLoadSlot *ins);
    bool visitGuardShape(MGuardShape *ins);
};

const char *ObjectMemoryView::phaseName = "Scalar Replacement of Object";

ObjectMemoryView::ObjectMemoryView(TempAllocator &alloc, MInstruction *obj)
  : alloc_(alloc),
    obj_(obj),
    startBlock_(obj->block())
{
}

MBasicBlock *
ObjectMemoryView::startingBlock()
{
    return startBlock_;
}

bool
ObjectMemoryView::initStartingState(BlockState **pState)
{
    
    undefinedVal_ = MConstant::New(alloc_, UndefinedValue());
    startBlock_->insertBefore(obj_, undefinedVal_);

    
    BlockState *state = BlockState::New(alloc_, obj_, undefinedVal_);
    startBlock_->insertAfter(obj_, state);

    *pState = state;
    return true;
}

void
ObjectMemoryView::setEntryBlockState(BlockState *state)
{
    state_ = state;
}

bool
ObjectMemoryView::mergeIntoSuccessorState(MBasicBlock *curr, MBasicBlock *succ,
                                          BlockState **pSuccState)
{
    BlockState *succState = *pSuccState;

    
    
    if (!succState) {
        
        
        
        
        
        
        if (!startBlock_->dominates(succ))
            return true;

        
        
        
        
        if (succ->numPredecessors() <= 1) {
            *pSuccState = state_;
            return true;
        }

        
        
        
        
        succState = BlockState::Copy(alloc_, state_);
        size_t numPreds = succ->numPredecessors();
        for (size_t slot = 0; slot < state_->numSlots(); slot++) {
            MPhi *phi = MPhi::New(alloc_);
            if (!phi->reserveLength(numPreds))
                return false;

            
            
            for (size_t p = 0; p < numPreds; p++)
                phi->addInput(undefinedVal_);

            
            succ->addPhi(phi);
            succState->setSlot(slot, phi);
        }

        
        
        
        
        succ->insertBefore(*succ->begin(), succState);
        *pSuccState = succState;
    }

    if (succ->numPredecessors() > 1) {
        
        
        size_t currIndex = succ->indexForPredecessor(curr);
        MOZ_ASSERT(succ->getPredecessor(currIndex) == curr);
        curr->setSuccessorWithPhis(succ, currIndex);

        
        
        for (size_t slot = 0; slot < state_->numSlots(); slot++) {
            MPhi *phi = succState->getSlot(slot)->toPhi();
            phi->replaceOperand(currIndex, state_->getSlot(slot));
        }
    }

    return true;
}

#ifdef DEBUG
void
ObjectMemoryView::assertSuccess()
{
    for (MUseIterator i(obj_->usesBegin()); i != obj_->usesEnd(); i++) {
        MNode *ins = (*i)->consumer();

        
        MOZ_ASSERT(!ins->isResumePoint());

        MDefinition *def = ins->toDefinition();

        if (def->isRecoveredOnBailout())
            continue;

        
        
        MOZ_ASSERT(def->isSlots());
        MOZ_ASSERT(!def->hasOneUse());
    }
}
#endif

bool
ObjectMemoryView::visitResumePoint(MResumePoint *rp)
{
    ReplaceResumePointOperands(rp, obj_, state_);
    return true;
}

bool
ObjectMemoryView::visitStoreFixedSlot(MStoreFixedSlot *ins)
{
    
    if (ins->object() != obj_)
        return true;

    
    state_ = BlockState::Copy(alloc_, state_);
    state_->setFixedSlot(ins->slot(), ins->value());
    ins->block()->insertBefore(ins->toInstruction(), state_);

    
    ins->block()->discard(ins);
    return true;
}

bool
ObjectMemoryView::visitLoadFixedSlot(MLoadFixedSlot *ins)
{
    
    if (ins->object() != obj_)
        return true;

    
    ins->replaceAllUsesWith(state_->getFixedSlot(ins->slot()));

    
    ins->block()->discard(ins);
    return true;
}

bool
ObjectMemoryView::visitStoreSlot(MStoreSlot *ins)
{
    
    MSlots *slots = ins->slots()->toSlots();
    if (slots->object() != obj_) {
        
        MOZ_ASSERT(!slots->object()->isGuardShape() || slots->object()->toGuardShape()->obj() != obj_);
        return true;
    }

    
    state_ = BlockState::Copy(alloc_, state_);
    state_->setDynamicSlot(ins->slot(), ins->value());
    ins->block()->insertBefore(ins->toInstruction(), state_);

    
    ins->block()->discard(ins);
    return true;
}

bool
ObjectMemoryView::visitLoadSlot(MLoadSlot *ins)
{
    
    MSlots *slots = ins->slots()->toSlots();
    if (slots->object() != obj_) {
        
        MOZ_ASSERT(!slots->object()->isGuardShape() || slots->object()->toGuardShape()->obj() != obj_);
        return true;
    }

    
    ins->replaceAllUsesWith(state_->getDynamicSlot(ins->slot()));

    
    ins->block()->discard(ins);
    return true;
}

bool
ObjectMemoryView::visitGuardShape(MGuardShape *ins)
{
    
    if (ins->obj() != obj_)
        return true;

    
    ins->replaceAllUsesWith(obj_);

    
    ins->block()->discard(ins);
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
                    if (index < 0 || count <= uint32_t(index))
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
                    if (index < 0 || count <= uint32_t(index))
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
                
                
                size_t currIndex = succ->indexForPredecessor(*block);
                MOZ_ASSERT(succ->getPredecessor(currIndex) == *block);
                block->setSuccessorWithPhis(succ, currIndex);

                
                
                for (size_t index = 0; index < state->numElements(); index++) {
                    MPhi *phi = succState->getElement(index)->toPhi();
                    phi->replaceOperand(currIndex, state->getElement(index));
                }
            }
        }
    }

    MOZ_ASSERT(!arr->hasLiveDefUses());
    arr->setRecoveredOnBailout();
    states.clear();
    return true;
}


bool
ScalarReplacement(MIRGenerator *mir, MIRGraph &graph)
{
    EmulateStateOf<ObjectMemoryView> replaceObject(mir, graph);
    ArrayTrait::GraphState arrayStates;
    bool addedPhi = false;

    for (ReversePostorderIterator block = graph.rpoBegin(); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Scalar Replacement (main loop)"))
            return false;

        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            if (ins->isNewObject() && !IsObjectEscaped(*ins)) {
                ObjectMemoryView view(graph.alloc(), *ins);
                if (!replaceObject.run(view))
                    return false;
                view.assertSuccess();
                addedPhi = true;
                continue;
            }

            if (ins->isNewArray() && !IsArrayEscaped(*ins)) {
                if (!ScalarReplacementOfArray(mir, graph, arrayStates, *ins))
                    return false;
                addedPhi = true;
                continue;
            }
        }
    }

    if (addedPhi) {
        
        
        
        
        if (!EliminatePhis(mir, graph, ConservativeObservability))
            return false;
    }

    return true;
}

} 
} 
