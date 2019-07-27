







#include "SocketBase.h"
#include <string.h>
#include "nsThreadUtils.h"

namespace mozilla {
namespace ipc {





UnixSocketRawData::UnixSocketRawData(size_t aSize)
: mSize(aSize)
, mCurrentWriteOffset(0)
{
  mData = new uint8_t[mSize];
}

UnixSocketRawData::UnixSocketRawData(const void* aData, size_t aSize)
: mSize(aSize)
, mCurrentWriteOffset(0)
{
  MOZ_ASSERT(aData || !mSize);

  mData = new uint8_t[mSize];
  memcpy(mData, aData, mSize);
}





SocketConsumerBase::~SocketConsumerBase()
{
  MOZ_ASSERT(mConnectionStatus == SOCKET_DISCONNECTED);
}

SocketConnectionStatus
SocketConsumerBase::GetConnectionStatus() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mConnectionStatus;
}

int
SocketConsumerBase::GetSuggestedConnectDelayMs() const
{
  MOZ_ASSERT(NS_IsMainThread());

  return mConnectDelayMs;
}

void
SocketConsumerBase::NotifySuccess()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnectionStatus = SOCKET_CONNECTED;
  mConnectTimestamp = PR_IntervalNow();
  OnConnectSuccess();
}

void
SocketConsumerBase::NotifyError()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  mConnectTimestamp = 0;
  OnConnectError();
}

void
SocketConsumerBase::NotifyDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());

  mConnectionStatus = SOCKET_DISCONNECTED;
  mConnectDelayMs = CalculateConnectDelayMs();
  mConnectTimestamp = 0;
  OnDisconnect();
}

uint32_t
SocketConsumerBase::CalculateConnectDelayMs() const
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

SocketConsumerBase::SocketConsumerBase()
: mConnectionStatus(SOCKET_DISCONNECTED)
, mConnectTimestamp(0)
, mConnectDelayMs(0)
{ }

void
SocketConsumerBase::SetConnectionStatus(
  SocketConnectionStatus aConnectionStatus)
{
  mConnectionStatus = aConnectionStatus;
}





SocketIOBase::~SocketIOBase()
{ }

void
SocketIOBase::EnqueueData(UnixSocketRawData* aData)
{
  if (!aData->mSize) {
    delete aData; 
    return;
  }
  mOutgoingQ.AppendElement(aData);
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
