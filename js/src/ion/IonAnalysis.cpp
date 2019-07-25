








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;




bool
ion::SplitCriticalEdges(MIRGenerator *gen, MIRGraph &graph)
{
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (block->numSuccessors() < 2)
            continue;
        for (size_t i = 0; i < block->numSuccessors(); i++) {
            MBasicBlock *target = block->getSuccessor(i);
            if (target->numPredecessors() < 2)
                continue;

            
            MBasicBlock *split = MBasicBlock::NewSplitEdge(graph, gen->info(), *block);
            graph.addBlock(split);
            split->end(MGoto::New(target));

            block->replaceSuccessor(i, split);
            target->replacePredecessor(*block, split);
        }
    }
    return true;
}




bool
ion::EliminateDeadCode(MIRGraph &graph)
{
    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        
        for (MInstructionReverseIterator inst = block->rbegin(); inst != block->rend(); ) {
            if (!inst->isEffectful() && !inst->hasUses() && !inst->isGuard() &&
                !inst->isControlInstruction()) {
                inst = block->discardAt(inst);
            } else {
                inst++;
            }
        }
    }

    return true;
}

static inline bool
IsPhiObservable(MPhi *phi)
{
    
    
    
    
    for (MUseDefIterator iter(phi); iter; iter++) {
        if (!iter.def()->isPhi())
            return true;
    }
    return false;
}

bool
ion::EliminateDeadPhis(MIRGraph &graph)
{
    Vector<MPhi *, 16, SystemAllocPolicy> worklist;

    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        for (MPhiIterator iter = block->phisBegin(); iter != block->phisEnd(); iter++) {
            if (IsPhiObservable(*iter)) {
                iter->setInWorklist();
                if (!worklist.append(*iter))
                    return false;
            }
        }
    }

    
    while (!worklist.empty()) {
        MPhi *phi = worklist.popCopy();

        for (size_t i = 0; i < phi->numOperands(); i++) {
            MDefinition *in = phi->getOperand(i);
            if (!in->isPhi() || in->isInWorklist())
                continue;
            in->setInWorklist();
            if (!worklist.append(in->toPhi()))
                return false;
        }
    }

    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        MPhiIterator iter = block->phisBegin();
        while (iter != block->phisEnd()) {
            if (iter->isInWorklist()) {
                iter->setNotInWorklist();
                iter++;
            } else {
                iter->setUnused();
                iter = block->discardPhiAt(iter);
            }
        }
    }

    return true;
}










































class TypeAnalyzer : public TypeAnalysis
{
    MIRGraph &graph;
    Vector<MInstruction *, 0, SystemAllocPolicy> worklist_;
    Vector<MPhi *, 0, SystemAllocPolicy> phiWorklist_;
    bool phisHaveBeenAnalyzed_;

    MInstruction *popInstruction() {
        MInstruction *ins = worklist_.popCopy();
        ins->setNotInWorklist();
        return ins;
    }
    MPhi *popPhi() {
        MPhi *phi = phiWorklist_.popCopy();
        phi->setNotInWorklist();
        return phi;
    }
    void repush(MDefinition *def) {
#ifdef DEBUG
        bool ok =
#endif
            push(def);
        JS_ASSERT(ok);
    }
    bool push(MDefinition *def) {
        if (def->isInWorklist())
            return true;
        if (!def->isPhi() && !def->typePolicy())
            return true;
        def->setInWorklist();
        if (def->isPhi())
            return phiWorklist_.append(def->toPhi());
        return worklist_.append(def->toInstruction());
    }

    
    
    bool buildWorklist();

    void addPreferredType(MDefinition *def, MIRType type);
    void reanalyzePhiUses(MDefinition *def);
    void reanalyzeUses(MDefinition *def);
    void despecializePhi(MPhi *phi);
    void specializePhi(MPhi *phi);
    void specializePhis();
    void specializeInstructions();
    void determineSpecializations();
    void replaceRedundantPhi(MPhi *phi);
    void adjustPhiInputs(MPhi *phi);
    bool adjustInputs(MDefinition *def);
    void adjustOutput(MDefinition *def);
    bool insertConversions();

  public:
    TypeAnalyzer(MIRGraph &graph)
      : graph(graph),
        phisHaveBeenAnalyzed_(false)
    { }

