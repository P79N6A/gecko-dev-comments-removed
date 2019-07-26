






#ifndef jsion_baselinecompiler_x64_h__
#define jsion_baselinecompiler_x64_h__

#include "ion/shared/BaselineCompiler-x86-shared.h"

namespace js {
namespace ion {

class BaselineCompilerX64 : public BaselineCompilerX86Shared
{
  protected:
    BaselineCompilerX64(JSContext *cx, JSScript *script);
};

typedef BaselineCompilerX64 BaselineCompilerSpecific;

} 
} 

#endif 
