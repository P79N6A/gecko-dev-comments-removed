





#include <stdio.h>

#include "Ion.h"
#include "IonSpewer.h"
#include "RangeAnalysis.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

RangeAnalysis::RangeAnalysis(MIRGraph &graph)
  : graph(graph)
{
}

bool
RangeAnalysis::analyzeLate()
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++)
            iter->analyzeRangeForward();
    }

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        for (MInstructionReverseIterator riter(block->rbegin()); riter != block->rend(); riter++)
            riter->analyzeRangeBackward();
    }

    return true;
}

bool
RangeAnalysis::analyzeEarly()
{

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        for (MInstructionReverseIterator riter(block->rbegin()); riter != block->rend(); riter++)
            riter->analyzeTruncateBackward();
    }

    return true;
}

bool
RangeAnalysis::AllUsesTruncate(MInstruction *m)
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
