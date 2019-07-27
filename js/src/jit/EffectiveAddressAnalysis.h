





#ifndef jit_EffectiveAddressAnalysis_h
#define jit_EffectiveAddressAnalysis_h

namespace js {
namespace jit {

class MIRGraph;

class EffectiveAddressAnalysis
{
    MIRGraph& graph_;

  public:
    explicit EffectiveAddressAnalysis(MIRGraph& graph)
      : graph_(graph)
    {}

    bool analyze();
};

} 
} 

#endif 
