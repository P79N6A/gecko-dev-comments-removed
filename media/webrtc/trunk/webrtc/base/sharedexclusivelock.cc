









#include "webrtc/base/sharedexclusivelock.h"

namespace rtc {

SharedExclusiveLock::SharedExclusiveLock()
    : shared_count_is_zero_(true, true),
      shared_count_(0) {
}

void SharedExclusiveLock::LockExclusive() {
  cs_exclusive_.Enter();
  shared_count_is_zero_.Wait(rtc::kForever);
}

void SharedExclusiveLock::UnlockExclusive() {
  cs_exclusive_.Leave();
}

void SharedExclusiveLock::LockShared() {
  CritScope exclusive_scope(&cs_exclusive_);
  CritScope shared_scope(&cs_shared_);
  if (++shared_count_ == 1) {
    shared_count_is_zero_.Reset();
  }
}

void SharedExclusiveLock::UnlockShared() {
  CritScope shared_scope(&cs_shared_);
  if (--shared_count_ == 0) {
    shared_count_is_zero_.Set();
  }
}

}  
