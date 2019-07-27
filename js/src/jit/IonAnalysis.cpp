





#include "jit/IonAnalysis.h"

#include "jit/AliasAnalysis.h"
#include "jit/BaselineInspector.h"
#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/IonBuilder.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/LIR.h"
#include "jit/Lowering.h"
#include "jit/MIRGraph.h"

#include "jsobjinlines.h"
#include "jsopcodeinlines.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;

static bool
SplitCriticalEdgesForBlock(MIRGraph& graph, MBasicBlock* block)
{
    if (block->numSuccessors() < 2)
        return true;
    for (size_t i = 0; i < block->numSuccessors(); i++) {
        MBasicBlock* target = block->getSuccessor(i);
        if (target->numPredecessors() < 2)
            continue;

        
        MBasicBlock* split = MBasicBlock::NewSplitEdge(graph, block->info(), block);
        if (!split)
            return false;
        split->setLoopDepth(block->loopDepth());
        graph.insertBlockAfter(block, split);
        split->end(MGoto::New(graph.alloc(), target));

        
        
        
        
        
        
        if (MResumePoint* rp = split->entryResumePoint()) {
            rp->releaseUses();
            split->clearEntryResumePoint();
        }

        block->replaceSuccessor(i, split);
        target->replacePredecessor(block, split);
    }
    return true;
}




bool
jit::SplitCriticalEdges(MIRGraph& graph)
{
    for (MBasicBlockIterator iter(graph.begin()); iter != graph.end(); iter++) {
        MBasicBlock* block = *iter;
        if (!SplitCriticalEdgesForBlock(graph, block))
            return false;
    }
    return true;
}


static bool
BlockComputesConstant(MBasicBlock* block, MDefinition* value)
{
    
    
    
    if (value->hasUses())
        return false;

    if (!value->isConstant() || value->block() != block)
        return false;
    if (!block->phisEmpty())
        return false;
    for (MInstructionIterator iter = block->begin(); iter != block->end(); ++iter) {
        if (*iter != value || !iter->isGoto())
            return false;
    }
    return true;
}




















static bool
IsPhiRedudantFilter(MPhi* phi)
{
    
    if (phi->operandIfRedundant())
        return true;

    
    bool onlyFilters = false;
    MDefinition* a = phi->getOperand(0);
    if (a->isFilterTypeSet()) {
        a = a->toFilterTypeSet()->input();
        onlyFilters = true;
    }

    for (size_t i = 1; i < phi->numOperands(); i++) {
        MDefinition* operand = phi->getOperand(i);
        if (operand == a) {
            onlyFilters = false;
            continue;
        }
        if (operand->isFilterTypeSet() && operand->toFilterTypeSet()->input() == a)
            continue;
        return false;
    }
    if (!onlyFilters)
        return true;

    
    MOZ_ASSERT(onlyFilters);
    return EqualTypes(a->type(), a->resultTypeSet(),
                      phi->type(), phi->resultTypeSet());
}



static bool
BlockIsSingleTest(MBasicBlock* phiBlock, MBasicBlock* testBlock, MPhi** pphi, MTest** ptest)
{
    *pphi = nullptr;
    *ptest = nullptr;

    if (phiBlock != testBlock) {
        MOZ_ASSERT(phiBlock->numSuccessors() == 1 && phiBlock->getSuccessor(0) == testBlock);
        if (!phiBlock->begin()->isGoto())
            return false;
    }

    MInstruction* ins = *testBlock->begin();
    if (!ins->isTest())
        return false;
    MTest* test = ins->toTest();
    if (!test->input()->isPhi())
        return false;
    MPhi* phi = test->input()->toPhi();
    if (phi->block() != phiBlock)
        return false;

    for (MUseIterator iter = phi->usesBegin(); iter != phi->usesEnd(); ++iter) {
        MUse* use = *iter;
        if (use->consumer() == test)
            continue;
        if (use->consumer()->isResumePoint()) {
            MBasicBlock* useBlock = use->consumer()->block();
            if (useBlock == phiBlock || useBlock == testBlock)
                continue;
        }
        return false;
    }

    for (MPhiIterator iter = phiBlock->phisBegin(); iter != phiBlock->phisEnd(); ++iter) {
        if (*iter == phi)
            continue;

        if (IsPhiRedudantFilter(*iter))
            continue;

        return false;
    }

    if (phiBlock != testBlock && !testBlock->phisEmpty())
        return false;

    *pphi = phi;
    *ptest = test;

    return true;
}






static void
UpdateTestSuccessors(TempAllocator& alloc, MBasicBlock* block,
                     MDefinition* value, MBasicBlock* ifTrue, MBasicBlock* ifFalse,
                     MBasicBlock* existingPred)
{
    MInstruction* ins = block->lastIns();
    if (ins->isTest()) {
        MTest* test = ins->toTest();
        MOZ_ASSERT(test->input() == value);

        if (ifTrue != test->ifTrue()) {
            test->ifTrue()->removePredecessor(block);
            ifTrue->addPredecessorSameInputsAs(block, existingPred);
            MOZ_ASSERT(test->ifTrue() == test->getSuccessor(0));
            test->replaceSuccessor(0, ifTrue);
        }

        if (ifFalse != test->ifFalse()) {
            test->ifFalse()->removePredecessor(block);
            ifFalse->addPredecessorSameInputsAs(block, existingPred);
            MOZ_ASSERT(test->ifFalse() == test->getSuccessor(1));
            test->replaceSuccessor(1, ifFalse);
        }

        return;
    }

    MOZ_ASSERT(ins->isGoto());
    ins->toGoto()->target()->removePredecessor(block);
    block->discardLastIns();

    MTest* test = MTest::New(alloc, value, ifTrue, ifFalse);
    block->end(test);

    ifTrue->addPredecessorSameInputsAs(block, existingPred);
    ifFalse->addPredecessorSameInputsAs(block, existingPred);
}

static void
MaybeFoldConditionBlock(MIRGraph& graph, MBasicBlock* initialBlock)
{
    
    
    
    
    
    

    
















    MInstruction* ins = initialBlock->lastIns();
    if (!ins->isTest())
        return;
    MTest* initialTest = ins->toTest();

    MBasicBlock* trueBranch = initialTest->ifTrue();
    if (trueBranch->numPredecessors() != 1 || trueBranch->numSuccessors() != 1)
        return;
    MBasicBlock* falseBranch = initialTest->ifFalse();
    if (falseBranch->numPredecessors() != 1 || falseBranch->numSuccessors() != 1)
        return;
    MBasicBlock* phiBlock = trueBranch->getSuccessor(0);
    if (phiBlock != falseBranch->getSuccessor(0))
        return;
    if (phiBlock->numPredecessors() != 2)
        return;

    if (initialBlock->isLoopBackedge() || trueBranch->isLoopBackedge() || falseBranch->isLoopBackedge())
        return;

    MBasicBlock* testBlock = phiBlock;
    if (testBlock->numSuccessors() == 1) {
        if (testBlock->isLoopBackedge())
            return;
        testBlock = testBlock->getSuccessor(0);
        if (testBlock->numPredecessors() != 1)
            return;
    }

    
    if (!SplitCriticalEdgesForBlock(graph, testBlock))
        CrashAtUnhandlableOOM("MaybeFoldConditionBlock");

    MPhi* phi;
    MTest* finalTest;
    if (!BlockIsSingleTest(phiBlock, testBlock, &phi, &finalTest))
        return;

    MDefinition* trueResult = phi->getOperand(phiBlock->indexForPredecessor(trueBranch));
    MDefinition* falseResult = phi->getOperand(phiBlock->indexForPredecessor(falseBranch));

    

    
    for (MPhiIterator iter = phiBlock->phisBegin(); iter != phiBlock->phisEnd(); ++iter) {
        if (*iter == phi)
            continue;

        MOZ_ASSERT(IsPhiRedudantFilter(*iter));
        MDefinition* redundant = (*iter)->operandIfRedundant();

        if (!redundant) {
            redundant = (*iter)->getOperand(0);
            if (redundant->isFilterTypeSet())
                redundant = redundant->toFilterTypeSet()->input();
        }

        (*iter)->replaceAllUsesWith(redundant);
    }

    
    phiBlock->discardPhi(*phiBlock->phisBegin());

    
    
    
    

    MBasicBlock* trueTarget = trueBranch;
    if (BlockComputesConstant(trueBranch, trueResult)) {
        trueTarget = trueResult->constantToBoolean()
                     ? finalTest->ifTrue()
                     : finalTest->ifFalse();
        phiBlock->removePredecessor(trueBranch);
        graph.removeBlock(trueBranch);
    } else if (initialTest->input() == trueResult) {
        trueTarget = finalTest->ifTrue();
        phiBlock->removePredecessor(trueBranch);
        graph.removeBlock(trueBranch);
    } else {
        UpdateTestSuccessors(graph.alloc(), trueBranch, trueResult,
                             finalTest->ifTrue(), finalTest->ifFalse(), testBlock);
    }

    MBasicBlock* falseTarget = falseBranch;
    if (BlockComputesConstant(falseBranch, falseResult)) {
        falseTarget = falseResult->constantToBoolean()
                      ? finalTest->ifTrue()
                      : finalTest->ifFalse();
        phiBlock->removePredecessor(falseBranch);
        graph.removeBlock(falseBranch);
    } else if (initialTest->input() == falseResult) {
        falseTarget = finalTest->ifFalse();
        phiBlock->removePredecessor(falseBranch);
        graph.removeBlock(falseBranch);
    } else {
        UpdateTestSuccessors(graph.alloc(), falseBranch, falseResult,
                             finalTest->ifTrue(), finalTest->ifFalse(), testBlock);
    }

    
    UpdateTestSuccessors(graph.alloc(), initialBlock, initialTest->input(),
                         trueTarget, falseTarget, testBlock);

    
    if (phiBlock != testBlock) {
        testBlock->removePredecessor(phiBlock);
        graph.removeBlock(phiBlock);
    }

    
    finalTest->ifTrue()->removePredecessor(testBlock);
    finalTest->ifFalse()->removePredecessor(testBlock);
    graph.removeBlock(testBlock);
}

void
jit::FoldTests(MIRGraph& graph)
{
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++)
        MaybeFoldConditionBlock(graph, *block);
}

static void
EliminateTriviallyDeadResumePointOperands(MIRGraph& graph, MResumePoint* rp)
{
    
    
    if (rp->mode() != MResumePoint::ResumeAt || *rp->pc() != JSOP_POP)
        return;

    size_t top = rp->stackDepth() - 1;
    MOZ_ASSERT(!rp->isObservableOperand(top));

    MDefinition* def = rp->getOperand(top);
    if (def->isConstant())
        return;

    MConstant* constant = rp->block()->optimizedOutConstant(graph.alloc());
    rp->replaceOperand(top, constant);
}











