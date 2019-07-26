





#include <stdio.h>

#include "ion/Ion.h"
#include "ion/IonBuilder.h"
#include "ion/IonSpewer.h"
#include "ion/EdgeCaseAnalysis.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"

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
