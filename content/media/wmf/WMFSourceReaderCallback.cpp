





#include "WMFSourceReaderCallback.h"
#include "WMFUtils.h"

namespace mozilla {

#ifdef PR_LOGGING
static PRLogModuleInfo* gWMFSourceReaderCallbackLog = nullptr;
#define LOG(...) PR_LOG(gWMFSourceReaderCallbackLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif


STDMETHODIMP
WMFSourceReaderCallback::QueryInterface(REFIID aIId, void **aInterface)
{
  LOG("WMFSourceReaderCallback::QueryInterface %s", GetGUIDName(aIId).get());

  if (aIId == IID_IMFSourceReaderCallback) {
    return DoGetInterface(static_cast<WMFSourceReaderCallback*>(this), aInterface);
  }
  if (aIId == IID_IUnknown) {
    return DoGetInterface(static_cast<WMFSourceReaderCallback*>(this), aInterface);
  }

  *aInterface = NULL;
  return E_NOINTERFACE;
}

NS_IMPL_THREADSAFE_ADDREF(WMFSourceReaderCallback)
NS_IMPL_THREADSAFE_RELEASE(WMFSourceReaderCallback)

WMFSourceReaderCallback::WMFSourceReaderCallback()
  : mResultStatus(S_OK)
  , mStreamFlags(0)
  , mTimestamp(0)
  , mSample(nullptr)
  , mReadFinished(false)
  , mMonitor("WMFSourceReaderCallback")
{
#ifdef PR_LOGGING
  if (!gWMFSourceReaderCallbackLog) {
    gWMFSourceReaderCallbackLog = PR_NewLogModule("WMFSourceReaderCallback");
  }
#endif
}

HRESULT
WMFSourceReaderCallback::NotifyReadComplete(HRESULT aReadStatus,
                                            DWORD aStreamIndex,
                                            DWORD aStreamFlags,
                                            LONGLONG aTimestamp,
                                            IMFSample *aSample)
{
  
  ReentrantMonitorAutoEnter mon(mMonitor);

  if (mSample) {
    
    
    
    
    mSample->Release();
    mSample = nullptr;
  }

  if (SUCCEEDED(aReadStatus)) {
    if (aSample) {
      mTimestamp = aTimestamp;
      mSample = aSample;
      mSample->AddRef();
    }
  }

  mResultStatus = aReadStatus;
  mStreamFlags = aStreamFlags;

  
  
  mReadFinished = true;
  mon.NotifyAll();

  return S_OK;
}

STDMETHODIMP
WMFSourceReaderCallback::OnReadSample(HRESULT aReadStatus,
                                      DWORD aStreamIndex,
                                      DWORD aStreamFlags,
                                      LONGLONG aTimestamp,
                                      IMFSample *aSample)
{
  LOG("WMFSourceReaderCallback::OnReadSample() hr=0x%x flags=0x%x time=%lld sample=%p",
      aReadStatus, aStreamFlags, aTimestamp, aSample);
  return NotifyReadComplete(aReadStatus,
                            aStreamIndex,
                            aStreamFlags,
                            aTimestamp,
                            aSample);
}

HRESULT
WMFSourceReaderCallback::Cancel()
{
  LOG("WMFSourceReaderCallback::Cancel()");
  return NotifyReadComplete(E_ABORT,
                            0,
                            0,
                            0,
                            nullptr);
}

STDMETHODIMP
WMFSourceReaderCallback::OnEvent(DWORD, IMFMediaEvent *)
{
  return S_OK;
}

STDMETHODIMP
WMFSourceReaderCallback::OnFlush(DWORD)
{
  return S_OK;
}

HRESULT
WMFSourceReaderCallback::Wait(DWORD* aStreamFlags,
                              LONGLONG* aTimeStamp,
                              IMFSample** aSample)
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  LOG("WMFSourceReaderCallback::Wait() starting wait");
  while (!mReadFinished) {
    mon.Wait();
  }
  mReadFinished = false;
  LOG("WMFSourceReaderCallback::Wait() done waiting");

  *aStreamFlags = mStreamFlags;
  *aTimeStamp = mTimestamp;
  *aSample = mSample;
  HRESULT hr = mResultStatus;

  mSample = nullptr;
  mTimestamp = 0;
  mStreamFlags = 0;
  mResultStatus = S_OK;

  return hr;
}

} 
