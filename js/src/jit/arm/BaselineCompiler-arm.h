





#ifndef jit_arm_BaselineCompiler_arm_h
#define jit_arm_BaselineCompiler_arm_h

#include "jit/shared/BaselineCompiler-shared.h"

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
