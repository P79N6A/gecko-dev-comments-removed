





#ifndef jit_x86_shared_BaselineCompiler_x86_shared_h
#define jit_x86_shared_BaselineCompiler_x86_shared_h

#include "jit/shared/BaselineCompiler-shared.h"

namespace js {
namespace jit {

class BaselineCompilerX86Shared : public BaselineCompilerShared
{
  protected:
    BaselineCompilerX86Shared(JSContext* cx, TempAllocator& alloc, JSScript* script);
};

} 
} 

#endif 
