





#include "mozilla/DebugOnly.h"

#include "prlog.h"

#include "WinMouseScrollHandler.h"
#include "nsWindow.h"
#include "nsWindowDefs.h"
#include "KeyboardLayout.h"
#include "WinUtils.h"
#include "nsGkAtoms.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDOMWheelEvent.h"

#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/WindowsVersion.h"

#include <psapi.h>

namespace mozilla {
namespace widget {

#ifdef PR_LOGGING
PRLogModuleInfo* gMouseScrollLog = nullptr;

static const char* GetBoolName(bool aBool)
{
  return aBool ? "TRUE" : "FALSE";
}

static void LogKeyStateImpl()
{
  if (!PR_LOG_TEST(gMouseScrollLog, PR_LOG_DEBUG)) {
    return;
  }
  BYTE keyboardState[256];
  if (::GetKeyboardState(keyboardState)) {
    for (size_t i = 0; i < ArrayLength(keyboardState); i++) {
      if (keyboardState[i]) {
        PR_LOG(gMouseScrollLog, PR_LOG_DEBUG,
          ("    Current key state: keyboardState[0x%02X]=0x%02X (%s)",
           i, keyboardState[i],
           ((keyboardState[i] & 0x81) == 0x81) ? "Pressed and Toggled" :
           (keyboardState[i] & 0x80) ? "Pressed" :
           (keyboardState[i] & 0x01) ? "Toggled" : "Unknown"));
      }
    }
  } else {
    PR_LOG(gMouseScrollLog, PR_LOG_DEBUG,
      ("MouseScroll::Device::Elantech::HandleKeyMessage(): Failed to print "
       "current keyboard state"));
  }
}

#define LOG_KEYSTATE() LogKeyStateImpl()
#else 
#define LOG_KEYSTATE()
#endif

MouseScrollHandler* MouseScrollHandler::sInstance = nullptr;

bool MouseScrollHandler::Device::sFakeScrollableWindowNeeded = false;

bool MouseScrollHandler::Device::Elantech::sUseSwipeHack = false;
bool MouseScrollHandler::Device::Elantech::sUsePinchHack = false;
DWORD MouseScrollHandler::Device::Elantech::sZoomUntil = 0;

bool MouseScrollHandler::Device::SetPoint::sMightBeUsing = false;




#define DEFAULT_TIMEOUT_DURATION 1500








POINTS
MouseScrollHandler::GetCurrentMessagePos()
{
  if (SynthesizingEvent::IsSynthesizing()) {
    return sInstance->mSynthesizingEvent->GetCursorPoint();
  }
  DWORD pos = ::GetMessagePos();
  return MAKEPOINTS(pos);
}


#define GetMessagePos()


void
MouseScrollHandler::Initialize()
{
#ifdef PR_LOGGING
  if (!gMouseScrollLog) {
    gMouseScrollLog = PR_NewLogModule("MouseScrollHandlerWidgets");
  }
#endif
  Device::Init();
}


void
MouseScrollHandler::Shutdown()
{
  delete sInstance;
  sInstance = nullptr;
}


MouseScrollHandler*
MouseScrollHandler::GetInstance()
{
  if (!sInstance) {
    sInstance = new MouseScrollHandler();
  }
  return sInstance;
}

MouseScrollHandler::MouseScrollHandler() :
  mIsWaitingInternalMessage(false),
  mSynthesizingEvent(nullptr)
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll: Creating an instance, this=%p, sInstance=%p",
     this, sInstance));
}

MouseScrollHandler::~MouseScrollHandler()
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll: Destroying an instance, this=%p, sInstance=%p",
     this, sInstance));

  delete mSynthesizingEvent;
}


bool
MouseScrollHandler::NeedsMessage(UINT aMsg)
{
  switch (aMsg) {
    case WM_SETTINGCHANGE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_HSCROLL:
    case WM_VSCROLL:
    case MOZ_WM_MOUSEVWHEEL:
    case MOZ_WM_MOUSEHWHEEL:
    case MOZ_WM_HSCROLL:
    case MOZ_WM_VSCROLL:
    case WM_KEYDOWN:
    case WM_KEYUP:
      return true;
  }
  return false;
}


bool
MouseScrollHandler::ProcessMessage(nsWindowBase* aWidget, UINT msg,
                                   WPARAM wParam, LPARAM lParam,
                                   MSGResult& aResult)
{
  Device::Elantech::UpdateZoomUntil();

  switch (msg) {
    case WM_SETTINGCHANGE:
      if (!sInstance) {
        return false;
      }
      if (wParam == SPI_SETWHEELSCROLLLINES ||
          wParam == SPI_SETWHEELSCROLLCHARS) {
        sInstance->mSystemSettings.MarkDirty();
      }
      return false;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      GetInstance()->
        ProcessNativeMouseWheelMessage(aWidget, msg, wParam, lParam);
      sInstance->mSynthesizingEvent->NotifyNativeMessageHandlingFinished();
      
      
      
      
      aResult.mConsumed = true;
      aResult.mResult = (msg != WM_MOUSEHWHEEL);
      return true;

    case WM_HSCROLL:
    case WM_VSCROLL:
      aResult.mConsumed =
        GetInstance()->ProcessNativeScrollMessage(aWidget, msg, wParam, lParam);
      sInstance->mSynthesizingEvent->NotifyNativeMessageHandlingFinished();
      aResult.mResult = 0;
      return true;

    case MOZ_WM_MOUSEVWHEEL:
    case MOZ_WM_MOUSEHWHEEL:
      GetInstance()->HandleMouseWheelMessage(aWidget, msg, wParam, lParam);
      sInstance->mSynthesizingEvent->NotifyInternalMessageHandlingFinished();
      
      aResult.mConsumed = true;
      return true;

    case MOZ_WM_HSCROLL:
    case MOZ_WM_VSCROLL:
      GetInstance()->
        HandleScrollMessageAsMouseWheelMessage(aWidget, msg, wParam, lParam);
      sInstance->mSynthesizingEvent->NotifyInternalMessageHandlingFinished();
      
      aResult.mConsumed = true;
      return true;

    case WM_KEYDOWN:
    case WM_KEYUP:
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::ProcessMessage(): aWidget=%p, "
         "msg=%s(0x%04X), wParam=0x%02X, ::GetMessageTime()=%d",
         aWidget, msg == WM_KEYDOWN ? "WM_KEYDOWN" :
                    msg == WM_KEYUP ? "WM_KEYUP" : "Unknown", msg, wParam,
         ::GetMessageTime()));
      LOG_KEYSTATE();
      if (Device::Elantech::HandleKeyMessage(aWidget, msg, wParam)) {
        aResult.mResult = 0;
        aResult.mConsumed = true;
        return true;
      }
      return false;

    default:
      return false;
  }
}


