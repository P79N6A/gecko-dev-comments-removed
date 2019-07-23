




































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
  if (rv != NPERR_NO_ERROR)
    mClosed = true;

  return rv;
}

bool
BrowserStreamChild::AnswerNPP_WriteReady(const int32_t& newlength,
                                         int32_t *size)
{
  PLUGIN_LOG_DEBUG_FUNCTION;

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
  PLUGIN_LOG_DEBUG(("%s (offset=%i, data.length=%i)", FULLFUNCTION,
                    offset, data.Length()));

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
  PLUGIN_LOG_DEBUG(("%s (fname=%s)", FULLFUNCTION, fname.get()));

  AssertPluginThread();

  if (mClosed)
    return true;

  mInstance->mPluginIface->asfile(&mInstance->mData, &mStream,
                                  fname.get());
  return true;
}

bool
BrowserStreamChild::Answer__delete__(const NPError& reason,
                                     const bool& artificial)
{
  AssertPluginThread();
  if (!artificial)
    NPP_DestroyStream(reason);
  return true;
}

NPError
BrowserStreamChild::NPN_RequestRead(NPByteRange* aRangeList)
{
  PLUGIN_LOG_DEBUG_FUNCTION;

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
  PLUGIN_LOG_DEBUG(("%s (reason=%i)", FULLFUNCTION, reason));

  AssertPluginThread();

  if (mClosed)
    return;

  mInstance->mPluginIface->destroystream(&mInstance->mData, &mStream, reason);
  mClosed = true;
}

} 
} 
