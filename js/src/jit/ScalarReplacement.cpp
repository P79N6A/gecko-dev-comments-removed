





#include "jit/ScalarReplacement.h"

#include "mozilla/Vector.h"

#include "jit/IonAnalysis.h"
#include "jit/JitSpewer.h"
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
            
            if (consumer->toResumePoint()->isObservableOperand(*i)) {
                JitSpewDef(JitSpew_Escape, "Object is observable\n", ins);
                return true;
            }
            continue;
        }

        MDefinition *def = consumer->toDefinition();
        switch (def->op()) {
          case MDefinition::Op_StoreFixedSlot:
          case MDefinition::Op_LoadFixedSlot:
            
            if (def->indexOf(*i) == 0)
                break;

            JitSpewDef(JitSpew_Escape, "Object ", ins);
            JitSpewDef(JitSpew_Escape, "  is escaped by\n", def);
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
            if (ins->toNewObject()->templateObject()->lastProperty() != guard->shape()) {
                JitSpewDef(JitSpew_Escape, "Object ", ins);
                JitSpewDef(JitSpew_Escape, "  has a non-matching guard shape\n", guard);
                return true;
            }
            if (IsObjectEscaped(def->toInstruction()))
                return true;
            break;
          }
          default:
            JitSpewDef(JitSpew_Escape, "Object ", ins);
            JitSpewDef(JitSpew_Escape, "  is escaped by\n", def);
            return true;
        }
    }

    JitSpewDef(JitSpew_Escape, "Object is not escaped\n", ins);
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

        
        
        
        
        if (succ->numPredecessors() <= 1 || !state_->numSlots()) {
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

    if (succ->numPredecessors() > 1 && succState->numSlots()) {
        
        
        size_t currIndex;
        MOZ_ASSERT(!succ->phisEmpty());
        if (curr->successorWithPhis()) {
            MOZ_ASSERT(curr->successorWithPhis() == succ);
            currIndex = curr->positionInPhiSuccessor();
        } else {
            currIndex = succ->indexForPredecessor(curr);
            curr->setSuccessorWithPhis(succ, currIndex);
        }
        MOZ_ASSERT(succ->getPredecessor(currIndex) == curr);

        
        
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

    
    
    if (!ins->toNewArray()->isAllocating()) {
        JitSpewDef(JitSpew_Escape, "Array is not allocated\n", ins);
        return true;
    }

    if (count >= 16) {
        JitSpewDef(JitSpew_Escape, "Array has too many elements\n", ins);
        return true;
    }

    
    
    
    for (MUseIterator i(ins->usesBegin()); i != ins->usesEnd(); i++) {
        MNode *consumer = (*i)->consumer();
        if (!consumer->isDefinition()) {
            
            if (consumer->toResumePoint()->isObservableOperand(*i)) {
                JitSpewDef(JitSpew_Escape, "Array is observable\n", ins);
                return true;
            }
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

                    
                    
                    
                    
                    
                    if (access->toLoadElement()->needsHoleCheck()) {
                        JitSpewDef(JitSpew_Escape, "Array ", ins);
                        JitSpewDef(JitSpew_Escape,
                                   "  has a load element which needs hole check\n", access);
                        return true;
                    }

                    
                    
                    int32_t index;
                    if (!IndexOf(access, &index)) {
                        JitSpewDef(JitSpew_Escape, "Array ", ins);
                        JitSpewDef(JitSpew_Escape,
                                   "  has a load element with a non-trivial index\n", access);
                        return true;
                    }
                    if (index < 0 || count <= uint32_t(index)) {
                        JitSpewDef(JitSpew_Escape, "Array ", ins);
                        JitSpewDef(JitSpew_Escape,
                                   "  has a load element with an out-of-bound index\n", access);
                        return true;
                    }
                    break;
                  }

                  case MDefinition::Op_StoreElement: {
                    MOZ_ASSERT(access->toStoreElement()->elements() == def);

                    
                    
                    
                    
                    
                    if (access->toStoreElement()->needsHoleCheck())
                        return true;

                    
                    
                    int32_t index;
                    if (!IndexOf(access, &index)) {
                        JitSpewDef(JitSpew_Escape, "Array ", ins);
                        JitSpewDef(JitSpew_Escape, "  has a store element with a non-trivial index\n", access);
                        return true;
                    }
                    if (index < 0 || count <= uint32_t(index)) {
                        JitSpewDef(JitSpew_Escape, "Array ", ins);
                        JitSpewDef(JitSpew_Escape, "  has a store element with an out-of-bound index\n", access);
                        return true;
                    }

                    
                    if (access->toStoreElement()->value()->type() == MIRType_MagicHole) {
                        JitSpewDef(JitSpew_Escape, "Array ", ins);
                        JitSpewDef(JitSpew_Escape, "  has a store element with an magic-hole constant\n", access);
                        return true;
                    }
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
                    JitSpewDef(JitSpew_Escape, "Array's element ", ins);
                    JitSpewDef(JitSpew_Escape, "  is escaped by\n", def);
                    return true;
                }
            }

            break;
          }

          default:
            JitSpewDef(JitSpew_Escape, "Array ", ins);
            JitSpewDef(JitSpew_Escape, "  is escaped by\n", def);
            return true;
        }
    }

    JitSpewDef(JitSpew_Escape, "Array is not escaped\n", ins);
    return false;
}







