





#include "MetroUtils.h" 
#include "MetroWidget.h" 
#include "mozilla/dom/Touch.h"  
#include "nsTArray.h" 
#include "nsIDOMSimpleGestureEvent.h" 
#include "InputData.h"


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

  const double WHEEL_DELTA_DOUBLE = static_cast<double>(WHEEL_DELTA);

  
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
}

namespace mozilla {
namespace widget {
namespace winrt {

MetroInput::MetroInput(MetroWidget* aWidget,
                       UI::Core::ICoreWindow* aWindow,
                       UI::Core::ICoreDispatcher* aDispatcher)
              : mWidget(aWidget),
                mWindow(aWindow),
                mDispatcher(aDispatcher),
                mTouchEvent(true, NS_TOUCH_MOVE, aWidget)
{
  LogFunction();
  NS_ASSERTION(aWidget, "Attempted to create MetroInput for null widget!");
  NS_ASSERTION(aWindow, "Attempted to create MetroInput for null window!");

  mWidget->SetMetroInput(this);

  mTokenPointerPressed.value = 0;
  mTokenPointerReleased.value = 0;
  mTokenPointerMoved.value = 0;
  mTokenPointerEntered.value = 0;
  mTokenPointerExited.value = 0;
  mTokenPointerWheelChanged.value = 0;
  mTokenEdgeStarted.value = 0;
  mTokenEdgeCanceled.value = 0;
  mTokenEdgeCompleted.value = 0;
  mTokenManipulationStarted.value = 0;
  mTokenManipulationUpdated.value = 0;
  mTokenManipulationCompleted.value = 0;
  mTokenTapped.value = 0;
  mTokenRightTapped.value = 0;

  mTouches.Init();

  
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




HRESULT
MetroInput::OnPointerWheelChanged(UI::Core::ICoreWindow* aSender,
                                  UI::Core::IPointerEventArgs* aArgs)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  WRL::ComPtr<UI::Input::IPointerPoint> currentPoint;
  WRL::ComPtr<UI::Input::IPointerPointProperties> props;
  Foundation::Point position;
  uint64_t timestamp;
  float pressure;
  boolean horzEvent;
  int32_t delta;

  aArgs->get_CurrentPoint(currentPoint.GetAddressOf());
  currentPoint->get_Position(&position);
  currentPoint->get_Timestamp(&timestamp);
  currentPoint->get_Properties(props.GetAddressOf());
  props->get_Pressure(&pressure);
  props->get_IsHorizontalMouseWheel(&horzEvent);
  props->get_MouseWheelDelta(&delta);

  WheelEvent wheelEvent(true, NS_WHEEL_WHEEL, mWidget.Get());
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(wheelEvent);
  wheelEvent.refPoint = MetroUtils::LogToPhys(position);
  wheelEvent.time = timestamp;
  wheelEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_MOUSE;
  wheelEvent.pressure = pressure;
  wheelEvent.deltaMode = nsIDOMWheelEvent::DOM_DELTA_LINE;

  static int previousVertLeftOverDelta = 0;
  static int previousHorLeftOverDelta = 0;
  
  
  
  
  
  if (horzEvent) {
    wheelEvent.deltaX = delta / WHEEL_DELTA_DOUBLE;
    if ((delta > 0 && previousHorLeftOverDelta < 0)
     || (delta < 0 && previousHorLeftOverDelta > 0)) {
      previousHorLeftOverDelta = 0;
    }
    previousHorLeftOverDelta += delta;
    wheelEvent.lineOrPageDeltaX = previousHorLeftOverDelta / WHEEL_DELTA;
    previousHorLeftOverDelta %= WHEEL_DELTA;
  } else {
    int mouseWheelDelta = -1 * delta;
    wheelEvent.deltaY = mouseWheelDelta / WHEEL_DELTA_DOUBLE;
    if ((mouseWheelDelta > 0 && previousVertLeftOverDelta < 0)
     || (mouseWheelDelta < 0 && previousVertLeftOverDelta > 0)) {
      previousVertLeftOverDelta = 0;
    }
    previousVertLeftOverDelta += mouseWheelDelta;
    wheelEvent.lineOrPageDeltaY = previousVertLeftOverDelta / WHEEL_DELTA;
    previousVertLeftOverDelta %= WHEEL_DELTA;
  }

  DispatchEventIgnoreStatus(&wheelEvent);

  WRL::ComPtr<UI::Input::IPointerPoint> point;
  aArgs->get_CurrentPoint(point.GetAddressOf());
  mGestureRecognizer->ProcessMouseWheelEvent(point.Get(),
                                             wheelEvent.IsShift(),
                                             wheelEvent.IsControl());
  return S_OK;
}










void
MetroInput::OnPointerNonTouch(UI::Input::IPointerPoint* aPoint) {
  WRL::ComPtr<UI::Input::IPointerPointProperties> props;
  UI::Input::PointerUpdateKind pointerUpdateKind;

  aPoint->get_Properties(props.GetAddressOf());
  props->get_PointerUpdateKind(&pointerUpdateKind);

  nsMouseEvent mouseEvent(true,
                          NS_MOUSE_MOVE,
                          mWidget.Get(),
                          nsMouseEvent::eReal,
                          nsMouseEvent::eNormal);

  switch (pointerUpdateKind) {
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonPressed:
      
      
      mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonPressed:
      mouseEvent.button = nsMouseEvent::buttonType::eMiddleButton;
      mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonPressed:
      mouseEvent.button = nsMouseEvent::buttonType::eRightButton;
      mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonReleased:
      
      
      mouseEvent.message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonReleased:
      mouseEvent.button = nsMouseEvent::buttonType::eMiddleButton;
      mouseEvent.message = NS_MOUSE_BUTTON_UP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonReleased:
      mouseEvent.button = nsMouseEvent::buttonType::eRightButton;
      mouseEvent.message = NS_MOUSE_BUTTON_UP;
      break;
  }
  InitGeckoMouseEventFromPointerPoint(mouseEvent, aPoint);
  DispatchEventIgnoreStatus(&mouseEvent);
  return;
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
  mTouchEvent.message = NS_TOUCH_START;

  
  
  if (mTouches.Count() == 1) {
    nsEventStatus status;
    DispatchPendingTouchEvent(status, true);
    mTouchStartDefaultPrevented = (nsEventStatus_eConsumeNoDefault == status);
    
    
    
    
    mTouchMoveDefaultPrevented = mTouchStartDefaultPrevented;
    mIsFirstTouchMove = !mTouchStartDefaultPrevented;
  }

  
  
  
  if (!mTouchStartDefaultPrevented) {
    mGestureRecognizer->ProcessDownEvent(currentPoint.Get());
  }

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

  
  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = mTouches.Get(pointerId);

  
  
  
  if (touch->mChanged) {
    DispatchPendingTouchEvent(true);
  }
  mTouches.Remove(pointerId);

  
  mTouchEvent.message = NS_TOUCH_END;
  mTouchEvent.touches.Clear();
  mTouchEvent.touches.AppendElement(CreateDOMTouch(currentPoint.Get()));
  mTouchEvent.time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(mTouchEvent);

  nsEventStatus status;
  mWidget->DispatchEvent(&mTouchEvent, status);
  if (status != nsEventStatus_eConsumeNoDefault) {
    MultiTouchInput inputData(mTouchEvent);
    if (MetroWidget::sAPZC) {
      status = MetroWidget::sAPZC->ReceiveInputEvent(inputData);
    }
  }
  
  
  mTouchEvent.message = NS_TOUCH_MOVE;

  
  
  
  
  
  
  
  if (!mTouchStartDefaultPrevented) {
    mGestureRecognizer->ProcessUpEvent(currentPoint.Get());
  }

  return S_OK;
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
    WRL::ComPtr<Foundation::Collections::IVector<UI::Input::PointerPoint*>>
        pointerPoints;
    aArgs->GetIntermediatePoints(pointerPoints.GetAddressOf());
    mGestureRecognizer->ProcessMoveEvents(pointerPoints.Get());
    return S_OK;
  }

  
  
