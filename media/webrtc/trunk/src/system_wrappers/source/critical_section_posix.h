









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_

#include "critical_section_wrapper.h"

#include <pthread.h>

namespace webrtc {
class CriticalSectionPosix : public CriticalSectionWrapper
{
public:
    CriticalSectionPosix();

    virtual ~CriticalSectionPosix();

    virtual void Enter();
    virtual void Leave();

private:
    pthread_mutex_t _mutex;
    friend class ConditionVariablePosix;
};
} 

#endif 
