









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_SINK_FILTER_DS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_SINK_FILTER_DS_H_

#include <Streams.h> 

#include "video_capture_defines.h"

namespace webrtc
{
namespace videocapturemodule
{


class CaptureSinkFilter;




class CaptureInputPin: public CBaseInputPin
{
public:
    WebRtc_Word32 _moduleId;

    VideoCaptureCapability _requestedCapability;
    VideoCaptureCapability _resultingCapability;
    HANDLE _threadHandle;

    CaptureInputPin ( WebRtc_Word32 moduleId,
                      IN TCHAR* szName,
                      IN CaptureSinkFilter* pFilter,
                      IN CCritSec * pLock,
                      OUT HRESULT * pHr,
                      IN LPCWSTR pszName);
    virtual ~CaptureInputPin();

    HRESULT GetMediaType (IN int iPos, OUT CMediaType * pmt);
    HRESULT CheckMediaType (IN const CMediaType * pmt);
    STDMETHODIMP Receive (IN IMediaSample *);
    HRESULT SetMatchingMediaType(const VideoCaptureCapability& capability);
};

class CaptureSinkFilter: public CBaseFilter
{

public:
    CaptureSinkFilter (IN TCHAR * tszName,
                   IN LPUNKNOWN punk,
                   OUT HRESULT * phr,
                   VideoCaptureExternal& captureObserver,
                   WebRtc_Word32 moduleId);
    virtual ~CaptureSinkFilter();

    
    

    void ProcessCapturedFrame(unsigned char* pBuffer, WebRtc_Word32 length,
                              const VideoCaptureCapability& frameInfo);
    
    void LockReceive()  { m_crtRecv.Lock();}
    void UnlockReceive() {m_crtRecv.Unlock();}
    
    void LockFilter() {m_crtFilter.Lock();}
    void UnlockFilter() { m_crtFilter.Unlock(); }
    void SetFilterGraph(IGraphBuilder* graph); 

    
    
DECLARE_IUNKNOWN    ;
    STDMETHODIMP SetMatchingMediaType(const VideoCaptureCapability& capability);

    
    
    int GetPinCount ();
    CBasePin * GetPin ( IN int Index);
    STDMETHODIMP Pause ();
    STDMETHODIMP Stop ();
    STDMETHODIMP GetClassID ( OUT CLSID * pCLSID);
    
    
    static CUnknown * CreateInstance (IN LPUNKNOWN punk, OUT HRESULT * phr);
private:
    CCritSec m_crtFilter; 
    CCritSec m_crtRecv;  
    CaptureInputPin * m_pInput;
    VideoCaptureExternal& _captureObserver;
    WebRtc_Word32 _moduleId;
};
} 
} 
#endif 
