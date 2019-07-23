



#ifndef CHROME_COMMON_IPC_SYNC_SENDER_H__
#define CHROME_COMMON_IPC_SYNC_SENDER_H__

#include <string>
#include <deque>
#include "base/basictypes.h"
#include "base/lock.h"
#include "base/ref_counted.h"
#include "base/scoped_handle.h"
#include "base/waitable_event.h"
#include "base/waitable_event_watcher.h"
#include "chrome/common/ipc_channel_proxy.h"

namespace IPC {

class SyncMessage;
class MessageReplyDeserializer;







class SyncChannel : public ChannelProxy,
                    public base::WaitableEventWatcher::Delegate {
 public:
  SyncChannel(const std::wstring& channel_id, Channel::Mode mode,
              Channel::Listener* listener, MessageFilter* filter,
              MessageLoop* ipc_message_loop, bool create_pipe_now,
              base::WaitableEvent* shutdown_event);
  ~SyncChannel();

  virtual bool Send(Message* message);
  virtual bool SendWithTimeout(Message* message, int timeout_ms);

  
  void set_sync_messages_with_no_timeout_allowed(bool value) {
    sync_messages_with_no_timeout_allowed_ = value;
  }

 protected:
  class ReceivedSyncMsgQueue;
  friend class ReceivedSyncMsgQueue;

  
  
  
  class SyncContext : public Context,
                      public base::WaitableEventWatcher::Delegate {
   public:
    SyncContext(Channel::Listener* listener,
                MessageFilter* filter,
                MessageLoop* ipc_thread,
                base::WaitableEvent* shutdown_event);

    ~SyncContext();

    
    
    void Push(IPC::SyncMessage* sync_msg);

    
    
    bool Pop();

    
    
    base::WaitableEvent* GetSendDoneEvent();

    
    
    base::WaitableEvent* GetDispatchEvent();

    void DispatchMessages();

    
    
    
    bool TryToUnblockListener(const Message* msg);

    
    
    void OnSendTimeout(int message_id);

    base::WaitableEvent* shutdown_event() { return shutdown_event_; }

   private:
    

    
   virtual void Clear();

    
    virtual void OnMessageReceived(const Message& msg);
    virtual void OnChannelError();
    virtual void OnChannelOpened();
    virtual void OnChannelClosed();

    
    void CancelPendingSends();

    
    virtual void OnWaitableEventSignaled(base::WaitableEvent* arg);

    
    
    struct PendingSyncMsg {
      PendingSyncMsg(int id, IPC::MessageReplyDeserializer* d,
                     base::WaitableEvent* e) :
          id(id), deserializer(d), done_event(e), send_result(false) { }
      int id;
      IPC::MessageReplyDeserializer* deserializer;
      base::WaitableEvent* done_event;
      bool send_result;
    };

    typedef std::deque<PendingSyncMsg> PendingSyncMessageQueue;
    PendingSyncMessageQueue deserializers_;
    Lock deserializers_lock_;

    scoped_refptr<ReceivedSyncMsgQueue> received_sync_msgs_;

    base::WaitableEvent* shutdown_event_;
    base::WaitableEventWatcher shutdown_watcher_;
  };

 private:
  
  virtual void OnWaitableEventSignaled(base::WaitableEvent* arg);

  SyncContext* sync_context() {
    return reinterpret_cast<SyncContext*>(context());
  }

  
  
  void WaitForReply(base::WaitableEvent* pump_messages_event);

  
  
  void WaitForReplyWithNestedMessageLoop();

  bool sync_messages_with_no_timeout_allowed_;

  
  base::WaitableEventWatcher send_done_watcher_;
  base::WaitableEventWatcher dispatch_watcher_;

  DISALLOW_EVIL_CONSTRUCTORS(SyncChannel);
};

}  

#endif  