  uint32_t pointerId;
  currentPoint->get_PointerId(&pointerId);
  nsRefPtr<Touch> touch = mTouches.Get(pointerId);

  
  
  
  
  if (!touch) {
    return S_OK;
  }

  
  
  if (touch->mChanged) {
    DispatchPendingTouchEvent(true);
  }

  touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  mTouches.Put(pointerId, touch);

  
  
  if (mIsFirstTouchMove) {
    nsEventStatus status;
    DispatchPendingTouchEvent(status, true);
    mTouchMoveDefaultPrevented = (nsEventStatus_eConsumeNoDefault == status);
    mIsFirstTouchMove = false;
  }

  
  
  
  
  
  if (!mTouchMoveDefaultPrevented) {
    WRL::ComPtr<Foundation::Collections::IVector<UI::Input::PointerPoint*>>
        pointerPoints;
    aArgs->GetIntermediatePoints(pointerPoints.GetAddressOf());
    mGestureRecognizer->ProcessMoveEvents(pointerPoints.Get());
  }
  return S_OK;
}

void
MetroInput::InitGeckoMouseEventFromPointerPoint(
                                  nsMouseEvent& aEvent,
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

  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(aEvent);
  aEvent.refPoint = MetroUtils::LogToPhys(position);
  aEvent.time = timestamp;

  if (!canBeDoubleTap) {
    aEvent.clickCount = 1;
  } else {
    aEvent.clickCount = 2;
  }
  aEvent.pressure = pressure;

  MozInputSourceFromDeviceType(deviceType, aEvent.inputSource);
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
    nsMouseEvent mouseEvent(true,
                            NS_MOUSE_ENTER,
                            mWidget.Get(),
                            nsMouseEvent::eReal,
                            nsMouseEvent::eNormal);
    InitGeckoMouseEventFromPointerPoint(mouseEvent, currentPoint.Get());
    DispatchEventIgnoreStatus(&mouseEvent);
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
    nsMouseEvent mouseEvent(true,
                            NS_MOUSE_EXIT,
                            mWidget.Get(),
                            nsMouseEvent::eReal,
                            nsMouseEvent::eNormal);
    InitGeckoMouseEventFromPointerPoint(mouseEvent, currentPoint.Get());
    DispatchEventIgnoreStatus(&mouseEvent);
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

  
  nsSimpleGestureEvent magEvent(true,
                                aMagEventType,
                                mWidget.Get(), 0, 0.0);
  magEvent.delta = aDelta.Expansion;
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(magEvent);
  magEvent.time = ::GetMessageTime();
  magEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  magEvent.refPoint = MetroUtils::LogToPhys(aPosition);
  DispatchEventIgnoreStatus(&magEvent);

  
  nsSimpleGestureEvent rotEvent(true,
                                aRotEventType,
                                mWidget.Get(), 0, 0.0);
  rotEvent.delta = aDelta.Rotation;
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(rotEvent);
  rotEvent.time = ::GetMessageTime();
  rotEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  rotEvent.refPoint = MetroUtils::LogToPhys(aPosition);
  if (rotEvent.delta >= 0) {
    rotEvent.direction = nsIDOMSimpleGestureEvent::ROTATION_COUNTERCLOCKWISE;
  } else {
    rotEvent.direction = nsIDOMSimpleGestureEvent::ROTATION_CLOCKWISE;
  }
  DispatchEventIgnoreStatus(&rotEvent);
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
    nsSimpleGestureEvent swipeEvent(true, NS_SIMPLE_GESTURE_SWIPE,
                                    mWidget.Get(), 0, 0.0);
    swipeEvent.direction = delta.Translation.X > 0
                         ? nsIDOMSimpleGestureEvent::DIRECTION_RIGHT
                         : nsIDOMSimpleGestureEvent::DIRECTION_LEFT;
    swipeEvent.delta = delta.Translation.X;
    mModifierKeyState.Update();
    mModifierKeyState.InitInputEvent(swipeEvent);
    swipeEvent.time = ::GetMessageTime();
    swipeEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    swipeEvent.refPoint = MetroUtils::LogToPhys(position);
    DispatchEventIgnoreStatus(&swipeEvent);
  }

