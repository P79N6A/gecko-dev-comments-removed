





#include "MetroInput.h"
#include "MetroUtils.h" 
#include "MetroWidget.h" 
#include "mozilla/dom/Touch.h"  
#include "nsTArray.h" 
#include "nsIDOMSimpleGestureEvent.h" 
#include "InputData.h"
#include "UIABridgePrivate.h"
#include "MetroAppShell.h"


#include <windows.ui.core.h> 
#include <windows.ui.input.h> 




using namespace ABI::Windows; 
using namespace Microsoft; 
using namespace mozilla;
using namespace mozilla::widget::winrt;
using namespace mozilla::dom;


namespace {
  
  const double SWIPE_MIN_DISTANCE = 5.0;
  const double SWIPE_MIN_VELOCITY = 5.0;

  
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CInput__CEdgeGesture_Windows__CUI__CInput__CEdgeGestureEventArgs_t EdgeGestureHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CCore__CCoreDispatcher_Windows__CUI__CCore__CAcceleratorKeyEventArgs_t AcceleratorKeyActivatedHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs_t PointerEventHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CInput__CGestureRecognizer_Windows__CUI__CInput__CTappedEventArgs_t TappedEventHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CInput__CGestureRecognizer_Windows__CUI__CInput__CRightTappedEventArgs_t RightTappedEventHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CInput__CGestureRecognizer_Windows__CUI__CInput__CManipulationStartedEventArgs_t ManipulationStartedEventHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CInput__CGestureRecognizer_Windows__CUI__CInput__CManipulationUpdatedEventArgs_t ManipulationUpdatedEventHandler;
  typedef Foundation::__FITypedEventHandler_2_Windows__CUI__CInput__CGestureRecognizer_Windows__CUI__CInput__CManipulationCompletedEventArgs_t ManipulationCompletedEventHandler;

  
  typedef ABI::Windows::UI::Core::ICoreAcceleratorKeys ICoreAcceleratorKeys;

  










  Touch*
  CreateDOMTouch(UI::Input::IPointerPoint* aPoint) {
    WRL::ComPtr<UI::Input::IPointerPointProperties> props;
    Foundation::Point position;
    uint32_t pointerId;
    Foundation::Rect contactRect;
    float pressure;

    aPoint->get_Properties(props.GetAddressOf());
    aPoint->get_Position(&position);
    aPoint->get_PointerId(&pointerId);
    props->get_ContactRect(&contactRect);
    props->get_Pressure(&pressure);

    nsIntPoint touchPoint = MetroUtils::LogToPhys(position);
    nsIntPoint touchRadius;
    touchRadius.x = MetroUtils::LogToPhys(contactRect.Width) / 2;
    touchRadius.y = MetroUtils::LogToPhys(contactRect.Height) / 2;
    return new Touch(pointerId,
                     touchPoint,
                     
                     
                     
                     
                     
                     
                     
                     
                     
                     touchRadius,
                     0.0f,
                     
                     
                     
                     
                     
                     
                     
                     
                     pressure);
  }

  







  bool
  HasPointMoved(Touch* aTouch, UI::Input::IPointerPoint* aPoint) {
    WRL::ComPtr<UI::Input::IPointerPointProperties> props;
    Foundation::Point position;
    Foundation::Rect contactRect;
    float pressure;

    aPoint->get_Properties(props.GetAddressOf());
    aPoint->get_Position(&position);
    props->get_ContactRect(&contactRect);
    props->get_Pressure(&pressure);
    nsIntPoint touchPoint = MetroUtils::LogToPhys(position);
    nsIntPoint touchRadius;
    touchRadius.x = MetroUtils::LogToPhys(contactRect.Width) / 2;
    touchRadius.y = MetroUtils::LogToPhys(contactRect.Height) / 2;

    
    return touchPoint != aTouch->mRefPoint ||
           pressure != aTouch->Force() ||
           
           touchRadius.x != aTouch->RadiusX() ||
           touchRadius.y != aTouch->RadiusY();
  }

  






  void
  MozInputSourceFromDeviceType(
              Devices::Input::PointerDeviceType const& aDeviceType,
              unsigned short& aMozInputSource) {
    if (Devices::Input::PointerDeviceType::PointerDeviceType_Mouse
                  == aDeviceType) {
      aMozInputSource = nsIDOMMouseEvent::MOZ_SOURCE_MOUSE;
    } else if (Devices::Input::PointerDeviceType::PointerDeviceType_Touch
                  == aDeviceType) {
      aMozInputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    } else if (Devices::Input::PointerDeviceType::PointerDeviceType_Pen
                  == aDeviceType) {
      aMozInputSource = nsIDOMMouseEvent::MOZ_SOURCE_PEN;
    }
  }

  