bool
jit::EliminateDeadResumePointOperands(MIRGenerator* mir, MIRGraph& graph)
{
    
    
    
    if (graph.hasTryBlock())
        return true;

    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Dead Resume Point Operands (main loop)"))
            return false;

        if (MResumePoint* rp = block->entryResumePoint())
            EliminateTriviallyDeadResumePointOperands(graph, rp);

        
        if (block->isLoopHeader() && block->backedge() == *block)
            continue;

        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            if (MResumePoint* rp = ins->resumePoint())
                EliminateTriviallyDeadResumePointOperands(graph, rp);

            
            if (ins->isConstant())
                continue;

            
            
            
            
            
            if (ins->isUnbox() || ins->isParameter() || ins->isTypeBarrier() ||
                ins->isComputeThis() || ins->isFilterTypeSet())
            {
                continue;
            }

            
            
            
            
            if (ins->isNewDerivedTypedObject() || ins->isRecoveredOnBailout()) {
                MOZ_ASSERT(ins->canRecoverOnBailout());
                continue;
            }

            
            
            
            if (ins->isImplicitlyUsed())
                continue;

            
            
            
            
            
            uint32_t maxDefinition = 0;
            for (MUseIterator uses(ins->usesBegin()); uses != ins->usesEnd(); uses++) {
                MNode* consumer = uses->consumer();
                if (consumer->isResumePoint()) {
                    
                    
                    
                    MResumePoint* resume = consumer->toResumePoint();
                    if (resume->isObservableOperand(*uses)) {
                        maxDefinition = UINT32_MAX;
                        break;
                    }
                    continue;
                }

                MDefinition* def = consumer->toDefinition();
                if (def->block() != *block || def->isBox() || def->isPhi()) {
                    maxDefinition = UINT32_MAX;
                    break;
                }
                maxDefinition = Max(maxDefinition, def->id());
            }
            if (maxDefinition == UINT32_MAX)
                continue;

            
            
            for (MUseIterator uses(ins->usesBegin()); uses != ins->usesEnd(); ) {
                MUse* use = *uses++;
                if (use->consumer()->isDefinition())
                    continue;
                MResumePoint* mrp = use->consumer()->toResumePoint();
                if (mrp->block() != *block ||
                    !mrp->instruction() ||
                    mrp->instruction() == *ins ||
                    mrp->instruction()->id() <= maxDefinition)
                {
                    continue;
                }

                
                
                
                
                
                
                
                
                MConstant* constant = MConstant::New(graph.alloc(), MagicValue(JS_OPTIMIZED_OUT));
                block->insertBefore(*(block->begin()), constant);
                use->replaceProducer(constant);
            }
        }
    }

    return true;
}


bool
js::jit::DeadIfUnused(const MDefinition* def)
{
    return !def->isEffectful() && !def->isGuard() && !def->isGuardRangeBailouts() &&
           !def->isControlInstruction() &&
           (!def->isInstruction() || !def->toInstruction()->resumePoint());
}



bool
js::jit::IsDiscardable(const MDefinition* def)
{
    return !def->hasUses() && (DeadIfUnused(def) || def->block()->isMarked());
}




bool
jit::EliminateDeadCode(MIRGenerator* mir, MIRGraph& graph)
{
    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Dead Code (main loop)"))
            return false;

        
        for (MInstructionReverseIterator iter = block->rbegin(); iter != block->rend(); ) {
            MInstruction* inst = *iter++;
            if (js::jit::IsDiscardable(inst))
            {
                block->discard(inst);
            }
        }
    }

    return true;
}

static inline bool
IsPhiObservable(MPhi* phi, Observability observe)
{
    
    
    if (phi->isImplicitlyUsed())
        return true;

    
    
    
    
    
    
    
    
    for (MUseIterator iter(phi->usesBegin()); iter != phi->usesEnd(); iter++) {
        MNode* consumer = iter->consumer();
        if (consumer->isResumePoint()) {
            MResumePoint* resume = consumer->toResumePoint();
            if (observe == ConservativeObservability)
                return true;
            if (resume->isObservableOperand(*iter))
                return true;
        } else {
            MDefinition* def = consumer->toDefinition();
            if (!def->isPhi())
                return true;
        }
    }

    return false;
}




static inline MDefinition*
IsPhiRedundant(MPhi* phi)
{
    MDefinition* first = phi->operandIfRedundant();
    if (first == nullptr)
        return nullptr;

    
    if (phi->isImplicitlyUsed())
        first->setImplicitlyUsedUnchecked();

    return first;
}

bool
jit::EliminatePhis(MIRGenerator* mir, MIRGraph& graph,
                   Observability observe)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    Vector<MPhi*, 16, SystemAllocPolicy> worklist;

    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Phis (populate loop)"))
            return false;

        MPhiIterator iter = block->phisBegin();
        while (iter != block->phisEnd()) {
            MPhi* phi = *iter++;

            
            
            phi->setUnused();

            
            if (MDefinition* redundant = IsPhiRedundant(phi)) {
                phi->justReplaceAllUsesWith(redundant);
                block->discardPhi(phi);
                continue;
            }

            
            if (IsPhiObservable(phi, observe)) {
                phi->setInWorklist();
                if (!worklist.append(phi))
                    return false;
            }
        }
    }

    
    while (!worklist.empty()) {
        if (mir->shouldCancel("Eliminate Phis (worklist)"))
            return false;

        MPhi* phi = worklist.popCopy();
        MOZ_ASSERT(phi->isUnused());
        phi->setNotInWorklist();

        
        if (MDefinition* redundant = IsPhiRedundant(phi)) {
            
            for (MUseDefIterator it(phi); it; it++) {
                if (it.def()->isPhi()) {
                    MPhi* use = it.def()->toPhi();
                    if (!use->isUnused()) {
                        use->setUnusedUnchecked();
                        use->setInWorklist();
                        if (!worklist.append(use))
                            return false;
                    }
                }
            }
            phi->justReplaceAllUsesWith(redundant);
        } else {
            
            phi->setNotUnused();
        }

        
        for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
            MDefinition* in = phi->getOperand(i);
            if (!in->isPhi() || !in->isUnused() || in->isInWorklist())
                continue;
            in->setInWorklist();
            if (!worklist.append(in->toPhi()))
                return false;
        }
    }

    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        MPhiIterator iter = block->phisBegin();
        while (iter != block->phisEnd()) {
            MPhi* phi = *iter++;
            if (phi->isUnused()) {
                phi->optimizeOutAllUses(graph.alloc());
                block->discardPhi(phi);
            }
        }
    }

    return true;
}

namespace {











class TypeAnalyzer
{
    MIRGenerator* mir;
    MIRGraph& graph;
    Vector<MPhi*, 0, SystemAllocPolicy> phiWorklist_;

    TempAllocator& alloc() const {
        return graph.alloc();
    }

    bool addPhiToWorklist(MPhi* phi) {
        if (phi->isInWorklist())
            return true;
        if (!phiWorklist_.append(phi))
            return false;
        phi->setInWorklist();
        return true;
    }
    MPhi* popPhi() {
        MPhi* phi = phiWorklist_.popCopy();
        phi->setNotInWorklist();
        return phi;
    }

    bool respecialize(MPhi* phi, MIRType type);
    bool propagateSpecialization(MPhi* phi);
    bool specializePhis();
    void replaceRedundantPhi(MPhi* phi);
    void adjustPhiInputs(MPhi* phi);
    bool adjustInputs(MDefinition* def);
    bool insertConversions();

    bool checkFloatCoherency();
    bool graphContainsFloat32();
    bool markPhiConsumers();
    bool markPhiProducers();
    bool specializeValidFloatOps();
    bool tryEmitFloatOperations();

  public:
    TypeAnalyzer(MIRGenerator* mir, MIRGraph& graph)
      : mir(mir), graph(graph)
    { }

    bool analyze();
};

} 


static MIRType
GuessPhiType(MPhi* phi, bool* hasInputsWithEmptyTypes)
{
#ifdef DEBUG
    
    
    
    MIRType magicType = MIRType_None;
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition* in = phi->getOperand(i);
        if (in->type() == MIRType_MagicOptimizedArguments ||
            in->type() == MIRType_MagicHole ||
            in->type() == MIRType_MagicIsConstructing)
        {
            if (magicType == MIRType_None)
                magicType = in->type();
            MOZ_ASSERT(magicType == in->type());
        }
    }
#endif

    *hasInputsWithEmptyTypes = false;

    MIRType type = MIRType_None;
    bool convertibleToFloat32 = false;
    bool hasPhiInputs = false;
    for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
        MDefinition* in = phi->getOperand(i);
        if (in->isPhi()) {
            hasPhiInputs = true;
            if (!in->toPhi()->triedToSpecialize())
                continue;
            if (in->type() == MIRType_None) {
                
                
                
                continue;
            }
        }

        
        if (in->resultTypeSet() && in->resultTypeSet()->empty()) {
            *hasInputsWithEmptyTypes = true;
            continue;
        }

        if (type == MIRType_None) {
            type = in->type();
            if (in->canProduceFloat32())
                convertibleToFloat32 = true;
            continue;
        }
        if (type != in->type()) {
            if (convertibleToFloat32 && in->type() == MIRType_Float32) {
                
                
                type = MIRType_Float32;
            } else if (IsNumberType(type) && IsNumberType(in->type())) {
                
                type = MIRType_Double;
                convertibleToFloat32 &= in->canProduceFloat32();
            } else {
                return MIRType_Value;
            }
        }
    }

    if (type == MIRType_None && !hasPhiInputs) {
        
        
        MOZ_ASSERT(*hasInputsWithEmptyTypes);
        type = MIRType_Value;
    }

    return type;
}

bool
TypeAnalyzer::respecialize(MPhi* phi, MIRType type)
{
    if (phi->type() == type)
        return true;
    phi->specialize(type);
    return addPhiToWorklist(phi);
}

bool
TypeAnalyzer::propagateSpecialization(MPhi* phi)
{
    MOZ_ASSERT(phi->type() != MIRType_None);

    
    for (MUseDefIterator iter(phi); iter; iter++) {
        if (!iter.def()->isPhi())
            continue;
        MPhi* use = iter.def()->toPhi();
        if (!use->triedToSpecialize())
            continue;
        if (use->type() == MIRType_None) {
            
            
            
            if (!respecialize(use, phi->type()))
                return false;
            continue;
        }
        if (use->type() != phi->type()) {
            
            if ((use->type() == MIRType_Int32 && use->canProduceFloat32() && phi->type() == MIRType_Float32) ||
                (phi->type() == MIRType_Int32 && phi->canProduceFloat32() && use->type() == MIRType_Float32))
            {
                if (!respecialize(use, MIRType_Float32))
                    return false;
                continue;
            }

            
            if (IsNumberType(use->type()) && IsNumberType(phi->type())) {
                if (!respecialize(use, MIRType_Double))
                    return false;
                continue;
            }

            
            if (!respecialize(use, MIRType_Value))
                return false;
        }
    }

    return true;
}

bool
TypeAnalyzer::specializePhis()
{
    Vector<MPhi*, 0, SystemAllocPolicy> phisWithEmptyInputTypes;

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Specialize Phis (main loop)"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
            bool hasInputsWithEmptyTypes;
            MIRType type = GuessPhiType(*phi, &hasInputsWithEmptyTypes);
            phi->specialize(type);
            if (type == MIRType_None) {
                
                
                
                

                
                
                
                
                if (hasInputsWithEmptyTypes && !phisWithEmptyInputTypes.append(*phi))
                    return false;
                continue;
            }
            if (!propagateSpecialization(*phi))
                return false;
        }
    }

    do {
        while (!phiWorklist_.empty()) {
            if (mir->shouldCancel("Specialize Phis (worklist)"))
                return false;

            MPhi* phi = popPhi();
            if (!propagateSpecialization(phi))
                return false;
        }

        
        
        
        while (!phisWithEmptyInputTypes.empty()) {
            if (mir->shouldCancel("Specialize Phis (phisWithEmptyInputTypes)"))
                return false;

            MPhi* phi = phisWithEmptyInputTypes.popCopy();
            if (phi->type() == MIRType_None) {
                phi->specialize(MIRType_Value);
                if (!propagateSpecialization(phi))
                    return false;
            }
        }
    } while (!phiWorklist_.empty());

    return true;
}

