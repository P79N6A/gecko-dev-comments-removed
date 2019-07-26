




#include "mozilla/DebugOnly.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Util.h"

#include "KeyboardLayout.h"
#include "nsIMM32Handler.h"

#include "nsMemory.h"
#include "nsToolkit.h"
#include "nsQuickSort.h"
#include "nsAlgorithm.h"
#include "nsGUIEvent.h"
#include "nsUnicharUtils.h"
#include "WidgetUtils.h"
#include "WinUtils.h"
#include "nsWindowDbg.h"
#include "nsServiceManagerUtils.h"
#include "nsPrintfCString.h"

#include "nsIDOMKeyEvent.h"
#include "nsIIdleServiceInternal.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#include "npapi.h"

#include <windows.h>
#include <winuser.h>
#include <algorithm>

#ifndef WINABLEAPI
#include <winable.h>
#endif

namespace mozilla {
namespace widget {




static uint32_t sUniqueKeyEventId = 0;

struct DeadKeyEntry
{
  PRUnichar BaseChar;
  PRUnichar CompositeChar;
};


class DeadKeyTable
{
  friend class KeyboardLayout;

  uint16_t mEntries;
  
  
  DeadKeyEntry mTable[1];

  void Init(const DeadKeyEntry* aDeadKeyArray, uint32_t aEntries)
  {
    mEntries = aEntries;
    memcpy(mTable, aDeadKeyArray, aEntries * sizeof(DeadKeyEntry));
  }

  static uint32_t SizeInBytes(uint32_t aEntries)
  {
    return offsetof(DeadKeyTable, mTable) + aEntries * sizeof(DeadKeyEntry);
  }

public:
  uint32_t Entries() const
  {
    return mEntries;
  }

  bool IsEqual(const DeadKeyEntry* aDeadKeyArray, uint32_t aEntries) const
  {
    return (mEntries == aEntries &&
            !memcmp(mTable, aDeadKeyArray,
                    aEntries * sizeof(DeadKeyEntry)));
  }

  PRUnichar GetCompositeChar(PRUnichar aBaseChar) const;
};






void
ModifierKeyState::Update()
{
  mModifiers = 0;
  if (IS_VK_DOWN(VK_SHIFT)) {
    mModifiers |= MODIFIER_SHIFT;
  }
  if (IS_VK_DOWN(VK_CONTROL)) {
    mModifiers |= MODIFIER_CONTROL;
  }
  if (IS_VK_DOWN(VK_MENU)) {
    mModifiers |= MODIFIER_ALT;
  }
  if (IS_VK_DOWN(VK_LWIN) || IS_VK_DOWN(VK_RWIN)) {
    mModifiers |= MODIFIER_OS;
  }
  if (::GetKeyState(VK_CAPITAL) & 1) {
    mModifiers |= MODIFIER_CAPSLOCK;
  }
  if (::GetKeyState(VK_NUMLOCK) & 1) {
    mModifiers |= MODIFIER_NUMLOCK;
  }
  if (::GetKeyState(VK_SCROLL) & 1) {
    mModifiers |= MODIFIER_SCROLLLOCK;
  }

  EnsureAltGr();
}

void
ModifierKeyState::InitInputEvent(nsInputEvent& aInputEvent) const
{
  aInputEvent.modifiers = mModifiers;

  switch(aInputEvent.eventStructType) {
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
    case NS_WHEEL_EVENT:
    case NS_DRAG_EVENT:
    case NS_SIMPLE_GESTURE_EVENT:
      InitMouseEvent(aInputEvent);
      break;
    default:
      break;
  }
}

void
ModifierKeyState::InitMouseEvent(nsInputEvent& aMouseEvent) const
{
  NS_ASSERTION(aMouseEvent.eventStructType == NS_MOUSE_EVENT ||
               aMouseEvent.eventStructType == NS_WHEEL_EVENT ||
               aMouseEvent.eventStructType == NS_DRAG_EVENT ||
               aMouseEvent.eventStructType == NS_SIMPLE_GESTURE_EVENT,
               "called with non-mouse event");

  nsMouseEvent_base& mouseEvent = static_cast<nsMouseEvent_base&>(aMouseEvent);
  mouseEvent.buttons = 0;
  if (::GetKeyState(VK_LBUTTON) < 0) {
    mouseEvent.buttons |= nsMouseEvent::eLeftButtonFlag;
  }
  if (::GetKeyState(VK_RBUTTON) < 0) {
    mouseEvent.buttons |= nsMouseEvent::eRightButtonFlag;
  }
  if (::GetKeyState(VK_MBUTTON) < 0) {
    mouseEvent.buttons |= nsMouseEvent::eMiddleButtonFlag;
  }
  if (::GetKeyState(VK_XBUTTON1) < 0) {
    mouseEvent.buttons |= nsMouseEvent::e4thButtonFlag;
  }
  if (::GetKeyState(VK_XBUTTON2) < 0) {
    mouseEvent.buttons |= nsMouseEvent::e5thButtonFlag;
  }
}





void
UniCharsAndModifiers::Append(PRUnichar aUniChar, Modifiers aModifiers)
{
  MOZ_ASSERT(mLength < 5);
  mChars[mLength] = aUniChar;
  mModifiers[mLength] = aModifiers;
  mLength++;
}

void
UniCharsAndModifiers::FillModifiers(Modifiers aModifiers)
{
  for (uint32_t i = 0; i < mLength; i++) {
    mModifiers[i] = aModifiers;
  }
}

bool
UniCharsAndModifiers::UniCharsEqual(const UniCharsAndModifiers& aOther) const
{
  if (mLength != aOther.mLength) {
    return false;
  }
  return !memcmp(mChars, aOther.mChars, mLength * sizeof(PRUnichar));
}

bool
UniCharsAndModifiers::UniCharsCaseInsensitiveEqual(
                        const UniCharsAndModifiers& aOther) const
{
  if (mLength != aOther.mLength) {
    return false;
  }

  nsCaseInsensitiveStringComparator comp;
  return !comp(mChars, aOther.mChars, mLength, aOther.mLength);
}

UniCharsAndModifiers&
UniCharsAndModifiers::operator+=(const UniCharsAndModifiers& aOther)
{
  uint32_t copyCount = std::min(aOther.mLength, 5 - mLength);
  NS_ENSURE_TRUE(copyCount > 0, *this);
  memcpy(&mChars[mLength], aOther.mChars, copyCount * sizeof(PRUnichar));
  memcpy(&mModifiers[mLength], aOther.mModifiers,
         copyCount * sizeof(Modifiers));
  mLength += copyCount;
  return *this;
}

UniCharsAndModifiers
UniCharsAndModifiers::operator+(const UniCharsAndModifiers& aOther) const
{
  UniCharsAndModifiers result(*this);
  result += aOther;
  return result;
}





inline PRUnichar
VirtualKey::GetCompositeChar(ShiftState aShiftState, PRUnichar aBaseChar) const
{
  return mShiftStates[aShiftState].DeadKey.Table->GetCompositeChar(aBaseChar);
}

const DeadKeyTable*
VirtualKey::MatchingDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                 uint32_t aEntries) const
{
  if (!mIsDeadKey) {
    return nullptr;
  }

  for (ShiftState shiftState = 0; shiftState < 16; shiftState++) {
    if (!IsDeadKey(shiftState)) {
      continue;
    }
    const DeadKeyTable* dkt = mShiftStates[shiftState].DeadKey.Table;
    if (dkt && dkt->IsEqual(aDeadKeyArray, aEntries)) {
      return dkt;
    }
  }

  return nullptr;
}

void
VirtualKey::SetNormalChars(ShiftState aShiftState,
                           const PRUnichar* aChars,
                           uint32_t aNumOfChars)
{
  NS_ASSERTION(aShiftState < ArrayLength(mShiftStates), "invalid index");

  SetDeadKey(aShiftState, false);

  for (uint32_t index = 0; index < aNumOfChars; index++) {
    
    mShiftStates[aShiftState].Normal.Chars[index] =
      (aChars[index] >= 0x20) ? aChars[index] : 0;
  }

  uint32_t len = ArrayLength(mShiftStates[aShiftState].Normal.Chars);
  for (uint32_t index = aNumOfChars; index < len; index++) {
    mShiftStates[aShiftState].Normal.Chars[index] = 0;
  }
}

void
VirtualKey::SetDeadChar(ShiftState aShiftState, PRUnichar aDeadChar)
{
  NS_ASSERTION(aShiftState < ArrayLength(mShiftStates), "invalid index");

  SetDeadKey(aShiftState, true);

  mShiftStates[aShiftState].DeadKey.DeadChar = aDeadChar;
  mShiftStates[aShiftState].DeadKey.Table = nullptr;
}

UniCharsAndModifiers
VirtualKey::GetUniChars(ShiftState aShiftState) const
{
  UniCharsAndModifiers result = GetNativeUniChars(aShiftState);

  const ShiftState STATE_ALT_CONTROL = (STATE_ALT | STATE_CONTROL);
  if (!(aShiftState & STATE_ALT_CONTROL)) {
    return result;
  }

  if (!result.mLength) {
    result = GetNativeUniChars(aShiftState & ~STATE_ALT_CONTROL);
    result.FillModifiers(ShiftStateToModifiers(aShiftState));
    return result;
  }

  if ((aShiftState & STATE_ALT_CONTROL) == STATE_ALT_CONTROL) {
    
    
    
    
    
    Modifiers finalModifiers = ShiftStateToModifiers(aShiftState);
    finalModifiers &= ~(MODIFIER_ALT | MODIFIER_CONTROL);
    result.FillModifiers(finalModifiers);
    return result;
  }

  UniCharsAndModifiers unmodifiedReslt =
    GetNativeUniChars(aShiftState & ~STATE_ALT_CONTROL);
  if (!result.UniCharsEqual(unmodifiedReslt)) {
    
    
    Modifiers finalModifiers = ShiftStateToModifiers(aShiftState);
    finalModifiers &= ~(MODIFIER_ALT | MODIFIER_CONTROL);
    result.FillModifiers(finalModifiers);
  }
  return result;
}


UniCharsAndModifiers
VirtualKey::GetNativeUniChars(ShiftState aShiftState) const
{
#ifdef DEBUG
  if (aShiftState < 0 || aShiftState >= ArrayLength(mShiftStates)) {
    nsPrintfCString warning("Shift state is out of range: "
                            "aShiftState=%d, ArrayLength(mShiftState)=%d",
                            aShiftState, ArrayLength(mShiftStates));
    NS_WARNING(warning.get());
  }
#endif

  UniCharsAndModifiers result;
  Modifiers modifiers = ShiftStateToModifiers(aShiftState);
  if (IsDeadKey(aShiftState)) {
    result.Append(mShiftStates[aShiftState].DeadKey.DeadChar, modifiers);
    return result;
  }

  uint32_t index;
  uint32_t len = ArrayLength(mShiftStates[aShiftState].Normal.Chars);
  for (index = 0;
       index < len && mShiftStates[aShiftState].Normal.Chars[index]; index++) {
    result.Append(mShiftStates[aShiftState].Normal.Chars[index], modifiers);
  }
  return result;
}


void
VirtualKey::FillKbdState(PBYTE aKbdState,
                         const ShiftState aShiftState)
{
  NS_ASSERTION(aShiftState < 16, "aShiftState out of range");

  if (aShiftState & STATE_SHIFT) {
    aKbdState[VK_SHIFT] |= 0x80;
  } else {
    aKbdState[VK_SHIFT]  &= ~0x80;
    aKbdState[VK_LSHIFT] &= ~0x80;
    aKbdState[VK_RSHIFT] &= ~0x80;
  }

  if (aShiftState & STATE_CONTROL) {
    aKbdState[VK_CONTROL] |= 0x80;
  } else {
    aKbdState[VK_CONTROL]  &= ~0x80;
    aKbdState[VK_LCONTROL] &= ~0x80;
    aKbdState[VK_RCONTROL] &= ~0x80;
  }

  if (aShiftState & STATE_ALT) {
    aKbdState[VK_MENU] |= 0x80;
  } else {
    aKbdState[VK_MENU]  &= ~0x80;
    aKbdState[VK_LMENU] &= ~0x80;
    aKbdState[VK_RMENU] &= ~0x80;
  }

  if (aShiftState & STATE_CAPSLOCK) {
    aKbdState[VK_CAPITAL] |= 0x01;
  } else {
    aKbdState[VK_CAPITAL] &= ~0x01;
  }
}





NativeKey::NativeKey(nsWindowBase* aWidget,
                     const MSG& aKeyOrCharMessage,
                     const ModifierKeyState& aModKeyState,
                     nsTArray<FakeCharMsg>* aFakeCharMsgs) :
  mWidget(aWidget), mMsg(aKeyOrCharMessage), mDOMKeyCode(0),
  mModKeyState(aModKeyState), mVirtualKeyCode(0), mOriginalVirtualKeyCode(0),
  mFakeCharMsgs(aFakeCharMsgs && aFakeCharMsgs->Length() ?
                  aFakeCharMsgs : nullptr)
{
  MOZ_ASSERT(aWidget);
  KeyboardLayout* keyboardLayout = KeyboardLayout::GetInstance();
  mKeyboardLayout = keyboardLayout->GetLayout();
  mScanCode = WinUtils::GetScanCode(mMsg.lParam);
  mIsExtended = WinUtils::IsExtendedScanCode(mMsg.lParam);
  
  
  bool canComputeVirtualKeyCodeFromScanCode =
    (!mIsExtended || WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION);
  switch (mMsg.message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
      
      
      if (mMsg.wParam == VK_PROCESSKEY) {
        mOriginalVirtualKeyCode =
          static_cast<uint8_t>(::ImmGetVirtualKey(mMsg.hwnd));
      } else {
        mOriginalVirtualKeyCode = static_cast<uint8_t>(mMsg.wParam);
      }

      
      bool isLeftRightDistinguishedKey = false;

      
      
      switch (mOriginalVirtualKeyCode) {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
          isLeftRightDistinguishedKey = true;
          break;
        case VK_LSHIFT:
        case VK_RSHIFT:
          mVirtualKeyCode = mOriginalVirtualKeyCode;
          mOriginalVirtualKeyCode = VK_SHIFT;
          isLeftRightDistinguishedKey = true;
          break;
        case VK_LCONTROL:
        case VK_RCONTROL:
          mVirtualKeyCode = mOriginalVirtualKeyCode;
          mOriginalVirtualKeyCode = VK_CONTROL;
          isLeftRightDistinguishedKey = true;
          break;
        case VK_LMENU:
        case VK_RMENU:
          mVirtualKeyCode = mOriginalVirtualKeyCode;
          mOriginalVirtualKeyCode = VK_MENU;
          isLeftRightDistinguishedKey = true;
          break;
      }

      
      
      if (mVirtualKeyCode) {
        break;
      }

      
      
      
      
      
      if (!isLeftRightDistinguishedKey) {
        break;
      }

      if (!canComputeVirtualKeyCodeFromScanCode) {
        
        
        
        
        
        
        switch (mOriginalVirtualKeyCode) {
          case VK_CONTROL:
            mVirtualKeyCode = VK_RCONTROL;
            break;
          case VK_MENU:
            mVirtualKeyCode = VK_RMENU;
            break;
          case VK_SHIFT:
            
            
            mVirtualKeyCode = VK_LSHIFT;
            break;
          default:
            MOZ_CRASH("Unsupported mOriginalVirtualKeyCode");
        }
        break;
      }

      NS_ASSERTION(!mVirtualKeyCode,
                   "mVirtualKeyCode has been computed already");

      
      mVirtualKeyCode = ComputeVirtualKeyCodeFromScanCodeEx();

      
      
      
      
      
      switch (mOriginalVirtualKeyCode) {
        case VK_CONTROL:
          if (mVirtualKeyCode != VK_LCONTROL &&
              mVirtualKeyCode != VK_RCONTROL) {
            mVirtualKeyCode = mIsExtended ? VK_RCONTROL : VK_LCONTROL;
          }
          break;
        case VK_MENU:
          if (mVirtualKeyCode != VK_LMENU && mVirtualKeyCode != VK_RMENU) {
            mVirtualKeyCode = mIsExtended ? VK_RMENU : VK_LMENU;
          }
          break;
        case VK_SHIFT:
          if (mVirtualKeyCode != VK_LSHIFT && mVirtualKeyCode != VK_RSHIFT) {
            
            
            mVirtualKeyCode = VK_LSHIFT;
          }
          break;
        default:
          MOZ_CRASH("Unsupported mOriginalVirtualKeyCode");
      }
      break;
    }
    case WM_CHAR:
    case WM_UNICHAR:
    case WM_SYSCHAR:
      
      
      if (!canComputeVirtualKeyCodeFromScanCode) {
        break;
      }
      mVirtualKeyCode = mOriginalVirtualKeyCode =
        ComputeVirtualKeyCodeFromScanCodeEx();
      NS_ASSERTION(mVirtualKeyCode, "Failed to compute virtual keycode");
      break;
    default:
      MOZ_CRASH("Unsupported message");
  }

