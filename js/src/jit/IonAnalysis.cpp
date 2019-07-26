





#include "jit/IonAnalysis.h"

#include "jit/BaselineInspector.h"
#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/IonBuilder.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/LIR.h"
#include "jit/Lowering.h"
#include "jit/MIRGraph.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;




bool
jit::SplitCriticalEdges(MIRGraph &graph)
{
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (block->numSuccessors() < 2)
            continue;
        for (size_t i = 0; i < block->numSuccessors(); i++) {
            MBasicBlock *target = block->getSuccessor(i);
            if (target->numPredecessors() < 2)
                continue;

            
            MBasicBlock *split = MBasicBlock::NewSplitEdge(graph, block->info(), *block);
            if (!split)
                return false;
            split->setLoopDepth(block->loopDepth());
            graph.insertBlockAfter(*block, split);
            split->end(MGoto::New(graph.alloc(), target));

            block->replaceSuccessor(i, split);
            target->replacePredecessor(*block, split);
        }
    }
    return true;
}











bool
jit::EliminateDeadResumePointOperands(MIRGenerator *mir, MIRGraph &graph)
{
    
    
    
    if (graph.hasTryBlock())
        return true;

    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Dead Resume Point Operands (main loop)"))
            return false;

        
        if (block->isLoopHeader() && block->backedge() == *block)
            continue;

        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            
            if (ins->isConstant())
                continue;

            
            
            
            
            
            if (ins->isUnbox() || ins->isParameter() || ins->isTypeBarrier() || ins->isComputeThis())
                continue;

            
            
            
            if (ins->isNewDerivedTypedObject()) {
                MOZ_ASSERT(ins->canRecoverOnBailout());
                continue;
            }

            
            
            
            if (ins->isImplicitlyUsed())
                continue;

            
            
            
            
            
            uint32_t maxDefinition = 0;
            for (MUseIterator uses(ins->usesBegin()); uses != ins->usesEnd(); uses++) {
                MNode *consumer = uses->consumer();
                if (consumer->isResumePoint()) {
                    
                    
                    
                    MResumePoint *resume = consumer->toResumePoint();
                    if (resume->isObservableOperand(*uses)) {
                        maxDefinition = UINT32_MAX;
                        break;
                    }
                    continue;
                }

                MDefinition *def = consumer->toDefinition();
                if (def->block() != *block || def->isBox() || def->isPhi()) {
                    maxDefinition = UINT32_MAX;
                    break;
                }
                maxDefinition = Max(maxDefinition, def->id());
            }
            if (maxDefinition == UINT32_MAX)
                continue;

            
            
            for (MUseIterator uses(ins->usesBegin()); uses != ins->usesEnd(); ) {
                if (uses->consumer()->isDefinition()) {
                    uses++;
                    continue;
                }
                MResumePoint *mrp = uses->consumer()->toResumePoint();
                if (mrp->block() != *block ||
                    !mrp->instruction() ||
                    mrp->instruction() == *ins ||
                    mrp->instruction()->id() <= maxDefinition)
                {
                    uses++;
                    continue;
                }

                
                
                
                
                
                
                
                
                MConstant *constant = MConstant::New(graph.alloc(), MagicValue(JS_OPTIMIZED_OUT));
                block->insertBefore(*(block->begin()), constant);
                uses = mrp->replaceOperand(uses, constant);
            }
        }
    }

    return true;
}




bool
jit::EliminateDeadCode(MIRGenerator *mir, MIRGraph &graph)
{
    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Dead Code (main loop)"))
            return false;

        
        for (MInstructionReverseIterator inst = block->rbegin(); inst != block->rend(); ) {
            if (!inst->isEffectful() && !inst->resumePoint() &&
                !inst->hasUses() && !inst->isGuard() &&
                !inst->isControlInstruction()) {
                inst = block->discardAt(inst);
            } else if (!inst->hasLiveDefUses() && inst->canRecoverOnBailout()) {
                inst->setRecoveredOnBailout();
                inst++;
            } else {
                inst++;
            }
        }
    }

    return true;
}

static inline bool
IsPhiObservable(MPhi *phi, Observability observe)
{
    
    
    if (phi->isImplicitlyUsed())
        return true;

    
    
    
    
    
    
    
    
    for (MUseIterator iter(phi->usesBegin()); iter != phi->usesEnd(); iter++) {
        MNode *consumer = iter->consumer();
        if (consumer->isResumePoint()) {
            MResumePoint *resume = consumer->toResumePoint();
            if (observe == ConservativeObservability)
                return true;
            if (resume->isObservableOperand(*iter))
                return true;
        } else {
            MDefinition *def = consumer->toDefinition();
            if (!def->isPhi())
                return true;
        }
    }

    return false;
}




static inline MDefinition *
IsPhiRedundant(MPhi *phi)
{
    MDefinition *first = phi->operandIfRedundant();
    if (first == nullptr)
        return nullptr;

    
    if (phi->isImplicitlyUsed())
        first->setImplicitlyUsedUnchecked();

    return first;
}

