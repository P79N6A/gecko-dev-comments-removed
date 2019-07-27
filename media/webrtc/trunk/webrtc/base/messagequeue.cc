









#if defined(WEBRTC_POSIX)
#include <sys/time.h>
#endif

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/messagequeue.h"
#if defined(__native_client__)
#include "webrtc/base/nullsocketserver.h"
typedef rtc::NullSocketServer DefaultSocketServer;
#else
#include "webrtc/base/physicalsocketserver.h"
typedef rtc::PhysicalSocketServer DefaultSocketServer;
#endif

namespace rtc {

const uint32 kMaxMsgLatency = 150;  




MessageQueueManager* MessageQueueManager::instance_ = NULL;

MessageQueueManager* MessageQueueManager::Instance() {
  
  
  if (!instance_)
    instance_ = new MessageQueueManager;
  return instance_;
}

bool MessageQueueManager::IsInitialized() {
  return instance_ != NULL;
}

MessageQueueManager::MessageQueueManager() {
}

MessageQueueManager::~MessageQueueManager() {
}

void MessageQueueManager::Add(MessageQueue *message_queue) {
  return Instance()->AddInternal(message_queue);
}
void MessageQueueManager::AddInternal(MessageQueue *message_queue) {
  
  
  
  ASSERT(!crit_.CurrentThreadIsOwner());
  CritScope cs(&crit_);
  message_queues_.push_back(message_queue);
}

void MessageQueueManager::Remove(MessageQueue *message_queue) {
  
  
  if (!instance_) return;
  return Instance()->RemoveInternal(message_queue);
}
void MessageQueueManager::RemoveInternal(MessageQueue *message_queue) {
  ASSERT(!crit_.CurrentThreadIsOwner());  
  
  
  
  
  bool destroy = false;
  {
    CritScope cs(&crit_);
    std::vector<MessageQueue *>::iterator iter;
    iter = std::find(message_queues_.begin(), message_queues_.end(),
                     message_queue);
    if (iter != message_queues_.end()) {
      message_queues_.erase(iter);
    }
    destroy = message_queues_.empty();
  }
  if (destroy) {
    instance_ = NULL;
    delete this;
  }
}

void MessageQueueManager::Clear(MessageHandler *handler) {
  
  
  if (!instance_) return;
  return Instance()->ClearInternal(handler);
}
void MessageQueueManager::ClearInternal(MessageHandler *handler) {
  ASSERT(!crit_.CurrentThreadIsOwner());  
  CritScope cs(&crit_);
  std::vector<MessageQueue *>::iterator iter;
  for (iter = message_queues_.begin(); iter != message_queues_.end(); iter++)
    (*iter)->Clear(handler);
}




MessageQueue::MessageQueue(SocketServer* ss)
    : ss_(ss), fStop_(false), fPeekKeep_(false),
      dmsgq_next_num_(0) {
  if (!ss_) {
    
    
    
    
    
    default_ss_.reset(new DefaultSocketServer());
    ss_ = default_ss_.get();
  }
  ss_->SetMessageQueue(this);
  MessageQueueManager::Add(this);
}

MessageQueue::~MessageQueue() {
  
  
  
  SignalQueueDestroyed();
  MessageQueueManager::Remove(this);
  Clear(NULL);
  if (ss_) {
    ss_->SetMessageQueue(NULL);
  }
}

void MessageQueue::set_socketserver(SocketServer* ss) {
  ss_ = ss ? ss : default_ss_.get();
  ss_->SetMessageQueue(this);
}

void MessageQueue::Quit() {
  fStop_ = true;
  ss_->WakeUp();
}

bool MessageQueue::IsQuitting() {
  return fStop_;
}

void MessageQueue::Restart() {
  fStop_ = false;
}

bool MessageQueue::Peek(Message *pmsg, int cmsWait) {
  if (fPeekKeep_) {
    *pmsg = msgPeek_;
    return true;
  }
  if (!Get(pmsg, cmsWait))
    return false;
  msgPeek_ = *pmsg;
  fPeekKeep_ = true;
  return true;
}

bool MessageQueue::Get(Message *pmsg, int cmsWait, bool process_io) {
  
  

  if (fPeekKeep_) {
    *pmsg = msgPeek_;
    fPeekKeep_ = false;
    return true;
  }

  

  int cmsTotal = cmsWait;
  int cmsElapsed = 0;
  uint32 msStart = Time();
  uint32 msCurrent = msStart;
  while (true) {
    
    ReceiveSends();

    
    int cmsDelayNext = kForever;
    bool first_pass = true;
    while (true) {
      
      
      
      {
        CritScope cs(&crit_);
        
        
        if (first_pass) {
          first_pass = false;
          while (!dmsgq_.empty()) {
            if (TimeIsLater(msCurrent, dmsgq_.top().msTrigger_)) {
              cmsDelayNext = TimeDiff(dmsgq_.top().msTrigger_, msCurrent);
              break;
            }
            msgq_.push_back(dmsgq_.top().msg_);
            dmsgq_.pop();
          }
        }
        
        if (msgq_.empty()) {
          break;
        } else {
          *pmsg = msgq_.front();
          msgq_.pop_front();
        }
      }  

      
      if (pmsg->ts_sensitive) {
        int32 delay = TimeDiff(msCurrent, pmsg->ts_sensitive);
        if (delay > 0) {
          LOG_F(LS_WARNING) << "id: " << pmsg->message_id << "  delay: "
                            << (delay + kMaxMsgLatency) << "ms";
        }
      }
      
      if (MQID_DISPOSE == pmsg->message_id) {
        ASSERT(NULL == pmsg->phandler);
        delete pmsg->pdata;
        *pmsg = Message();
        continue;
      }
      return true;
    }

    if (fStop_)
      break;

    

    int cmsNext;
    if (cmsWait == kForever) {
      cmsNext = cmsDelayNext;
    } else {
      cmsNext = _max(0, cmsTotal - cmsElapsed);
      if ((cmsDelayNext != kForever) && (cmsDelayNext < cmsNext))
        cmsNext = cmsDelayNext;
    }

    
    if (!ss_->Wait(cmsNext, process_io))
      return false;

    

    msCurrent = Time();
    cmsElapsed = TimeDiff(msCurrent, msStart);
    if (cmsWait != kForever) {
      if (cmsElapsed >= cmsWait)
        return false;
    }
  }
  return false;
}

void MessageQueue::ReceiveSends() {
}

void MessageQueue::Post(MessageHandler *phandler, uint32 id,
    MessageData *pdata, bool time_sensitive) {
  if (fStop_)
    return;

  
  
  

  CritScope cs(&crit_);
  Message msg;
  msg.phandler = phandler;
  msg.message_id = id;
  msg.pdata = pdata;
  if (time_sensitive) {
    msg.ts_sensitive = Time() + kMaxMsgLatency;
  }
  msgq_.push_back(msg);
  ss_->WakeUp();
}

void MessageQueue::DoDelayPost(int cmsDelay, uint32 tstamp,
    MessageHandler *phandler, uint32 id, MessageData* pdata) {
  if (fStop_)
    return;

  
  
  

  CritScope cs(&crit_);
  Message msg;
  msg.phandler = phandler;
  msg.message_id = id;
  msg.pdata = pdata;
  DelayedMessage dmsg(cmsDelay, tstamp, dmsgq_next_num_, msg);
  dmsgq_.push(dmsg);
  
  
  
  VERIFY(0 != ++dmsgq_next_num_);
  ss_->WakeUp();
}

int MessageQueue::GetDelay() {
  CritScope cs(&crit_);

  if (!msgq_.empty())
    return 0;

  if (!dmsgq_.empty()) {
    int delay = TimeUntil(dmsgq_.top().msTrigger_);
    if (delay < 0)
      delay = 0;
    return delay;
  }

  return kForever;
}

void MessageQueue::Clear(MessageHandler *phandler, uint32 id,
                         MessageList* removed) {
  CritScope cs(&crit_);

  

  if (fPeekKeep_ && msgPeek_.Match(phandler, id)) {
    if (removed) {
      removed->push_back(msgPeek_);
    } else {
      delete msgPeek_.pdata;
    }
    fPeekKeep_ = false;
  }

  

  for (MessageList::iterator it = msgq_.begin(); it != msgq_.end();) {
    if (it->Match(phandler, id)) {
      if (removed) {
        removed->push_back(*it);
      } else {
        delete it->pdata;
      }
      it = msgq_.erase(it);
    } else {
      ++it;
    }
  }

  

  PriorityQueue::container_type::iterator new_end = dmsgq_.container().begin();
  for (PriorityQueue::container_type::iterator it = new_end;
       it != dmsgq_.container().end(); ++it) {
    if (it->msg_.Match(phandler, id)) {
      if (removed) {
        removed->push_back(it->msg_);
      } else {
        delete it->msg_.pdata;
      }
    } else {
      *new_end++ = *it;
    }
  }
  dmsgq_.container().erase(new_end, dmsgq_.container().end());
  dmsgq_.reheap();
}

void MessageQueue::Dispatch(Message *pmsg) {
  pmsg->phandler->OnMessage(pmsg);
}

}  