  if (!mVirtualKeyCode) {
    mVirtualKeyCode = mOriginalVirtualKeyCode;
  }

  mDOMKeyCode =
    keyboardLayout->ConvertNativeKeyCodeToDOMKeyCode(mOriginalVirtualKeyCode);
  mKeyNameIndex =
    keyboardLayout->ConvertNativeKeyCodeToKeyNameIndex(mOriginalVirtualKeyCode);

  keyboardLayout->InitNativeKey(*this, mModKeyState);

  mIsDeadKey =
    (IsFollowedByDeadCharMessage() ||
     keyboardLayout->IsDeadKey(mOriginalVirtualKeyCode, mModKeyState));
  mIsPrintableKey = KeyboardLayout::IsPrintableCharKey(mOriginalVirtualKeyCode);
}

bool
NativeKey::IsFollowedByCharMessage() const
{
  MSG nextMsg;
  if (mFakeCharMsgs) {
    nextMsg = mFakeCharMsgs->ElementAt(0).GetCharMsg(mMsg.hwnd);
  } else {
    if (!WinUtils::PeekMessage(&nextMsg, mMsg.hwnd, WM_KEYFIRST, WM_KEYLAST,
                               PM_NOREMOVE | PM_NOYIELD)) {
      return false;
    }
  }
  return (nextMsg.message == WM_CHAR ||
          nextMsg.message == WM_SYSCHAR ||
          nextMsg.message == WM_DEADCHAR);
}

bool
NativeKey::IsFollowedByDeadCharMessage() const
{
  MSG nextMsg;
  if (mFakeCharMsgs) {
    nextMsg = mFakeCharMsgs->ElementAt(0).GetCharMsg(mMsg.hwnd);
  } else {
    if (!WinUtils::PeekMessage(&nextMsg, mMsg.hwnd, WM_KEYFIRST, WM_KEYLAST,
                               PM_NOREMOVE | PM_NOYIELD)) {
      return false;
    }
  }
  return (nextMsg.message == WM_DEADCHAR);
}

