






#ifndef jit_Sink_h
#define jit_Sink_h

namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

bool
Sink(MIRGenerator* mir, MIRGraph& graph);

} 
} 

#endif 
