



#ifndef CHROME_COMMON_CHILD_THREAD_H_
#define CHROME_COMMON_CHILD_THREAD_H_

#include "base/thread.h"
#include "chrome/common/ipc_sync_channel.h"
#include "chrome/common/message_router.h"

#ifdef CHROMIUM_MOZILLA_BUILD
class ResourceDispatcher;
#else
#include "chrome/common/resource_dispatcher.h"
#endif


class ChildThread : public IPC::Channel::Listener,
                    public IPC::Message::Sender,
                    public base::Thread {
 public:
  
  ChildThread(Thread::Options options);
  virtual ~ChildThread();

  
  virtual bool Send(IPC::Message* msg);

  
  void AddRoute(int32 routing_id, IPC::Channel::Listener* listener);
  void RemoveRoute(int32 routing_id);

  MessageLoop* owner_loop() { return owner_loop_; }

#ifndef CHROMIUM_MOZILLA_BUILD
  ResourceDispatcher* resource_dispatcher() {
    return resource_dispatcher_.get();
  }
#endif

 protected:
  friend class ChildProcess;

  
  bool Run();

  
  void SetChannelName(const std::wstring& name) { channel_name_ = name; }

  
  void OnProcessFinalRelease();

 protected:
  
  static const size_t kV8StackSize;

  virtual void OnControlMessageReceived(const IPC::Message& msg) { }

  
  static ChildThread* current();

#ifndef CHROMIUM_MOZILLA_BUILD
  IPC::SyncChannel* channel() { return channel_.get(); }
#else
  IPC::Channel* channel() { return channel_.get(); }
#endif

  
  virtual void Init();
  virtual void CleanUp();

 private:
  
  virtual void OnMessageReceived(const IPC::Message& msg);
  virtual void OnChannelError();

  
  MessageLoop* owner_loop_;

  std::wstring channel_name_;
#ifndef CHROMIUM_MOZILLA_BUILD
  scoped_ptr<IPC::SyncChannel> channel_;
#else
  scoped_ptr<IPC::Channel> channel_;
#endif

  
  
  MessageRouter router_;

  Thread::Options options_;

#ifndef CHROMIUM_MOZILLA_BUILD
  
  
  scoped_ptr<ResourceDispatcher> resource_dispatcher_;
#endif

  
  
  
  bool check_with_browser_before_shutdown_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildThread);
};

#endif  
