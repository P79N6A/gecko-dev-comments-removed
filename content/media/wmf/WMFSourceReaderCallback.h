




#if !defined(WMFSourceReaderCallback_h_)
#define WMFSourceReaderCallback_h_

#include "WMF.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "nsISupportsImpl.h"

namespace mozilla {







class WMFSourceReaderCallback : public IMFSourceReaderCallback
{
public:
  WMFSourceReaderCallback();

  
  STDMETHODIMP QueryInterface(REFIID aIId, LPVOID *aInterface);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  
  STDMETHODIMP OnReadSample(HRESULT hrStatus,
                            DWORD dwStreamIndex,
                            DWORD dwStreamFlags,
                            LONGLONG llTimestamp,
                            IMFSample *pSample);
  STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *);
  STDMETHODIMP OnFlush(DWORD);

  
  
  
  HRESULT Wait(DWORD* aStreamFlags,
               LONGLONG* aTimeStamp,
               IMFSample** aSample);

  
  HRESULT Cancel();

private:

  
  
  HRESULT NotifyReadComplete(HRESULT aReadStatus,
                             DWORD aStreamIndex,
                             DWORD aStreamFlags,
                             LONGLONG aTimestamp,
                             IMFSample *aSample);

  
  
  ReentrantMonitor mMonitor;

  
  HRESULT mResultStatus;
  DWORD mStreamFlags;
  LONGLONG mTimestamp;
  IMFSample* mSample;

  
  
  bool mReadFinished;

  
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

};

} 

#endif 