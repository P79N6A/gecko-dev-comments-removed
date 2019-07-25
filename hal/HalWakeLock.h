




#ifndef __HAL_WAKELOCK_H_
#define __HAL_WAKELOCK_H_

namespace mozilla {
namespace hal {

enum WakeLockState {
  WAKE_LOCK_STATE_UNLOCKED,
  WAKE_LOCK_STATE_HIDDEN,
  WAKE_LOCK_STATE_VISIBLE
};




WakeLockState ComputeWakeLockState(int aNumLocks, int aNumHidden);

} 
} 

#endif 