class ArrayMemoryView : public MDefinitionVisitorDefaultNoop
{
  public:
    typedef MArrayState BlockState;
    static const char *phaseName;

  private:
    TempAllocator &alloc_;
    MConstant *undefinedVal_;
    MConstant *length_;
    MInstruction *arr_;
    MBasicBlock *startBlock_;
    BlockState *state_;

  public:
    ArrayMemoryView(TempAllocator &alloc, MInstruction *arr);

    MBasicBlock *startingBlock();
    bool initStartingState(BlockState **pState);

    void setEntryBlockState(BlockState *state);
    bool mergeIntoSuccessorState(MBasicBlock *curr, MBasicBlock *succ, BlockState **pSuccState);

#ifdef DEBUG
    void assertSuccess();
#else
    void assertSuccess() {}
#endif

  private:
    bool isArrayStateElements(MDefinition *elements);
    void discardInstruction(MInstruction *ins, MDefinition *elements);

  public:
    bool visitResumePoint(MResumePoint *rp);
    bool visitStoreElement(MStoreElement *ins);
    bool visitLoadElement(MLoadElement *ins);
    bool visitSetInitializedLength(MSetInitializedLength *ins);
    bool visitInitializedLength(MInitializedLength *ins);
    bool visitArrayLength(MArrayLength *ins);
};

const char *ArrayMemoryView::phaseName = "Scalar Replacement of Array";

ArrayMemoryView::ArrayMemoryView(TempAllocator &alloc, MInstruction *arr)
  : alloc_(alloc),
    undefinedVal_(nullptr),
    length_(nullptr),
    arr_(arr),
    startBlock_(arr->block()),
    state_(nullptr)
{
}

MBasicBlock *
ArrayMemoryView::startingBlock()
{
    return startBlock_;
}

bool
ArrayMemoryView::initStartingState(BlockState **pState)
{
    
    undefinedVal_ = MConstant::New(alloc_, UndefinedValue());
    MConstant *initLength = MConstant::New(alloc_, Int32Value(0));
    arr_->block()->insertBefore(arr_, undefinedVal_);
    arr_->block()->insertBefore(arr_, initLength);

    
    BlockState *state = BlockState::New(alloc_, arr_, undefinedVal_, initLength);
    startBlock_->insertAfter(arr_, state);

    *pState = state;
    return true;
}

void
ArrayMemoryView::setEntryBlockState(BlockState *state)
{
    state_ = state;
}

bool
ArrayMemoryView::mergeIntoSuccessorState(MBasicBlock *curr, MBasicBlock *succ,
                                          BlockState **pSuccState)
{
    BlockState *succState = *pSuccState;

    
    
    if (!succState) {
        
        
        
        
        
        
        if (!startBlock_->dominates(succ))
            return true;

        
        
        
        
        if (succ->numPredecessors() <= 1 || !state_->numElements()) {
            *pSuccState = state_;
            return true;
        }

        
        
        
        
        succState = BlockState::Copy(alloc_, state_);
        size_t numPreds = succ->numPredecessors();
        for (size_t index = 0; index < state_->numElements(); index++) {
            MPhi *phi = MPhi::New(alloc_);
            if (!phi->reserveLength(numPreds))
                return false;

            
            
            for (size_t p = 0; p < numPreds; p++)
                phi->addInput(undefinedVal_);

            
            succ->addPhi(phi);
            succState->setElement(index, phi);
        }

        
        
        
        
        succ->insertBefore(*succ->begin(), succState);
        *pSuccState = succState;
    }

    if (succ->numPredecessors() > 1 && succState->numElements()) {
        
        
        size_t currIndex;
        MOZ_ASSERT(!succ->phisEmpty());
        if (curr->successorWithPhis()) {
            MOZ_ASSERT(curr->successorWithPhis() == succ);
            currIndex = curr->positionInPhiSuccessor();
        } else {
            currIndex = succ->indexForPredecessor(curr);
            curr->setSuccessorWithPhis(succ, currIndex);
        }
        MOZ_ASSERT(succ->getPredecessor(currIndex) == curr);

        
        
        for (size_t index = 0; index < state_->numElements(); index++) {
            MPhi *phi = succState->getElement(index)->toPhi();
            phi->replaceOperand(currIndex, state_->getElement(index));
        }
    }

    return true;
}

