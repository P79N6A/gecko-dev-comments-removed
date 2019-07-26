








































#include <stdio.h>

#include "Ion.h"
#include "IonSpewer.h"
#include "LICM.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

bool
ion::ExtractLinearInequality(MTest *test, BranchDirection direction,
                             LinearSum *plhs, MDefinition **prhs, bool *plessEqual)
{
    if (!test->getOperand(0)->isCompare())
        return false;

    MCompare *compare = test->getOperand(0)->toCompare();

    MDefinition *lhs = compare->getOperand(0);
    MDefinition *rhs = compare->getOperand(1);

    if (compare->specialization() != MIRType_Int32)
        return false;

    JS_ASSERT(lhs->type() == MIRType_Int32);
    JS_ASSERT(rhs->type() == MIRType_Int32);

    JSOp jsop = compare->jsop();
    if (direction == FALSE_BRANCH)
        jsop = analyze::NegateCompareOp(jsop);

    LinearSum lsum = ExtractLinearSum(lhs);
    LinearSum rsum = ExtractLinearSum(rhs);

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

LICM::LICM(MIRGraph &graph)
  : graph(graph)
{
}

bool
LICM::analyze()
{
    IonSpew(IonSpew_LICM, "Beginning LICM pass.");

    
    for (ReversePostorderIterator i(graph.rpoBegin()); i != graph.rpoEnd(); i++) {
        MBasicBlock *header = *i;

        
        if (!header->isLoopHeader() || header->numPredecessors() < 2)
            continue;

        
        Loop loop(header->backedge(), header, graph);

        Loop::LoopReturn lr = loop.init();
        if (lr == Loop::LoopReturn_Error)
            return false;
        if (lr == Loop::LoopReturn_Skip)
            continue;

        if (!loop.optimize())
            return false;
    }

    return true;
}

Loop::Loop(MBasicBlock *footer, MBasicBlock *header, MIRGraph &graph)
  : graph(graph),
    footer_(footer),
    header_(header)
{
    preLoop_ = header_->getPredecessor(0);
}

Loop::LoopReturn
Loop::init()
{
    IonSpew(IonSpew_LICM, "Loop identified, headed by block %d", header_->id());
    IonSpew(IonSpew_LICM, "footer is block %d", footer_->id());

    
    JS_ASSERT(header_->id() > header_->getPredecessor(0)->id());

    LoopReturn lr = iterateLoopBlocks(footer_);
    if (lr == LoopReturn_Error)
        return LoopReturn_Error;

    graph.unmarkBlocks();
    return lr;
}

Loop::LoopReturn
Loop::iterateLoopBlocks(MBasicBlock *current)
{
    
    current->mark();

    
    
    
    
    
    
    if (current->immediateDominator() == current)
        return LoopReturn_Skip;

    
    
    if (current != header_) {
        for (size_t i = 0; i < current->numPredecessors(); i++) {
            if (current->getPredecessor(i)->isMarked())
                continue;
            LoopReturn lr = iterateLoopBlocks(current->getPredecessor(i));
            if (lr != LoopReturn_Success)
                return lr;
        }
    }

    
    for (MInstructionIterator i = current->begin(); i != current->end(); i++) {
        MInstruction *ins = *i;

        if (ins->isMovable() && !ins->isEffectful()) {
            if (!insertInWorklist(ins))
                return LoopReturn_Error;
        }
    }
    return LoopReturn_Success;
}

bool
Loop::optimize()
{
    InstructionQueue invariantInstructions;
    InstructionQueue boundsChecks;

    IonSpew(IonSpew_LICM, "These instructions are in the loop: ");

    while (!worklist_.empty()) {
        MInstruction *ins = popFromWorklist();

        IonSpewHeader(IonSpew_LICM);

        if (IonSpewEnabled(IonSpew_LICM)) {
            ins->printName(IonSpewFile);
            fprintf(IonSpewFile, " <- ");
            ins->printOpcode(IonSpewFile);
            fprintf(IonSpewFile, ":  ");
        }

        if (isLoopInvariant(ins)) {
            
            ins->setLoopInvariant();
            if (!invariantInstructions.append(ins))
                return false;

            
            for (MUseDefIterator iter(ins->toDefinition()); iter; iter++) {
                MDefinition *consumer = iter.def();

                if (consumer->isInWorklist())
                    continue;

                
                
                if (isInLoop(consumer) && isHoistable(consumer)) {
                    if (!insertInWorklist(consumer->toInstruction()))
                        return false;
                }
            }

            if (IonSpewEnabled(IonSpew_LICM))
                fprintf(IonSpewFile, " Loop Invariant!\n");
        } else if (ins->isBoundsCheck()) {
            if (!boundsChecks.append(ins))
                return false;
        }
    }

    if (!hoistInstructions(invariantInstructions, boundsChecks))
        return false;
    return true;
}

bool
Loop::hoistInstructions(InstructionQueue &toHoist, InstructionQueue &boundsChecks)
{
    
    
    
    InstructionQueue hoistedChecks;
    for (size_t i = 0; i < boundsChecks.length(); i++) {
        MBoundsCheck *ins = boundsChecks[i]->toBoundsCheck();
        if (isLoopInvariant(ins) || !isInLoop(ins))
            continue;

        
        
        
        
        MBasicBlock *block = ins->block();
        while (true) {
            BranchDirection direction;
            MTest *branch = block->immediateDominatorBranch(&direction);
            if (branch) {
                MInstruction *upper, *lower;
                tryHoistBoundsCheck(ins, branch, direction, &upper, &lower);
                if (upper && !hoistedChecks.append(upper))
                    return false;
                if (lower && !hoistedChecks.append(lower))
                    return false;
                if (upper || lower) {
                    
                    
                    
                    
                    
                    ins->replaceAllUsesWith(ins->index());
                    ins->block()->discard(ins);
                    break;
                }
            }
            MBasicBlock *dom = block->immediateDominator();
            if (dom == block)
                break;
            block = dom;
        }
    }

    
    for (size_t i = 0; i < toHoist.length(); i++) {
        MInstruction *ins = toHoist[i];

        
        
        JS_ASSERT(!ins->isControlInstruction());
        JS_ASSERT(!ins->isEffectful());
        JS_ASSERT(ins->isMovable());

        if (checkHotness(ins->block())) {
            ins->block()->moveBefore(preLoop_->lastIns(), ins);
            ins->setNotLoopInvariant();
        }
    }

    for (size_t i = 0; i < hoistedChecks.length(); i++) {
        MInstruction *ins = hoistedChecks[i];
        preLoop_->insertBefore(preLoop_->lastIns(), ins);
    }

    return true;
}

bool
Loop::isInLoop(MDefinition *ins)
{
    return ins->block()->id() >= header_->id();
}

bool
Loop::isLoopInvariant(MInstruction *ins)
{
    if (!isHoistable(ins))
        return false;

    
    if (ins->dependency() && isInLoop(ins->dependency())) {
        if (IonSpewEnabled(IonSpew_LICM))
            fprintf(IonSpewFile, "depends on store inside loop.\n");
        return false;
    }

    
    
    for (size_t i = 0; i < ins->numOperands(); i ++) {
        if (isInLoop(ins->getOperand(i)) &&
            !ins->getOperand(i)->isLoopInvariant()) {

            if (IonSpewEnabled(IonSpew_LICM)) {
                ins->getOperand(i)->printName(IonSpewFile);
                fprintf(IonSpewFile, " is in the loop.\n");
            }

            return false;
        }
    }
    return true;
}

bool
Loop::isLoopInvariant(MDefinition *ins)
{
    if (!isInLoop(ins))
        return true;

    return ins->isInstruction() && isLoopInvariant(ins->toInstruction());
}

bool
Loop::checkHotness(MBasicBlock *block)
{
    
    
    
    return true;
}

bool
Loop::insertInWorklist(MInstruction *ins)
{
    if (!worklist_.insert(worklist_.begin(), ins))
        return false;
    ins->setInWorklist();
    return true;
}

MInstruction*
Loop::popFromWorklist()
{
    MInstruction* toReturn = worklist_.popCopy();
    toReturn->setNotInWorklist();
    return toReturn;
}






























void
Loop::tryHoistBoundsCheck(MBoundsCheck *ins, MTest *test, BranchDirection direction,
                          MInstruction **pupper, MInstruction **plower)
{
    *pupper = NULL;
    *plower = NULL;

    if (!isLoopInvariant(ins->length()))
        return;

    LinearSum lhs(NULL, 0);
    MDefinition *rhs;
    bool lessEqual;
    if (!ExtractLinearInequality(test, direction, &lhs, &rhs, &lessEqual))
        return;

    
    if (rhs && !isLoopInvariant(rhs)) {
        if (lhs.term && !isLoopInvariant(lhs.term))
            return;
        MDefinition *temp = lhs.term;
        lhs.term = rhs;
        rhs = temp;
        if (!SafeSub(0, lhs.constant, &lhs.constant))
            return;
        lessEqual = !lessEqual;
    }

    JS_ASSERT_IF(rhs, isLoopInvariant(rhs));

    
    if (!lhs.term || !lhs.term->isPhi() || lhs.term->block() != header_)
        return;

    
    LinearSum index = ExtractLinearSum(ins->index());
    if (index.term != lhs.term)
        return;

    if (!lessEqual)
        return;

    
    
    
    
    

    int32 adjustment;
    if (!SafeSub(index.constant, lhs.constant, &adjustment))
        return;
    if (!SafeAdd(adjustment, ins->maximum(), &adjustment))
        return;

    
    
    
    
    
    
    

    uint32 position = preLoop_->positionInPhiSuccessor();
    MDefinition *initialIndex = lhs.term->toPhi()->getOperand(position);
    if (!nonDecreasing(initialIndex, lhs.term))
        return;

    int32 lowerBound;
    if (!SafeSub(0, index.constant, &lowerBound))
        return;
    if (!SafeSub(lowerBound, ins->minimum(), &lowerBound))
        return;

    

    if (!rhs) {
        rhs = MConstant::New(Int32Value(adjustment));
        adjustment = 0;
        preLoop_->insertBefore(preLoop_->lastIns(), rhs->toInstruction());
    }

    MBoundsCheck *upper = MBoundsCheck::New(rhs, ins->length());
    upper->setMinimum(adjustment);
    upper->setMaximum(adjustment);

    MBoundsCheckLower *lower = MBoundsCheckLower::New(initialIndex);
    lower->setMinimum(lowerBound);

    *pupper = upper;
    *plower = lower;
}



bool
Loop::nonDecreasing(MDefinition *initial, MDefinition *start)
{
    MDefinitionVector worklist;
    MDefinitionVector seen;

    if (!worklist.append(start))
        return false;

    while (!worklist.empty()) {
        MDefinition *def = worklist.popCopy();
        bool duplicate = false;
        for (size_t i = 0; i < seen.length() && !duplicate; i++) {
            if (seen[i] == def)
                duplicate = true;
        }
        if (duplicate)
            continue;
        if (!seen.append(def))
            return false;

        if (def->type() != MIRType_Int32)
            return false;

        if (!isInLoop(def)) {
            if (def != initial)
                return false;
            continue;
        }

        if (def->isPhi()) {
            MPhi *phi = def->toPhi();
            for (size_t i = 0; i < phi->numOperands(); i++) {
                if (!worklist.append(phi->getOperand(i)))
                    return false;
            }
            continue;
        }

        if (def->isAdd()) {
            if (def->toAdd()->specialization() != MIRType_Int32)
                return false;
            MDefinition *lhs = def->toAdd()->getOperand(0);
            MDefinition *rhs = def->toAdd()->getOperand(1);
            if (!rhs->isConstant())
                return false;
            Value v = rhs->toConstant()->value();
            if (!v.isInt32() || v.toInt32() < 0)
                return false;
            if (!worklist.append(lhs))
                return false;
            continue;
        }

        return false;
    }

    return true;
}
