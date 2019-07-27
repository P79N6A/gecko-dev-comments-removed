







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









class UnixSocketBuffer
{
public:
  virtual ~UnixSocketBuffer();

  const uint8_t* GetData() const
  {
    return mData + mOffset;
  }

  size_t GetSize() const
  {
    return mSize - mOffset;
  }

  const uint8_t* Consume(size_t aLen);

  nsresult Read(void* aValue, size_t aLen);

  nsresult Read(int8_t& aValue)
  {
    return Read(&aValue, sizeof(aValue));
  }

  nsresult Read(uint8_t& aValue)
  {
    return Read(&aValue, sizeof(aValue));
  }

  nsresult Read(int16_t& aValue)
  {
    return Read(&aValue, sizeof(aValue));
  }

  nsresult Read(uint16_t& aValue)
  {
    return Read(&aValue, sizeof(aValue));
  }

  nsresult Read(int32_t& aValue)
  {
    return Read(&aValue, sizeof(aValue));
  }

  nsresult Read(uint32_t& aValue)
  {
    return Read(&aValue, sizeof(aValue));
  }

  uint8_t* Append(size_t aLen);

  nsresult Write(const void* aValue, size_t aLen);

  nsresult Write(int8_t aValue)
  {
    return Write(&aValue, sizeof(aValue));
  }

  nsresult Write(uint8_t aValue)
  {
    return Write(&aValue, sizeof(aValue));
  }

  nsresult Write(int16_t aValue)
  {
    return Write(&aValue, sizeof(aValue));
  }

  nsresult Write(uint16_t aValue)
  {
    return Write(&aValue, sizeof(aValue));
  }

  nsresult Write(int32_t aValue)
  {
    return Write(&aValue, sizeof(aValue));
  }

  nsresult Write(uint32_t aValue)
  {
    return Write(&aValue, sizeof(aValue));
  }

protected:

  


  UnixSocketBuffer(const void* aData, size_t aSize);

  

  UnixSocketBuffer(size_t aAvailableSpace);

  size_t GetLeadingSpace() const
  {
    return mOffset;
  }

  size_t GetTrailingSpace() const
  {
    return mAvailableSpace - mSize;
  }

  size_t GetAvailableSpace() const
  {
    return mAvailableSpace;
  }

  void* GetTrailingBytes()
  {
    return mData + mSize;
  }

  uint8_t* GetData(size_t aOffset)
  {
    MOZ_ASSERT(aOffset <= mSize);

    return mData + aOffset;
  }

  void SetRange(size_t aOffset, size_t aSize)
  {
    MOZ_ASSERT((aOffset + aSize) <= mAvailableSpace);

    mOffset = aOffset;
    mSize = mOffset + aSize;
  }

  void CleanupLeadingSpace();

private:
  size_t mSize;
  size_t mOffset;
  size_t mAvailableSpace;
  nsAutoArrayPtr<uint8_t> mData;
};











class UnixSocketIOBuffer : public UnixSocketBuffer
{
public:
  virtual ~UnixSocketIOBuffer();

  




  virtual ssize_t Receive(int aFd) = 0;

  



  virtual ssize_t Send(int aFd) = 0;

protected:

  


  UnixSocketIOBuffer(const void* aData, size_t aSize);

  

  UnixSocketIOBuffer(size_t aAvailableSpace);
};





class UnixSocketRawData final : public UnixSocketIOBuffer
{
public:
  


  UnixSocketRawData(const void* aData, size_t aSize);

  


  UnixSocketRawData(size_t aSize);

  




  ssize_t Receive(int aFd) override;

  



  ssize_t Send(int aFd) override;
};

enum SocketConnectionStatus {
  SOCKET_DISCONNECTED = 0,
  SOCKET_LISTENING = 1,
  SOCKET_CONNECTING = 2,
  SOCKET_CONNECTED = 3
};





class SocketBase
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SocketBase)

  SocketConnectionStatus GetConnectionStatus() const;

  int GetSuggestedConnectDelayMs() const;

  



  virtual void CloseSocket() = 0;

  



  virtual void OnConnectSuccess() = 0;

  


  virtual void OnConnectError() = 0;

  


  virtual void OnDisconnect() = 0;

  


  void NotifySuccess();

  


  void NotifyError();

  


  void NotifyDisconnect();

