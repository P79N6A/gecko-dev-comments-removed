





#ifndef jit_UnreachableCodeElimination_h
#define jit_UnreachableCodeElimination_h

#include "jit/MIRGraph.h"

namespace js {
namespace jit {

class MIRGraph;

class UnreachableCodeElimination
{
    typedef Vector<MBasicBlock *, 16, SystemAllocPolicy> BlockList;

    MIRGenerator *mir_;
    MIRGraph &graph_;
    uint32_t marked_;
    bool redundantPhis_;
    bool rerunAliasAnalysis_;

    bool prunePointlessBranchesAndMarkReachableBlocks();
    void checkDependencyAndRemoveUsesFromUnmarkedBlocks(MDefinition *instr);
    bool removeUnmarkedBlocksAndClearDominators();
    bool removeUnmarkedBlocksAndCleanup();

    bool enqueue(MBasicBlock *block, BlockList &list);
    MBasicBlock *optimizableSuccessor(MBasicBlock *block);

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
