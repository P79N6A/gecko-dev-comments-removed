









#ifndef WEBRTC_BASE_MESSAGEHANDLER_H_
#define WEBRTC_BASE_MESSAGEHANDLER_H_

#include "webrtc/base/constructormagic.h"

namespace rtc {

struct Message;



class MessageHandler {
 public:
  virtual ~MessageHandler();
  virtual void OnMessage(Message* msg) = 0;

 protected:
  MessageHandler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageHandler);
};


template <class ReturnT, class FunctorT>
class FunctorMessageHandler : public MessageHandler {
 public:
  explicit FunctorMessageHandler(const FunctorT& functor)
      : functor_(functor) {}
  virtual void OnMessage(Message* msg) {
    result_ = functor_();
  }
  const ReturnT& result() const { return result_; }

 private:
  FunctorT functor_;
  ReturnT result_;
};


template <class FunctorT>
class FunctorMessageHandler<void, FunctorT> : public MessageHandler {
 public:
  explicit FunctorMessageHandler(const FunctorT& functor)
      : functor_(functor) {}
  virtual void OnMessage(Message* msg) {
    functor_();
  }
  void result() const {}

 private:
  FunctorT functor_;
};


} 

#endif 