    bool analyze();
};

bool
TypeAnalyzer::buildWorklist()
{
    
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MPhiIterator iter = block->phisBegin(); iter != block->phisEnd(); iter++) {
            if (!push(*iter))
                return false;
        }
        MInstructionIterator iter = block->begin();
        while (iter != block->end()) {
            if (iter->isCopy()) {
                
                MCopy *copy = iter->toCopy();
                copy->replaceAllUsesWith(copy->getOperand(0));
                iter = block->discardAt(iter);
                continue;
            }
            if (!push(*iter))
                return false;
            iter++;
        }
    }
    return true;
}

void
TypeAnalyzer::reanalyzePhiUses(MDefinition *def)
{
    
    
    if (!phisHaveBeenAnalyzed_)
        return;

    for (MUseDefIterator uses(def); uses; uses++) {
        if (uses.def()->isPhi())
            repush(uses.def());
    }
}

void
TypeAnalyzer::reanalyzeUses(MDefinition *def)
{
    
    
    
    for (MUseDefIterator uses(def); uses; uses++)
        repush(uses.def());
}

void
TypeAnalyzer::addPreferredType(MDefinition *def, MIRType type)
{
    MIRType usedAsType = def->usedAsType();
    def->useAsType(type);
    if (usedAsType != def->usedAsType())
        reanalyzePhiUses(def);
}

void
TypeAnalyzer::specializeInstructions()
{
    
    
    
    while (!worklist_.empty()) {
        MInstruction *ins = popInstruction();

        TypePolicy *policy = ins->typePolicy();
        if (policy->respecialize(ins))
            reanalyzeUses(ins);
        policy->specializeInputs(ins, this);
    }
}

static inline MIRType
GetObservedType(MDefinition *def)
{
    return def->type() != MIRType_Value
           ? def->type()
           : def->usedAsType();
}

void
TypeAnalyzer::despecializePhi(MPhi *phi)
{
    
    if (phi->type() == MIRType_Value)
        return;

    phi->specialize(MIRType_Value);
    reanalyzeUses(phi);
}

void
TypeAnalyzer::specializePhi(MPhi *phi)
{
    
    
    if (phi->triedToSpecialize() && phi->type() == MIRType_Value)
        return;

    MIRType phiType = GetObservedType(phi);
    if (phiType != MIRType_Value) {
        
        
        
        phi->setInWorklist();
        for (size_t i = 0; i < phi->numOperands(); i++)
            addPreferredType(phi->getOperand(i), phiType);
        phi->setNotInWorklist();
    }

    
    MDefinition *in = phi->getOperand(0);
    MIRType first = GetObservedType(in);

    
    if (first == MIRType_Value) {
        despecializePhi(phi);
        return;
    }

    for (size_t i = 1; i < phi->numOperands(); i++) {
        MDefinition *other = phi->getOperand(i);
        MIRType otherType = GetObservedType(other);
        if (otherType != first) {
            if (IsNumberType(otherType) && IsNumberType(first)) {
                
                
                first = MIRType_Double;
                continue;
            }
            
            despecializePhi(phi);
            return;
        }
    }

    if (phi->type() == first)
        return;

    
    phi->specialize(first);
    reanalyzeUses(phi);
}

void
TypeAnalyzer::specializePhis()
{
    phisHaveBeenAnalyzed_ = true;

    while (!phiWorklist_.empty()) {
        MPhi *phi = popPhi();
        specializePhi(phi);
    }
}
 

void
TypeAnalyzer::determineSpecializations()
{
    do {
        
        specializeInstructions();

        
        
        specializePhis();
    } while (!worklist_.empty());
}

static inline bool
ShouldSpecializeInput(MDefinition *box, MNode *use, MUnbox *unbox)
{
    
    
    if (use->isResumePoint()) {
        MResumePoint *resumePoint = use->toResumePoint();
            
        
        
        
        MResumePoint *defResumePoint = NULL;
        if (box->isInstruction())
            defResumePoint = box->toInstruction()->resumePoint();
        else if (box->isPhi())
            defResumePoint = box->block()->entryResumePoint();
        return !defResumePoint || (defResumePoint != resumePoint);
    }

    MDefinition *def = use->toDefinition();

    
    
    if (def->isPhi())
        return def->type() != MIRType_Value;

    
    
    if (def->typePolicy())
        return true;

    return false;
}

