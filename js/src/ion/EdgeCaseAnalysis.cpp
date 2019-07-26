






#include <stdio.h>

#include "Ion.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "EdgeCaseAnalysis.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

EdgeCaseAnalysis::EdgeCaseAnalysis(MIRGenerator *mir, MIRGraph &graph)
  : mir(mir), graph(graph)
{
}

bool
EdgeCaseAnalysis::analyzeLate()
{
    
    uint32_t nextId = 1;

    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Analyze Late (first loop)"))
            return false;
        for (MDefinitionIterator iter(*block); iter; iter++) {
            iter->setId(nextId++);
            iter->analyzeEdgeCasesForward();
        }
    }

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Analyze Late (second loop)"))
            return false;
        for (MInstructionReverseIterator riter(block->rbegin()); riter != block->rend(); riter++)
            riter->analyzeEdgeCasesBackward();
    }

    return true;
}

int
EdgeCaseAnalysis::AllUsesTruncate(MInstruction *m)
{
    
    
    int ret = 1;
    for (MUseIterator use = m->usesBegin(); use != m->usesEnd(); use++) {
        
        if (use->consumer()->isResumePoint())
            continue;

        MDefinition *def = use->consumer()->toDefinition();
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
        if (def->isAdd() && def->toAdd()->isTruncated()) {
            ret = Max(ret, def->toAdd()->isTruncated() + 1);
            continue;
        }
        if (def->isSub() && def->toSub()->isTruncated()) {
            ret = Max(ret, def->toSub()->isTruncated() + 1);
            continue;
        }
        
        return 0;
    }
    return ret;
}