  PLDHashOperator
  AppendToTouchList(const unsigned int& aKey,
                    nsRefPtr<Touch>& aData,
                    void *aTouchList)
  {
    nsTArray<nsRefPtr<Touch> > *touches =
              static_cast<nsTArray<nsRefPtr<Touch> > *>(aTouchList);
    touches->AppendElement(aData);
    aData->mChanged = false;
    return PL_DHASH_NEXT;
  }

  
  class AutoDeleteEvent
  {
  public:
    AutoDeleteEvent(nsGUIEvent* aPtr) :
      mPtr(aPtr) {}
    ~AutoDeleteEvent() {
      if (mPtr) {
        delete mPtr;
      }
    }
    nsGUIEvent* mPtr;
  };
}

namespace mozilla {
namespace widget {
namespace winrt {

MetroInput::MetroInput(MetroWidget* aWidget,
                       UI::Core::ICoreWindow* aWindow)
              : mWidget(aWidget),
                mChromeHitTestCacheForTouch(false),
                mWindow(aWindow)
{
  LogFunction();
  NS_ASSERTION(aWidget, "Attempted to create MetroInput for null widget!");
  NS_ASSERTION(aWindow, "Attempted to create MetroInput for null window!");

  mTokenPointerPressed.value = 0;
  mTokenPointerReleased.value = 0;
  mTokenPointerMoved.value = 0;
  mTokenPointerEntered.value = 0;
  mTokenPointerExited.value = 0;
  mTokenEdgeStarted.value = 0;
  mTokenEdgeCanceled.value = 0;
  mTokenEdgeCompleted.value = 0;
  mTokenManipulationStarted.value = 0;
  mTokenManipulationUpdated.value = 0;
  mTokenManipulationCompleted.value = 0;
  mTokenTapped.value = 0;
  mTokenRightTapped.value = 0;

  
  ActivateGenericInstance(RuntimeClass_Windows_UI_Input_GestureRecognizer,
                          mGestureRecognizer);
  NS_ASSERTION(mGestureRecognizer, "Failed to create GestureRecognizer!");

  RegisterInputEvents();
}

MetroInput::~MetroInput()
{
  LogFunction();
  UnregisterInputEvents();
}









HRESULT
MetroInput::OnEdgeGestureStarted(UI::Input::IEdgeGesture* sender,
                                 UI::Input::IEdgeGestureEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  nsSimpleGestureEvent geckoEvent(true,
                                  NS_SIMPLE_GESTURE_EDGE_STARTED,
                                  mWidget.Get(),
                                  0,
                                  0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();

  geckoEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

  
  DispatchEventIgnoreStatus(&geckoEvent);
  return S_OK;
}










HRESULT
MetroInput::OnEdgeGestureCanceled(UI::Input::IEdgeGesture* sender,
                                  UI::Input::IEdgeGestureEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  nsSimpleGestureEvent geckoEvent(true,
                                  NS_SIMPLE_GESTURE_EDGE_CANCELED,
                                  mWidget.Get(),
                                  0,
                                  0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();

  geckoEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

  
  DispatchEventIgnoreStatus(&geckoEvent);
  return S_OK;
}









HRESULT
MetroInput::OnEdgeGestureCompleted(UI::Input::IEdgeGesture* sender,
                                   UI::Input::IEdgeGestureEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  nsSimpleGestureEvent geckoEvent(true,
                                  NS_SIMPLE_GESTURE_EDGE_COMPLETED,
                                  mWidget.Get(),
                                  0,
                                  0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();

  UI::Input::EdgeGestureKind value;
  aArgs->get_Kind(&value);

  if (value == UI::Input::EdgeGestureKind::EdgeGestureKind_Keyboard) {
    geckoEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
  } else {
    geckoEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  }

  
  DispatchEventIgnoreStatus(&geckoEvent);
  return S_OK;
}










void
MetroInput::OnPointerNonTouch(UI::Input::IPointerPoint* aPoint) {
  WRL::ComPtr<UI::Input::IPointerPointProperties> props;
  UI::Input::PointerUpdateKind pointerUpdateKind;

  aPoint->get_Properties(props.GetAddressOf());
  props->get_PointerUpdateKind(&pointerUpdateKind);

  nsMouseEvent* event =
    new nsMouseEvent(true,
                     NS_MOUSE_MOVE,
                     mWidget.Get(),
                     nsMouseEvent::eReal,
                     nsMouseEvent::eNormal);

  switch (pointerUpdateKind) {
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonPressed:
      
      
      event->message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonPressed:
      event->button = nsMouseEvent::buttonType::eMiddleButton;
      event->message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonPressed:
      event->button = nsMouseEvent::buttonType::eRightButton;
      event->message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonReleased:
      
      
      event->message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonReleased:
      event->button = nsMouseEvent::buttonType::eMiddleButton;
      event->message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonReleased:
      event->button = nsMouseEvent::buttonType::eRightButton;
      event->message = NS_MOUSE_BUTTON_UP;
      break;
  }
  InitGeckoMouseEventFromPointerPoint(event, aPoint);
  DispatchAsyncEventIgnoreStatus(event);
}

void
MetroInput::InitTouchEventTouchList(nsTouchEvent* aEvent)
{
  MOZ_ASSERT(aEvent);
  mTouches.Enumerate(&AppendToTouchList,
                      static_cast<void*>(&aEvent->touches));
}



HRESULT
MetroInput::OnPointerPressed(UI::Core::ICoreWindow* aSender,
                             UI::Core::IPointerEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  WRL::ComPtr<UI::Input::IPointerPoint> currentPoint;
  WRL::ComPtr<Devices::Input::IPointerDevice> device;
  Devices::Input::PointerDeviceType deviceType;

  aArgs->get_CurrentPoint(currentPoint.GetAddressOf());
  currentPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);

  
  if (deviceType !=
          Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    OnPointerNonTouch(currentPoint.Get());
    mGestureRecognizer->ProcessDownEvent(currentPoint.Get());
    return S_OK;
  }

  
  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  mTouches.Put(pointerId, touch);

  nsTouchEvent* touchEvent =
    new nsTouchEvent(true, NS_TOUCH_START, mWidget.Get());

  if (mTouches.Count() == 1) {
    
    
    
    mTouchStartDefaultPrevented = false;
    mTouchMoveDefaultPrevented = false;
    mIsFirstTouchMove = true;
    mCancelable = true;
    mTouchCancelSent = false;
    InitTouchEventTouchList(touchEvent);
    DispatchAsyncTouchEventWithCallback(touchEvent, &MetroInput::OnPointerPressedCallback);
  } else {
    InitTouchEventTouchList(touchEvent);
    DispatchAsyncTouchEventIgnoreStatus(touchEvent);
  }

  if (!mTouchStartDefaultPrevented) {
    mGestureRecognizer->ProcessDownEvent(currentPoint.Get());
  }
  return S_OK;
}

void
MetroInput::OnPointerPressedCallback()
{
  nsEventStatus status = DeliverNextQueuedTouchEvent();
  mTouchStartDefaultPrevented = (nsEventStatus_eConsumeNoDefault == status);
  if (mTouchStartDefaultPrevented) {
    
    
    mGestureRecognizer->CompleteGesture();
    
    mWidget->ApzContentConsumingTouch();
  }
}

void
MetroInput::AddPointerMoveDataToRecognizer(UI::Core::IPointerEventArgs* aArgs)
{
  
  
  if (!mTouchStartDefaultPrevented && !mTouchMoveDefaultPrevented) {
    WRL::ComPtr<Foundation::Collections::IVector<UI::Input::PointerPoint*>>
        pointerPoints;
    aArgs->GetIntermediatePoints(pointerPoints.GetAddressOf());
    mGestureRecognizer->ProcessMoveEvents(pointerPoints.Get());
  }
}




HRESULT
MetroInput::OnPointerMoved(UI::Core::ICoreWindow* aSender,
                           UI::Core::IPointerEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  WRL::ComPtr<UI::Input::IPointerPoint> currentPoint;
  WRL::ComPtr<Devices::Input::IPointerDevice> device;
  Devices::Input::PointerDeviceType deviceType;

  aArgs->get_CurrentPoint(currentPoint.GetAddressOf());
  currentPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);

  
  if (deviceType !=
          Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    OnPointerNonTouch(currentPoint.Get());
    AddPointerMoveDataToRecognizer(aArgs);
    return S_OK;
  }

  
  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = mTouches.Get(pointerId);

  
  
  
  
  if (!touch) {
    return S_OK;
  }

  
  
  
  if (!HasPointMoved(touch, currentPoint.Get())) {
    
    AddPointerMoveDataToRecognizer(aArgs);
    return S_OK;
  }

  
  
  if (!mIsFirstTouchMove && touch->mChanged) {
    nsTouchEvent* touchEvent =
      new nsTouchEvent(true, NS_TOUCH_MOVE, mWidget.Get());
    InitTouchEventTouchList(touchEvent);
    DispatchAsyncTouchEventIgnoreStatus(touchEvent);
  }

  touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  
  mTouches.Put(pointerId, touch);

  nsTouchEvent* touchEvent =
    new nsTouchEvent(true, NS_TOUCH_MOVE, mWidget.Get());

  
  
  
  
  if (mIsFirstTouchMove) {
    InitTouchEventTouchList(touchEvent);
    DispatchAsyncTouchEventWithCallback(touchEvent, &MetroInput::OnFirstPointerMoveCallback);
    mIsFirstTouchMove = false;
  }

  AddPointerMoveDataToRecognizer(aArgs);

  return S_OK;
}

void
MetroInput::OnFirstPointerMoveCallback()
{
  nsEventStatus status = DeliverNextQueuedTouchEvent();
  mCancelable = false;
  mTouchMoveDefaultPrevented = (nsEventStatus_eConsumeNoDefault == status);
  
  if (mTouchMoveDefaultPrevented) {
    mWidget->ApzContentConsumingTouch();
    
    mGestureRecognizer->CompleteGesture();
  } else if (!mTouchMoveDefaultPrevented && !mTouchStartDefaultPrevented) {
    mWidget->ApzContentIgnoringTouch();
  }
}



HRESULT
MetroInput::OnPointerReleased(UI::Core::ICoreWindow* aSender,
                              UI::Core::IPointerEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  WRL::ComPtr<UI::Input::IPointerPoint> currentPoint;
  WRL::ComPtr<Devices::Input::IPointerDevice> device;
  Devices::Input::PointerDeviceType deviceType;

  aArgs->get_CurrentPoint(currentPoint.GetAddressOf());
  currentPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);

  
  if (deviceType !=
          Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    OnPointerNonTouch(currentPoint.Get());
    mGestureRecognizer->ProcessUpEvent(currentPoint.Get());
    return S_OK;
  }

  
  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = mTouches.Get(pointerId);

  
  if (touch->mChanged) {
    nsTouchEvent* touchEvent =
      new nsTouchEvent(true, NS_TOUCH_MOVE, mWidget.Get());
    InitTouchEventTouchList(touchEvent);
    DispatchAsyncTouchEventIgnoreStatus(touchEvent);
  }

  
  
  
  mTouches.Remove(pointerId);

  
  nsTouchEvent* touchEvent =
    new nsTouchEvent(true, NS_TOUCH_END, mWidget.Get());
  touchEvent->touches.AppendElement(CreateDOMTouch(currentPoint.Get()));
  DispatchAsyncTouchEventIgnoreStatus(touchEvent);

  
  
  if (!mTouchStartDefaultPrevented && !mTouchMoveDefaultPrevented) {
    mGestureRecognizer->ProcessUpEvent(currentPoint.Get());
  }

  
  
  
  MetroAppShell::MarkEventQueueForPurge();

  return S_OK;
}




bool
MetroInput::HitTestChrome(const LayoutDeviceIntPoint& pt)
{
  
  nsMouseEvent hittest(true, NS_MOUSE_MOZHITTEST, mWidget.Get(), nsMouseEvent::eReal, nsMouseEvent::eNormal);
  hittest.refPoint = pt;
  nsEventStatus status;
  mWidget->DispatchEvent(&hittest, status);
  return (status == nsEventStatus_eConsumeNoDefault);
}

void
MetroInput::TransformRefPoint(const Foundation::Point& aPosition, LayoutDeviceIntPoint& aRefPointOut)
{
  
  
  LayoutDeviceIntPoint pt = LayoutDeviceIntPoint::FromUntyped(MetroUtils::LogToPhys(aPosition));
  aRefPointOut = pt;
  
  
  bool apzIntersect = mWidget->HitTestAPZC(mozilla::ScreenPoint(pt.x, pt.y));
  if (apzIntersect && HitTestChrome(pt)) {
    return;
  }
  nsMouseEvent event(true,
                     NS_MOUSE_MOVE,
                     mWidget.Get(),
                     nsMouseEvent::eReal,
                     nsMouseEvent::eNormal);
  event.refPoint = aRefPointOut;
  mWidget->ApzReceiveInputEvent(&event);
  aRefPointOut = event.refPoint;
}

void
MetroInput::InitGeckoMouseEventFromPointerPoint(
                                  nsMouseEvent* aEvent,
                                  UI::Input::IPointerPoint* aPointerPoint) {
  NS_ASSERTION(aPointerPoint, "InitGeckoMouseEventFromPointerPoint "
                              "called with null PointerPoint!");

  WRL::ComPtr<UI::Input::IPointerPointProperties> props;
  WRL::ComPtr<Devices::Input::IPointerDevice> device;
  Devices::Input::PointerDeviceType deviceType;
  Foundation::Point position;
  uint64_t timestamp;
  float pressure;
  boolean canBeDoubleTap;

  aPointerPoint->get_Position(&position);
  aPointerPoint->get_Timestamp(&timestamp);
  aPointerPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);
  aPointerPoint->get_Properties(props.GetAddressOf());
  props->get_Pressure(&pressure);
  mGestureRecognizer->CanBeDoubleTap(aPointerPoint, &canBeDoubleTap);

