






#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;




bool
ion::SplitCriticalEdges(MIRGraph &graph)
{
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        if (block->numSuccessors() < 2)
            continue;
        for (size_t i = 0; i < block->numSuccessors(); i++) {
            MBasicBlock *target = block->getSuccessor(i);
            if (target->numPredecessors() < 2)
                continue;

            
            MBasicBlock *split = MBasicBlock::NewSplitEdge(graph, block->info(), *block);
            split->setLoopDepth(block->loopDepth());
            graph.insertBlockAfter(*block, split);
            split->end(MGoto::New(target));

            block->replaceSuccessor(i, split);
            target->replacePredecessor(*block, split);
        }
    }
    return true;
}











bool
ion::EliminateDeadResumePointOperands(MIRGenerator *mir, MIRGraph &graph)
{
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Dead Resume Point Operands (main loop)"))
            return false;

        
        if (block->isLoopHeader() && block->backedge() == *block)
            continue;

        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            
            if (ins->isConstant())
                continue;

            
            
            
            
            
            if (ins->isUnbox() || ins->isParameter())
                continue;

            
            
            
            if (ins->isFolded())
                continue;

            
            
            
            
            
            uint32_t maxDefinition = 0;
            for (MUseDefIterator uses(*ins); uses; uses++) {
                if (uses.def()->block() != *block ||
                    uses.def()->isBox() ||
                    uses.def()->isPassArg() ||
                    uses.def()->isPhi())
                {
                    maxDefinition = UINT32_MAX;
                    break;
                }
                maxDefinition = Max(maxDefinition, uses.def()->id());
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

                
                
                
                
                
                
                
                
                MConstant *constant = MConstant::New(UndefinedValue());
                block->insertBefore(*(block->begin()), constant);
                uses = mrp->replaceOperand(uses, constant);
            }
        }
    }

    return true;
}




