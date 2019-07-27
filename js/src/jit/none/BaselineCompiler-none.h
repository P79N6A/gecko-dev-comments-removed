





#ifndef jit_none_BaselineCompiler_none_h
#define jit_none_BaselineCompiler_none_h

#include "jit/shared/BaselineCompiler-shared.h"

namespace js {
namespace jit {

class BaselineCompilerNone : public BaselineCompilerShared
{
  protected:
    BaselineCompilerNone(JSContext *cx, TempAllocator &alloc, JSScript *script)
      : BaselineCompilerShared(cx, alloc, script)
    {
        MOZ_CRASH();
    }
};

typedef BaselineCompilerNone BaselineCompilerSpecific;

} 
} 

#endif 
