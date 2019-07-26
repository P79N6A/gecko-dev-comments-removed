



#ifndef __IPC_GLUE_MESSAGEPUMP_H__
#define __IPC_GLUE_MESSAGEPUMP_H__

#include "base/message_pump_default.h"
#include "base/time.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

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

} 
} 

#endif 
