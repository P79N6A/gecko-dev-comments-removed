





#include "WMF.h"

#include <unknwn.h>
#include <ole2.h>

#include "WMFByteStream.h"
#include "WMFUtils.h"
#include "MediaResource.h"
#include "nsISeekableStream.h"
#include "mozilla/RefPtr.h"

namespace mozilla {

#ifdef PR_LOGGING
PRLogModuleInfo* gWMFByteStreamLog = nullptr;
#define LOG(...) PR_LOG(gWMFByteStreamLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

HRESULT
DoGetInterface(IUnknown* aUnknown, void** aInterface)
{
  if (!aInterface)
    return E_POINTER;
  *aInterface = aUnknown;
  aUnknown->AddRef();
  return S_OK;
}

WMFByteStream::WMFByteStream(MediaResource* aResource)
  : mWorkQueueId(MFASYNC_CALLBACK_QUEUE_UNDEFINED),
    mResource(aResource),
    mReentrantMonitor("WMFByteStream"),
    mOffset(0)
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
  NS_ASSERTION(mResource, "Must have a valid media resource");

#ifdef PR_LOGGING
  if (!gWMFByteStreamLog) {
    gWMFByteStreamLog = PR_NewLogModule("WMFByteStream");
  }
#endif

  MOZ_COUNT_CTOR(WMFByteStream);
}

WMFByteStream::~WMFByteStream()
{
  MOZ_COUNT_DTOR(WMFByteStream);
}

