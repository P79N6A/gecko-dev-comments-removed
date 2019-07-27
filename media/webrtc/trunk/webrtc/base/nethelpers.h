









#ifndef WEBRTC_BASE_NETHELPERS_H_
#define WEBRTC_BASE_NETHELPERS_H_

#if defined(WEBRTC_POSIX)
#include <netdb.h>
#include <stddef.h>
#elif WEBRTC_WIN
#include <winsock2.h>  
#endif

#include <list>

#include "webrtc/base/asyncresolverinterface.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/socketaddress.h"

namespace rtc {

class AsyncResolverTest;



class AsyncResolver : public SignalThread, public AsyncResolverInterface {
 public:
  AsyncResolver();
  virtual ~AsyncResolver() {}

  virtual void Start(const SocketAddress& addr);
  virtual bool GetResolvedAddress(int family, SocketAddress* addr) const;
  virtual int GetError() const { return error_; }
  virtual void Destroy(bool wait) { SignalThread::Destroy(wait); }

  const std::vector<IPAddress>& addresses() const { return addresses_; }
  void set_error(int error) { error_ = error; }

 protected:
  virtual void DoWork();
  virtual void OnWorkDone();

 private:
  SocketAddress addr_;
  std::vector<IPAddress> addresses_;
  int error_;
};



const char* inet_ntop(int af, const void *src, char* dst, socklen_t size);
int inet_pton(int af, const char* src, void *dst);

bool HasIPv6Enabled();
}  

#endif  
