





#ifndef ion_EffectiveAddressAnalysis_h
#define ion_EffectiveAddressAnalysis_h

#include "ion/MIR.h"
#include "ion/MIRGraph.h"

namespace js {
namespace ion {

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