void
TypeAnalyzer::adjustPhiInputs(MPhi* phi)
{
    MIRType phiType = phi->type();
    MOZ_ASSERT(phiType != MIRType_None);

    
    
    
    
    if (phiType != MIRType_Value) {
        for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
            MDefinition* in = phi->getOperand(i);
            if (in->type() == phiType)
                continue;

            if (in->isBox() && in->toBox()->input()->type() == phiType) {
                phi->replaceOperand(i, in->toBox()->input());
            } else {
                MInstruction* replacement;

                if (phiType == MIRType_Double && IsFloatType(in->type())) {
                    
                    replacement = MToDouble::New(alloc(), in);
                } else if (phiType == MIRType_Float32) {
                    if (in->type() == MIRType_Int32 || in->type() == MIRType_Double) {
                        replacement = MToFloat32::New(alloc(), in);
                    } else {
                        
                        if (in->type() != MIRType_Value) {
                            MBox* box = MBox::New(alloc(), in);
                            in->block()->insertBefore(in->block()->lastIns(), box);
                            in = box;
                        }

                        MUnbox* unbox = MUnbox::New(alloc(), in, MIRType_Double, MUnbox::Fallible);
                        in->block()->insertBefore(in->block()->lastIns(), unbox);
                        replacement = MToFloat32::New(alloc(), in);
                    }
                } else {
                    
                    
                    
                    if (in->type() != MIRType_Value) {
                        MBox* box = MBox::New(alloc(), in);
                        in->block()->insertBefore(in->block()->lastIns(), box);
                        in = box;
                    }

                    
                    
                    replacement = MUnbox::New(alloc(), in, phiType, MUnbox::Fallible);
                }

                in->block()->insertBefore(in->block()->lastIns(), replacement);
                phi->replaceOperand(i, replacement);
            }
        }

        return;
    }

    
    for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
        MDefinition* in = phi->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;

        if (in->isUnbox() && phi->typeIncludes(in->toUnbox()->input())) {
            
            
            phi->replaceOperand(i, in->toUnbox()->input());
        } else {
            MDefinition* box = AlwaysBoxAt(alloc(), in->block()->lastIns(), in);
            phi->replaceOperand(i, box);
        }
    }
}

bool
TypeAnalyzer::adjustInputs(MDefinition* def)
{
    
    if (!def->isInstruction())
        return true;

    MInstruction* ins = def->toInstruction();
    TypePolicy* policy = ins->typePolicy();
    if (policy && !policy->adjustInputs(alloc(), ins))
        return false;
    return true;
}

void
TypeAnalyzer::replaceRedundantPhi(MPhi* phi)
{
    MBasicBlock* block = phi->block();
    js::Value v;
    switch (phi->type()) {
      case MIRType_Undefined:
        v = UndefinedValue();
        break;
      case MIRType_Null:
        v = NullValue();
        break;
      case MIRType_MagicOptimizedArguments:
        v = MagicValue(JS_OPTIMIZED_ARGUMENTS);
        break;
      case MIRType_MagicOptimizedOut:
        v = MagicValue(JS_OPTIMIZED_OUT);
        break;
      case MIRType_MagicUninitializedLexical:
        v = MagicValue(JS_UNINITIALIZED_LEXICAL);
        break;
      default:
        MOZ_CRASH("unexpected type");
    }
    MConstant* c = MConstant::New(alloc(), v);
    
    block->insertBefore(*(block->begin()), c);
    phi->justReplaceAllUsesWith(c);
}

bool
TypeAnalyzer::insertConversions()
{
    
    
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Insert Conversions"))
            return false;

        for (MPhiIterator iter(block->phisBegin()), end(block->phisEnd()); iter != end; ) {
            MPhi* phi = *iter++;
            if (phi->type() == MIRType_Undefined ||
                phi->type() == MIRType_Null ||
                phi->type() == MIRType_MagicOptimizedArguments ||
                phi->type() == MIRType_MagicOptimizedOut ||
                phi->type() == MIRType_MagicUninitializedLexical)
            {
                replaceRedundantPhi(phi);
                block->discardPhi(phi);
            } else {
                adjustPhiInputs(phi);
            }
        }
        for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
            if (!adjustInputs(*iter))
                return false;
        }
    }
    return true;
}




































bool
TypeAnalyzer::markPhiConsumers()
{
    MOZ_ASSERT(phiWorklist_.empty());

    
    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); ++block) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Consumer Phis - Initial state"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); ++phi) {
            MOZ_ASSERT(!phi->isInWorklist());
            bool canConsumeFloat32 = true;
            for (MUseDefIterator use(*phi); canConsumeFloat32 && use; use++) {
                MDefinition* usedef = use.def();
                canConsumeFloat32 &= usedef->isPhi() || usedef->canConsumeFloat32(use.use());
            }
            phi->setCanConsumeFloat32(canConsumeFloat32);
            if (canConsumeFloat32 && !addPhiToWorklist(*phi))
                return false;
        }
    }

    while (!phiWorklist_.empty()) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Consumer Phis - Fixed point"))
            return false;

        MPhi* phi = popPhi();
        MOZ_ASSERT(phi->canConsumeFloat32(nullptr ));

        bool validConsumer = true;
        for (MUseDefIterator use(phi); use; use++) {
            MDefinition* def = use.def();
            if (def->isPhi() && !def->canConsumeFloat32(use.use())) {
                validConsumer = false;
                break;
            }
        }

        if (validConsumer)
            continue;

        
        phi->setCanConsumeFloat32(false);
        for (size_t i = 0, e = phi->numOperands(); i < e; ++i) {
            MDefinition* input = phi->getOperand(i);
            if (input->isPhi() && !input->isInWorklist() && input->canConsumeFloat32(nullptr ))
            {
                if (!addPhiToWorklist(input->toPhi()))
                    return false;
            }
        }
    }
    return true;
}

bool
TypeAnalyzer::markPhiProducers()
{
    MOZ_ASSERT(phiWorklist_.empty());

    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); ++block) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Producer Phis - initial state"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); ++phi) {
            MOZ_ASSERT(!phi->isInWorklist());
            bool canProduceFloat32 = true;
            for (size_t i = 0, e = phi->numOperands(); canProduceFloat32 && i < e; ++i) {
                MDefinition* input = phi->getOperand(i);
                canProduceFloat32 &= input->isPhi() || input->canProduceFloat32();
            }
            phi->setCanProduceFloat32(canProduceFloat32);
            if (canProduceFloat32 && !addPhiToWorklist(*phi))
                return false;
        }
    }

    while (!phiWorklist_.empty()) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Producer Phis - Fixed point"))
            return false;

        MPhi* phi = popPhi();
        MOZ_ASSERT(phi->canProduceFloat32());

        bool validProducer = true;
        for (size_t i = 0, e = phi->numOperands(); i < e; ++i) {
            MDefinition* input = phi->getOperand(i);
            if (input->isPhi() && !input->canProduceFloat32()) {
                validProducer = false;
                break;
            }
        }

        if (validProducer)
            continue;

        
        phi->setCanProduceFloat32(false);
        for (MUseDefIterator use(phi); use; use++) {
            MDefinition* def = use.def();
            if (def->isPhi() && !def->isInWorklist() && def->canProduceFloat32())
            {
                if (!addPhiToWorklist(def->toPhi()))
                    return false;
            }
        }
    }
    return true;
}

bool
TypeAnalyzer::specializeValidFloatOps()
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); ++block) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Instructions"))
            return false;

        for (MInstructionIterator ins(block->begin()); ins != block->end(); ++ins) {
            if (!ins->isFloat32Commutative())
                continue;

            if (ins->type() == MIRType_Float32)
                continue;

            
            
            ins->trySpecializeFloat32(alloc());
        }
    }
    return true;
}

bool
TypeAnalyzer::graphContainsFloat32()
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); ++block) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Graph contains Float32"))
            return false;

        for (MDefinitionIterator def(*block); def; def++) {
            if (def->type() == MIRType_Float32)
                return true;
        }
    }
    return false;
}

bool
TypeAnalyzer::tryEmitFloatOperations()
{
    
    
    if (mir->compilingAsmJS())
        return true;

    
    
    if (!graphContainsFloat32())
        return true;

    if (!markPhiConsumers())
       return false;
    if (!markPhiProducers())
       return false;
    if (!specializeValidFloatOps())
       return false;
    return true;
}

bool
TypeAnalyzer::checkFloatCoherency()
{
#ifdef DEBUG
    
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); ++block) {
        if (mir->shouldCancel("Check Float32 coherency"))
            return false;

        for (MDefinitionIterator def(*block); def; def++) {
            if (def->type() != MIRType_Float32)
                continue;

            for (MUseDefIterator use(*def); use; use++) {
                MDefinition* consumer = use.def();
                MOZ_ASSERT(consumer->isConsistentFloat32Use(use.use()));
            }
        }
    }
#endif
    return true;
}

bool
TypeAnalyzer::analyze()
{
    if (!tryEmitFloatOperations())
        return false;
    if (!specializePhis())
        return false;
    if (!insertConversions())
        return false;
    if (!checkFloatCoherency())
        return false;
    return true;
}

bool
jit::ApplyTypeInformation(MIRGenerator* mir, MIRGraph& graph)
{
    TypeAnalyzer analyzer(mir, graph);

    if (!analyzer.analyze())
        return false;

    return true;
}

bool
jit::MakeMRegExpHoistable(MIRGraph& graph)
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++) {
            if (!iter->isRegExp())
                continue;

            MRegExp* regexp = iter->toRegExp();

            
            bool hoistable = true;
            for (MUseIterator i = regexp->usesBegin(); i != regexp->usesEnd(); i++) {
                
                
                if (i->consumer()->isResumePoint())
                    continue;

                MOZ_ASSERT(i->consumer()->isDefinition());

                
                MDefinition* use = i->consumer()->toDefinition();
                if (use->isRegExpReplace())
                    continue;
                if (use->isRegExpExec())
                    continue;
                if (use->isRegExpTest())
                    continue;

                hoistable = false;
                break;
            }

            if (!hoistable)
                continue;

            
            regexp->setMovable();

            
            
            RegExpObject* source = regexp->source();
            if (source->sticky() || source->global()) {
                MOZ_ASSERT(regexp->mustClone());
                MConstant* zero = MConstant::New(graph.alloc(), Int32Value(0));
                regexp->block()->insertAfter(regexp, zero);

                MStoreFixedSlot* lastIndex =
                    MStoreFixedSlot::New(graph.alloc(), regexp, RegExpObject::lastIndexSlot(), zero);
                regexp->block()->insertAfter(zero, lastIndex);
            }
        }
    }

    return true;
}

bool
jit::RenumberBlocks(MIRGraph& graph)
{
    size_t id = 0;
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++)
        block->setId(id++);

    return true;
}



bool
jit::AccountForCFGChanges(MIRGenerator* mir, MIRGraph& graph, bool updateAliasAnalysis)
{
    
    size_t id = 0;
    for (ReversePostorderIterator i(graph.rpoBegin()), e(graph.rpoEnd()); i != e; ++i) {
        i->clearDominatorInfo();
        i->setId(id++);
    }

    
    if (!BuildDominatorTree(graph))
        return false;

    
    if (updateAliasAnalysis) {
        if (!AliasAnalysis(mir, graph).analyze())
             return false;
    }

    AssertExtendedGraphCoherency(graph);
    return true;
}



