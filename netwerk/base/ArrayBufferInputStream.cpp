




#include <algorithm>
#include "ArrayBufferInputStream.h"
#include "nsStreamUtils.h"
#include "jsapi.h"
#include "jsfriendapi.h"

NS_IMPL_ISUPPORTS(ArrayBufferInputStream, nsIArrayBufferInputStream, nsIInputStream);

ArrayBufferInputStream::ArrayBufferInputStream()
: mBufferLength(0)
, mOffset(0)
, mPos(0)
, mClosed(false)
{
}

NS_IMETHODIMP
ArrayBufferInputStream::SetData(JS::Handle<JS::Value> aBuffer,
                                uint32_t aByteOffset,
                                uint32_t aLength,
                                JSContext* aCx)
{
  if (!aBuffer.isObject()) {
    return NS_ERROR_FAILURE;
  }
  JS::RootedObject arrayBuffer(aCx, &aBuffer.toObject());
  if (!JS_IsArrayBufferObject(arrayBuffer)) {
    return NS_ERROR_FAILURE;
  }

  mArrayBuffer.emplace(aCx, arrayBuffer);

  uint32_t buflen = JS_GetArrayBufferByteLength(arrayBuffer);
  mOffset = std::min(buflen, aByteOffset);
  mBufferLength = std::min(buflen - mOffset, aLength);
  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::Close()
{
  mClosed = true;
  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::Available(uint64_t* aCount)
{
  if (mClosed) {
    return NS_BASE_STREAM_CLOSED;
  }
  if (mArrayBuffer) {
    uint32_t buflen = JS_GetArrayBufferByteLength(mArrayBuffer->get());
    *aCount = buflen ? buflen - mPos : 0;
  } else {
    *aCount = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::Read(char* aBuf, uint32_t aCount, uint32_t *aReadCount)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, aReadCount);
}

NS_IMETHODIMP
ArrayBufferInputStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                     uint32_t aCount, uint32_t *result)
{
  NS_ASSERTION(result, "null ptr");
  NS_ASSERTION(mBufferLength >= mPos, "bad stream state");

  if (mClosed) {
    return NS_BASE_STREAM_CLOSED;
  }

  MOZ_ASSERT(mArrayBuffer || (mPos == mBufferLength), "stream inited incorrectly");

  *result = 0;
  while (mPos < mBufferLength) {
    uint32_t remaining = mBufferLength - mPos;
    MOZ_ASSERT(mArrayBuffer);
    uint32_t byteLength = JS_GetArrayBufferByteLength(mArrayBuffer->get());
    if (byteLength == 0) {
      mClosed = true;
      return NS_BASE_STREAM_CLOSED;
    }

    char buffer[8192];
    uint32_t count = std::min(std::min(aCount, remaining), uint32_t(mozilla::ArrayLength(buffer)));
    if (count == 0) {
      break;
    }

    
    
    
    
    
    {
      JS::AutoCheckCannotGC nogc;
      char* src = (char*) JS_GetArrayBufferData(mArrayBuffer->get(), nogc) + mOffset + mPos;
      memcpy(buffer, src, count);
    }
    uint32_t written;
    nsresult rv = writer(this, closure, buffer, 0, count, &written);
    if (NS_FAILED(rv)) {
      
      return NS_OK;
    }

    NS_ASSERTION(written <= count,
                 "writer should not write more than we asked it to write");
    mPos += written;
    *result += written;
    aCount -= written;
  }

  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::IsNonBlocking(bool *aNonBlocking)
{
  *aNonBlocking = true;
  return NS_OK;
}
