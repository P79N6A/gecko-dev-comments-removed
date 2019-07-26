





#ifndef mozilla_ProcessPriorityManager_h_
#define mozilla_ProcessPriorityManager_h_

namespace mozilla {
namespace dom {
namespace ipc {














void InitProcessPriorityManager();






bool CurrentProcessIsForeground();











void TemporarilyLockProcessPriority();

} 
} 
} 

#endif
