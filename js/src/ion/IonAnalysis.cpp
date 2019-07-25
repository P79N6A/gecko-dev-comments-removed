








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;




























class TypeAnalyzer
{
    MIRGraph &graph;
    js::Vector<MInstruction *, 0, IonAllocPolicy> worklist;

  private:
    bool addToWorklist(MInstruction *ins);
    MInstruction *popFromWorklist();

  public:
    TypeAnalyzer(MIRGraph &graph);

    bool analyze();
    bool populate();
    bool propagate();
    bool inspectOperands(MInstruction *ins);
    bool propagateUsedTypes(MInstruction *ins);
};

TypeAnalyzer::TypeAnalyzer(MIRGraph &graph)
  : graph(graph)
{
}

bool
TypeAnalyzer::addToWorklist(MInstruction *ins)
{
    if (!ins->inWorklist()){
        ins->setInWorklist();
        return worklist.append(ins);
    }
    return true;
}

MInstruction *
TypeAnalyzer::popFromWorklist()
{
    MInstruction *ins = worklist.popCopy();
    ins->setNotInWorklist();
    return ins;
}

bool
TypeAnalyzer::inspectOperands(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MIRType required = ins->requiredInputType(i);
        if (required >= MIRType_Value)
            continue;
        ins->getInput(i)->useAsType(required);
    }

    return true;
}

bool
TypeAnalyzer::propagateUsedTypes(MInstruction *ins)
{
    
    MPhi *phi = ins->toPhi();
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MInstruction *input = phi->getInput(i);
        bool changed = input->addUsedTypes(phi->usedTypes());
        if (changed && (input->isPhi() || ins->isCopy())) {
            
            
            
            if (!addToWorklist(input))
                return false;
        }
    }

    return true;
}

bool
TypeAnalyzer::populate()
{
    
    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (size_t i = 0; i < block->numPhis(); i++) {
            if (!addToWorklist(block->getPhi(i)))
                return false;
        }
        MInstructionIterator i = block->begin();
        while (i != block->end()) {
            if (i->isCopy()) {
                
                MCopy *copy = i->toCopy();
                MUseIterator uses(copy);
                while (uses.more())
                    uses->ins()->replaceOperand(uses, copy->getInput(0));
                i = copy->block()->removeAt(i);
                continue;
            }
            addToWorklist(*i);
            i++;
        }
    }
    
    return true;
}

bool
TypeAnalyzer::propagate()
{
    
    while (!worklist.empty()) {
        MInstruction *ins = popFromWorklist();

        
        
        
        if (ins->adjustForInputs()) {
            for (MUseIterator uses(ins); uses.more(); uses.next()) {
                if (!addToWorklist(uses->ins()))
                    return false;
            }
        }

        
        JS_ASSERT(!ins->isCopy());

        if (ins->isPhi()) {
            if (!propagateUsedTypes(ins))
                return false;
        } else {
            
            
            if (!inspectOperands(ins))
                return false;
        }
    }

    return true;
}

bool
TypeAnalyzer::analyze()
{
    if (!populate())
        return false;

    if (!propagate())
        return false;

    return true;
}

bool
ion::ApplyTypeInformation(MIRGraph &graph)
{
    TypeAnalyzer analysis(graph);
    if (!analysis.analyze())
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

void
ion::RenumberInstructions(MIRGraph &graph)
{
    graph.resetInstructionNumber();

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (MInstructionIterator i = block->begin(); i != block->end(); i++)
            graph.allocInstructionId(*i);
    }
}