bool
jit::EliminatePhis(MIRGenerator *mir, MIRGraph &graph,
                   Observability observe)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    Vector<MPhi *, 16, SystemAllocPolicy> worklist;

    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Phis (populate loop)"))
            return false;

        MPhiIterator iter = block->phisBegin();
        while (iter != block->phisEnd()) {
            
            
            iter->setUnused();

            
            if (MDefinition *redundant = IsPhiRedundant(*iter)) {
                iter->replaceAllUsesWith(redundant);
                iter = block->discardPhiAt(iter);
                continue;
            }

            
            if (IsPhiObservable(*iter, observe)) {
                iter->setInWorklist();
                if (!worklist.append(*iter))
                    return false;
            }
            iter++;
        }
    }

    
    while (!worklist.empty()) {
        if (mir->shouldCancel("Eliminate Phis (worklist)"))
            return false;

        MPhi *phi = worklist.popCopy();
        JS_ASSERT(phi->isUnused());
        phi->setNotInWorklist();

        
        if (MDefinition *redundant = IsPhiRedundant(phi)) {
            
            for (MUseDefIterator it(phi); it; it++) {
                if (it.def()->isPhi()) {
                    MPhi *use = it.def()->toPhi();
                    if (!use->isUnused()) {
                        use->setUnusedUnchecked();
                        use->setInWorklist();
                        if (!worklist.append(use))
                            return false;
                    }
                }
            }
            phi->replaceAllUsesWith(redundant);
        } else {
            
            phi->setNotUnused();
        }

        
        for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
            MDefinition *in = phi->getOperand(i);
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
            if (iter->isUnused())
                iter = block->discardPhiAt(iter);
            else
                iter++;
        }
    }

    return true;
}

namespace {











class TypeAnalyzer
{
    MIRGenerator *mir;
    MIRGraph &graph;
    Vector<MPhi *, 0, SystemAllocPolicy> phiWorklist_;

    TempAllocator &alloc() const {
        return graph.alloc();
    }

    bool addPhiToWorklist(MPhi *phi) {
        if (phi->isInWorklist())
            return true;
        if (!phiWorklist_.append(phi))
            return false;
        phi->setInWorklist();
        return true;
    }
    MPhi *popPhi() {
        MPhi *phi = phiWorklist_.popCopy();
        phi->setNotInWorklist();
        return phi;
    }

    bool respecialize(MPhi *phi, MIRType type);
    bool propagateSpecialization(MPhi *phi);
    bool specializePhis();
    void replaceRedundantPhi(MPhi *phi);
    void adjustPhiInputs(MPhi *phi);
    bool adjustInputs(MDefinition *def);
    bool insertConversions();

    bool checkFloatCoherency();
    bool graphContainsFloat32();
    bool markPhiConsumers();
    bool markPhiProducers();
    bool specializeValidFloatOps();
    bool tryEmitFloatOperations();

  public:
    TypeAnalyzer(MIRGenerator *mir, MIRGraph &graph)
      : mir(mir), graph(graph)
    { }

    bool analyze();
};

} 


static MIRType
GuessPhiType(MPhi *phi, bool *hasInputsWithEmptyTypes)
{
#ifdef DEBUG
    
    
    
    MIRType magicType = MIRType_None;
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition *in = phi->getOperand(i);
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
        MDefinition *in = phi->getOperand(i);
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
        
        
        JS_ASSERT(*hasInputsWithEmptyTypes);
        type = MIRType_Value;
    }

    return type;
}

bool
TypeAnalyzer::respecialize(MPhi *phi, MIRType type)
{
    if (phi->type() == type)
        return true;
    phi->specialize(type);
    return addPhiToWorklist(phi);
}

