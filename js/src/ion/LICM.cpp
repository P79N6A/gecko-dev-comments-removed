






#include <stdio.h>

#include "Ion.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "LICM.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

LICM::LICM(MIRGenerator *mir, MIRGraph &graph)
  : mir(mir), graph(graph)
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

        
        Loop loop(mir, header->backedge(), header);

        Loop::LoopReturn lr = loop.init();
        if (lr == Loop::LoopReturn_Error)
            return false;
        if (lr == Loop::LoopReturn_Skip) {
            graph.unmarkBlocks();
            continue;
        }

        if (!loop.optimize())
            return false;

        graph.unmarkBlocks();
    }

    return true;
}

Loop::Loop(MIRGenerator *mir, MBasicBlock *footer, MBasicBlock *header)
  : mir(mir),
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

    IonSpew(IonSpew_LICM, "These instructions are in the loop: ");

    while (!worklist_.empty()) {
        if (mir->shouldCancel("LICM (worklist)"))
            return false;

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
        }
    }

    if (!hoistInstructions(invariantInstructions))
        return false;
    return true;
}

bool
Loop::hoistInstructions(InstructionQueue &toHoist)
{
    
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

    return true;
}

bool
Loop::isInLoop(MDefinition *ins)
{
    return ins->block()->isMarked();
}

bool
Loop::isBeforeLoop(MDefinition *ins)
{
    return ins->block()->id() < header_->id();
}

bool
Loop::isLoopInvariant(MInstruction *ins)
{
    if (!isHoistable(ins)) {
        if (IonSpewEnabled(IonSpew_LICM))
            fprintf(IonSpewFile, "not hoistable\n");
        return false;
    }

    
    
    
    
    if (ins->dependency() && !isBeforeLoop(ins->dependency())) {
        if (IonSpewEnabled(IonSpew_LICM)) {
            fprintf(IonSpewFile, "depends on store inside or after loop: ");
            ins->dependency()->printName(IonSpewFile);
            fprintf(IonSpewFile, "\n");
        }
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