bool
jit::RemoveUnmarkedBlocks(MIRGenerator* mir, MIRGraph& graph, uint32_t numMarkedBlocks)
{
    if (numMarkedBlocks == graph.numBlocks()) {
        
        
        
        graph.unmarkBlocks();
    } else {
        
        for (ReversePostorderIterator iter(graph.rpoBegin()); iter != graph.rpoEnd();) {
            MBasicBlock* block = *iter++;

            if (block->isMarked()) {
                block->unmark();
                continue;
            }

            
            
            
            if (block->isLoopHeader())
                block->clearLoopHeader();

            for (size_t i = 0, e = block->numSuccessors(); i != e; ++i)
                block->getSuccessor(i)->removePredecessor(block);
            graph.removeBlockIncludingPhis(block);
        }
    }

    
    return AccountForCFGChanges(mir, graph, false);
}



static MBasicBlock*
IntersectDominators(MBasicBlock* block1, MBasicBlock* block2)
{
    MBasicBlock* finger1 = block1;
    MBasicBlock* finger2 = block2;

    MOZ_ASSERT(finger1);
    MOZ_ASSERT(finger2);

    
    

    
    
    
    

    while (finger1->id() != finger2->id()) {
        while (finger1->id() > finger2->id()) {
            MBasicBlock* idom = finger1->immediateDominator();
            if (idom == finger1)
                return nullptr; 
            finger1 = idom;
        }

        while (finger2->id() > finger1->id()) {
            MBasicBlock* idom = finger2->immediateDominator();
            if (idom == finger2)
                return nullptr; 
            finger2 = idom;
        }
    }
    return finger1;
}

void
jit::ClearDominatorTree(MIRGraph& graph)
{
    for (MBasicBlockIterator iter = graph.begin(); iter != graph.end(); iter++)
        iter->clearDominatorInfo();
}

static void
ComputeImmediateDominators(MIRGraph& graph)
{
    
    MBasicBlock* startBlock = graph.entryBlock();
    startBlock->setImmediateDominator(startBlock);

    
    MBasicBlock* osrBlock = graph.osrBlock();
    if (osrBlock)
        osrBlock->setImmediateDominator(osrBlock);

    bool changed = true;

    while (changed) {
        changed = false;

        ReversePostorderIterator block = graph.rpoBegin();

        
        for (; block != graph.rpoEnd(); block++) {
            
            
            if (block->immediateDominator() == *block)
                continue;

            
            
            if (MOZ_UNLIKELY(block->numPredecessors() == 0)) {
                block->setImmediateDominator(*block);
                continue;
            }

            MBasicBlock* newIdom = block->getPredecessor(0);

            
            for (size_t i = 1; i < block->numPredecessors(); i++) {
                MBasicBlock* pred = block->getPredecessor(i);
                if (pred->immediateDominator() == nullptr)
                    continue;

                newIdom = IntersectDominators(pred, newIdom);

                
                if (newIdom == nullptr) {
                    block->setImmediateDominator(*block);
                    changed = true;
                    break;
                }
            }

            if (newIdom && block->immediateDominator() != newIdom) {
                block->setImmediateDominator(newIdom);
                changed = true;
            }
        }
    }

#ifdef DEBUG
    
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        MOZ_ASSERT(block->immediateDominator() != nullptr);
    }
#endif
}

bool
jit::BuildDominatorTree(MIRGraph& graph)
{
    ComputeImmediateDominators(graph);

    Vector<MBasicBlock*, 4, JitAllocPolicy> worklist(graph.alloc());

    
    
    
    
    
    for (PostorderIterator i(graph.poBegin()); i != graph.poEnd(); i++) {
        MBasicBlock* child = *i;
        MBasicBlock* parent = child->immediateDominator();

        
        child->addNumDominated(1);

        
        
        
        if (child == parent) {
            if (!worklist.append(child))
                return false;
            continue;
        }

        if (!parent->addImmediatelyDominatedBlock(child))
            return false;

        parent->addNumDominated(child->numDominated());
    }

#ifdef DEBUG
    
    
    if (!graph.osrBlock())
        MOZ_ASSERT(graph.entryBlock()->numDominated() == graph.numBlocks());
#endif
    
    
    size_t index = 0;
    while (!worklist.empty()) {
        MBasicBlock* block = worklist.popCopy();
        block->setDomIndex(index);

        if (!worklist.append(block->immediatelyDominatedBlocksBegin(),
                             block->immediatelyDominatedBlocksEnd())) {
            return false;
        }
        index++;
    }

    return true;
}

bool
jit::BuildPhiReverseMapping(MIRGraph& graph)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (block->phisEmpty())
            continue;

        
        for (size_t j = 0; j < block->numPredecessors(); j++) {
            MBasicBlock* pred = block->getPredecessor(j);

#ifdef DEBUG
            size_t numSuccessorsWithPhis = 0;
            for (size_t k = 0; k < pred->numSuccessors(); k++) {
                MBasicBlock* successor = pred->getSuccessor(k);
                if (!successor->phisEmpty())
                    numSuccessorsWithPhis++;
            }
            MOZ_ASSERT(numSuccessorsWithPhis <= 1);
#endif

            pred->setSuccessorWithPhis(*block, j);
        }
    }

    return true;
}

#ifdef DEBUG
static bool
CheckSuccessorImpliesPredecessor(MBasicBlock* A, MBasicBlock* B)
{
    
    for (size_t i = 0; i < B->numPredecessors(); i++) {
        if (A == B->getPredecessor(i))
            return true;
    }
    return false;
}

static bool
CheckPredecessorImpliesSuccessor(MBasicBlock* A, MBasicBlock* B)
{
    
    for (size_t i = 0; i < B->numSuccessors(); i++) {
        if (A == B->getSuccessor(i))
            return true;
    }
    return false;
}


































static void
CheckOperand(const MNode* consumer, const MUse* use, int32_t* usesBalance)
{
    MOZ_ASSERT(use->hasProducer());
    MDefinition* producer = use->producer();
    MOZ_ASSERT(!producer->isDiscarded());
    MOZ_ASSERT(producer->block() != nullptr);
    MOZ_ASSERT(use->consumer() == consumer);
#ifdef _DEBUG_CHECK_OPERANDS_USES_BALANCE
    fprintf(stderr, "==Check Operand\n");
    use->producer()->dump(stderr);
    fprintf(stderr, "  index: %" PRIuSIZE "\n", use->consumer()->indexOf(use));
    use->consumer()->dump(stderr);
    fprintf(stderr, "==End\n");
#endif
    --*usesBalance;
}

static void
CheckUse(const MDefinition* producer, const MUse* use, int32_t* usesBalance)
{
    MOZ_ASSERT(!use->consumer()->block()->isDead());
    MOZ_ASSERT_IF(use->consumer()->isDefinition(),
                  !use->consumer()->toDefinition()->isDiscarded());
    MOZ_ASSERT(use->consumer()->block() != nullptr);
    MOZ_ASSERT(use->consumer()->getOperand(use->index()) == producer);
#ifdef _DEBUG_CHECK_OPERANDS_USES_BALANCE
    fprintf(stderr, "==Check Use\n");
    use->producer()->dump(stderr);
    fprintf(stderr, "  index: %" PRIuSIZE "\n", use->consumer()->indexOf(use));
    use->consumer()->dump(stderr);
    fprintf(stderr, "==End\n");
#endif
    ++*usesBalance;
}
#endif 

void
jit::AssertBasicGraphCoherency(MIRGraph& graph)
{
#ifdef DEBUG
    MOZ_ASSERT(graph.entryBlock()->numPredecessors() == 0);
    MOZ_ASSERT(graph.entryBlock()->phisEmpty());
    MOZ_ASSERT(!graph.entryBlock()->unreachable());

    if (MBasicBlock* osrBlock = graph.osrBlock()) {
        MOZ_ASSERT(osrBlock->numPredecessors() == 0);
        MOZ_ASSERT(osrBlock->phisEmpty());
        MOZ_ASSERT(osrBlock != graph.entryBlock());
        MOZ_ASSERT(!osrBlock->unreachable());
    }

    if (MResumePoint* resumePoint = graph.entryResumePoint())
        MOZ_ASSERT(resumePoint->block() == graph.entryBlock());

    
    uint32_t count = 0;
    int32_t usesBalance = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        count++;

        MOZ_ASSERT(&block->graph() == &graph);
        MOZ_ASSERT(!block->isDead());
        MOZ_ASSERT_IF(block->outerResumePoint() != nullptr,
                      block->entryResumePoint() != nullptr);

        for (size_t i = 0; i < block->numSuccessors(); i++)
            MOZ_ASSERT(CheckSuccessorImpliesPredecessor(*block, block->getSuccessor(i)));

        for (size_t i = 0; i < block->numPredecessors(); i++)
            MOZ_ASSERT(CheckPredecessorImpliesSuccessor(*block, block->getPredecessor(i)));

        if (block->entryResumePoint()) {
            MOZ_ASSERT(!block->entryResumePoint()->instruction());
            MOZ_ASSERT(block->entryResumePoint()->block() == *block);
        }
        if (block->outerResumePoint()) {
            MOZ_ASSERT(!block->outerResumePoint()->instruction());
            MOZ_ASSERT(block->outerResumePoint()->block() == *block);
        }
        for (MResumePointIterator iter(block->resumePointsBegin()); iter != block->resumePointsEnd(); iter++) {
            
            
            
            MOZ_ASSERT_IF(iter->instruction(), iter->instruction()->block() == *block);
            for (uint32_t i = 0, e = iter->numOperands(); i < e; i++)
                CheckOperand(*iter, iter->getUseFor(i), &usesBalance);
        }
        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
            MOZ_ASSERT(phi->numOperands() == block->numPredecessors());
            MOZ_ASSERT(!phi->isRecoveredOnBailout());
            MOZ_ASSERT(phi->type() != MIRType_None);
            MOZ_ASSERT(phi->dependency() == nullptr);
        }
        for (MDefinitionIterator iter(*block); iter; iter++) {
            MOZ_ASSERT(iter->block() == *block);
            MOZ_ASSERT_IF(iter->hasUses(), iter->type() != MIRType_None);
            MOZ_ASSERT(!iter->isDiscarded());
            MOZ_ASSERT_IF(iter->isStart(),
                          *block == graph.entryBlock() || *block == graph.osrBlock());
            MOZ_ASSERT_IF(iter->isParameter(),
                          *block == graph.entryBlock() || *block == graph.osrBlock());
            MOZ_ASSERT_IF(iter->isOsrEntry(), *block == graph.osrBlock());
            MOZ_ASSERT_IF(iter->isOsrValue(), *block == graph.osrBlock());

            
            for (uint32_t i = 0, end = iter->numOperands(); i < end; i++)
                CheckOperand(*iter, iter->getUseFor(i), &usesBalance);
            for (MUseIterator use(iter->usesBegin()); use != iter->usesEnd(); use++)
                CheckUse(*iter, *use, &usesBalance);

            if (iter->isInstruction()) {
                if (MResumePoint* resume = iter->toInstruction()->resumePoint()) {
                    MOZ_ASSERT(resume->instruction() == *iter);
                    MOZ_ASSERT(resume->block() == *block);
                    MOZ_ASSERT(resume->block()->entryResumePoint() != nullptr);
                }
            }

            if (iter->isRecoveredOnBailout())
                MOZ_ASSERT(!iter->hasLiveDefUses());
        }

        
        MControlInstruction* control = block->lastIns();
        MOZ_ASSERT(control->block() == *block);
        MOZ_ASSERT(!control->hasUses());
        MOZ_ASSERT(control->type() == MIRType_None);
        MOZ_ASSERT(!control->isDiscarded());
        MOZ_ASSERT(!control->isRecoveredOnBailout());
        MOZ_ASSERT(control->resumePoint() == nullptr);
        for (uint32_t i = 0, end = control->numOperands(); i < end; i++)
            CheckOperand(control, control->getUseFor(i), &usesBalance);
    }

    
    MOZ_ASSERT(usesBalance <= 0, "More use checks than operand checks");
    MOZ_ASSERT(usesBalance >= 0, "More operand checks than use checks");
    MOZ_ASSERT(graph.numBlocks() == count);
