






#ifdef DEBUG

#ifndef jsion_c1spewer_h__
#define jsion_c1spewer_h__

#include "jsscript.h"

#include "js/RootingAPI.h"

namespace js {
namespace ion {

class MDefinition;
class MInstruction;
class MBasicBlock;
class MIRGraph;
class LinearScanAllocator;
class LInstruction;

class C1Spewer
{
    MIRGraph *graph;
    HandleScript script;
    FILE *spewout_;

  public:
    C1Spewer()
      : graph(NULL), script(NullPtr()), spewout_(NULL)
    { }

    bool init(const char *path);
    void beginFunction(MIRGraph *graph, HandleScript script);
    void spewPass(const char *pass);
    void spewIntervals(const char *pass, LinearScanAllocator *regalloc);
    void endFunction();
    void finish();

  private:
    void spewPass(FILE *fp, MBasicBlock *block);
    void spewIntervals(FILE *fp, LinearScanAllocator *regalloc, LInstruction *ins, size_t &nextId);
    void spewIntervals(FILE *fp, MBasicBlock *block, LinearScanAllocator *regalloc, size_t &nextId);
};

} 
} 

#endif 

#endif 

