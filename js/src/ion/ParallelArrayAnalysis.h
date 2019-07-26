





#ifndef ion_ParallelArrayAnalysis_h
#define ion_ParallelArrayAnalysis_h

#include "MIR.h"
#include "CompileInfo.h"

namespace js {

class StackFrame;

namespace ion {

class MIRGraph;
class AutoDestroyAllocator;




class ParallelArrayAnalysis
{
    MIRGenerator *mir_;
    MIRGraph &graph_;

    bool removeResumePointOperands();
    void replaceOperandsOnResumePoint(MResumePoint *resumePoint, MDefinition *withDef);

  public:
    ParallelArrayAnalysis(MIRGenerator *mir,
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
