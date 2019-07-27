





#ifndef mozilla_ProcessHangMonitor_h
#define mozilla_ProcessHangMonitor_h

#include "base/task.h"
#include "base/thread.h"

#include "mozilla/Atomics.h"
#include "mozilla/PProcessHangMonitor.h"
#include "mozilla/PProcessHangMonitorParent.h"
#include "mozilla/PProcessHangMonitorChild.h"
#include "nsIObserver.h"

class nsGlobalWindow;
class nsITabChild;

namespace mozilla {

namespace dom {
class ContentParent;
}

class PProcessHangMonitorParent;
class PProcessHangMonitorChild;

class ProcessHangMonitor MOZ_FINAL
  : public nsIObserver
{
 private:
  ProcessHangMonitor();
  virtual ~ProcessHangMonitor();

 public:
  static ProcessHangMonitor* Get() { return sInstance; }
  static ProcessHangMonitor* GetOrCreate();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static void AddProcess(dom::ContentParent* aContentParent);
  static void RemoveProcess(PProcessHangMonitorParent* aParent);

  static void ClearHang();

  PProcessHangMonitorParent* CreateParent(mozilla::dom::ContentParent* aContentParent,
                                          mozilla::ipc::Transport* aTransport,
                                          base::ProcessId aOtherProcess);
  PProcessHangMonitorChild* CreateChild(mozilla::ipc::Transport* aTransport,
                                        base::ProcessId aOtherProcess);

  enum SlowScriptAction {
    Continue,
    Terminate,
    StartDebugger
  };
  SlowScriptAction NotifySlowScript(nsITabChild* aTabChild,
                                    const char* aFileName,
                                    unsigned aLineNo);

  void NotifyPluginHang(uint32_t aPluginId);

  bool IsDebuggerStartupComplete();

  void InitiateCPOWTimeout();
  bool ShouldTimeOutCPOWs();

  MessageLoop* MonitorLoop() { return mThread->message_loop(); }

 private:
  static ProcessHangMonitor* sInstance;

  Atomic<bool> mCPOWTimeout;

  base::Thread* mThread;
};

} 

#endif 
