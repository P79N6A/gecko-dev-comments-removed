








































#ifndef jsion_ion_spew_h__
#define jsion_ion_spew_h__

#include "jscntxt.h"
#include "MIR.h"

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
    void spew(const char *pass);

  private:
    void spew(FILE *fp, const char *pass);
    void spew(FILE *fp, MBasicBlock *block);
};

} 
} 

#endif 

