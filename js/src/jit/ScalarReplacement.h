






#ifndef jit_ScalarReplacement_h
#define jit_ScalarReplacement_h

namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

bool
ScalarReplacement(MIRGenerator* mir, MIRGraph& graph, bool* success);

} 
} 

#endif 
