





#ifndef jit_LoopUnroller_h
#define jit_LoopUnroller_h

#include "jit/RangeAnalysis.h"

namespace js {
namespace jit {

bool
UnrollLoops(MIRGraph &graph, const LoopIterationBoundVector &bounds);

} 
} 

#endif 
