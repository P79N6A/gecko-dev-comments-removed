






#ifndef jsion_parallel_array_analysis_h__
#define jsion_parallel_array_analysis_h__

#include "MIR.h"
#include "CompileInfo.h"

namespace js {

class StackFrame;

namespace ion {

class MIRGraph;
class AutoDestroyAllocator;

class ParallelCompileContext
{
  private:
    JSContext *cx_;

    
    AutoObjectVector worklist_;

    
    bool analyzeAndGrowWorklist(MIRGenerator *mir, MIRGraph &graph);

    bool removeResumePointOperands(MIRGenerator *mir, MIRGraph &graph);
    void replaceOperandsOnResumePoint(MResumePoint *resumePoint, MDefinition *withDef);

  public:
    ParallelCompileContext(JSContext *cx)
      : cx_(cx),
        worklist_(cx)
    { }

    
    bool appendToWorklist(HandleFunction fun);

    ExecutionMode executionMode() {
        return ParallelExecution;
    }

    
    MethodStatus checkScriptSize(JSContext *cx, RawScript script);
    MethodStatus compileTransitively();
    AbortReason compile(IonBuilder *builder, MIRGraph *graph,
                        ScopedJSDeletePtr<LifoAlloc> &autoDelete);
};


} 
} 

#endif 
