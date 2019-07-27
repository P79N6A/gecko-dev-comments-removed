







#ifndef mozilla_ipc_SocketBase_h
#define mozilla_ipc_SocketBase_h

#include "base/message_loop.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
using namespace mozilla::tasktracer;
#endif

namespace mozilla {
namespace ipc {





class UnixSocketRawData
{
public:
  


  UnixSocketRawData(const void* aData, size_t aSize);

  


  UnixSocketRawData(size_t aSize);

  




  ssize_t Receive(int aFd);

  



  ssize_t Send(int aFd);

  const uint8_t* GetData() const
  {
    return mData + mOffset;
  }

  size_t GetSize() const
  {
    return mSize;
  }

  void Consume(size_t aSize)
  {
    MOZ_ASSERT(aSize <= mSize);

    mSize -= aSize;
    mOffset += aSize;
  }

protected:
  size_t GetLeadingSpace() const
  {
    return mOffset;
  }

  size_t GetTrailingSpace() const
  {
    return mAvailableSpace - (mOffset + mSize);
  }

  size_t GetAvailableSpace() const
  {
    return mAvailableSpace;
  }

  void* GetTrailingBytes()
  {
    return mData + mOffset + mSize;
  }

private:
  size_t mSize;
  size_t mOffset;
  size_t mAvailableSpace;
  nsAutoArrayPtr<uint8_t> mData;
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




template <typename T>
class SocketIOReceiveRunnable MOZ_FINAL : public SocketIORunnable<T>
{
public:
  SocketIOReceiveRunnable(T* aIO, UnixSocketRawData* aData)
  : SocketIORunnable<T>(aIO)
  , mData(aData)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    T* io = SocketIORunnable<T>::GetIO();

    if (io->IsShutdownOnMainThread()) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }

    SocketConsumerBase* consumer = io->GetConsumer();
    MOZ_ASSERT(consumer);

    consumer->ReceiveSocketData(mData);

    return NS_OK;
  }

private:
  nsAutoPtr<UnixSocketRawData> mData;
};

template <typename T>
class SocketIORequestClosingRunnable MOZ_FINAL : public SocketIORunnable<T>
{
public:
  SocketIORequestClosingRunnable(T* aImpl)
  : SocketIORunnable<T>(aImpl)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    T* io = SocketIORunnable<T>::GetIO();

    if (io->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }

    SocketConsumerBase* consumer = io->GetConsumer();
    MOZ_ASSERT(consumer);

    consumer->CloseSocket();

    return NS_OK;
  }
};



template<class T>
class SocketIODeleteInstanceRunnable MOZ_FINAL : public nsRunnable
{
public:
  SocketIODeleteInstanceRunnable(T* aInstance)
  : mInstance(aInstance)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mInstance = nullptr; 

    return NS_OK;
  }

private:
  nsAutoPtr<T> mInstance;
};









class SocketIOBase
{
public:
  virtual ~SocketIOBase();

  void EnqueueData(UnixSocketRawData* aData);
  bool HasPendingData() const;

  template <typename T>
  ssize_t ReceiveData(int aFd, T* aIO)
  {
    MOZ_ASSERT(aFd >= 0);
    MOZ_ASSERT(aIO);

    nsAutoPtr<UnixSocketRawData> incoming(
      new UnixSocketRawData(mMaxReadSize));

    ssize_t res = incoming->Receive(aFd);
    if (res < 0) {
      
      nsRefPtr<nsRunnable> r = new SocketIORequestClosingRunnable<T>(aIO);
      NS_DispatchToMainThread(r);
      return -1;
    } else if (!res) {
      
      nsRefPtr<nsRunnable> r = new SocketIORequestClosingRunnable<T>(aIO);
      NS_DispatchToMainThread(r);
      return 0;
    }

#ifdef MOZ_TASK_TRACER
    
    
    AutoSourceEvent taskTracerEvent(SourceEventType::UNIXSOCKET);
#endif

    nsRefPtr<nsRunnable> r =
      new SocketIOReceiveRunnable<T>(aIO, incoming.forget());
    NS_DispatchToMainThread(r);

    return res;
  }

  template <typename T>
  nsresult SendPendingData(int aFd, T* aIO)
  {
    MOZ_ASSERT(aFd >= 0);
    MOZ_ASSERT(aIO);

    while (HasPendingData()) {
      UnixSocketRawData* outgoing = mOutgoingQ.ElementAt(0);

      ssize_t res = outgoing->Send(aFd);
      if (res < 0) {
        
        nsRefPtr<nsRunnable> r = new SocketIORequestClosingRunnable<T>(aIO);
        NS_DispatchToMainThread(r);
        return NS_ERROR_FAILURE;
      } else if (!res && outgoing->GetSize()) {
        
        return NS_OK;
      }
      if (!outgoing->GetSize()) {
        mOutgoingQ.RemoveElementAt(0);
        delete outgoing;
      }
    }

    return NS_OK;
  }

protected:
  SocketIOBase(size_t aMaxReadSize);

private:
  const size_t mMaxReadSize;

  


  nsTArray<UnixSocketRawData*> mOutgoingQ;
};








template <typename T>
class SocketIOTask : public CancelableTask
{
public:
  virtual ~SocketIOTask()
  { }

  T* GetIO() const
  {
    return mIO;
  }

  void Cancel() MOZ_OVERRIDE
  {
    mIO = nullptr;
  }

  bool IsCanceled() const
  {
    return !mIO;
  }

protected:
  SocketIOTask(T* aIO)
  : mIO(aIO)
  {
    MOZ_ASSERT(mIO);
  }

private:
  T* mIO;
};




template <typename T>
class SocketIOSendTask MOZ_FINAL : public SocketIOTask<T>
{
public:
  SocketIOSendTask(T* aIO, UnixSocketRawData* aData)
  : SocketIOTask<T>(aIO)
  , mData(aData)
  {
    MOZ_ASSERT(aData);
  }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!SocketIOTask<T>::IsCanceled());

    T* io = SocketIOTask<T>::GetIO();
    MOZ_ASSERT(!io->IsShutdownOnIOThread());

    io->Send(mData);
  }

private:
  UnixSocketRawData* mData;
};




template <typename T>
class SocketIOShutdownTask MOZ_FINAL : public SocketIOTask<T>
{
public:
  SocketIOShutdownTask(T* aIO)
  : SocketIOTask<T>(aIO)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());

    T* io = SocketIOTask<T>::GetIO();

    
    
    
    
    
    io->ShutdownOnIOThread();

    nsRefPtr<nsRunnable> r = new SocketIODeleteInstanceRunnable<T>(io);
    nsresult rv = NS_DispatchToMainThread(r);
    NS_ENSURE_SUCCESS_VOID(rv);
  }
};

}
}

#endif
