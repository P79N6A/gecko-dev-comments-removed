





#ifndef jit_AliasAnalysis_h
#define jit_AliasAnalysis_h

#include "jit/MIR.h"
#include "jit/MIRGraph.h"

namespace js {
namespace jit {

class LoopAliasInfo;
class MIRGraph;

class AliasAnalysis
{
    MIRGenerator *mir;
    MIRGraph &graph_;
    LoopAliasInfo *loop_;

    TempAllocator &alloc() const {
        return graph_.alloc();
    }

  public:
    AliasAnalysis(MIRGenerator *mir, MIRGraph &graph);
    bool analyze();
};

} 
} 

#endif 
