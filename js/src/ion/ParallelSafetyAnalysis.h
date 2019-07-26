





#ifndef ion_ParallelSafetyAnalysis_h
#define ion_ParallelSafetyAnalysis_h

#include "ion/MIR.h"
#include "ion/CompileInfo.h"

namespace js {

class StackFrame;

namespace ion {

class MIRGraph;
class AutoDestroyAllocator;




class ParallelSafetyAnalysis
{
    MIRGenerator *mir_;
    MIRGraph &graph_;

    bool removeResumePointOperands();
    void replaceOperandsOnResumePoint(MResumePoint *resumePoint, MDefinition *withDef);

  public:
    ParallelSafetyAnalysis(MIRGenerator *mir,
                           MIRGraph &graph)
      : mir_(mir),
        graph_(graph)
    {}

    bool analyze();
};







typedef Vector<JSScript *, 4, IonAllocPolicy> CallTargetVector;
bool AddPossibleCallees(MIRGraph &graph, CallTargetVector &targets);

} 
} 

#endif 
