



#ifndef __IPC_GLUE_MESSAGEPUMP_H__
#define __IPC_GLUE_MESSAGEPUMP_H__

#include "base/message_pump_default.h"
#if defined(XP_WIN)
#include "base/message_pump_win.h"
#endif

#include "base/time.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIThreadInternal.h"

class nsIThread;
class nsITimer;

namespace mozilla {
namespace ipc {

class DoWorkRunnable;

class MessagePump : public base::MessagePumpDefault
{
  friend class DoWorkRunnable;

public:
  MessagePump();

  
  virtual void
  Run(base::MessagePump::Delegate* aDelegate) MOZ_OVERRIDE;

  
  virtual void
  ScheduleWork() MOZ_OVERRIDE;

  
  virtual void
  ScheduleWorkForNestedLoop() MOZ_OVERRIDE;

  
  virtual void
  ScheduleDelayedWork(const base::TimeTicks& aDelayedWorkTime) MOZ_OVERRIDE;

protected:
  virtual ~MessagePump();

private:
  
  void DoDelayedWork(base::MessagePump::Delegate* aDelegate);

protected:
  
  
  nsCOMPtr<nsITimer> mDelayedWorkTimer;
  nsIThread* mThread;

private:
  
  nsRefPtr<DoWorkRunnable> mDoWorkEvent;
};

class MessagePumpForChildProcess MOZ_FINAL: public MessagePump
{
public:
  MessagePumpForChildProcess()
  : mFirstRun(true)
  { }

  virtual void Run(base::MessagePump::Delegate* aDelegate) MOZ_OVERRIDE;

private:
  ~MessagePumpForChildProcess()
  { }

  bool mFirstRun;
};

class MessagePumpForNonMainThreads MOZ_FINAL : public MessagePump
{
public:
  MessagePumpForNonMainThreads()
  { }

  virtual void Run(base::MessagePump::Delegate* aDelegate) MOZ_OVERRIDE;

private:
  ~MessagePumpForNonMainThreads()
  { }
};

#if defined(XP_WIN)


class MessagePumpForNonMainUIThreads MOZ_FINAL:
  public base::MessagePumpForUI,
  public nsIThreadObserver
{
public:
  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) {
    return 2;
  }
  NS_IMETHOD_(MozExternalRefCountType) Release(void) {
    return 1;
  }
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_NSITHREADOBSERVER

public:
  MessagePumpForNonMainUIThreads() :
    mThread(nullptr),
    mInWait(false),
    mWaitLock("mInWait")
  {
  }

  
  virtual void DoRunLoop();

protected:
  nsIThread* mThread;

  void SetInWait() {
    MutexAutoLock lock(mWaitLock);
    mInWait = true;
  }

  void ClearInWait() {
    MutexAutoLock lock(mWaitLock);
    mInWait = false;
  }

  bool GetInWait() {
    MutexAutoLock lock(mWaitLock);
    return mInWait;
  }

private:
  ~MessagePumpForNonMainUIThreads()
  {
  }

  bool mInWait;
  mozilla::Mutex mWaitLock;
};
#endif 

} 
} 

#endif 
