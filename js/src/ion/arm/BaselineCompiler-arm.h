






#ifndef jsion_baselinecompiler_arm_h__
#define jsion_baselinecompiler_arm_h__

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
