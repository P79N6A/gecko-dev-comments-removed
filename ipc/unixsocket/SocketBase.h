







#ifndef mozilla_ipc_SocketBase_h
#define mozilla_ipc_SocketBase_h

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace ipc {





class UnixSocketRawData
{
public:
  
  size_t mSize;
  size_t mCurrentWriteOffset;
  nsAutoArrayPtr<uint8_t> mData;

  



  UnixSocketRawData(size_t aSize);

  



  UnixSocketRawData(const void* aData, size_t aSize);
};

enum SocketConnectionStatus {
  SOCKET_DISCONNECTED = 0,
  SOCKET_LISTENING = 1,
  SOCKET_CONNECTING = 2,
  SOCKET_CONNECTED = 3
};





class SocketConsumerBase
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SocketConsumerBase)

  virtual ~SocketConsumerBase();

  SocketConnectionStatus GetConnectionStatus() const;

  int GetSuggestedConnectDelayMs() const;

  



  virtual void CloseSocket() = 0;

  





  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage) = 0;

  







  virtual bool SendSocketData(UnixSocketRawData* aMessage) = 0;

  



  virtual void OnConnectSuccess() = 0;

  


  virtual void OnConnectError() = 0;

  


  virtual void OnDisconnect() = 0;

  


  void NotifySuccess();

  


  void NotifyError();

  


  void NotifyDisconnect();

protected:
  SocketConsumerBase();

  void SetConnectionStatus(SocketConnectionStatus aConnectionStatus);

private:
  uint32_t CalculateConnectDelayMs() const;

  SocketConnectionStatus mConnectionStatus;
  PRIntervalTime mConnectTimestamp;
  uint32_t mConnectDelayMs;
};








template <typename T>
class SocketIORunnable : public nsRunnable
{
public:
  virtual ~SocketIORunnable()
  { }

  T* GetIO() const
  {
    return mIO;
  }

protected:
  SocketIORunnable(T* aIO)
  : mIO(aIO)
  {
    MOZ_ASSERT(aIO);
  }

private:
  T* mIO;
};




template <typename T>
class SocketIOEventRunnable MOZ_FINAL : public SocketIORunnable<T>
{
public:
  enum SocketEvent {
    CONNECT_SUCCESS,
    CONNECT_ERROR,
    DISCONNECT
  };

  SocketIOEventRunnable(T* aIO, SocketEvent e)
  : SocketIORunnable<T>(aIO)
  , mEvent(e)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    T* io = SocketIORunnable<T>::GetIO();

    if (io->IsShutdownOnMainThread()) {
      NS_WARNING("I/O consumer has already been closed!");
      
      
      return NS_OK;
    }

    SocketConsumerBase* consumer = io->GetConsumer();
    MOZ_ASSERT(consumer);

    if (mEvent == CONNECT_SUCCESS) {
      consumer->NotifySuccess();
    } else if (mEvent == CONNECT_ERROR) {
      consumer->NotifyError();
    } else if (mEvent == DISCONNECT) {
      consumer->NotifyDisconnect();
    }

    return NS_OK;
  }

private:
  SocketEvent mEvent;
};

}
}

#endif
