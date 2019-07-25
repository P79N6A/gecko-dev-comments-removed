








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;




bool
ion::SplitCriticalEdges(MIRGenerator *gen, MIRGraph &graph)
{
    size_t preSplitEdges = graph.numBlocks();
    for (size_t i = 0; i < preSplitEdges; i++) {
        MBasicBlock *block = graph.getBlock(i);
        if (block->numSuccessors() < 2)
            continue;
        for (size_t i = 0; i < block->numSuccessors(); i++) {
            MBasicBlock *target = block->getSuccessor(i);
            if (target->numPredecessors() < 2)
                continue;

            
            MBasicBlock *split = MBasicBlock::NewSplitEdge(gen, block);
            if (!graph.addBlock(split))
                return false;
            split->end(MGoto::New(target));

            block->replaceSuccessor(i, split);
            target->replacePredecessor(block, split);
        }
    }

    return true;
}

class TypeAnalyzer
{
    MIRGraph &graph;
    js::Vector<MInstruction *, 0, SystemAllocPolicy> worklist_;

    bool empty() const {
        return worklist_.empty();
    }
    MInstruction *pop() {
        MInstruction *ins = worklist_.popCopy();
        ins->setNotInWorklist();
        return ins;
    }
    bool push(MDefinition *def) {
        if (def->isInWorklist() || !def->typePolicy())
            return true;
        JS_ASSERT(!def->isPhi());
        def->setInWorklist();
        return worklist_.append(def->toInstruction());
    }

    bool buildWorklist();
    bool reflow(MDefinition *def);
    bool despecializePhi(MPhi *phi);
    bool specializePhi(MPhi *phi);
    bool specializePhis();
    bool specializeInstructions();
    bool determineSpecializations();
    bool insertConversions();
    bool adjustPhiInputs(MPhi *phi);
    bool adjustInputs(MDefinition *def);
    bool adjustOutput(MDefinition *def);

  public:
    TypeAnalyzer(MIRGraph &graph)
      : graph(graph)
    { }

    bool analyze();
};

bool
TypeAnalyzer::buildWorklist()
{
    
    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        MDefinitionIterator iter(block);
        while (iter) {
            if (iter->isCopy()) {
                
                MCopy *copy = iter->toCopy();
                copy->replaceAllUsesWith(copy->getOperand(0));
                iter = block->removeDefAt(iter);
                continue;
            }
            if (!push(*iter))
                return false;
            iter++;
        }
    }
    return true;
}

bool
TypeAnalyzer::reflow(MDefinition *def)
{
    
    
    
    for (MUseDefIterator uses(def); uses; uses++) {
        if (!push(uses.def()))
            return false;
    }
    return true;
}

bool
TypeAnalyzer::specializeInstructions()
{
    
    
    
    while (!empty()) {
        MInstruction *ins = pop();

        TypePolicy *policy = ins->typePolicy();
        if (policy->respecialize(ins)) {
            if (!reflow(ins))
                return false;
        }
    }
    return true;
}

static inline MIRType
GetEffectiveType(MDefinition *def)
{
    return def->type() != MIRType_Value
           ? def->type()
           : def->usedAsType();
}

bool
TypeAnalyzer::despecializePhi(MPhi *phi)
{
    
    if (phi->type() == MIRType_Value)
        return true;

    phi->specialize(MIRType_Value);
    if (!reflow(phi))
        return false;
    return true;
}

bool
TypeAnalyzer::specializePhi(MPhi *phi)
{
    
    
    if (phi->triedToSpecialize() && phi->type() == MIRType_Value)
        return true;

    
    MDefinition *in = phi->getOperand(0);
    MIRType first = GetEffectiveType(in);

    
    if (first == MIRType_Value)
        return despecializePhi(phi);

    for (size_t i = 1; i < phi->numOperands(); i++) {
        MDefinition *other = phi->getOperand(i);
        if (GetEffectiveType(other) != first)
            return despecializePhi(phi);
    }

    if (phi->type() == first)
        return true;

    
    phi->specialize(first);
    if (!reflow(phi))
        return false;

    return true;
}