void
TypeAnalyzer::adjustOutput(MDefinition *def)
{
    JS_ASSERT(def->type() == MIRType_Value);

    MIRType usedAs = def->usedAsType();
    if (usedAs == MIRType_Value) {
        
        
        
        return;
    }

    MBasicBlock *block = def->block();
    MUnbox *unbox = MUnbox::New(def, usedAs, MUnbox::Fallible);
    if (def->isPhi()) {
        
        block->insertBefore(*block->begin(), unbox);
    } else if (block->start() && def->id() < block->start()->id()) {
        
        
        block->insertAfter(block->start(), unbox);
    } else if (def->isOsrValue()) {
        
        JS_ASSERT(graph.osrStart()->block() == block);
        block->insertAfter(graph.osrStart(), unbox);
    } else {
        
        block->insertAfter(def->toInstruction(), unbox);
    }

    JS_ASSERT(def->usesBegin()->node() == unbox);

    for (MUseIterator use(def->usesBegin()); use != def->usesEnd(); ) {
        if (ShouldSpecializeInput(def, use->node(), unbox))
            use = use->node()->replaceOperand(use, unbox);
        else
            use++;
    }
}

void
TypeAnalyzer::adjustPhiInputs(MPhi *phi)
{
    
    MIRType phiType = phi->type();
    if (phiType != MIRType_Value) {
        for (size_t i = 0; i < phi->numOperands(); i++) {
            MDefinition *in = phi->getOperand(i);
            MIRType inType = GetObservedType(in);

            if (phiType == MIRType_Double && inType == MIRType_Int32) {
                MToDouble *convert = MToDouble::New(in);

                
                
                
                
                MBasicBlock *pred = phi->block()->getPredecessor(i);
                pred->insertBefore(pred->lastIns(), convert);
                phi->replaceOperand(i, convert);
                continue;
            }

            JS_ASSERT(GetObservedType(in) == phi->type());
        }
        return;
    }

    
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition *in = phi->getOperand(i);
        if (in->type() == MIRType_Value)
            continue;

        if (in->isUnbox()) {
            
            
            phi->replaceOperand(i, in->toUnbox()->input());
        } else {
            MBox *box = MBox::New(in);
            in->block()->insertBefore(in->block()->lastIns(), box);
            phi->replaceOperand(i, box);
        }
    }
}

bool
TypeAnalyzer::adjustInputs(MDefinition *def)
{
    
    
    TypePolicy *policy = def->typePolicy();
    if (policy && !policy->adjustInputs(def->toInstruction()))
        return false;
    return true;
}

void
TypeAnalyzer::replaceRedundantPhi(MPhi *phi)
{
    MBasicBlock *block = phi->block();
    js::Value v = (phi->type() == MIRType_Undefined) ? UndefinedValue() : NullValue();
    MConstant *c = MConstant::New(v);
    
    block->insertBefore(*(block->begin()), c);
    phi->replaceAllUsesWith(c);
}

bool
TypeAnalyzer::insertConversions()
{
    
    
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd();) {
            if (phi->type() <= MIRType_Null) {
                replaceRedundantPhi(*phi);
                phi = block->discardPhiAt(phi);
            } else {
                adjustPhiInputs(*phi);
                if (phi->type() == MIRType_Value)
                    adjustOutput(*phi);
                phi++;
            }
        }
        for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
            if (!adjustInputs(*iter))
                return false;
            if (iter->type() == MIRType_Value)
                adjustOutput(*iter);
        }
    }
    return true;
}

