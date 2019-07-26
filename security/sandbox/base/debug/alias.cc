



#include "base/debug/alias.h"
#include "build/build_config.h"

namespace base {
namespace debug {

#if defined(COMPILER_MSVC)
#pragma optimize("", off)
#endif

void Alias(const void* var) {
}

#if defined(COMPILER_MSVC)
#pragma optimize("", on)
#endif

}  
}  