bool
TypeAnalyzer::specializePhis()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (size_t i = 0; i < block->numPhis(); i++) {
            if (!specializePhi(block->getPhi(i)))
                return false;
        }
    }
    return true;
}

bool
TypeAnalyzer::determineSpecializations()
{
    do {
        
        if (!specializeInstructions())
            return false;

        
        
        
        
        if (!specializePhis())
            return false;
    } while (!empty());
    return true;
}

bool
TypeAnalyzer::adjustOutput(MDefinition *def)
{
    JS_ASSERT(def->type() == MIRType_Value);

    MIRType usedAs = def->usedAsType();
    if (usedAs == MIRType_Value) {
        
        
        
        return true;
    }

    MBasicBlock *block = def->block();
    MUnbox *unbox = MUnbox::New(def, usedAs);
    if (def->isPhi()) {
        
        block->insertBefore(*block->begin(), unbox);
    } else if (block->start() && def->id() < block->start()->id()) {
        
        
        block->insertAfter(block->start(), unbox);
    } else {
        
        block->insertAfter(def->toInstruction(), unbox);
    }

    JS_ASSERT(def->usesBegin()->node() == unbox);

    for (MUseIterator use(def->usesBegin()); use != def->usesEnd(); ) {
        bool replace = true;

        if (use->node()->isSnapshot()) {
            MSnapshot *snapshot = use->node()->toSnapshot();
            
            
            
            
            if (def->isInstruction() && def->toInstruction()->snapshot() == snapshot)
                replace = false;
        } else {
            MDefinition *other = use->node()->toDefinition();
            if (TypePolicy *policy = other->typePolicy())
                replace = policy->useSpecializedInput(def->toInstruction(), use->index(), unbox);
        }

        if (replace)
            use = use->node()->replaceOperand(use, unbox);
        else
            use++;
    }

    return true;
}

bool
TypeAnalyzer::adjustPhiInputs(MPhi *phi)
{
    
    if (phi->type() != MIRType_Value) {
#ifdef DEBUG
        for (size_t i = 0; i < phi->numOperands(); i++) {
            MDefinition *in = phi->getOperand(i);
            JS_ASSERT(GetEffectiveType(in) == phi->type());
        }
#endif
        return true;
    }

    
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition *in = phi->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;

        MBox *box = MBox::New(in);
        in->block()->insertBefore(in->block()->lastIns(), box);
        phi->replaceOperand(i, box);
    }

    return true;
}

bool
TypeAnalyzer::adjustInputs(MDefinition *def)
{
    
    
    TypePolicy *policy = def->typePolicy();
    if (policy && !policy->adjustInputs(def->toInstruction()))
        return false;
    return true;
}

bool
TypeAnalyzer::insertConversions()
{
    
    
    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (size_t i = 0; i < block->numPhis(); i++) {
            MPhi *phi = block->getPhi(i);
            if (!adjustPhiInputs(phi))
                return false;
            if (phi->type() == MIRType_Value && !adjustOutput(phi))
                return false;
        }
        for (MInstructionIterator iter = block->begin(); iter != block->end(); iter++) {
            if (!adjustInputs(*iter))
                return false;
            if (iter->type() == MIRType_Value && !adjustOutput(*iter))
                return false;
        }
    }
    return true;
}

bool
TypeAnalyzer::analyze()
{
    if (!buildWorklist())
        return false;
    if (!determineSpecializations())
        return false;
    if (!insertConversions())
        return false;
    return true;
}

bool
ion::ApplyTypeInformation(MIRGraph &graph)
{
    TypeAnalyzer analyzer(graph);

    if (!analyzer.analyze())
        return false;

    return true;
}

