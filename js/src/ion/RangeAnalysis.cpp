





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
RangeAnalysis::analyze()
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++) {
            (*iter)->analyzeRange();
        }
    }

    return true;
}
