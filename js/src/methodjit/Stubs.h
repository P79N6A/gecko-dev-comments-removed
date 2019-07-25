







































#ifndef jslogic_h__
#define jslogic_h__

#include "MethodJIT.h"

namespace js {
namespace mjit {
namespace stubs {

void * JS_FASTCALL Return(VMFrame &f);

}}} 

extern "C" void *
js_InternalThrow(js::VMFrame &f);

#endif 

