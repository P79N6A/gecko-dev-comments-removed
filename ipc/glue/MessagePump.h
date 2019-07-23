



































#ifndef __IPC_GLUE_MESSAGEPUMP_H__
#define __IPC_GLUE_MESSAGEPUMP_H__

#include "base/message_pump_default.h"

#include "prtypes.h"
#include "nsCOMPtr.h"

class nsIRunnable;
class nsIThread;

namespace mozilla {
namespace ipc {

class MessagePump : public base::MessagePumpDefault
{
public:
  MessagePump();
  ~MessagePump();

  virtual void Run(base::MessagePump::Delegate* aDelegate);
  virtual void ScheduleWork();

private:
  nsIRunnable* mDummyEvent;

  
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
