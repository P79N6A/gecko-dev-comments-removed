






#include "InkCollector.h"



#include <msinkaut_i.c>

StaticAutoPtr<InkCollector> InkCollector::sInkCollector;

InkCollector::~InkCollector()
{
  Shutdown();
  MOZ_ASSERT(!mCookie && !mEnabled && !mComInitialized
              && !mMarshaller && !mInkCollector
              && !mConnectionPoint && !mInkCollectorEvent);
}

void InkCollector::Initialize()
{
  
  
  
  static bool sInkCollectorCreated = false;
  if (sInkCollectorCreated) {
    return;
  }
  sInkCollectorCreated = true;

  
  mComInitialized = SUCCEEDED(::CoInitialize(nullptr));

  
  mInkCollectorEvent = new InkCollectorEvent();

  
  if (FAILED(::CoCreateFreeThreadedMarshaler(mInkCollectorEvent, getter_AddRefs(mMarshaller)))) {
    return;
  }

  
  if (FAILED(::CoCreateInstance(CLSID_InkCollector, NULL, CLSCTX_INPROC_SERVER,
                                IID_IInkCollector, getter_AddRefs(mInkCollector)))) {
    return;
  }

  
  nsRefPtr<IConnectionPointContainer> connPointContainer;

  
  if (SUCCEEDED(mInkCollector->QueryInterface(IID_IConnectionPointContainer,
                                              getter_AddRefs(connPointContainer)))) {

    
    if (SUCCEEDED(connPointContainer->FindConnectionPoint(__uuidof(_IInkCollectorEvents),
                                                          getter_AddRefs(mConnectionPoint)))) {

      
      if (SUCCEEDED(mConnectionPoint->Advise(mInkCollectorEvent, &mCookie))) {
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
    mCookie = 0;
    mConnectionPoint = nullptr;
  }
  mInkCollector = nullptr;
  mMarshaller = nullptr;
  mInkCollectorEvent = nullptr;

  
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
      if (SUCCEEDED(mInkCollector->put_Enabled(aNewState ? VARIANT_TRUE : VARIANT_FALSE))) {
        mEnabled = aNewState;
      } else {
        NS_WARNING("InkCollector did not change status successfully");
      }
    } else {
      NS_WARNING("InkCollector should be exist");
    }
  }
}

HWND InkCollector::GetTarget()
{
  return mTargetWindow;
}

void InkCollector::SetTarget(HWND aTargetWindow)
{
  NS_ASSERTION(aTargetWindow, "aTargetWindow should be exist");
  if (aTargetWindow && (aTargetWindow != mTargetWindow)) {
    Initialize();
    if (mInkCollector) {
      Enable(false);
      if (SUCCEEDED(mInkCollector->put_hWnd((LONG_PTR)aTargetWindow))) {
        mTargetWindow = aTargetWindow;
      } else {
        NS_WARNING("InkCollector did not change window property successfully");
      }
      Enable(true);
    }
  }
}

void InkCollector::ClearTarget()
{
  if (mTargetWindow && mInkCollector) {
    Enable(false);
    if (SUCCEEDED(mInkCollector->put_hWnd(0))) {
      mTargetWindow = 0;
    } else {
      NS_WARNING("InkCollector did not clear window property successfully");
    }
  }
}






bool InkCollectorEvent::IsHardProximityTablet(IInkTablet* aTablet) const
{
  if (aTablet) {
    TabletHardwareCapabilities caps;
    if (SUCCEEDED(aTablet->get_HardwareCapabilities(&caps))) {
      return (TabletHardwareCapabilities::THWC_HardProximity & caps);
    }
  }
  return false;
}

HRESULT __stdcall InkCollectorEvent::QueryInterface(REFIID aRiid, void **aObject)
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
  }
  return result;
}

HRESULT InkCollectorEvent::Invoke(DISPID aDispIdMember, REFIID ,
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
  return S_OK;
}

void InkCollectorEvent::CursorOutOfRange(IInkCursor* aCursor) const
{
  IInkTablet* curTablet = nullptr;
  if (FAILED(aCursor->get_Tablet(&curTablet))) {
    return;
  }
  
  
  if (!IsHardProximityTablet(curTablet)) {
    return;
  }
  
  if (HWND targetWindow = InkCollector::sInkCollector->GetTarget()) {
    ::SendMessage(targetWindow, MOZ_WM_PEN_LEAVES_HOVER_OF_DIGITIZER, 0, 0);
  }
}
