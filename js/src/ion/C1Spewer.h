








































#ifndef jsion_ion_spew_h__
#define jsion_ion_spew_h__

#include "jscntxt.h"
#include "MIR.h"
#include "LinearScan.h"

namespace js {
namespace ion {

class C1Spewer
{
    MIRGraph &graph;
    JSScript *script;
    FILE *spewout_;

  public:
    C1Spewer(MIRGraph &graph, JSScript *script);
    ~C1Spewer();
    void enable(const char *path);
    void spewCFG(const char *pass);
    void spewIntervals(const char *pass, RegisterAllocator *regalloc);

  private:
    void spewCFG(FILE *fp, MBasicBlock *block);
    void spewIntervals(FILE *fp, MBasicBlock *block, RegisterAllocator *regalloc, size_t &nextId);
};

} 
} 

#endif 

