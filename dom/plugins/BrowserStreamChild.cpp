




































#include "BrowserStreamChild.h"
#include "PluginInstanceChild.h"

namespace mozilla {
namespace plugins {

BrowserStreamChild::BrowserStreamChild(PluginInstanceChild* instance,
                                       const nsCString& url,
                                       const uint32_t& length,
                                       const uint32_t& lastmodified,
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
  memset(&mStream, 0, sizeof(mStream));
  mStream.ndata = this;
  if (!mURL.IsEmpty())
    mStream.url = mURL.get();
  mStream.end = length;
  mStream.lastmodified = lastmodified;
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
  if (mClosed) {
    *consumed = -1;
    return true;
  }

  *consumed = mInstance->mPluginIface->write(&mInstance->mData, &mStream,
                                             offset, data.Length(),
                                             const_cast<char*>(data.get()));
  if (*consumed < 0)
    mClosed = true;

  return true;
}

bool
BrowserStreamChild::AnswerNPP_StreamAsFile(const nsCString& fname)
{
  if (mClosed)
    return true;

  mInstance->mPluginIface->asfile(&mInstance->mData, &mStream,
                                  fname.get());
  return true;
}

} 
} 
