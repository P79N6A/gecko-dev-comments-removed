





#ifndef ion_x86_BaselineCompiler_x86_h
#define ion_x86_BaselineCompiler_x86_h

#include "ion/shared/BaselineCompiler-x86-shared.h"

namespace js {
namespace ion {

class BaselineCompilerX86 : public BaselineCompilerX86Shared
{
  protected:
    BaselineCompilerX86(JSContext *cx, HandleScript script);
};

typedef BaselineCompilerX86 BaselineCompilerSpecific;

} 
} 

#endif 
