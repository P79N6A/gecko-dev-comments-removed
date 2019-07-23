



































#ifndef __IPC_GLUE_MESSAGEPUMP_H__
#define __IPC_GLUE_MESSAGEPUMP_H__

#include "base/basictypes.h"
#include "base/message_pump_default.h"
#include "base/time.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsITimer.h"

namespace mozilla {
namespace ipc {

class MessagePump;

class DoWorkRunnable : public nsIRunnable,
                       public nsITimerCallback
{
public:
  DoWorkRunnable(MessagePump* aPump)
  : mPump(aPump) { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSITIMERCALLBACK

private:
  MessagePump* mPump;
};

class MessagePump : public base::MessagePumpDefault
{

public:
  MessagePump();

  virtual void Run(base::MessagePump::Delegate* aDelegate);
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const base::Time& delayed_work_time);

  void DoDelayedWork(base::MessagePump::Delegate* aDelegate);

private:
  nsRefPtr<DoWorkRunnable> mDoWorkEvent;
  nsCOMPtr<nsITimer> mDelayedWorkTimer;

  
  nsIThread* mThread;
};

class MessagePumpForChildProcess : public MessagePump
{
public:
  MessagePumpForChildProcess()
  : mFirstRun(true)
  { }

  virtual void Run(base::MessagePump::Delegate* aDelegate);

private:
  bool mFirstRun;
};

} 
} 

#endif 