  TransformRefPoint(position, aEvent->refPoint);

  if (!canBeDoubleTap) {
    aEvent->clickCount = 1;
  } else {
    aEvent->clickCount = 2;
  }
  aEvent->pressure = pressure;

  MozInputSourceFromDeviceType(deviceType, aEvent->inputSource);
}




HRESULT
MetroInput::OnPointerEntered(UI::Core::ICoreWindow* aSender,
                             UI::Core::IPointerEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  WRL::ComPtr<UI::Input::IPointerPoint> currentPoint;
  WRL::ComPtr<Devices::Input::IPointerDevice> device;
  Devices::Input::PointerDeviceType deviceType;

  aArgs->get_CurrentPoint(currentPoint.GetAddressOf());
  currentPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);

  
  if (deviceType !=
          Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    nsMouseEvent* event = new nsMouseEvent(true,
                                           NS_MOUSE_ENTER,
                                           mWidget.Get(),
                                           nsMouseEvent::eReal,
                                           nsMouseEvent::eNormal);
    InitGeckoMouseEventFromPointerPoint(event, currentPoint.Get());
    DispatchAsyncEventIgnoreStatus(event);
  }
  return S_OK;
}




HRESULT
MetroInput::OnPointerExited(UI::Core::ICoreWindow* aSender,
                            UI::Core::IPointerEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  WRL::ComPtr<UI::Input::IPointerPoint> currentPoint;
  WRL::ComPtr<Devices::Input::IPointerDevice> device;
  Devices::Input::PointerDeviceType deviceType;

  aArgs->get_CurrentPoint(currentPoint.GetAddressOf());
  currentPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);

  
  if (deviceType !=
          Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    nsMouseEvent* event = new nsMouseEvent(true,
                                           NS_MOUSE_EXIT,
                                           mWidget.Get(),
                                           nsMouseEvent::eReal,
                                           nsMouseEvent::eNormal);
    InitGeckoMouseEventFromPointerPoint(event, currentPoint.Get());
    DispatchAsyncEventIgnoreStatus(event);
  }
  return S_OK;
}












