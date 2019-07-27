





#include "MetroInput.h"
#include "MetroUtils.h" 
#include "MetroWidget.h" 
#include "mozilla/dom/Touch.h"  
#include "nsTArray.h" 
#include "nsIDOMSimpleGestureEvent.h" 
#include "InputData.h"
#include "UIABridgePrivate.h"
#include "MetroAppShell.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/Preferences.h"  
#include "WinUtils.h"
#include "nsIPresShell.h"
#include "nsPoint.h"


#include <windows.ui.core.h> 
#include <windows.ui.input.h> 




using namespace ABI::Windows; 
using namespace Microsoft; 
using namespace mozilla;
using namespace mozilla::widget;
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

  


  static bool gTouchActionPropertyEnabled = false;

  










  Touch*
  CreateDOMTouch(UI::Input::IPointerPoint* aPoint) {
    WRL::ComPtr<UI::Input::IPointerPointProperties> props;
    Foundation::Point position;
    uint32_t pointerId;
    Foundation::Rect contactRect;
    float pressure;
    float tiltX;
    float tiltY;

    aPoint->get_Properties(props.GetAddressOf());
    aPoint->get_Position(&position);
    aPoint->get_PointerId(&pointerId);
    props->get_ContactRect(&contactRect);
    props->get_Pressure(&pressure);
    props->get_XTilt(&tiltX);
    props->get_YTilt(&tiltY);

    nsIntPoint touchPoint = MetroUtils::LogToPhys(position);
    nsIntPoint touchRadius;
    touchRadius.x = WinUtils::LogToPhys(contactRect.Width) / 2;
    touchRadius.y = WinUtils::LogToPhys(contactRect.Height) / 2;
    Touch* touch =
           new Touch(pointerId,
                     touchPoint,
                     
                     
                     
                     
                     
                     
                     
                     
                     
                     touchRadius,
                     0.0f,
                     
                     
                     
                     
                     
                     
                     
                     
                     pressure);
    touch->tiltX = tiltX;
    touch->tiltY = tiltY;
    return touch;
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
    touchRadius.x = WinUtils::LogToPhys(contactRect.Width) / 2;
    touchRadius.y = WinUtils::LogToPhys(contactRect.Height) / 2;

    
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

  int16_t
  ButtonsForPointerPoint(UI::Input::IPointerPoint* aPoint) {
    WRL::ComPtr<UI::Input::IPointerPointProperties> props;
    aPoint->get_Properties(props.GetAddressOf());

    int16_t buttons = 0;
    boolean buttonPressed;

    props->get_IsLeftButtonPressed(&buttonPressed);
    if (buttonPressed) {
      buttons |= WidgetMouseEvent::eLeftButtonFlag;
    }
    props->get_IsMiddleButtonPressed(&buttonPressed);
    if (buttonPressed) {
      buttons |= WidgetMouseEvent::eMiddleButtonFlag;
    }
    props->get_IsRightButtonPressed(&buttonPressed);
    if (buttonPressed) {
      buttons |= WidgetMouseEvent::eRightButtonFlag;
    }
    props->get_IsXButton1Pressed(&buttonPressed);
    if (buttonPressed) {
      buttons |= WidgetMouseEvent::e4thButtonFlag;
    }
    props->get_IsXButton2Pressed(&buttonPressed);
    if (buttonPressed) {
      buttons |= WidgetMouseEvent::e5thButtonFlag;
    }
    return buttons;
  }

  














  PLDHashOperator
  AppendToTouchList(const unsigned int& aKey,
                    nsRefPtr<Touch>& aData,
                    void *aTouchList)
  {
    WidgetTouchEvent::TouchArray* touches =
              static_cast<WidgetTouchEvent::TouchArray*>(aTouchList);
    nsRefPtr<Touch> copy = new Touch(aData->mIdentifier,
               aData->mRefPoint,
               aData->mRadius,
               aData->mRotationAngle,
               aData->mForce);
    copy->tiltX = aData->tiltX;
    copy->tiltY = aData->tiltY;
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

MetroInput::InputPrecisionLevel MetroInput::sCurrentInputLevel =
  MetroInput::InputPrecisionLevel::LEVEL_IMPRECISE;

MetroInput::MetroInput(MetroWidget* aWidget,
                       UI::Core::ICoreWindow* aWindow)
              : mWidget(aWidget),
                mNonApzTargetForTouch(false),
                mWindow(aWindow),
                mInputBlockId(0)
{
  LogFunction();
  NS_ASSERTION(aWidget, "Attempted to create MetroInput for null widget!");
  NS_ASSERTION(aWindow, "Attempted to create MetroInput for null window!");

  mWidget->SetApzPendingResponseFlusher(this);

  Preferences::AddBoolVarCache(&gTouchActionPropertyEnabled, "layout.css.touch_action.enabled", gTouchActionPropertyEnabled);
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


bool MetroInput::IsInputModeImprecise()
{
  return sCurrentInputLevel == LEVEL_IMPRECISE;
}





void
MetroInput::UpdateInputLevel(InputPrecisionLevel aInputLevel)
{
  
  if (aInputLevel == LEVEL_PRECISE && mTouches.Count() > 0) {
    return;
  }
  if (sCurrentInputLevel != aInputLevel) {
    sCurrentInputLevel = aInputLevel;
    MetroUtils::FireObserver(sCurrentInputLevel == LEVEL_PRECISE ?
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
                                      mWidget.Get());
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
                                      mWidget.Get());
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
                                      mWidget.Get());
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

  uint32_t message = NS_MOUSE_MOVE;
  int16_t button = 0;

  switch (pointerUpdateKind) {
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonPressed:
      button = WidgetMouseEvent::buttonType::eLeftButton;
      message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonPressed:
      button = WidgetMouseEvent::buttonType::eMiddleButton;
      message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonPressed:
      button = WidgetMouseEvent::buttonType::eRightButton;
      message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonReleased:
      button = WidgetMouseEvent::buttonType::eLeftButton;
      message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonReleased:
      button = WidgetMouseEvent::buttonType::eMiddleButton;
      message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonReleased:
      button = WidgetMouseEvent::buttonType::eRightButton;
      message = NS_MOUSE_BUTTON_UP;
      break;
  }

  UpdateInputLevel(LEVEL_PRECISE);

  WidgetMouseEvent* event =
    new WidgetMouseEvent(true, message, mWidget.Get(),
                         WidgetMouseEvent::eReal,
                         WidgetMouseEvent::eNormal);
  event->button = button;
  aPoint->get_PointerId(&event->pointerId);
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

void
MetroInput::GetAllowedTouchBehavior(WidgetTouchEvent* aTransformedEvent, nsTArray<TouchBehaviorFlags>& aOutBehaviors)
{
  for (uint32_t i = 0; i < aTransformedEvent->touches.Length(); i++) {
    
    
    aOutBehaviors.AppendElement(mWidget->ContentGetAllowedTouchBehavior(aTransformedEvent->touches[i]->mRefPoint));
  }
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
    
    
    mRecognizerWantsEvents = true;
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




bool
MetroInput::TransformRefPoint(const Foundation::Point& aPosition, LayoutDeviceIntPoint& aRefPointOut)
{
  
  
  aRefPointOut = LayoutDeviceIntPoint::FromUntyped(MetroUtils::LogToPhys(aPosition));
  ScreenIntPoint spt(aRefPointOut.x, aRefPointOut.y);
  
  
  bool apzIntersect = mWidget->ApzHitTest(spt);
  if (!apzIntersect) {
    return true;
  }
  if (HitTestChrome(aRefPointOut)) {
    return true;
  }
  mWidget->ApzTransformGeckoCoordinate(spt, &aRefPointOut);
  return false;
}

void
MetroInput::TransformTouchEvent(WidgetTouchEvent* aEvent)
{
  WidgetTouchEvent::TouchArray& touches = aEvent->touches;
  for (uint32_t i = 0; i < touches.Length(); ++i) {
    dom::Touch* touch = touches[i];
    if (touch) {
      LayoutDeviceIntPoint lpt;
      ScreenIntPoint spt;
      spt.x = touch->mRefPoint.x;
      spt.y = touch->mRefPoint.y;
      mWidget->ApzTransformGeckoCoordinate(spt, &lpt);
      touch->mRefPoint.x = lpt.x;
      touch->mRefPoint.y = lpt.y;
    }
  }
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
  float tiltX;
  float tiltY;

  aPointerPoint->get_Position(&position);
  aPointerPoint->get_Timestamp(&timestamp);
  aPointerPoint->get_PointerDevice(device.GetAddressOf());
  device->get_PointerDeviceType(&deviceType);
  aPointerPoint->get_Properties(props.GetAddressOf());
  aPointerPoint->get_PointerId(&aEvent->pointerId);
  props->get_Pressure(&pressure);
  props->get_XTilt(&tiltX);
  props->get_YTilt(&tiltY);

  mGestureRecognizer->CanBeDoubleTap(aPointerPoint, &canBeDoubleTap);

  TransformRefPoint(position, aEvent->refPoint);

  if (!canBeDoubleTap) {
    aEvent->clickCount = 1;
  } else {
    aEvent->clickCount = 2;
  }
  aEvent->pressure = pressure;
  aEvent->tiltX = tiltX;
  aEvent->tiltY = tiltY;
  aEvent->buttons = ButtonsForPointerPoint(aPointerPoint);

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
    event->exit = WidgetMouseEvent::eTopLevel;
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
                                   mWidget.Get());
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
                                   mWidget.Get());
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

  unsigned int tapCount;
  aArgs->get_TapCount(&tapCount);

  
  
  
  if (deviceType != Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    return S_OK;
  }

  Foundation::Point position;
  aArgs->get_Position(&position);
  HandleTap(position, tapCount);
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
MetroInput::HandleTap(const Foundation::Point& aPoint, unsigned int aTapCount)
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
  mouseEvent->clickCount = aTapCount;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  mouseEvent =
    new WidgetMouseEvent(true, NS_MOUSE_BUTTON_DOWN, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = aTapCount;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  mouseEvent->button = WidgetMouseEvent::buttonType::eLeftButton;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  mouseEvent =
    new WidgetMouseEvent(true, NS_MOUSE_BUTTON_UP, mWidget.Get(),
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  mouseEvent->refPoint = refPoint;
  mouseEvent->clickCount = aTapCount;
  mouseEvent->inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  mouseEvent->button = WidgetMouseEvent::buttonType::eLeftButton;
  DispatchAsyncEventIgnoreStatus(mouseEvent);

  
  
  
  MetroAppShell::MarkEventQueueForPurge();
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
  nsAutoPtr<WidgetGUIEvent> event =
    static_cast<WidgetGUIEvent*>(mInputEventQueue.PopFront());
  MOZ_ASSERT(event.get());
  DispatchEventIgnoreStatus(event.get());

  
  
  if (event->message == NS_MOUSE_BUTTON_UP) {
    MetroAppShell::InputEventsDispatched();
  }

  
  WidgetMouseEvent* mouseEvent = event.get()->AsMouseEvent();
  if (!mouseEvent) {
    return;
  }
  if (mouseEvent->message != NS_MOUSE_BUTTON_UP ||
      mouseEvent->inputSource != nsIDOMMouseEvent::MOZ_SOURCE_TOUCH) {
    return;
  }
  nsCOMPtr<nsIPresShell> presShell = mWidget->GetPresShell();
  if (presShell) {
    EventStateManager* esm = presShell->GetPresContext()->EventStateManager();
    if (esm) {
      esm->SetContentState(nullptr, NS_EVENT_STATE_HOVER);
    }
  }
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
  WidgetTouchEvent::TouchArray& touches = aEvent->touches;
  for (uint32_t i = 0; i < touches.Length(); ++i) {
    dom::Touch* touch = touches[i];
    if (!touch) {
      continue;
    }
    int32_t id = touch->Identifier();
    WinUtils::Log("   id=%d target=%s", id, aTarget);
  }
}

static void DumpTouchBehavior(nsTArray<uint32_t>& aBehavior)
{
  WinUtils::Log("DumpTouchBehavior: Touch behavior flags set for current touch session:");
  for (uint32_t i = 0; i < aBehavior.Length(); i++) {
    if (mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN & aBehavior[i]) {
      WinUtils::Log("VERTICAL_PAN");
    }

    if (mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN & aBehavior[i]) {
      WinUtils::Log("HORIZONTAL_PAN");
    }

    if (mozilla::layers::AllowedTouchBehavior::UNKNOWN & aBehavior[i]) {
      WinUtils::Log("UNKNOWN");
    }

    if ((mozilla::layers::AllowedTouchBehavior::NONE & aBehavior[i]) == 0) {
      WinUtils::Log("NONE");
    }
  }
}

















#define DUMP_TOUCH_IDS(...)


#define DUMP_ALLOWED_TOUCH_BEHAVIOR(...)

void
MetroInput::HandleTouchStartEvent(WidgetTouchEvent* aEvent)
{
  

  WidgetTouchEvent transformedEvent(*aEvent);
  DUMP_TOUCH_IDS("APZC(1)", aEvent);
  nsEventStatus result = mWidget->ApzReceiveInputEvent(&transformedEvent, &mTargetAPZCGuid, &mInputBlockId);
  if (result == nsEventStatus_eConsumeNoDefault) {
    
    CancelGesture();
    return;
  }

  
  if (gTouchActionPropertyEnabled) {
    nsTArray<TouchBehaviorFlags> touchBehaviors;
    
    
    
    
    GetAllowedTouchBehavior(&transformedEvent, touchBehaviors);
    
    
    
    
    DUMP_ALLOWED_TOUCH_BEHAVIOR(touchBehaviors);
    mWidget->ApzcSetAllowedTouchBehavior(mInputBlockId, touchBehaviors);
  }

  
  DUMP_TOUCH_IDS("DOM(2)", aEvent);
  nsEventStatus contentStatus = nsEventStatus_eIgnore;
  mWidget->DispatchEvent(&transformedEvent, contentStatus);
  if (nsEventStatus_eConsumeNoDefault == contentStatus) {
    
    
    mWidget->ApzContentConsumingTouch(mInputBlockId);
    mCancelable = false;

    
    CancelGesture();
  }
}

void
MetroInput::HandleFirstTouchMoveEvent(WidgetTouchEvent* aEvent)
{
  
  WidgetTouchEvent transformedEvent(*aEvent);
  DUMP_TOUCH_IDS("APZC(2)", aEvent);
  nsEventStatus apzcStatus = mWidget->ApzReceiveInputEvent(&transformedEvent, &mTargetAPZCGuid, &mInputBlockId);
  if (apzcStatus == nsEventStatus_eConsumeNoDefault) {
    
    CancelGesture();
    return;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  DUMP_TOUCH_IDS("DOM(3)", aEvent);
  nsEventStatus contentStatus = nsEventStatus_eIgnore;
  mWidget->DispatchEvent(&transformedEvent, contentStatus);

  
  if (mCancelable) {
    if (nsEventStatus_eConsumeNoDefault == contentStatus) {
      mWidget->ApzContentConsumingTouch(mInputBlockId);
    } else {
      mWidget->ApzContentIgnoringTouch(mInputBlockId);
      if (apzcStatus == nsEventStatus_eConsumeDoDefault) {
        SendPointerCancelToContent(transformedEvent);
      }
    }
    mCancelable = false;
  }

  
  if (nsEventStatus_eConsumeNoDefault == contentStatus) {
    CancelGesture();
  }
}

void
MetroInput::SendPointerCancelToContent(const WidgetTouchEvent& aEvent)
{
  
  
  
  
  WidgetTouchEvent cancel(aEvent);
  cancel.message = NS_TOUCH_CANCEL;
  for (uint32_t i = 0; i < cancel.touches.Length(); i++) {
    cancel.touches[i]->convertToPointer = true;
  }
  nsEventStatus status;
  mWidget->DispatchEvent(&cancel, status);
}

bool
MetroInput::SendPendingResponseToApz()
{
  
  
  if (mCancelable) {
    mWidget->ApzContentIgnoringTouch(mInputBlockId);
    mCancelable = false;
    return true;
  }
  return false;
}

void
MetroInput::FlushPendingContentResponse()
{
  SendPendingResponseToApz();
}

void
MetroInput::CancelGesture()
{
  if (mRecognizerWantsEvents) {
    mRecognizerWantsEvents = false;
    mGestureRecognizer->CompleteGesture();
  }
}

void
MetroInput::DeliverNextQueuedTouchEvent()
{
  




  nsEventStatus status = nsEventStatus_eIgnore;

  WidgetTouchEvent* event =
    static_cast<WidgetTouchEvent*>(mInputEventQueue.PopFront());
  MOZ_ASSERT(event);

  AutoDeleteEvent wrap(event);

  
  
  
  if (event->message == NS_TOUCH_START) {
    SendPendingResponseToApz();

    mCancelable = true;
    mTargetAPZCGuid = ScrollableLayerGuid();
    mInputBlockId = 0;
  }

  
  
  
  if (event->message == NS_TOUCH_START && event->touches.Length() == 1) {
    nsRefPtr<Touch> touch = event->touches[0];
    LayoutDeviceIntPoint pt = LayoutDeviceIntPoint::FromUntyped(touch->mRefPoint);
    
    
    
    bool apzIntersect = mWidget->ApzHitTest(mozilla::ScreenIntPoint(pt.x, pt.y));
    mNonApzTargetForTouch = (!apzIntersect || HitTestChrome(pt));
  }

  
  
  
  if (mNonApzTargetForTouch) {
    DUMP_TOUCH_IDS("DOM(1)", event);
    mWidget->DispatchEvent(event, status);
    if (mCancelable) {
      
      
      if (nsEventStatus_eConsumeNoDefault == status) {
        CancelGesture();
      }
      if (event->message == NS_TOUCH_MOVE) {
        mCancelable = false;
      }
    }
    return;
  }

  
  if (event->message == NS_TOUCH_START) {
    HandleTouchStartEvent(event);
    return;
  } else if (mCancelable && event->message == NS_TOUCH_MOVE) {
    HandleFirstTouchMoveEvent(event);
    return;
  }

  
  
  
  bool responseSent = SendPendingResponseToApz();

  
  
  DUMP_TOUCH_IDS("APZC(3)", event);
  status = mWidget->ApzReceiveInputEvent(event, nullptr, nullptr);
  if (status == nsEventStatus_eConsumeNoDefault) {
    CancelGesture();
    return;
  }
  if (responseSent && status == nsEventStatus_eConsumeDoDefault) {
    SendPointerCancelToContent(*event);
    return;
  }
  DUMP_TOUCH_IDS("DOM(4)", event);
  mWidget->DispatchEvent(event, status);
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
