





#ifndef jit_LICM_h
#define jit_LICM_h

#include "jit/IonAllocPolicy.h"
#include "jit/IonAnalysis.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"



namespace js {
namespace jit {

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