bool
NativeKey::IsIMEDoingKakuteiUndo() const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  MSG startCompositionMsg, compositionMsg, charMsg;
  return WinUtils::PeekMessage(&startCompositionMsg, mMsg.hwnd,
                               WM_IME_STARTCOMPOSITION, WM_IME_STARTCOMPOSITION,
                               PM_NOREMOVE | PM_NOYIELD) &&
         WinUtils::PeekMessage(&compositionMsg, mMsg.hwnd, WM_IME_COMPOSITION,
                               WM_IME_COMPOSITION, PM_NOREMOVE | PM_NOYIELD) &&
         WinUtils::PeekMessage(&charMsg, mMsg.hwnd, WM_CHAR, WM_CHAR,
                               PM_NOREMOVE | PM_NOYIELD) &&
         startCompositionMsg.wParam == 0x0 &&
         startCompositionMsg.lParam == 0x0 &&
         compositionMsg.wParam == 0x0 &&
         compositionMsg.lParam == 0x1BF &&
         charMsg.wParam == VK_BACK && charMsg.lParam == 0x1 &&
         startCompositionMsg.time <= compositionMsg.time &&
         compositionMsg.time <= charMsg.time;
}

UINT
NativeKey::GetScanCodeWithExtendedFlag() const
{
  
  
  
  
  
  if (!mIsExtended ||
      WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION) {
    return mScanCode;
  }
  return (0xE000 | mScanCode);
}

uint32_t
NativeKey::GetKeyLocation() const
{
  switch (mVirtualKeyCode) {
    case VK_LSHIFT:
    case VK_LCONTROL:
    case VK_LMENU:
    case VK_LWIN:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_LEFT;

    case VK_RSHIFT:
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_RWIN:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_RIGHT;

    case VK_RETURN:
      
      return !mIsExtended ? nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD :
                            nsIDOMKeyEvent::DOM_KEY_LOCATION_NUMPAD;

    case VK_INSERT:
    case VK_DELETE:
    case VK_END:
    case VK_DOWN:
    case VK_NEXT:
    case VK_LEFT:
    case VK_CLEAR:
    case VK_RIGHT:
    case VK_HOME:
    case VK_UP:
    case VK_PRIOR:
      
      return mIsExtended ? nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD :
                           nsIDOMKeyEvent::DOM_KEY_LOCATION_NUMPAD;

    
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DECIMAL:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    
    case VK_ABNT_C2:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_NUMPAD;

    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
      NS_WARNING("Failed to decide the key location?");

    default:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD;
  }
}

uint8_t
NativeKey::ComputeVirtualKeyCodeFromScanCode() const
{
  return static_cast<uint8_t>(
           ::MapVirtualKeyEx(mScanCode, MAPVK_VSC_TO_VK, mKeyboardLayout));
}

uint8_t
NativeKey::ComputeVirtualKeyCodeFromScanCodeEx() const
{
  bool VistaOrLater =
    (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION);
  
  
  NS_ENSURE_TRUE(!mIsExtended || VistaOrLater, 0);
  return static_cast<uint8_t>(
           ::MapVirtualKeyEx(GetScanCodeWithExtendedFlag(), MAPVK_VSC_TO_VK_EX,
                             mKeyboardLayout));
}

PRUnichar
NativeKey::ComputeUnicharFromScanCode() const
{
  return static_cast<PRUnichar>(
           ::MapVirtualKeyEx(ComputeVirtualKeyCodeFromScanCode(),
                             MAPVK_VK_TO_CHAR, mKeyboardLayout));
}

void
NativeKey::InitKeyEvent(nsKeyEvent& aKeyEvent,
                        const ModifierKeyState& aModKeyState) const
{
  nsIntPoint point(0, 0);
  mWidget->InitEvent(aKeyEvent, &point);

  switch (aKeyEvent.message) {
    case NS_KEY_DOWN:
      aKeyEvent.keyCode = mDOMKeyCode;
      
      sUniqueKeyEventId++;
      aKeyEvent.mUniqueId = sUniqueKeyEventId;
      break;
    case NS_KEY_UP:
      aKeyEvent.keyCode = mDOMKeyCode;
      
      
      
      
      
      aKeyEvent.mFlags.mDefaultPrevented =
        (mOriginalVirtualKeyCode == VK_MENU && mMsg.message != WM_SYSKEYUP);
      break;
    case NS_KEY_PRESS:
      aKeyEvent.mUniqueId = sUniqueKeyEventId;
      break;
    default:
      MOZ_CRASH("Invalid event message");
  }

  aKeyEvent.mKeyNameIndex = mKeyNameIndex;
  aKeyEvent.location = GetKeyLocation();
  aModKeyState.InitInputEvent(aKeyEvent);
}

bool
NativeKey::DispatchKeyEvent(nsKeyEvent& aKeyEvent,
                            const MSG* aMsgSentToPlugin) const
{
  if (mWidget->Destroyed()) {
    MOZ_CRASH("NativeKey tries to dispatch a key event on destroyed widget");
  }

  KeyboardLayout::NotifyIdleServiceOfUserActivity();

  NPEvent pluginEvent;
  if (aMsgSentToPlugin &&
      mWidget->GetInputContext().mIMEState.mEnabled == IMEState::PLUGIN) {
    pluginEvent.event = aMsgSentToPlugin->message;
    pluginEvent.wParam = aMsgSentToPlugin->wParam;
    pluginEvent.lParam = aMsgSentToPlugin->lParam;
    aKeyEvent.pluginEvent = static_cast<void*>(&pluginEvent);
  }

  return (mWidget->DispatchKeyboardEvent(&aKeyEvent) || mWidget->Destroyed());
}

bool
NativeKey::HandleKeyDownMessage(bool* aEventDispatched) const
{
  MOZ_ASSERT(mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN);

  if (aEventDispatched) {
    *aEventDispatched = false;
  }

  bool defaultPrevented = false;
  if (mFakeCharMsgs ||
      !RedirectedKeyDownMessageManager::IsRedirectedMessage(mMsg)) {
    
    if (mModKeyState.IsAlt() && !mModKeyState.IsControl() &&
        mVirtualKeyCode == VK_SPACE) {
      return false;
    }

    bool isIMEEnabled = WinUtils::IsIMEEnabled(mWidget->GetInputContext());
    nsKeyEvent keydownEvent(true, NS_KEY_DOWN, mWidget);
    InitKeyEvent(keydownEvent, mModKeyState);
    if (aEventDispatched) {
      *aEventDispatched = true;
    }
    defaultPrevented = DispatchKeyEvent(keydownEvent, &mMsg);

    if (mWidget->Destroyed()) {
      return true;
    }

    
    
    
    
    
    
    
    
    HWND focusedWnd = ::GetFocus();
    if (!defaultPrevented && !mFakeCharMsgs && focusedWnd &&
        !mWidget->PluginHasFocus() && !isIMEEnabled &&
        WinUtils::IsIMEEnabled(mWidget->GetInputContext())) {
      RedirectedKeyDownMessageManager::RemoveNextCharMessage(focusedWnd);

      INPUT keyinput;
      keyinput.type = INPUT_KEYBOARD;
      keyinput.ki.wVk = mOriginalVirtualKeyCode;
      keyinput.ki.wScan = mScanCode;
      keyinput.ki.dwFlags = KEYEVENTF_SCANCODE;
      if (mIsExtended) {
        keyinput.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
      }
      keyinput.ki.time = 0;
      keyinput.ki.dwExtraInfo = 0;

      RedirectedKeyDownMessageManager::WillRedirect(mMsg, defaultPrevented);

      ::SendInput(1, &keyinput, sizeof(keyinput));

      
      
      
      return true;
    }
  } else {
    defaultPrevented = RedirectedKeyDownMessageManager::DefaultPrevented();
    
    
    if (aEventDispatched) {
      *aEventDispatched = true;
    }
  }

  RedirectedKeyDownMessageManager::Forget();

  
  if (mOriginalVirtualKeyCode == VK_PROCESSKEY) {
    return defaultPrevented;
  }

  
  switch (mDOMKeyCode) {
    case NS_VK_SHIFT:
    case NS_VK_CONTROL:
    case NS_VK_ALT:
    case NS_VK_CAPS_LOCK:
    case NS_VK_NUM_LOCK:
    case NS_VK_SCROLL_LOCK:
    case NS_VK_WIN:
      return defaultPrevented;
  }

  if (defaultPrevented) {
    DispatchPluginEventsAndDiscardsCharMessages();
    return true;
  }

  
  
  if (NeedsToHandleWithoutFollowingCharMessages()) {
    return (DispatchPluginEventsAndDiscardsCharMessages() ||
            DispatchKeyPressEventsWithKeyboardLayout());
  }

  if (IsFollowedByCharMessage()) {
    return DispatchKeyPressEventForFollowingCharMessage();
  }

  if (!mModKeyState.IsControl() && !mModKeyState.IsAlt() &&
      !mModKeyState.IsWin() && mIsPrintableKey) {
    
    
    
    return false;
  }

  if (mIsDeadKey) {
    return false;
  }

  return DispatchKeyPressEventsWithKeyboardLayout();
}

