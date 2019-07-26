





#ifndef jit_LICM_h
#define jit_LICM_h



namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

bool LICM(MIRGenerator *mir, MIRGraph &graph);

} 
} 

#endif 
