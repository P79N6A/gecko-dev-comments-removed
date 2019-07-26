





#ifndef ion_arm_BaselineCompiler_arm_h
#define ion_arm_BaselineCompiler_arm_h

#include "ion/shared/BaselineCompiler-shared.h"

namespace js {
namespace ion {

class BaselineCompilerARM : public BaselineCompilerShared
{
  protected:
    BaselineCompilerARM(JSContext *cx, HandleScript script);
};

typedef BaselineCompilerARM BaselineCompilerSpecific;

} 
} 

#endif 
