



#ifndef CHROME_COMMON_CHILD_THREAD_H_
#define CHROME_COMMON_CHILD_THREAD_H_

#include "base/thread.h"
#include "chrome/common/ipc_sync_channel.h"
#include "chrome/common/message_router.h"

class ResourceDispatcher;


class ChildThread : public IPC::Channel::Listener,
                    public IPC::Message::Sender,
                    public base::Thread {
 public:
  
  explicit ChildThread(Thread::Options options);
  virtual ~ChildThread();

  
  virtual bool Send(IPC::Message* msg);

  
  void AddRoute(int32_t routing_id, IPC::Channel::Listener* listener);
  void RemoveRoute(int32_t routing_id);

  MessageLoop* owner_loop() { return owner_loop_; }

 protected:
  friend class ChildProcess;

  
  bool Run();

  
  void SetChannelName(const std::wstring& name) { channel_name_ = name; }

  
  void OnProcessFinalRelease();

 protected:
  
  static const size_t kV8StackSize;

  virtual void OnControlMessageReceived(const IPC::Message& msg) { }

  
  static ChildThread* current();

  IPC::Channel* channel() { return channel_.get(); }

  
  virtual void Init();
  virtual void CleanUp();

 private:
  
  virtual void OnMessageReceived(const IPC::Message& msg);
  virtual void OnChannelError();

#ifdef MOZ_NUWA_PROCESS
  static void MarkThread();
#endif

  
  MessageLoop* owner_loop_;

  std::wstring channel_name_;
  scoped_ptr<IPC::Channel> channel_;

  
  
  MessageRouter router_;

  Thread::Options options_;

  
  
  
  bool check_with_browser_before_shutdown_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildThread);
};

#endif  
