



































#ifndef __IPC_GLUE_MESSAGEPUMP_H__
#define __IPC_GLUE_MESSAGEPUMP_H__

#include "base/message_pump_default.h"

namespace mozilla {
namespace ipc {

class MessagePump : public base::MessagePumpDefault
{
public:
  friend class UIThreadObserver;

  virtual void Run(base::MessagePump::Delegate* aDelegate);
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