#ifdef DEBUG
void
ArrayMemoryView::assertSuccess()
{
    MOZ_ASSERT(!arr_->hasLiveDefUses());
}
#endif

bool
ArrayMemoryView::visitResumePoint(MResumePoint *rp)
{
    ReplaceResumePointOperands(rp, arr_, state_);
    return true;
}

bool
ArrayMemoryView::isArrayStateElements(MDefinition *elements)
{
    return elements->isElements() && elements->toElements()->object() == arr_;
}

void
ArrayMemoryView::discardInstruction(MInstruction *ins, MDefinition *elements)
{
    MOZ_ASSERT(elements->isElements());
    ins->block()->discard(ins);
    if (!elements->hasLiveDefUses())
        elements->block()->discard(elements->toInstruction());
}

bool
ArrayMemoryView::visitStoreElement(MStoreElement *ins)
{
    
    MDefinition *elements = ins->elements();
    if (!isArrayStateElements(elements))
        return true;

    
    int32_t index;
    MOZ_ALWAYS_TRUE(IndexOf(ins, &index));
    state_ = BlockState::Copy(alloc_, state_);
    state_->setElement(index, ins->value());
    ins->block()->insertBefore(ins, state_);

    
    discardInstruction(ins, elements);
    return true;
}

bool
ArrayMemoryView::visitLoadElement(MLoadElement *ins)
{
    
    MDefinition *elements = ins->elements();
    if (!isArrayStateElements(elements))
        return true;

    
    int32_t index;
    MOZ_ALWAYS_TRUE(IndexOf(ins, &index));
    ins->replaceAllUsesWith(state_->getElement(index));

    
    discardInstruction(ins, elements);
    return true;
}

bool
ArrayMemoryView::visitSetInitializedLength(MSetInitializedLength *ins)
{
    
    MDefinition *elements = ins->elements();
    if (!isArrayStateElements(elements))
        return true;

    
    
    
    
    state_ = BlockState::Copy(alloc_, state_);
    int32_t initLengthValue = ins->index()->toConstant()->value().toInt32() + 1;
    MConstant *initLength = MConstant::New(alloc_, Int32Value(initLengthValue));
    ins->block()->insertBefore(ins, initLength);
    ins->block()->insertBefore(ins, state_);
    state_->setInitializedLength(initLength);

    
    discardInstruction(ins, elements);
    return true;
}

bool
ArrayMemoryView::visitInitializedLength(MInitializedLength *ins)
{
    
    MDefinition *elements = ins->elements();
    if (!isArrayStateElements(elements))
        return true;

    
    ins->replaceAllUsesWith(state_->initializedLength());

    
    discardInstruction(ins, elements);
    return true;
}

bool
ArrayMemoryView::visitArrayLength(MArrayLength *ins)
{
    
    MDefinition *elements = ins->elements();
    if (!isArrayStateElements(elements))
        return true;

    
    if (!length_) {
        length_ = MConstant::New(alloc_, Int32Value(state_->numElements()));
        arr_->block()->insertBefore(arr_, length_);
    }
    ins->replaceAllUsesWith(length_);

    
    discardInstruction(ins, elements);
    return true;
}

bool
ScalarReplacement(MIRGenerator *mir, MIRGraph &graph)
{
    EmulateStateOf<ObjectMemoryView> replaceObject(mir, graph);
    EmulateStateOf<ArrayMemoryView> replaceArray(mir, graph);
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
                ArrayMemoryView view(graph.alloc(), *ins);
                if (!replaceArray.run(view))
                    return false;
                view.assertSuccess();
                addedPhi = true;
                continue;
            }
        }
    }

    if (addedPhi) {
        
        
        
        
        AssertExtendedGraphCoherency(graph);
        if (!EliminatePhis(mir, graph, ConservativeObservability))
            return false;
    }

    return true;
}

} 
} 
