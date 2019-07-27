





#ifndef mozilla_ipc_UnixSocket_h
#define mozilla_ipc_UnixSocket_h

#include <stdlib.h>
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "mozilla/ipc/SocketBase.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace ipc {

class UnixSocketConsumerIO;












class UnixSocketConnector
{
public:
  UnixSocketConnector()
  {}

  virtual ~UnixSocketConnector()
  {}

  




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

class UnixSocketConsumer : public SocketConsumerBase
{
protected:
  virtual ~UnixSocketConsumer();

public:
  UnixSocketConsumer();

  







  bool SendSocketData(UnixSocketRawData* aMessage);

  








  bool SendSocketData(const nsACString& aMessage);

  









  bool ConnectSocket(UnixSocketConnector* aConnector,
                     const char* aAddress,
                     int aDelayMs = 0);

  







  bool ListenSocket(UnixSocketConnector* aConnector);

  



  void CloseSocket();

  


  void GetSocketAddr(nsAString& aAddrStr);

private:
  UnixSocketConsumerIO* mIO;
};

} 
} 

#endif 
