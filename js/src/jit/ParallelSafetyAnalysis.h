





#ifndef jit_ParallelSafetyAnalysis_h
#define jit_ParallelSafetyAnalysis_h

#include "jit/MIR.h"

namespace js {

class InterpreterFrame;

namespace jit {

class MIRGraph;
class AutoDestroyAllocator;




class ParallelSafetyAnalysis
{
    MIRGenerator *mir_;
    MIRGraph &graph_;

  public:
    ParallelSafetyAnalysis(MIRGenerator *mir,
                           MIRGraph &graph)
      : mir_(mir),
        graph_(graph)
    {}

    bool analyze();
};







typedef Vector<JSScript *, 4, JitAllocPolicy> CallTargetVector;
bool AddPossibleCallees(JSContext *cx, MIRGraph &graph, CallTargetVector &targets);

} 
} 

#endif 
