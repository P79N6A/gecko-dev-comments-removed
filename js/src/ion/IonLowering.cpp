








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonLowering.h"

using namespace js;
using namespace js::ion;



class LoweringPhase : public MInstructionVisitor
{
    MIRGraph &graph;

  public:
    LoweringPhase(MIRGraph &graph)
      : graph(graph)
    { }

    bool lowerBlock(MBasicBlock *block);
    bool lowerInstruction(MInstruction *ins);
    bool analyze();

#define VISITOR(op) bool visit(M##op *ins) { return true; }
    MIR_OPCODE_LIST(VISITOR)
#undef VISITOR
};

bool
LoweringPhase::lowerInstruction(MInstruction *ins)
{
    return ins->accept(this);
}

bool
LoweringPhase::lowerBlock(MBasicBlock *block)
{
    
    for (size_t i = 0; i < block->numPhis(); i++) {
        if (!lowerInstruction(block->getPhi(i)))
            return false;
    }
    for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
        if (!lowerInstruction(*i))
            return false;
    }
    return true;
}

bool
LoweringPhase::analyze()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (!lowerBlock(graph.getBlock(i)))
            return false;
    }
    return true;
}

bool
ion::Lower(MIRGraph &graph)
{
    LoweringPhase phase(graph);
    return phase.analyze();
}

