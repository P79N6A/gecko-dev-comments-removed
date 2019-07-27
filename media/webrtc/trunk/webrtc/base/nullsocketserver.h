









#ifndef WEBRTC_BASE_NULLSOCKETSERVER_H_
#define WEBRTC_BASE_NULLSOCKETSERVER_H_

#include "webrtc/base/event.h"
#include "webrtc/base/physicalsocketserver.h"

namespace rtc {



class NullSocketServer : public rtc::SocketServer {
 public:
  NullSocketServer() : event_(false, false) {}

  virtual bool Wait(int cms, bool process_io) {
    event_.Wait(cms);
    return true;
  }

  virtual void WakeUp() {
    event_.Set();
  }

  virtual rtc::Socket* CreateSocket(int type) {
    ASSERT(false);
    return NULL;
  }

  virtual rtc::Socket* CreateSocket(int family, int type) {
    ASSERT(false);
    return NULL;
  }

  virtual rtc::AsyncSocket* CreateAsyncSocket(int type) {
    ASSERT(false);
    return NULL;
  }

  virtual rtc::AsyncSocket* CreateAsyncSocket(int family, int type) {
    ASSERT(false);
    return NULL;
  }


 private:
  rtc::Event event_;
};

}  

#endif  
