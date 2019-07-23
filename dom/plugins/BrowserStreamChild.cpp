




































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
  , mURL(url)
  , mHeaders(headers)
{
  AssertPluginThread();

  memset(&mStream, 0, sizeof(mStream));
  mStream.ndata = static_cast<AStream*>(this);
  if (!mURL.IsEmpty())
    mStream.url = mURL.get();
  mStream.end = length;
  mStream.lastmodified = lastmodified;
  if (notifyData)
    mStream.notifyData =
      static_cast<const StreamNotifyChild*>(notifyData)->mClosure;
  if (!mHeaders.IsEmpty())
    mStream.headers = mHeaders.get();

  *rv = mInstance->mPluginIface->newstream(&mInstance->mData,
                                           const_cast<char*>(mimeType.get()),
                                           &mStream, seekable, stype);
  if (*rv != NPERR_NO_ERROR)
    mClosed = true;
}

bool
BrowserStreamChild::AnswerNPP_WriteReady(const int32_t& newlength,
                                         int32_t *size)
{
  AssertPluginThread();

  if (mClosed) {
    *size = 0;
    return true;
  }

  mStream.end = newlength;

  *size = mInstance->mPluginIface->writeready(&mInstance->mData, &mStream);
  return true;
}

bool
BrowserStreamChild::AnswerNPP_Write(const int32_t& offset,
                                    const Buffer& data,
                                    int32_t* consumed)
{
  _MOZ_LOG(__FUNCTION__);
  AssertPluginThread();

  if (mClosed) {
    *consumed = -1;
    return true;
  }

  *consumed = mInstance->mPluginIface->write(&mInstance->mData, &mStream,
                                             offset, data.Length(),
                                             const_cast<char*>(data.get()));
  return true;
}

bool
BrowserStreamChild::AnswerNPP_StreamAsFile(const nsCString& fname)
{
  _MOZ_LOG(__FUNCTION__);
  AssertPluginThread();
  printf("mClosed: %i\n", mClosed);

  if (mClosed)
    return true;

  mInstance->mPluginIface->asfile(&mInstance->mData, &mStream,
                                  fname.get());
  return true;
}

NPError
BrowserStreamChild::NPN_RequestRead(NPByteRange* aRangeList)
{
  AssertPluginThread();

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
BrowserStreamChild::NPP_DestroyStream(NPError reason)
{
  AssertPluginThread();

  if (mClosed)
    return;

  mInstance->mPluginIface->destroystream(&mInstance->mData, &mStream, reason);
  mClosed = true;
}

} 
} 
