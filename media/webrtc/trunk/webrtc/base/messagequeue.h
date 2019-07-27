









#ifndef WEBRTC_BASE_MESSAGEQUEUE_H_
#define WEBRTC_BASE_MESSAGEQUEUE_H_

#include <string.h>

#include <algorithm>
#include <list>
#include <queue>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/socketserver.h"
#include "webrtc/base/timeutils.h"

namespace rtc {

struct Message;
class MessageQueue;



class MessageQueueManager {
 public:
  static void Add(MessageQueue *message_queue);
  static void Remove(MessageQueue *message_queue);
  static void Clear(MessageHandler *handler);

  
  
  
  
  static bool IsInitialized();

 private:
  static MessageQueueManager* Instance();

  MessageQueueManager();
  ~MessageQueueManager();

  void AddInternal(MessageQueue *message_queue);
  void RemoveInternal(MessageQueue *message_queue);
  void ClearInternal(MessageHandler *handler);

  static MessageQueueManager* instance_;
  
  std::vector<MessageQueue *> message_queues_;
  CriticalSection crit_;
};




class MessageData {
 public:
  MessageData() {}
  virtual ~MessageData() {}
};

template <class T>
class TypedMessageData : public MessageData {
 public:
  explicit TypedMessageData(const T& data) : data_(data) { }
  const T& data() const { return data_; }
  T& data() { return data_; }
 private:
  T data_;
};


template <class T>
class ScopedMessageData : public MessageData {
 public:
  explicit ScopedMessageData(T* data) : data_(data) { }
  const scoped_ptr<T>& data() const { return data_; }
  scoped_ptr<T>& data() { return data_; }
 private:
  scoped_ptr<T> data_;
};


template <class T>
class ScopedRefMessageData : public MessageData {
 public:
  explicit ScopedRefMessageData(T* data) : data_(data) { }
  const scoped_refptr<T>& data() const { return data_; }
  scoped_refptr<T>& data() { return data_; }
 private:
  scoped_refptr<T> data_;
};

template<class T>
inline MessageData* WrapMessageData(const T& data) {
  return new TypedMessageData<T>(data);
}

template<class T>
inline const T& UseMessageData(MessageData* data) {
  return static_cast< TypedMessageData<T>* >(data)->data();
}

template<class T>
class DisposeData : public MessageData {
 public:
  explicit DisposeData(T* data) : data_(data) { }
  virtual ~DisposeData() { delete data_; }
 private:
  T* data_;
};

const uint32 MQID_ANY = static_cast<uint32>(-1);
const uint32 MQID_DISPOSE = static_cast<uint32>(-2);



struct Message {
  Message() {
    memset(this, 0, sizeof(*this));
  }
  inline bool Match(MessageHandler* handler, uint32 id) const {
    return (handler == NULL || handler == phandler)
           && (id == MQID_ANY || id == message_id);
  }
  MessageHandler *phandler;
  uint32 message_id;
  MessageData *pdata;
  uint32 ts_sensitive;
};

typedef std::list<Message> MessageList;




class DelayedMessage {
 public:
  DelayedMessage(int delay, uint32 trigger, uint32 num, const Message& msg)
  : cmsDelay_(delay), msTrigger_(trigger), num_(num), msg_(msg) { }

  bool operator< (const DelayedMessage& dmsg) const {
    return (dmsg.msTrigger_ < msTrigger_)
           || ((dmsg.msTrigger_ == msTrigger_) && (dmsg.num_ < num_));
  }

  int cmsDelay_;  
  uint32 msTrigger_;
  uint32 num_;
  Message msg_;
};

class MessageQueue {
 public:
  explicit MessageQueue(SocketServer* ss = NULL);
  virtual ~MessageQueue();

  SocketServer* socketserver() { return ss_; }
  void set_socketserver(SocketServer* ss);

  
  
  
  
  
  
  virtual void Quit();
  virtual bool IsQuitting();
  virtual void Restart();

  
  
  
  
  virtual bool Get(Message *pmsg, int cmsWait = kForever,
                   bool process_io = true);
  virtual bool Peek(Message *pmsg, int cmsWait = 0);
  virtual void Post(MessageHandler *phandler, uint32 id = 0,
                    MessageData *pdata = NULL, bool time_sensitive = false);
  virtual void PostDelayed(int cmsDelay, MessageHandler *phandler,
                           uint32 id = 0, MessageData *pdata = NULL) {
    return DoDelayPost(cmsDelay, TimeAfter(cmsDelay), phandler, id, pdata);
  }
  virtual void PostAt(uint32 tstamp, MessageHandler *phandler,
                      uint32 id = 0, MessageData *pdata = NULL) {
    return DoDelayPost(TimeUntil(tstamp), tstamp, phandler, id, pdata);
  }
  virtual void Clear(MessageHandler *phandler, uint32 id = MQID_ANY,
                     MessageList* removed = NULL);
  virtual void Dispatch(Message *pmsg);
  virtual void ReceiveSends();

  
  virtual int GetDelay();

  bool empty() const { return size() == 0u; }
  size_t size() const {
    CritScope cs(&crit_);  
    return msgq_.size() + dmsgq_.size() + (fPeekKeep_ ? 1u : 0u);
  }

  
  template<class T> void Dispose(T* doomed) {
    if (doomed) {
      Post(NULL, MQID_DISPOSE, new DisposeData<T>(doomed));
    }
  }

  
  
  sigslot::signal0<> SignalQueueDestroyed;

 protected:
  class PriorityQueue : public std::priority_queue<DelayedMessage> {
   public:
    container_type& container() { return c; }
    void reheap() { make_heap(c.begin(), c.end(), comp); }
  };

  void DoDelayPost(int cmsDelay, uint32 tstamp, MessageHandler *phandler,
                   uint32 id, MessageData* pdata);

  
  SocketServer* ss_;
  
  scoped_ptr<SocketServer> default_ss_;
  bool fStop_;
  bool fPeekKeep_;
  Message msgPeek_;
  MessageList msgq_;
  PriorityQueue dmsgq_;
  uint32 dmsgq_next_num_;
  mutable CriticalSection crit_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageQueue);
};

}  

#endif  
