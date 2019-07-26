






#ifndef jsion_baselinecompiler_x86_shared_h__
#define jsion_baselinecompiler_x86_shared_h__

#include "ion/shared/BaselineCompiler-shared.h"

namespace js {
namespace ion {

class BaselineCompilerX86Shared : public BaselineCompilerShared
{
  protected:
    BaselineCompilerX86Shared(JSContext *cx, JSScript *script);
};

} 
} 

#endif 