#endif
}

#ifdef DEBUG
static void
AssertReversePostorder(MIRGraph& graph)
{
    
    for (ReversePostorderIterator iter(graph.rpoBegin()); iter != graph.rpoEnd(); ++iter) {
        MBasicBlock* block = *iter;
        MOZ_ASSERT(!block->isMarked());

        for (size_t i = 0; i < block->numPredecessors(); i++) {
            MBasicBlock* pred = block->getPredecessor(i);
            if (!pred->isMarked()) {
                MOZ_ASSERT(pred->isLoopBackedge());
                MOZ_ASSERT(block->backedge() == pred);
            }
        }

        block->mark();
    }

    graph.unmarkBlocks();
}
#endif

#ifdef DEBUG
static void
AssertDominatorTree(MIRGraph& graph)
{
    

    MOZ_ASSERT(graph.entryBlock()->immediateDominator() == graph.entryBlock());
    if (MBasicBlock* osrBlock = graph.osrBlock())
        MOZ_ASSERT(osrBlock->immediateDominator() == osrBlock);
    else
        MOZ_ASSERT(graph.entryBlock()->numDominated() == graph.numBlocks());

    size_t i = graph.numBlocks();
    size_t totalNumDominated = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        MOZ_ASSERT(block->dominates(*block));

        MBasicBlock* idom = block->immediateDominator();
        MOZ_ASSERT(idom->dominates(*block));
        MOZ_ASSERT(idom == *block || idom->id() < block->id());

        if (idom == *block) {
            totalNumDominated += block->numDominated();
        } else {
            bool foundInParent = false;
            for (size_t j = 0; j < idom->numImmediatelyDominatedBlocks(); j++) {
                if (idom->getImmediatelyDominatedBlock(j) == *block) {
                    foundInParent = true;
                    break;
                }
            }
            MOZ_ASSERT(foundInParent);
        }

        size_t numDominated = 1;
        for (size_t j = 0; j < block->numImmediatelyDominatedBlocks(); j++) {
            MBasicBlock* dom = block->getImmediatelyDominatedBlock(j);
            MOZ_ASSERT(block->dominates(dom));
            MOZ_ASSERT(dom->id() > block->id());
            MOZ_ASSERT(dom->immediateDominator() == *block);

            numDominated += dom->numDominated();
        }
        MOZ_ASSERT(block->numDominated() == numDominated);
        MOZ_ASSERT(block->numDominated() <= i);
        MOZ_ASSERT(block->numSuccessors() != 0 || block->numDominated() == 1);
        i--;
    }
    MOZ_ASSERT(i == 0);
    MOZ_ASSERT(totalNumDominated == graph.numBlocks());
}
#endif

void
jit::AssertGraphCoherency(MIRGraph& graph)
{
#ifdef DEBUG
    if (!js_JitOptions.checkGraphConsistency)
        return;
    AssertBasicGraphCoherency(graph);
    AssertReversePostorder(graph);
#endif
}

#ifdef DEBUG
static bool
IsResumableMIRType(MIRType type)
{
    
    switch (type) {
      case MIRType_Undefined:
      case MIRType_Null:
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
      case MIRType_Float32:
      case MIRType_String:
      case MIRType_Symbol:
      case MIRType_Object:
      case MIRType_MagicOptimizedArguments:
      case MIRType_MagicOptimizedOut:
      case MIRType_MagicUninitializedLexical:
      case MIRType_Value:
      case MIRType_Float32x4:
      case MIRType_Int32x4:
        return true;

      case MIRType_MagicHole:
      case MIRType_MagicIsConstructing:
      case MIRType_ObjectOrNull:
      case MIRType_None:
      case MIRType_Slots:
      case MIRType_Elements:
      case MIRType_Pointer:
      case MIRType_Shape:
      case MIRType_ObjectGroup:
      case MIRType_Doublex2: 
        return false;
    }
    MOZ_CRASH("Unknown MIRType.");
}

static void
AssertResumableOperands(MNode* node)
{
    for (size_t i = 0, e = node->numOperands(); i < e; ++i) {
        MDefinition* op = node->getOperand(i);
        if (op->isRecoveredOnBailout())
            continue;
        MOZ_ASSERT(IsResumableMIRType(op->type()),
                   "Resume point cannot encode its operands");
    }
}

static void
AssertIfResumableInstruction(MDefinition* def)
{
    if (!def->isRecoveredOnBailout())
        return;
    AssertResumableOperands(def);
}

static void
AssertResumePointDominatedByOperands(MResumePoint* resume)
{
    for (size_t i = 0, e = resume->numOperands(); i < e; ++i) {
        MDefinition* op = resume->getOperand(i);
        if (op->type() == MIRType_MagicOptimizedArguments)
            continue;
        MOZ_ASSERT(op->block()->dominates(resume->block()),
                   "Resume point is not dominated by its operands");
    }
}
#endif 

void
jit::AssertExtendedGraphCoherency(MIRGraph& graph)
{
    
    
    

#ifdef DEBUG
    if (!js_JitOptions.checkGraphConsistency)
        return;

    AssertGraphCoherency(graph);

    AssertDominatorTree(graph);

    uint32_t idx = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        MOZ_ASSERT(block->id() == idx++);

        
        if (block->numSuccessors() > 1)
            for (size_t i = 0; i < block->numSuccessors(); i++)
                MOZ_ASSERT(block->getSuccessor(i)->numPredecessors() == 1);

        if (block->isLoopHeader()) {
            MOZ_ASSERT(block->numPredecessors() == 2);
            MBasicBlock* backedge = block->getPredecessor(1);
            MOZ_ASSERT(backedge->id() >= block->id());
            MOZ_ASSERT(backedge->numSuccessors() == 1);
            MOZ_ASSERT(backedge->getSuccessor(0) == *block);
        }

        if (!block->phisEmpty()) {
            for (size_t i = 0; i < block->numPredecessors(); i++) {
                MBasicBlock* pred = block->getPredecessor(i);
                MOZ_ASSERT(pred->successorWithPhis() == *block);
                MOZ_ASSERT(pred->positionInPhiSuccessor() == i);
            }
        }

        uint32_t successorWithPhis = 0;
        for (size_t i = 0; i < block->numSuccessors(); i++)
            if (!block->getSuccessor(i)->phisEmpty())
                successorWithPhis++;

        MOZ_ASSERT(successorWithPhis <= 1);
        MOZ_ASSERT((successorWithPhis != 0) == (block->successorWithPhis() != nullptr));

        
        
        for (MPhiIterator iter(block->phisBegin()), end(block->phisEnd()); iter != end; ++iter) {
            MPhi* phi = *iter;
            for (size_t i = 0, e = phi->numOperands(); i < e; ++i) {
                
                
                
                if (phi->getOperand(i)->type() == MIRType_MagicOptimizedArguments)
                    continue;

                MOZ_ASSERT(phi->getOperand(i)->block()->dominates(block->getPredecessor(i)),
                           "Phi input is not dominated by its operand");
            }
        }

        
        for (MInstructionIterator iter(block->begin()), end(block->end()); iter != end; ++iter) {
            MInstruction* ins = *iter;
            for (size_t i = 0, e = ins->numOperands(); i < e; ++i) {
                MDefinition* op = ins->getOperand(i);
                MBasicBlock* opBlock = op->block();
                MOZ_ASSERT(opBlock->dominates(*block),
                           "Instruction is not dominated by its operands");

                
                
                if (opBlock == *block && !op->isPhi()) {
                    MInstructionIterator opIter = block->begin(op->toInstruction());
                    do {
                        ++opIter;
                        MOZ_ASSERT(opIter != block->end(),
                                   "Operand in same block as instruction does not precede");
                    } while (*opIter != ins);
                }
            }
            AssertIfResumableInstruction(ins);
            if (MResumePoint* resume = ins->resumePoint()) {
                AssertResumePointDominatedByOperands(resume);
                AssertResumableOperands(resume);
            }
        }

        
        if (MResumePoint* resume = block->entryResumePoint()) {
            AssertResumePointDominatedByOperands(resume);
            AssertResumableOperands(resume);
        }
        if (MResumePoint* resume = block->outerResumePoint()) {
            AssertResumePointDominatedByOperands(resume);
            AssertResumableOperands(resume);
        }
    }
#endif
}


struct BoundsCheckInfo
{
    MBoundsCheck* check;
    uint32_t validEnd;
};

typedef HashMap<uint32_t,
                BoundsCheckInfo,
                DefaultHasher<uint32_t>,
                JitAllocPolicy> BoundsCheckMap;


static HashNumber
BoundsCheckHashIgnoreOffset(MBoundsCheck* check)
{
    SimpleLinearSum indexSum = ExtractLinearSum(check->index());
    uintptr_t index = indexSum.term ? uintptr_t(indexSum.term) : 0;
    uintptr_t length = uintptr_t(check->length());
    return index ^ length;
}

static MBoundsCheck*
FindDominatingBoundsCheck(BoundsCheckMap& checks, MBoundsCheck* check, size_t index)
{
    
    
    
    
    
    
    
    HashNumber hash = BoundsCheckHashIgnoreOffset(check);
    BoundsCheckMap::Ptr p = checks.lookup(hash);
    if (!p || index >= p->value().validEnd) {
        
        BoundsCheckInfo info;
        info.check = check;
        info.validEnd = index + check->block()->numDominated();

        if(!checks.put(hash, info))
            return nullptr;

        return check;
    }

    return p->value().check;
}


SimpleLinearSum
jit::ExtractLinearSum(MDefinition* ins)
{
    if (ins->isBeta())
        ins = ins->getOperand(0);

    if (ins->type() != MIRType_Int32)
        return SimpleLinearSum(ins, 0);

    if (ins->isConstantValue()) {
        const Value& v = ins->constantValue();
        MOZ_ASSERT(v.isInt32());
        return SimpleLinearSum(nullptr, v.toInt32());
    } else if (ins->isAdd() || ins->isSub()) {
        MDefinition* lhs = ins->getOperand(0);
        MDefinition* rhs = ins->getOperand(1);
        if (lhs->type() == MIRType_Int32 && rhs->type() == MIRType_Int32) {
            SimpleLinearSum lsum = ExtractLinearSum(lhs);
            SimpleLinearSum rsum = ExtractLinearSum(rhs);

            if (lsum.term && rsum.term)
                return SimpleLinearSum(ins, 0);

            
            if (ins->isAdd()) {
                int32_t constant;
                if (!SafeAdd(lsum.constant, rsum.constant, &constant))
                    return SimpleLinearSum(ins, 0);
                return SimpleLinearSum(lsum.term ? lsum.term : rsum.term, constant);
            } else if (lsum.term) {
                int32_t constant;
                if (!SafeSub(lsum.constant, rsum.constant, &constant))
                    return SimpleLinearSum(ins, 0);
                return SimpleLinearSum(lsum.term, constant);
            }
        }
    }

    return SimpleLinearSum(ins, 0);
}



