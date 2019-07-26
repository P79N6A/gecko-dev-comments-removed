





#ifndef ion_shared_BaselineCompiler_x86_shared_h
#define ion_shared_BaselineCompiler_x86_shared_h

#include "ion/shared/BaselineCompiler-shared.h"

namespace js {
namespace ion {

class BaselineCompilerX86Shared : public BaselineCompilerShared
{
  protected:
    BaselineCompilerX86Shared(JSContext *cx, HandleScript script);
};

} 
} 

#endif 
