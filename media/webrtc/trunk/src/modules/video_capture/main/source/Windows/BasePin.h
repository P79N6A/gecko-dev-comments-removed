





#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_BASEPIN_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_BASEPIN_H_


#include "BaseFilter.h"
#include "MediaType.h"

#include "dshow.h"
#include "strmif.h"
#include <string>

namespace mozilla {
namespace media {

_COM_SMARTPTR_TYPEDEF(IPin, __uuidof(IPin));









class DECLSPEC_UUID("199669c6-672a-4130-b13e-57aa830eae55")
  BasePin
  : public IPin
  , public IQualityControl
{
public:

  BasePin(BaseFilter* aFilter,
          CriticalSection* aLock,
          const wchar_t* aName, 
          PIN_DIRECTION aDirection);

  virtual ~BasePin() {}

  
  
  STDMETHODIMP QueryInterface(REFIID aIId, void **aInterface);
  STDMETHODIMP_(ULONG) AddRef() { return mFilter->AddRef(); }
  STDMETHODIMP_(ULONG) Release() { return mFilter->Release(); }

  

  
  
  STDMETHODIMP Connect(IPin* aReceivePin,
                       const AM_MEDIA_TYPE* aMediaType);

  
  STDMETHODIMP ReceiveConnection(IPin* aConnector,
                                 const AM_MEDIA_TYPE* aMediaType);

  
  STDMETHODIMP Disconnect();

  
  STDMETHODIMP ConnectedTo(IPin** aPin);

  
  STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE* aMediaType);

  
  
  STDMETHODIMP QueryPinInfo(PIN_INFO* aInfo);

  
  STDMETHODIMP QueryDirection(PIN_DIRECTION* aDirection);

  
  STDMETHODIMP QueryId(LPWSTR* Id);

 	
  STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* aMediaType);

  
  STDMETHODIMP EnumMediaTypes(IEnumMediaTypes** aEnum);

  
  
  STDMETHODIMP QueryInternalConnections(IPin** apPin,
                                        ULONG* aPin);

  
  STDMETHODIMP EndOfStream(void);

  
  

  
  
  STDMETHODIMP NewSegment(
    REFERENCE_TIME aStartTime,
    REFERENCE_TIME aStopTime,
    double aRate);



  
 
  
  STDMETHODIMP Notify(IBaseFilter * aSender, Quality aQuality);

  
  STDMETHODIMP SetSink(IQualityControl* aQualitySink);



  

  
  virtual HRESULT SetMediaType(const MediaType *aMediaType);

  
  virtual HRESULT CheckMediaType(const MediaType *) = 0;

  
  virtual HRESULT BreakConnect();

  
  
  
  virtual HRESULT CompleteConnect(IPin *pReceivePin);

  
  
  
  virtual HRESULT CheckConnect(IPin *);

  
  BOOL IsStopped() {
    return mFilter->mState == State_Stopped;
  };

  
  
  virtual HRESULT Active(void);

  
  
  virtual HRESULT Inactive(void);

  
  
  virtual HRESULT Run(REFERENCE_TIME aStartTime);

  
  virtual HRESULT GetMediaType(int aIndex, MediaType *aMediaType);

  
  const std::wstring& Name() { return mName; };

  bool IsConnected() { return mConnectedPin != NULL; }

  IPin* GetConnected() { return mConnectedPin; }

protected:

  
  std::wstring mName;
  
  
  IQualityControl *mQualitySink;

  
  IPinPtr mConnectedPin;

  
  PIN_DIRECTION mDirection;

  
  MediaType mMediaType;

  
  mozilla::CriticalSection *mLock;

  
  BaseFilter *mFilter;
 
  
  
  
  
  
  HRESULT AttemptConnection(IPin* aPin, const MediaType* aMediaType);

  
  HRESULT TryMediaTypes(IPin *aPin,
                        const MediaType *aMediaType,
                        IEnumMediaTypes *aEnum);
};

_COM_SMARTPTR_TYPEDEF(BasePin, __uuidof(BasePin));

}
}

#endif