nsresult
MouseScrollHandler::SynthesizeNativeMouseScrollEvent(nsWindowBase* aWidget,
                                                     const LayoutDeviceIntPoint& aPoint,
                                                     uint32_t aNativeMessage,
                                                     int32_t aDelta,
                                                     uint32_t aModifierFlags,
                                                     uint32_t aAdditionalFlags)
{
  bool useFocusedWindow =
    !(aAdditionalFlags & nsIDOMWindowUtils::MOUSESCROLL_PREFER_WIDGET_AT_POINT);

  POINT pt;
  pt.x = aPoint.x;
  pt.y = aPoint.y;

  HWND target = useFocusedWindow ? ::WindowFromPoint(pt) : ::GetFocus();
  NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);

  WPARAM wParam = 0;
  LPARAM lParam = 0;
  switch (aNativeMessage) {
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
      lParam = MAKELPARAM(pt.x, pt.y);
      WORD mod = 0;
      if (aModifierFlags & (nsIWidget::CTRL_L | nsIWidget::CTRL_R)) {
        mod |= MK_CONTROL;
      }
      if (aModifierFlags & (nsIWidget::SHIFT_L | nsIWidget::SHIFT_R)) {
        mod |= MK_SHIFT;
      }
      wParam = MAKEWPARAM(mod, aDelta);
      break;
    }
    case WM_VSCROLL:
    case WM_HSCROLL:
      lParam = (aAdditionalFlags &
                  nsIDOMWindowUtils::MOUSESCROLL_WIN_SCROLL_LPARAM_NOT_NULL) ?
        reinterpret_cast<LPARAM>(target) : 0;
      wParam = aDelta;
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }

  
  GetInstance();

  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));

  nsAutoTArray<KeyPair,10> keySequence;
  WinUtils::SetupKeyModifiersSequence(&keySequence, aModifierFlags);

  for (uint32_t i = 0; i < keySequence.Length(); ++i) {
    uint8_t key = keySequence[i].mGeneral;
    uint8_t keySpecific = keySequence[i].mSpecific;
    kbdState[key] = 0x81; 
    if (keySpecific) {
      kbdState[keySpecific] = 0x81;
    }
  }

  if (!sInstance->mSynthesizingEvent) {
    sInstance->mSynthesizingEvent = new SynthesizingEvent();
  }

  POINTS pts;
  pts.x = static_cast<SHORT>(pt.x);
  pts.y = static_cast<SHORT>(pt.y);
  return sInstance->mSynthesizingEvent->
           Synthesize(pts, target, aNativeMessage, wParam, lParam, kbdState);
}


void
MouseScrollHandler::InitEvent(nsWindowBase* aWidget,
                              WidgetGUIEvent& aEvent,
                              nsIntPoint* aPoint)
{
  NS_ENSURE_TRUE_VOID(aWidget);
  nsIntPoint point;
  if (aPoint) {
    point = *aPoint;
  } else {
    POINTS pts = GetCurrentMessagePos();
    POINT pt;
    pt.x = pts.x;
    pt.y = pts.y;
    ::ScreenToClient(aWidget->GetWindowHandle(), &pt);
    point.x = pt.x;
    point.y = pt.y;
  }
  aWidget->InitEvent(aEvent, &point);
}


ModifierKeyState
MouseScrollHandler::GetModifierKeyState(UINT aMessage)
{
  ModifierKeyState result;
  
  
  
  if ((aMessage == MOZ_WM_MOUSEVWHEEL || aMessage == WM_MOUSEWHEEL) &&
      !result.IsControl() && Device::Elantech::IsZooming()) {
    result.Set(MODIFIER_CONTROL);
  }
  return result;
}

POINT
MouseScrollHandler::ComputeMessagePos(UINT aMessage,
                                      WPARAM aWParam,
                                      LPARAM aLParam)
{
  POINT point;
  if (Device::SetPoint::IsGetMessagePosResponseValid(aMessage,
                                                     aWParam, aLParam)) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::ComputeMessagePos: Using ::GetCursorPos()"));
    ::GetCursorPos(&point);
  } else {
    POINTS pts = GetCurrentMessagePos();
    point.x = pts.x;
    point.y = pts.y;
  }
  return point;
}

