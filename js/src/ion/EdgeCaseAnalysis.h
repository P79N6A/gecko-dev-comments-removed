





#ifndef ion_EdgeCaseAnalysis_h
#define ion_EdgeCaseAnalysis_h

#include "ion/MIRGenerator.h"

namespace js {
namespace ion {

class MIRGraph;

class EdgeCaseAnalysis
{
    MIRGenerator *mir;
    MIRGraph &graph;

  public:
    EdgeCaseAnalysis(MIRGenerator *mir, MIRGraph &graph);
    bool analyzeLate();
};


} 
} 

#endif 