bool
NativeKey::HandleCharMessage(const MSG& aCharMsg,
                             bool* aEventDispatched) const
{
  MOZ_ASSERT(mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN ||
             mMsg.message == WM_CHAR || mMsg.message == WM_SYSCHAR);
  MOZ_ASSERT(aCharMsg.message == WM_CHAR || aCharMsg.message == WM_SYSCHAR);

  if (aEventDispatched) {
    *aEventDispatched = false;
  }

  
  if (mModKeyState.IsAlt() && !mModKeyState.IsControl() &&
      mVirtualKeyCode == VK_SPACE) {
    return false;
  }

  
  if (!mModKeyState.IsAlt() && mModKeyState.IsControl() &&
      mVirtualKeyCode == VK_RETURN) {
    return false;
  }

  
  
  

  static const PRUnichar U_SPACE = 0x20;
  static const PRUnichar U_EQUAL = 0x3D;

  
  if ((!mModKeyState.IsAlt() && !mModKeyState.IsControl()) ||
      mModKeyState.IsAltGr() ||
      (mOriginalVirtualKeyCode &&
       !KeyboardLayout::IsPrintableCharKey(mOriginalVirtualKeyCode))) {
    nsKeyEvent keypressEvent(true, NS_KEY_PRESS, mWidget);
    if (aCharMsg.wParam >= U_SPACE) {
      keypressEvent.charCode = static_cast<uint32_t>(aCharMsg.wParam);
    } else {
      keypressEvent.keyCode = mDOMKeyCode;
    }
    
    
    
    
    ModifierKeyState modKeyState(mModKeyState);
    modKeyState.Unset(MODIFIER_ALT | MODIFIER_CONTROL);
    InitKeyEvent(keypressEvent, modKeyState);
    if (aEventDispatched) {
      *aEventDispatched = true;
    }
    return DispatchKeyEvent(keypressEvent, &aCharMsg);
  }

  
  
  
  
  

  PRUnichar uniChar;
  
  if (mModKeyState.IsControl() && aCharMsg.wParam <= 0x1A) {
    
    uniChar = aCharMsg.wParam - 1 + (mModKeyState.IsShift() ? 'A' : 'a');
  } else if (mModKeyState.IsControl() && aCharMsg.wParam <= 0x1F) {
    
    
    
    
    uniChar = aCharMsg.wParam - 1 + 'A';
  } else if (aCharMsg.wParam < U_SPACE ||
             (aCharMsg.wParam == U_EQUAL && mModKeyState.IsControl())) {
    uniChar = 0;
  } else {
    uniChar = aCharMsg.wParam;
  }

  
  
  if (uniChar && (mModKeyState.IsControl() || mModKeyState.IsAlt())) {
    KeyboardLayout* keyboardLayout = KeyboardLayout::GetInstance();
    PRUnichar unshiftedCharCode =
      (mVirtualKeyCode >= '0' && mVirtualKeyCode <= '9') ?
        mVirtualKeyCode :  mModKeyState.IsShift() ?
                             ComputeUnicharFromScanCode() : 0;
    
    if (static_cast<int32_t>(unshiftedCharCode) > 0) {
      uniChar = unshiftedCharCode;
    }
  }

  
  
  
  if (!mModKeyState.IsShift() &&
      (mModKeyState.IsAlt() || mModKeyState.IsControl())) {
    uniChar = towlower(uniChar);
  }

  nsKeyEvent keypressEvent(true, NS_KEY_PRESS, mWidget);
  keypressEvent.charCode = uniChar;
  if (!keypressEvent.charCode) {
    keypressEvent.keyCode = mDOMKeyCode;
  }
  InitKeyEvent(keypressEvent, mModKeyState);
  if (aEventDispatched) {
    *aEventDispatched = true;
  }
  return DispatchKeyEvent(keypressEvent, &aCharMsg);
}

bool
NativeKey::HandleKeyUpMessage(bool* aEventDispatched) const
{
  MOZ_ASSERT(mMsg.message == WM_KEYUP || mMsg.message == WM_SYSKEYUP);

  if (aEventDispatched) {
    *aEventDispatched = false;
  }

  
  if (mModKeyState.IsAlt() && !mModKeyState.IsControl() &&
      mVirtualKeyCode == VK_SPACE) {
    return false;
  }

  nsKeyEvent keyupEvent(true, NS_KEY_UP, mWidget);
  InitKeyEvent(keyupEvent, mModKeyState);
  if (aEventDispatched) {
    *aEventDispatched = true;
  }
  return DispatchKeyEvent(keyupEvent, &mMsg);
}

bool
NativeKey::NeedsToHandleWithoutFollowingCharMessages() const
{
  MOZ_ASSERT(mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN);

  
  
  if (mDOMKeyCode == NS_VK_RETURN || mDOMKeyCode == NS_VK_BACK) {
    return true;
  }

  
  
  if (!mModKeyState.IsControl() && !mModKeyState.IsAlt() &&
      !mModKeyState.IsWin()) {
    return false;
  }

  
  
  if (mIsDeadKey && mCommittedCharsAndModifiers.IsEmpty()) {
    return false;
  }

  
  
  return mIsPrintableKey;
}

MSG
NativeKey::RemoveFollowingCharMessage() const
{
  MOZ_ASSERT(IsFollowedByCharMessage());

  if (mFakeCharMsgs) {
    MOZ_ASSERT(!mFakeCharMsgs->ElementAt(0).mConsumed,
      "Doesn't assume that it's used for removing two or more char messages");
    mFakeCharMsgs->ElementAt(0).mConsumed = true;
    return mFakeCharMsgs->ElementAt(0).GetCharMsg(mMsg.hwnd);
  }

  MSG msg;
  if (!WinUtils::GetMessage(&msg, mMsg.hwnd, WM_KEYFIRST, WM_KEYLAST) ||
      !(msg.message == WM_CHAR || msg.message == WM_SYSCHAR ||
        msg.message == WM_DEADCHAR)) {
    MOZ_CRASH("We lost the following char message");
  }

  return msg;
}

bool
NativeKey::RemoveMessageAndDispatchPluginEvent(UINT aFirstMsg,
                                               UINT aLastMsg) const
{
  MSG msg;
  if (mFakeCharMsgs) {
    DebugOnly<bool> found = false;
    for (uint32_t i = 0; i < mFakeCharMsgs->Length(); i++) {
      FakeCharMsg& fakeCharMsg = mFakeCharMsgs->ElementAt(i);
      if (fakeCharMsg.mConsumed) {
        continue;
      }
      MSG fakeMsg = fakeCharMsg.GetCharMsg(mMsg.hwnd);
      if (fakeMsg.message < aFirstMsg || fakeMsg.message > aLastMsg) {
        continue;
      }
      fakeCharMsg.mConsumed = true;
      msg = fakeMsg;
      found = true;
      break;
    }
    MOZ_ASSERT(found, "Fake char message must be found");
  } else {
    WinUtils::GetMessage(&msg, mMsg.hwnd, aFirstMsg, aLastMsg);
  }
  if (mWidget->Destroyed()) {
    MOZ_CRASH("NativeKey tries to dispatch a plugin event on destroyed widget");
  }
  mWidget->DispatchPluginEvent(msg);
  return mWidget->Destroyed();
}

bool
NativeKey::DispatchPluginEventsAndDiscardsCharMessages() const
{
  MOZ_ASSERT(mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN);

  if (mFakeCharMsgs) {
    for (uint32_t i = 0; i < mFakeCharMsgs->Length(); i++) {
      if (RemoveMessageAndDispatchPluginEvent(WM_KEYFIRST, WM_KEYLAST)) {
        return true;
      }
    }
    return false;
  }

  
  
  
  
  bool anyCharMessagesRemoved = true;
  MSG msg;
  bool gotMsg =
    WinUtils::PeekMessage(&msg, mMsg.hwnd, WM_KEYFIRST, WM_KEYLAST,
                          PM_NOREMOVE | PM_NOYIELD);
  while (gotMsg &&
         (msg.message == WM_CHAR || msg.message == WM_SYSCHAR ||
          msg.message == WM_DEADCHAR)) {
    if (RemoveMessageAndDispatchPluginEvent(WM_KEYFIRST, WM_KEYLAST)) {
      return true;
    }
    anyCharMessagesRemoved = true;
    gotMsg = WinUtils::PeekMessage(&msg, mMsg.hwnd, WM_KEYFIRST, WM_KEYLAST,
                                   PM_NOREMOVE | PM_NOYIELD);
  }

  if (!anyCharMessagesRemoved &&
      mDOMKeyCode == NS_VK_BACK && IsIMEDoingKakuteiUndo() &&
      RemoveMessageAndDispatchPluginEvent(WM_CHAR, WM_CHAR)) {
    return true;
  }

  return false;
}

