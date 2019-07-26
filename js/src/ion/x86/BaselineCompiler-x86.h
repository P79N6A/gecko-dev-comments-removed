






#ifndef jsion_baselinecompiler_x86_h__
#define jsion_baselinecompiler_x86_h__

#include "ion/shared/BaselineCompiler-x86-shared.h"

namespace js {
namespace ion {

class BaselineCompilerX86 : public BaselineCompilerX86Shared
{
  protected:
    BaselineCompilerX86(JSContext *cx, JSScript *script);
};

typedef BaselineCompilerX86 BaselineCompilerSpecific;

} 
} 

#endif 
