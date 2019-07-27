









#ifndef WEBRTC_BASE_ASYNCRESOLVERINTERFACE_H_
#define WEBRTC_BASE_ASYNCRESOLVERINTERFACE_H_

#include "webrtc/base/sigslot.h"
#include "webrtc/base/socketaddress.h"

namespace rtc {


class AsyncResolverInterface {
 public:
  AsyncResolverInterface() {}
  virtual ~AsyncResolverInterface() {}

  
  virtual void Start(const SocketAddress& addr) = 0;
  
  virtual bool GetResolvedAddress(int family, SocketAddress* addr) const = 0;
  
  virtual int GetError() const = 0;
  
  virtual void Destroy(bool wait) = 0;
  
  
  SocketAddress address() const {
    SocketAddress addr;
    GetResolvedAddress(AF_INET, &addr);
    return addr;
  }

  
  sigslot::signal1<AsyncResolverInterface*> SignalDone;
};

}  

#endif
