








































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
    bool inspectOperands(MInstruction *ins);
    bool reflow(MInstruction *ins);
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

bool
TypeAnalyzer::inspectOperands(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
    }
    return true;
}

bool
TypeAnalyzer::reflow(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        if (!addToWorklist(ins->getOperand(i)->ins()))
            return false;
    }
    for (MUseIterator iter(ins); iter.more(); iter.next()) {
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
        for (size_t i = 0; i < block->numInstructions(); i++) {
            if (!addToWorklist(block->getInstruction(i)))
                return false;
        }
    }

    while (!worklist.empty()) {
        MInstruction *ins = popFromWorklist();
        if (!inspectOperands(ins))
            return false;
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

