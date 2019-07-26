






#include <stdio.h>

#include "Ion.h"
#include "IonSpewer.h"
#include "EdgeCaseAnalysis.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

EdgeCaseAnalysis::EdgeCaseAnalysis(MIRGraph &graph)
  : graph(graph)
{
}

bool
EdgeCaseAnalysis::analyzeLate()
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++)
            iter->analyzeEdgeCasesForward();
    }

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        for (MInstructionReverseIterator riter(block->rbegin()); riter != block->rend(); riter++)
            riter->analyzeEdgeCasesBackward();
    }

    return true;
}

bool
EdgeCaseAnalysis::analyzeEarly()
{

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        for (MInstructionReverseIterator riter(block->rbegin()); riter != block->rend(); riter++)
            riter->analyzeTruncateBackward();
    }

    return true;
}

bool
EdgeCaseAnalysis::AllUsesTruncate(MInstruction *m)
{
    for (MUseIterator use = m->usesBegin(); use != m->usesEnd(); use++) {
        if (use->node()->isResumePoint())
            return false;

        MDefinition *def = use->node()->toDefinition();
        if (def->isTruncateToInt32())
            continue;
        if (def->isBitAnd())
            continue;
        if (def->isBitOr())
            continue;
        if (def->isBitXor())
            continue;
        if (def->isLsh())
            continue;
        if (def->isRsh())
            continue;
        if (def->isBitNot())
            continue;
        if (def->isAdd() && def->toAdd()->isTruncated())
            continue;
        if (def->isSub() && def->toSub()->isTruncated())
            continue;
        
        return false;
    }
    return true;
}
