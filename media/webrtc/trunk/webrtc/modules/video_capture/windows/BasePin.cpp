





#include <assert.h>
#include "nsAutoPtr.h"
#include "BasePin.h"

namespace mozilla {
namespace media {








class DECLSPEC_UUID("4de7a03c-6c3f-4314-949a-ee7e1ad05083")
  EnumMediaTypes
    : public IEnumMediaTypes
{
public:

  EnumMediaTypes(BasePin* aPin)
    : mPin(aPin)
    , mIndex(0)
    , mRefCnt(0)
  {
  }

  EnumMediaTypes(EnumMediaTypes* aEnum)
    : mPin(aEnum->mPin)
    , mIndex(aEnum->mIndex)
    , mRefCnt(0)
  {
  }

  STDMETHODIMP QueryInterface(REFIID aIId, void **aInterface)
  {
    if (!aInterface) {
      return E_POINTER;
    }

    if (aIId == IID_IEnumMediaTypes) {
      *aInterface = static_cast<IEnumMediaTypes*>(this);
      AddRef();
      return S_OK;
    }

    return E_NOINTERFACE;
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

  
  STDMETHODIMP Next(ULONG aCount,
                    AM_MEDIA_TYPE ** aMediaTypes,
                    ULONG * aNumFetched)
  {
    if (!aMediaTypes)
      return E_POINTER;
  
    if (aNumFetched) {
      *aNumFetched = 0;
    } else if (aCount > 1) {
      
      return E_INVALIDARG;
    }
  
    unsigned int numFetched = 0;
    while (numFetched < aCount) {
      
      MediaType mediaType;
      HRESULT hr = mPin->GetMediaType(mIndex++, &mediaType);
      if (hr != S_OK) {
        break;
      }
  
      
      MediaType* m = new MediaType(mediaType);
      if (!m)
        return E_OUTOFMEMORY;
      aMediaTypes[numFetched] = m;
      if (!aMediaTypes[numFetched])
        break;
  
      numFetched++;
    }
  
    if (aNumFetched)
      *aNumFetched = numFetched;
  
    return (numFetched == aCount) ? S_OK : S_FALSE;
  }

  STDMETHODIMP Skip(ULONG aCount)
  {
    if (aCount == 0)
      return S_OK;
  
    
    mIndex += aCount;
  
    
    MediaType mediaType;
    HRESULT hr = mPin->GetMediaType(mIndex-1, &mediaType);
  
    
    mediaType.Forget();
  
    return (hr != S_OK) ? S_OK : S_FALSE;
  }
  
  STDMETHODIMP Reset()
  {
    mIndex = 0;
    return S_OK;
  }

  STDMETHODIMP Clone(IEnumMediaTypes **aClone)
  {
    if (!aClone)
      return E_POINTER;
  
    EnumMediaTypes* e = new EnumMediaTypes(this);
    if (!e)
      return E_OUTOFMEMORY;
  
    e->AddRef();
  
    *aClone = static_cast<IEnumMediaTypes*>(e);
  
    return S_OK;
  }

private:

  
  BasePinPtr mPin;

  
  int mIndex;

  unsigned long mRefCnt;

};
_COM_SMARTPTR_TYPEDEF(IEnumMediaTypes, __uuidof(IEnumMediaTypes));

BasePin::BasePin(BaseFilter* aFilter,
                 CriticalSection* aLock,
                 const wchar_t* aName,
                 PIN_DIRECTION aDirection)
  : mFilter(aFilter)
  , mLock(aLock)
  , mName(aName)
  , mDirection(aDirection)
  , mQualitySink(NULL)
{
  assert(aFilter != NULL);
  assert(aLock != NULL);
}


STDMETHODIMP
BasePin::QueryInterface(REFIID aIID, void ** aInterface)
{
  if (!aInterface) {
    return E_POINTER;
  }

  if (aIID == IID_IPin || aIID == IID_IUnknown) {
    *aInterface = static_cast<IPin*>(this);
  } else if (aIID == IID_IQualityControl) {
    *aInterface = static_cast<IQualityControl*>(this);
  } else {
    *aInterface = NULL;
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}


STDMETHODIMP
BasePin::QueryPinInfo(PIN_INFO * aInfo)
{
  if (!aInfo)
    return E_POINTER;

  memset(aInfo, 0, sizeof(PIN_INFO));

  aInfo->pFilter = mFilter;
  if (mFilter) {
    mFilter->AddRef();
  }

  if (!mName.empty()) {
    
    
    unsigned int len = PR_MIN((MAX_PIN_NAME-1)*sizeof(WCHAR), (sizeof(WCHAR)*mName.length()));
    memcpy(aInfo->achName, mName.data(), len);
  }

  aInfo->dir = mDirection;

  return NOERROR;
}


STDMETHODIMP
BasePin::QueryDirection(PIN_DIRECTION * aPinDir)
{
  if (!aPinDir)
    return E_POINTER;
  *aPinDir = mDirection;
  return S_OK;
}


STDMETHODIMP
BasePin::QueryId(LPWSTR * aId)
{
  if (!aId)
    return E_POINTER;

  *aId = NULL;

  WCHAR* str = NULL;

  
  unsigned int sz = mName.length() * sizeof(WCHAR); 

  
  str = (LPWSTR)CoTaskMemAlloc(sz + sizeof(WCHAR));
  if (!str)
    return E_OUTOFMEMORY;

  
  memcpy(str, mName.data(), sz);

  
  str[mName.length()] = 0;

  *aId = str;

  return S_OK;
}


STDMETHODIMP
BasePin::EnumMediaTypes(IEnumMediaTypes **aEnum)
{
  if (!aEnum)
    return E_POINTER;

  *aEnum = new mozilla::media::EnumMediaTypes(this);

  if (*aEnum == NULL)
    return E_OUTOFMEMORY;

  
  NS_ADDREF(*aEnum);

  return S_OK;
}



HRESULT
BasePin::GetMediaType(int, MediaType*)
{
  return E_UNEXPECTED;
}


STDMETHODIMP
BasePin::QueryAccept(const AM_MEDIA_TYPE *aMediaType)
{
  if (!aMediaType)
    return E_POINTER;

  
  

  if (FAILED(CheckMediaType((MediaType*)aMediaType)))
    return S_FALSE;

  return S_OK;
}


HRESULT
BasePin::Active(void)
{
  return S_OK;
}


HRESULT
BasePin::Run(REFERENCE_TIME)
{
  return S_OK;
}


HRESULT
BasePin::Inactive(void)
{
  return S_OK;
}


STDMETHODIMP
BasePin::EndOfStream(void)
{
  return S_OK;
}


STDMETHODIMP
BasePin::SetSink(IQualityControl* aQualitySink)
{
  CriticalSectionAutoEnter monitor(*mLock);
  mQualitySink = aQualitySink;
  return S_OK;
}


STDMETHODIMP
BasePin::Notify(IBaseFilter *, Quality)
{
  return E_NOTIMPL;
}


STDMETHODIMP
BasePin::NewSegment(REFERENCE_TIME aStartTime,
                      REFERENCE_TIME aStopTime,
                      double aRate)
{
  return E_NOTIMPL;
}







STDMETHODIMP
BasePin::Connect(IPin * aPin,
                   const AM_MEDIA_TYPE* aMediaType)
{
  if (!aPin)
    return E_POINTER;

  CriticalSectionAutoEnter monitor(*mLock);

  if (IsConnected())
    return VFW_E_ALREADY_CONNECTED;

  
  if (!IsStopped())
    return VFW_E_NOT_STOPPED;

  
  

  const MediaType* mediaType = reinterpret_cast<const MediaType*>(aMediaType);

  if (aMediaType && !mediaType->IsPartiallySpecified()) {
    
    
    return AttemptConnection(aPin, mediaType);
  }

  
  IEnumMediaTypesPtr enumMediaTypes;
  HRESULT hr = EnumMediaTypes(&enumMediaTypes);
  assert(SUCCEEDED(hr));
  if (enumMediaTypes) {
    hr = TryMediaTypes(aPin, mediaType, enumMediaTypes);
    if (SUCCEEDED(hr))
      return S_OK;
  }

  
  enumMediaTypes = NULL;
  hr = aPin->EnumMediaTypes(&enumMediaTypes);
  assert(SUCCEEDED(hr));
  if (enumMediaTypes) {
    hr = TryMediaTypes(aPin, mediaType, enumMediaTypes);
    if (SUCCEEDED(hr))
      return S_OK;
  }

  
  return VFW_E_NO_ACCEPTABLE_TYPES;
}





HRESULT
BasePin::TryMediaTypes(IPin *aPin,
                         const MediaType *aFilter,
                         IEnumMediaTypes *aEnum)
{
  
  HRESULT hr = aEnum->Reset();
  if (FAILED(hr))
    return hr;

  while (true) {
    AM_MEDIA_TYPE* amt = NULL;
    ULONG mediaCount = 0;
    HRESULT hr = aEnum->Next(1, &amt, &mediaCount);
    if (hr != S_OK) 
      return VFW_E_NO_ACCEPTABLE_TYPES;

    assert(mediaCount == 1 && hr == S_OK);

    MediaType* mediaType = reinterpret_cast<MediaType*>(amt);
    if (!aFilter || mediaType->MatchesPartial(aFilter)) {
      
      
      if (SUCCEEDED(AttemptConnection(aPin, mediaType)))
        return S_OK;
    }
  }
}




HRESULT
BasePin::CheckConnect(IPin* aPin)
{
  PIN_DIRECTION otherPinsDirection;
  aPin->QueryDirection(&otherPinsDirection);

  
  if (otherPinsDirection == mDirection) {
    return VFW_E_INVALID_DIRECTION;
  }

  return S_OK;
}





HRESULT
BasePin::CompleteConnect(IPin *)
{
  return S_OK;
}



HRESULT
BasePin::SetMediaType(const MediaType *aMediaType)
{
  return mMediaType.Assign(aMediaType);
}


STDMETHODIMP
BasePin::QueryInternalConnections(IPin**, ULONG*)
{
  return E_NOTIMPL;
}



HRESULT
BasePin::BreakConnect()
{
  return S_OK;
}


STDMETHODIMP
BasePin::Disconnect()
{
  CriticalSectionAutoEnter monitor(*mLock);

  
  if (!IsStopped())
    return VFW_E_NOT_STOPPED;

  if (!IsConnected())
     return S_FALSE;

  HRESULT hr = BreakConnect();
  mConnectedPin = NULL;
  return hr;
}



HRESULT
BasePin::AttemptConnection(IPin* aPin,
                             const MediaType* aMediaType)
{
  CriticalSectionAutoEnter monitor(*mLock);

  
  
  HRESULT hr = CheckConnect(aPin);
  if (FAILED(hr)) {
    BreakConnect();
    return hr;
  }

  
  
  hr = CheckMediaType(aMediaType);
  if (FAILED(hr))
    return hr;

  hr = SetMediaType(aMediaType);
  if (FAILED(hr))
    return hr;

  
  hr = aPin->ReceiveConnection(static_cast<IPin*>(this), aMediaType);
  if (FAILED(hr))
    return hr;

  
  mConnectedPin = aPin;
  hr = CompleteConnect(aPin);

  if (FAILED(hr)) {
    
    
    aPin->Disconnect();
    BreakConnect();
    mConnectedPin = NULL;
    mMediaType.Clear();
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  
  return S_OK;
}


STDMETHODIMP
BasePin::ReceiveConnection(IPin * aPin,
                             const AM_MEDIA_TYPE *aMediaType)
{
  if (!aPin)
    return E_POINTER;

  if (!aMediaType)
    E_POINTER;

  CriticalSectionAutoEnter monitor(*mLock);

  if (IsConnected())
    return VFW_E_ALREADY_CONNECTED;

  if (!IsStopped())
    return VFW_E_NOT_STOPPED;

  HRESULT hr = CheckConnect(aPin);
  if (FAILED(hr)) {
    BreakConnect();
    return hr;
  }

  
  const MediaType* mediaType = reinterpret_cast<const MediaType*>(aMediaType);
  hr = CheckMediaType(mediaType);
  if (FAILED(hr)) {
    BreakConnect();
    return hr;
  }

  
  hr = SetMediaType(mediaType);
  if (FAILED(hr))
    return hr;

  
  mConnectedPin = aPin;
  
  hr = CompleteConnect(aPin);
  if (FAILED(hr)) {
    
    mConnectedPin = NULL;
    BreakConnect();
    return hr;
  }

  
  return S_OK;
}


STDMETHODIMP
BasePin::ConnectedTo(IPin** aPin)
{
  if (!aPin)
    return E_POINTER;

  if (!IsConnected())
    return VFW_E_NOT_CONNECTED;

  *aPin = mConnectedPin;
  (*aPin)->AddRef();

  return S_OK;
}


STDMETHODIMP
BasePin::ConnectionMediaType(AM_MEDIA_TYPE *aMediaType)
{
  if (!aMediaType)
    return E_POINTER;

  CriticalSectionAutoEnter monitor(*mLock);

  if (IsConnected()) {
    reinterpret_cast<MediaType*>(aMediaType)->Assign(&mMediaType);
    return S_OK;
  } else {
    memset(aMediaType, 0, sizeof(AM_MEDIA_TYPE));
    return VFW_E_NOT_CONNECTED;
  }
}

}
}
