






#ifndef jit_ScalarReplacement_h
#define jit_ScalarReplacement_h

#ifdef JS_ION

namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

bool
ScalarReplacement(MIRGenerator *mir, MIRGraph &graph);

} 
} 

#endif 

#endif 
