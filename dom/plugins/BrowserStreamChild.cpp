




































#include "BrowserStreamChild.h"
#include "PluginInstanceChild.h"
#include "StreamNotifyChild.h"

namespace mozilla {
namespace plugins {

BrowserStreamChild::BrowserStreamChild(PluginInstanceChild* instance,
                                       const nsCString& url,
                                       const uint32_t& length,
                                       const uint32_t& lastmodified,
                                       const PStreamNotifyChild* notifyData,
                                       const nsCString& headers,
                                       const nsCString& mimeType,
                                       const bool& seekable,
                                       NPError* rv,
                                       uint16_t* stype)
  : mInstance(instance)
  , mClosed(false)
  , mState(CONSTRUCTING)
  , mURL(url)
  , mHeaders(headers)
  , mDestroyPending(kDestroyNotPending)
  , mDeliverDataTracker(this)
{
  PLUGIN_LOG_DEBUG(("%s (%s, %i, %i, %p, %s, %s)", FULLFUNCTION,
                    url.get(), length, lastmodified, (void*) notifyData,
                    headers.get(), mimeType.get()));

  AssertPluginThread();

  memset(&mStream, 0, sizeof(mStream));
  mStream.ndata = static_cast<AStream*>(this);
  mStream.url = NullableStringGet(mURL);
  mStream.end = length;
  mStream.lastmodified = lastmodified;
  if (notifyData)
    mStream.notifyData =
      static_cast<const StreamNotifyChild*>(notifyData)->mClosure;
  mStream.headers = NullableStringGet(mHeaders);
}

NPError
BrowserStreamChild::StreamConstructed(
            const nsCString& url,
            const uint32_t& length,
            const uint32_t& lastmodified,
            PStreamNotifyChild* notifyData,
            const nsCString& headers,
            const nsCString& mimeType,
            const bool& seekable,
            uint16_t* stype)
{
  NPError rv = NPERR_NO_ERROR;

  *stype = NP_NORMAL;
  rv = mInstance->mPluginIface->newstream(
    &mInstance->mData, const_cast<char*>(NullableStringGet(mimeType)),
    &mStream, seekable, stype);
  if (rv != NPERR_NO_ERROR) {
    mClosed = true;
    mState = DELETING;
  }
  else {
    mState = ALIVE;
  }

  return rv;
}

bool
BrowserStreamChild::RecvWrite(const int32_t& offset,
                              const Buffer& data,
                              const uint32_t& newlength)
{
  PLUGIN_LOG_DEBUG_FUNCTION;

  AssertPluginThread();

  if (ALIVE != mState)
    NS_RUNTIMEABORT("Unexpected state: received data after NPP_DestroyStream?");

  if (mClosed)
    return true;

  mStream.end = newlength;

  NS_ASSERTION(data.Length() > 0, "Empty data");

  PendingData* newdata = mPendingData.AppendElement();
  newdata->offset = offset;
  newdata->data = data;
  newdata->curpos = 0;

  if (mDeliverDataTracker.empty())
    MessageLoop::current()->PostTask(FROM_HERE,
      mDeliverDataTracker.NewRunnableMethod(&BrowserStreamChild::DeliverData));

  return true;
}

bool
BrowserStreamChild::AnswerNPP_StreamAsFile(const nsCString& fname)
{
  PLUGIN_LOG_DEBUG(("%s (fname=%s)", FULLFUNCTION, fname.get()));

  AssertPluginThread();

  if (ALIVE != mState)
    NS_RUNTIMEABORT("Unexpected state: received file after NPP_DestroyStream?");

  if (mClosed)
    return true;

  mInstance->mPluginIface->asfile(&mInstance->mData, &mStream,
                                  fname.get());
  return true;
}

bool
BrowserStreamChild::RecvNPP_DestroyStream(const NPReason& reason)
{
  PLUGIN_LOG_DEBUG_METHOD;

  if (ALIVE != mState)
    NS_RUNTIMEABORT("Unexpected state: recevied NPP_DestroyStream twice?");

  mState = DYING;
  mDestroyPending = reason;

  if (mDeliverDataTracker.empty())
    MessageLoop::current()->PostTask(FROM_HERE,
      mDeliverDataTracker.NewRunnableMethod(&BrowserStreamChild::DeliverData));

  return true;
}

bool
BrowserStreamChild::Recv__delete__()
{
  AssertPluginThread();

  if (DELETING != mState)
    NS_RUNTIMEABORT("Bad state, not DELETING");

  return true;
}

NPError
BrowserStreamChild::NPN_RequestRead(NPByteRange* aRangeList)
{
  PLUGIN_LOG_DEBUG_FUNCTION;

  AssertPluginThread();

  if (ALIVE != mState || mClosed)
    return NPERR_GENERIC_ERROR;

  IPCByteRanges ranges;
  for (; aRangeList; aRangeList = aRangeList->next) {
    IPCByteRange br = {aRangeList->offset, aRangeList->length};
    ranges.push_back(br);
  }

  NPError result;
  CallNPN_RequestRead(ranges, &result);
  return result;
}

void
BrowserStreamChild::NPN_DestroyStream(NPReason reason)
{
  mClosed = true;
  if (ALIVE == mState)
    SendNPN_DestroyStream(NPRES_NETWORK_ERR);

  ClearSuspendedTimer();
  mPendingData.Clear();
  MaybeDeliverNPP_DestroyStream();
}

void
BrowserStreamChild::DeliverData()
{
  if (mState != ALIVE && mState != DYING)
    NS_RUNTIMEABORT("Unexpected state");

  if (mClosed) {
    ClearSuspendedTimer();
    MaybeDeliverNPP_DestroyStream();
    return;
  }

  while (mPendingData.Length()) {
    while (mPendingData[0].curpos < mPendingData[0].data.Length()) {
      int32_t r = mInstance->mPluginIface->writeready(&mInstance->mData, &mStream);
      if (r == 0) {
        SetSuspendedTimer();
        return;
      }
      if (!mPendingData.Length())
        break;
      r = mInstance->mPluginIface->write(
        &mInstance->mData, &mStream,
        mPendingData[0].offset + mPendingData[0].curpos, 
        mPendingData[0].data.Length() - mPendingData[0].curpos, 
        const_cast<char*>(mPendingData[0].data.BeginReading() + mPendingData[0].curpos));
      if (r == 0) {
        SetSuspendedTimer();
        return;
      }
      if (r < 0) { 
        NPN_DestroyStream(NPRES_NETWORK_ERR);
        return;
      }
      if (!mPendingData.Length())
        break;

      mPendingData[0].curpos += r;
    }
    if (!mPendingData.Length())
      break;
    mPendingData.RemoveElementAt(0);
  }

  ClearSuspendedTimer();
  MaybeDeliverNPP_DestroyStream();
}

void
BrowserStreamChild::MaybeDeliverNPP_DestroyStream()
{
  if (kDestroyNotPending != mDestroyPending) {
    NS_ASSERTION(mPendingData.Length() == 0, "Pending data?");

    if (mState != DYING)
      NS_RUNTIMEABORT("mDestroyPending but state not DYING");

    mClosed = true;
    NPReason reason = mDestroyPending;
    mDestroyPending = kDestroyNotPending;

    (void) mInstance->mPluginIface
      ->destroystream(&mInstance->mData, &mStream, reason);

    SendStreamDestroyed();
    mState = DELETING;
  }
}


void
BrowserStreamChild::SetSuspendedTimer()
{
  if (mSuspendedTimer.IsRunning())
    return;
  mSuspendedTimer.Start(
    base::TimeDelta::FromMilliseconds(100),
    this, &BrowserStreamChild::DeliverData);
}

void
BrowserStreamChild::ClearSuspendedTimer()
{
  mSuspendedTimer.Stop();
}

} 
} 