bool
ion::ReorderBlocks(MIRGraph &graph)
{
    Vector<MBasicBlock *, 0, IonAllocPolicy> pending;
    Vector<unsigned int, 0, IonAllocPolicy> successors;
    Vector<MBasicBlock *, 0, IonAllocPolicy> done;

    MBasicBlock *current = graph.getBlock(0);
    unsigned int nextSuccessor = 0;

    graph.clearBlockList();

    
    while (true) {
        if (!current->isMarked()) {
            current->mark();

            if (nextSuccessor < current->lastIns()->numSuccessors()) {
                if (!pending.append(current))
                    return false;
                if (!successors.append(nextSuccessor))
                    return false;

                current = current->lastIns()->getSuccessor(nextSuccessor);
                nextSuccessor = 0;
                continue;
            }

            if (!done.append(current))
                return false;
        }

        if (pending.empty())
            break;

        current = pending.popCopy();
        current->unmark();
        nextSuccessor = successors.popCopy() + 1;
    }

    JS_ASSERT(pending.empty());
    JS_ASSERT(successors.empty());

    while (!done.empty()) {
        current = done.popCopy();
        current->unmark();
        if (!graph.addBlock(current))
            return false;
    }

    return true;
}


static MBasicBlock *
IntersectDominators(MBasicBlock *block1, MBasicBlock *block2)
{
    MBasicBlock *finger1 = block1;
    MBasicBlock *finger2 = block2;

    while (finger1->id() != finger2->id()) {
        
        
        
        while (finger1->id() > finger2->id())
            finger1 = finger1->immediateDominator();

        while (finger2->id() > finger1->id())
            finger2 = finger2->immediateDominator();
    }
    return finger1;
}

static void
ComputeImmediateDominators(MIRGraph &graph)
{

    if (graph.numBlocks() == 0)
        return;

    MBasicBlock *startBlock = graph.getBlock(0);
    startBlock->setImmediateDominator(startBlock);

    bool changed = true;

    while (changed) {
        changed = false;
        
        for (size_t i = 1; i < graph.numBlocks(); i++) {
            MBasicBlock *block = graph.getBlock(i);

            if (block->numPredecessors() == 0)
                continue;

            MBasicBlock *newIdom = block->getPredecessor(0);

            for (size_t i = 1; i < block->numPredecessors(); i++) {
                MBasicBlock *pred = graph.getBlock(i);
                if (pred->immediateDominator() != NULL)
                    newIdom = IntersectDominators(pred, newIdom);
            }

            if (block->immediateDominator() != newIdom) {
                block->setImmediateDominator(newIdom);
                changed = true;
            }
        }
    }
}

bool
ion::BuildDominatorTree(MIRGraph &graph)
{
    if (graph.numBlocks() == 0)
        return true;

    ComputeImmediateDominators(graph);

    
    
    
    
    
    for (size_t i = graph.numBlocks() - 1; i > 0; i--) { 
        MBasicBlock *child = graph.getBlock(i);
        MBasicBlock *parent = child->immediateDominator();

        if (!parent->addImmediatelyDominatedBlock(child))
            return false;

        
        parent->addNumDominated(child->numDominated() + 1);
    }
    JS_ASSERT(graph.getBlock(0)->numDominated() == graph.numBlocks() - 1);
    return true;
}

bool
ion::BuildPhiReverseMapping(MIRGraph &graph)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        if (block->numPredecessors() < 2) {
            JS_ASSERT(block->numPhis() == 0);
            continue;
        }

        
        for (size_t j = 0; j < block->numPredecessors(); j++) {
            MBasicBlock *pred = block->getPredecessor(j);

#ifdef DEBUG
            size_t numSuccessorsWithPhis = 0;
            for (size_t k = 0; k < pred->numSuccessors(); k++) {
                MBasicBlock *successor = pred->getSuccessor(k);
                if (successor->numPhis() > 0)
                    numSuccessorsWithPhis++;
            }
            JS_ASSERT(numSuccessorsWithPhis <= 1);
#endif

            pred->setSuccessorWithPhis(block, j);
        }
    }

    return true;
}

