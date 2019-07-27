







#include "SocketBase.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace mozilla {
namespace ipc {





UnixSocketBuffer::UnixSocketBuffer(const void* aData, size_t aSize)
  : mSize(aSize)
  , mOffset(0)
  , mAvailableSpace(aSize)
{
  MOZ_ASSERT(aData || !mSize);

  mData = new uint8_t[mAvailableSpace];
  memcpy(mData, aData, mSize);
}

UnixSocketBuffer::UnixSocketBuffer(size_t aAvailableSpace)
  : mSize(0)
  , mOffset(0)
  , mAvailableSpace(aAvailableSpace)
{
  mData = new uint8_t[mAvailableSpace];
}

UnixSocketBuffer::~UnixSocketBuffer()
{ }

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





UnixSocketIOBuffer::UnixSocketIOBuffer(const void* aData, size_t aSize)
  : UnixSocketBuffer(aData, aSize)
{ }

UnixSocketIOBuffer::UnixSocketIOBuffer(size_t aAvailableSpace)
  : UnixSocketBuffer(aAvailableSpace)
{ }

UnixSocketIOBuffer::~UnixSocketIOBuffer()
{ }





UnixSocketRawData::UnixSocketRawData(const void* aData, size_t aSize)
: UnixSocketIOBuffer(aData, aSize)
{ }

UnixSocketRawData::UnixSocketRawData(size_t aSize)
: UnixSocketIOBuffer(aSize)
{ }

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
  MOZ_ASSERT(NS_IsMainThread());

  return mConnectionStatus;
}

int
SocketBase::GetSuggestedConnectDelayMs() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mConnectDelayMs;
}

void
SocketBase::NotifySuccess()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnectionStatus = SOCKET_CONNECTED;
  mConnectTimestamp = PR_IntervalNow();
  OnConnectSuccess();
}

void
SocketBase::NotifyError()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  mConnectTimestamp = 0;
  OnConnectError();
}

void
SocketBase::NotifyDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  mConnectTimestamp = 0;
  OnDisconnect();
}

uint32_t
SocketBase::CalculateConnectDelayMs() const
{
  MOZ_ASSERT(NS_IsMainThread());

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





SocketConsumerBase::~SocketConsumerBase()
{ }





SocketIOBase::~SocketIOBase()
{ }

void
SocketIOBase::EnqueueData(UnixSocketIOBuffer* aBuffer)
{
  if (!aBuffer->GetSize()) {
    delete aBuffer; 
    return;
  }
  mOutgoingQ.AppendElement(aBuffer);
}

bool
SocketIOBase::HasPendingData() const
{
  return !mOutgoingQ.IsEmpty();
}

SocketIOBase::SocketIOBase(size_t aMaxReadSize)
  : mMaxReadSize(aMaxReadSize)
{
  MOZ_ASSERT(mMaxReadSize);
}

}
}
