





#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_BASEFILTER_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_BASEFILTER_H_

#include <assert.h>
#include "dshow.h"
#include <comdef.h>
#include "DShowTools.h"

#include <string> 
 
namespace mozilla {
namespace media {

_COM_SMARTPTR_TYPEDEF(IReferenceClock, __uuidof(IReferenceClock));
 
class BasePin;









class DECLSPEC_UUID("4debd354-b0c6-44ab-93cf-49f64ed36ab8")
BaseFilter : public IBaseFilter
{
  friend class BasePin;
public:
  
  
  
  
  static const unsigned int sMaxNumBuffers = 256;

  
  void* operator new(size_t sz) throw() {
    void* rv = ::operator new(sz);
    if (rv) {
      memset(rv, 0, sz);
    }
    return rv;
  }
  void operator delete(void* ptr) {
    ::operator delete(ptr);
  }

  
  
  BaseFilter(const wchar_t* aName, REFCLSID aClsID);

  ~BaseFilter() {}

  STDMETHODIMP QueryInterface(REFIID aIId, void **aInterface);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  
  STDMETHODIMP GetClassID(CLSID* aClsID);

  

  
  STDMETHODIMP GetState(DWORD aTimeout, FILTER_STATE* aState);

  
  STDMETHODIMP SetSyncSource(IReferenceClock* aClock);

  
  STDMETHODIMP GetSyncSource(IReferenceClock** aClock);

  
  STDMETHODIMP Stop();

  
  STDMETHODIMP Pause();

  
  STDMETHODIMP Run(REFERENCE_TIME aStartTime);

  

  
  STDMETHODIMP EnumPins(IEnumPins** aEnum);

  
  STDMETHODIMP FindPin(LPCWSTR aId, IPin** aPin);

  
  STDMETHODIMP QueryFilterInfo(FILTER_INFO* aInfo);

  
  STDMETHODIMP JoinFilterGraph(IFilterGraph* aGraph, LPCWSTR aName);

  
  
  STDMETHODIMP QueryVendorInfo(LPWSTR* aVendorInfo) { return E_NOTIMPL; }

  
  HRESULT NotifyEvent(long aEventCode,
  LONG_PTR aEventParam1,
  LONG_PTR aEventParam2);

  
  
  virtual int GetPinCount() = 0;

  
  virtual BasePin* GetPin(int n) = 0;

  
  virtual void AboutToPause() { }

protected:
  
  FILTER_STATE mState;

  
  
  IReferenceClockPtr mClock;

  
  REFERENCE_TIME mStartTime;

  
  CLSID mClsId;

  
  CriticalSection mLock;

  
  
  
  
  IFilterGraph* mGraph;

  
  
  IMediaEventSink* mEventSink;

  
  std::wstring mName;

  
  unsigned long mRefCnt;
};

_COM_SMARTPTR_TYPEDEF(BaseFilter, __uuidof(BaseFilter));

}
}

#endif
