









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CRITICAL_SECTION_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CRITICAL_SECTION_WRAPPER_H_




#include "webrtc/common_types.h"

namespace webrtc {
class CriticalSectionWrapper {
 public:
  
  static CriticalSectionWrapper* CreateCriticalSection();

  virtual ~CriticalSectionWrapper() {}

  
  
  virtual void Enter() = 0;

  
  virtual void Leave() = 0;
};



class CriticalSectionScoped {
 public:
  explicit CriticalSectionScoped(CriticalSectionWrapper* critsec)
    : ptr_crit_sec_(critsec) {
    ptr_crit_sec_->Enter();
  }

  ~CriticalSectionScoped() {
    if (ptr_crit_sec_) {
      Leave();
    }
  }

 private:
  void Leave() {
    ptr_crit_sec_->Leave();
    ptr_crit_sec_ = 0;
  }

  CriticalSectionWrapper* ptr_crit_sec_;
};

} 

#endif  
