









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_GENERIC_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_RW_LOCK_GENERIC_H_

#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class ConditionVariableWrapper;

class RWLockGeneric : public RWLockWrapper {
 public:
  RWLockGeneric();
  virtual ~RWLockGeneric();

  virtual void AcquireLockExclusive() OVERRIDE;
  virtual void ReleaseLockExclusive() OVERRIDE;

  virtual void AcquireLockShared() OVERRIDE;
  virtual void ReleaseLockShared() OVERRIDE;

 private:
  CriticalSectionWrapper* critical_section_;
  ConditionVariableWrapper* read_condition_;
  ConditionVariableWrapper* write_condition_;

  int readers_active_;
  bool writer_active_;
  int readers_waiting_;
  int writers_waiting_;
};

}  

#endif  
