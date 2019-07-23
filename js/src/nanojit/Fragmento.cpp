








































#include "nanojit.h"
#undef MEMORY_INFO

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

    using namespace avmplus;

    
    
    
    Fragment::Fragment(const void* _ip)
        :
#ifdef NJ_VERBOSE
          _called(0),
          _native(0),
          _exitNative(0),
          _lir(0),
          _lirbytes(0),
          _token(NULL),
          traceTicks(0),
          interpTicks(0),
          eot_target(NULL),
          sid(0),
          compileNbr(0),
#endif
          treeBranches(NULL),
          branches(NULL),
          nextbranch(NULL),
          anchor(NULL),
          root(NULL),
          parent(NULL),
          first(NULL),
          peer(NULL),
          lirbuf(NULL),
          lastIns(NULL),
          spawnedFrom(NULL),
          kind(LoopTrace),
          ip(_ip),
          guardCount(0),
          xjumpCount(0),
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