  if (isVerticalSwipe) {
    nsSimpleGestureEvent swipeEvent(true, NS_SIMPLE_GESTURE_SWIPE,
                                    mWidget.Get(), 0, 0.0);
    swipeEvent.direction = delta.Translation.Y > 0
                         ? nsIDOMSimpleGestureEvent::DIRECTION_DOWN
                         : nsIDOMSimpleGestureEvent::DIRECTION_UP;
    swipeEvent.delta = delta.Translation.Y;
    mModifierKeyState.Update();
    mModifierKeyState.InitInputEvent(swipeEvent);
    swipeEvent.time = ::GetMessageTime();
    swipeEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    swipeEvent.refPoint = MetroUtils::LogToPhys(position);
    DispatchEventIgnoreStatus(&swipeEvent);
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
  nsIntPoint pt = MetroUtils::LogToPhys(position);

  LayoutDeviceIntPoint point(pt.x, pt.y);
  HandleSingleTap(point);
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
  nsIntPoint pt = MetroUtils::LogToPhys(position);

  LayoutDeviceIntPoint point(pt.x, pt.y);
  HandleLongTap(point);

  return S_OK;
}


void
MetroInput::HandleDoubleTap(const LayoutDeviceIntPoint& aPoint)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif
  nsSimpleGestureEvent geckoEvent(true, NS_SIMPLE_GESTURE_TAP, mWidget.Get(), 0, 0.0);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(geckoEvent);
  geckoEvent.time = ::GetMessageTime();
  geckoEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  geckoEvent.refPoint.x = aPoint.x;
  geckoEvent.refPoint.y = aPoint.y;
  geckoEvent.clickCount = 2;
  geckoEvent.pressure = 1;
  DispatchEventIgnoreStatus(&geckoEvent);
}

void
MetroInput::HandleSingleTap(const LayoutDeviceIntPoint& aPoint)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  
  