void
MouseScrollHandler::ProcessNativeMouseWheelMessage(nsWindowBase* aWidget,
                                                   UINT aMessage,
                                                   WPARAM aWParam,
                                                   LPARAM aLParam)
{
  if (SynthesizingEvent::IsSynthesizing()) {
    mSynthesizingEvent->NativeMessageReceived(aWidget, aMessage,
                                              aWParam, aLParam);
  }

  POINT point = ComputeMessagePos(aMessage, aWParam, aLParam);

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::ProcessNativeMouseWheelMessage: aWidget=%p, "
     "aMessage=%s, wParam=0x%08X, lParam=0x%08X, point: { x=%d, y=%d }",
     aWidget, aMessage == WM_MOUSEWHEEL ? "WM_MOUSEWHEEL" :
              aMessage == WM_MOUSEHWHEEL ? "WM_MOUSEHWHEEL" :
              aMessage == WM_VSCROLL ? "WM_VSCROLL" : "WM_HSCROLL",
     aWParam, aLParam, point.x, point.y));
  LOG_KEYSTATE();

  HWND underCursorWnd = ::WindowFromPoint(point);
  if (!underCursorWnd) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::ProcessNativeMouseWheelMessage: "
       "No window is not found under the cursor"));
    return;
  }

  if (Device::Elantech::IsPinchHackNeeded() &&
      Device::Elantech::IsHelperWindow(underCursorWnd)) {
    
    
    
    
    underCursorWnd = WinUtils::FindOurWindowAtPoint(point);
    if (!underCursorWnd) {
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::ProcessNativeMouseWheelMessage: "
         "Our window is not found under the Elantech helper window"));
      return;
    }
  }

  
  
  
  if (WinUtils::IsOurProcessWindow(underCursorWnd)) {
    nsWindowBase* destWindow = WinUtils::GetNSWindowBasePtr(underCursorWnd);
    if (!destWindow) {
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::ProcessNativeMouseWheelMessage: "
         "Found window under the cursor isn't managed by nsWindow..."));
      HWND wnd = ::GetParent(underCursorWnd);
      for (; wnd; wnd = ::GetParent(wnd)) {
        destWindow = WinUtils::GetNSWindowBasePtr(wnd);
        if (destWindow) {
          break;
        }
      }
      if (!wnd) {
        PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
          ("MouseScroll::ProcessNativeMouseWheelMessage: Our window which is "
           "managed by nsWindow is not found under the cursor"));
        return;
      }
    }

    MOZ_ASSERT(destWindow, "destWindow must not be NULL");

    
    
    
    
    
    if (destWindow->IsPlugin()) {
      destWindow = destWindow->GetParentWindowBase(false);
      if (!destWindow) {
        PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
          ("MouseScroll::ProcessNativeMouseWheelMessage: "
           "Our window which is a parent of a plugin window is not found"));
        return;
      }
    }
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::ProcessNativeMouseWheelMessage: Succeeded, "
       "Posting internal message to an nsWindow (%p)...",
       destWindow));
    mIsWaitingInternalMessage = true;
    UINT internalMessage = WinUtils::GetInternalMessage(aMessage);
    ::PostMessage(destWindow->GetWindowHandle(), internalMessage,
                  aWParam, aLParam);
    return;
  }

  
  
  
  HWND pluginWnd = WinUtils::FindOurProcessWindow(underCursorWnd);
  if (!pluginWnd) {
    
    
    
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::ProcessNativeMouseWheelMessage: "
       "Our window is not found under the cursor"));
    return;
  }

  
  
  
  
  
  if (aWidget->IsPlugin() &&
      aWidget->GetWindowHandle() == pluginWnd) {
    nsWindowBase* destWindow = aWidget->GetParentWindowBase(false);
    if (!destWindow) {
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::ProcessNativeMouseWheelMessage: Our normal window which "
         "is a parent of this plugin window is not found"));
      return;
    }
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::ProcessNativeMouseWheelMessage: Succeeded, "
       "Posting internal message to an nsWindow (%p) which is parent of this "
       "plugin window...",
       destWindow));
    mIsWaitingInternalMessage = true;
    UINT internalMessage = WinUtils::GetInternalMessage(aMessage);
    ::PostMessage(destWindow->GetWindowHandle(), internalMessage,
                  aWParam, aLParam);
    return;
  }

  
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::ProcessNativeMouseWheelMessage: Succeeded, "
     "Redirecting the message to a window which is a plugin child window"));
  ::PostMessage(underCursorWnd, aMessage, aWParam, aLParam);
}

bool
MouseScrollHandler::ProcessNativeScrollMessage(nsWindowBase* aWidget,
                                               UINT aMessage,
                                               WPARAM aWParam,
                                               LPARAM aLParam)
{
  if (aLParam || mUserPrefs.IsScrollMessageHandledAsWheelMessage()) {
    
    
    ProcessNativeMouseWheelMessage(aWidget, aMessage, aWParam, aLParam);
    
    
    return true;
  }

  if (SynthesizingEvent::IsSynthesizing()) {
    mSynthesizingEvent->NativeMessageReceived(aWidget, aMessage,
                                              aWParam, aLParam);
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::ProcessNativeScrollMessage: aWidget=%p, "
     "aMessage=%s, wParam=0x%08X, lParam=0x%08X",
     aWidget, aMessage == WM_VSCROLL ? "WM_VSCROLL" : "WM_HSCROLL",
     aWParam, aLParam));

  
  WidgetContentCommandEvent commandEvent(true, NS_CONTENT_COMMAND_SCROLL,
                                         aWidget);

  commandEvent.mScroll.mIsHorizontal = (aMessage == WM_HSCROLL);

  switch (LOWORD(aWParam)) {
    case SB_LINEUP:   
      commandEvent.mScroll.mUnit =
        WidgetContentCommandEvent::eCmdScrollUnit_Line;
      commandEvent.mScroll.mAmount = -1;
      break;
    case SB_LINEDOWN: 
      commandEvent.mScroll.mUnit =
        WidgetContentCommandEvent::eCmdScrollUnit_Line;
      commandEvent.mScroll.mAmount = 1;
      break;
    case SB_PAGEUP:   
      commandEvent.mScroll.mUnit =
        WidgetContentCommandEvent::eCmdScrollUnit_Page;
      commandEvent.mScroll.mAmount = -1;
      break;
    case SB_PAGEDOWN: 
      commandEvent.mScroll.mUnit =
        WidgetContentCommandEvent::eCmdScrollUnit_Page;
      commandEvent.mScroll.mAmount = 1;
      break;
    case SB_TOP:      
      commandEvent.mScroll.mUnit =
        WidgetContentCommandEvent::eCmdScrollUnit_Whole;
      commandEvent.mScroll.mAmount = -1;
      break;
    case SB_BOTTOM:   
      commandEvent.mScroll.mUnit =
        WidgetContentCommandEvent::eCmdScrollUnit_Whole;
      commandEvent.mScroll.mAmount = 1;
      break;
    default:
      return false;
  }
  
  
  aWidget->DispatchContentCommandEvent(&commandEvent);
  return true;
}

