







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









class SocketIOBase
{
public:
  virtual ~SocketIOBase();

  





  virtual SocketBase* GetSocketBase() = 0;

  





  virtual bool IsShutdownOnMainThread() const = 0;

protected:
  SocketIOBase();
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





class SocketIOEventRunnable final : public SocketIORunnable<SocketIOBase>
{
public:
  enum SocketEvent {
    CONNECT_SUCCESS,
    CONNECT_ERROR,
    DISCONNECT
  };

  SocketIOEventRunnable(SocketIOBase* aIO, SocketEvent aEvent);

  NS_IMETHOD Run() override;

private:
  SocketEvent mEvent;
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