  nsMouseEvent mouseEvent(true,
                          NS_MOUSE_MOVE,
                          mWidget.Get(),
                          nsMouseEvent::eReal,
                          nsMouseEvent::eNormal);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(mouseEvent);
  mouseEvent.refPoint.x = aPoint.x;
  mouseEvent.refPoint.y = aPoint.y;
  mouseEvent.time = ::GetMessageTime();
  mouseEvent.clickCount = 1;
  mouseEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

  
  DispatchEventIgnoreStatus(&mouseEvent);

  
  mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
  mouseEvent.button = nsMouseEvent::buttonType::eLeftButton;
  DispatchEventIgnoreStatus(&mouseEvent);

  
  mouseEvent.message = NS_MOUSE_BUTTON_UP;
  DispatchEventIgnoreStatus(&mouseEvent);

  
  
  
  
  POINT point;
  if (GetCursorPos(&point)) {
    ScreenToClient((HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW), &point);
    Foundation::Point oldMousePosition;
    oldMousePosition.X = static_cast<FLOAT>(point.x);
    oldMousePosition.Y = static_cast<FLOAT>(point.y);
    mouseEvent.refPoint.x = aPoint.x;
    mouseEvent.refPoint.y = aPoint.y;
    mouseEvent.message = NS_MOUSE_MOVE;
    mouseEvent.button = 0;

    DispatchEventIgnoreStatus(&mouseEvent);
  }

}

void
MetroInput::HandleLongTap(const LayoutDeviceIntPoint& aPoint)
{
#ifdef DEBUG_INPUT
  LogFunction();
#endif

  nsMouseEvent contextMenu(true,
                           NS_CONTEXTMENU,
                           mWidget.Get(),
                           nsMouseEvent::eReal,
                           nsMouseEvent::eNormal);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(contextMenu);
  contextMenu.refPoint.x = aPoint.x;
  contextMenu.refPoint.y = aPoint.y;
  contextMenu.time = ::GetMessageTime();
  contextMenu.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  DispatchEventIgnoreStatus(&contextMenu);
}




nsEventStatus MetroInput::sThrowawayStatus;





void
MetroInput::DispatchEventIgnoreStatus(nsGUIEvent *aEvent) {
  mWidget->DispatchEvent(aEvent, sThrowawayStatus);
}

void
MetroInput::DispatchPendingTouchEvent(nsEventStatus& aStatus, bool aDispatchToAPZC) {
  mTouchEvent.touches.Clear();
  mTouches.Enumerate(&AppendToTouchList,
                     static_cast<void*>(&mTouchEvent.touches));
  mTouchEvent.time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(mTouchEvent);

  mWidget->DispatchEvent(&mTouchEvent, aStatus);
  if (aStatus != nsEventStatus_eConsumeNoDefault && aDispatchToAPZC && MetroWidget::sAPZC) {
    MultiTouchInput inputData(mTouchEvent);
    aStatus = MetroWidget::sAPZC->ReceiveInputEvent(inputData);
  }

  
  mTouchEvent.message = NS_TOUCH_MOVE;
}

void
MetroInput::DispatchPendingTouchEvent(bool aDispatchToAPZC) {
  DispatchPendingTouchEvent(sThrowawayStatus, aDispatchToAPZC);
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
  mWindow->remove_PointerWheelChanged(mTokenPointerWheelChanged);

  
  
  
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
  NS_ASSERTION(mDispatcher,
               "Must have a CoreDispatcher to register for input events!");
  
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

  mWindow->add_PointerWheelChanged(
      WRL::Callback<PointerEventHandler>(
        this,
        &MetroInput::OnPointerWheelChanged).Get(),
      &mTokenPointerWheelChanged);

  
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
