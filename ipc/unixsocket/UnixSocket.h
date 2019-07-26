





#ifndef mozilla_ipc_UnixSocket_h
#define mozilla_ipc_UnixSocket_h


#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#ifdef MOZ_B2G_BT
#include <bluetooth/bluetooth.h>
#include <bluetooth/sco.h>
#include <bluetooth/rfcomm.h>
#endif
#include <stdlib.h>
#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace ipc {

union sockaddr_any {
  sockaddr_storage storage; 
  sockaddr_un un;
  sockaddr_in in;
  sockaddr_in6 in6;
#ifdef MOZ_B2G_BT
  sockaddr_sco sco;
  sockaddr_rc rc;
#endif
  
};

class UnixSocketRawData
{
public:
  nsAutoArrayPtr<uint8_t> mData;

  
  size_t mSize;
  size_t mCurrentWriteOffset;

  




  UnixSocketRawData(int aSize) :
    mSize(aSize),
    mCurrentWriteOffset(0)
  {
    mData = new uint8_t[aSize];
  }
private:
  UnixSocketRawData() {}
};

class UnixSocketImpl;












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

  






  virtual void GetSocketAddr(const sockaddr_any& aAddr,
                             nsAString& aAddrStr) = 0;

};

enum SocketConnectionStatus {
  SOCKET_DISCONNECTED = 0,
  SOCKET_LISTENING = 1,
  SOCKET_CONNECTING = 2,
  SOCKET_CONNECTED = 3
};

class UnixSocketConsumer : public RefCounted<UnixSocketConsumer>
{
public:
  UnixSocketConsumer();

  virtual ~UnixSocketConsumer();

  SocketConnectionStatus GetConnectionStatus()
  {
    return mConnectionStatus;
  }

  





  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage) = 0;

  







  bool SendSocketData(UnixSocketRawData* aMessage);

  








  bool SendSocketData(const nsACString& aMessage);

  









  bool ConnectSocket(UnixSocketConnector* aConnector,
                     const char* aAddress,
                     int aDelayMs = 0);

  







  bool ListenSocket(UnixSocketConnector* aConnector);

  



  void CloseSocket();

  



  virtual void OnConnectSuccess() = 0;

  


  virtual void OnConnectError() = 0;

  


  virtual void OnDisconnect() = 0;

  


  void NotifySuccess();

  


  void NotifyError();

  


  void NotifyDisconnect();

  


  void GetSocketAddr(nsAString& aAddrStr);

private:
  UnixSocketImpl* mImpl;
  SocketConnectionStatus mConnectionStatus;
};

} 
} 

#endif 
