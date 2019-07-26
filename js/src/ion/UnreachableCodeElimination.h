






#ifndef jsion_unreachable_code_elimination_h__
#define jsion_unreachable_code_elimination_h__

#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

class MIRGraph;

class UnreachableCodeElimination
{
    MIRGenerator *mir_;
    MIRGraph &graph_;
    uint32_t marked_;
    bool redundantPhis_;
    bool rerunAliasAnalysis_;

    bool prunePointlessBranchesAndMarkReachableBlocks();
    void checkDependencyAndRemoveUsesFromUnmarkedBlocks(MDefinition *instr);
    bool removeUnmarkedBlocksAndClearDominators();
    bool removeUnmarkedBlocksAndCleanup();

  public:
    UnreachableCodeElimination(MIRGenerator *mir, MIRGraph &graph)
      : mir_(mir),
        graph_(graph),
        marked_(0),
        redundantPhis_(false),
        rerunAliasAnalysis_(false)
    {}

    
    bool analyze();

    
    
    
    bool removeUnmarkedBlocks(size_t marked);
};

} 
} 

#endif
