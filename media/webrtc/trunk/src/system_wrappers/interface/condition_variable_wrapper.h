









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CONDITION_VARIABLE_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CONDITION_VARIABLE_WRAPPER_H_

namespace webrtc {
class CriticalSectionWrapper;

class ConditionVariableWrapper
{
public:
    
    static ConditionVariableWrapper* CreateConditionVariable();

    virtual ~ConditionVariableWrapper() {}

    
    
    virtual void SleepCS(CriticalSectionWrapper& critSect) = 0;

    
    virtual bool SleepCS(CriticalSectionWrapper& critSect,
                         unsigned long maxTimeInMS) = 0;

    
    virtual void Wake() = 0;

    
    virtual void WakeAll() = 0;
};
} 

#endif 
