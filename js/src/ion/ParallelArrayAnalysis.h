





#ifndef jsion_parallel_array_analysis_h__
#define jsion_parallel_array_analysis_h__

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
