





#include "MetroInput.h"
#include "MetroUtils.h" 
#include "MetroWidget.h" 
#include "mozilla/dom/Touch.h"  
#include "nsTArray.h" 
#include "nsIDOMSimpleGestureEvent.h" 
#include "InputData.h"
#include "UIABridgePrivate.h"
#include "MetroAppShell.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TouchEvents.h"


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
    nsRefPtr<Touch> copy = new Touch(aData->mIdentifier,
               aData->mRefPoint,
               aData->mRadius,
               aData->mRotationAngle,
               aData->mForce);
    touches->AppendElement(copy);
    aData->mChanged = false;
    return PL_DHASH_NEXT;
  }

  
  class AutoDeleteEvent
  {
  public:
    AutoDeleteEvent(WidgetGUIEvent* aPtr) :
      mPtr(aPtr) {}
    ~AutoDeleteEvent() {
      if (mPtr) {
        delete mPtr;
      }
    }
    WidgetGUIEvent* mPtr;
  };
}

namespace mozilla {
namespace widget {
namespace winrt {

MetroInput::MetroInput(MetroWidget* aWidget,
                       UI::Core::ICoreWindow* aWindow)
              : mWidget(aWidget),
                mChromeHitTestCacheForTouch(false),
                mCurrentInputLevel(LEVEL_IMPRECISE),
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






void
MetroInput::UpdateInputLevel(InputPrecisionLevel aInputLevel)
{
  
  if (aInputLevel == LEVEL_PRECISE && mTouches.Count() > 0) {
    return;
  }
  if (mCurrentInputLevel != aInputLevel) {
    mCurrentInputLevel = aInputLevel;
    MetroUtils::FireObserver(mCurrentInputLevel == LEVEL_PRECISE ?
                               "metro_precise_input" : "metro_imprecise_input");
  }
}





uint16_t
MetroInput::ProcessInputTypeForGesture(UI::Input::IEdgeGestureEventArgs* aArgs)
{
  MOZ_ASSERT(aArgs);
  UI::Input::EdgeGestureKind kind;
  aArgs->get_Kind(&kind);
  switch(kind) {
    case UI::Input::EdgeGestureKind::EdgeGestureKind_Touch:
      UpdateInputLevel(LEVEL_IMPRECISE);
      return nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    break;
    case UI::Input::EdgeGestureKind::EdgeGestureKind_Keyboard:
      return nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
    break;
    case UI::Input::EdgeGestureKind::EdgeGestureKind_Mouse:
      UpdateInputLevel(LEVEL_PRECISE);
      return nsIDOMMouseEvent::MOZ_SOURCE_MOUSE;
    break;
  }
  return nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN;
}









HRESULT
MetroInput::OnEdgeGestureStarted(UI::Input::IEdgeGesture* sender,
                                 UI::Input::IEdgeGestureEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  WidgetSimpleGestureEvent geckoEvent(true,
                                      NS_SIMPLE_GESTURE_EDGE_STARTED,
                                      mWidget.Get(),
                                      0,
                                      0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();
  geckoEvent.inputSource = ProcessInputTypeForGesture(aArgs);

  
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
  WidgetSimpleGestureEvent geckoEvent(true,
                                      NS_SIMPLE_GESTURE_EDGE_CANCELED,
                                      mWidget.Get(),
                                      0,
                                      0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();
  geckoEvent.inputSource = ProcessInputTypeForGesture(aArgs);

  
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
  WidgetSimpleGestureEvent geckoEvent(true,
                                      NS_SIMPLE_GESTURE_EDGE_COMPLETED,
                                      mWidget.Get(),
                                      0,
                                      0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();
  geckoEvent.inputSource = ProcessInputTypeForGesture(aArgs);

  
  DispatchEventIgnoreStatus(&geckoEvent);
  return S_OK;
}










void
MetroInput::OnPointerNonTouch(UI::Input::IPointerPoint* aPoint) {
  WRL::ComPtr<UI::Input::IPointerPointProperties> props;
  UI::Input::PointerUpdateKind pointerUpdateKind;

  aPoint->get_Properties(props.GetAddressOf());
  props->get_PointerUpdateKind(&pointerUpdateKind);

  WidgetMouseEvent* event =
    new WidgetMouseEvent(true, NS_MOUSE_MOVE, mWidget.Get(),
                         WidgetMouseEvent::eReal,
                         WidgetMouseEvent::eNormal);

  switch (pointerUpdateKind) {
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonPressed:
      
      
      event->message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonPressed:
      event->button = WidgetMouseEvent::buttonType::eMiddleButton;
      event->message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonPressed:
      event->button = WidgetMouseEvent::buttonType::eRightButton;
      event->message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonReleased:
      
      
      event->message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonReleased:
      event->button = WidgetMouseEvent::buttonType::eMiddleButton;
      event->message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonReleased:
      event->button = WidgetMouseEvent::buttonType::eRightButton;
      event->message = NS_MOUSE_BUTTON_UP;
      break;
  }
  UpdateInputLevel(LEVEL_PRECISE);
  InitGeckoMouseEventFromPointerPoint(event, aPoint);
  DispatchAsyncEventIgnoreStatus(event);
}

void
MetroInput::InitTouchEventTouchList(WidgetTouchEvent* aEvent)
{
  MOZ_ASSERT(aEvent);
  mTouches.Enumerate(&AppendToTouchList,
                      static_cast<void*>(&aEvent->touches));
}

bool
MetroInput::ShouldDeliverInputToRecognizer()
{
  return mRecognizerWantsEvents;
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

  
  UpdateInputLevel(LEVEL_IMPRECISE);

  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  mTouches.Put(pointerId, touch);

  WidgetTouchEvent* touchEvent =
    new WidgetTouchEvent(true, NS_TOUCH_START, mWidget.Get());

  if (mTouches.Count() == 1) {
    
    
    mContentConsumingTouch = false;
    mApzConsumingTouch = false;
    mRecognizerWantsEvents = true;
    mCancelable = true;
    mCanceledIds.Clear();
  }

  InitTouchEventTouchList(touchEvent);
  DispatchAsyncTouchEvent(touchEvent);

  if (ShouldDeliverInputToRecognizer()) {
    mGestureRecognizer->ProcessDownEvent(currentPoint.Get());
  }
  return S_OK;
}

void
MetroInput::AddPointerMoveDataToRecognizer(UI::Core::IPointerEventArgs* aArgs)
{
  if (ShouldDeliverInputToRecognizer()) {
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

  
  UpdateInputLevel(LEVEL_IMPRECISE);

  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = mTouches.Get(pointerId);

  
  
  
  
  if (!touch) {
    return S_OK;
  }

  AddPointerMoveDataToRecognizer(aArgs);

  
  
  
  if (!HasPointMoved(touch, currentPoint.Get())) {
    return S_OK;
  }

  touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  
  mTouches.Put(pointerId, touch);

  WidgetTouchEvent* touchEvent =
    new WidgetTouchEvent(true, NS_TOUCH_MOVE, mWidget.Get());
  InitTouchEventTouchList(touchEvent);
  DispatchAsyncTouchEvent(touchEvent);

  return S_OK;
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

  
  UpdateInputLevel(LEVEL_IMPRECISE);

  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = mTouches.Get(pointerId);

  
  if (touch->mChanged) {
    WidgetTouchEvent* touchEvent =
      new WidgetTouchEvent(true, NS_TOUCH_MOVE, mWidget.Get());
    InitTouchEventTouchList(touchEvent);
    DispatchAsyncTouchEvent(touchEvent);
  }

  
  
  
  mTouches.Remove(pointerId);

  
  WidgetTouchEvent* touchEvent =
    new WidgetTouchEvent(true, NS_TOUCH_END, mWidget.Get());
  touchEvent->touches.AppendElement(CreateDOMTouch(currentPoint.Get()));
  DispatchAsyncTouchEvent(touchEvent);

  if (ShouldDeliverInputToRecognizer()) {
    mGestureRecognizer->ProcessUpEvent(currentPoint.Get());
  }

  
  
  
  MetroAppShell::MarkEventQueueForPurge();

  return S_OK;
}




bool
MetroInput::HitTestChrome(const LayoutDeviceIntPoint& pt)
{
  
  WidgetMouseEvent hittest(true, NS_MOUSE_MOZHITTEST, mWidget.Get(),
                           WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
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
  WidgetMouseEvent event(true, NS_MOUSE_MOVE, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  event.refPoint = aRefPointOut;
  mWidget->ApzReceiveInputEvent(&event);
  aRefPointOut = event.refPoint;
}

void
MetroInput::InitGeckoMouseEventFromPointerPoint(
                                  WidgetMouseEvent* aEvent,
                                  UI::Input::IPointerPoint* aPointerPoint)
{
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
    WidgetMouseEvent* event =
      new WidgetMouseEvent(true, NS_MOUSE_ENTER, mWidget.Get(),
                           WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
    UpdateInputLevel(LEVEL_PRECISE);
    InitGeckoMouseEventFromPointerPoint(event, currentPoint.Get());
    DispatchAsyncEventIgnoreStatus(event);
    return S_OK;
  }
  UpdateInputLevel(LEVEL_IMPRECISE);
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
    WidgetMouseEvent* event =
      new WidgetMouseEvent(true, NS_MOUSE_EXIT, mWidget.Get(),
                           WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
    UpdateInputLevel(LEVEL_PRECISE);
    InitGeckoMouseEventFromPointerPoint(event, currentPoint.Get());
    DispatchAsyncEventIgnoreStatus(event);
    return S_OK;
  }
  UpdateInputLevel(LEVEL_IMPRECISE);
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

  Devices::Input::PointerDeviceType deviceType;
  aArgs->get_PointerDeviceType(&deviceType);
  if (deviceType ==
              Devices::Input::PointerDeviceType::PointerDeviceType_Mouse) {
    return S_OK;
  }

  UI::Input::ManipulationDelta delta;
  Foundation::Point position;

  aArgs->get_Position(&position);
  aArgs->get_Cumulative(&delta);

  
  
  
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
    WidgetSimpleGestureEvent* swipeEvent =
      new WidgetSimpleGestureEvent(true, NS_SIMPLE_GESTURE_SWIPE,
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
    WidgetSimpleGestureEvent* swipeEvent =
      new WidgetSimpleGestureEvent(true, NS_SIMPLE_GESTURE_SWIPE,
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

  
  WidgetMouseEvent* mouseEvent =
    new WidgetMouseEvent(true, NS_MOUSE_MOVE, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = 1;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  
  mouseEvent =
    new WidgetMouseEvent(true, NS_MOUSE_BUTTON_DOWN, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = 1;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  mouseEvent->button = WidgetMouseEvent::buttonType::eLeftButton;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  mouseEvent =
    new WidgetMouseEvent(true, NS_MOUSE_BUTTON_UP, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = 1;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  mouseEvent->button = WidgetMouseEvent::buttonType::eLeftButton;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  
  
  
  
  POINT point;
  if (GetCursorPos(&point)) {
    ScreenToClient((HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW), &point);
    mouseEvent =
      new WidgetMouseEvent(true, NS_MOUSE_MOVE, mWidget.Get(),
                           WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
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

  WidgetMouseEvent* contextEvent =
    new WidgetMouseEvent(true, NS_CONTEXTMENU, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  contextEvent->refPoint = refPoint;
  contextEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  DispatchAsyncEventIgnoreStatus(contextEvent);
}




nsEventStatus MetroInput::sThrowawayStatus;

void
MetroInput::DispatchAsyncEventIgnoreStatus(WidgetInputEvent* aEvent)
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
  WidgetGUIEvent* event =
    static_cast<WidgetGUIEvent*>(mInputEventQueue.PopFront());
  MOZ_ASSERT(event);
  DispatchEventIgnoreStatus(event);
  delete event;
}

void
MetroInput::DispatchAsyncTouchEvent(WidgetTouchEvent* aEvent)
{
  aEvent->time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(*aEvent);
  mInputEventQueue.Push(aEvent);
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &MetroInput::DeliverNextQueuedTouchEvent);
  NS_DispatchToCurrentThread(runnable);
}

static void DumpTouchIds(const char* aTarget, WidgetTouchEvent* aEvent)
{
  
  if (aEvent->message == NS_TOUCH_MOVE) {
    return;
  }
  switch(aEvent->message) {
    case NS_TOUCH_START:
    WinUtils::Log("DumpTouchIds: NS_TOUCH_START block");
    break;
    case NS_TOUCH_MOVE:
    WinUtils::Log("DumpTouchIds: NS_TOUCH_MOVE block");
    break;
    case NS_TOUCH_END:
    WinUtils::Log("DumpTouchIds: NS_TOUCH_END block");
    break;
    case NS_TOUCH_CANCEL:
    WinUtils::Log("DumpTouchIds: NS_TOUCH_CANCEL block");
    break;
  }
  nsTArray< nsRefPtr<dom::Touch> >& touches = aEvent->touches;
  for (uint32_t i = 0; i < touches.Length(); ++i) {
    dom::Touch* touch = touches[i];
    if (!touch) {
      continue;
    }
    int32_t id = touch->Identifier();
    WinUtils::Log("   id=%d target=%s", id, aTarget);
  }
}

















#define DUMP_TOUCH_IDS(...)

void
MetroInput::DeliverNextQueuedTouchEvent()
{
  nsEventStatus status;
  WidgetTouchEvent* event =
    static_cast<WidgetTouchEvent*>(mInputEventQueue.PopFront());
  MOZ_ASSERT(event);

  AutoDeleteEvent wrap(event);

  






















  
  
  
  if (mCancelable && event->message == NS_TOUCH_START) {
    nsRefPtr<Touch> touch = event->touches[0];
    LayoutDeviceIntPoint pt = LayoutDeviceIntPoint::FromUntyped(touch->mRefPoint);
    bool apzIntersect = mWidget->HitTestAPZC(mozilla::ScreenPoint(pt.x, pt.y));
    mChromeHitTestCacheForTouch = (apzIntersect && HitTestChrome(pt));
  }

  
  
  if (!mCancelable && mChromeHitTestCacheForTouch) {
    DUMP_TOUCH_IDS("DOM(1)", event);
    mWidget->DispatchEvent(event, status);
    return;
  }

  
  
  
  if (mCancelable) {
    WidgetTouchEvent transformedEvent(*event);
    DUMP_TOUCH_IDS("APZC(1)", event);
    mWidget->ApzReceiveInputEvent(event, &transformedEvent);
    DUMP_TOUCH_IDS("DOM(2)", event);
    mWidget->DispatchEvent(mChromeHitTestCacheForTouch ? event : &transformedEvent, status);
    if (event->message == NS_TOUCH_START) {
      mContentConsumingTouch = (nsEventStatus_eConsumeNoDefault == status);
      
      
      mRecognizerWantsEvents = !(nsEventStatus_eConsumeNoDefault == status);
    } else if (event->message == NS_TOUCH_MOVE) {
      mCancelable = false;
      
      if (!mContentConsumingTouch) {
        mContentConsumingTouch = (nsEventStatus_eConsumeNoDefault == status);
      }
      
      
      if (mContentConsumingTouch) {
        mWidget->ApzContentConsumingTouch();
        DispatchTouchCancel(event);
      } else {
        mWidget->ApzContentIgnoringTouch();
      }
    }
    
    
    if (!ShouldDeliverInputToRecognizer()) {
      mGestureRecognizer->CompleteGesture();
    }
    return;
  }

  
  
  
  DUMP_TOUCH_IDS("APZC(2)", event);
  status = mWidget->ApzReceiveInputEvent(event);

  
  
  if (mContentConsumingTouch) {
    DUMP_TOUCH_IDS("DOM(3)", event);
    mWidget->DispatchEvent(event, status);
    return;
  }

  
  if (!mApzConsumingTouch) {
    if (status == nsEventStatus_eConsumeNoDefault) {
      mApzConsumingTouch = true;
      DispatchTouchCancel(event);
      return;
    }
    DUMP_TOUCH_IDS("DOM(4)", event);
    mWidget->DispatchEvent(event, status);
  }
}

void
MetroInput::DispatchTouchCancel(WidgetTouchEvent* aEvent)
{
  MOZ_ASSERT(aEvent);
  
  
  
  WidgetTouchEvent touchEvent(true, NS_TOUCH_CANCEL, mWidget.Get());
  nsTArray< nsRefPtr<dom::Touch> >& touches = aEvent->touches;
  for (uint32_t i = 0; i < touches.Length(); ++i) {
    dom::Touch* touch = touches[i];
    if (!touch) {
      continue;
    }
    int32_t id = touch->Identifier();
    if (mCanceledIds.Contains(id)) {
      continue;
    }
    mCanceledIds.AppendElement(id);
    touchEvent.touches.AppendElement(touch);
  }
  if (!touchEvent.touches.Length()) {
    return;
  }
  if (mContentConsumingTouch) {
    DUMP_TOUCH_IDS("APZC(3)", &touchEvent);
    mWidget->ApzReceiveInputEvent(&touchEvent);
  } else {
    DUMP_TOUCH_IDS("DOM(5)", &touchEvent);
    mWidget->DispatchEvent(&touchEvent, sThrowawayStatus);
  }
}

void
MetroInput::DispatchEventIgnoreStatus(WidgetGUIEvent *aEvent)
{
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
          | UI::Input::GestureSettings::GestureSettings_ManipulationTranslateY);

  
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

  mGestureRecognizer->add_ManipulationCompleted(
      WRL::Callback<ManipulationCompletedEventHandler>(
        this,
        &MetroInput::OnManipulationCompleted).Get(),
      &mTokenManipulationCompleted);
}

} } }