bool
jit::ExtractLinearInequality(MTest* test, BranchDirection direction,
                             SimpleLinearSum* plhs, MDefinition** prhs, bool* plessEqual)
{
    if (!test->getOperand(0)->isCompare())
        return false;

    MCompare* compare = test->getOperand(0)->toCompare();

    MDefinition* lhs = compare->getOperand(0);
    MDefinition* rhs = compare->getOperand(1);

    
    if (!compare->isInt32Comparison())
        return false;

    MOZ_ASSERT(lhs->type() == MIRType_Int32);
    MOZ_ASSERT(rhs->type() == MIRType_Int32);

    JSOp jsop = compare->jsop();
    if (direction == FALSE_BRANCH)
        jsop = NegateCompareOp(jsop);

    SimpleLinearSum lsum = ExtractLinearSum(lhs);
    SimpleLinearSum rsum = ExtractLinearSum(rhs);

    if (!SafeSub(lsum.constant, rsum.constant, &lsum.constant))
        return false;

    
    switch (jsop) {
      case JSOP_LE:
        *plessEqual = true;
        break;
      case JSOP_LT:
        
        if (!SafeAdd(lsum.constant, 1, &lsum.constant))
            return false;
        *plessEqual = true;
        break;
      case JSOP_GE:
        *plessEqual = false;
        break;
      case JSOP_GT:
        
        if (!SafeSub(lsum.constant, 1, &lsum.constant))
            return false;
        *plessEqual = false;
        break;
      default:
        return false;
    }

    *plhs = lsum;
    *prhs = rsum.term;

    return true;
}

static bool
TryEliminateBoundsCheck(BoundsCheckMap& checks, size_t blockIndex, MBoundsCheck* dominated, bool* eliminated)
{
    MOZ_ASSERT(!*eliminated);

    
    
    
    
    
    dominated->replaceAllUsesWith(dominated->index());

    if (!dominated->isMovable())
        return true;

    MBoundsCheck* dominating = FindDominatingBoundsCheck(checks, dominated, blockIndex);
    if (!dominating)
        return false;

    if (dominating == dominated) {
        
        return true;
    }

    
    
    if (dominating->length() != dominated->length())
        return true;

    SimpleLinearSum sumA = ExtractLinearSum(dominating->index());
    SimpleLinearSum sumB = ExtractLinearSum(dominated->index());

    
    if (sumA.term != sumB.term)
        return true;

    
    *eliminated = true;

    
    int32_t minimumA, maximumA, minimumB, maximumB;
    if (!SafeAdd(sumA.constant, dominating->minimum(), &minimumA) ||
        !SafeAdd(sumA.constant, dominating->maximum(), &maximumA) ||
        !SafeAdd(sumB.constant, dominated->minimum(), &minimumB) ||
        !SafeAdd(sumB.constant, dominated->maximum(), &maximumB))
    {
        return false;
    }

    
    
    int32_t newMinimum, newMaximum;
    if (!SafeSub(Min(minimumA, minimumB), sumA.constant, &newMinimum) ||
        !SafeSub(Max(maximumA, maximumB), sumA.constant, &newMaximum))
    {
        return false;
    }

    dominating->setMinimum(newMinimum);
    dominating->setMaximum(newMaximum);
    return true;
}

static void
TryEliminateTypeBarrierFromTest(MTypeBarrier* barrier, bool filtersNull, bool filtersUndefined,
                                MTest* test, BranchDirection direction, bool* eliminated)
{
    MOZ_ASSERT(filtersNull || filtersUndefined);

    
    
    
    
    
    

    

    
    MDefinition* input = barrier->input();
    MUnbox* inputUnbox = nullptr;
    if (input->isUnbox() && input->toUnbox()->mode() != MUnbox::Fallible) {
        inputUnbox = input->toUnbox();
        input = inputUnbox->input();
    }

    MDefinition* subject = nullptr;
    bool removeUndefined;
    bool removeNull;
    test->filtersUndefinedOrNull(direction == TRUE_BRANCH, &subject, &removeUndefined, &removeNull);

    
    if (!subject)
        return;

    
    if (subject != input)
        return;

    
    
    if (!removeUndefined && filtersUndefined)
        return;

    
    
    if (!removeNull && filtersNull)
        return;

    
    
    *eliminated = true;
    if (inputUnbox)
        inputUnbox->makeInfallible();
    barrier->replaceAllUsesWith(barrier->input());
}

static bool
TryEliminateTypeBarrier(MTypeBarrier* barrier, bool* eliminated)
{
    MOZ_ASSERT(!*eliminated);

    const TemporaryTypeSet* barrierTypes = barrier->resultTypeSet();
    const TemporaryTypeSet* inputTypes = barrier->input()->resultTypeSet();

    
    if (barrier->input()->isUnbox() && barrier->input()->toUnbox()->mode() != MUnbox::Fallible)
        inputTypes = barrier->input()->toUnbox()->input()->resultTypeSet();

    if (!barrierTypes || !inputTypes)
        return true;

    bool filtersNull = barrierTypes->filtersType(inputTypes, TypeSet::NullType());
    bool filtersUndefined = barrierTypes->filtersType(inputTypes, TypeSet::UndefinedType());

    if (!filtersNull && !filtersUndefined)
        return true;

    MBasicBlock* block = barrier->block();
    while (true) {
        BranchDirection direction;
        MTest* test = block->immediateDominatorBranch(&direction);

        if (test) {
            TryEliminateTypeBarrierFromTest(barrier, filtersNull, filtersUndefined,
                                            test, direction, eliminated);
        }

        MBasicBlock* previous = block->immediateDominator();
        if (previous == block)
            break;
        block = previous;
    }

    return true;
}

static bool
TryOptimizeLoadObjectOrNull(MDefinition* def, MDefinitionVector* peliminateList)
{
    if (def->type() != MIRType_Value)
        return true;

    
    TemporaryTypeSet* types = def->resultTypeSet();
    if (!types)
        return true;
    if (types->baseFlags() & ~(TYPE_FLAG_NULL | TYPE_FLAG_ANYOBJECT))
        return true;

    MDefinitionVector eliminateList(def->block()->graph().alloc());

    for (MUseDefIterator iter(def); iter; ++iter) {
        MDefinition* ndef = iter.def();
        switch (ndef->op()) {
          case MDefinition::Op_Compare:
            if (ndef->toCompare()->compareType() != MCompare::Compare_Null)
                return true;
            break;
          case MDefinition::Op_Test:
            break;
          case MDefinition::Op_PostWriteBarrier:
            break;
          case MDefinition::Op_StoreFixedSlot:
            break;
          case MDefinition::Op_StoreSlot:
            break;
          case MDefinition::Op_ToObjectOrNull:
            if (!eliminateList.append(ndef->toToObjectOrNull()))
                return false;
            break;
          case MDefinition::Op_Unbox:
            if (ndef->type() != MIRType_Object)
                return true;
            break;
          case MDefinition::Op_TypeBarrier:
            
            
            if (ndef->hasUses() || ndef->resultTypeSet()->getKnownMIRType() != MIRType_Null)
                return true;
            break;
          default:
            return true;
        }
    }

    
    
#ifdef JS_PUNBOX64
    bool foundUse = false;
    for (MUseDefIterator iter(def); iter; ++iter) {
        MDefinition* ndef = iter.def();
        if (!ndef->isStoreFixedSlot() && !ndef->isStoreSlot()) {
            foundUse = true;
            break;
        }
    }
    if (!foundUse)
        return true;
#endif 

    def->setResultType(MIRType_ObjectOrNull);

    
    for (MUseDefIterator iter(def); iter; ++iter) {
        MDefinition* ndef = iter.def();
        if (ndef->isTypeBarrier())
            ndef->setResultType(MIRType_ObjectOrNull);
    }

    
    for (size_t i = 0; i < eliminateList.length(); i++) {
        MDefinition* ndef = eliminateList[i];
        ndef->replaceAllUsesWith(def);
        if (!peliminateList->append(ndef))
            return false;
    }

    return true;
}

static inline MDefinition*
PassthroughOperand(MDefinition* def)
{
    if (def->isConvertElementsToDoubles())
        return def->toConvertElementsToDoubles()->elements();
    if (def->isMaybeCopyElementsForWrite())
        return def->toMaybeCopyElementsForWrite()->object();
    if (def->isConvertUnboxedObjectToNative())
        return def->toConvertUnboxedObjectToNative()->object();
    return nullptr;
}














bool
jit::EliminateRedundantChecks(MIRGraph& graph)
{
    BoundsCheckMap checks(graph.alloc());

    if (!checks.init())
        return false;

    
    Vector<MBasicBlock*, 1, JitAllocPolicy> worklist(graph.alloc());

    
    size_t index = 0;

    
    
    for (MBasicBlockIterator i(graph.begin()); i != graph.end(); i++) {
        MBasicBlock* block = *i;
        if (block->immediateDominator() == block) {
            if (!worklist.append(block))
                return false;
        }
    }

    MDefinitionVector eliminateList(graph.alloc());

    
    while (!worklist.empty()) {
        MBasicBlock* block = worklist.popCopy();

        
        if (!worklist.append(block->immediatelyDominatedBlocksBegin(),
                             block->immediatelyDominatedBlocksEnd())) {
            return false;
        }

        for (MDefinitionIterator iter(block); iter; ) {
            MDefinition* def = *iter++;

            bool eliminated = false;

            switch (def->op()) {
              case MDefinition::Op_BoundsCheck:
                if (!TryEliminateBoundsCheck(checks, index, def->toBoundsCheck(), &eliminated))
                    return false;
                break;
              case MDefinition::Op_TypeBarrier:
                if (!TryEliminateTypeBarrier(def->toTypeBarrier(), &eliminated))
                    return false;
                break;
              case MDefinition::Op_LoadFixedSlot:
              case MDefinition::Op_LoadSlot:
              case MDefinition::Op_LoadUnboxedObjectOrNull:
                if (!TryOptimizeLoadObjectOrNull(def, &eliminateList))
                    return false;
                break;
              default:
                
                
                
                if (MDefinition* passthrough = PassthroughOperand(def))
                    def->replaceAllUsesWith(passthrough);
                break;
            }

            if (eliminated)
                block->discardDef(def);
        }
        index++;
    }

    MOZ_ASSERT(index == graph.numBlocks());

    for (size_t i = 0; i < eliminateList.length(); i++) {
        MDefinition* def = eliminateList[i];
        def->block()->discardDef(def);
    }

    return true;
}

bool
LinearSum::multiply(int32_t scale)
{
    for (size_t i = 0; i < terms_.length(); i++) {
        if (!SafeMul(scale, terms_[i].scale, &terms_[i].scale))
            return false;
    }
    return SafeMul(scale, constant_, &constant_);
}

bool
LinearSum::divide(int32_t scale)
{
    MOZ_ASSERT(scale > 0);

    for (size_t i = 0; i < terms_.length(); i++) {
        if (terms_[i].scale % scale != 0)
            return false;
    }
    if (constant_ % scale != 0)
        return false;

    for (size_t i = 0; i < terms_.length(); i++)
        terms_[i].scale /= scale;
    constant_ /= scale;

    return true;
}

bool
LinearSum::add(const LinearSum& other, int32_t scale )
{
    for (size_t i = 0; i < other.terms_.length(); i++) {
        int32_t newScale = scale;
        if (!SafeMul(scale, other.terms_[i].scale, &newScale))
            return false;
        if (!add(other.terms_[i].term, newScale))
            return false;
    }
    int32_t newConstant = scale;
    if (!SafeMul(scale, other.constant_, &newConstant))
        return false;
    return add(newConstant);
}