bool
NativeKey::DispatchKeyPressEventsWithKeyboardLayout() const
{
  MOZ_ASSERT(mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN);
  MOZ_ASSERT(!mIsDeadKey);

  KeyboardLayout* keyboardLayout = KeyboardLayout::GetInstance();

  UniCharsAndModifiers inputtingChars(mCommittedCharsAndModifiers);
  UniCharsAndModifiers shiftedChars;
  UniCharsAndModifiers unshiftedChars;
  uint32_t shiftedLatinChar = 0;
  uint32_t unshiftedLatinChar = 0;

  if (!KeyboardLayout::IsPrintableCharKey(mVirtualKeyCode)) {
    inputtingChars.Clear();
  }

  if (mModKeyState.IsControl() ^ mModKeyState.IsAlt()) {
    ModifierKeyState capsLockState(
                       mModKeyState.GetModifiers() & MODIFIER_CAPSLOCK);

    unshiftedChars =
      keyboardLayout->GetUniCharsAndModifiers(mVirtualKeyCode, capsLockState);
    capsLockState.Set(MODIFIER_SHIFT);
    shiftedChars =
      keyboardLayout->GetUniCharsAndModifiers(mVirtualKeyCode, capsLockState);

    
    
    
    capsLockState.Unset(MODIFIER_SHIFT);
    WidgetUtils::GetLatinCharCodeForKeyCode(mDOMKeyCode,
                                            capsLockState.GetModifiers(),
                                            &unshiftedLatinChar,
                                            &shiftedLatinChar);

    
    if (shiftedLatinChar) {
      
      
      
      if (unshiftedLatinChar == unshiftedChars.mChars[0] &&
          shiftedLatinChar == shiftedChars.mChars[0]) {
        shiftedLatinChar = unshiftedLatinChar = 0;
      }
    } else if (unshiftedLatinChar) {
      
      
      
      
      
      
      
      if (unshiftedLatinChar == unshiftedChars.mChars[0] ||
          unshiftedLatinChar == shiftedChars.mChars[0]) {
        unshiftedLatinChar = 0;
      }
    }

    
    
    
    
    
    if (mModKeyState.IsControl()) {
      uint32_t ch =
        mModKeyState.IsShift() ? shiftedLatinChar : unshiftedLatinChar;
      if (ch &&
          (!inputtingChars.mLength ||
           inputtingChars.UniCharsCaseInsensitiveEqual(
             mModKeyState.IsShift() ? shiftedChars : unshiftedChars))) {
        inputtingChars.Clear();
        inputtingChars.Append(ch, mModKeyState.GetModifiers());
      }
    }
  }

  if (inputtingChars.IsEmpty() &&
      shiftedChars.IsEmpty() && unshiftedChars.IsEmpty()) {
    nsKeyEvent keypressEvent(true, NS_KEY_PRESS, mWidget);
    keypressEvent.keyCode = mDOMKeyCode;
    InitKeyEvent(keypressEvent, mModKeyState);
    return DispatchKeyEvent(keypressEvent);
  }

  uint32_t longestLength =
    std::max(inputtingChars.mLength,
             std::max(shiftedChars.mLength, unshiftedChars.mLength));
  uint32_t skipUniChars = longestLength - inputtingChars.mLength;
  uint32_t skipShiftedChars = longestLength - shiftedChars.mLength;
  uint32_t skipUnshiftedChars = longestLength - unshiftedChars.mLength;
  UINT keyCode = !inputtingChars.mLength ? mDOMKeyCode : 0;
  bool defaultPrevented = false;
  for (uint32_t cnt = 0; cnt < longestLength; cnt++) {
    uint16_t uniChar, shiftedChar, unshiftedChar;
    uniChar = shiftedChar = unshiftedChar = 0;
    ModifierKeyState modKeyState(mModKeyState);
    if (skipUniChars <= cnt) {
      if (cnt - skipUniChars  < inputtingChars.mLength) {
        
        
        
        
        
        
        modKeyState.Unset(MODIFIER_SHIFT | MODIFIER_CONTROL | MODIFIER_ALT |
                          MODIFIER_ALTGRAPH | MODIFIER_CAPSLOCK);
        modKeyState.Set(inputtingChars.mModifiers[cnt - skipUniChars]);
      }
      uniChar = inputtingChars.mChars[cnt - skipUniChars];
    }
    if (skipShiftedChars <= cnt)
      shiftedChar = shiftedChars.mChars[cnt - skipShiftedChars];
    if (skipUnshiftedChars <= cnt)
      unshiftedChar = unshiftedChars.mChars[cnt - skipUnshiftedChars];
    nsAutoTArray<nsAlternativeCharCode, 5> altArray;

    if (shiftedChar || unshiftedChar) {
      nsAlternativeCharCode chars(unshiftedChar, shiftedChar);
      altArray.AppendElement(chars);
    }
    if (cnt == longestLength - 1) {
      if (unshiftedLatinChar || shiftedLatinChar) {
        nsAlternativeCharCode chars(unshiftedLatinChar, shiftedLatinChar);
        altArray.AppendElement(chars);
      }

      
      
      
      
      
      
      PRUnichar charForOEMKeyCode = 0;
      switch (mVirtualKeyCode) {
        case VK_OEM_PLUS:   charForOEMKeyCode = '+'; break;
        case VK_OEM_COMMA:  charForOEMKeyCode = ','; break;
        case VK_OEM_MINUS:  charForOEMKeyCode = '-'; break;
        case VK_OEM_PERIOD: charForOEMKeyCode = '.'; break;
      }
      if (charForOEMKeyCode &&
          charForOEMKeyCode != unshiftedChars.mChars[0] &&
          charForOEMKeyCode != shiftedChars.mChars[0] &&
          charForOEMKeyCode != unshiftedLatinChar &&
          charForOEMKeyCode != shiftedLatinChar) {
        nsAlternativeCharCode OEMChars(charForOEMKeyCode, charForOEMKeyCode);
        altArray.AppendElement(OEMChars);
      }
    }

    nsKeyEvent keypressEvent(true, NS_KEY_PRESS, mWidget);
    keypressEvent.charCode = uniChar;
    keypressEvent.alternativeCharCodes.AppendElements(altArray);
    InitKeyEvent(keypressEvent, modKeyState);
    defaultPrevented = (DispatchKeyEvent(keypressEvent) || defaultPrevented);
    if (mWidget->Destroyed()) {
      return true;
    }
  }

  return defaultPrevented;
}

bool
NativeKey::DispatchKeyPressEventForFollowingCharMessage() const
{
  MOZ_ASSERT(mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN);

  MSG msg = RemoveFollowingCharMessage();
  if (mFakeCharMsgs) {
    if (msg.message == WM_DEADCHAR) {
      return false;
    }
#ifdef DEBUG
    if (mIsPrintableKey) {
      nsPrintfCString log(
        "mOriginalVirtualKeyCode=0x%02X, mCommittedCharsAndModifiers={ "
        "mChars=[ 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X ], mLength=%d }, "
        "wParam=0x%04X",
        mOriginalVirtualKeyCode, mCommittedCharsAndModifiers.mChars[0],
        mCommittedCharsAndModifiers.mChars[1],
        mCommittedCharsAndModifiers.mChars[2],
        mCommittedCharsAndModifiers.mChars[3],
        mCommittedCharsAndModifiers.mChars[4],
        mCommittedCharsAndModifiers.mLength, msg.wParam);
      if (mCommittedCharsAndModifiers.IsEmpty()) {
        log.Insert("length is zero: ", 0);
        NS_ERROR(log.get());
        NS_ABORT();
      } else if (mCommittedCharsAndModifiers.mChars[0] != msg.wParam) {
        log.Insert("character mismatch: ", 0);
        NS_ERROR(log.get());
        NS_ABORT();
      }
    }
#endif 
    return HandleCharMessage(msg);
  }

  if (msg.message == WM_DEADCHAR) {
    if (!mWidget->PluginHasFocus()) {
      return false;
    }
    return (mWidget->DispatchPluginEvent(msg) || mWidget->Destroyed());
  }

  bool defaultPrevented = HandleCharMessage(msg);
  
  
  if (!defaultPrevented && msg.message == WM_SYSCHAR) {
    ::DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
  }
  return defaultPrevented;
}





KeyboardLayout* KeyboardLayout::sInstance = nullptr;
nsIIdleServiceInternal* KeyboardLayout::sIdleService = nullptr;


KeyboardLayout*
KeyboardLayout::GetInstance()
{
  if (!sInstance) {
    sInstance = new KeyboardLayout();
    nsCOMPtr<nsIIdleServiceInternal> idleService =
      do_GetService("@mozilla.org/widget/idleservice;1");
    
    sIdleService = idleService.forget().get();
  }
  return sInstance;
}


void
KeyboardLayout::Shutdown()
{
  delete sInstance;
  sInstance = nullptr;
  NS_IF_RELEASE(sIdleService);
}


void
KeyboardLayout::NotifyIdleServiceOfUserActivity()
{
  sIdleService->ResetIdleTimeOut(0);
}

KeyboardLayout::KeyboardLayout() :
  mKeyboardLayout(0), mIsOverridden(false),
  mIsPendingToRestoreKeyboardLayout(false)
{
  mDeadKeyTableListHead = nullptr;

  
}

KeyboardLayout::~KeyboardLayout()
{
  ReleaseDeadKeyTables();
}

bool
KeyboardLayout::IsPrintableCharKey(uint8_t aVirtualKey)
{
  return GetKeyIndex(aVirtualKey) >= 0;
}

WORD
KeyboardLayout::ComputeScanCodeForVirtualKeyCode(uint8_t aVirtualKeyCode) const
{
  return static_cast<WORD>(
           ::MapVirtualKeyEx(aVirtualKeyCode, MAPVK_VK_TO_VSC, GetLayout()));
}

bool
KeyboardLayout::IsDeadKey(uint8_t aVirtualKey,
                          const ModifierKeyState& aModKeyState) const
{
  int32_t virtualKeyIndex = GetKeyIndex(aVirtualKey);
  if (virtualKeyIndex < 0) {
    return false;
  }

  return mVirtualKeys[virtualKeyIndex].IsDeadKey(
           VirtualKey::ModifiersToShiftState(aModKeyState.GetModifiers()));
}