void
MouseScrollHandler::HandleMouseWheelMessage(nsWindowBase* aWidget,
                                            UINT aMessage,
                                            WPARAM aWParam,
                                            LPARAM aLParam)
{
  MOZ_ASSERT(
    (aMessage == MOZ_WM_MOUSEVWHEEL || aMessage == MOZ_WM_MOUSEHWHEEL),
    "HandleMouseWheelMessage must be called with "
    "MOZ_WM_MOUSEVWHEEL or MOZ_WM_MOUSEHWHEEL");

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::HandleMouseWheelMessage: aWidget=%p, "
     "aMessage=MOZ_WM_MOUSE%sWHEEL, aWParam=0x%08X, aLParam=0x%08X",
     aWidget, aMessage == MOZ_WM_MOUSEVWHEEL ? "V" : "H",
     aWParam, aLParam));

  mIsWaitingInternalMessage = false;

  EventInfo eventInfo(aWidget, WinUtils::GetNativeMessage(aMessage),
                      aWParam, aLParam);
  if (!eventInfo.CanDispatchWheelEvent()) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::HandleMouseWheelMessage: Cannot dispatch the events"));
    mLastEventInfo.ResetTransaction();
    return;
  }

  
  
  
  if (!mLastEventInfo.CanContinueTransaction(eventInfo)) {
    mLastEventInfo.ResetTransaction();
  }

  mLastEventInfo.RecordEvent(eventInfo);

  ModifierKeyState modKeyState = GetModifierKeyState(aMessage);

  
  nsRefPtr<nsWindowBase> kungFuDethGrip(aWidget);

  WidgetWheelEvent wheelEvent(true, NS_WHEEL_WHEEL, aWidget);
  if (mLastEventInfo.InitWheelEvent(aWidget, wheelEvent, modKeyState)) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::HandleMouseWheelMessage: dispatching "
       "NS_WHEEL_WHEEL event"));
    aWidget->DispatchWheelEvent(&wheelEvent);
    if (aWidget->Destroyed()) {
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::HandleMouseWheelMessage: The window was destroyed "
         "by NS_WHEEL_WHEEL event"));
      mLastEventInfo.ResetTransaction();
      return;
    }
  }
#ifdef PR_LOGGING
  else {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::HandleMouseWheelMessage: NS_WHEEL_WHEEL event is not "
       "dispatched"));
  }
#endif
}

void
MouseScrollHandler::HandleScrollMessageAsMouseWheelMessage(nsWindowBase* aWidget,
                                                           UINT aMessage,
                                                           WPARAM aWParam,
                                                           LPARAM aLParam)
{
  MOZ_ASSERT(
    (aMessage == MOZ_WM_VSCROLL || aMessage == MOZ_WM_HSCROLL),
    "HandleScrollMessageAsMouseWheelMessage must be called with "
    "MOZ_WM_VSCROLL or MOZ_WM_HSCROLL");

  mIsWaitingInternalMessage = false;

  ModifierKeyState modKeyState = GetModifierKeyState(aMessage);

  WidgetWheelEvent wheelEvent(true, NS_WHEEL_WHEEL, aWidget);
  double& delta =
   (aMessage == MOZ_WM_VSCROLL) ? wheelEvent.deltaY : wheelEvent.deltaX;
  int32_t& lineOrPageDelta =
   (aMessage == MOZ_WM_VSCROLL) ? wheelEvent.lineOrPageDeltaY :
                                  wheelEvent.lineOrPageDeltaX;

  delta = 1.0;
  lineOrPageDelta = 1;

  switch (LOWORD(aWParam)) {
    case SB_PAGEUP:
      delta = -1.0;
      lineOrPageDelta = -1;
    case SB_PAGEDOWN:
      wheelEvent.deltaMode = nsIDOMWheelEvent::DOM_DELTA_PAGE;
      break;

    case SB_LINEUP:
      delta = -1.0;
      lineOrPageDelta = -1;
    case SB_LINEDOWN:
      wheelEvent.deltaMode = nsIDOMWheelEvent::DOM_DELTA_LINE;
      break;

    default:
      return;
  }
  modKeyState.InitInputEvent(wheelEvent);
  
  
  
  InitEvent(aWidget, wheelEvent);

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::HandleScrollMessageAsMouseWheelMessage: aWidget=%p, "
     "aMessage=MOZ_WM_%sSCROLL, aWParam=0x%08X, aLParam=0x%08X, "
     "wheelEvent { refPoint: { x: %d, y: %d }, deltaX: %f, deltaY: %f, "
     "lineOrPageDeltaX: %d, lineOrPageDeltaY: %d, "
     "isShift: %s, isControl: %s, isAlt: %s, isMeta: %s }",
     aWidget, (aMessage == MOZ_WM_VSCROLL) ? "V" : "H", aWParam, aLParam,
     wheelEvent.refPoint.x, wheelEvent.refPoint.y,
     wheelEvent.deltaX, wheelEvent.deltaY,
     wheelEvent.lineOrPageDeltaX, wheelEvent.lineOrPageDeltaY,
     GetBoolName(wheelEvent.IsShift()),
     GetBoolName(wheelEvent.IsControl()),
     GetBoolName(wheelEvent.IsAlt()),
     GetBoolName(wheelEvent.IsMeta())));

  aWidget->DispatchWheelEvent(&wheelEvent);
}







