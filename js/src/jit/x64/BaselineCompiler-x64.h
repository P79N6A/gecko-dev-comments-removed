





#ifndef jit_x64_BaselineCompiler_x64_h
#define jit_x64_BaselineCompiler_x64_h

#include "jit/shared/BaselineCompiler-x86-shared.h"

namespace js {
namespace jit {

class BaselineCompilerX64 : public BaselineCompilerX86Shared
{
  protected:
    BaselineCompilerX64(JSContext *cx, HandleScript script);
};

typedef BaselineCompilerX64 BaselineCompilerSpecific;

} 
} 

#endif 
