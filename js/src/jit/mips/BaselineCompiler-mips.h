





#ifndef jit_mips_BaselineCompiler_mips_h
#define jit_mips_BaselineCompiler_mips_h

#include "jit/shared/BaselineCompiler-shared.h"

namespace js {
namespace jit {

class BaselineCompilerMIPS : public BaselineCompilerShared
{
  protected:
    BaselineCompilerMIPS(JSContext *cx, TempAllocator &alloc, JSScript *script);
};

typedef BaselineCompilerMIPS BaselineCompilerSpecific;

} 
} 

#endif 
