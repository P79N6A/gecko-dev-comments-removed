









#include "rw_lock_wrapper.h"

#include <assert.h>

#if defined(_WIN32)
    #include "rw_lock_win.h"
#else
    #include "rw_lock_posix.h"
#endif

namespace webrtc {
RWLockWrapper* RWLockWrapper::CreateRWLock()
{
#ifdef _WIN32
    RWLockWrapper* lock =  new RWLockWindows();
#else
    RWLockWrapper* lock =  new RWLockPosix();
#endif
    if(lock->Init() != 0)
    {
        delete lock;
        assert(false);
        return NULL;
    }
    return lock;
}

RWLockWrapper::~RWLockWrapper()
{
}
} 
