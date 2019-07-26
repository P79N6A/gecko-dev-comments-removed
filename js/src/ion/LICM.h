





#ifndef ion_LICM_h
#define ion_LICM_h

#include "ion/IonAllocPolicy.h"
#include "ion/IonAnalysis.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"



namespace js {
namespace ion {

class LICM
{
    MIRGenerator *mir;
    MIRGraph &graph;

  public:
    LICM(MIRGenerator *mir, MIRGraph &graph);
    bool analyze();
};

} 
} 

#endif 
