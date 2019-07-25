









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CONDITION_VARIABLE_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CONDITION_VARIABLE_POSIX_H_

#include "condition_variable_wrapper.h"

#include <pthread.h>

namespace webrtc {
class ConditionVariablePosix : public ConditionVariableWrapper
{
public:
    static ConditionVariableWrapper* Create();
    ~ConditionVariablePosix();

    void SleepCS(CriticalSectionWrapper& critSect);
    bool SleepCS(CriticalSectionWrapper& critSect, unsigned long maxTimeInMS);
    void Wake();
    void WakeAll();

private:
    ConditionVariablePosix();
    int Construct();

private:
    pthread_cond_t _cond;
};
} 

#endif 
