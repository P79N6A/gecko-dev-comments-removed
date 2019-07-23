



#include "chrome/common/ipc_sync_channel.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/thread_local.h"
#include "base/message_loop.h"
#include "base/waitable_event.h"
#include "base/waitable_event_watcher.h"
#include "chrome/common/ipc_sync_message.h"

using base::TimeDelta;
using base::TimeTicks;
using base::WaitableEvent;

namespace IPC {

















class SyncChannel::ReceivedSyncMsgQueue :
    public base::RefCountedThreadSafe<ReceivedSyncMsgQueue> {
 public:
  
  
  static ReceivedSyncMsgQueue* AddContext() {
    
    
    ReceivedSyncMsgQueue* rv = lazy_tls_ptr_.Pointer()->Get();
    if (!rv) {
      rv = new ReceivedSyncMsgQueue();
      ReceivedSyncMsgQueue::lazy_tls_ptr_.Pointer()->Set(rv);
    }
    rv->listener_count_++;
    return rv;
  }

  ~ReceivedSyncMsgQueue() {
  }

  
  void QueueMessage(const Message& msg, SyncChannel::SyncContext* context) {
    bool was_task_pending;
    {
      AutoLock auto_lock(message_lock_);

      was_task_pending = task_pending_;
      task_pending_ = true;

      
      
      message_queue_.push_back(QueuedMessage(new Message(msg), context));
    }

    dispatch_event_.Signal();
    if (!was_task_pending) {
      listener_message_loop_->PostTask(FROM_HERE, NewRunnableMethod(
          this, &ReceivedSyncMsgQueue::DispatchMessagesTask));
    }
  }

  void QueueReply(const Message &msg, SyncChannel::SyncContext* context) {
    received_replies_.push_back(QueuedMessage(new Message(msg), context));
  }

  
  
  void DispatchMessagesTask() {
    {
      AutoLock auto_lock(message_lock_);
      task_pending_ = false;
    }
    DispatchMessages();
  }

  void DispatchMessages() {
    while (true) {
      Message* message;
      scoped_refptr<SyncChannel::SyncContext> context;
      {
        AutoLock auto_lock(message_lock_);
        if (message_queue_.empty())
          break;

        message = message_queue_.front().message;
        context = message_queue_.front().context;
        message_queue_.pop_front();
      }

      context->OnDispatchMessage(*message);
      delete message;
    }
  }

  
  void RemoveContext(SyncContext* context) {
    AutoLock auto_lock(message_lock_);

    SyncMessageQueue::iterator iter = message_queue_.begin();
    while (iter != message_queue_.end()) {
      if (iter->context == context) {
        delete iter->message;
        iter = message_queue_.erase(iter);
      } else {
        iter++;
      }
    }

    if (--listener_count_ == 0) {
      DCHECK(lazy_tls_ptr_.Pointer()->Get());
      lazy_tls_ptr_.Pointer()->Set(NULL);
    }
  }

  WaitableEvent* dispatch_event() { return &dispatch_event_; }
  MessageLoop* listener_message_loop() { return listener_message_loop_; }

  
  static base::LazyInstance<base::ThreadLocalPointer<ReceivedSyncMsgQueue> >
      lazy_tls_ptr_;

  
  
  void DispatchReplies() {
    for (size_t i = 0; i < received_replies_.size(); ++i) {
      Message* message = received_replies_[i].message;
      if (received_replies_[i].context->TryToUnblockListener(message)) {
        delete message;
        received_replies_.erase(received_replies_.begin() + i);
        return;
      }
    }
  }

 private:
  
  
  ReceivedSyncMsgQueue() :
      dispatch_event_(true, false),
      listener_message_loop_(MessageLoop::current()),
      task_pending_(false),
      listener_count_(0) {
  }

  
  struct QueuedMessage {
    QueuedMessage(Message* m, SyncContext* c) : message(m), context(c) { }
    Message* message;
    scoped_refptr<SyncChannel::SyncContext> context;
  };

  typedef std::deque<QueuedMessage> SyncMessageQueue;
  SyncMessageQueue message_queue_;

  std::vector<QueuedMessage> received_replies_;

  
  
  
  WaitableEvent dispatch_event_;
  MessageLoop* listener_message_loop_;
  Lock message_lock_;
  bool task_pending_;
  int listener_count_;
};

base::LazyInstance<base::ThreadLocalPointer<SyncChannel::ReceivedSyncMsgQueue> >
    SyncChannel::ReceivedSyncMsgQueue::lazy_tls_ptr_(base::LINKER_INITIALIZED);

SyncChannel::SyncContext::SyncContext(
    Channel::Listener* listener,
    MessageFilter* filter,
    MessageLoop* ipc_thread,
    WaitableEvent* shutdown_event)
    : ChannelProxy::Context(listener, filter, ipc_thread),
      received_sync_msgs_(ReceivedSyncMsgQueue::AddContext()),
      shutdown_event_(shutdown_event) {
}

SyncChannel::SyncContext::~SyncContext() {
  while (!deserializers_.empty())
    Pop();
}




void SyncChannel::SyncContext::Push(SyncMessage* sync_msg) {
  
  
  
  
  PendingSyncMsg pending(SyncMessage::GetMessageId(*sync_msg),
                         sync_msg->GetReplyDeserializer(),
                         new WaitableEvent(true, false));
  AutoLock auto_lock(deserializers_lock_);
  deserializers_.push_back(pending);
}

bool SyncChannel::SyncContext::Pop() {
  bool result;
  {
    AutoLock auto_lock(deserializers_lock_);
    PendingSyncMsg msg = deserializers_.back();
    delete msg.deserializer;
    delete msg.done_event;
    msg.done_event = NULL;
    deserializers_.pop_back();
    result = msg.send_result;
  }

  
  
  
  
  
  ipc_message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
      received_sync_msgs_.get(), &ReceivedSyncMsgQueue::DispatchReplies));

  return result;
}

