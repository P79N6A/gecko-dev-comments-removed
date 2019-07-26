






#ifndef jsion_effective_address_analysis_h__
#define jsion_effective_address_analysis_h__

#include "MIR.h"
#include "MIRGraph.h"

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
