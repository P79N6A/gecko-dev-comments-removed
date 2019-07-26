





#ifndef frontend_SharedContext_inl_h
#define frontend_SharedContext_inl_h

#include "frontend/Parser.h"
#include "frontend/SharedContext.h"

namespace js {
namespace frontend {

inline bool
SharedContext::needStrictChecks()
{
    return strict || extraWarnings;
}

} 
} 

#endif 
