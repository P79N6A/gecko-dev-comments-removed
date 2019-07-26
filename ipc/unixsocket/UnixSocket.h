





#ifndef mozilla_ipc_UnixSocket_h
#define mozilla_ipc_UnixSocket_h

#include <sys/socket.h>
#include <stdlib.h>
#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace ipc {

struct UnixSocketRawData
{
  static const size_t MAX_DATA_SIZE = 1024;
  uint8_t mData[MAX_DATA_SIZE];

  
  size_t mSize;
  size_t mCurrentWriteOffset;

  




  UnixSocketRawData() :
    mSize(0),
    mCurrentWriteOffset(0)
  {
  }

  




  UnixSocketRawData(int aSize) :
    mSize(aSize),
    mCurrentWriteOffset(0)
  {
  }

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

  









  virtual void CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          struct sockaddr *aAddr,
                          const char* aAddress) = 0;

  






  virtual bool SetUp(int aFd) = 0;
};

enum SocketConnectionStatus {
  SOCKET_DISCONNECTED = 0,
  SOCKET_CONNECTING = 1,
  SOCKET_CONNECTED = 2
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

  





  virtual void ReceiveSocketData(UnixSocketRawData* aMessage) = 0;

  







  bool SendSocketData(UnixSocketRawData* aMessage);

  








  bool SendSocketData(const nsACString& aMessage);

  








  bool ConnectSocket(UnixSocketConnector* aConnector, const char* aAddress);

  







  bool ListenSocket(UnixSocketConnector* aConnector);

  



  void CloseSocket();

  


  void CancelSocketTask();

  



  virtual void OnConnectSuccess() = 0;

  


  virtual void OnConnectError() = 0;

  


  virtual void OnDisconnect() = 0;

  


  void NotifySuccess();

  


  void NotifyError();

  


  void NotifyDisconnect();
private:
  UnixSocketImpl* mImpl;
  SocketConnectionStatus mConnectionStatus;
};

} 
} 

#endif 
