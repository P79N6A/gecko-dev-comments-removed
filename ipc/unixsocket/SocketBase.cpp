







#include "SocketBase.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace mozilla {
namespace ipc {





UnixSocketBuffer::UnixSocketBuffer()
  : mSize(0)
  , mOffset(0)
  , mAvailableSpace(0)
  , mData(nullptr)
{ }

UnixSocketBuffer::~UnixSocketBuffer()
{
  
  MOZ_ASSERT(!GetBuffer());
}

const uint8_t*
UnixSocketBuffer::Consume(size_t aLen)
{
  if (NS_WARN_IF(GetSize() < aLen)) {
    return nullptr;
  }
  uint8_t* data = mData + mOffset;
  mOffset += aLen;
  return data;
}

nsresult
UnixSocketBuffer::Read(void* aValue, size_t aLen)
{
  const uint8_t* data = Consume(aLen);
  if (!data) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  memcpy(aValue, data, aLen);
  return NS_OK;
}

uint8_t*
UnixSocketBuffer::Append(size_t aLen)
{
  if (((mAvailableSpace - mSize) < aLen)) {
    size_t availableSpace = mAvailableSpace + std::max(mAvailableSpace, aLen);
    uint8_t* data = new uint8_t[availableSpace];
    memcpy(data, mData, mSize);
    mData = data;
    mAvailableSpace = availableSpace;
  }
  uint8_t* data = mData + mSize;
  mSize += aLen;
  return data;
}

nsresult
UnixSocketBuffer::Write(const void* aValue, size_t aLen)
{
  uint8_t* data = Append(aLen);
  if (!data) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  memcpy(data, aValue, aLen);
  return NS_OK;
}

void
UnixSocketBuffer::CleanupLeadingSpace()
{
  if (GetLeadingSpace()) {
    if (GetSize() <= GetLeadingSpace()) {
      memcpy(mData, GetData(), GetSize());
    } else {
      memmove(mData, GetData(), GetSize());
    }
    mOffset = 0;
  }
}





UnixSocketIOBuffer::~UnixSocketIOBuffer()
{ }





UnixSocketRawData::UnixSocketRawData(const void* aData, size_t aSize)
{
  MOZ_ASSERT(aData || !aSize);

  ResetBuffer(static_cast<uint8_t*>(memcpy(new uint8_t[aSize], aData, aSize)),
              0, aSize, aSize);
}

UnixSocketRawData::UnixSocketRawData(size_t aSize)
{
  ResetBuffer(new uint8_t[aSize], 0, 0, aSize);
}

UnixSocketRawData::~UnixSocketRawData()
{
  nsAutoArrayPtr<uint8_t> data(GetBuffer());
  ResetBuffer(nullptr, 0, 0, 0);
}

ssize_t
UnixSocketRawData::Receive(int aFd)
{
  if (!GetTrailingSpace()) {
    if (!GetLeadingSpace()) {
      return -1; 
    }
    
    CleanupLeadingSpace();
  }

  ssize_t res =
    TEMP_FAILURE_RETRY(read(aFd, GetTrailingBytes(), GetTrailingSpace()));

  if (res < 0) {
    
    return -1;
  } else if (!res) {
    
    return 0;
  }

  Append(res); 

  return res;
}

ssize_t
UnixSocketRawData::Send(int aFd)
{
  if (!GetSize()) {
    return 0;
  }

  ssize_t res = TEMP_FAILURE_RETRY(write(aFd, GetData(), GetSize()));

  if (res < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0; 
    }
    return -1;
  } else if (!res) {
    
    return 0;
  }

  Consume(res);

  return res;
}





SocketConnectionStatus
SocketBase::GetConnectionStatus() const
{
  return mConnectionStatus;
}

int
SocketBase::GetSuggestedConnectDelayMs() const
{
  return mConnectDelayMs;
}

void
SocketBase::NotifySuccess()
{
  mConnectionStatus = SOCKET_CONNECTED;
  mConnectTimestamp = PR_IntervalNow();
  OnConnectSuccess();
}

void
SocketBase::NotifyError()
{
  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  mConnectTimestamp = 0;
  OnConnectError();
}

void
SocketBase::NotifyDisconnect()
{
  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  mConnectTimestamp = 0;
  OnDisconnect();
}

