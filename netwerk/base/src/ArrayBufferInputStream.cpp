




#include <algorithm>
#include "ArrayBufferInputStream.h"
#include "nsStreamUtils.h"
#include "jsfriendapi.h"

NS_IMPL_ISUPPORTS2(ArrayBufferInputStream, nsIArrayBufferInputStream, nsIInputStream);

ArrayBufferInputStream::ArrayBufferInputStream()
: mRt(nullptr)
, mArrayBuffer(JSVAL_VOID)
, mBuffer(nullptr)
, mBufferLength(0)
, mOffset(0)
, mClosed(false)
{
}

ArrayBufferInputStream::~ArrayBufferInputStream()
{
  if (mRt) {
    JS_RemoveValueRootRT(mRt, &mArrayBuffer);
  }
}

NS_IMETHODIMP
ArrayBufferInputStream::SetData(const JS::Value& aBuffer,
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

  mRt = JS_GetRuntime(aCx);
  mArrayBuffer = aBuffer;
  JS_AddNamedValueRootRT(mRt, &mArrayBuffer, "mArrayBuffer");

  uint32_t buflen = JS_GetArrayBufferByteLength(arrayBuffer);
  mOffset = std::min(buflen, aByteOffset);
  mBufferLength = std::min(buflen - mOffset, aLength);
  mBuffer = JS_GetArrayBufferData(arrayBuffer);
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
  *aCount = mBufferLength - mOffset;
  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::Read(char* aBuf, uint32_t aCount, uint32_t *aReadCount)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, aReadCount);
  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                     uint32_t aCount, uint32_t *result)
{
  NS_ASSERTION(result, "null ptr");
  NS_ASSERTION(mBufferLength >= mOffset, "bad stream state");

  if (mClosed) {
    return NS_BASE_STREAM_CLOSED;
  }

  uint32_t remaining = mBufferLength - mOffset;
  if (!remaining) {
    *result = 0;
    return NS_OK;
  }

  if (aCount > remaining) {
    aCount = remaining;
  }
  nsresult rv = writer(this, closure, reinterpret_cast<char*>(mBuffer + mOffset),
                       0, aCount, result);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(*result <= aCount,
                 "writer should not write more than we asked it to write");
    mOffset += *result;
  }

  return NS_OK;
}

NS_IMETHODIMP
ArrayBufferInputStream::IsNonBlocking(bool *aNonBlocking)
{
  *aNonBlocking = true;
  return NS_OK;
}
