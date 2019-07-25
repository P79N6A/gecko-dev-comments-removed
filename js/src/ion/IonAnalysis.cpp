








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;




























class TypeAnalyzer
{
    MIRGenerator *gen;
    MIRGraph &graph;
    js::Vector<MInstruction *, 0, ContextAllocPolicy> worklist;

  private:
    bool addToWorklist(MInstruction *ins);
    MInstruction *popFromWorklist();

  public:
    TypeAnalyzer(MIRGenerator *gen, MIRGraph &graph);

    bool analyze();
    void inspectOperands(MInstruction *ins);
    bool propagateUsedTypes(MInstruction *ins);
};

TypeAnalyzer::TypeAnalyzer(MIRGenerator *gen, MIRGraph &graph)
  : gen(gen),
    graph(graph),
    worklist(gen->cx)
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

void
TypeAnalyzer::inspectOperands(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MIRType required = ins->requiredInputType(i);
        if (required >= MIRType_Value)
            continue;
        ins->getInput(i)->useAsType(required);
    }
}

bool
TypeAnalyzer::propagateUsedTypes(MInstruction *ins)
{
    
    if (ins->isCopy()) {
        MCopy *copy = ins->toCopy();
        MInstruction *input = copy->getInput(0);
        input->addUsedTypes(copy->usedTypes());
        return true;
    }

    
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
TypeAnalyzer::analyze()
{
    
    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (size_t i = 0; i < block->numPhis(); i++) {
            if (!addToWorklist(block->getPhi(i)))
                return false;
        }
        for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
            if (!addToWorklist(*i))
                return false;
        }
    }

    while (!worklist.empty()) {
        MInstruction *ins = popFromWorklist();
        if (ins->isPhi() || ins->isCopy()) {
            if (!propagateUsedTypes(ins))
                return false;
        } else {
            
            
            inspectOperands(ins);
        }
    }

    return true;
}

bool
ion::ApplyTypeInformation(MIRGenerator *gen, MIRGraph &graph)
{
    TypeAnalyzer analysis(gen, graph);
    if (!analysis.analyze())
        return false;
    return true;
}

