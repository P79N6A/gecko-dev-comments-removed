





#ifndef jit_arm64_BaselineCompiler_arm64_h
#define jit_arm64_BaselineCompiler_arm64_h

#include "jit/shared/BaselineCompiler-shared.h"

namespace js {
namespace jit {

class BaselineCompilerARM64 : public BaselineCompilerShared
{
  protected:
    BaselineCompilerARM64(JSContext* cx, TempAllocator& alloc, JSScript* script)
      : BaselineCompilerShared(cx, alloc, script)
    { }
};

typedef BaselineCompilerARM64 BaselineCompilerSpecific;

} 
} 

#endif 