void
MetroInput::ProcessManipulationDelta(
                            UI::Input::ManipulationDelta const& aDelta,
                            Foundation::Point const& aPosition,
                            uint32_t aMagEventType,
                            uint32_t aRotEventType) {
  
  
  
  
  if ((aDelta.Translation.X != 0.0f
    || aDelta.Translation.Y != 0.0f)
   && (aDelta.Rotation == 0.0f
    && aDelta.Expansion == 0.0f)) {
    return;
  }

  
  nsSimpleGestureEvent* magEvent =
    new nsSimpleGestureEvent(true, aMagEventType, mWidget.Get(), 0, 0.0);

  magEvent->delta = aDelta.Expansion;
  magEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  magEvent->refPoint = LayoutDeviceIntPoint::FromUntyped(MetroUtils::LogToPhys(aPosition));
  DispatchAsyncEventIgnoreStatus(magEvent);

  
  nsSimpleGestureEvent* rotEvent =
    new nsSimpleGestureEvent(true, aRotEventType, mWidget.Get(), 0, 0.0);

  rotEvent->delta = aDelta.Rotation;
  rotEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  rotEvent->refPoint = LayoutDeviceIntPoint::FromUntyped(MetroUtils::LogToPhys(aPosition));
  if (rotEvent->delta >= 0) {
    rotEvent->direction = nsIDOMSimpleGestureEvent::ROTATION_COUNTERCLOCKWISE;
  } else {
    rotEvent->direction = nsIDOMSimpleGestureEvent::ROTATION_CLOCKWISE;
  }
  DispatchAsyncEventIgnoreStatus(rotEvent);
}