void
KeyboardLayout::InitNativeKey(NativeKey& aNativeKey,
                              const ModifierKeyState& aModKeyState)
{
  if (mIsPendingToRestoreKeyboardLayout) {
    LoadLayout(::GetKeyboardLayout(0));
  }

  uint8_t virtualKey = aNativeKey.mOriginalVirtualKeyCode;
  int32_t virtualKeyIndex = GetKeyIndex(virtualKey);

  if (virtualKeyIndex < 0) {
    
    
    return;
  }

  bool isKeyDown = aNativeKey.IsKeyDownMessage();
  uint8_t shiftState =
    VirtualKey::ModifiersToShiftState(aModKeyState.GetModifiers());

  if (mVirtualKeys[virtualKeyIndex].IsDeadKey(shiftState)) {
    if ((isKeyDown && mActiveDeadKey < 0) ||
        (!isKeyDown && mActiveDeadKey == virtualKey)) {
      
      if (isKeyDown) {
        
        mActiveDeadKey = virtualKey;
        mDeadKeyShiftState = shiftState;
      }
      UniCharsAndModifiers deadChars =
        mVirtualKeys[virtualKeyIndex].GetNativeUniChars(shiftState);
      NS_ASSERTION(deadChars.mLength == 1,
                   "dead key must generate only one character");
      aNativeKey.mKeyNameIndex =
        WidgetUtils::GetDeadKeyNameIndex(deadChars.mChars[0]);
      return;
    }

    
    
    
    
    
    
    if (mActiveDeadKey < 0) {
      aNativeKey.mCommittedCharsAndModifiers =
        mVirtualKeys[virtualKeyIndex].GetUniChars(shiftState);
      return;
    }

    int32_t activeDeadKeyIndex = GetKeyIndex(mActiveDeadKey);
    if (activeDeadKeyIndex < 0 || activeDeadKeyIndex >= NS_NUM_OF_KEYS) {
#if defined(DEBUG) || defined(MOZ_CRASHREPORTER)
      nsPrintfCString warning("The virtual key index (%d) of mActiveDeadKey "
                              "(0x%02X) is not a printable key (virtualKey="
                              "0x%02X)",
                              activeDeadKeyIndex, mActiveDeadKey, virtualKey);
      NS_WARNING(warning.get());
#ifdef MOZ_CRASHREPORTER
      CrashReporter::AppendAppNotesToCrashReport(
                       NS_LITERAL_CSTRING("\n") + warning);
#endif 
#endif 
      MOZ_CRASH("Trying to reference out of range of mVirtualKeys");
    }
    UniCharsAndModifiers prevDeadChars =
      mVirtualKeys[activeDeadKeyIndex].GetUniChars(mDeadKeyShiftState);
    UniCharsAndModifiers newChars =
      mVirtualKeys[virtualKeyIndex].GetUniChars(shiftState);
    
    aNativeKey.mCommittedCharsAndModifiers = prevDeadChars + newChars;
    if (isKeyDown) {
      DeactivateDeadKeyState();
    }
    return;
  }

  UniCharsAndModifiers baseChars =
    mVirtualKeys[virtualKeyIndex].GetUniChars(shiftState);
  if (mActiveDeadKey < 0) {
    
    aNativeKey.mCommittedCharsAndModifiers = baseChars;
    return;
  }

  
  
  int32_t activeDeadKeyIndex = GetKeyIndex(mActiveDeadKey);
  PRUnichar compositeChar = (baseChars.mLength == 1 && baseChars.mChars[0]) ?
    mVirtualKeys[activeDeadKeyIndex].GetCompositeChar(mDeadKeyShiftState,
                                                      baseChars.mChars[0]) : 0;
  if (compositeChar) {
    
    
    aNativeKey.mCommittedCharsAndModifiers.Append(compositeChar,
                                                  baseChars.mModifiers[0]);
    if (isKeyDown) {
      DeactivateDeadKeyState();
    }
    return;
  }

  
  
  UniCharsAndModifiers deadChars =
    mVirtualKeys[activeDeadKeyIndex].GetUniChars(mDeadKeyShiftState);
  
  aNativeKey.mCommittedCharsAndModifiers = deadChars + baseChars;
  if (isKeyDown) {
    DeactivateDeadKeyState();
  }

  return;
}

UniCharsAndModifiers
KeyboardLayout::GetUniCharsAndModifiers(
                  uint8_t aVirtualKey,
                  const ModifierKeyState& aModKeyState) const
{
  UniCharsAndModifiers result;
  int32_t key = GetKeyIndex(aVirtualKey);
  if (key < 0) {
    return result;
  }
  return mVirtualKeys[key].
    GetUniChars(VirtualKey::ModifiersToShiftState(aModKeyState.GetModifiers()));
}

void
KeyboardLayout::LoadLayout(HKL aLayout)
{
  mIsPendingToRestoreKeyboardLayout = false;

  if (mKeyboardLayout == aLayout) {
    return;
  }

  mKeyboardLayout = aLayout;

  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));

  BYTE originalKbdState[256];
  
  uint16_t shiftStatesWithDeadKeys = 0;
  
  
  uint16_t shiftStatesWithBaseChars = 0;

  mActiveDeadKey = -1;

  ReleaseDeadKeyTables();

  ::GetKeyboardState(originalKbdState);

  
  

  for (VirtualKey::ShiftState shiftState = 0; shiftState < 16; shiftState++) {
    VirtualKey::FillKbdState(kbdState, shiftState);
    for (uint32_t virtualKey = 0; virtualKey < 256; virtualKey++) {
      int32_t vki = GetKeyIndex(virtualKey);
      if (vki < 0) {
        continue;
      }
      NS_ASSERTION(uint32_t(vki) < ArrayLength(mVirtualKeys), "invalid index");
      PRUnichar uniChars[5];
      int32_t ret =
        ::ToUnicodeEx(virtualKey, 0, kbdState, (LPWSTR)uniChars,
                      ArrayLength(uniChars), 0, mKeyboardLayout);
      
      if (ret < 0) {
        shiftStatesWithDeadKeys |= (1 << shiftState);
        
        
        PRUnichar deadChar[2];
        ret = ::ToUnicodeEx(virtualKey, 0, kbdState, (LPWSTR)deadChar,
                            ArrayLength(deadChar), 0, mKeyboardLayout);
        NS_ASSERTION(ret == 2, "Expecting twice repeated dead-key character");
        mVirtualKeys[vki].SetDeadChar(shiftState, deadChar[0]);
      } else {
        if (ret == 1) {
          
          shiftStatesWithBaseChars |= (1 << shiftState);
        }
        mVirtualKeys[vki].SetNormalChars(shiftState, uniChars, ret);
      }
    }
  }

  
  
  for (VirtualKey::ShiftState shiftState = 0; shiftState < 16; shiftState++) {
    if (!(shiftStatesWithDeadKeys & (1 << shiftState))) {
      continue;
    }

    VirtualKey::FillKbdState(kbdState, shiftState);

    for (uint32_t virtualKey = 0; virtualKey < 256; virtualKey++) {
      int32_t vki = GetKeyIndex(virtualKey);
      if (vki >= 0 && mVirtualKeys[vki].IsDeadKey(shiftState)) {
        DeadKeyEntry deadKeyArray[256];
        int32_t n = GetDeadKeyCombinations(virtualKey, kbdState,
                                           shiftStatesWithBaseChars,
                                           deadKeyArray,
                                           ArrayLength(deadKeyArray));
        const DeadKeyTable* dkt =
          mVirtualKeys[vki].MatchingDeadKeyTable(deadKeyArray, n);
        if (!dkt) {
          dkt = AddDeadKeyTable(deadKeyArray, n);
        }
        mVirtualKeys[vki].AttachDeadKeyTable(shiftState, dkt);
      }
    }
  }

  ::SetKeyboardState(originalKbdState);
}

inline int32_t
KeyboardLayout::GetKeyIndex(uint8_t aVirtualKey)
{































  static const int8_t xlat[256] =
  {
  
  
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
     0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, -1, -1, -1, -1, -1, -1,   
    -1, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,   
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, -1, -1, -1, -1, -1,   
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, -1, 49, 50, 51,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 52, 53, 54, 55, 56, 57,   
    58, 59, 60, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 61, 62, 63, 64, 65,   
    -1, 66, 67, 68, 69, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1    
  };

  return xlat[aVirtualKey];
}

int
KeyboardLayout::CompareDeadKeyEntries(const void* aArg1,
                                      const void* aArg2,
                                      void*)
{
  const DeadKeyEntry* arg1 = static_cast<const DeadKeyEntry*>(aArg1);
  const DeadKeyEntry* arg2 = static_cast<const DeadKeyEntry*>(aArg2);

  return arg1->BaseChar - arg2->BaseChar;
}

const DeadKeyTable*
KeyboardLayout::AddDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                uint32_t aEntries)
{
  DeadKeyTableListEntry* next = mDeadKeyTableListHead;

  const size_t bytes = offsetof(DeadKeyTableListEntry, data) +
    DeadKeyTable::SizeInBytes(aEntries);
  uint8_t* p = new uint8_t[bytes];

  mDeadKeyTableListHead = reinterpret_cast<DeadKeyTableListEntry*>(p);
  mDeadKeyTableListHead->next = next;

  DeadKeyTable* dkt =
    reinterpret_cast<DeadKeyTable*>(mDeadKeyTableListHead->data);

  dkt->Init(aDeadKeyArray, aEntries);

  return dkt;
}

void
KeyboardLayout::ReleaseDeadKeyTables()
{
  while (mDeadKeyTableListHead) {
    uint8_t* p = reinterpret_cast<uint8_t*>(mDeadKeyTableListHead);
    mDeadKeyTableListHead = mDeadKeyTableListHead->next;

    delete [] p;
  }
}

bool
KeyboardLayout::EnsureDeadKeyActive(bool aIsActive,
                                    uint8_t aDeadKey,
                                    const PBYTE aDeadKeyKbdState)
{
  int32_t ret;
  do {
    PRUnichar dummyChars[5];
    ret = ::ToUnicodeEx(aDeadKey, 0, (PBYTE)aDeadKeyKbdState,
                        (LPWSTR)dummyChars, ArrayLength(dummyChars), 0,
                        mKeyboardLayout);
    
    
    
    
    
    
    
  } while ((ret < 0) != aIsActive);

  return (ret < 0);
}

void
KeyboardLayout::DeactivateDeadKeyState()
{
  if (mActiveDeadKey < 0) {
    return;
  }

  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));

  VirtualKey::FillKbdState(kbdState, mDeadKeyShiftState);

  EnsureDeadKeyActive(false, mActiveDeadKey, kbdState);
  mActiveDeadKey = -1;
}

bool
KeyboardLayout::AddDeadKeyEntry(PRUnichar aBaseChar,
                                PRUnichar aCompositeChar,
                                DeadKeyEntry* aDeadKeyArray,
                                uint32_t aEntries)
{
  for (uint32_t index = 0; index < aEntries; index++) {
    if (aDeadKeyArray[index].BaseChar == aBaseChar) {
      return false;
    }
  }

  aDeadKeyArray[aEntries].BaseChar = aBaseChar;
  aDeadKeyArray[aEntries].CompositeChar = aCompositeChar;

  return true;
}