bool
LinearSum::add(SimpleLinearSum other, int32_t scale)
{
    if (other.term && !add(other.term, scale))
        return false;

    int32_t constant;
    if (!SafeMul(other.constant, scale, &constant))
        return false;

    return add(constant);
}

bool
LinearSum::add(MDefinition* term, int32_t scale)
{
    MOZ_ASSERT(term);

    if (scale == 0)
        return true;

    if (term->isConstantValue()) {
        int32_t constant = term->constantValue().toInt32();
        if (!SafeMul(constant, scale, &constant))
            return false;
        return add(constant);
    }

    for (size_t i = 0; i < terms_.length(); i++) {
        if (term == terms_[i].term) {
            if (!SafeAdd(scale, terms_[i].scale, &terms_[i].scale))
                return false;
            if (terms_[i].scale == 0) {
                terms_[i] = terms_.back();
                terms_.popBack();
            }
            return true;
        }
    }

    if (!terms_.append(LinearTerm(term, scale)))
        CrashAtUnhandlableOOM("LinearSum::add");

    return true;
}

bool
LinearSum::add(int32_t constant)
{
    return SafeAdd(constant, constant_, &constant_);
}

void
LinearSum::print(Sprinter& sp) const
{
    for (size_t i = 0; i < terms_.length(); i++) {
        int32_t scale = terms_[i].scale;
        int32_t id = terms_[i].term->id();
        MOZ_ASSERT(scale);
        if (scale > 0) {
            if (i)
                sp.printf("+");
            if (scale == 1)
                sp.printf("#%d", id);
            else
                sp.printf("%d*#%d", scale, id);
        } else if (scale == -1) {
            sp.printf("-#%d", id);
        } else {
            sp.printf("%d*#%d", scale, id);
        }
    }
    if (constant_ > 0)
        sp.printf("+%d", constant_);
    else if (constant_ < 0)
        sp.printf("%d", constant_);
}

void
LinearSum::dump(FILE* fp) const
{
    Sprinter sp(GetJitContext()->cx);
    sp.init();
    print(sp);
    fprintf(fp, "%s\n", sp.string());
}

void
LinearSum::dump() const
{
    dump(stderr);
}

MDefinition*
jit::ConvertLinearSum(TempAllocator& alloc, MBasicBlock* block, const LinearSum& sum, bool convertConstant)
{
    MDefinition* def = nullptr;

    for (size_t i = 0; i < sum.numTerms(); i++) {
        LinearTerm term = sum.term(i);
        MOZ_ASSERT(!term.term->isConstantValue());
        if (term.scale == 1) {
            if (def) {
                def = MAdd::New(alloc, def, term.term);
                def->toAdd()->setInt32();
                block->insertAtEnd(def->toInstruction());
                def->computeRange(alloc);
            } else {
                def = term.term;
            }
        } else if (term.scale == -1) {
            if (!def) {
                def = MConstant::New(alloc, Int32Value(0));
                block->insertAtEnd(def->toInstruction());
                def->computeRange(alloc);
            }
            def = MSub::New(alloc, def, term.term);
            def->toSub()->setInt32();
            block->insertAtEnd(def->toInstruction());
            def->computeRange(alloc);
        } else {
            MOZ_ASSERT(term.scale != 0);
            MConstant* factor = MConstant::New(alloc, Int32Value(term.scale));
            block->insertAtEnd(factor);
            MMul* mul = MMul::New(alloc, term.term, factor);
            mul->setInt32();
            block->insertAtEnd(mul);
            mul->computeRange(alloc);
            if (def) {
                def = MAdd::New(alloc, def, mul);
                def->toAdd()->setInt32();
                block->insertAtEnd(def->toInstruction());
                def->computeRange(alloc);
            } else {
                def = mul;
            }
        }
    }

    if (convertConstant && sum.constant()) {
        MConstant* constant = MConstant::New(alloc, Int32Value(sum.constant()));
        block->insertAtEnd(constant);
        constant->computeRange(alloc);
        if (def) {
            def = MAdd::New(alloc, def, constant);
            def->toAdd()->setInt32();
            block->insertAtEnd(def->toInstruction());
            def->computeRange(alloc);
        } else {
            def = constant;
        }
    }

    if (!def) {
        def = MConstant::New(alloc, Int32Value(0));
        block->insertAtEnd(def->toInstruction());
        def->computeRange(alloc);
    }

    return def;
}

MCompare*
jit::ConvertLinearInequality(TempAllocator& alloc, MBasicBlock* block, const LinearSum& sum)
{
    LinearSum lhs(sum);

    
    MDefinition* rhsDef = nullptr;
    for (size_t i = 0; i < lhs.numTerms(); i++) {
        if (lhs.term(i).scale == -1) {
            rhsDef = lhs.term(i).term;
            lhs.add(rhsDef, 1);
            break;
        }
    }

    MDefinition* lhsDef = nullptr;
    JSOp op = JSOP_GE;

    do {
        if (!lhs.numTerms()) {
            lhsDef = MConstant::New(alloc, Int32Value(lhs.constant()));
            block->insertAtEnd(lhsDef->toInstruction());
            lhsDef->computeRange(alloc);
            break;
        }

        lhsDef = ConvertLinearSum(alloc, block, lhs);
        if (lhs.constant() == 0)
            break;

        if (lhs.constant() == -1) {
            op = JSOP_GT;
            break;
        }

        if (!rhsDef) {
            int32_t constant = lhs.constant();
            if (SafeMul(constant, -1, &constant)) {
                rhsDef = MConstant::New(alloc, Int32Value(constant));
                block->insertAtEnd(rhsDef->toInstruction());
                rhsDef->computeRange(alloc);
                break;
            }
        }

        MDefinition* constant = MConstant::New(alloc, Int32Value(lhs.constant()));
        block->insertAtEnd(constant->toInstruction());
        constant->computeRange(alloc);
        lhsDef = MAdd::New(alloc, lhsDef, constant);
        lhsDef->toAdd()->setInt32();
        block->insertAtEnd(lhsDef->toInstruction());
        lhsDef->computeRange(alloc);
    } while (false);

    if (!rhsDef) {
        rhsDef = MConstant::New(alloc, Int32Value(0));
        block->insertAtEnd(rhsDef->toInstruction());
        rhsDef->computeRange(alloc);
    }

    MCompare* compare = MCompare::New(alloc, lhsDef, rhsDef, op);
    block->insertAtEnd(compare);
    compare->setCompareType(MCompare::Compare_Int32);

    return compare;
}

static bool
AnalyzePoppedThis(JSContext* cx, ObjectGroup* group,
                  MDefinition* thisValue, MInstruction* ins, bool definitelyExecuted,
                  HandlePlainObject baseobj,
                  Vector<TypeNewScript::Initializer>* initializerList,
                  Vector<PropertyName*>* accessedProperties,
                  bool* phandled)
{
    
    

    if (ins->isCallSetProperty()) {
        MCallSetProperty* setprop = ins->toCallSetProperty();

        if (setprop->object() != thisValue)
            return true;

        if (setprop->name() == cx->names().prototype ||
            setprop->name() == cx->names().proto ||
            setprop->name() == cx->names().constructor)
        {
            return true;
        }

        
        if (baseobj->lookup(cx, NameToId(setprop->name()))) {
            *phandled = true;
            return true;
        }

        
        
        for (size_t i = 0; i < accessedProperties->length(); i++) {
            if ((*accessedProperties)[i] == setprop->name())
                return true;
        }

        
        if (!definitelyExecuted)
            return true;

        RootedId id(cx, NameToId(setprop->name()));
        if (!AddClearDefiniteGetterSetterForPrototypeChain(cx, group, id)) {
            
            
            return true;
        }

        
        DebugOnly<unsigned> slotSpan = baseobj->slotSpan();
        MOZ_ASSERT(!baseobj->containsPure(id));
        if (!baseobj->addDataProperty(cx, id, baseobj->slotSpan(), JSPROP_ENUMERATE))
            return false;
        MOZ_ASSERT(baseobj->slotSpan() != slotSpan);
        MOZ_ASSERT(!baseobj->inDictionaryMode());

        Vector<MResumePoint*> callerResumePoints(cx);
        for (MResumePoint* rp = ins->block()->callerResumePoint();
             rp;
             rp = rp->block()->callerResumePoint())
        {
            if (!callerResumePoints.append(rp))
                return false;
        }

        for (int i = callerResumePoints.length() - 1; i >= 0; i--) {
            MResumePoint* rp = callerResumePoints[i];
            JSScript* script = rp->block()->info().script();
            TypeNewScript::Initializer entry(TypeNewScript::Initializer::SETPROP_FRAME,
                                             script->pcToOffset(rp->pc()));
            if (!initializerList->append(entry))
                return false;
        }

        JSScript* script = ins->block()->info().script();
        TypeNewScript::Initializer entry(TypeNewScript::Initializer::SETPROP,
                                         script->pcToOffset(setprop->resumePoint()->pc()));
        if (!initializerList->append(entry))
            return false;

        *phandled = true;
        return true;
    }

    if (ins->isCallGetProperty()) {
        MCallGetProperty* get = ins->toCallGetProperty();

        










        RootedId id(cx, NameToId(get->name()));
        if (!baseobj->lookup(cx, id) && !accessedProperties->append(get->name()))
            return false;

        if (!AddClearDefiniteGetterSetterForPrototypeChain(cx, group, id)) {
            
            
            return true;
        }

        *phandled = true;
        return true;
    }

    if (ins->isPostWriteBarrier()) {
        *phandled = true;
        return true;
    }

    return true;
}

static int
CmpInstructions(const void* a, const void* b)
{
    return (*static_cast<MInstruction * const*>(a))->id() -
           (*static_cast<MInstruction * const*>(b))->id();
}

