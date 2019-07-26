





#ifndef ion_x64_BaselineCompiler_x64_h
#define ion_x64_BaselineCompiler_x64_h

#include "ion/shared/BaselineCompiler-x86-shared.h"

namespace js {
namespace ion {

class BaselineCompilerX64 : public BaselineCompilerX86Shared
{
  protected:
    BaselineCompilerX64(JSContext *cx, HandleScript script);
};

typedef BaselineCompilerX64 BaselineCompilerSpecific;

} 
} 

#endif 
