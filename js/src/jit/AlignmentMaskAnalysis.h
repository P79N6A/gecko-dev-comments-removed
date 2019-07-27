





#ifndef jit_AlignmentMaskAnalysis_h
#define jit_AlignmentMaskAnalysis_h

namespace js {
namespace jit {

class MIRGraph;

class AlignmentMaskAnalysis
{
    MIRGraph &graph_;

  public:
    explicit AlignmentMaskAnalysis(MIRGraph &graph)
      : graph_(graph)
    {}

    bool analyze();
};

} 
} 

#endif 
