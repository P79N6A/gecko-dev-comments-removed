









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

#include <pthread.h>

namespace webrtc {

class CriticalSectionPosix : public CriticalSectionWrapper {
 public:
  CriticalSectionPosix();

  virtual ~CriticalSectionPosix();

  virtual void Enter() OVERRIDE;
  virtual void Leave() OVERRIDE;

 private:
  pthread_mutex_t mutex_;
  friend class ConditionVariablePosix;
};

}  

#endif  