MouseScrollHandler::EventInfo::EventInfo(nsWindowBase* aWidget,
                                         UINT aMessage,
                                         WPARAM aWParam, LPARAM aLParam)
{
  MOZ_ASSERT(aMessage == WM_MOUSEWHEEL || aMessage == WM_MOUSEHWHEEL,
    "EventInfo must be initialized with WM_MOUSEWHEEL or WM_MOUSEHWHEEL");

  MouseScrollHandler::GetInstance()->mSystemSettings.Init();

  mIsVertical = (aMessage == WM_MOUSEWHEEL);
  mIsPage = MouseScrollHandler::sInstance->
              mSystemSettings.IsPageScroll(mIsVertical);
  mDelta = (short)HIWORD(aWParam);
  mWnd = aWidget->GetWindowHandle();
  mTimeStamp = TimeStamp::Now();
}

bool
MouseScrollHandler::EventInfo::CanDispatchWheelEvent() const
{
  if (!GetScrollAmount()) {
    
    
    
    return false;
  }

  return (mDelta != 0);
}

int32_t
MouseScrollHandler::EventInfo::GetScrollAmount() const
{
  if (mIsPage) {
    return 1;
  }
  return MouseScrollHandler::sInstance->
           mSystemSettings.GetScrollAmount(mIsVertical);
}







bool
MouseScrollHandler::LastEventInfo::CanContinueTransaction(
                                     const EventInfo& aNewEvent)
{
  int32_t timeout = MouseScrollHandler::sInstance->
                      mUserPrefs.GetMouseScrollTransactionTimeout();
  return !mWnd ||
           (mWnd == aNewEvent.GetWindowHandle() &&
            IsPositive() == aNewEvent.IsPositive() &&
            mIsVertical == aNewEvent.IsVertical() &&
            mIsPage == aNewEvent.IsPage() &&
            (timeout < 0 ||
             TimeStamp::Now() - mTimeStamp <=
               TimeDuration::FromMilliseconds(timeout)));
}

void
MouseScrollHandler::LastEventInfo::ResetTransaction()
{
  if (!mWnd) {
    return;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::LastEventInfo::ResetTransaction()"));

  mWnd = nullptr;
  mAccumulatedDelta = 0;
}

void
MouseScrollHandler::LastEventInfo::RecordEvent(const EventInfo& aEvent)
{
  mWnd = aEvent.GetWindowHandle();
  mDelta = aEvent.GetNativeDelta();
  mIsVertical = aEvent.IsVertical();
  mIsPage = aEvent.IsPage();
  mTimeStamp = TimeStamp::Now();
}


int32_t
MouseScrollHandler::LastEventInfo::RoundDelta(double aDelta)
{
  return (aDelta >= 0) ? (int32_t)floor(aDelta) : (int32_t)ceil(aDelta);
}

bool
MouseScrollHandler::LastEventInfo::InitWheelEvent(
                                     nsWindowBase* aWidget,
                                     WidgetWheelEvent& aWheelEvent,
                                     const ModifierKeyState& aModKeyState)
{
  MOZ_ASSERT(aWheelEvent.message == NS_WHEEL_WHEEL);

  
  
  
  InitEvent(aWidget, aWheelEvent);

  aModKeyState.InitInputEvent(aWheelEvent);

  
  
  
  int32_t orienter = mIsVertical ? -1 : 1;

  aWheelEvent.deltaMode = mIsPage ? nsIDOMWheelEvent::DOM_DELTA_PAGE :
                                    nsIDOMWheelEvent::DOM_DELTA_LINE;

  double& delta = mIsVertical ? aWheelEvent.deltaY : aWheelEvent.deltaX;
  int32_t& lineOrPageDelta = mIsVertical ? aWheelEvent.lineOrPageDeltaY :
                                           aWheelEvent.lineOrPageDeltaX;

  double nativeDeltaPerUnit =
    mIsPage ? static_cast<double>(WHEEL_DELTA) :
              static_cast<double>(WHEEL_DELTA) / GetScrollAmount();

  delta = static_cast<double>(mDelta) * orienter / nativeDeltaPerUnit;
  mAccumulatedDelta += mDelta;
  lineOrPageDelta =
    mAccumulatedDelta * orienter / RoundDelta(nativeDeltaPerUnit);
  mAccumulatedDelta -=
    lineOrPageDelta * orienter * RoundDelta(nativeDeltaPerUnit);

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::LastEventInfo::InitWheelEvent: aWidget=%p, "
     "aWheelEvent { refPoint: { x: %d, y: %d }, deltaX: %f, deltaY: %f, "
     "lineOrPageDeltaX: %d, lineOrPageDeltaY: %d, "
     "isShift: %s, isControl: %s, isAlt: %s, isMeta: %s }, "
     "mAccumulatedDelta: %d",
     aWidget, aWheelEvent.refPoint.x, aWheelEvent.refPoint.y,
     aWheelEvent.deltaX, aWheelEvent.deltaY,
     aWheelEvent.lineOrPageDeltaX, aWheelEvent.lineOrPageDeltaY,
     GetBoolName(aWheelEvent.IsShift()),
     GetBoolName(aWheelEvent.IsControl()),
     GetBoolName(aWheelEvent.IsAlt()),
     GetBoolName(aWheelEvent.IsMeta()), mAccumulatedDelta));

  return (delta != 0);
}