HRESULT
MetroInput::OnManipulationStarted(
                      UI::Input::IGestureRecognizer* aSender,
                      UI::Input::IManipulationStartedEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  UI::Input::ManipulationDelta delta;
  Foundation::Point position;

  aArgs->get_Cumulative(&delta);
  aArgs->get_Position(&position);

  ProcessManipulationDelta(delta,
                           position,
                           NS_SIMPLE_GESTURE_MAGNIFY_START,
                           NS_SIMPLE_GESTURE_ROTATE_START);
  return S_OK;
}





HRESULT
MetroInput::OnManipulationUpdated(
                        UI::Input::IGestureRecognizer* aSender,
                        UI::Input::IManipulationUpdatedEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  UI::Input::ManipulationDelta delta;
  Foundation::Point position;

  aArgs->get_Delta(&delta);
  aArgs->get_Position(&position);

  ProcessManipulationDelta(delta,
                           position,
                           NS_SIMPLE_GESTURE_MAGNIFY_UPDATE,
                           NS_SIMPLE_GESTURE_ROTATE_UPDATE);
  return S_OK;
}









HRESULT
MetroInput::OnManipulationCompleted(
                        UI::Input::IGestureRecognizer* aSender,
                        UI::Input::IManipulationCompletedEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  UI::Input::ManipulationDelta delta;
  Foundation::Point position;
  Devices::Input::PointerDeviceType deviceType;

  aArgs->get_Position(&position);
  aArgs->get_Cumulative(&delta);
  aArgs->get_PointerDeviceType(&deviceType);

  
  
  ProcessManipulationDelta(delta,
                           position,
                           NS_SIMPLE_GESTURE_MAGNIFY,
                           NS_SIMPLE_GESTURE_ROTATE);

  
  
  
  
  
  
  if (delta.Rotation != 0.0f
   || delta.Expansion != 0.0f
   || deviceType ==
              Devices::Input::PointerDeviceType::PointerDeviceType_Mouse) {
    return S_OK;
  }

  
  
  
  
  UI::Input::ManipulationVelocities velocities;
  aArgs->get_Velocities(&velocities);

  bool isHorizontalSwipe =
            abs(velocities.Linear.X) >= SWIPE_MIN_VELOCITY
         && abs(delta.Translation.X) >= SWIPE_MIN_DISTANCE;
  bool isVerticalSwipe =
            abs(velocities.Linear.Y) >= SWIPE_MIN_VELOCITY
         && abs(delta.Translation.Y) >= SWIPE_MIN_DISTANCE;

  
  
  
  if (isHorizontalSwipe && isVerticalSwipe) {
    return S_OK;
  }

  if (isHorizontalSwipe) {
    nsSimpleGestureEvent* swipeEvent =
      new nsSimpleGestureEvent(true, NS_SIMPLE_GESTURE_SWIPE,
                               mWidget.Get(), 0, 0.0);
    swipeEvent->direction = delta.Translation.X > 0
                         ? nsIDOMSimpleGestureEvent::DIRECTION_RIGHT
                         : nsIDOMSimpleGestureEvent::DIRECTION_LEFT;
    swipeEvent->delta = delta.Translation.X;
    swipeEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    swipeEvent->refPoint = LayoutDeviceIntPoint::FromUntyped(MetroUtils::LogToPhys(position));
    DispatchAsyncEventIgnoreStatus(swipeEvent);
  }

  if (isVerticalSwipe) {
    nsSimpleGestureEvent* swipeEvent =
      new nsSimpleGestureEvent(true, NS_SIMPLE_GESTURE_SWIPE,
                               mWidget.Get(), 0, 0.0);
    swipeEvent->direction = delta.Translation.Y > 0
                         ? nsIDOMSimpleGestureEvent::DIRECTION_DOWN
                         : nsIDOMSimpleGestureEvent::DIRECTION_UP;
    swipeEvent->delta = delta.Translation.Y;
    swipeEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    swipeEvent->refPoint = LayoutDeviceIntPoint::FromUntyped(MetroUtils::LogToPhys(position));
    DispatchAsyncEventIgnoreStatus(swipeEvent);
  }

  return S_OK;
}




