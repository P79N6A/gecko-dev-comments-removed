





#ifndef jit_LICM_h
#define jit_LICM_h



namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

class LICM
{
    MIRGenerator *mir;
    MIRGraph &graph;

  public:
    LICM(MIRGenerator *mir, MIRGraph &graph);
    bool analyze();
};

} 
} 

#endif 