nsresult
WMFByteStream::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
  
  HRESULT hr = wmf::MFAllocateWorkQueue(&mWorkQueueId);
  if (FAILED(hr)) {
    NS_WARNING("WMFByteStream Failed to allocate work queue.");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
WMFByteStream::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
  if (mWorkQueueId != MFASYNC_CALLBACK_QUEUE_UNDEFINED) {
    HRESULT hr = wmf::MFUnlockWorkQueue(mWorkQueueId);
    if (FAILED(hr)) {
      NS_WARNING("WMFByteStream Failed to unlock work queue.");
      LOG("WMFByteStream unlock work queue hr=%x %d\n", hr, hr);
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}


STDMETHODIMP
WMFByteStream::QueryInterface(REFIID aIId, void **aInterface)
{
  LOG("WMFByteStream::QueryInterface %s", GetGUIDName(aIId).get());

  if (aIId == IID_IMFByteStream) {
    return DoGetInterface(static_cast<IMFByteStream*>(this), aInterface);
  }
  if (aIId == IID_IMFAsyncCallback) {
    return DoGetInterface(static_cast<IMFAsyncCallback*>(this), aInterface);
  }
  if (aIId == IID_IUnknown) {
    return DoGetInterface(static_cast<IMFByteStream*>(this), aInterface);
  }

  *aInterface = NULL;
  return E_NOINTERFACE;
}

NS_IMPL_THREADSAFE_ADDREF(WMFByteStream)
NS_IMPL_THREADSAFE_RELEASE(WMFByteStream)

NS_IMPL_THREADSAFE_ADDREF(WMFByteStream::AsyncReadRequestState)
NS_IMPL_THREADSAFE_RELEASE(WMFByteStream::AsyncReadRequestState)


STDMETHODIMP
WMFByteStream::AsyncReadRequestState::QueryInterface(REFIID aIId, void **aInterface)
{
  LOG("WMFByteStream::AsyncReadRequestState::QueryInterface %s", GetGUIDName(aIId).get());

  if (aIId == IID_IUnknown) {
    return DoGetInterface(static_cast<IUnknown*>(this), aInterface);
  }

  *aInterface = NULL;
  return E_NOINTERFACE;
}


STDMETHODIMP
WMFByteStream::BeginRead(BYTE *aBuffer,
                         ULONG aLength,
                         IMFAsyncCallback *aCallback,
                         IUnknown *aCallerState)
{
  NS_ENSURE_TRUE(aBuffer, E_POINTER);
  NS_ENSURE_TRUE(aCallback, E_POINTER);

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  LOG("WMFByteStream::BeginRead() mOffset=%lld tell=%lld length=%lu",
      mOffset, mResource->Tell(), aLength);

  
  RefPtr<IUnknown> requestState = new AsyncReadRequestState(mOffset, aBuffer, aLength);

  
  
  RefPtr<IMFAsyncResult> callersResult;
  HRESULT hr = wmf::MFCreateAsyncResult(requestState,
                                        aCallback,
                                        aCallerState,
                                        byRef(callersResult));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  
  
  
  
  hr = wmf::MFPutWorkItem(mWorkQueueId, this, callersResult);

  return hr;
}



STDMETHODIMP
WMFByteStream::Invoke(IMFAsyncResult* aResult)
{
  
  
  
  
  

  
  RefPtr<IMFAsyncResult> callerResult;
  RefPtr<IUnknown> unknown;
  HRESULT hr = aResult->GetState(byRef(unknown));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  hr = unknown->QueryInterface(static_cast<IMFAsyncResult**>(byRef(callerResult)));
  NS_ENSURE_TRUE(SUCCEEDED(hr), E_FAIL);

  
  hr = callerResult->GetObject(byRef(unknown));
  NS_ENSURE_TRUE(SUCCEEDED(hr) && unknown, hr);
  AsyncReadRequestState* requestState =
    static_cast<AsyncReadRequestState*>(unknown.get());

  
  
  if (mResource->Tell() != requestState->mOffset) {
    nsresult rv = mResource->Seek(nsISeekableStream::NS_SEEK_SET,
                                  requestState->mOffset);
    if (NS_FAILED(rv)) {
      
      callerResult->SetStatus(E_FAIL);
      wmf::MFInvokeCallback(callerResult);
      LOG("WMFByteStream::Invoke() seek to read offset failed, aborting read");
      return S_OK;
    }
  }
  NS_ASSERTION(mResource->Tell() == requestState->mOffset, "State mismatch!");

  
  ULONG totalBytesRead = 0;
  nsresult rv = NS_OK;
  while (totalBytesRead < requestState->mBufferLength) {
    BYTE* buffer = requestState->mBuffer + totalBytesRead;
    ULONG bytesRead = 0;
    ULONG length = requestState->mBufferLength - totalBytesRead;
    rv = mResource->Read(reinterpret_cast<char*>(buffer),
                         length,
                         reinterpret_cast<uint32_t*>(&bytesRead));
    totalBytesRead += bytesRead;
    if (NS_FAILED(rv) || bytesRead == 0) {
      break;
    }
  }

  
  
  requestState->mBytesRead = NS_SUCCEEDED(rv) ? totalBytesRead : 0;
  callerResult->SetStatus(S_OK);

  LOG("WMFByteStream::Invoke() read %d at %lld finished rv=%x",
       requestState->mBytesRead, requestState->mOffset, rv);

  
  wmf::MFInvokeCallback(callerResult);

  return S_OK;
}

STDMETHODIMP
WMFByteStream::BeginWrite(const BYTE *, ULONG ,
                          IMFAsyncCallback *,
                          IUnknown *)
{
  LOG("WMFByteStream::BeginWrite()");
  return E_NOTIMPL;
}

STDMETHODIMP
WMFByteStream::Close()
{
  LOG("WMFByteStream::Close()");
  return S_OK;
}

STDMETHODIMP
WMFByteStream::EndRead(IMFAsyncResult* aResult, ULONG *aBytesRead)
{
  NS_ENSURE_TRUE(aResult, E_POINTER);
  NS_ENSURE_TRUE(aBytesRead, E_POINTER);

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  
  RefPtr<IUnknown> unknown;
  HRESULT hr = aResult->GetObject(byRef(unknown));
  if (FAILED(hr) || !unknown) {
    return E_INVALIDARG;
  }
  AsyncReadRequestState* requestState =
    static_cast<AsyncReadRequestState*>(unknown.get());

  
  
  
  
  
  
  if (mOffset == requestState->mOffset) {
    mOffset += requestState->mBytesRead;
  }

  
  *aBytesRead = requestState->mBytesRead;

  LOG("WMFByteStream::EndRead() offset=%lld *aBytesRead=%u mOffset=%lld hr=0x%x eof=%d",
      requestState->mOffset, *aBytesRead, mOffset, hr, (mOffset == mResource->GetLength()));

  return S_OK;
}

STDMETHODIMP
WMFByteStream::EndWrite(IMFAsyncResult *, ULONG *)
{
  LOG("WMFByteStream::EndWrite()");
  return E_NOTIMPL;
}

STDMETHODIMP
WMFByteStream::Flush()
{
  LOG("WMFByteStream::Flush()");
  return S_OK;
}

STDMETHODIMP
WMFByteStream::GetCapabilities(DWORD *aCapabilities)
{
  LOG("WMFByteStream::GetCapabilities()");
  NS_ENSURE_TRUE(aCapabilities, E_POINTER);
  *aCapabilities = MFBYTESTREAM_IS_READABLE |
                   MFBYTESTREAM_IS_SEEKABLE;
  return S_OK;
}

STDMETHODIMP
WMFByteStream::GetCurrentPosition(QWORD *aPosition)
{
  NS_ENSURE_TRUE(aPosition, E_POINTER);
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  *aPosition = mOffset;
  LOG("WMFByteStream::GetCurrentPosition() %lld", mOffset);
  return S_OK;
}

STDMETHODIMP
WMFByteStream::GetLength(QWORD *aLength)
{
  NS_ENSURE_TRUE(aLength, E_POINTER);
  int64_t length = mResource->GetLength();
  *aLength = length;
  LOG("WMFByteStream::GetLength() %lld", length);
  return S_OK;
}

STDMETHODIMP
WMFByteStream::IsEndOfStream(BOOL *aEndOfStream)
{
  NS_ENSURE_TRUE(aEndOfStream, E_POINTER);
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  *aEndOfStream = (mOffset == mResource->GetLength());
  LOG("WMFByteStream::IsEndOfStream() %d", *aEndOfStream);
  return S_OK;
}

STDMETHODIMP
WMFByteStream::Read(BYTE*, ULONG, ULONG*)
{
  LOG("WMFByteStream::Read()");
  return E_NOTIMPL;
}

STDMETHODIMP
WMFByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN aSeekOrigin,
                    LONGLONG aSeekOffset,
                    DWORD aSeekFlags,
                    QWORD *aCurrentPosition)
{
  LOG("WMFByteStream::Seek(%d, %lld)", aSeekOrigin, aSeekOffset);

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (aSeekOrigin == msoBegin) {
    mOffset = aSeekOffset;
  } else {
    mOffset += aSeekOffset;
  }

  if (aCurrentPosition) {
    *aCurrentPosition = mOffset;
  }

  return S_OK;
}

STDMETHODIMP
WMFByteStream::SetCurrentPosition(QWORD aPosition)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  LOG("WMFByteStream::SetCurrentPosition(%lld)",
      aPosition);

  
  
  

  int64_t length = mResource->GetLength();
  if (length >= 0 && aPosition > uint64_t(length)) {
    
    
    
    
    
    LOG("WMFByteStream::SetCurrentPosition(%lld) clamping position to eos (%lld)", aPosition, length);
    aPosition = length;
  }
  mOffset = aPosition;

  return S_OK;
}

STDMETHODIMP
WMFByteStream::SetLength(QWORD)
{
  LOG("WMFByteStream::SetLength()");
  return E_NOTIMPL;
}

STDMETHODIMP
WMFByteStream::Write(const BYTE *, ULONG, ULONG *)
{
  LOG("WMFByteStream::Write()");
  return E_NOTIMPL;
}

STDMETHODIMP
WMFByteStream::GetParameters(DWORD*, DWORD*)
{
  LOG("WMFByteStream::GetParameters()");
  return E_NOTIMPL;
}

} 
