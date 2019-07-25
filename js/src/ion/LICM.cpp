








































#include <stdio.h>

#include "MIR.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "LICM.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

LICM::LICM(MIRGraph &graph)
  : graph(graph)
{
}

bool
LICM::analyze()
{
    IonSpew(IonSpew_LICM, "Beginning LICM pass ...");
    
    for (size_t i = 0; i < graph.numBlocks(); i ++) {
        MBasicBlock *header = graph.getBlock(i);
        if (header->isLoopHeader()) {
            
            MBasicBlock *footer = header->getPredecessor(header->numPredecessors() - 1);

            
            Loop loop(footer, header, graph);
            if (!loop.init())
                return false;
            if (!loop.optimize())
                return false;
        }
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

bool
Loop::init()
{
    IonSpew(IonSpew_LICM, "Loop identified, headed by block %d", header_->id());
    
    
    JS_ASSERT(header_->id() > header_->getPredecessor(0)->id());
#ifdef DEBUG
    for (size_t i = 1; i < header_->numPredecessors(); i ++) {
        JS_ASSERT(header_->id() <= header_->getPredecessor(i)->id());
    }
#endif

    if (!iterateLoopBlocks(footer_))
        return false;

    graph.unmarkBlocks();
    return true;
}

bool
Loop::iterateLoopBlocks(MBasicBlock *current)
{
    
    current->mark();

    
    
    if (current != header_) {
        for (size_t i = 0; i < current->numPredecessors(); i++) {
            if (current->getPredecessor(i)->isMarked())
                continue;
            if (!iterateLoopBlocks(current->getPredecessor(i)))
                return false;
        }
    }

    
    for (MInstructionIterator i = current->begin(); i != current->end(); i ++) {
        MInstruction *ins = *i;

        if (isHoistable(ins)) {
            if (!insertInWorklist(ins))
                return false;
        }
    }
    return true;
}

bool
Loop::optimize()
{
    InstructionQueue invariantInstructions;
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
            if (!invariantInstructions.insert(invariantInstructions.begin(), ins))
                return false;

            
            MUse *use = ins->uses();
            while (use != NULL) {
                
                
                
                if (!use->ins()->inWorklist() &&
                    isInLoop(use->ins()) &&
                    isHoistable(use->ins())) {

                    if(!insertInWorklist(use->ins()))
                        return false;
                }
                use = use->next();
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
    return true;
}

bool
Loop::isInLoop(MInstruction *ins)
{
    return ins->block()->id() >= header_->id();
}

bool
Loop::isLoopInvariant(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i ++) {

        
        if (isInLoop(ins->getInput(i)) &&
            !ins->getInput(i)->isLoopInvariant()) {

            if (IonSpewEnabled(IonSpew_LICM)) {
                ins->getInput(i)->printName(IonSpewFile);
                fprintf(IonSpewFile, " is in the loop.\n");
            }

            return false;
        }
    }
    return true;
}

bool
Loop::isHoistable(MInstruction *ins)
{
    if (ins->op() == MInstruction::Op_Phi ||
        ins->op() == MInstruction::Op_Test ||
        ins->op() == MInstruction::Op_Goto)
        return false;

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