void
MouseScrollHandler::SystemSettings::Init()
{
  if (mInitialized) {
    return;
  }

  mInitialized = true;

  MouseScrollHandler::UserPrefs& userPrefs =
    MouseScrollHandler::sInstance->mUserPrefs;

  mScrollLines = userPrefs.GetOverriddenVerticalScrollAmout();
  if (mScrollLines >= 0) {
    
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): mScrollLines is overridden by "
       "the pref: %d",
       mScrollLines));
  } else if (!::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
                                     &mScrollLines, 0)) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): ::SystemParametersInfo("
         "SPI_GETWHEELSCROLLLINES) failed"));
    mScrollLines = 3;
  }

  if (mScrollLines > WHEEL_DELTA) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): the result of "
         "::SystemParametersInfo(SPI_GETWHEELSCROLLLINES) is too large: %d",
       mScrollLines));
    
    
    
    
    
    
    mScrollLines = WHEEL_PAGESCROLL;
  }

  mScrollChars = userPrefs.GetOverriddenHorizontalScrollAmout();
  if (mScrollChars >= 0) {
    
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): mScrollChars is overridden by "
       "the pref: %d",
       mScrollChars));
  } else if (!::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0,
                                     &mScrollChars, 0)) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): ::SystemParametersInfo("
         "SPI_GETWHEELSCROLLCHARS) failed, %s",
       IsVistaOrLater() ?
         "this is unexpected on Vista or later" :
         "but on XP or earlier, this is not a problem"));
    mScrollChars = 1;
  }

  if (mScrollChars > WHEEL_DELTA) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): the result of "
         "::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS) is too large: %d",
       mScrollChars));
    
    mScrollChars = WHEEL_PAGESCROLL;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::SystemSettings::Init(): initialized, "
       "mScrollLines=%d, mScrollChars=%d",
     mScrollLines, mScrollChars));
}

void
MouseScrollHandler::SystemSettings::MarkDirty()
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SystemSettings::MarkDirty(): "
       "Marking SystemSettings dirty"));
  mInitialized = false;
  
  MOZ_ASSERT(sInstance,
    "Must not be called at initializing MouseScrollHandler");
  MouseScrollHandler::sInstance->mLastEventInfo.ResetTransaction();
}







MouseScrollHandler::UserPrefs::UserPrefs() :
  mInitialized(false)
{
  
  
  DebugOnly<nsresult> rv =
    Preferences::RegisterCallback(OnChange, "mousewheel.", this);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
    "Failed to register callback for mousewheel.");
}

MouseScrollHandler::UserPrefs::~UserPrefs()
{
  DebugOnly<nsresult> rv =
    Preferences::UnregisterCallback(OnChange, "mousewheel.", this);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
    "Failed to unregister callback for mousewheel.");
}

void
MouseScrollHandler::UserPrefs::Init()
{
  if (mInitialized) {
    return;
  }

  mInitialized = true;

  mScrollMessageHandledAsWheelMessage =
    Preferences::GetBool("mousewheel.emulate_at_wm_scroll", false);
  mOverriddenVerticalScrollAmount =
    Preferences::GetInt("mousewheel.windows.vertical_amount_override", -1);
  mOverriddenHorizontalScrollAmount =
    Preferences::GetInt("mousewheel.windows.horizontal_amount_override", -1);
  mMouseScrollTransactionTimeout =
    Preferences::GetInt("mousewheel.windows.transaction.timeout",
                        DEFAULT_TIMEOUT_DURATION);

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::UserPrefs::Init(): initialized, "
       "mScrollMessageHandledAsWheelMessage=%s, "
       "mOverriddenVerticalScrollAmount=%d, "
       "mOverriddenHorizontalScrollAmount=%d, "
       "mMouseScrollTransactionTimeout=%d",
     GetBoolName(mScrollMessageHandledAsWheelMessage),
     mOverriddenVerticalScrollAmount, mOverriddenHorizontalScrollAmount,
     mMouseScrollTransactionTimeout));
}

void
MouseScrollHandler::UserPrefs::MarkDirty()
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::UserPrefs::MarkDirty(): Marking UserPrefs dirty"));
  mInitialized = false;
  
  MouseScrollHandler::sInstance->mSystemSettings.MarkDirty();
  
  
  MOZ_ASSERT(sInstance,
    "Must not be called at initializing MouseScrollHandler");
  MouseScrollHandler::sInstance->mLastEventInfo.ResetTransaction();
}








bool
MouseScrollHandler::Device::GetWorkaroundPref(const char* aPrefName,
                                              bool aValueIfAutomatic)
{
  if (!aPrefName) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::GetWorkaroundPref(): Failed, aPrefName is NULL"));
    return aValueIfAutomatic;
  }

  int32_t lHackValue = 0;
  if (NS_FAILED(Preferences::GetInt(aPrefName, &lHackValue))) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::GetWorkaroundPref(): Preferences::GetInt() failed,"
       " aPrefName=\"%s\", aValueIfAutomatic=%s",
       aPrefName, GetBoolName(aValueIfAutomatic)));
    return aValueIfAutomatic;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::Device::GetWorkaroundPref(): Succeeded, "
     "aPrefName=\"%s\", aValueIfAutomatic=%s, lHackValue=%d",
     aPrefName, GetBoolName(aValueIfAutomatic), lHackValue));

  switch (lHackValue) {
    case 0: 
      return false;
    case 1: 
      return true;
    default: 
      return aValueIfAutomatic;
  }
}


void
MouseScrollHandler::Device::Init()
{
  sFakeScrollableWindowNeeded =
    GetWorkaroundPref("ui.trackpoint_hack.enabled",
                      (TrackPoint::IsDriverInstalled() ||
                       UltraNav::IsObsoleteDriverInstalled()));

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::Device::Init(): sFakeScrollableWindowNeeded=%s",
     GetBoolName(sFakeScrollableWindowNeeded)));

  Elantech::Init();
}








void
MouseScrollHandler::Device::Elantech::Init()
{
  int32_t version = GetDriverMajorVersion();
  bool needsHack =
    Device::GetWorkaroundPref("ui.elantech_gesture_hacks.enabled",
                              version != 0);
  sUseSwipeHack = needsHack && version <= 7;
  sUsePinchHack = needsHack && version <= 8;

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::Device::Elantech::Init(): version=%d, sUseSwipeHack=%s, "
     "sUsePinchHack=%s",
     version, GetBoolName(sUseSwipeHack), GetBoolName(sUsePinchHack)));
}


