







#ifndef mozilla_ipc_SocketBase_h
#define mozilla_ipc_SocketBase_h

#include "base/message_loop.h"
#include "nsAutoPtr.h"

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
  UnixSocketBuffer();

  








  void ResetBuffer(uint8_t* aData,
                   size_t aOffset, size_t aSize, size_t aAvailableSpace)
  {
    MOZ_ASSERT(aData || !aAvailableSpace);
    MOZ_ASSERT((aOffset + aSize) <= aAvailableSpace);

    mOffset = aOffset;
    mSize = aSize;
    mAvailableSpace = aAvailableSpace;
    mData = aData;
  }

  




  uint8_t* GetBuffer()
  {
    return mData;
  }

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
  uint8_t* mData;
};











class UnixSocketIOBuffer : public UnixSocketBuffer
{
public:
  UnixSocketIOBuffer();
  virtual ~UnixSocketIOBuffer();

  




  virtual ssize_t Receive(int aFd) = 0;

  



  virtual ssize_t Send(int aFd) = 0;
};





class UnixSocketRawData final : public UnixSocketIOBuffer
{
public:
  






  UnixSocketRawData(const void* aData, size_t aSize);

  





  UnixSocketRawData(size_t aSize);

  


  ~UnixSocketRawData();

  




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

  



  virtual void Close() = 0;

  



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

  





  virtual bool IsShutdownOnIOThread() const = 0;

  





  virtual bool IsShutdownOnConsumerThread() const = 0;

  


  virtual void ShutdownOnIOThread() = 0;

  



  virtual void ShutdownOnConsumerThread() = 0;

  




  MessageLoop* GetConsumerThread() const;

  



  bool IsConsumerThread() const;

protected:
  SocketIOBase(MessageLoop* aConsumerLoop);

private:
  MessageLoop* mConsumerLoop;
};








template <typename T>
class SocketTask : public Task
{
public:
  virtual ~SocketTask()
  { }

  T* GetIO() const
  {
    return mIO;
  }

protected:
  SocketTask(T* aIO)
    : mIO(aIO)
  {
    MOZ_ASSERT(aIO);
  }

private:
  T* mIO;
};





class SocketEventTask final : public SocketTask<SocketIOBase>
{
public:
  enum SocketEvent {
    CONNECT_SUCCESS,
    CONNECT_ERROR,
    DISCONNECT
  };

  SocketEventTask(SocketIOBase* aIO, SocketEvent aEvent);
  ~SocketEventTask();

  void Run() override;

private:
  SocketEvent mEvent;
};





class SocketRequestClosingTask final : public SocketTask<SocketIOBase>
{
public:
  SocketRequestClosingTask(SocketIOBase* aIO);
  ~SocketRequestClosingTask();

  void Run() override;
};




class SocketDeleteInstanceTask final : public Task
{
public:
  SocketDeleteInstanceTask(SocketIOBase* aIO);
  ~SocketDeleteInstanceTask();

  void Run() override;

private:
  nsAutoPtr<SocketIOBase> mIO;
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





class SocketIOShutdownTask final : public SocketIOTask<SocketIOBase>
{
public:
  SocketIOShutdownTask(SocketIOBase* aIO);
  ~SocketIOShutdownTask();

  void Run() override;
};

}
}

#endif
