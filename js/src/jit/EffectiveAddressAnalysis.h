





#ifndef jit_EffectiveAddressAnalysis_h
#define jit_EffectiveAddressAnalysis_h

#include "jit/MIR.h"
#include "jit/MIRGraph.h"

namespace js {
namespace jit {

class EffectiveAddressAnalysis
{
    MIRGraph &graph_;

  public:
    EffectiveAddressAnalysis(MIRGraph &graph)
      : graph_(graph)
    {}

    bool analyze();
};

} 
} 

#endif 