int32_t
MouseScrollHandler::Device::Elantech::GetDriverMajorVersion()
{
  wchar_t buf[40];
  
  bool foundKey =
    WinUtils::GetRegistryKey(HKEY_CURRENT_USER,
                             L"Software\\Elantech\\MainOption",
                             L"DriverVersion",
                             buf, sizeof buf);
  if (!foundKey) {
    foundKey =
      WinUtils::GetRegistryKey(HKEY_CURRENT_USER,
                               L"Software\\Elantech",
                               L"DriverVersion",
                               buf, sizeof buf);
  }

  if (!foundKey) {
    return 0;
  }

  
  
  for (wchar_t* p = buf; *p; p++) {
    if (*p >= L'0' && *p <= L'9' && (p == buf || *(p - 1) == L' ')) {
      return wcstol(p, nullptr, 10);
    }
  }

  return 0;
}


bool
MouseScrollHandler::Device::Elantech::IsHelperWindow(HWND aWnd)
{
  
  

  const wchar_t* filenameSuffix = L"\\etdctrl.exe";
  const int filenameSuffixLength = 12;

  DWORD pid;
  ::GetWindowThreadProcessId(aWnd, &pid);

  HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (!hProcess) {
    return false;
  }

  bool result = false;
  wchar_t path[256] = {L'\0'};
  if (::GetProcessImageFileNameW(hProcess, path, ArrayLength(path))) {
    int pathLength = lstrlenW(path);
    if (pathLength >= filenameSuffixLength) {
      if (lstrcmpiW(path + pathLength - filenameSuffixLength,
                    filenameSuffix) == 0) {
        result = true;
      }
    }
  }
  ::CloseHandle(hProcess);

  return result;
}


bool
MouseScrollHandler::Device::Elantech::HandleKeyMessage(nsWindowBase* aWidget,
                                                       UINT aMsg,
                                                       WPARAM aWParam)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (sUseSwipeHack &&
      (aWParam == VK_NEXT || aWParam == VK_PRIOR) &&
      (IS_VK_DOWN(0xFF) || IS_VK_DOWN(0xCC))) {
    if (aMsg == WM_KEYDOWN) {
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::Device::Elantech::HandleKeyMessage(): Dispatching "
         "%s command event",
         aWParam == VK_NEXT ? "Forward" : "Back"));

      WidgetCommandEvent commandEvent(true, nsGkAtoms::onAppCommand,
        (aWParam == VK_NEXT) ? nsGkAtoms::Forward : nsGkAtoms::Back, aWidget);
      InitEvent(aWidget, commandEvent);
      aWidget->DispatchWindowEvent(&commandEvent);
    }
#ifdef PR_LOGGING
    else {
      PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
        ("MouseScroll::Device::Elantech::HandleKeyMessage(): Consumed"));
    }
#endif
    return true; 
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (sUsePinchHack && aMsg == WM_KEYUP &&
      aWParam == VK_CONTROL && ::GetMessageTime() == 10) {
    
    
    
    sZoomUntil = ::GetTickCount() & 0x7FFFFFFF;

    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::Elantech::HandleKeyMessage(): sZoomUntil=%d",
       sZoomUntil));
  }

  return false;
}


void
MouseScrollHandler::Device::Elantech::UpdateZoomUntil()
{
  if (!sZoomUntil) {
    return;
  }

  
  
  
  
  
  
  
  
  
  
  LONG msgTime = ::GetMessageTime();
  if ((sZoomUntil >= 0x3fffffffu && DWORD(msgTime) < 0x40000000u) ||
      (sZoomUntil < DWORD(msgTime))) {
    sZoomUntil = 0;

    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::Elantech::UpdateZoomUntil(): "
       "sZoomUntil was reset"));
  }
}


bool
MouseScrollHandler::Device::Elantech::IsZooming()
{
  
  
  
  return (sZoomUntil && static_cast<DWORD>(::GetMessageTime()) < sZoomUntil);
}








bool
MouseScrollHandler::Device::TrackPoint::IsDriverInstalled()
{
  if (WinUtils::HasRegistryKey(HKEY_CURRENT_USER,
                               L"Software\\Lenovo\\TrackPoint")) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::TrackPoint::IsDriverInstalled(): "
       "Lenovo's TrackPoint driver is found"));
    return true;
  }

  if (WinUtils::HasRegistryKey(HKEY_CURRENT_USER,
                               L"Software\\Alps\\Apoint\\TrackPoint")) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::TrackPoint::IsDriverInstalled(): "
       "Alps's TrackPoint driver is found"));
  }

  return false;
}








bool
MouseScrollHandler::Device::UltraNav::IsObsoleteDriverInstalled()
{
  if (WinUtils::HasRegistryKey(HKEY_CURRENT_USER,
                               L"Software\\Lenovo\\UltraNav")) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::UltraNav::IsObsoleteDriverInstalled(): "
       "Lenovo's UltraNav driver is found"));
    return true;
  }

  bool installed = false;
  if (WinUtils::HasRegistryKey(HKEY_CURRENT_USER,
        L"Software\\Synaptics\\SynTPEnh\\UltraNavUSB")) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::UltraNav::IsObsoleteDriverInstalled(): "
       "Synaptics's UltraNav (USB) driver is found"));
    installed = true;
  } else if (WinUtils::HasRegistryKey(HKEY_CURRENT_USER,
               L"Software\\Synaptics\\SynTPEnh\\UltraNavPS2")) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::UltraNav::IsObsoleteDriverInstalled(): "
       "Synaptics's UltraNav (PS/2) driver is found"));
    installed = true;
  }

  if (!installed) {
    return false;
  }

  wchar_t buf[40];
  bool foundKey =
    WinUtils::GetRegistryKey(HKEY_LOCAL_MACHINE,
                             L"Software\\Synaptics\\SynTP\\Install",
                             L"DriverVersion",
                             buf, sizeof buf);
  if (!foundKey) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::UltraNav::IsObsoleteDriverInstalled(): "
       "Failed to get UltraNav driver version"));
    return false;
  }

  int majorVersion = wcstol(buf, nullptr, 10);
  int minorVersion = 0;
  wchar_t* p = wcschr(buf, L'.');
  if (p) {
    minorVersion = wcstol(p + 1, nullptr, 10);
  }
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::Device::UltraNav::IsObsoleteDriverInstalled(): "
     "found driver version = %d.%d",
     majorVersion, minorVersion));
  return majorVersion < 15 || (majorVersion == 15 && minorVersion == 0);
}








