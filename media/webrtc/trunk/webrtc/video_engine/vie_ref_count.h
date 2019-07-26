











#ifndef WEBRTC_VIDEO_ENGINE_VIE_REF_COUNT_H_
#define WEBRTC_VIDEO_ENGINE_VIE_REF_COUNT_H_

#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;

class ViERefCount {
 public:
  ViERefCount();
  ~ViERefCount();

  ViERefCount& operator++(int);  
  ViERefCount& operator--(int);  

  void Reset();
  int GetCount() const;

 private:
  volatile int count_;
  scoped_ptr<CriticalSectionWrapper> crit_;
};

}  

#endif  