HRESULT
MetroInput::OnTapped(UI::Input::IGestureRecognizer* aSender,
                     UI::Input::ITappedEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  Devices::Input::PointerDeviceType deviceType;
  aArgs->get_PointerDeviceType(&deviceType);

  
  
  
  if (deviceType != Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    return S_OK;
  }

  Foundation::Point position;
  aArgs->get_Position(&position);
  HandleSingleTap(position);
  return S_OK;
}





HRESULT
MetroInput::OnRightTapped(UI::Input::IGestureRecognizer* aSender,
                          UI::Input::IRightTappedEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  Devices::Input::PointerDeviceType deviceType;
  aArgs->get_PointerDeviceType(&deviceType);

  Foundation::Point position;
  aArgs->get_Position(&position);
  HandleLongTap(position);

  return S_OK;
}

void
MetroInput::HandleSingleTap(const Foundation::Point& aPoint)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  
  LayoutDeviceIntPoint refPoint;
  TransformRefPoint(aPoint, refPoint);

  
  nsMouseEvent* mouseEvent = new nsMouseEvent(true,
                                              NS_MOUSE_MOVE,
                                              mWidget.Get(),
                                              nsMouseEvent::eReal,
                                              nsMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = 1;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  
  mouseEvent = new nsMouseEvent(true,
                                NS_MOUSE_BUTTON_DOWN,
                                mWidget.Get(),
                                nsMouseEvent::eReal,
                                nsMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = 1;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  mouseEvent->button = nsMouseEvent::buttonType::eLeftButton;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  mouseEvent = new nsMouseEvent(true,
                                NS_MOUSE_BUTTON_UP,
                                mWidget.Get(),
                                nsMouseEvent::eReal,
                                nsMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = 1;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  mouseEvent->button = nsMouseEvent::buttonType::eLeftButton;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  
  
  
  
  POINT point;
  if (GetCursorPos(&point)) {
    ScreenToClient((HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW), &point);
    mouseEvent = new nsMouseEvent(true,
                                  NS_MOUSE_MOVE,
                                  mWidget.Get(),
                                  nsMouseEvent::eReal,
                                  nsMouseEvent::eNormal);
    mouseEvent->refPoint = LayoutDeviceIntPoint(point.x, point.y);
    mouseEvent->clickCount = 1;
    mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    DispatchAsyncEventIgnoreStatus(mouseEvent);
  }

}

void
MetroInput::HandleLongTap(const Foundation::Point& aPoint)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  LayoutDeviceIntPoint refPoint;
  TransformRefPoint(aPoint, refPoint);

  nsMouseEvent* contextEvent = new nsMouseEvent(true,
                                                NS_CONTEXTMENU,
                                                mWidget.Get(),
                                                nsMouseEvent::eReal,
                                                nsMouseEvent::eNormal);
  contextEvent->refPoint = refPoint;
  contextEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  DispatchAsyncEventIgnoreStatus(contextEvent);
}




nsEventStatus MetroInput::sThrowawayStatus;

void
MetroInput::DispatchAsyncEventIgnoreStatus(nsInputEvent* aEvent)
{
  aEvent->time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(*aEvent);
  mInputEventQueue.Push(aEvent);
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &MetroInput::DeliverNextQueuedEventIgnoreStatus);
  NS_DispatchToCurrentThread(runnable);
}

