









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CONDITION_VARIABLE_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CONDITION_VARIABLE_WRAPPER_H_

namespace webrtc {

class CriticalSectionWrapper;

class ConditionVariableWrapper {
 public:
  
  static ConditionVariableWrapper* CreateConditionVariable();

  virtual ~ConditionVariableWrapper() {}

  
  
  virtual void SleepCS(CriticalSectionWrapper& crit_sect) = 0;

  
  virtual bool SleepCS(CriticalSectionWrapper& crit_sect,
                       unsigned long max_time_in_ms) = 0;

  
  virtual void Wake() = 0;

  
  virtual void WakeAll() = 0;
};

} 

#endif  
