



#ifndef CHROME_COMMON_IPC_LOGGING_H_
#define CHROME_COMMON_IPC_LOGGING_H_

#include "chrome/common/ipc_message.h"  

#ifdef IPC_MESSAGE_LOG_ENABLED

#include "base/lock.h"
#include "base/message_loop.h"
#include "base/singleton.h"
#include "base/waitable_event_watcher.h"
#include "chrome/common/ipc_message_utils.h"

class MessageLoop;

namespace IPC {

class Message;




class Logging : public base::WaitableEventWatcher::Delegate,
                public MessageLoop::DestructionObserver {
 public:
  
  class Consumer {
   public:
    virtual void Log(const LogData& data) = 0;
  };

  void SetConsumer(Consumer* consumer);

  ~Logging();
  static Logging* current();

  void Enable();
  void Disable();
  bool Enabled() const { return enabled_; }

  
  
  void SetIPCSender(Message::Sender* sender);

  
  
  void OnReceivedLoggingMessage(const Message& message);

  void OnSendMessage(Message* message, const std::wstring& channel_id);
  void OnPreDispatchMessage(const Message& message);
  void OnPostDispatchMessage(const Message& message,
                             const std::wstring& channel_id);

  
  
  
  static std::wstring GetEventName(bool enabled);

  
  
  
  static void GetMessageText(uint16 type, std::wstring* name,
                             const Message* message, std::wstring* params);

  
  void OnWaitableEventSignaled(base::WaitableEvent* event);

  
  void WillDestroyCurrentMessageLoop();

  typedef void (*LogFunction)(uint16 type,
                             std::wstring* name,
                             const Message* msg,
                             std::wstring* params);

  static void SetLoggerFunctions(LogFunction *functions);

 private:
  friend struct DefaultSingletonTraits<Logging>;
  Logging();

  std::wstring GetEventName(int browser_pid, bool enabled);
  void OnSendLogs();
  void Log(const LogData& data);

  void RegisterWaitForEvent(bool enabled);

  base::WaitableEventWatcher watcher_;

  scoped_ptr<base::WaitableEvent> logging_event_on_;
  scoped_ptr<base::WaitableEvent> logging_event_off_;
  bool enabled_;

  std::vector<LogData> queued_logs_;
  bool queue_invoke_later_pending_;

  Message::Sender* sender_;
  MessageLoop* main_thread_;

  Consumer* consumer_;

  static LogFunction *log_function_mapping_;
};

}  

#endif 

#endif  
