








































#include "nanojit.h"
#undef MEMORY_INFO

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

    using namespace avmplus;

    
    
    
    Fragment::Fragment(const void* _ip)
        :
          root(NULL),
          lirbuf(NULL),
          lastIns(NULL),
          ip(_ip),
          recordAttempts(0),
          fragEntry(NULL),
          loopEntry(NULL),
          vmprivate(NULL),
          _code(NULL),
          _hits(0)
    {
    }
    #endif 
}


