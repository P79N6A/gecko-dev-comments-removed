





#ifndef mozilla_ProcessHangMonitorIPC_h
#define mozilla_ProcessHangMonitorIPC_h

#include "base/task.h"
#include "base/thread.h"

#include "mozilla/PProcessHangMonitor.h"
#include "mozilla/PProcessHangMonitorParent.h"
#include "mozilla/PProcessHangMonitorChild.h"

namespace mozilla {

namespace dom {
class ContentParent;
} 

PProcessHangMonitorParent*
CreateHangMonitorParent(mozilla::dom::ContentParent* aContentParent,
                        mozilla::ipc::Transport* aTransport,
                        base::ProcessId aOtherProcess);

PProcessHangMonitorChild*
CreateHangMonitorChild(mozilla::ipc::Transport* aTransport,
                       base::ProcessId aOtherProcess);

} 

#endif 