bool
jit::AnalyzeNewScriptDefiniteProperties(JSContext* cx, JSFunction* fun,
                                        ObjectGroup* group, HandlePlainObject baseobj,
                                        Vector<TypeNewScript::Initializer>* initializerList)
{
    MOZ_ASSERT(cx->zone()->types.activeAnalysis);

    
    
    

    RootedScript script(cx, fun->getOrCreateScript(cx));
    if (!script)
        return false;

    if (!jit::IsIonEnabled(cx) || !jit::IsBaselineEnabled(cx) || !script->canBaselineCompile())
        return true;

    static const uint32_t MAX_SCRIPT_SIZE = 2000;
    if (script->length() > MAX_SCRIPT_SIZE)
        return true;

    Vector<PropertyName*> accessedProperties(cx);

    LifoAlloc alloc(TempAllocator::PreferredLifoChunkSize);
    TempAllocator temp(&alloc);
    JitContext jctx(cx, &temp);

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    if (!script->hasBaselineScript()) {
        MethodStatus status = BaselineCompile(cx, script);
        if (status == Method_Error)
            return false;
        if (status != Method_Compiled)
            return true;
    }

    TypeScript::SetThis(cx, script, TypeSet::ObjectType(group));

    MIRGraph graph(&temp);
    InlineScriptTree* inlineScriptTree = InlineScriptTree::New(&temp, nullptr, nullptr, script);
    if (!inlineScriptTree)
        return false;

    CompileInfo info(script, fun,
                      nullptr,  false,
                     Analysis_DefiniteProperties,
                     script->needsArgsObj(),
                     inlineScriptTree);

    const OptimizationInfo* optimizationInfo = js_IonOptimizations.get(Optimization_Normal);

    CompilerConstraintList* constraints = NewCompilerConstraintList(temp);
    if (!constraints) {
        ReportOutOfMemory(cx);
        return false;
    }

    BaselineInspector inspector(script);
    const JitCompileOptions options(cx);

    IonBuilder builder(cx, CompileCompartment::get(cx->compartment()), options, &temp, &graph, constraints,
                       &inspector, &info, optimizationInfo,  nullptr);

    if (!builder.build()) {
        if (builder.abortReason() == AbortReason_Alloc)
            return false;
        return true;
    }

    FinishDefinitePropertiesAnalysis(cx, constraints);

    if (!SplitCriticalEdges(graph))
        return false;

    if (!RenumberBlocks(graph))
        return false;

    if (!BuildDominatorTree(graph))
        return false;

    if (!EliminatePhis(&builder, graph, AggressiveObservability))
        return false;

    MDefinition* thisValue = graph.entryBlock()->getSlot(info.thisSlot());

    
    
    Vector<MInstruction*> instructions(cx);

    for (MUseDefIterator uses(thisValue); uses; uses++) {
        MDefinition* use = uses.def();

        
        if (!use->isInstruction())
            return true;

        if (!instructions.append(use->toInstruction()))
            return false;
    }

    
    qsort(instructions.begin(), instructions.length(),
          sizeof(MInstruction*), CmpInstructions);

    
    Vector<MBasicBlock*> exitBlocks(cx);
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (!block->numSuccessors() && !exitBlocks.append(*block))
            return false;
    }

    
    size_t lastAddedBlock = 0;

    for (size_t i = 0; i < instructions.length(); i++) {
        MInstruction* ins = instructions[i];

        
        
        bool definitelyExecuted = true;
        for (size_t i = 0; i < exitBlocks.length(); i++) {
            for (MBasicBlock* exit = exitBlocks[i];
                 exit != ins->block();
                 exit = exit->immediateDominator())
            {
                if (exit == exit->immediateDominator()) {
                    definitelyExecuted = false;
                    break;
                }
            }
        }

        
        
        
        
        if (ins->block()->loopDepth() != 0)
            definitelyExecuted = false;

        bool handled = false;
        size_t slotSpan = baseobj->slotSpan();
        if (!AnalyzePoppedThis(cx, group, thisValue, ins, definitelyExecuted,
                               baseobj, initializerList, &accessedProperties, &handled))
        {
            return false;
        }
        if (!handled)
            break;

        if (slotSpan != baseobj->slotSpan()) {
            MOZ_ASSERT(ins->block()->id() >= lastAddedBlock);
            lastAddedBlock = ins->block()->id();
        }
    }

    if (baseobj->slotSpan() != 0) {
        
        
        
        
        Vector<MBasicBlock*> exitBlocks(cx);
        for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
            
            
            if (block->id() > lastAddedBlock)
                break;
            if (MResumePoint* rp = block->callerResumePoint()) {
                if (block->numPredecessors() == 1 && block->getPredecessor(0) == rp->block()) {
                    JSScript* script = rp->block()->info().script();
                    if (!AddClearDefiniteFunctionUsesInScript(cx, group, script, block->info().script()))
                        return false;
                }
            }
        }
    }

    return true;
}

static bool
ArgumentsUseCanBeLazy(JSContext* cx, JSScript* script, MInstruction* ins, size_t index,
                      bool* argumentsContentsObserved)
{
    
    if (ins->isCall()) {
        if (*ins->toCall()->resumePoint()->pc() == JSOP_FUNAPPLY &&
            ins->toCall()->numActualArgs() == 2 &&
            index == MCall::IndexOfArgument(1))
        {
            *argumentsContentsObserved = true;
            return true;
        }
    }

    
    if (ins->isCallGetElement() && index == 0) {
        *argumentsContentsObserved = true;
        return true;
    }

    
    if (ins->isGetArgumentsObjectArg() && index == 0)
        return true;

    
    
    if (ins->isCallGetProperty() && index == 0 &&
        (ins->toCallGetProperty()->name() == cx->names().length ||
         (!script->strict() && ins->toCallGetProperty()->name() == cx->names().callee)))
    {
        return true;
    }

    return false;
}

bool
jit::AnalyzeArgumentsUsage(JSContext* cx, JSScript* scriptArg)
{
    RootedScript script(cx, scriptArg);
    AutoEnterAnalysis enter(cx);

    MOZ_ASSERT(!script->analyzedArgsUsage());

    
    
    
    
    script->setNeedsArgsObj(true);

    
    
    
    
    if (scriptArg->isDebuggee() || script->isGenerator())
        return true;

    
    
    
    
    
    
    
    
    if (script->bindingsAccessedDynamically()) {
        script->setNeedsArgsObj(false);
        return true;
    }

    if (!jit::IsIonEnabled(cx))
        return true;

    static const uint32_t MAX_SCRIPT_SIZE = 10000;
    if (script->length() > MAX_SCRIPT_SIZE)
        return true;

    if (!script->ensureHasTypes(cx))
        return false;

    LifoAlloc alloc(TempAllocator::PreferredLifoChunkSize);
    TempAllocator temp(&alloc);
    JitContext jctx(cx, &temp);

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    MIRGraph graph(&temp);
    InlineScriptTree* inlineScriptTree = InlineScriptTree::New(&temp, nullptr, nullptr, script);
    if (!inlineScriptTree)
        return false;
    CompileInfo info(script, script->functionNonDelazifying(),
                      nullptr,  false,
                     Analysis_ArgumentsUsage,
                      true,
                     inlineScriptTree);

    const OptimizationInfo* optimizationInfo = js_IonOptimizations.get(Optimization_Normal);

    CompilerConstraintList* constraints = NewCompilerConstraintList(temp);
    if (!constraints)
        return false;

    BaselineInspector inspector(script);
    const JitCompileOptions options(cx);

    IonBuilder builder(nullptr, CompileCompartment::get(cx->compartment()), options, &temp, &graph, constraints,
                       &inspector, &info, optimizationInfo,  nullptr);

    if (!builder.build()) {
        if (builder.abortReason() == AbortReason_Alloc)
            return false;
        return true;
    }

    if (!SplitCriticalEdges(graph))
        return false;

    if (!RenumberBlocks(graph))
        return false;

    if (!BuildDominatorTree(graph))
        return false;

    if (!EliminatePhis(&builder, graph, AggressiveObservability))
        return false;

    MDefinition* argumentsValue = graph.entryBlock()->getSlot(info.argsObjSlot());

    bool argumentsContentsObserved = false;

    for (MUseDefIterator uses(argumentsValue); uses; uses++) {
        MDefinition* use = uses.def();

        
        if (!use->isInstruction())
            return true;

        if (!ArgumentsUseCanBeLazy(cx, script, use->toInstruction(), use->indexOf(uses.use()),
                                   &argumentsContentsObserved))
        {
            return true;
        }
    }

    
    
    
    
    if (script->funHasAnyAliasedFormal() && argumentsContentsObserved)
        return true;

    script->setNeedsArgsObj(false);
    return true;
}




size_t
jit::MarkLoopBlocks(MIRGraph& graph, MBasicBlock* header, bool* canOsr)
{
#ifdef DEBUG
    for (ReversePostorderIterator i = graph.rpoBegin(), e = graph.rpoEnd(); i != e; ++i)
        MOZ_ASSERT(!i->isMarked(), "Some blocks already marked");
#endif

    MBasicBlock* osrBlock = graph.osrBlock();
    *canOsr = false;

    
    
    
    
    
    
    MBasicBlock* backedge = header->backedge();
    backedge->mark();
    size_t numMarked = 1;
    for (PostorderIterator i = graph.poBegin(backedge); ; ++i) {
        MOZ_ASSERT(i != graph.poEnd(),
                   "Reached the end of the graph while searching for the loop header");
        MBasicBlock* block = *i;
        
        if (block == header)
            break;
        
        if (!block->isMarked())
            continue;
        
        for (size_t p = 0, e = block->numPredecessors(); p != e; ++p) {
            MBasicBlock* pred = block->getPredecessor(p);
            if (pred->isMarked())
                continue;

            
            
            if (osrBlock && pred != header &&
                osrBlock->dominates(pred) && !osrBlock->dominates(header))
            {
                *canOsr = true;
                continue;
            }

            MOZ_ASSERT(pred->id() >= header->id() && pred->id() <= backedge->id(),
                       "Loop block not between loop header and loop backedge");

            pred->mark();
            ++numMarked;

            
            
            
            if (pred->isLoopHeader()) {
                MBasicBlock* innerBackedge = pred->backedge();
                if (!innerBackedge->isMarked()) {
                    
                    
                    innerBackedge->mark();
                    ++numMarked;

                    
                    
                    if (backedge->id() > block->id()) {
                        i = graph.poBegin(innerBackedge);
                        --i;
                    }
                }
            }
        }
    }

    
    
    
    if (!header->isMarked()) {
        jit::UnmarkLoopBlocks(graph, header);
        return 0;
    }

    return numMarked;
}


void
jit::UnmarkLoopBlocks(MIRGraph& graph, MBasicBlock* header)
{
    MBasicBlock* backedge = header->backedge();
    for (ReversePostorderIterator i = graph.rpoBegin(header); ; ++i) {
        MOZ_ASSERT(i != graph.rpoEnd(),
                   "Reached the end of the graph while searching for the backedge");
        MBasicBlock* block = *i;
        if (block->isMarked()) {
            block->unmark();
            if (block == backedge)
                break;
        }
    }

#ifdef DEBUG
    for (ReversePostorderIterator i = graph.rpoBegin(), e = graph.rpoEnd(); i != e; ++i)
        MOZ_ASSERT(!i->isMarked(), "Not all blocks got unmarked");
#endif
}


static void
MakeLoopContiguous(MIRGraph& graph, MBasicBlock* header, size_t numMarked)
{
    MBasicBlock* backedge = header->backedge();

    MOZ_ASSERT(header->isMarked(), "Loop header is not part of loop");
    MOZ_ASSERT(backedge->isMarked(), "Loop backedge is not part of loop");

    
    
    
    ReversePostorderIterator insertIter = graph.rpoBegin(backedge);
    insertIter++;
    MBasicBlock* insertPt = *insertIter;

    
    size_t headerId = header->id();
    size_t inLoopId = headerId;
    size_t notInLoopId = inLoopId + numMarked;
    ReversePostorderIterator i = graph.rpoBegin(header);
    for (;;) {
        MBasicBlock* block = *i++;
        MOZ_ASSERT(block->id() >= header->id() && block->id() <= backedge->id(),
                   "Loop backedge should be last block in loop");

        if (block->isMarked()) {
            
            block->unmark();
            block->setId(inLoopId++);
            
            if (block == backedge)
                break;
        } else {
            
            graph.moveBlockBefore(insertPt, block);
            block->setId(notInLoopId++);
        }
    }
    MOZ_ASSERT(header->id() == headerId, "Loop header id changed");
    MOZ_ASSERT(inLoopId == headerId + numMarked, "Wrong number of blocks kept in loop");
    MOZ_ASSERT(notInLoopId == (insertIter != graph.rpoEnd() ? insertPt->id() : graph.numBlocks()),
               "Wrong number of blocks moved out of loop");
}


bool
jit::MakeLoopsContiguous(MIRGraph& graph)
{
    
    for (MBasicBlockIterator i(graph.begin()); i != graph.end(); i++) {
        MBasicBlock* header = *i;
        if (!header->isLoopHeader())
            continue;

        
        bool canOsr;
        size_t numMarked = MarkLoopBlocks(graph, header, &canOsr);

        
        if (numMarked == 0)
            continue;

        
        
        if (canOsr) {
            UnmarkLoopBlocks(graph, header);
            continue;
        }

        
        
        MakeLoopContiguous(graph, header, numMarked);
    }

    return true;
}
