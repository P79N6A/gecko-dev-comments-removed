





#ifndef jit_EdgeCaseAnalysis_h
#define jit_EdgeCaseAnalysis_h

#include "jit/MIRGenerator.h"

namespace js {
namespace jit {

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
