





#ifndef mozilla_ipc_unixsocket_h
#define mozilla_ipc_unixsocket_h

#include <stdlib.h>
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "mozilla/ipc/SocketBase.h"
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "mozilla/RefPtr.h"
#include "UnixSocketConnector.h"

namespace mozilla {
namespace ipc {

class UnixSocketConsumerIO;

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
