





#include "jit/EdgeCaseAnalysis.h"

#include "jit/MIR.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;

EdgeCaseAnalysis::EdgeCaseAnalysis(MIRGenerator *mir, MIRGraph &graph)
  : mir(mir), graph(graph)
{
}

bool
EdgeCaseAnalysis::analyzeLate()
{
    
    uint32_t nextId = 0;

    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("Analyze Late (first loop)"))
            return false;
        for (MDefinitionIterator iter(*block); iter; iter++) {
            iter->setId(nextId++);
            iter->analyzeEdgeCasesForward();
        }
        block->lastIns()->setId(nextId++);
    }

    for (PostorderIterator block(graph.poBegin()); block != graph.poEnd(); block++) {
        if (mir->shouldCancel("Analyze Late (second loop)"))
            return false;
        for (MInstructionReverseIterator riter(block->rbegin()); riter != block->rend(); riter++)
            riter->analyzeEdgeCasesBackward();
    }

    return true;
}
