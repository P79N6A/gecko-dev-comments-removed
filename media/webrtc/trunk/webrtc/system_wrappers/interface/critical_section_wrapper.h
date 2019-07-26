









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CRITICAL_SECTION_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CRITICAL_SECTION_WRAPPER_H_




#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/thread_annotations.h"

namespace webrtc {
class LOCKABLE CriticalSectionWrapper {
 public:
  
  static CriticalSectionWrapper* CreateCriticalSection();

  virtual ~CriticalSectionWrapper() {}

  
  
  virtual void Enter() EXCLUSIVE_LOCK_FUNCTION() = 0;

  
  virtual void Leave() UNLOCK_FUNCTION() = 0;
};



class SCOPED_LOCKABLE CriticalSectionScoped {
 public:
  explicit CriticalSectionScoped(CriticalSectionWrapper* critsec)
      EXCLUSIVE_LOCK_FUNCTION(critsec)
      : ptr_crit_sec_(critsec) {
    ptr_crit_sec_->Enter();
  }

  ~CriticalSectionScoped() UNLOCK_FUNCTION() { ptr_crit_sec_->Leave(); }

 private:
  CriticalSectionWrapper* ptr_crit_sec_;
};

}  

#endif  