bool
ion::EliminateDeadCode(MIRGenerator *mir, MIRGraph &graph)
{
    
    
    for (PostorderIterator block = graph.poBegin(); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Eliminate Dead Code (main loop)"))
            return false;

        
        for (MInstructionReverseIterator inst = block->rbegin(); inst != block->rend(); ) {
            if (!inst->isEffectful() && !inst->resumePoint() &&
                !inst->hasUses() && !inst->isGuard() &&
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
IsPhiObservable(MPhi *phi, Observability observe)
{
    
    
    if (phi->isFolded())
        return true;

    
    
    
    
    
    
    
    
    switch (observe) {
      case AggressiveObservability:
        for (MUseDefIterator iter(phi); iter; iter++) {
            if (!iter.def()->isPhi())
                return true;
        }
        break;

      case ConservativeObservability:
        for (MUseIterator iter(phi->usesBegin()); iter != phi->usesEnd(); iter++) {
            if (!iter->consumer()->isDefinition() ||
                !iter->consumer()->toDefinition()->isPhi())
                return true;
        }
        break;
    }

    
    uint32_t slot = phi->slot();
    if (slot == 1)
        return true;

    
    
    
    CompileInfo &info = phi->block()->info();
    if (info.fun() && info.hasArguments()) {
        uint32_t first = info.firstArgSlot();
        if (first <= slot && slot - first < info.nargs())
            return true;
    }
    return false;
}




inline MDefinition *
IsPhiRedundant(MPhi *phi)
{
    MDefinition *first = phi->operandIfRedundant();
    if (first == NULL)
        return NULL;

    
    if (phi->isFolded())
        first->setFoldedUnchecked();

    return first;
}

bool
ion::EliminatePhis(MIRGenerator *mir, MIRGraph &graph,
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

        
        for (size_t i = 0; i < phi->numOperands(); i++) {
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











class TypeAnalyzer
{
    MIRGenerator *mir;
    MIRGraph &graph;
    Vector<MPhi *, 0, SystemAllocPolicy> phiWorklist_;

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

  public:
    TypeAnalyzer(MIRGenerator *mir, MIRGraph &graph)
      : mir(mir), graph(graph)
    { }

    bool analyze();
};


static MIRType
GuessPhiType(MPhi *phi)
{
    MIRType type = MIRType_None;
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition *in = phi->getOperand(i);
        if (in->isPhi()) {
            if (!in->toPhi()->triedToSpecialize())
                continue;
            if (in->type() == MIRType_None) {
                
                
                
                continue;
            }
        }
        if (type == MIRType_None) {
            type = in->type();
            continue;
        }
        if (type != in->type()) {
            
            if (IsNumberType(type) && IsNumberType(in->type()))
                type = MIRType_Double;
            else
                return MIRType_Value;
        }
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
    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Specialize Phis (main loop)"))
            return false;

        for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++) {
            MIRType type = GuessPhiType(*phi);
            phi->specialize(type);
            if (type == MIRType_None) {
                
                
                
                
                continue;
            }
            if (!propagateSpecialization(*phi))
                return false;
        }
    }

    while (!phiWorklist_.empty()) {
        if (mir->shouldCancel("Specialize Phis (worklist)"))
            return false;

        MPhi *phi = popPhi();
        if (!propagateSpecialization(phi))
            return false;
    }

    return true;
}

void
TypeAnalyzer::adjustPhiInputs(MPhi *phi)
{
    MIRType phiType = phi->type();

    if (phiType == MIRType_Double) {
        
        for (size_t i = 0; i < phi->numOperands(); i++) {
            MDefinition *in = phi->getOperand(i);

            if (in->type() == MIRType_Int32) {
                MToDouble *toDouble = MToDouble::New(in);
                in->block()->insertBefore(in->block()->lastIns(), toDouble);
                phi->replaceOperand(i, toDouble);
            } else {
                JS_ASSERT(in->type() == MIRType_Double);
            }
        }
        return;
    }

    if (phiType != MIRType_Value)
        return;

    
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
    js::Value v;
    switch (phi->type()) {
      case MIRType_Undefined:
        v = UndefinedValue();
        break;
      case MIRType_Null:
        v = NullValue();
        break;
      case MIRType_Magic:
        v = MagicValue(JS_OPTIMIZED_ARGUMENTS);
        break;
      default:
        JS_NOT_REACHED("unexpected type");
        return;
    }
    MConstant *c = MConstant::New(v);
    
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
            if (phi->type() <= MIRType_Null || phi->type() == MIRType_Magic) {
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
TypeAnalyzer::analyze()
{
    if (!specializePhis())
        return false;
    if (!insertConversions())
        return false;
    return true;
}

bool
ion::ApplyTypeInformation(MIRGenerator *mir, MIRGraph &graph)
{
    TypeAnalyzer analyzer(mir, graph);

    if (!analyzer.analyze())
        return false;

    return true;
}

bool
ion::RenumberBlocks(MIRGraph &graph)
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
    
    
    
    Vector<MBasicBlock *, 1, IonAllocPolicy> worklist;

    
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

        for (size_t i = 0; i < block->numImmediatelyDominatedBlocks(); i++) {
            if (!worklist.append(block->getImmediatelyDominatedBlock(i)))
                return false;
        }
        index++;
    }

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
CheckOperandImpliesUse(MInstruction *ins, MDefinition *operand)
{
    for (MUseIterator i = operand->usesBegin(); i != operand->usesEnd(); i++) {
        if (i->consumer()->isDefinition() && i->consumer()->toDefinition() == ins)
            return true;
    }
    return false;
}

static bool
CheckUseImpliesOperand(MInstruction *ins, MUse *use)
{
    MNode *consumer = use->consumer();
    uint32_t index = use->index();

    if (consumer->isDefinition()) {
        MDefinition *def = consumer->toDefinition();
        return (def->getOperand(index) == ins);
    }

    JS_ASSERT(consumer->isResumePoint());
    MResumePoint *res = consumer->toResumePoint();
    return (res->getOperand(index) == ins);
}
#endif 

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

void
ion::AssertGraphCoherency(MIRGraph &graph)
{
#ifdef DEBUG
    
    uint32_t count = 0;
    for (MBasicBlockIterator block(graph.begin()); block != graph.end(); block++) {
        count++;

        for (size_t i = 0; i < block->numSuccessors(); i++)
            JS_ASSERT(CheckSuccessorImpliesPredecessor(*block, block->getSuccessor(i)));

        for (size_t i = 0; i < block->numPredecessors(); i++)
            JS_ASSERT(CheckPredecessorImpliesSuccessor(*block, block->getPredecessor(i)));

        
        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            for (uint32_t i = 0; i < ins->numOperands(); i++)
                JS_ASSERT(CheckOperandImpliesUse(*ins, ins->getOperand(i)));
        }
        for (MInstructionIterator ins = block->begin(); ins != block->end(); ins++) {
            for (MUseIterator i(ins->usesBegin()); i != ins->usesEnd(); i++)
                JS_ASSERT(CheckUseImpliesOperand(*ins, *i));
        }
    }

    JS_ASSERT(graph.numBlocks() == count);

    AssertReversePostOrder(graph);
#endif
}

void
ion::AssertExtendedGraphCoherency(MIRGraph &graph)
{
    
    
    

#ifdef DEBUG
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
        JS_ASSERT_IF(successorWithPhis, block->successorWithPhis() != NULL);

        
        
        
        
        
    }
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
    if (!p || index > p->value.validUntil) {
        
        BoundsCheckInfo info;
        info.check = check;
        info.validUntil = index + check->block()->numDominated();

        if(!checks.put(hash, info))
            return NULL;

        return check;
    }

    return p->value.check;
}


SimpleLinearSum
ion::ExtractLinearSum(MDefinition *ins)
{
    if (ins->isBeta())
        ins = ins->getOperand(0);

    if (ins->type() != MIRType_Int32)
        return SimpleLinearSum(ins, 0);

    if (ins->isConstant()) {
        const Value &v = ins->toConstant()->value();
        JS_ASSERT(v.isInt32());
        return SimpleLinearSum(NULL, v.toInt32());
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
ion::ExtractLinearInequality(MTest *test, BranchDirection direction,
                             SimpleLinearSum *plhs, MDefinition **prhs, bool *plessEqual)
{
    if (!test->getOperand(0)->isCompare())
        return false;

    MCompare *compare = test->getOperand(0)->toCompare();

    MDefinition *lhs = compare->getOperand(0);
    MDefinition *rhs = compare->getOperand(1);

    if (compare->compareType() != MCompare::Compare_Int32)
        return false;

    JS_ASSERT(lhs->type() == MIRType_Int32);
    JS_ASSERT(rhs->type() == MIRType_Int32);

    JSOp jsop = compare->jsop();
    if (direction == FALSE_BRANCH)
        jsop = analyze::NegateCompareOp(jsop);

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

    
    
    
    
    
    

    
    if (test->getOperand(0) == barrier->input() && direction == TRUE_BRANCH) {
        *eliminated = true;
        barrier->replaceAllUsesWith(barrier->input());
        return;
    }

    if (!test->getOperand(0)->isCompare())
        return;

    MCompare *compare = test->getOperand(0)->toCompare();
    MCompare::CompareType compareType = compare->compareType();

    if (compareType != MCompare::Compare_Undefined && compareType != MCompare::Compare_Null)
        return;
    if (compare->getOperand(0) != barrier->input())
        return;

    JSOp op = compare->jsop();
    JS_ASSERT(op == JSOP_EQ || op == JSOP_STRICTEQ ||
              op == JSOP_NE || op == JSOP_STRICTNE);

    if ((direction == TRUE_BRANCH) != (op == JSOP_NE || op == JSOP_STRICTNE))
        return;

    
    
    
    if (op == JSOP_STRICTEQ || op == JSOP_STRICTNE) {
        if (compareType == MCompare::Compare_Undefined && !filtersUndefined)
            return;
        if (compareType == MCompare::Compare_Null && !filtersNull)
            return;
    }

    *eliminated = true;
    barrier->replaceAllUsesWith(barrier->input());
}

static bool
TryEliminateTypeBarrier(MTypeBarrier *barrier, bool *eliminated)
{
    JS_ASSERT(!*eliminated);

    const types::StackTypeSet *barrierTypes = barrier->typeSet();
    const types::StackTypeSet *inputTypes = barrier->input()->typeSet();

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
ion::EliminateRedundantChecks(MIRGraph &graph)
{
    BoundsCheckMap checks;

    if (!checks.init())
        return false;

    
    Vector<MBasicBlock *, 1, IonAllocPolicy> worklist;

    
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

        
        for (size_t i = 0; i < block->numImmediatelyDominatedBlocks(); i++) {
            if (!worklist.append(block->getImmediatelyDominatedBlock(i)))
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
