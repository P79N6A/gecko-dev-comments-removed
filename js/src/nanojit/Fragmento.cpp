








































#include "nanojit.h"
#undef MEMORY_INFO

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

    using namespace avmplus;

    
    
    
    Fragment::Fragment(const void* _ip
                       verbose_only(, uint32_t profFragID))
        :
          root(NULL),
          lirbuf(NULL),
          lastIns(NULL),
          ip(_ip),
          recordAttempts(0),
          fragEntry(NULL),
          vmprivate(NULL),
          verbose_only( loopLabel(NULL), )
          verbose_only( profFragID(profFragID), )
          verbose_only( profCount(0), )
          verbose_only( nStaticExits(0), )
          verbose_only( nCodeBytes(0), )
          verbose_only( nExitBytes(0), )
          verbose_only( guardNumberer(1), )
          verbose_only( guardsForFrag(NULL), )
          _code(NULL),
          _hits(0)
    {
        
        
        
        
    }
    #endif 
}