uint32_t
KeyboardLayout::GetDeadKeyCombinations(uint8_t aDeadKey,
                                       const PBYTE aDeadKeyKbdState,
                                       uint16_t aShiftStatesWithBaseChars,
                                       DeadKeyEntry* aDeadKeyArray,
                                       uint32_t aMaxEntries)
{
  bool deadKeyActive = false;
  uint32_t entries = 0;
  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));

  for (uint32_t shiftState = 0; shiftState < 16; shiftState++) {
    if (!(aShiftStatesWithBaseChars & (1 << shiftState))) {
      continue;
    }

    VirtualKey::FillKbdState(kbdState, shiftState);

    for (uint32_t virtualKey = 0; virtualKey < 256; virtualKey++) {
      int32_t vki = GetKeyIndex(virtualKey);
      
      
      if (vki >= 0 &&
          mVirtualKeys[vki].GetNativeUniChars(shiftState).mLength == 1) {
        
        
        if (!deadKeyActive) {
          deadKeyActive = EnsureDeadKeyActive(true, aDeadKey,
                                              aDeadKeyKbdState);
        }

        
        
        
        PRUnichar compositeChars[5];
        int32_t ret =
          ::ToUnicodeEx(virtualKey, 0, kbdState, (LPWSTR)compositeChars,
                        ArrayLength(compositeChars), 0, mKeyboardLayout);
        switch (ret) {
          case 0:
            
            
            break;
          case 1: {
            
            
            
            PRUnichar baseChars[5];
            ret = ::ToUnicodeEx(virtualKey, 0, kbdState, (LPWSTR)baseChars,
                                ArrayLength(baseChars), 0, mKeyboardLayout);
            NS_ASSERTION(ret == 1, "One base character expected");
            if (ret == 1 && entries < aMaxEntries &&
                AddDeadKeyEntry(baseChars[0], compositeChars[0],
                                aDeadKeyArray, entries)) {
              entries++;
            }
            deadKeyActive = false;
            break;
          }
          default:
            
            
            
            deadKeyActive = false;
            break;
        }
      }
    }
  }

  if (deadKeyActive) {
    deadKeyActive = EnsureDeadKeyActive(false, aDeadKey, aDeadKeyKbdState);
  }

  NS_QuickSort(aDeadKeyArray, entries, sizeof(DeadKeyEntry),
               CompareDeadKeyEntries, nullptr);
  return entries;
}

uint32_t
KeyboardLayout::ConvertNativeKeyCodeToDOMKeyCode(UINT aNativeKeyCode) const
{
  
  if ((aNativeKeyCode >= 0x30 && aNativeKeyCode <= 0x39) ||
      (aNativeKeyCode >= 0x41 && aNativeKeyCode <= 0x5A) ||
      (aNativeKeyCode >= 0x60 && aNativeKeyCode <= 0x87)) {
    return static_cast<uint32_t>(aNativeKeyCode);
  }
  switch (aNativeKeyCode) {
    
    case VK_CANCEL:
    case VK_BACK:
    case VK_TAB:
    case VK_CLEAR:
    case VK_RETURN:
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU: 
    case VK_PAUSE:
    case VK_CAPITAL: 
    case VK_KANA: 
    case VK_JUNJA:
    case VK_FINAL:
    case VK_HANJA: 
    case VK_ESCAPE:
    case VK_CONVERT:
    case VK_NONCONVERT:
    case VK_ACCEPT:
    case VK_MODECHANGE:
    case VK_SPACE:
    case VK_PRIOR: 
    case VK_NEXT: 
    case VK_END:
    case VK_HOME:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_SELECT:
    case VK_PRINT:
    case VK_EXECUTE:
    case VK_SNAPSHOT:
    case VK_INSERT:
    case VK_DELETE:
    case VK_APPS: 
    case VK_SLEEP:
    case VK_NUMLOCK:
    case VK_SCROLL: 
    case VK_ATTN: 
    case VK_CRSEL: 
    case VK_EXSEL: 
    case VK_EREOF: 
    case VK_PLAY:
    case VK_ZOOM:
    case VK_PA1: 
      return uint32_t(aNativeKeyCode);

    case VK_HELP:
      return NS_VK_HELP;

    
    
    case VK_LWIN:
    case VK_RWIN:
      return NS_VK_WIN;

    case VK_VOLUME_MUTE:
      return NS_VK_VOLUME_MUTE;
    case VK_VOLUME_DOWN:
      return NS_VK_VOLUME_DOWN;
    case VK_VOLUME_UP:
      return NS_VK_VOLUME_UP;

    
    case VK_BROWSER_BACK:
    case VK_BROWSER_FORWARD:
    case VK_BROWSER_REFRESH:
    case VK_BROWSER_STOP:
    case VK_BROWSER_SEARCH:
    case VK_BROWSER_FAVORITES:
    case VK_BROWSER_HOME:
    case VK_MEDIA_NEXT_TRACK:
    case VK_MEDIA_STOP:
    case VK_MEDIA_PLAY_PAUSE:
    case VK_LAUNCH_MAIL:
    case VK_LAUNCH_MEDIA_SELECT:
    case VK_LAUNCH_APP1:
    case VK_LAUNCH_APP2:
      return 0;

    
    

    
    case VK_OEM_FJ_JISHO:
    case VK_OEM_FJ_MASSHOU:
    case VK_OEM_FJ_TOUROKU:
    case VK_OEM_FJ_LOYA:
    case VK_OEM_FJ_ROYA:
    
    case VK_ICO_HELP:
    case VK_ICO_00:
    case VK_ICO_CLEAR:
    
    case VK_OEM_RESET:
    case VK_OEM_JUMP:
    case VK_OEM_PA1:
    case VK_OEM_PA2:
    case VK_OEM_PA3:
    case VK_OEM_WSCTRL:
    case VK_OEM_CUSEL:
    case VK_OEM_ATTN:
    case VK_OEM_FINISH:
    case VK_OEM_COPY:
    case VK_OEM_AUTO:
    case VK_OEM_ENLW:
    case VK_OEM_BACKTAB:
    
    
    case VK_OEM_CLEAR:
      return uint32_t(aNativeKeyCode);

    
    
    
    case 0xE1:
      return 0;

    
    
    
    
    
    case VK_OEM_1:
    case VK_OEM_PLUS:
    case VK_OEM_COMMA:
    case VK_OEM_MINUS:
    case VK_OEM_PERIOD:
    case VK_OEM_2:
    case VK_OEM_3:
    case VK_OEM_4:
    case VK_OEM_5:
    case VK_OEM_6:
    case VK_OEM_7:
    case VK_OEM_8:
    case VK_OEM_102:
    case VK_ABNT_C1:
    {
      NS_ASSERTION(IsPrintableCharKey(aNativeKeyCode),
                   "The key must be printable");
      ModifierKeyState modKeyState(0);
      UniCharsAndModifiers uniChars =
        GetUniCharsAndModifiers(aNativeKeyCode, modKeyState);
      if (uniChars.mLength != 1 ||
          uniChars.mChars[0] < ' ' || uniChars.mChars[0] > 0x7F) {
        modKeyState.Set(MODIFIER_SHIFT);
        uniChars = GetUniCharsAndModifiers(aNativeKeyCode, modKeyState);
        if (uniChars.mLength != 1 ||
            uniChars.mChars[0] < ' ' || uniChars.mChars[0] > 0x7F) {
          return 0;
        }
      }
      return WidgetUtils::ComputeKeyCodeFromChar(uniChars.mChars[0]);
    }

    
    
    
    
    
    case VK_ABNT_C2:
      return NS_VK_SEPARATOR;

    
    case VK_PROCESSKEY:
      return 0;
    
    
    case VK_PACKET:
      return 0;
    
    case 0xFF:
      NS_WARNING("The key is failed to be converted to a virtual keycode");
      return 0;
  }
#ifdef DEBUG
  nsPrintfCString warning("Unknown virtual keycode (0x%08X), please check the "
                          "latest MSDN document, there may be some new "
                          "keycodes we've never known.",
                          aNativeKeyCode);
  NS_WARNING(warning.get());
#endif
  return 0;
}

KeyNameIndex
KeyboardLayout::ConvertNativeKeyCodeToKeyNameIndex(uint8_t aVirtualKey) const
{
#define NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)
#define NS_JAPANESE_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)
#define NS_KOREAN_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)
#define NS_OTHER_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)

  switch (aVirtualKey) {

#undef NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex) \
    case aNativeKey: return aKeyNameIndex;

#include "NativeKeyToDOMKeyName.h"

#undef NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)

    default:
      if (IsPrintableCharKey(aVirtualKey)) {
        return KEY_NAME_INDEX_PrintableKey;
      }
      break;
  }

  HKL layout = GetLayout();
  WORD langID = LOWORD(static_cast<HKL>(layout));
  WORD primaryLangID = PRIMARYLANGID(langID);

  if (primaryLangID == LANG_JAPANESE) {
    switch (aVirtualKey) {

#undef NS_JAPANESE_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_JAPANESE_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)\
      case aNativeKey: return aKeyNameIndex;

#include "NativeKeyToDOMKeyName.h"

#undef NS_JAPANESE_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_JAPANESE_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)

      default:
        break;
    }
  } else if (primaryLangID == LANG_KOREAN) {
    switch (aVirtualKey) {

#undef NS_KOREAN_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_KOREAN_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)\
      case aNativeKey: return aKeyNameIndex;

#include "NativeKeyToDOMKeyName.h"

#undef NS_KOREAN_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_KOREAN_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)

      default:
        return KEY_NAME_INDEX_Unidentified;
    }
  }

  switch (aVirtualKey) {

#undef NS_OTHER_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#define NS_OTHER_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex)\
    case aNativeKey: return aKeyNameIndex;

#include "NativeKeyToDOMKeyName.h"

#undef NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#undef NS_JAPANESE_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#undef NS_KOREAN_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX
#undef NS_OTHER_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX

    default:
      return KEY_NAME_INDEX_Unidentified;
  }
}

