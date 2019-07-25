















#include "critical_section_posix.h"

namespace webrtc {

CriticalSectionPosix::CriticalSectionPosix()
{
    pthread_mutexattr_t attr;
    (void) pthread_mutexattr_init(&attr);
    (void) pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    (void) pthread_mutex_init(&_mutex, &attr);
}

CriticalSectionPosix::~CriticalSectionPosix()
{
    (void) pthread_mutex_destroy(&_mutex);
}

void
CriticalSectionPosix::Enter()
{
    (void) pthread_mutex_lock(&_mutex);
}

void
CriticalSectionPosix::Leave()
{
    (void) pthread_mutex_unlock(&_mutex);
}

} 