protected:
  SocketBase();
  virtual ~SocketBase();

  void SetConnectionStatus(SocketConnectionStatus aConnectionStatus);

private:
  uint32_t CalculateConnectDelayMs() const;

  SocketConnectionStatus mConnectionStatus;
  PRIntervalTime mConnectTimestamp;
  uint32_t mConnectDelayMs;
};





class SocketConsumerBase : public SocketBase
{
public:
  virtual ~SocketConsumerBase();

  





  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage) = 0;

  







  virtual bool SendSocketData(UnixSocketRawData* aMessage) = 0;
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
class SocketIOEventRunnable final : public SocketIORunnable<T>
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

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    T* io = SocketIORunnable<T>::GetIO();

    if (io->IsShutdownOnMainThread()) {
      NS_WARNING("I/O consumer has already been closed!");
      
      
      return NS_OK;
    }

    SocketBase* base = io->GetSocketBase();
    MOZ_ASSERT(base);

    if (mEvent == CONNECT_SUCCESS) {
      base->NotifySuccess();
    } else if (mEvent == CONNECT_ERROR) {
      base->NotifyError();
    } else if (mEvent == DISCONNECT) {
      base->NotifyDisconnect();
    }

    return NS_OK;
  }

private:
  SocketEvent mEvent;
};




template <typename T>
class SocketIOReceiveRunnable final : public SocketIORunnable<T>
{
public:
  SocketIOReceiveRunnable(T* aIO, UnixSocketRawData* aData)
  : SocketIORunnable<T>(aIO)
  , mData(aData)
  { }

  NS_IMETHOD Run() override
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
class SocketIORequestClosingRunnable final : public SocketIORunnable<T>
{
public:
  SocketIORequestClosingRunnable(T* aImpl)
  : SocketIORunnable<T>(aImpl)
  { }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    T* io = SocketIORunnable<T>::GetIO();

    if (io->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }

    SocketBase* base = io->GetSocketBase();
    MOZ_ASSERT(base);

    base->CloseSocket();

    return NS_OK;
  }
};



template<class T>
class SocketIODeleteInstanceRunnable final : public nsRunnable
{
public:
  SocketIODeleteInstanceRunnable(T* aInstance)
  : mInstance(aInstance)
  { }

  NS_IMETHOD Run() override
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
    
    
    AutoSourceEvent taskTracerEvent(SourceEventType::Unixsocket);
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








template<typename Tio>
class SocketIOTask : public CancelableTask
{
public:
  virtual ~SocketIOTask()
  { }

  Tio* GetIO() const
  {
    return mIO;
  }

  void Cancel() override
  {
    mIO = nullptr;
  }

  bool IsCanceled() const
  {
    return !mIO;
  }

protected:
  SocketIOTask(Tio* aIO)
  : mIO(aIO)
  {
    MOZ_ASSERT(mIO);
  }

private:
  Tio* mIO;
};





template<typename Tio, typename Tdata>
class SocketIOSendTask final : public SocketIOTask<Tio>
{
public:
  SocketIOSendTask(Tio* aIO, Tdata* aData)
  : SocketIOTask<Tio>(aIO)
  , mData(aData)
  {
    MOZ_ASSERT(aData);
  }

  void Run() override
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!SocketIOTask<Tio>::IsCanceled());

    Tio* io = SocketIOTask<Tio>::GetIO();
    MOZ_ASSERT(!io->IsShutdownOnIOThread());

    io->Send(mData);
  }

private:
  Tdata* mData;
};




template<typename Tio>
class SocketIOShutdownTask final : public SocketIOTask<Tio>
{
public:
  SocketIOShutdownTask(Tio* aIO)
  : SocketIOTask<Tio>(aIO)
  { }

  void Run() override
  {
    MOZ_ASSERT(!NS_IsMainThread());

    Tio* io = SocketIOTask<Tio>::GetIO();

    
    
    
    
    
    io->ShutdownOnIOThread();

    nsRefPtr<nsRunnable> r = new SocketIODeleteInstanceRunnable<Tio>(io);
    nsresult rv = NS_DispatchToMainThread(r);
    NS_ENSURE_SUCCESS_VOID(rv);
  }
};

}
}

#endif
