




#include "MetroInput.h"


#include "MetroUtils.h" 
#include "MetroWidget.h" 
#include "npapi.h" 
#include "nsDOMTouchEvent.h"  
#include "nsTArray.h" 
#include "nsIDOMSimpleGestureEvent.h" 


#include <windows.ui.core.h> 
#include <windows.ui.input.h> 




using namespace ABI::Windows; 
using namespace Microsoft; 
using namespace mozilla::widget::winrt;


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

  










  nsDOMTouch*
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
    return new nsDOMTouch(pointerId,
                          touchPoint,
                          
                          
                          
                          
                          
                          
                          
                          
                          
                          touchRadius,
                          0.0f,
                          
                          
                          
                          
                          
                          
                          
                          
                          pressure);
  }

  bool
  IsControlCharacter(uint32_t aCharCode) {
    return (0x1F >= aCharCode
         || (0x7F <= aCharCode && 0x9F >= aCharCode));
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
                    nsCOMPtr<nsIDOMTouch>& aData,
                    void *aTouchList)
  {
    nsTArray<nsCOMPtr<nsIDOMTouch> > *touches =
              static_cast<nsTArray<nsCOMPtr<nsIDOMTouch> > *>(aTouchList);
    touches->AppendElement(aData);
    aData->mChanged = false;
    return PL_DHASH_NEXT;
  }

  
  
  
  
  
  
  
  
  union LParamForKeyEvents {
    uintptr_t lParam;
    struct {
      
      uint16_t repeatCount;
      
      uint8_t scanCode;
      
      uint8_t flags;
    } parts;
  };
  
  
  enum LParamFlagsForKeyEvents {
    
    isExtendedKey = 1,
    
    
    
    
    isMenuKeyDown = 1<<5,
    
    
    wasKeyDown = 1<<6,
    
    
    
    isKeyReleased = 1<<7
  };
  
  
  void
  InitPluginKeyEventLParamFromKeyStatus(
                      uintptr_t& aLParam,
                      UI::Core::CorePhysicalKeyStatus const& aKeyStatus) {
    LParamForKeyEvents lParam;

    lParam.parts.repeatCount = aKeyStatus.RepeatCount;
    lParam.parts.scanCode = aKeyStatus.ScanCode;
    if (aKeyStatus.IsExtendedKey) {
      lParam.parts.flags |= LParamFlagsForKeyEvents::isExtendedKey;
    }
    if (aKeyStatus.IsMenuKeyDown) {
      lParam.parts.flags |= LParamFlagsForKeyEvents::isMenuKeyDown;
    }
    if (aKeyStatus.WasKeyDown) {
      lParam.parts.flags |= LParamFlagsForKeyEvents::wasKeyDown;
    }
    if (aKeyStatus.IsKeyReleased) {
      lParam.parts.flags |= LParamFlagsForKeyEvents::isKeyReleased;
    }

    aLParam = lParam.lParam;
  }

  
  
  
  
  
  
  
  
  union LParamForMouseEvents {
    uintptr_t lParam;
    
    struct lParamDeconstruction {
      
      uint16_t x;
      
      uint16_t y;
    } parts;
  };
  
  
  void
  InitPluginMouseEventParams(nsInputEvent const& aEvent,
                             uintptr_t& aWParam,
                             uintptr_t& aLParam) {
    
    aWParam = 0;
    if (IS_VK_DOWN(VK_LBUTTON)) {
      aWParam |= MK_LBUTTON;
    }
    if (IS_VK_DOWN(VK_MBUTTON)) {
      aWParam |= MK_MBUTTON;
    }
    if (IS_VK_DOWN(VK_RBUTTON)) {
      aWParam |= MK_RBUTTON;
    }
    if (aEvent.IsControl()) {
      aWParam |= MK_CONTROL;
    }
    if (aEvent.IsShift()) {
      aWParam |= MK_SHIFT;
    }

    Foundation::Point logPoint = MetroUtils::PhysToLog(aEvent.refPoint);
    LParamForMouseEvents lParam;
    lParam.parts.x = static_cast<uint16_t>(NS_round(logPoint.X));
    lParam.parts.y = static_cast<uint16_t>(NS_round(logPoint.Y));
    aLParam = lParam.lParam;
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

  mTokenPointerPressed.value = 0;
  mTokenPointerReleased.value = 0;
  mTokenPointerMoved.value = 0;
  mTokenPointerEntered.value = 0;
  mTokenPointerExited.value = 0;
  mTokenPointerWheelChanged.value = 0;
  mTokenAcceleratorKeyActivated.value = 0;
  mTokenEdgeGesture.value = 0;
  mTokenManipulationStarted.value = 0;
  mTokenManipulationUpdated.value = 0;
  mTokenManipulationCompleted.value = 0;
  mTokenTapped.value = 0;
  mTokenRightTapped.value = 0;

  mTouches.Init();

  
  if (!sIsVirtualKeyMapInitialized) {
    InitializeVirtualKeyMap();
    sIsVirtualKeyMapInitialized = true;
  }

  
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
MetroInput::OnAcceleratorKeyActivated(UI::Core::ICoreDispatcher* sender,
                                      UI::Core::IAcceleratorKeyEventArgs* aArgs) {
  UI::Core::CoreAcceleratorKeyEventType type;
  System::VirtualKey vkey;
  UI::Core::CorePhysicalKeyStatus keyStatus;

  aArgs->get_EventType(&type);
  aArgs->get_VirtualKey(&vkey);
  aArgs->get_KeyStatus(&keyStatus);

#ifdef DEBUG_INPUT
  LogFunction();
  Log(L"Accelerator key! Type: %d Value: %d", type, vkey);
#endif

  switch(type) {
    case UI::Core::CoreAcceleratorKeyEventType_KeyUp:
    case UI::Core::CoreAcceleratorKeyEventType_SystemKeyUp:
      OnKeyUp(vkey, keyStatus);
      break;
    case UI::Core::CoreAcceleratorKeyEventType_KeyDown:
    case UI::Core::CoreAcceleratorKeyEventType_SystemKeyDown:
      OnKeyDown(vkey, keyStatus);
      break;
    case UI::Core::CoreAcceleratorKeyEventType_Character:
    case UI::Core::CoreAcceleratorKeyEventType_SystemCharacter:
    case UI::Core::CoreAcceleratorKeyEventType_UnicodeCharacter:
      OnCharacterReceived(vkey, keyStatus);
      break;
  }

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
                                  NS_SIMPLE_GESTURE_EDGEUI,
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

  NPEvent pluginEvent;
  pluginEvent.event = horzEvent ? WM_MOUSEHWHEEL : WM_MOUSEWHEEL;

  union {
    uintptr_t wParam;
    uint16_t parts[2];
  } wParam;

  
  InitPluginMouseEventParams(wheelEvent,
                             wParam.wParam,
                             pluginEvent.lParam);

  
  
  
  
  wParam.parts[1] = horzEvent
                  ? wheelEvent.lineOrPageDeltaX
                  : wheelEvent.lineOrPageDeltaY * -1;

  pluginEvent.wParam = wParam.wParam;

  wheelEvent.pluginEvent = static_cast<void*>(&pluginEvent);
  DispatchEventIgnoreStatus(&wheelEvent);

  WRL::ComPtr<UI::Input::IPointerPoint> point;
  aArgs->get_CurrentPoint(point.GetAddressOf());
  mGestureRecognizer->ProcessMouseWheelEvent(point.Get(),
                                             wheelEvent.IsShift(),
                                             wheelEvent.IsControl());
  return S_OK;
}




void
MetroInput::OnCharacterReceived(uint32_t aCharCode,
                                UI::Core::CorePhysicalKeyStatus const& aKeyStatus)
{
  
  if (IsControlCharacter(aCharCode)) {
    return;
  }

  nsKeyEvent keyEvent(true, NS_KEY_PRESS, mWidget.Get());
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(keyEvent);
  keyEvent.time = ::GetMessageTime();
  keyEvent.isChar = true;
  keyEvent.charCode = aCharCode;

  NPEvent pluginEvent;
  pluginEvent.event = WM_CHAR;
  InitPluginKeyEventLParamFromKeyStatus(pluginEvent.lParam,
                                        aKeyStatus);
  
  
  
  
  if (IS_IN_BMP(aCharCode)) {
    pluginEvent.wParam = aCharCode;
  } else {
    pluginEvent.wParam = H_SURROGATE(aCharCode);
    nsPluginEvent surrogateEvent(true, NS_PLUGIN_INPUT_EVENT, mWidget.Get());
    surrogateEvent.time = ::GetMessageTime();
    surrogateEvent.pluginEvent = static_cast<void*>(&pluginEvent);
    DispatchEventIgnoreStatus(&surrogateEvent);
    pluginEvent.wParam = L_SURROGATE(aCharCode);
  }

  keyEvent.pluginEvent = static_cast<void*>(&pluginEvent);

  DispatchEventIgnoreStatus(&keyEvent);
}



void
MetroInput::OnKeyDown(uint32_t aVKey,
                      UI::Core::CorePhysicalKeyStatus const& aKeyStatus)
{
  
  
  uint32_t mozKey = GetMozKeyCode(aVKey);
  if (!mozKey) {
    return;
  }

  nsKeyEvent keyEvent(true, NS_KEY_DOWN, mWidget.Get());
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(keyEvent);
  keyEvent.time = ::GetMessageTime();
  keyEvent.keyCode = mozKey;

  NPEvent pluginEvent;
  pluginEvent.event = WM_KEYDOWN;
  InitPluginKeyEventLParamFromKeyStatus(pluginEvent.lParam,
                                        aKeyStatus);
  
  pluginEvent.wParam = aVKey;

  keyEvent.pluginEvent = static_cast<void*>(&pluginEvent);
  DispatchEventIgnoreStatus(&keyEvent);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  keyEvent.charCode = MapVirtualKey(aVKey, MAPVK_VK_TO_CHAR);
  keyEvent.isChar = !IsControlCharacter(keyEvent.charCode);
  if (!keyEvent.isChar
   || mModifierKeyState.IsControl()) {
    
    
    keyEvent.pluginEvent = nullptr;
    keyEvent.message = NS_KEY_PRESS;
    DispatchEventIgnoreStatus(&keyEvent);
  }
}



void
MetroInput::OnKeyUp(uint32_t aVKey,
                    UI::Core::CorePhysicalKeyStatus const& aKeyStatus)
{
  uint32_t mozKey = GetMozKeyCode(aVKey);
  if (!mozKey) {
    return;
  }

  nsKeyEvent keyEvent(true, NS_KEY_UP, mWidget.Get());
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(keyEvent);
  keyEvent.time = ::GetMessageTime();
  keyEvent.keyCode = mozKey;

  NPEvent pluginEvent;
  pluginEvent.event = WM_KEYUP;
  InitPluginKeyEventLParamFromKeyStatus(pluginEvent.lParam,
                                        aKeyStatus);
  
  pluginEvent.wParam = aVKey;

  keyEvent.pluginEvent = static_cast<void*>(&pluginEvent);
  DispatchEventIgnoreStatus(&keyEvent);
}










void
MetroInput::OnPointerNonTouch(UI::Input::IPointerPoint* aPoint) {
  WRL::ComPtr<UI::Input::IPointerPointProperties> props;
  UI::Input::PointerUpdateKind pointerUpdateKind;
  boolean canBeDoubleTap;

  aPoint->get_Properties(props.GetAddressOf());
  props->get_PointerUpdateKind(&pointerUpdateKind);
  mGestureRecognizer->CanBeDoubleTap(aPoint, &canBeDoubleTap);

  nsMouseEvent mouseEvent(true,
                          NS_MOUSE_MOVE,
                          mWidget.Get(),
                          nsMouseEvent::eReal,
                          nsMouseEvent::eNormal);

  NPEvent pluginEvent;
  pluginEvent.event = WM_MOUSEMOVE;

  switch (pointerUpdateKind) {
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonPressed:
      
      
      mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
      pluginEvent.event = (canBeDoubleTap
                        ? WM_LBUTTONDBLCLK
                        : WM_LBUTTONDOWN);
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonPressed:
      mouseEvent.button = nsMouseEvent::buttonType::eMiddleButton;
      mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
      pluginEvent.event = (canBeDoubleTap
                        ? WM_MBUTTONDBLCLK
                        : WM_MBUTTONDOWN);
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonPressed:
      mouseEvent.button = nsMouseEvent::buttonType::eRightButton;
      mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
      pluginEvent.event = (canBeDoubleTap
                        ? WM_RBUTTONDBLCLK
                        : WM_RBUTTONDOWN);
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_LeftButtonReleased:
      
      
      mouseEvent.message = NS_MOUSE_BUTTON_UP;
      pluginEvent.event = WM_LBUTTONUP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_MiddleButtonReleased:
      mouseEvent.button = nsMouseEvent::buttonType::eMiddleButton;
      mouseEvent.message = NS_MOUSE_BUTTON_UP;
      pluginEvent.event = WM_MBUTTONUP;
      break;
    case UI::Input::PointerUpdateKind::PointerUpdateKind_RightButtonReleased:
      mouseEvent.button = nsMouseEvent::buttonType::eRightButton;
      mouseEvent.message = NS_MOUSE_BUTTON_UP;
      pluginEvent.event = WM_RBUTTONUP;
      break;
  }
  InitGeckoMouseEventFromPointerPoint(mouseEvent, aPoint);

  mouseEvent.pluginEvent = static_cast<void*>(&pluginEvent);
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
  nsCOMPtr<nsIDOMTouch> touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  mTouches.Put(pointerId, touch);
  mTouchEvent.message = NS_TOUCH_START;

  
  
  if (mTouches.Count() == 1) {
    nsEventStatus status;
    DispatchPendingTouchEvent(status);
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
  nsCOMPtr<nsIDOMTouch> touch = mTouches.Get(pointerId);

  
  
  
  if (touch->mChanged) {
    DispatchPendingTouchEvent();
  }
  mTouches.Remove(pointerId);

  
  mTouchEvent.message = NS_TOUCH_END;
  mTouchEvent.touches.Clear();
  mTouchEvent.touches.AppendElement(CreateDOMTouch(currentPoint.Get()));
  mTouchEvent.time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(mTouchEvent);
  DispatchEventIgnoreStatus(&mTouchEvent);
  
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
  nsCOMPtr<nsIDOMTouch> touch = mTouches.Get(pointerId);

  
  
  
  
  if (!touch) {
    return S_OK;
  }

  
  
  if (touch->mChanged) {
    DispatchPendingTouchEvent();
  }

  touch = CreateDOMTouch(currentPoint.Get());
  touch->mChanged = true;
  mTouches.Put(pointerId, touch);

  
  
  if (mIsFirstTouchMove) {
    nsEventStatus status;
    DispatchPendingTouchEvent(status);
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

  
  
  
  if (deviceType ==
              Devices::Input::PointerDeviceType::PointerDeviceType_Touch) {
    Foundation::Point position;
    aArgs->get_Position(&position);

    
    
    nsMouseEvent mouseEvent(true,
                            NS_MOUSE_MOVE,
                            mWidget.Get(),
                            nsMouseEvent::eReal,
                            nsMouseEvent::eNormal);
    mModifierKeyState.Update();
    mModifierKeyState.InitInputEvent(mouseEvent);
    mouseEvent.refPoint = MetroUtils::LogToPhys(position);
    mouseEvent.time = ::GetMessageTime();
    aArgs->get_TapCount(&mouseEvent.clickCount);
    mouseEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

    
    DispatchEventIgnoreStatus(&mouseEvent);

    
    mouseEvent.message = NS_MOUSE_BUTTON_DOWN;
    mouseEvent.button = nsMouseEvent::buttonType::eLeftButton;
    DispatchEventIgnoreStatus(&mouseEvent);

    
    mouseEvent.message = NS_MOUSE_BUTTON_UP;
    DispatchEventIgnoreStatus(&mouseEvent);
  }

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
  Foundation::Point position;

  aArgs->get_PointerDeviceType(&deviceType);
  aArgs->get_Position(&position);

  nsMouseEvent contextMenu(true,
                           NS_CONTEXTMENU,
                           mWidget.Get(),
                           nsMouseEvent::eReal,
                           nsMouseEvent::eNormal);
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(contextMenu);
  contextMenu.refPoint = MetroUtils::LogToPhys(position);
  contextMenu.time = ::GetMessageTime();
  MozInputSourceFromDeviceType(deviceType, contextMenu.inputSource);
  DispatchEventIgnoreStatus(&contextMenu);

  return S_OK;
}




nsEventStatus MetroInput::sThrowawayStatus;





void
MetroInput::DispatchEventIgnoreStatus(nsGUIEvent *aEvent) {
  mWidget->DispatchEvent(aEvent, sThrowawayStatus);
}

void
MetroInput::DispatchPendingTouchEvent(nsEventStatus& aStatus) {
  mTouchEvent.touches.Clear();
  mTouches.Enumerate(&AppendToTouchList,
                     static_cast<void*>(&mTouchEvent.touches));
  mTouchEvent.time = ::GetMessageTime();
  mModifierKeyState.Update();
  mModifierKeyState.InitInputEvent(mTouchEvent);
  mWidget->DispatchEvent(&mTouchEvent, aStatus);

  
  mTouchEvent.message = NS_TOUCH_MOVE;
}

void
MetroInput::DispatchPendingTouchEvent() {
  DispatchPendingTouchEvent(sThrowawayStatus);
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
      edge->remove_Completed(mTokenEdgeGesture);
    }
  }

  
  WRL::ComPtr<ICoreAcceleratorKeys> coreAcceleratorKeys;
  if (SUCCEEDED(mDispatcher.As<ICoreAcceleratorKeys>(&coreAcceleratorKeys))) {
    coreAcceleratorKeys->remove_AcceleratorKeyActivated(
                            mTokenAcceleratorKeyActivated);
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



uint32_t MetroInput::sVirtualKeyMap[255];
bool MetroInput::sIsVirtualKeyMapInitialized = false;




void
MetroInput::InitializeVirtualKeyMap() {
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Cancel] = NS_VK_CANCEL;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Help] = NS_VK_HELP;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Back] = NS_VK_BACK;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Tab] = NS_VK_TAB;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Clear] = NS_VK_CLEAR;
  
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Enter] = NS_VK_RETURN;
  
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Shift] = NS_VK_SHIFT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Control] = NS_VK_CONTROL;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Menu] = NS_VK_ALT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Pause] = NS_VK_PAUSE;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_CapitalLock] = NS_VK_CAPS_LOCK;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Kana] = NS_VK_KANA;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Hangul] = NS_VK_HANGUL;
  
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Junja] = NS_VK_JUNJA;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Final] = NS_VK_FINAL;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Hanja] = NS_VK_HANJA;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Kanji] = NS_VK_KANJI;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Escape] = NS_VK_ESCAPE;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Convert] = NS_VK_CONVERT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NonConvert] = NS_VK_NONCONVERT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Accept] = NS_VK_ACCEPT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_ModeChange] = NS_VK_MODECHANGE;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Space] = NS_VK_SPACE;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_PageUp] = NS_VK_PAGE_UP;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_PageDown] = NS_VK_PAGE_DOWN;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_End] = NS_VK_END;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Home] = NS_VK_HOME;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Left] = NS_VK_LEFT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Up] = NS_VK_UP;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Right] = NS_VK_RIGHT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Down] = NS_VK_DOWN;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Select] = NS_VK_SELECT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Print] = NS_VK_PRINT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Execute] = NS_VK_EXECUTE;
  sVirtualKeyMap[VK_SNAPSHOT] = NS_VK_PRINTSCREEN;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Insert] = NS_VK_INSERT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Delete] = NS_VK_DELETE;

  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number0] = NS_VK_0;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number1] = NS_VK_1;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number2] = NS_VK_2;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number3] = NS_VK_3;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number4] = NS_VK_4;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number5] = NS_VK_5;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number6] = NS_VK_6;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number7] = NS_VK_7;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number8] = NS_VK_8;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Number9] = NS_VK_9;

  
  sVirtualKeyMap[VK_OEM_1] = NS_VK_SEMICOLON;
  
  
  
  
  

  sVirtualKeyMap[System::VirtualKey::VirtualKey_A] = NS_VK_A;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_B] = NS_VK_B;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_C] = NS_VK_C;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_D] = NS_VK_D;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_E] = NS_VK_E;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F] = NS_VK_F;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_G] = NS_VK_G;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_H] = NS_VK_H;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_I] = NS_VK_I;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_J] = NS_VK_J;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_K] = NS_VK_K;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_L] = NS_VK_L;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_M] = NS_VK_M;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_N] = NS_VK_N;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_O] = NS_VK_O;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_P] = NS_VK_P;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Q] = NS_VK_Q;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_R] = NS_VK_R;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_S] = NS_VK_S;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_T] = NS_VK_T;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_U] = NS_VK_U;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_V] = NS_VK_V;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_W] = NS_VK_W;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_X] = NS_VK_X;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Y] = NS_VK_Y;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Z] = NS_VK_Z;

  sVirtualKeyMap[System::VirtualKey::VirtualKey_LeftWindows] = NS_VK_WIN;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_RightWindows] = NS_VK_WIN;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_LeftMenu] = NS_VK_CONTEXT_MENU;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_RightMenu] = NS_VK_CONTEXT_MENU;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Sleep] = NS_VK_SLEEP;

  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad0] = NS_VK_NUMPAD0;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad1] = NS_VK_NUMPAD1;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad2] = NS_VK_NUMPAD2;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad3] = NS_VK_NUMPAD3;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad4] = NS_VK_NUMPAD4;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad5] = NS_VK_NUMPAD5;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad6] = NS_VK_NUMPAD6;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad7] = NS_VK_NUMPAD7;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad8] = NS_VK_NUMPAD8;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberPad9] = NS_VK_NUMPAD9;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Multiply] = NS_VK_MULTIPLY;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Add] = NS_VK_ADD;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Separator] = NS_VK_SEPARATOR;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Subtract] = NS_VK_SUBTRACT;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Decimal] = NS_VK_DECIMAL;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Divide] = NS_VK_DIVIDE;

  sVirtualKeyMap[System::VirtualKey::VirtualKey_F1] = NS_VK_F1;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F2] = NS_VK_F2;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F3] = NS_VK_F3;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F4] = NS_VK_F4;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F5] = NS_VK_F5;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F6] = NS_VK_F6;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F7] = NS_VK_F7;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F8] = NS_VK_F8;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F9] = NS_VK_F9;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F10] = NS_VK_F10;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F11] = NS_VK_F11;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F12] = NS_VK_F12;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F13] = NS_VK_F13;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F14] = NS_VK_F14;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F15] = NS_VK_F15;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F16] = NS_VK_F16;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F17] = NS_VK_F17;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F18] = NS_VK_F18;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F19] = NS_VK_F19;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F20] = NS_VK_F20;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F21] = NS_VK_F21;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F22] = NS_VK_F22;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F23] = NS_VK_F23;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_F24] = NS_VK_F24;

  sVirtualKeyMap[System::VirtualKey::VirtualKey_NumberKeyLock] = NS_VK_NUM_LOCK;
  sVirtualKeyMap[System::VirtualKey::VirtualKey_Scroll] = NS_VK_SCROLL_LOCK;

  
  
  
  
  
  
  
  
  
  
  
  sVirtualKeyMap[VK_OEM_PLUS] = NS_VK_PLUS;
  
  sVirtualKeyMap[VK_OEM_MINUS] = NS_VK_HYPHEN_MINUS;

  
  

  

  sVirtualKeyMap[VK_OEM_COMMA] = NS_VK_COMMA;
  sVirtualKeyMap[VK_OEM_PERIOD] = NS_VK_PERIOD;
  sVirtualKeyMap[VK_OEM_2] = NS_VK_SLASH;
  sVirtualKeyMap[VK_OEM_3] = NS_VK_BACK_QUOTE;
  sVirtualKeyMap[VK_OEM_4] = NS_VK_OPEN_BRACKET;
  sVirtualKeyMap[VK_OEM_5] = NS_VK_BACK_SLASH;
  sVirtualKeyMap[VK_OEM_6] = NS_VK_CLOSE_BRACKET;
  sVirtualKeyMap[VK_OEM_7] = NS_VK_QUOTE;

  
  
}

