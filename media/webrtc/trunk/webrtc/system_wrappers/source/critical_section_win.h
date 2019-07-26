









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_WIN_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_WIN_H_

#include <windows.h>
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWindows : public CriticalSectionWrapper {
 public:
  CriticalSectionWindows();

  virtual ~CriticalSectionWindows();

  virtual void Enter();
  virtual void Leave();

 private:
  CRITICAL_SECTION crit;

  friend class ConditionVariableEventWin;
  friend class ConditionVariableNativeWin;
};

} 

#endif  