bool
TypeAnalyzer::analyze()
{
    if (!buildWorklist())
        return false;
    determineSpecializations();
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
    InlineList<MBasicBlock> pending;
    Vector<unsigned int, 0, IonAllocPolicy> successors;
    InlineList<MBasicBlock> done;

    MBasicBlock *current = *graph.begin();

    
    
    
    unsigned int nextSuccessor = current->numSuccessors() - 1;

#ifdef DEBUG
    size_t numBlocks = graph.numBlocks();
#endif

    graph.clearBlockList();

    
    while (true) {
        if (!current->isMarked()) {
            current->mark();

            
            
            
            if (nextSuccessor < current->numSuccessors()) {
                pending.pushFront(current);
                if (!successors.append(nextSuccessor))
                    return false;

                current = current->getSuccessor(nextSuccessor);
                nextSuccessor = current->numSuccessors() - 1;
                continue;
            }

            done.pushFront(current);
        }

        if (pending.empty())
            break;

        current = pending.popFront();
        current->unmark();
        nextSuccessor = successors.popCopy() - 1;
    }

    JS_ASSERT(pending.empty());
    JS_ASSERT(successors.empty());

    
    current = done.popFront();
    current->unmark();
    graph.addBlock(current);

    
    
    
    if (graph.osrBlock())
        graph.addBlock(graph.osrBlock());

    
    while (!done.empty()) {
        current = done.popFront();
        current->unmark();
        graph.addBlock(current);
    }

    JS_ASSERT(graph.numBlocks() == numBlocks);

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
                return NULL; 
            finger1 = idom;
        }

        while (finger2->id() > finger1->id()) {
            MBasicBlock *idom = finger2->immediateDominator();
            if (idom == finger2)
                return NULL; 
            finger2 = finger2->immediateDominator();
        }
    }
    return finger1;
}

static void
ComputeImmediateDominators(MIRGraph &graph)
{
    
    MBasicBlock *startBlock = *graph.begin();
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
                if (pred->immediateDominator() != NULL)
                    newIdom = IntersectDominators(pred, newIdom);

                
                if (newIdom == NULL) {
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
        JS_ASSERT(block->immediateDominator() != NULL);
    }
#endif
}

bool
ion::BuildDominatorTree(MIRGraph &graph)
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
        JS_ASSERT(graph.begin()->numDominated() == graph.numBlocks() - 1);
#endif

    return true;
}

bool
ion::BuildPhiReverseMapping(MIRGraph &graph)
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

static inline MBasicBlock *
SkipContainedLoop(MBasicBlock *block, MBasicBlock *header)
{
    while (block->loopHeader() || block->isLoopHeader()) {
        if (block->loopHeader())
            block = block->loopHeader();
        if (block == header)
            break;
        block = block->loopPredecessor();
    }
    return block;
}


bool
ion::FindNaturalLoops(MIRGraph &graph)
{
    Vector<MBasicBlock *, 8, SystemAllocPolicy> worklist;

    
    
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (!block->isLoopBackedge())
            continue;

        MBasicBlock *header = block->loopHeaderOfBackedge();
        JS_ASSERT(!block->loopHeader());
        JS_ASSERT(!header->loopHeader());

        
        header->setLoopHeader(header);
        if (!header->addContainedInLoop(header))
            return false;

        MBasicBlock *current = *block;
        do {
            
            for (size_t i = 0; i < current->numPredecessors(); i++) {
                MBasicBlock *pred = current->getPredecessor(i);

                
                
                if (pred->loopHeader() == header)
                    continue;

                
                
                JS_ASSERT_IF(pred != graph.osrBlock(),
                             header->id() < pred->id() && pred->id() < block->id());

                
                
                pred = SkipContainedLoop(pred, header);
                if (pred == header)
                    continue;

                JS_ASSERT(!pred->isLoopBackedge());

                if (!worklist.append(pred))
                    return false;
            }

            current->setLoopHeader(header);
            if (!header->addContainedInLoop(current))
                return false;
            if (worklist.empty())
                break;
            current = worklist.popCopy();
        } while (true);
    }

    return true;
}

void
ion::AssertGraphCoherency(MIRGraph &graph)
{
#ifdef DEBUG
    
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        
        for (size_t i = 0; i < block->numSuccessors(); i++) {
            MBasicBlock *succ = block->getSuccessor(i);
            int found = 0;

            for (size_t j = 0; j < succ->numPredecessors(); j++) {
                if (succ->getPredecessor(j) == *block)
                    found++;
            }

            JS_ASSERT(found == 1);
        }

        
        for (size_t i = 0; i < block->numPredecessors(); i++) {
            MBasicBlock *pred = block->getPredecessor(i);
            int found = 0;

            for (size_t j = 0; j < pred->numSuccessors(); j++) {
                if (pred->getSuccessor(j) == *block)
                    found++;
            }

            JS_ASSERT(found == 1);
        }
    }
#endif
}

