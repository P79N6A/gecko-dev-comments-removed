









#ifndef WEBRTC_BASE_SOCKETFACTORY_H__
#define WEBRTC_BASE_SOCKETFACTORY_H__

#include "webrtc/base/socket.h"
#include "webrtc/base/asyncsocket.h"

namespace rtc {

class SocketFactory {
public:
  virtual ~SocketFactory() {}

  
  
  
  
  
  virtual Socket* CreateSocket(int type) = 0;
  virtual Socket* CreateSocket(int family, int type) = 0;
  
  
  virtual AsyncSocket* CreateAsyncSocket(int type) = 0;
  virtual AsyncSocket* CreateAsyncSocket(int family, int type) = 0;
};

} 

#endif 