nsresult
KeyboardLayout::SynthesizeNativeKeyEvent(nsWindowBase* aWidget,
                                         int32_t aNativeKeyboardLayout,
                                         int32_t aNativeKeyCode,
                                         uint32_t aModifierFlags,
                                         const nsAString& aCharacters,
                                         const nsAString& aUnmodifiedCharacters)
{
  UINT keyboardLayoutListCount = ::GetKeyboardLayoutList(0, NULL);
  NS_ASSERTION(keyboardLayoutListCount > 0,
               "One keyboard layout must be installed at least");
  HKL keyboardLayoutListBuff[50];
  HKL* keyboardLayoutList =
    keyboardLayoutListCount < 50 ? keyboardLayoutListBuff :
                                   new HKL[keyboardLayoutListCount];
  keyboardLayoutListCount =
    ::GetKeyboardLayoutList(keyboardLayoutListCount, keyboardLayoutList);
  NS_ASSERTION(keyboardLayoutListCount > 0,
               "Failed to get all keyboard layouts installed on the system");

  nsPrintfCString layoutName("%08x", aNativeKeyboardLayout);
  HKL loadedLayout = LoadKeyboardLayoutA(layoutName.get(), KLF_NOTELLSHELL);
  if (loadedLayout == NULL) {
    if (keyboardLayoutListBuff != keyboardLayoutList) {
      delete [] keyboardLayoutList;
    }
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  BYTE originalKbdState[256];
  ::GetKeyboardState(originalKbdState);
  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));
  
  
  ::SetKeyboardState(kbdState);

  OverrideLayout(loadedLayout);

  uint8_t argumentKeySpecific = 0;
  switch (aNativeKeyCode) {
    case VK_SHIFT:
      aModifierFlags &= ~(nsIWidget::SHIFT_L | nsIWidget::SHIFT_R);
      argumentKeySpecific = VK_LSHIFT;
      break;
    case VK_LSHIFT:
      aModifierFlags &= ~nsIWidget::SHIFT_L;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_SHIFT;
      break;
    case VK_RSHIFT:
      aModifierFlags &= ~nsIWidget::SHIFT_R;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_SHIFT;
      break;
    case VK_CONTROL:
      aModifierFlags &= ~(nsIWidget::CTRL_L | nsIWidget::CTRL_R);
      argumentKeySpecific = VK_LCONTROL;
      break;
    case VK_LCONTROL:
      aModifierFlags &= ~nsIWidget::CTRL_L;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_CONTROL;
      break;
    case VK_RCONTROL:
      aModifierFlags &= ~nsIWidget::CTRL_R;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_CONTROL;
      break;
    case VK_MENU:
      aModifierFlags &= ~(nsIWidget::ALT_L | nsIWidget::ALT_R);
      argumentKeySpecific = VK_LMENU;
      break;
    case VK_LMENU:
      aModifierFlags &= ~nsIWidget::ALT_L;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_MENU;
      break;
    case VK_RMENU:
      aModifierFlags &= ~nsIWidget::ALT_R;
      argumentKeySpecific = aNativeKeyCode;
      aNativeKeyCode = VK_MENU;
      break;
    case VK_CAPITAL:
      aModifierFlags &= ~nsIWidget::CAPS_LOCK;
      argumentKeySpecific = VK_CAPITAL;
      break;
    case VK_NUMLOCK:
      aModifierFlags &= ~nsIWidget::NUM_LOCK;
      argumentKeySpecific = VK_NUMLOCK;
      break;
  }

  nsAutoTArray<KeyPair,10> keySequence;
  WinUtils::SetupKeyModifiersSequence(&keySequence, aModifierFlags);
  NS_ASSERTION(aNativeKeyCode >= 0 && aNativeKeyCode < 256,
               "Native VK key code out of range");
  keySequence.AppendElement(KeyPair(aNativeKeyCode, argumentKeySpecific));

  
  for (uint32_t i = 0; i < keySequence.Length(); ++i) {
    uint8_t key = keySequence[i].mGeneral;
    uint8_t keySpecific = keySequence[i].mSpecific;
    kbdState[key] = 0x81; 
    if (keySpecific) {
      kbdState[keySpecific] = 0x81;
    }
    ::SetKeyboardState(kbdState);
    ModifierKeyState modKeyState;
    UINT scanCode =
      ComputeScanCodeForVirtualKeyCode(keySpecific ? keySpecific : key);
    LPARAM lParam = static_cast<LPARAM>(scanCode << 16);
    
    
    if (keySpecific == VK_RCONTROL || keySpecific == VK_RMENU) {
      lParam |= 0x1000000;
    }
    MSG keyDownMsg = WinUtils::InitMSG(WM_KEYDOWN, key, lParam,
                                       aWidget->GetWindowHandle());
    if (i == keySequence.Length() - 1) {
      bool makeDeadCharMsg =
        (IsDeadKey(key, modKeyState) && aCharacters.IsEmpty());
      nsAutoString chars(aCharacters);
      if (makeDeadCharMsg) {
        UniCharsAndModifiers deadChars =
          GetUniCharsAndModifiers(key, modKeyState);
        chars = deadChars.ToString();
        NS_ASSERTION(chars.Length() == 1,
                     "Dead char must be only one character");
      }
      if (chars.IsEmpty()) {
        NativeKey nativeKey(aWidget, keyDownMsg, modKeyState);
        nativeKey.HandleKeyDownMessage();
      } else {
        nsAutoTArray<NativeKey::FakeCharMsg, 10> fakeCharMsgs;
        for (uint32_t j = 0; j < chars.Length(); j++) {
          NativeKey::FakeCharMsg* fakeCharMsg = fakeCharMsgs.AppendElement();
          fakeCharMsg->mCharCode = chars.CharAt(j);
          fakeCharMsg->mScanCode = scanCode;
          fakeCharMsg->mIsDeadKey = makeDeadCharMsg;
        }
        NativeKey nativeKey(aWidget, keyDownMsg, modKeyState, &fakeCharMsgs);
        bool dispatched;
        nativeKey.HandleKeyDownMessage(&dispatched);
        
        
        for (uint32_t j = 1; j < fakeCharMsgs.Length(); j++) {
          if (fakeCharMsgs[j].mConsumed) {
            continue;
          }
          MSG charMsg = fakeCharMsgs[j].GetCharMsg(aWidget->GetWindowHandle());
          NativeKey nativeKey(aWidget, charMsg, modKeyState);
          nativeKey.HandleCharMessage(charMsg);
        }
      }
    } else {
      NativeKey nativeKey(aWidget, keyDownMsg, modKeyState);
      nativeKey.HandleKeyDownMessage();
    }
  }
  for (uint32_t i = keySequence.Length(); i > 0; --i) {
    uint8_t key = keySequence[i - 1].mGeneral;
    uint8_t keySpecific = keySequence[i - 1].mSpecific;
    kbdState[key] = 0; 
    if (keySpecific) {
      kbdState[keySpecific] = 0;
    }
    ::SetKeyboardState(kbdState);
    ModifierKeyState modKeyState;
    UINT scanCode =
      ComputeScanCodeForVirtualKeyCode(keySpecific ? keySpecific : key);
    LPARAM lParam = static_cast<LPARAM>(scanCode << 16);
    
    
    if (keySpecific == VK_RCONTROL || keySpecific == VK_RMENU) {
      lParam |= 0x1000000;
    }
    MSG keyUpMsg = WinUtils::InitMSG(WM_KEYUP, key, lParam,
                                     aWidget->GetWindowHandle());
    NativeKey nativeKey(aWidget, keyUpMsg, modKeyState);
    nativeKey.HandleKeyUpMessage();
  }

  
  ::SetKeyboardState(originalKbdState);
  RestoreLayout();

  
  for (uint32_t i = 0; i < keyboardLayoutListCount; i++) {
    if (keyboardLayoutList[i] == loadedLayout) {
      loadedLayout = 0;
      break;
    }
  }
  if (keyboardLayoutListBuff != keyboardLayoutList) {
    delete [] keyboardLayoutList;
  }
  if (loadedLayout) {
    ::UnloadKeyboardLayout(loadedLayout);
  }
  return NS_OK;
}





PRUnichar
DeadKeyTable::GetCompositeChar(PRUnichar aBaseChar) const
{
  
  

  for (uint32_t index = 0; index < mEntries; index++) {
    if (mTable[index].BaseChar == aBaseChar) {
      return mTable[index].CompositeChar;
    }
    if (mTable[index].BaseChar > aBaseChar) {
      break;
    }
  }

  return 0;
}





MSG RedirectedKeyDownMessageManager::sRedirectedKeyDownMsg;
bool RedirectedKeyDownMessageManager::sDefaultPreventedOfRedirectedMsg = false;


bool
RedirectedKeyDownMessageManager::IsRedirectedMessage(const MSG& aMsg)
{
  return (aMsg.message == WM_KEYDOWN || aMsg.message == WM_SYSKEYDOWN) &&
         (sRedirectedKeyDownMsg.message == aMsg.message &&
          WinUtils::GetScanCode(sRedirectedKeyDownMsg.lParam) ==
            WinUtils::GetScanCode(aMsg.lParam));
}


void
RedirectedKeyDownMessageManager::RemoveNextCharMessage(HWND aWnd)
{
  MSG msg;
  if (WinUtils::PeekMessage(&msg, aWnd, WM_KEYFIRST, WM_KEYLAST,
                            PM_NOREMOVE | PM_NOYIELD) &&
      (msg.message == WM_CHAR || msg.message == WM_SYSCHAR)) {
    WinUtils::GetMessage(&msg, aWnd, msg.message, msg.message);
  }
}

} 
} 

