



#ifndef CHROME_COMMON_CHILD_PROCESS_HOST_H_
#define CHROME_COMMON_CHILD_PROCESS_HOST_H_

#include "build/build_config.h"

#include <list>

#include "base/basictypes.h"
#include "base/waitable_event_watcher.h"
#include "chrome/common/child_process_info.h"
#include "chrome/common/ipc_channel.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {
namespace ipc {
class FileDescriptor;
}
}

class NotificationType;



class ChildProcessHost :
                         public IPC::Message::Sender,
                         public ChildProcessInfo,
                         public base::WaitableEventWatcher::Delegate,
                         public IPC::Channel::Listener {
 public:
  virtual ~ChildProcessHost();

  
  virtual bool Send(IPC::Message* msg);

  
  
  
  
  
  class Iterator {
   public:
    Iterator();
    explicit Iterator(ProcessType type);
    ChildProcessHost* operator->() { return *iterator_; }
    ChildProcessHost* operator*() { return *iterator_; }
    ChildProcessHost* operator++();
    bool Done();

   private:
    bool all_;
    ProcessType type_;
    std::list<ChildProcessHost*>::iterator iterator_;
  };

 protected:
  explicit ChildProcessHost(ProcessType type);

  
  virtual bool CanShutdown() = 0;

  
  bool CreateChannel();

  bool CreateChannel(mozilla::ipc::FileDescriptor& aFileDescriptor);

  
  
  void SetHandle(base::ProcessHandle handle);

  
  void InstanceCreated();

  
  virtual void OnMessageReceived(const IPC::Message& msg) { }
  virtual void OnChannelConnected(int32_t peer_pid) { }
  virtual void OnChannelError() { }

  bool opening_channel() { return opening_channel_; }
  const std::wstring& channel_id() { return channel_id_; }

  base::WaitableEvent* GetProcessEvent() { return process_event_.get(); }

  const IPC::Channel& channel() const { return *channel_; }
  IPC::Channel* channelp() const { return channel_.get(); }

 private:
  
  void Notify(NotificationType type);

 protected:
  
  virtual void OnWaitableEventSignaled(base::WaitableEvent *event);

 private:
  
  
  
  class ListenerHook : public IPC::Channel::Listener {
   public:
    explicit ListenerHook(ChildProcessHost* host);
    virtual void OnMessageReceived(const IPC::Message& msg);
    virtual void OnChannelConnected(int32_t peer_pid);
    virtual void OnChannelError();
    virtual void GetQueuedMessages(std::queue<IPC::Message>& queue);
   private:
    ChildProcessHost* host_;
  };

  ListenerHook listener_;

  
  bool opening_channel_;

  
  mozilla::UniquePtr<IPC::Channel> channel_;

  
  std::wstring channel_id_;

  
  base::WaitableEventWatcher watcher_;

  mozilla::UniquePtr<base::WaitableEvent> process_event_;
};

#endif  
