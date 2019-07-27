









#ifndef WEBRTC_BASE_SOCKETSERVER_H_
#define WEBRTC_BASE_SOCKETSERVER_H_

#include "webrtc/base/socketfactory.h"

namespace rtc {

class MessageQueue;






class SocketServer : public SocketFactory {
 public:
  
  
  
  virtual void SetMessageQueue(MessageQueue* queue) {}

  
  
  
  
  virtual bool Wait(int cms, bool process_io) = 0;

  
  virtual void WakeUp() = 0;
};

}  

#endif  
