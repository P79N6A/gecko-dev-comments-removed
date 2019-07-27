





#ifndef mozilla_ProcessHangMonitor_h
#define mozilla_ProcessHangMonitor_h

#include "mozilla/Atomics.h"
#include "nsIObserver.h"

class nsITabChild;

class MessageLoop;

namespace base {
class Thread;
};

namespace mozilla {

namespace dom {
class ContentParent;
}

class PProcessHangMonitorParent;

class ProcessHangMonitor final
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

  MessageLoop* MonitorLoop();

 private:
  static ProcessHangMonitor* sInstance;

  Atomic<bool> mCPOWTimeout;

  base::Thread* mThread;
};

} 

#endif 