bool
MouseScrollHandler::Device::SetPoint::IsGetMessagePosResponseValid(
                                        UINT aMessage,
                                        WPARAM aWParam,
                                        LPARAM aLParam)
{
  if (aMessage != WM_MOUSEHWHEEL) {
    return false;
  }

  POINTS pts = MouseScrollHandler::GetCurrentMessagePos();
  LPARAM messagePos = MAKELPARAM(pts.x, pts.y);

  

  
  
  
  
  
  
  
  
  if (!sMightBeUsing && !aLParam && aLParam != messagePos &&
      ::InSendMessage()) {
    sMightBeUsing = true;
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::SetPoint::IsGetMessagePosResponseValid(): "
       "Might using SetPoint"));
  } else if (sMightBeUsing && aLParam != 0 && ::InSendMessage()) {
    
    
    sMightBeUsing = false;
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::Device::SetPoint::IsGetMessagePosResponseValid(): "
       "Might stop using SetPoint"));
  }
  return (sMightBeUsing && !aLParam && !messagePos);
}








bool
MouseScrollHandler::SynthesizingEvent::IsSynthesizing()
{
  return MouseScrollHandler::sInstance &&
    MouseScrollHandler::sInstance->mSynthesizingEvent &&
    MouseScrollHandler::sInstance->mSynthesizingEvent->mStatus !=
      NOT_SYNTHESIZING;
}

nsresult
MouseScrollHandler::SynthesizingEvent::Synthesize(const POINTS& aCursorPoint,
                                                  HWND aWnd,
                                                  UINT aMessage,
                                                  WPARAM aWParam,
                                                  LPARAM aLParam,
                                                  const BYTE (&aKeyStates)[256])
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SynthesizingEvent::Synthesize(): aCursorPoint: { "
     "x: %d, y: %d }, aWnd=0x%X, aMessage=0x%04X, aWParam=0x%08X, "
     "aLParam=0x%08X, IsSynthesized()=%s, mStatus=%s",
     aCursorPoint.x, aCursorPoint.y, aWnd, aMessage, aWParam, aLParam,
     GetBoolName(IsSynthesizing()), GetStatusName()));

  if (IsSynthesizing()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  ::GetKeyboardState(mOriginalKeyState);

  
  
  
  mCursorPoint = aCursorPoint;

  mWnd = aWnd;
  mMessage = aMessage;
  mWParam = aWParam;
  mLParam = aLParam;

  memcpy(mKeyState, aKeyStates, sizeof(mKeyState));
  ::SetKeyboardState(mKeyState);

  mStatus = SENDING_MESSAGE;

  
  
  ::SendMessage(aWnd, aMessage, aWParam, aLParam);

  return NS_OK;
}

void
MouseScrollHandler::SynthesizingEvent::NativeMessageReceived(nsWindowBase* aWidget,
                                                             UINT aMessage,
                                                             WPARAM aWParam,
                                                             LPARAM aLParam)
{
  if (mStatus == SENDING_MESSAGE && mMessage == aMessage &&
      mWParam == aWParam && mLParam == aLParam) {
    mStatus = NATIVE_MESSAGE_RECEIVED;
    if (aWidget && aWidget->GetWindowHandle() == mWnd) {
      return;
    }
    
    
    if (aWidget && aWidget->IsPlugin() &&
        !WinUtils::GetNSWindowBasePtr(mWnd)) {
      return;
    }
    
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SynthesizingEvent::NativeMessageReceived(): "
     "aWidget=%p, aWidget->GetWindowHandle()=0x%X, mWnd=0x%X, "
     "aMessage=0x%04X, aWParam=0x%08X, aLParam=0x%08X, mStatus=%s",
     aWidget, aWidget ? aWidget->GetWindowHandle() : 0, mWnd,
     aMessage, aWParam, aLParam, GetStatusName()));

  
  Finish();

  return;
}

void
MouseScrollHandler::SynthesizingEvent::NotifyNativeMessageHandlingFinished()
{
  if (!IsSynthesizing()) {
    return;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SynthesizingEvent::"
     "NotifyNativeMessageHandlingFinished(): IsWaitingInternalMessage=%s",
     GetBoolName(MouseScrollHandler::IsWaitingInternalMessage())));

  if (MouseScrollHandler::IsWaitingInternalMessage()) {
    mStatus = INTERNAL_MESSAGE_POSTED;
    return;
  }

  
  
  
  Finish();
}

void
MouseScrollHandler::SynthesizingEvent::NotifyInternalMessageHandlingFinished()
{
  if (!IsSynthesizing()) {
    return;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SynthesizingEvent::"
     "NotifyInternalMessageHandlingFinished()"));

  Finish();
}

void
MouseScrollHandler::SynthesizingEvent::Finish()
{
  if (!IsSynthesizing()) {
    return;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SynthesizingEvent::Finish()"));

  
  ::SetKeyboardState(mOriginalKeyState);

  mStatus = NOT_SYNTHESIZING;
}

} 
} 
