








































#ifndef jsion_ion_analysis_h__
#define jsion_ion_analysis_h__

#include "IonAllocPolicy.h"

namespace js {
namespace ion {

class MIRGenerator;
class MIRGraph;

bool
ApplyTypeInformation(MIRGraph &graph);

bool
RenumberInstructions(MIRGraph &graph);

} 
} 

#endif 