uint32_t
MetroInput::GetMozKeyCode(uint32_t aKey)
{
  return sVirtualKeyMap[aKey];
}

void
MetroInput::RegisterInputEvents()
{
  NS_ASSERTION(mWindow, "Must have a window to register for input events!");
  NS_ASSERTION(mGestureRecognizer,
               "Must have a GestureRecognizer for input events!");
  NS_ASSERTION(mDispatcher,
               "Must have a CoreDispatcher to register for input events!");

  
  
  
  WRL::ComPtr<ICoreAcceleratorKeys> coreAcceleratorKeys;
  mDispatcher.As<ICoreAcceleratorKeys>(&coreAcceleratorKeys);
  coreAcceleratorKeys->add_AcceleratorKeyActivated(
      WRL::Callback<AcceleratorKeyActivatedHandler>(
                                  this,
                                  &MetroInput::OnAcceleratorKeyActivated).Get(),
      &mTokenAcceleratorKeyActivated);

  
  WRL::ComPtr<UI::Input::IEdgeGestureStatics> edgeStatics;
  Foundation::GetActivationFactory(
            WRL::Wrappers::HStringReference(
                    RuntimeClass_Windows_UI_Input_EdgeGesture)
            .Get(),
            edgeStatics.GetAddressOf());
  WRL::ComPtr<UI::Input::IEdgeGesture> edge;
  edgeStatics->GetForCurrentView(edge.GetAddressOf());

  edge->add_Completed(
      WRL::Callback<EdgeGestureHandler>(
                                  this,
                                  &MetroInput::OnEdgeGestureCompleted).Get(),
      &mTokenEdgeGesture);

  
  
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
