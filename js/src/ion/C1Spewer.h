








































#ifdef DEBUG

#ifndef jsion_c1spewer_h__
#define jsion_c1spewer_h__

#include "jscntxt.h"
#include "MIR.h"
#include "LinearScan.h"

namespace js {
namespace ion {

class C1Spewer
{
    MIRGraph *graph;
    JSScript *script;
    FILE *spewout_;

  public:
    C1Spewer()
      : graph(NULL), script(NULL), spewout_(NULL)
    { }

    bool init(const char *path);
    void beginFunction(MIRGraph *graph, JSScript *script);
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