WaitableEvent* SyncChannel::SyncContext::GetSendDoneEvent() {
  AutoLock auto_lock(deserializers_lock_);
  return deserializers_.back().done_event;
}

WaitableEvent* SyncChannel::SyncContext::GetDispatchEvent() {
  return received_sync_msgs_->dispatch_event();
}

void SyncChannel::SyncContext::DispatchMessages() {
  received_sync_msgs_->DispatchMessages();
}

bool SyncChannel::SyncContext::TryToUnblockListener(const Message* msg) {
  AutoLock auto_lock(deserializers_lock_);
  if (deserializers_.empty() ||
      !SyncMessage::IsMessageReplyTo(*msg, deserializers_.back().id)) {
    return false;
  }

  if (!msg->is_reply_error()) {
    deserializers_.back().send_result = deserializers_.back().deserializer->
        SerializeOutputParameters(*msg);
  }
  deserializers_.back().done_event->Signal();

  return true;
}

void SyncChannel::SyncContext::Clear() {
  CancelPendingSends();
  received_sync_msgs_->RemoveContext(this);

  Context::Clear();
}

void SyncChannel::SyncContext::OnMessageReceived(const Message& msg) {
  
  if (TryFilters(msg))
    return;

  if (TryToUnblockListener(&msg))
    return;

  if (msg.should_unblock()) {
    received_sync_msgs_->QueueMessage(msg, this);
    return;
  }

  if (msg.is_reply()) {
    received_sync_msgs_->QueueReply(msg, this);
    return;
  }

  return Context::OnMessageReceivedNoFilter(msg);
}

void SyncChannel::SyncContext::OnChannelError() {
  CancelPendingSends();
  shutdown_watcher_.StopWatching();
  Context::OnChannelError();
}

void SyncChannel::SyncContext::OnChannelOpened() {
  shutdown_watcher_.StartWatching(shutdown_event_, this);
  Context::OnChannelOpened();
}

void SyncChannel::SyncContext::OnChannelClosed() {
  shutdown_watcher_.StopWatching();
  Context::OnChannelClosed();
}

void SyncChannel::SyncContext::OnSendTimeout(int message_id) {
  AutoLock auto_lock(deserializers_lock_);
  PendingSyncMessageQueue::iterator iter;
  for (iter = deserializers_.begin(); iter != deserializers_.end(); iter++) {
    if (iter->id == message_id) {
      iter->done_event->Signal();
      break;
    }
  }
}

