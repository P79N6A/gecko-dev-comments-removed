





#ifndef mozilla_ipc_unixsocketconnector_h
#define mozilla_ipc_unixsocketconnector_h

#include "mozilla/ipc/UnixSocketWatcher.h"
#include "nsString.h"

namespace mozilla {
namespace ipc {








class UnixSocketConnector
{
public:
  virtual ~UnixSocketConnector();

  




  virtual int Create() = 0;

  











  virtual bool CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          sockaddr_any& aAddr,
                          const char* aAddress) = 0;

  







  virtual bool SetUp(int aFd) = 0;

  






  virtual bool SetUpListenSocket(int aFd) = 0;

  






  virtual void GetSocketAddr(const sockaddr_any& aAddr,
                             nsAString& aAddrStr) = 0;
};

}
}

#endif