void
MetroInput::DeliverNextQueuedEventIgnoreStatus()
{
  nsGUIEvent* event = static_cast<nsGUIEvent*>(mInputEventQueue.PopFront());
  MOZ_ASSERT(event);
  DispatchEventIgnoreStatus(event);
  delete event;
}

void
MetroInput::DispatchAsyncTouchEventIgnoreStatus(nsTouchEvent* aEvent)
{
  aEvent->time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(*aEvent);
  mInputEventQueue.Push(aEvent);
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &MetroInput::DeliverNextQueuedTouchEvent);
  NS_DispatchToCurrentThread(runnable);
}

nsEventStatus
MetroInput::DeliverNextQueuedTouchEvent()
{
  nsEventStatus status;
  nsTouchEvent* event = static_cast<nsTouchEvent*>(mInputEventQueue.PopFront());
  MOZ_ASSERT(event);

  AutoDeleteEvent wrap(event);

  


















  
  
  
  if (event->message == NS_TOUCH_START) {
    nsRefPtr<Touch> touch = event->touches[0];
    LayoutDeviceIntPoint pt = LayoutDeviceIntPoint::FromUntyped(touch->mRefPoint);
    bool apzIntersect = mWidget->HitTestAPZC(mozilla::ScreenPoint(pt.x, pt.y));
    mChromeHitTestCacheForTouch = (apzIntersect && HitTestChrome(pt));
  }

  
  
  
  if (mTouchStartDefaultPrevented || mTouchMoveDefaultPrevented) {
    if (!mChromeHitTestCacheForTouch) {
      
      
      mWidget->ApzReceiveInputEvent(event);
    }
    mWidget->DispatchEvent(event, status);
    return status;
  }

  
  
  nsTouchEvent transformedEvent(*event);
  status = mWidget->ApzReceiveInputEvent(event, &transformedEvent);
  if (!mCancelable && status == nsEventStatus_eConsumeNoDefault) {
    if (!mTouchCancelSent) {
      mTouchCancelSent = true;
      DispatchTouchCancel();
    }
    return status;
  }

  
  
  mWidget->DispatchEvent(!mChromeHitTestCacheForTouch ? &transformedEvent : event, status);
  return status;
}

void
MetroInput::DispatchTouchCancel()
{
  LogFunction();
  
  
  
  
  
  nsTouchEvent touchEvent(true, NS_TOUCH_CANCEL, mWidget.Get());
  InitTouchEventTouchList(&touchEvent);
  mWidget->DispatchEvent(&touchEvent, sThrowawayStatus);
}

void
MetroInput::DispatchAsyncTouchEventWithCallback(nsTouchEvent* aEvent, void (MetroInput::*Callback)())
{
  aEvent->time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(*aEvent);
  mInputEventQueue.Push(aEvent);
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, Callback);
  NS_DispatchToCurrentThread(runnable);
}

void
MetroInput::DispatchEventIgnoreStatus(nsGUIEvent *aEvent) {
  mWidget->DispatchEvent(aEvent, sThrowawayStatus);
}