bool
TypeAnalyzer::propagateSpecialization(MPhi *phi)
{
    JS_ASSERT(phi->type() != MIRType_None);

    
    for (MUseDefIterator iter(phi); iter; iter++) {
        if (!iter.def()->isPhi())
            continue;
        MPhi *use = iter.def()->toPhi();
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
    Vector<MPhi *, 0, SystemAllocPolicy> phisWithEmptyInputTypes;

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

            MPhi *phi = popPhi();
            if (!propagateSpecialization(phi))
                return false;
        }

        
        
        
        while (!phisWithEmptyInputTypes.empty()) {
            if (mir->shouldCancel("Specialize Phis (phisWithEmptyInputTypes)"))
                return false;

            MPhi *phi = phisWithEmptyInputTypes.popCopy();
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
TypeAnalyzer::adjustPhiInputs(MPhi *phi)
{
    MIRType phiType = phi->type();
    JS_ASSERT(phiType != MIRType_None);

    
    
    
    
    if (phiType != MIRType_Value) {
        for (size_t i = 0, e = phi->numOperands(); i < e; i++) {
            MDefinition *in = phi->getOperand(i);
            if (in->type() == phiType)
                continue;

            if (in->isBox() && in->toBox()->input()->type() == phiType) {
                phi->replaceOperand(i, in->toBox()->input());
            } else {
                MInstruction *replacement;

                if (phiType == MIRType_Double && IsFloatType(in->type())) {
                    
                    replacement = MToDouble::New(alloc(), in);
                } else if (phiType == MIRType_Float32) {
                    if (in->type() == MIRType_Int32 || in->type() == MIRType_Double) {
                        replacement = MToFloat32::New(alloc(), in);
                    } else {
                        
                        if (in->type() != MIRType_Value) {
                            MBox *box = MBox::New(alloc(), in);
                            in->block()->insertBefore(in->block()->lastIns(), box);
                            in = box;
                        }

                        MUnbox *unbox = MUnbox::New(alloc(), in, MIRType_Double, MUnbox::Fallible);
                        in->block()->insertBefore(in->block()->lastIns(), unbox);
                        replacement = MToFloat32::New(alloc(), in);
                    }
                } else {
                    
                    
                    
                    if (in->type() != MIRType_Value) {
                        MBox *box = MBox::New(alloc(), in);
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
        MDefinition *in = phi->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;

        if (in->isUnbox() && phi->typeIncludes(in->toUnbox()->input())) {
            
            
            phi->replaceOperand(i, in->toUnbox()->input());
        } else {
            MDefinition *box = BoxInputsPolicy::alwaysBoxAt(alloc(), in->block()->lastIns(), in);
            phi->replaceOperand(i, box);
        }
    }
}

bool
TypeAnalyzer::adjustInputs(MDefinition *def)
{
    TypePolicy *policy = def->typePolicy();
    if (policy && !policy->adjustInputs(alloc(), def->toInstruction()))
        return false;
    return true;
}

void
TypeAnalyzer::replaceRedundantPhi(MPhi *phi)
{
    MBasicBlock *block = phi->block();
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
      default:
        MOZ_ASSUME_UNREACHABLE("unexpected type");
    }
    MConstant *c = MConstant::New(alloc(), v);
    
    block->insertBefore(*(block->begin()), c);
    phi->replaceAllUsesWith(c);
}

bool
TypeAnalyzer::insertConversions()
{
    
    
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Insert Conversions"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd();) {
            if (phi->type() == MIRType_Undefined ||
                phi->type() == MIRType_Null ||
                phi->type() == MIRType_MagicOptimizedArguments ||
                phi->type() == MIRType_MagicOptimizedOut)
            {
                replaceRedundantPhi(*phi);
                phi = block->discardPhiAt(phi);
            } else {
                adjustPhiInputs(*phi);
                phi++;
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
    JS_ASSERT(phiWorklist_.empty());

    
    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); ++block) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Consumer Phis - Initial state"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); ++phi) {
            JS_ASSERT(!phi->isInWorklist());
            bool canConsumeFloat32 = true;
            for (MUseDefIterator use(*phi); canConsumeFloat32 && use; use++) {
                MDefinition *usedef = use.def();
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

        MPhi *phi = popPhi();
        JS_ASSERT(phi->canConsumeFloat32(nullptr ));

        bool validConsumer = true;
        for (MUseDefIterator use(phi); use; use++) {
            MDefinition *def = use.def();
            if (def->isPhi() && !def->canConsumeFloat32(use.use())) {
                validConsumer = false;
                break;
            }
        }

        if (validConsumer)
            continue;

        
        phi->setCanConsumeFloat32(false);
        for (size_t i = 0, e = phi->numOperands(); i < e; ++i) {
            MDefinition *input = phi->getOperand(i);
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
    JS_ASSERT(phiWorklist_.empty());

    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); ++block) {
        if (mir->shouldCancel("Ensure Float32 commutativity - Producer Phis - initial state"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); ++phi) {
            JS_ASSERT(!phi->isInWorklist());
            bool canProduceFloat32 = true;
            for (size_t i = 0, e = phi->numOperands(); canProduceFloat32 && i < e; ++i) {
                MDefinition *input = phi->getOperand(i);
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

        MPhi *phi = popPhi();
        JS_ASSERT(phi->canProduceFloat32());

        bool validProducer = true;
        for (size_t i = 0, e = phi->numOperands(); i < e; ++i) {
            MDefinition *input = phi->getOperand(i);
            if (input->isPhi() && !input->canProduceFloat32()) {
                validProducer = false;
                break;
            }
        }

        if (validProducer)
            continue;

        
        phi->setCanProduceFloat32(false);
        for (MUseDefIterator use(phi); use; use++) {
            MDefinition *def = use.def();
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
    
    
    if (!LIRGenerator::allowFloat32Optimizations())
        return true;

    
    
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
                MDefinition *consumer = use.def();
                JS_ASSERT(consumer->isConsistentFloat32Use(use.use()));
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
jit::ApplyTypeInformation(MIRGenerator *mir, MIRGraph &graph)
{
    TypeAnalyzer analyzer(mir, graph);

    if (!analyzer.analyze())
        return false;

    return true;
}

bool
jit::MakeMRegExpHoistable(MIRGraph &graph)
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++) {
            if (!iter->isRegExp())
                continue;

            MRegExp *regexp = iter->toRegExp();

            
            bool hoistable = true;
            for (MUseIterator i = regexp->usesBegin(); i != regexp->usesEnd(); i++) {
                
                
                if (i->consumer()->isResumePoint())
                    continue;

                JS_ASSERT(i->consumer()->isDefinition());

                
                MDefinition *use = i->consumer()->toDefinition();
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

            
            
            RegExpObject *source = regexp->source();
            if (source->sticky() || source->global()) {
                JS_ASSERT(regexp->mustClone());
                MConstant *zero = MConstant::New(graph.alloc(), Int32Value(0));
                regexp->block()->insertAfter(regexp, zero);

                MStoreFixedSlot *lastIndex =
                    MStoreFixedSlot::New(graph.alloc(), regexp, RegExpObject::lastIndexSlot(), zero);
                regexp->block()->insertAfter(zero, lastIndex);
            }
        }
    }

    return true;
}

bool
jit::RenumberBlocks(MIRGraph &graph)
{
    size_t id = 0;
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++)
        block->setId(id++);

    return true;
}



static MBasicBlock *
IntersectDominators(MBasicBlock *block1, MBasicBlock *block2)
{
    MBasicBlock *finger1 = block1;
    MBasicBlock *finger2 = block2;

    JS_ASSERT(finger1);
    JS_ASSERT(finger2);

    
    

    
    
    
    

    while (finger1->id() != finger2->id()) {
        while (finger1->id() > finger2->id()) {
            MBasicBlock *idom = finger1->immediateDominator();
            if (idom == finger1)
                return nullptr; 
            finger1 = idom;
        }

        while (finger2->id() > finger1->id()) {
            MBasicBlock *idom = finger2->immediateDominator();
            if (idom == finger2)
                return nullptr; 
            finger2 = idom;
        }
    }
    return finger1;
}

static void
ComputeImmediateDominators(MIRGraph &graph)
{
    
    MBasicBlock *startBlock = graph.entryBlock();
    startBlock->setImmediateDominator(startBlock);

    
    MBasicBlock *osrBlock = graph.osrBlock();
    if (osrBlock)
        osrBlock->setImmediateDominator(osrBlock);

    bool changed = true;

    while (changed) {
        changed = false;

        ReversePostorderIterator block = graph.rpoBegin();

        
        for (; block != graph.rpoEnd(); block++) {
            
            
            if (block->immediateDominator() == *block)
                continue;

            MBasicBlock *newIdom = block->getPredecessor(0);

            
            for (size_t i = 1; i < block->numPredecessors(); i++) {
                MBasicBlock *pred = block->getPredecessor(i);
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
        JS_ASSERT(block->immediateDominator() != nullptr);
    }
#endif
}

bool
jit::BuildDominatorTree(MIRGraph &graph)
{
    ComputeImmediateDominators(graph);

    
    
    
    
    
    for (PostorderIterator i(graph.poBegin()); i != graph.poEnd(); i++) {
        MBasicBlock *child = *i;
        MBasicBlock *parent = child->immediateDominator();

        
        if (child == parent)
            continue;

        if (!parent->addImmediatelyDominatedBlock(child))
            return false;

        
        parent->addNumDominated(child->numDominated() + 1);
    }

#ifdef DEBUG
    
    
    if (!graph.osrBlock())
        JS_ASSERT(graph.entryBlock()->numDominated() == graph.numBlocks() - 1);
#endif
    
    
    
    Vector<MBasicBlock *, 1, IonAllocPolicy> worklist(graph.alloc());

    
    size_t index = 0;

    
    
    for (MBasicBlockIterator i(graph.begin()); i != graph.end(); i++) {
        MBasicBlock *block = *i;
        if (block->immediateDominator() == block) {
            if (!worklist.append(block))
                return false;
        }
    }
    
    while (!worklist.empty()) {
        MBasicBlock *block = worklist.popCopy();
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
jit::BuildPhiReverseMapping(MIRGraph &graph)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (block->numPredecessors() < 2) {
            JS_ASSERT(block->phisEmpty());
            continue;
        }

        
        for (size_t j = 0; j < block->numPredecessors(); j++) {
            MBasicBlock *pred = block->getPredecessor(j);

#ifdef DEBUG
            size_t numSuccessorsWithPhis = 0;
            for (size_t k = 0; k < pred->numSuccessors(); k++) {
                MBasicBlock *successor = pred->getSuccessor(k);
                if (!successor->phisEmpty())
                    numSuccessorsWithPhis++;
            }
            JS_ASSERT(numSuccessorsWithPhis <= 1);
#endif

            pred->setSuccessorWithPhis(*block, j);
        }
    }

    return true;
}

#ifdef DEBUG
static bool
CheckSuccessorImpliesPredecessor(MBasicBlock *A, MBasicBlock *B)
{
    
    for (size_t i = 0; i < B->numPredecessors(); i++) {
        if (A == B->getPredecessor(i))
            return true;
    }
    return false;
}

static bool
CheckPredecessorImpliesSuccessor(MBasicBlock *A, MBasicBlock *B)
{
    
    for (size_t i = 0; i < B->numSuccessors(); i++) {
        if (A == B->getSuccessor(i))
            return true;
    }
    return false;
}

static bool
CheckOperandImpliesUse(MNode *n, MDefinition *operand)
{
    for (MUseIterator i = operand->usesBegin(); i != operand->usesEnd(); i++) {
        if (i->consumer() == n)
            return true;
    }
    return false;
}

static bool
CheckUseImpliesOperand(MDefinition *def, MUse *use)
{
    return use->consumer()->getOperand(use->index()) == def;
}
#endif 

void
jit::AssertBasicGraphCoherency(MIRGraph &graph)
{
#ifdef DEBUG
    JS_ASSERT(graph.entryBlock()->numPredecessors() == 0);
    JS_ASSERT(graph.entryBlock()->phisEmpty());
    JS_ASSERT(!graph.entryBlock()->unreachable());

    if (MBasicBlock *osrBlock = graph.osrBlock()) {
        JS_ASSERT(osrBlock->numPredecessors() == 0);
        JS_ASSERT(osrBlock->phisEmpty());
        JS_ASSERT(osrBlock != graph.entryBlock());
        JS_ASSERT(!osrBlock->unreachable());
    }

    if (MResumePoint *resumePoint = graph.entryResumePoint())
        JS_ASSERT(resumePoint->block() == graph.entryBlock());

    
    uint32_t count = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        count++;

        JS_ASSERT(&block->graph() == &graph);

        for (size_t i = 0; i < block->numSuccessors(); i++)
            JS_ASSERT(CheckSuccessorImpliesPredecessor(*block, block->getSuccessor(i)));

        for (size_t i = 0; i < block->numPredecessors(); i++)
            JS_ASSERT(CheckPredecessorImpliesSuccessor(*block, block->getPredecessor(i)));

        for (MResumePointIterator iter(block->resumePointsBegin()); iter != block->resumePointsEnd(); iter++) {
            for (uint32_t i = 0, e = iter->numOperands(); i < e; i++) {
                if (iter->getUseFor(i)->hasProducer())
                    JS_ASSERT(CheckOperandImpliesUse(*iter, iter->getOperand(i)));
            }
        }
        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
            JS_ASSERT(phi->numOperands() == block->numPredecessors());
            MOZ_ASSERT(!phi->isRecoveredOnBailout());
        }
        for (MDefinitionIterator iter(*block); iter; iter++) {
            JS_ASSERT(iter->block() == *block);

            
            for (uint32_t i = 0, end = iter->numOperands(); i < end; i++)
                JS_ASSERT(CheckOperandImpliesUse(*iter, iter->getOperand(i)));
            for (MUseIterator use(iter->usesBegin()); use != iter->usesEnd(); use++)
                JS_ASSERT(CheckUseImpliesOperand(*iter, *use));

            if (iter->isInstruction()) {
                if (MResumePoint *resume = iter->toInstruction()->resumePoint()) {
                    if (MInstruction *ins = resume->instruction())
                        JS_ASSERT(ins->block() == iter->block());
                }
            }

            if (iter->isRecoveredOnBailout())
                MOZ_ASSERT(!iter->hasLiveDefUses());
        }
    }

    JS_ASSERT(graph.numBlocks() == count);
#endif
}

#ifdef DEBUG
static void
AssertReversePostOrder(MIRGraph &graph)
{
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        JS_ASSERT(!block->isMarked());

        for (size_t i = 0; i < block->numPredecessors(); i++) {
            MBasicBlock *pred = block->getPredecessor(i);
            JS_ASSERT_IF(!pred->isLoopBackedge(), pred->isMarked());
        }

        block->mark();
    }

    graph.unmarkBlocks();
}
#endif

#ifdef DEBUG
static void
AssertDominatorTree(MIRGraph &graph)
{
    
    size_t i = graph.numBlocks();
    size_t totalNumDominated = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        MBasicBlock *idom = block->immediateDominator();
        JS_ASSERT(idom->dominates(*block));
        JS_ASSERT(idom == *block || idom->id() < block->id());

        if (idom == *block) {
            totalNumDominated += block->numDominated() + 1;
        } else {
            bool foundInParent = false;
            for (size_t j = 0; j < idom->numImmediatelyDominatedBlocks(); j++) {
                if (idom->getImmediatelyDominatedBlock(j) == *block) {
                    foundInParent = true;
                    break;
                }
            }
            JS_ASSERT(foundInParent);
        }

        size_t numDominated = 0;
        for (size_t j = 0; j < block->numImmediatelyDominatedBlocks(); j++) {
            MBasicBlock *dom = block->getImmediatelyDominatedBlock(j);
            JS_ASSERT(block->dominates(dom));
            JS_ASSERT(dom->id() > block->id());
            JS_ASSERT(dom->immediateDominator() == *block);

            numDominated += dom->numDominated() + 1;
        }
        JS_ASSERT(block->numDominated() == numDominated);
        JS_ASSERT(block->numDominated() + 1 <= i);
        JS_ASSERT(block->numSuccessors() != 0 || block->numDominated() == 0);
        i--;
    }
    JS_ASSERT(i == 0);
    JS_ASSERT(totalNumDominated == graph.numBlocks());
}
#endif

void
jit::AssertGraphCoherency(MIRGraph &graph)
{
#ifdef DEBUG
    if (!js_JitOptions.checkGraphConsistency)
        return;
    AssertBasicGraphCoherency(graph);
    AssertReversePostOrder(graph);
#endif
}

void
jit::AssertExtendedGraphCoherency(MIRGraph &graph)
{
    
    
    

#ifdef DEBUG
    if (!js_JitOptions.checkGraphConsistency)
        return;
    AssertGraphCoherency(graph);

    uint32_t idx = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        JS_ASSERT(block->id() == idx++);

        
        if (block->numSuccessors() > 1)
            for (size_t i = 0; i < block->numSuccessors(); i++)
                JS_ASSERT(block->getSuccessor(i)->numPredecessors() == 1);

        if (block->isLoopHeader()) {
            JS_ASSERT(block->numPredecessors() == 2);
            MBasicBlock *backedge = block->getPredecessor(1);
            JS_ASSERT(backedge->id() >= block->id());
            JS_ASSERT(backedge->numSuccessors() == 1);
            JS_ASSERT(backedge->getSuccessor(0) == *block);
        }

        if (!block->phisEmpty()) {
            for (size_t i = 0; i < block->numPredecessors(); i++) {
                MBasicBlock *pred = block->getPredecessor(i);
                JS_ASSERT(pred->successorWithPhis() == *block);
                JS_ASSERT(pred->positionInPhiSuccessor() == i);
            }
        }

        uint32_t successorWithPhis = 0;
        for (size_t i = 0; i < block->numSuccessors(); i++)
            if (!block->getSuccessor(i)->phisEmpty())
                successorWithPhis++;

        JS_ASSERT(successorWithPhis <= 1);
        JS_ASSERT_IF(successorWithPhis, block->successorWithPhis() != nullptr);

        
        
        
        
        
    }

    AssertDominatorTree(graph);
#endif
}


struct BoundsCheckInfo
{
    MBoundsCheck *check;
    uint32_t validUntil;
};

typedef HashMap<uint32_t,
                BoundsCheckInfo,
                DefaultHasher<uint32_t>,
                IonAllocPolicy> BoundsCheckMap;


static HashNumber
BoundsCheckHashIgnoreOffset(MBoundsCheck *check)
{
    SimpleLinearSum indexSum = ExtractLinearSum(check->index());
    uintptr_t index = indexSum.term ? uintptr_t(indexSum.term) : 0;
    uintptr_t length = uintptr_t(check->length());
    return index ^ length;
}

static MBoundsCheck *
FindDominatingBoundsCheck(BoundsCheckMap &checks, MBoundsCheck *check, size_t index)
{
    
    HashNumber hash = BoundsCheckHashIgnoreOffset(check);
    BoundsCheckMap::Ptr p = checks.lookup(hash);
    if (!p || index > p->value().validUntil) {
        
        BoundsCheckInfo info;
        info.check = check;
        info.validUntil = index + check->block()->numDominated();

        if(!checks.put(hash, info))
            return nullptr;

        return check;
    }

    return p->value().check;
}


SimpleLinearSum
jit::ExtractLinearSum(MDefinition *ins)
{
    if (ins->isBeta())
        ins = ins->getOperand(0);

    if (ins->type() != MIRType_Int32)
        return SimpleLinearSum(ins, 0);

    if (ins->isConstant()) {
        const Value &v = ins->toConstant()->value();
        JS_ASSERT(v.isInt32());
        return SimpleLinearSum(nullptr, v.toInt32());
    } else if (ins->isAdd() || ins->isSub()) {
        MDefinition *lhs = ins->getOperand(0);
        MDefinition *rhs = ins->getOperand(1);
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
jit::ExtractLinearInequality(MTest *test, BranchDirection direction,
                             SimpleLinearSum *plhs, MDefinition **prhs, bool *plessEqual)
{
    if (!test->getOperand(0)->isCompare())
        return false;

    MCompare *compare = test->getOperand(0)->toCompare();

    MDefinition *lhs = compare->getOperand(0);
    MDefinition *rhs = compare->getOperand(1);

    
    if (!compare->isInt32Comparison())
        return false;

    JS_ASSERT(lhs->type() == MIRType_Int32);
    JS_ASSERT(rhs->type() == MIRType_Int32);

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
TryEliminateBoundsCheck(BoundsCheckMap &checks, size_t blockIndex, MBoundsCheck *dominated, bool *eliminated)
{
    JS_ASSERT(!*eliminated);

    
    
    
    
    
    dominated->replaceAllUsesWith(dominated->index());

    if (!dominated->isMovable())
        return true;

    MBoundsCheck *dominating = FindDominatingBoundsCheck(checks, dominated, blockIndex);
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
TryEliminateTypeBarrierFromTest(MTypeBarrier *barrier, bool filtersNull, bool filtersUndefined,
                                MTest *test, BranchDirection direction, bool *eliminated)
{
    JS_ASSERT(filtersNull || filtersUndefined);

    
    
    
    
    
    

    

    
    MDefinition *input = barrier->input();
    MUnbox *inputUnbox = nullptr;
    if (input->isUnbox() && input->toUnbox()->mode() != MUnbox::Fallible) {
        inputUnbox = input->toUnbox();
        input = inputUnbox->input();
    }

    MDefinition *subject = nullptr;
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
TryEliminateTypeBarrier(MTypeBarrier *barrier, bool *eliminated)
{
    JS_ASSERT(!*eliminated);

    const types::TemporaryTypeSet *barrierTypes = barrier->resultTypeSet();
    const types::TemporaryTypeSet *inputTypes = barrier->input()->resultTypeSet();

    
    if (barrier->input()->isUnbox() && barrier->input()->toUnbox()->mode() != MUnbox::Fallible)
        inputTypes = barrier->input()->toUnbox()->input()->resultTypeSet();

    if (!barrierTypes || !inputTypes)
        return true;

    bool filtersNull = barrierTypes->filtersType(inputTypes, types::Type::NullType());
    bool filtersUndefined = barrierTypes->filtersType(inputTypes, types::Type::UndefinedType());

    if (!filtersNull && !filtersUndefined)
        return true;

    MBasicBlock *block = barrier->block();
    while (true) {
        BranchDirection direction;
        MTest *test = block->immediateDominatorBranch(&direction);

        if (test) {
            TryEliminateTypeBarrierFromTest(barrier, filtersNull, filtersUndefined,
                                            test, direction, eliminated);
        }

        MBasicBlock *previous = block->immediateDominator();
        if (previous == block)
            break;
        block = previous;
    }

    return true;
}














bool
jit::EliminateRedundantChecks(MIRGraph &graph)
{
    BoundsCheckMap checks(graph.alloc());

    if (!checks.init())
        return false;

    
    Vector<MBasicBlock *, 1, IonAllocPolicy> worklist(graph.alloc());

    
    size_t index = 0;

    
    
    for (MBasicBlockIterator i(graph.begin()); i != graph.end(); i++) {
        MBasicBlock *block = *i;
        if (block->immediateDominator() == block) {
            if (!worklist.append(block))
                return false;
        }
    }

    
    while (!worklist.empty()) {
        MBasicBlock *block = worklist.popCopy();

        
        if (!worklist.append(block->immediatelyDominatedBlocksBegin(),
                             block->immediatelyDominatedBlocksEnd())) {
            return false;
        }

        for (MDefinitionIterator iter(block); iter; ) {
            bool eliminated = false;

            if (iter->isBoundsCheck()) {
                if (!TryEliminateBoundsCheck(checks, index, iter->toBoundsCheck(), &eliminated))
                    return false;
            } else if (iter->isTypeBarrier()) {
                if (!TryEliminateTypeBarrier(iter->toTypeBarrier(), &eliminated))
                    return false;
            } else if (iter->isConvertElementsToDoubles()) {
                
                
                MConvertElementsToDoubles *ins = iter->toConvertElementsToDoubles();
                ins->replaceAllUsesWith(ins->elements());
            }

            if (eliminated)
                iter = block->discardDefAt(iter);
            else
                iter++;
        }
        index++;
    }

    JS_ASSERT(index == graph.numBlocks());
    return true;
}



static LGoto *
FindLeadingGoto(LBlock *bb)
{
    for (LInstructionIterator ins(bb->begin()); ins != bb->end(); ins++) {
        
        if (ins->isLabel())
            continue;
        
        if (ins->isGoto())
            return ins->toGoto();
        break;
    }
    return nullptr;
}





bool
jit::UnsplitEdges(LIRGraph *lir)
{
    for (size_t i = 0; i < lir->numBlocks(); i++) {
        LBlock *bb = lir->getBlock(i);
        MBasicBlock *mirBlock = bb->mir();

        
        mirBlock->setId(i);

        
        
        
        bb->clearPhis();
        mirBlock->discardAllPhis();

        
        
        
        if (mirBlock->numPredecessors() == 0 || mirBlock->numSuccessors() != 1 ||
            !mirBlock->begin()->isGoto())
        {
            continue;
        }

        
        
        LGoto *theGoto = FindLeadingGoto(bb);
        if (!theGoto)
            continue;
        MBasicBlock *target = theGoto->target();
        if (target == mirBlock || target != mirBlock->getSuccessor(0))
            continue;

        
        
        if (!target->phisEmpty()) {
            target->discardAllPhis();
            target->lir()->clearPhis();
        }

        
        for (size_t j = 0; j < mirBlock->numPredecessors(); j++) {
            MBasicBlock *mirPred = mirBlock->getPredecessor(j);

            for (size_t k = 0; k < mirPred->numSuccessors(); k++) {
                if (mirPred->getSuccessor(k) == mirBlock) {
                    mirPred->replaceSuccessor(k, target);
                    if (!target->addPredecessorWithoutPhis(mirPred))
                        return false;
                }
            }

            LInstruction *predTerm = *mirPred->lir()->rbegin();
            for (size_t k = 0; k < predTerm->numSuccessors(); k++) {
                if (predTerm->getSuccessor(k) == mirBlock)
                    predTerm->setSuccessor(k, target);
            }
        }
        target->removePredecessor(mirBlock);

        
        lir->removeBlock(i);
        lir->mir().removeBlock(mirBlock);
        --i;
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
LinearSum::add(const LinearSum &other)
{
    for (size_t i = 0; i < other.terms_.length(); i++) {
        if (!add(other.terms_[i].term, other.terms_[i].scale))
            return false;
    }
    return add(other.constant_);
}

bool
LinearSum::add(MDefinition *term, int32_t scale)
{
    JS_ASSERT(term);

    if (scale == 0)
        return true;

    if (term->isConstant()) {
        int32_t constant = term->toConstant()->value().toInt32();
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

    terms_.append(LinearTerm(term, scale));
    return true;
}

bool
LinearSum::add(int32_t constant)
{
    return SafeAdd(constant, constant_, &constant_);
}

void
LinearSum::print(Sprinter &sp) const
{
    for (size_t i = 0; i < terms_.length(); i++) {
        int32_t scale = terms_[i].scale;
        int32_t id = terms_[i].term->id();
        JS_ASSERT(scale);
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
LinearSum::dump(FILE *fp) const
{
    Sprinter sp(GetIonContext()->cx);
    sp.init();
    print(sp);
    fprintf(fp, "%s\n", sp.string());
}

void
LinearSum::dump() const
{
    dump(stderr);
}

static bool
AnalyzePoppedThis(JSContext *cx, types::TypeObject *type,
                  MDefinition *thisValue, MInstruction *ins, bool definitelyExecuted,
                  HandleObject baseobj,
                  Vector<types::TypeNewScript::Initializer> *initializerList,
                  Vector<PropertyName *> *accessedProperties,
                  bool *phandled)
{
    
    

    if (ins->isCallSetProperty()) {
        MCallSetProperty *setprop = ins->toCallSetProperty();

        if (setprop->object() != thisValue)
            return true;

        
        
        
        if (setprop->name() == cx->names().prototype ||
            setprop->name() == cx->names().proto ||
            setprop->name() == cx->names().constructor)
        {
            return true;
        }

        
        if (baseobj->nativeLookup(cx, NameToId(setprop->name()))) {
            *phandled = true;
            return true;
        }

        
        
        for (size_t i = 0; i < accessedProperties->length(); i++) {
            if ((*accessedProperties)[i] == setprop->name())
                return true;
        }

        
        
        if (GetGCKindSlots(gc::GetGCObjectKind(baseobj->slotSpan() + 1)) <= baseobj->slotSpan())
            return true;

        
        if (!definitelyExecuted)
            return true;

        RootedId id(cx, NameToId(setprop->name()));
        if (!types::AddClearDefiniteGetterSetterForPrototypeChain(cx, type, id)) {
            
            
            return true;
        }

        DebugOnly<unsigned> slotSpan = baseobj->slotSpan();
        if (!DefineNativeProperty(cx, baseobj, id, UndefinedHandleValue, nullptr, nullptr,
                                  JSPROP_ENUMERATE))
        {
            return false;
        }
        JS_ASSERT(baseobj->slotSpan() != slotSpan);
        JS_ASSERT(!baseobj->inDictionaryMode());

        Vector<MResumePoint *> callerResumePoints(cx);
        for (MResumePoint *rp = ins->block()->callerResumePoint();
             rp;
             rp = rp->block()->callerResumePoint())
        {
            if (!callerResumePoints.append(rp))
                return false;
        }

        for (int i = callerResumePoints.length() - 1; i >= 0; i--) {
            MResumePoint *rp = callerResumePoints[i];
            JSScript *script = rp->block()->info().script();
            types::TypeNewScript::Initializer entry(types::TypeNewScript::Initializer::SETPROP_FRAME,
                                                    script->pcToOffset(rp->pc()));
            if (!initializerList->append(entry))
                return false;
        }

        JSScript *script = ins->block()->info().script();
        types::TypeNewScript::Initializer entry(types::TypeNewScript::Initializer::SETPROP,
                                                script->pcToOffset(setprop->resumePoint()->pc()));
        if (!initializerList->append(entry))
            return false;

        *phandled = true;
        return true;
    }

    if (ins->isCallGetProperty()) {
        MCallGetProperty *get = ins->toCallGetProperty();

        










        RootedId id(cx, NameToId(get->name()));
        if (!baseobj->nativeLookup(cx, id) && !accessedProperties->append(get->name()))
            return false;

        if (!types::AddClearDefiniteGetterSetterForPrototypeChain(cx, type, id)) {
            
            
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
CmpInstructions(const void *a, const void *b)
{
    return (*static_cast<MInstruction * const *>(a))->id() -
           (*static_cast<MInstruction * const *>(b))->id();
}

bool
jit::AnalyzeNewScriptProperties(JSContext *cx, JSFunction *fun,
                                types::TypeObject *type, HandleObject baseobj,
                                Vector<types::TypeNewScript::Initializer> *initializerList)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);

    
    
    

    RootedScript script(cx, fun->getOrCreateScript(cx));
    if (!script)
        return false;

    if (!jit::IsIonEnabled(cx) || !jit::IsBaselineEnabled(cx) ||
        !script->compileAndGo() || !script->canBaselineCompile())
    {
        return true;
    }

    static const uint32_t MAX_SCRIPT_SIZE = 2000;
    if (script->length() > MAX_SCRIPT_SIZE)
        return true;

    Vector<PropertyName *> accessedProperties(cx);

    LifoAlloc alloc(types::TypeZone::TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);

    TempAllocator temp(&alloc);
    IonContext ictx(cx, &temp);

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    if (!script->hasBaselineScript()) {
        MethodStatus status = BaselineCompile(cx, script);
        if (status == Method_Error)
            return false;
        if (status != Method_Compiled)
            return true;
    }

    types::TypeScript::SetThis(cx, script, types::Type::ObjectType(type));

    MIRGraph graph(&temp);
    InlineScriptTree *inlineScriptTree = InlineScriptTree::New(&temp, nullptr, nullptr, script);
    if (!inlineScriptTree)
        return false;

    CompileInfo info(script, fun,
                      nullptr,  false,
                     DefinitePropertiesAnalysis,
                     script->needsArgsObj(),
                     inlineScriptTree);

    AutoTempAllocatorRooter root(cx, &temp);

    const OptimizationInfo *optimizationInfo = js_IonOptimizations.get(Optimization_Normal);

    types::CompilerConstraintList *constraints = types::NewCompilerConstraintList(temp);
    if (!constraints) {
        js_ReportOutOfMemory(cx);
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

    types::FinishDefinitePropertiesAnalysis(cx, constraints);

    if (!SplitCriticalEdges(graph))
        return false;

    if (!RenumberBlocks(graph))
        return false;

    if (!BuildDominatorTree(graph))
        return false;

    if (!EliminatePhis(&builder, graph, AggressiveObservability))
        return false;

    MDefinition *thisValue = graph.entryBlock()->getSlot(info.thisSlot());

    
    
    Vector<MInstruction *> instructions(cx);

    for (MUseDefIterator uses(thisValue); uses; uses++) {
        MDefinition *use = uses.def();

        
        if (!use->isInstruction())
            return true;

        if (!instructions.append(use->toInstruction()))
            return false;
    }

    
    qsort(instructions.begin(), instructions.length(),
          sizeof(MInstruction *), CmpInstructions);

    
    Vector<MBasicBlock *> exitBlocks(cx);
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (!block->numSuccessors() && !exitBlocks.append(*block))
            return false;
    }

    
    size_t lastAddedBlock = 0;

    for (size_t i = 0; i < instructions.length(); i++) {
        MInstruction *ins = instructions[i];

        
        
        bool definitelyExecuted = true;
        for (size_t i = 0; i < exitBlocks.length(); i++) {
            for (MBasicBlock *exit = exitBlocks[i];
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
        if (!AnalyzePoppedThis(cx, type, thisValue, ins, definitelyExecuted,
                               baseobj, initializerList, &accessedProperties, &handled))
        {
            return false;
        }
        if (!handled)
            break;

        if (slotSpan != baseobj->slotSpan()) {
            JS_ASSERT(ins->block()->id() >= lastAddedBlock);
            lastAddedBlock = ins->block()->id();
        }
    }

    if (baseobj->slotSpan() != 0) {
        
        
        
        
        Vector<MBasicBlock *> exitBlocks(cx);
        for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
            
            
            if (block->id() > lastAddedBlock)
                break;
            if (MResumePoint *rp = block->callerResumePoint()) {
                if (block->numPredecessors() == 1 && block->getPredecessor(0) == rp->block()) {
                    JSScript *script = rp->block()->info().script();
                    if (!types::AddClearDefiniteFunctionUsesInScript(cx, type, script, block->info().script()))
                        return false;
                }
            }
        }
    }

    return true;
}

static bool
ArgumentsUseCanBeLazy(JSContext *cx, JSScript *script, MInstruction *ins, size_t index,
                      bool *argumentsContentsObserved)
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

    
    if (ins->isCallGetProperty() && index == 0 && ins->toCallGetProperty()->name() == cx->names().length)
        return true;

    return false;
}

bool
jit::AnalyzeArgumentsUsage(JSContext *cx, JSScript *scriptArg)
{
    RootedScript script(cx, scriptArg);
    types::AutoEnterAnalysis enter(cx);

    JS_ASSERT(!script->analyzedArgsUsage());

    
    
    
    
    script->setNeedsArgsObj(true);

    
    
    
    
    if (cx->compartment()->debugMode() || script->isGenerator())
        return true;

    
    
    
    
    
    
    
    
    if (script->bindingsAccessedDynamically()) {
        script->setNeedsArgsObj(false);
        return true;
    }

    if (!jit::IsIonEnabled(cx) || !script->compileAndGo())
        return true;

    static const uint32_t MAX_SCRIPT_SIZE = 10000;
    if (script->length() > MAX_SCRIPT_SIZE)
        return true;

    if (!script->ensureHasTypes(cx))
        return false;

    LifoAlloc alloc(types::TypeZone::TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);

    TempAllocator temp(&alloc);
    IonContext ictx(cx, &temp);

    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return false;

    MIRGraph graph(&temp);
    InlineScriptTree *inlineScriptTree = InlineScriptTree::New(&temp, nullptr, nullptr, script);
    if (!inlineScriptTree)
        return false;
    CompileInfo info(script, script->functionNonDelazifying(),
                      nullptr,  false,
                     ArgumentsUsageAnalysis,
                      true,
                     inlineScriptTree);

    AutoTempAllocatorRooter root(cx, &temp);

    const OptimizationInfo *optimizationInfo = js_IonOptimizations.get(Optimization_Normal);

    types::CompilerConstraintList *constraints = types::NewCompilerConstraintList(temp);
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

    MDefinition *argumentsValue = graph.entryBlock()->getSlot(info.argsObjSlot());

    bool argumentsContentsObserved = false;

    for (MUseDefIterator uses(argumentsValue); uses; uses++) {
        MDefinition *use = uses.def();

        
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
