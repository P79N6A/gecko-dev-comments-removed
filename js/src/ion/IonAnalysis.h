






#ifndef jsion_ion_analysis_h__
#define jsion_ion_analysis_h__



#include "IonAllocPolicy.h"

namespace js {
namespace ion {

class MIRGenerator;
class MIRGraph;

bool
SplitCriticalEdges(MIRGraph &graph);

bool
EliminatePhis(MIRGenerator *mir, MIRGraph &graph);

bool
EliminateDeadCode(MIRGenerator *mir, MIRGraph &graph);

bool
ApplyTypeInformation(MIRGenerator *mir, MIRGraph &graph);

bool
RenumberBlocks(MIRGraph &graph);

bool
BuildDominatorTree(MIRGraph &graph);

bool
BuildPhiReverseMapping(MIRGraph &graph);

void
AssertGraphCoherency(MIRGraph &graph);

bool
EliminateRedundantBoundsChecks(MIRGraph &graph);



class MDefinition;

struct LinearSum
{
    MDefinition *term;
    int32 constant;

    LinearSum(MDefinition *term, int32 constant)
        : term(term), constant(constant)
    {}
};

LinearSum
ExtractLinearSum(MDefinition *ins);

} 
} 

#endif