void SyncChannel::SyncContext::CancelPendingSends() {
  AutoLock auto_lock(deserializers_lock_);
  PendingSyncMessageQueue::iterator iter;
  for (iter = deserializers_.begin(); iter != deserializers_.end(); iter++)
    iter->done_event->Signal();
}

void SyncChannel::SyncContext::OnWaitableEventSignaled(WaitableEvent* event) {
  DCHECK(event == shutdown_event_);
  
  
  CancelPendingSends();
}


SyncChannel::SyncChannel(
    const std::wstring& channel_id, Channel::Mode mode,
    Channel::Listener* listener, MessageFilter* filter,
    MessageLoop* ipc_message_loop, bool create_pipe_now,
    WaitableEvent* shutdown_event)
    : ChannelProxy(
          channel_id, mode, ipc_message_loop,
          new SyncContext(listener, filter, ipc_message_loop, shutdown_event),
          create_pipe_now),
      sync_messages_with_no_timeout_allowed_(true) {
  
  
  
  
  
  
  dispatch_watcher_.StartWatching(sync_context()->GetDispatchEvent(), this);
}

SyncChannel::~SyncChannel() {
}

bool SyncChannel::Send(Message* message) {
  return SendWithTimeout(message, base::kNoTimeout);
}

bool SyncChannel::SendWithTimeout(Message* message, int timeout_ms) {
  if (!message->is_sync()) {
    ChannelProxy::Send(message);
    return true;
  }

  
  scoped_refptr<SyncContext> context(sync_context());
  if (context->shutdown_event()->IsSignaled()) {
    delete message;
    return false;
  }

  DCHECK(sync_messages_with_no_timeout_allowed_ ||
         timeout_ms != base::kNoTimeout);
  SyncMessage* sync_msg = static_cast<SyncMessage*>(message);
  context->Push(sync_msg);
  int message_id = SyncMessage::GetMessageId(*sync_msg);
  WaitableEvent* pump_messages_event = sync_msg->pump_messages_event();

  ChannelProxy::Send(message);

  if (timeout_ms != base::kNoTimeout) {
    
    
    
    context->ipc_message_loop()->PostDelayedTask(FROM_HERE,
        NewRunnableMethod(context.get(),
            &SyncContext::OnSendTimeout, message_id), timeout_ms);
  }

  
  WaitForReply(pump_messages_event);

  return context->Pop();
}

void SyncChannel::WaitForReply(WaitableEvent* pump_messages_event) {
  while (true) {
    WaitableEvent* objects[] = {
      sync_context()->GetDispatchEvent(),
      sync_context()->GetSendDoneEvent(),
      pump_messages_event
    };

    unsigned count = pump_messages_event ? 3: 2;
    unsigned result = WaitableEvent::WaitMany(objects, count);
    if (result == 0 ) {
      
      
      sync_context()->GetDispatchEvent()->Reset();
      sync_context()->DispatchMessages();
      continue;
    }

    if (result == 2 )
      WaitForReplyWithNestedMessageLoop();  

    break;
  }
}

void SyncChannel::WaitForReplyWithNestedMessageLoop() {
  WaitableEvent* old_done_event = send_done_watcher_.GetWatchedEvent();
  send_done_watcher_.StopWatching();
  send_done_watcher_.StartWatching(sync_context()->GetSendDoneEvent(), this);
  bool old_state = MessageLoop::current()->NestableTasksAllowed();
  MessageLoop::current()->SetNestableTasksAllowed(true);
  MessageLoop::current()->Run();
  MessageLoop::current()->SetNestableTasksAllowed(old_state);
  if (old_done_event)
    send_done_watcher_.StartWatching(old_done_event, this);
}

void SyncChannel::OnWaitableEventSignaled(WaitableEvent* event) {
  WaitableEvent* dispatch_event = sync_context()->GetDispatchEvent();
  if (event == dispatch_event) {
    
    
    dispatch_event->Reset();
    dispatch_watcher_.StartWatching(dispatch_event, this);
    sync_context()->DispatchMessages();
  } else {
    
    DCHECK(event == sync_context()->GetSendDoneEvent());
    MessageLoop::current()->Quit();
  }
}

}  