uint32_t
SocketBase::CalculateConnectDelayMs() const
{
  uint32_t connectDelayMs = mConnectDelayMs;

  if (mConnectTimestamp && (PR_IntervalNow()-mConnectTimestamp) > connectDelayMs) {
    
    connectDelayMs = 0;
  } else if (!connectDelayMs) {
    
    connectDelayMs = 1<<10;
  } else if (connectDelayMs < (1<<16)) {
    
    connectDelayMs <<= 1;
  }
  return connectDelayMs;
}

SocketBase::SocketBase()
: mConnectionStatus(SOCKET_DISCONNECTED)
, mConnectTimestamp(0)
, mConnectDelayMs(0)
{ }

SocketBase::~SocketBase()
{
  MOZ_ASSERT(mConnectionStatus == SOCKET_DISCONNECTED);
}

void
SocketBase::SetConnectionStatus(SocketConnectionStatus aConnectionStatus)
{
  mConnectionStatus = aConnectionStatus;
}





SocketIOBase::SocketIOBase(nsIThread* aConsumerThread)
  : mConsumerThread(aConsumerThread)
{
  MOZ_ASSERT(mConsumerThread);
}

SocketIOBase::~SocketIOBase()
{ }

nsIThread*
SocketIOBase::GetConsumerThread() const
{
  return mConsumerThread;
}

bool
SocketIOBase::IsConsumerThread() const
{
  nsIThread* thread = nullptr;
  if (NS_FAILED(NS_GetCurrentThread(&thread))) {
    return false;
  }
  return thread == GetConsumerThread();
}





SocketIOEventRunnable::SocketIOEventRunnable(SocketIOBase* aIO,
                                             SocketEvent aEvent)
  : SocketIORunnable<SocketIOBase>(aIO)
  , mEvent(aEvent)
{ }

NS_METHOD
SocketIOEventRunnable::Run()
{
  SocketIOBase* io = SocketIORunnable<SocketIOBase>::GetIO();

  MOZ_ASSERT(io->IsConsumerThread());

  if (NS_WARN_IF(io->IsShutdownOnConsumerThread())) {
    
    
    return NS_OK;
  }

  SocketBase* socketBase = io->GetSocketBase();
  MOZ_ASSERT(socketBase);

  if (mEvent == CONNECT_SUCCESS) {
    socketBase->NotifySuccess();
  } else if (mEvent == CONNECT_ERROR) {
    socketBase->NotifyError();
  } else if (mEvent == DISCONNECT) {
    socketBase->NotifyDisconnect();
  }

  return NS_OK;
}





SocketIORequestClosingRunnable::SocketIORequestClosingRunnable(
  SocketIOBase* aIO)
  : SocketIORunnable<SocketIOBase>(aIO)
{ }

NS_METHOD
SocketIORequestClosingRunnable::Run()
{
  SocketIOBase* io = SocketIORunnable<SocketIOBase>::GetIO();

  MOZ_ASSERT(io->IsConsumerThread());

  if (NS_WARN_IF(io->IsShutdownOnConsumerThread())) {
    
    
    return NS_OK;
  }

  SocketBase* socketBase = io->GetSocketBase();
  MOZ_ASSERT(socketBase);

  socketBase->Close();

  return NS_OK;
}





SocketIODeleteInstanceRunnable::SocketIODeleteInstanceRunnable(
  SocketIOBase* aIO)
  : mIO(aIO)
{ }

NS_METHOD
SocketIODeleteInstanceRunnable::Run()
{
  mIO = nullptr; 

  return NS_OK;
}





SocketIOShutdownTask::SocketIOShutdownTask(SocketIOBase* aIO)
  : SocketIOTask<SocketIOBase>(aIO)
{ }

void
SocketIOShutdownTask::Run()
{
  SocketIOBase* io = SocketIOTask<SocketIOBase>::GetIO();

  MOZ_ASSERT(!io->IsConsumerThread());
  MOZ_ASSERT(!io->IsShutdownOnIOThread());

  
  
  
  
  
  io->ShutdownOnIOThread();
  io->GetConsumerThread()->Dispatch(new SocketIODeleteInstanceRunnable(io),
                                    NS_DISPATCH_NORMAL);
}

}
}
