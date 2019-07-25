








































#ifndef jsion_ion_analysis_h__
#define jsion_ion_analysis_h__



#include "IonAllocPolicy.h"

namespace js {
namespace ion {

class MIRGenerator;
class MIRGraph;

bool
SplitCriticalEdges(MIRGenerator *gen, MIRGraph &graph);

bool
EliminateDeadPhis(MIRGraph &graph);

bool
EliminateDeadCode(MIRGraph &graph);

bool
ApplyTypeInformation(MIRGraph &graph);

bool
ReorderBlocks(MIRGraph &graph);

bool
BuildPhiReverseMapping(MIRGraph &graph);

bool
BuildDominatorTree(MIRGraph &graph);

} 
} 

#endif 

