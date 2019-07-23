



#ifndef CHROME_COMMON_CHILD_PROCESS_HOST_H_
#define CHROME_COMMON_CHILD_PROCESS_HOST_H_

#include "build/build_config.h"

#include <list>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/waitable_event_watcher.h"
#ifdef CHROMIUM_MOZILLA_BUILD
class ResourceDispatcherHost;
#else
#include "chrome/browser/renderer_host/resource_dispatcher_host.h"
#endif
#include "chrome/common/child_process_info.h"
#include "chrome/common/ipc_channel.h"

class NotificationType;



class ChildProcessHost :
#ifdef CHROMIUM_MOZILLA_BUILD
                         public IPC::Message::Sender,
                         public ChildProcessInfo,
#else
                         public ResourceDispatcherHost::Receiver,
#endif
                         public base::WaitableEventWatcher::Delegate,
                         public IPC::Channel::Listener {
 public:
  virtual ~ChildProcessHost();

  
  virtual bool Send(IPC::Message* msg);

  
  
  
  
  
  class Iterator {
   public:
    Iterator();
    Iterator(ProcessType type);
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
  ChildProcessHost(ProcessType type,
                   ResourceDispatcherHost* resource_dispatcher_host = 0);

  
  virtual bool CanShutdown() = 0;

  
  bool CreateChannel();

  
  
  void SetHandle(base::ProcessHandle handle);

  
  void InstanceCreated();

  
  virtual void OnMessageReceived(const IPC::Message& msg) { }
  virtual void OnChannelConnected(int32 peer_pid) { }
  virtual void OnChannelError() { }

  bool opening_channel() { return opening_channel_; }
  const std::wstring& channel_id() { return channel_id_; }

#ifdef CHROMIUM_MOZILLA_BUILD
  base::WaitableEvent* GetProcessEvent() { return process_event_.get(); }
#endif

  const IPC::Channel& channel() const { return *channel_; }
#ifdef CHROMIUM_MOZILLA_BUILD
  IPC::Channel* channelp() const { return channel_.get(); }
#endif

 private:
  
  void Notify(NotificationType type);

#ifdef CHROMIUM_MOZILLA_BUILD
 protected:
#endif
  
  virtual void OnWaitableEventSignaled(base::WaitableEvent *event);
#ifdef CHROMIUM_MOZILLA_BUILD
 private:
#endif

  
  
  
  class ListenerHook : public IPC::Channel::Listener {
   public:
    ListenerHook(ChildProcessHost* host);
    virtual void OnMessageReceived(const IPC::Message& msg);
    virtual void OnChannelConnected(int32 peer_pid);
    virtual void OnChannelError();
   private:
    ChildProcessHost* host_;
  };

  ListenerHook listener_;

  ResourceDispatcherHost* resource_dispatcher_host_;

  
  bool opening_channel_;

  
  scoped_ptr<IPC::Channel> channel_;

  
  std::wstring channel_id_;

  
  base::WaitableEventWatcher watcher_;

  scoped_ptr<base::WaitableEvent> process_event_;
};

#endif  
