






#ifndef jit_EagerSimdUnbox_h
#define jit_EagerSimdUnbox_h

namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

bool
EagerSimdUnbox(MIRGenerator *mir, MIRGraph &graph);

} 
} 

#endif 
