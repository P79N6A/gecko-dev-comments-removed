





#ifndef jit_x86_BaselineCompiler_x86_h
#define jit_x86_BaselineCompiler_x86_h

#include "jit/shared/BaselineCompiler-x86-shared.h"

namespace js {
namespace jit {

class BaselineCompilerX86 : public BaselineCompilerX86Shared
{
  protected:
    BaselineCompilerX86(JSContext *cx, TempAllocator &alloc, HandleScript script);
};

typedef BaselineCompilerX86 BaselineCompilerSpecific;

} 
} 

#endif 
