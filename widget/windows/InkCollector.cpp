






#include "InkCollector.h"



#include <msinkaut_i.c>

StaticRefPtr<InkCollector> InkCollector::sInkCollector;

InkCollector::InkCollector()
{
}

InkCollector::~InkCollector()
{
  Shutdown();
  MOZ_ASSERT(!mRefCount);
}

void InkCollector::Initialize()
{
  
  
  
  static bool sInkCollectorCreated = false;
  if (sInkCollectorCreated) {
    return;
  }
  sInkCollectorCreated = true;

  
  mComInitialized = SUCCEEDED(::CoInitialize(nullptr));

  
  if (FAILED(::CoCreateFreeThreadedMarshaler(this, getter_AddRefs(mMarshaller)))) {
    return;
  }
  
  if (FAILED(::CoCreateInstance(CLSID_InkCollector, NULL, CLSCTX_INPROC_SERVER,
                                IID_IInkCollector, getter_AddRefs(mInkCollector)))) {
    return;
  }
  NS_ADDREF(mInkCollector);
  
  nsRefPtr<IConnectionPointContainer> connPointContainer;
  
  if (SUCCEEDED(mInkCollector->QueryInterface(IID_IConnectionPointContainer,
                                              getter_AddRefs(connPointContainer)))) {
    
    if (SUCCEEDED(connPointContainer->FindConnectionPoint(__uuidof(_IInkCollectorEvents),
                                                          getter_AddRefs(mConnectionPoint)))) {
      NS_ADDREF(mConnectionPoint);
      
      if (SUCCEEDED(mConnectionPoint->Advise(this, &mCookie))) {
        OnInitialize();
      }
    }
  }
}

void InkCollector::Shutdown()
{
  Enable(false);
  if (mConnectionPoint) {
    
    mConnectionPoint->Unadvise(mCookie);
    NS_RELEASE(mConnectionPoint);
  }
  NS_IF_RELEASE(mMarshaller);
  NS_IF_RELEASE(mInkCollector);

  
  if (mComInitialized) {
    CoUninitialize();
    mComInitialized = false;
  }
}

void InkCollector::OnInitialize()
{
  
  
  mInkCollector->SetEventInterest(InkCollectorEventInterest::ICEI_AllEvents, VARIANT_FALSE);

  
  mInkCollector->SetEventInterest(InkCollectorEventInterest::ICEI_CursorOutOfRange, VARIANT_TRUE);

  
  
  
  mInkCollector->put_MouseIcon(nullptr);
  mInkCollector->put_MousePointer(InkMousePointer::IMP_Custom);

  
  
  
  
  mInkCollector->SetAllTabletsMode(VARIANT_FALSE);

  
  
  
  
  mInkCollector->put_DynamicRendering(VARIANT_FALSE);
}





void InkCollector::Enable(bool aNewState)
{
  if (aNewState != mEnabled) {
    if (mInkCollector) {
      if (S_OK == mInkCollector->put_Enabled(aNewState ? VARIANT_TRUE : VARIANT_FALSE)) {
        mEnabled = aNewState;
      } else {
        NS_WARNING("InkCollector did not change status successfully");
      }
    } else {
      NS_WARNING("InkCollector should be exist");
    }
  }
}

void InkCollector::SetTarget(HWND aTargetWindow)
{
  NS_ASSERTION(aTargetWindow, "aTargetWindow should be exist");
  if (aTargetWindow && (aTargetWindow != mTargetWindow)) {
    Initialize();
    if (mInkCollector) {
      Enable(false);
      if (S_OK == mInkCollector->put_hWnd((LONG_PTR)aTargetWindow)) {
        mTargetWindow = aTargetWindow;
      } else {
        NS_WARNING("InkCollector did not change window property successfully");
      }
      Enable(true);
    }
  }
}






bool InkCollector::IsHardProximityTablet(IInkTablet* aTablet) const
{
  if (aTablet) {
    TabletHardwareCapabilities caps;
    if (SUCCEEDED(aTablet->get_HardwareCapabilities(&caps))) {
      return (TabletHardwareCapabilities::THWC_HardProximity & caps);
    }
  }
  return false;
}

HRESULT __stdcall InkCollector::QueryInterface(REFIID aRiid, void **aObject)
{
  
  if (!aObject) {
    return E_POINTER;
  }
  HRESULT result = E_NOINTERFACE;
  
  if ((IID_IUnknown == aRiid) ||
      (IID_IDispatch == aRiid) ||
      (DIID__IInkCollectorEvents == aRiid)) {
    *aObject = this;
    
    NS_ADDREF_THIS();
    result = S_OK;
  } else if (IID_IMarshal == aRiid) {
    
    
    NS_ASSERTION(mMarshaller, "Free threaded marshaller is null!");
    
    result = mMarshaller->QueryInterface(aRiid, aObject);
  }
  return result;
}

HRESULT InkCollector::Invoke(DISPID aDispIdMember, REFIID ,
                             LCID , WORD ,
                             DISPPARAMS* aDispParams, VARIANT* ,
                             EXCEPINFO* , UINT* )
{
  switch (aDispIdMember) {
    case DISPID_ICECursorOutOfRange: {
      if (aDispParams && aDispParams->cArgs) {
        CursorOutOfRange(static_cast<IInkCursor*>(aDispParams->rgvarg[0].pdispVal));
      }
      break;
    }
  };
  
  NS_RELEASE_THIS();
  return S_OK;
}

void InkCollector::CursorOutOfRange(IInkCursor* aCursor) const
{
  IInkTablet* curTablet = nullptr;
  if (FAILED(aCursor->get_Tablet(&curTablet))) {
    return;
  }
  
  
  if (!IsHardProximityTablet(curTablet)) {
    return;
  }
  
  if (mTargetWindow) {
    ::PostMessage(mTargetWindow, MOZ_WM_PEN_LEAVES_HOVER_OF_DIGITIZER, 0, 0);
  }
}