void
MetroInput::UnregisterInputEvents() {
  
  WRL::ComPtr<UI::Input::IEdgeGestureStatics> edgeStatics;
  if (SUCCEEDED(Foundation::GetActivationFactory(
        WRL::Wrappers::HStringReference(
              RuntimeClass_Windows_UI_Input_EdgeGesture).Get(),
      edgeStatics.GetAddressOf()))) {
    WRL::ComPtr<UI::Input::IEdgeGesture> edge;
    if (SUCCEEDED(edgeStatics->GetForCurrentView(edge.GetAddressOf()))) {
      edge->remove_Starting(mTokenEdgeStarted);
      edge->remove_Canceled(mTokenEdgeCanceled);
      edge->remove_Completed(mTokenEdgeCompleted);
    }
  }
  
  
  
  mWindow->remove_PointerPressed(mTokenPointerPressed);
  mWindow->remove_PointerReleased(mTokenPointerReleased);
  mWindow->remove_PointerMoved(mTokenPointerMoved);
  mWindow->remove_PointerEntered(mTokenPointerEntered);
  mWindow->remove_PointerExited(mTokenPointerExited);

  
  
  
  mGestureRecognizer->remove_ManipulationStarted(mTokenManipulationStarted);
  mGestureRecognizer->remove_ManipulationUpdated(mTokenManipulationUpdated);
  mGestureRecognizer->remove_ManipulationCompleted(
                                        mTokenManipulationCompleted);
  mGestureRecognizer->remove_Tapped(mTokenTapped);
  mGestureRecognizer->remove_RightTapped(mTokenRightTapped);
}

void
MetroInput::RegisterInputEvents()
{
  NS_ASSERTION(mWindow, "Must have a window to register for input events!");
  NS_ASSERTION(mGestureRecognizer,
               "Must have a GestureRecognizer for input events!");
  
  WRL::ComPtr<UI::Input::IEdgeGestureStatics> edgeStatics;
  Foundation::GetActivationFactory(
            WRL::Wrappers::HStringReference(
                    RuntimeClass_Windows_UI_Input_EdgeGesture)
            .Get(),
            edgeStatics.GetAddressOf());
  WRL::ComPtr<UI::Input::IEdgeGesture> edge;
  edgeStatics->GetForCurrentView(edge.GetAddressOf());

  edge->add_Starting(
      WRL::Callback<EdgeGestureHandler>(
                                  this,
                                  &MetroInput::OnEdgeGestureStarted).Get(),
      &mTokenEdgeStarted);

  edge->add_Canceled(
      WRL::Callback<EdgeGestureHandler>(
                                  this,
                                  &MetroInput::OnEdgeGestureCanceled).Get(),
      &mTokenEdgeCanceled);

  edge->add_Completed(
      WRL::Callback<EdgeGestureHandler>(
                                  this,
                                  &MetroInput::OnEdgeGestureCompleted).Get(),
      &mTokenEdgeCompleted);

  
  
  mGestureRecognizer->put_GestureSettings(
            UI::Input::GestureSettings::GestureSettings_Tap
          | UI::Input::GestureSettings::GestureSettings_DoubleTap
          | UI::Input::GestureSettings::GestureSettings_RightTap
          | UI::Input::GestureSettings::GestureSettings_Hold
          | UI::Input::GestureSettings::GestureSettings_ManipulationTranslateX
          | UI::Input::GestureSettings::GestureSettings_ManipulationTranslateY
          | UI::Input::GestureSettings::GestureSettings_ManipulationScale
          | UI::Input::GestureSettings::GestureSettings_ManipulationRotate);

  
  mWindow->add_PointerPressed(
      WRL::Callback<PointerEventHandler>(
        this,
        &MetroInput::OnPointerPressed).Get(),
      &mTokenPointerPressed);

  mWindow->add_PointerReleased(
      WRL::Callback<PointerEventHandler>(
        this,
        &MetroInput::OnPointerReleased).Get(),
      &mTokenPointerReleased);

  mWindow->add_PointerMoved(
      WRL::Callback<PointerEventHandler>(
        this,
        &MetroInput::OnPointerMoved).Get(),
      &mTokenPointerMoved);

  mWindow->add_PointerEntered(
      WRL::Callback<PointerEventHandler>(
        this,
        &MetroInput::OnPointerEntered).Get(),
      &mTokenPointerEntered);

  mWindow->add_PointerExited(
      WRL::Callback<PointerEventHandler>(
        this,
        &MetroInput::OnPointerExited).Get(),
      &mTokenPointerExited);

  
  mGestureRecognizer->add_Tapped(
      WRL::Callback<TappedEventHandler>(
        this,
        &MetroInput::OnTapped).Get(),
      &mTokenTapped);

  mGestureRecognizer->add_RightTapped(
      WRL::Callback<RightTappedEventHandler>(
        this,
        &MetroInput::OnRightTapped).Get(),
      &mTokenRightTapped);

  mGestureRecognizer->add_ManipulationStarted(
      WRL::Callback<ManipulationStartedEventHandler>(
       this,
       &MetroInput::OnManipulationStarted).Get(),
     &mTokenManipulationStarted);

  mGestureRecognizer->add_ManipulationUpdated(
      WRL::Callback<ManipulationUpdatedEventHandler>(
        this,
        &MetroInput::OnManipulationUpdated).Get(),
      &mTokenManipulationUpdated);

  mGestureRecognizer->add_ManipulationCompleted(
      WRL::Callback<ManipulationCompletedEventHandler>(
        this,
        &MetroInput::OnManipulationCompleted).Get(),
      &mTokenManipulationCompleted);
}

} } }
