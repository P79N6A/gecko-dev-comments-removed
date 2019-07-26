









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_SINK_FILTER_DS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_SINK_FILTER_DS_H_

#include "webrtc/modules/video_capture/include/video_capture_defines.h"
#include "BaseInputPin.h"
#include "BaseFilter.h"
#include "MediaType.h"

namespace webrtc
{
namespace videocapturemodule
{


class CaptureSinkFilter;




class CaptureInputPin: public mozilla::media::BaseInputPin
{
public:
    int32_t _moduleId;

    VideoCaptureCapability _requestedCapability;
    VideoCaptureCapability _resultingCapability;
    HANDLE _threadHandle;

    CaptureInputPin(int32_t moduleId,
                    IN TCHAR* szName,
                    IN CaptureSinkFilter* pFilter,
                    IN mozilla::CriticalSection * pLock,
                    OUT HRESULT * pHr,
                    IN LPCWSTR pszName);
    virtual ~CaptureInputPin();

    HRESULT GetMediaType (IN int iPos, OUT mozilla::media::MediaType * pmt);
    HRESULT CheckMediaType (IN const mozilla::media::MediaType * pmt);
    STDMETHODIMP Receive (IN IMediaSample *);
    HRESULT SetMatchingMediaType(const VideoCaptureCapability& capability);
};

class CaptureSinkFilter: public mozilla::media::BaseFilter
{

public:
    CaptureSinkFilter(IN TCHAR * tszName,
                      IN LPUNKNOWN punk,
                      OUT HRESULT * phr,
                      VideoCaptureExternal& captureObserver,
                      int32_t moduleId);
    virtual ~CaptureSinkFilter();

    
    

    void ProcessCapturedFrame(unsigned char* pBuffer, int32_t length,
                              const VideoCaptureCapability& frameInfo);
    
    void LockReceive()  { m_crtRecv.Enter();}
    void UnlockReceive() {m_crtRecv.Leave();}

    
    void LockFilter() {m_crtFilter.Enter();}
    void UnlockFilter() { m_crtFilter.Leave(); }
    void SetFilterGraph(IGraphBuilder* graph); 

    
    
    STDMETHODIMP QueryInterface(REFIID aIId, void **aInterface)
    {
      return mozilla::media::BaseFilter::QueryInterface(aIId, aInterface);
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
      return ::InterlockedIncrement(&mRefCnt);
    }

    STDMETHODIMP_(ULONG) Release()
    {
      unsigned long newRefCnt = ::InterlockedDecrement(&mRefCnt);

      if (!newRefCnt) {
        delete this;
      }

      return newRefCnt;
    }

    STDMETHODIMP SetMatchingMediaType(const VideoCaptureCapability& capability);

    
    
    int GetPinCount ();
    mozilla::media::BasePin * GetPin ( IN int Index);
    STDMETHODIMP Pause ();
    STDMETHODIMP Stop ();
    STDMETHODIMP GetClassID ( OUT CLSID * pCLSID);
    
    
    static IUnknown * CreateInstance (IN LPUNKNOWN punk, OUT HRESULT * phr);
private:
    mozilla::CriticalSection m_crtFilter; 
    mozilla::CriticalSection m_crtRecv;  
    CaptureInputPin * m_pInput;
    VideoCaptureExternal& _captureObserver;
    int32_t _moduleId;
    unsigned long mRefCnt;
};
}  
}  
#endif 
