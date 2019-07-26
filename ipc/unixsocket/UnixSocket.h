





#ifndef mozilla_ipc_UnixSocket_h
#define mozilla_ipc_UnixSocket_h


#include <stdlib.h>
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace ipc {

class UnixSocketRawData
{
public:
  
  size_t mSize;
  size_t mCurrentWriteOffset;
  nsAutoArrayPtr<uint8_t> mData;

  



  UnixSocketRawData(size_t aSize) :
    mSize(aSize),
    mCurrentWriteOffset(0)
  {
    mData = new uint8_t[mSize];
  }

  



  UnixSocketRawData(const void* aData, size_t aSize)
    : mSize(aSize),
      mCurrentWriteOffset(0)
  {
    MOZ_ASSERT(aData || !mSize);
    mData = new uint8_t[mSize];
    memcpy(mData, aData, mSize);
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

  











  virtual bool CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          sockaddr_any& aAddr,
                          const char* aAddress) = 0;

  







  virtual bool SetUp(int aFd) = 0;

  






  virtual bool SetUpListenSocket(int aFd) = 0;

  






  virtual void GetSocketAddr(const sockaddr_any& aAddr,
                             nsAString& aAddrStr) = 0;

};

enum SocketConnectionStatus {
  SOCKET_DISCONNECTED = 0,
  SOCKET_LISTENING = 1,
  SOCKET_CONNECTING = 2,
  SOCKET_CONNECTED = 3
};

class UnixSocketConsumer
{
protected:
  virtual ~UnixSocketConsumer();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(UnixSocketConsumer)

  UnixSocketConsumer();

  SocketConnectionStatus GetConnectionStatus() const
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mConnectionStatus;
  }

  int GetSuggestedConnectDelayMs() const
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mConnectDelayMs;
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
  uint32_t CalculateConnectDelayMs() const;

  UnixSocketImpl* mImpl;
  SocketConnectionStatus mConnectionStatus;
  PRIntervalTime mConnectTimestamp;
  uint32_t mConnectDelayMs;
};

} 
} 

#endif 
