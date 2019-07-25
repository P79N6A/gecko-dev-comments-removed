




#include "mozilla/Util.h"

#include "KeyboardLayout.h"
#include "nsWindow.h"
#include "nsIMM32Handler.h"

#include "nsMemory.h"
#include "nsToolkit.h"
#include "nsQuickSort.h"
#include "nsAlgorithm.h"
#include "nsGUIEvent.h"
#include "nsUnicharUtils.h"
#include "WidgetUtils.h"
#include "WinUtils.h"

#include "nsIDOMKeyEvent.h"

#include <windows.h>
#include <winuser.h>

#ifndef WINABLEAPI
#include <winable.h>
#endif

namespace mozilla {
namespace widget {

struct DeadKeyEntry
{
  PRUnichar BaseChar;
  PRUnichar CompositeChar;
};


class DeadKeyTable
{
  friend class KeyboardLayout;

  PRUint16 mEntries;
  
  
  DeadKeyEntry mTable[1];

  void Init(const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries)
  {
    mEntries = aEntries;
    memcpy(mTable, aDeadKeyArray, aEntries * sizeof(DeadKeyEntry));
  }

  static PRUint32 SizeInBytes(PRUint32 aEntries)
  {
    return offsetof(DeadKeyTable, mTable) + aEntries * sizeof(DeadKeyEntry);
  }

public:
  PRUint32 Entries() const
  {
    return mEntries;
  }

  bool IsEqual(const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries) const
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
    mModifiers |= MODIFIER_WIN;
  }
  if (::GetKeyState(VK_CAPITAL) & 1) {
    mModifiers |= MODIFIER_CAPSLOCK;
  }
  if (::GetKeyState(VK_NUMLOCK) & 1) {
    mModifiers |= MODIFIER_NUMLOCK;
  }
  if (::GetKeyState(VK_SCROLL) & 1) {
    mModifiers |= MODIFIER_SCROLL;
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
    case NS_DRAG_EVENT:
    case NS_SIMPLE_GESTURE_EVENT:
    case NS_MOZTOUCH_EVENT:
      InitMouseEvent(aInputEvent);
      break;
  }
}

void
ModifierKeyState::InitMouseEvent(nsInputEvent& aMouseEvent) const
{
  NS_ASSERTION(aMouseEvent.eventStructType == NS_MOUSE_EVENT ||
               aMouseEvent.eventStructType == NS_MOUSE_SCROLL_EVENT ||
               aMouseEvent.eventStructType == NS_DRAG_EVENT ||
               aMouseEvent.eventStructType == NS_SIMPLE_GESTURE_EVENT ||
               aMouseEvent.eventStructType == NS_MOZTOUCH_EVENT,
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
  for (PRUint32 i = 0; i < mLength; i++) {
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
  PRUint32 copyCount = NS_MIN(aOther.mLength, 5 - mLength);
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
                                 PRUint32 aEntries) const
{
  if (!mIsDeadKey) {
    return nsnull;
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

  return nsnull;
}

void
VirtualKey::SetNormalChars(ShiftState aShiftState,
                           const PRUnichar* aChars,
                           PRUint32 aNumOfChars)
{
  NS_ASSERTION(aShiftState < ArrayLength(mShiftStates), "invalid index");

  SetDeadKey(aShiftState, false);

  for (PRUint32 index = 0; index < aNumOfChars; index++) {
    
    mShiftStates[aShiftState].Normal.Chars[index] =
      (aChars[index] >= 0x20) ? aChars[index] : 0;
  }

  PRUint32 len = ArrayLength(mShiftStates[aShiftState].Normal.Chars);
  for (PRUint32 index = aNumOfChars; index < len; index++) {
    mShiftStates[aShiftState].Normal.Chars[index] = 0;
  }
}

void
VirtualKey::SetDeadChar(ShiftState aShiftState, PRUnichar aDeadChar)
{
  NS_ASSERTION(aShiftState < ArrayLength(mShiftStates), "invalid index");

  SetDeadKey(aShiftState, true);

  mShiftStates[aShiftState].DeadKey.DeadChar = aDeadChar;
  mShiftStates[aShiftState].DeadKey.Table = nsnull;
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
  UniCharsAndModifiers result;
  Modifiers modifiers = ShiftStateToModifiers(aShiftState);
  if (IsDeadKey(aShiftState)) {
    result.Append(mShiftStates[aShiftState].DeadKey.DeadChar, modifiers);
    return result;
  }

  PRUint32 index;
  PRUint32 len = ArrayLength(mShiftStates[aShiftState].Normal.Chars);
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





NativeKey::NativeKey(const KeyboardLayout& aKeyboardLayout,
                     nsWindow* aWindow,
                     const MSG& aKeyOrCharMessage) :
  mDOMKeyCode(0), mVirtualKeyCode(0), mOriginalVirtualKeyCode(0)
{
  mScanCode = WinUtils::GetScanCode(aKeyOrCharMessage.lParam);
  mIsExtended = WinUtils::IsExtendedScanCode(aKeyOrCharMessage.lParam);
  switch (aKeyOrCharMessage.message) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
      mOriginalVirtualKeyCode = static_cast<PRUint8>(aKeyOrCharMessage.wParam);
      switch (aKeyOrCharMessage.wParam) {
        case VK_CONTROL:
        case VK_MENU:
        case VK_SHIFT:
          mVirtualKeyCode = static_cast<PRUint8>(
            ::MapVirtualKeyEx(GetScanCodeWithExtendedFlag(),
                              MAPVK_VSC_TO_VK_EX, aKeyboardLayout.GetLayout()));
          break;
        case VK_PROCESSKEY:
          mVirtualKeyCode = mOriginalVirtualKeyCode =
            static_cast<PRUint8>(
              ::ImmGetVirtualKey(aWindow->GetWindowHandle()));
          break;
        default:
          mVirtualKeyCode = mOriginalVirtualKeyCode;
          break;
      }
      break;
    case WM_CHAR:
    case WM_UNICHAR:
    case WM_SYSCHAR:
      
      
      if (mIsExtended &&
          WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION) {
        break;
      }
      mVirtualKeyCode = mOriginalVirtualKeyCode = static_cast<PRUint8>(
        ::MapVirtualKeyEx(GetScanCodeWithExtendedFlag(),
                          MAPVK_VSC_TO_VK_EX, aKeyboardLayout.GetLayout()));
      break;
    default:
      MOZ_NOT_REACHED("Unsupported message");
      break;
  }

  if (!mVirtualKeyCode) {
    mVirtualKeyCode = mOriginalVirtualKeyCode;
  }

  mDOMKeyCode =
    aKeyboardLayout.ConvertNativeKeyCodeToDOMKeyCode(mOriginalVirtualKeyCode);
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

PRUint32
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
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_NUMPAD;

    default:
      return nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD;
  }
}





KeyboardLayout::KeyboardLayout() :
  mKeyboardLayout(0), mPendingKeyboardLayout(0)
{
  mDeadKeyTableListHead = nsnull;

  
  
  
}

KeyboardLayout::~KeyboardLayout()
{
  ReleaseDeadKeyTables();
}

bool
KeyboardLayout::IsPrintableCharKey(PRUint8 aVirtualKey)
{
  return GetKeyIndex(aVirtualKey) >= 0;
}

bool
KeyboardLayout::IsDeadKey(PRUint8 aVirtualKey,
                          const ModifierKeyState& aModKeyState) const
{
  PRInt32 virtualKeyIndex = GetKeyIndex(aVirtualKey);
  if (virtualKeyIndex < 0) {
    return false;
  }

  return mVirtualKeys[virtualKeyIndex].IsDeadKey(
           VirtualKey::ModifiersToShiftState(aModKeyState.GetModifiers()));
}

UniCharsAndModifiers
KeyboardLayout::OnKeyDown(PRUint8 aVirtualKey,
                          const ModifierKeyState& aModKeyState)
{
  if (mPendingKeyboardLayout) {
    LoadLayout(mPendingKeyboardLayout);
  }

  PRInt32 virtualKeyIndex = GetKeyIndex(aVirtualKey);

  if (virtualKeyIndex < 0) {
    
    
    return UniCharsAndModifiers();
  }

  PRUint8 shiftState =
    VirtualKey::ModifiersToShiftState(aModKeyState.GetModifiers());

  if (mVirtualKeys[virtualKeyIndex].IsDeadKey(shiftState)) {
    if (mActiveDeadKey < 0) {
      
      mActiveDeadKey = aVirtualKey;
      mDeadKeyShiftState = shiftState;
      return UniCharsAndModifiers();
    }

    
    
    PRInt32 activeDeadKeyIndex = GetKeyIndex(mActiveDeadKey);
    UniCharsAndModifiers result =
      mVirtualKeys[activeDeadKeyIndex].GetUniChars(mDeadKeyShiftState);
    result += mVirtualKeys[virtualKeyIndex].GetUniChars(shiftState);
    DeactivateDeadKeyState();
    return result;
  }

  UniCharsAndModifiers baseChars =
    mVirtualKeys[virtualKeyIndex].GetUniChars(shiftState);
  if (mActiveDeadKey < 0) {
    
    return baseChars;
  }

  
  
  PRInt32 activeDeadKeyIndex = GetKeyIndex(mActiveDeadKey);
  PRUnichar compositeChar = (baseChars.mLength == 1 && baseChars.mChars[0]) ?
    mVirtualKeys[activeDeadKeyIndex].GetCompositeChar(mDeadKeyShiftState,
                                                      baseChars.mChars[0]) : 0;
  if (compositeChar) {
    
    
    UniCharsAndModifiers result;
    result.Append(compositeChar, baseChars.mModifiers[0]);
    DeactivateDeadKeyState();
    return result;
  }

  
  
  UniCharsAndModifiers result =
    mVirtualKeys[activeDeadKeyIndex].GetUniChars(mDeadKeyShiftState);
  result += baseChars;
  DeactivateDeadKeyState();

  return result;
}

UniCharsAndModifiers
KeyboardLayout::GetUniCharsAndModifiers(
                  PRUint8 aVirtualKey,
                  const ModifierKeyState& aModKeyState) const
{
  UniCharsAndModifiers result;
  PRInt32 key = GetKeyIndex(aVirtualKey);
  if (key < 0) {
    return result;
  }
  return mVirtualKeys[key].
    GetUniChars(VirtualKey::ModifiersToShiftState(aModKeyState.GetModifiers()));
}

void
KeyboardLayout::LoadLayout(HKL aLayout, bool aLoadLater)
{
  if (aLoadLater) {
    mPendingKeyboardLayout = aLayout;
    return;
  }

  mPendingKeyboardLayout = 0;

  if (mKeyboardLayout == aLayout) {
    return;
  }

  mKeyboardLayout = aLayout;

  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));

  BYTE originalKbdState[256];
  
  PRUint16 shiftStatesWithDeadKeys = 0;
  
  
  PRUint16 shiftStatesWithBaseChars = 0;

  mActiveDeadKey = -1;

  ReleaseDeadKeyTables();

  ::GetKeyboardState(originalKbdState);

  
  

  for (VirtualKey::ShiftState shiftState = 0; shiftState < 16; shiftState++) {
    VirtualKey::FillKbdState(kbdState, shiftState);
    for (PRUint32 virtualKey = 0; virtualKey < 256; virtualKey++) {
      PRInt32 vki = GetKeyIndex(virtualKey);
      if (vki < 0) {
        continue;
      }
      NS_ASSERTION(PRUint32(vki) < ArrayLength(mVirtualKeys), "invalid index");
      PRUnichar uniChars[5];
      PRInt32 ret =
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

    for (PRUint32 virtualKey = 0; virtualKey < 256; virtualKey++) {
      PRInt32 vki = GetKeyIndex(virtualKey);
      if (vki >= 0 && mVirtualKeys[vki].IsDeadKey(shiftState)) {
        DeadKeyEntry deadKeyArray[256];
        PRInt32 n = GetDeadKeyCombinations(virtualKey, kbdState,
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

inline PRInt32
KeyboardLayout::GetKeyIndex(PRUint8 aVirtualKey)
{





























  static const PRInt8 xlat[256] =
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
    58, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 59, 60, 61, 62, 63,   
    -1, 64, 65, 66, 67, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
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
                                PRUint32 aEntries)
{
  DeadKeyTableListEntry* next = mDeadKeyTableListHead;

  const size_t bytes = offsetof(DeadKeyTableListEntry, data) +
    DeadKeyTable::SizeInBytes(aEntries);
  PRUint8* p = new PRUint8[bytes];

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
    PRUint8* p = reinterpret_cast<PRUint8*>(mDeadKeyTableListHead);
    mDeadKeyTableListHead = mDeadKeyTableListHead->next;

    delete [] p;
  }
}

bool
KeyboardLayout::EnsureDeadKeyActive(bool aIsActive,
                                    PRUint8 aDeadKey,
                                    const PBYTE aDeadKeyKbdState)
{
  PRInt32 ret;
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
                                PRUint32 aEntries)
{
  for (PRUint32 index = 0; index < aEntries; index++) {
    if (aDeadKeyArray[index].BaseChar == aBaseChar) {
      return false;
    }
  }

  aDeadKeyArray[aEntries].BaseChar = aBaseChar;
  aDeadKeyArray[aEntries].CompositeChar = aCompositeChar;

  return true;
}

PRUint32
KeyboardLayout::GetDeadKeyCombinations(PRUint8 aDeadKey,
                                       const PBYTE aDeadKeyKbdState,
                                       PRUint16 aShiftStatesWithBaseChars,
                                       DeadKeyEntry* aDeadKeyArray,
                                       PRUint32 aMaxEntries)
{
  bool deadKeyActive = false;
  PRUint32 entries = 0;
  BYTE kbdState[256];
  memset(kbdState, 0, sizeof(kbdState));

  for (PRUint32 shiftState = 0; shiftState < 16; shiftState++) {
    if (!(aShiftStatesWithBaseChars & (1 << shiftState))) {
      continue;
    }

    VirtualKey::FillKbdState(kbdState, shiftState);

    for (PRUint32 virtualKey = 0; virtualKey < 256; virtualKey++) {
      PRInt32 vki = GetKeyIndex(virtualKey);
      
      
      if (vki >= 0 &&
          mVirtualKeys[vki].GetNativeUniChars(shiftState).mLength == 1) {
        
        
        if (!deadKeyActive) {
          deadKeyActive = EnsureDeadKeyActive(true, aDeadKey,
                                              aDeadKeyKbdState);
        }

        
        
        
        PRUnichar compositeChars[5];
        PRInt32 ret =
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
               CompareDeadKeyEntries, nsnull);
  return entries;
}

PRUint32
KeyboardLayout::ConvertNativeKeyCodeToDOMKeyCode(UINT aNativeKeyCode) const
{
  
  if ((aNativeKeyCode >= 0x30 && aNativeKeyCode <= 0x39) ||
      (aNativeKeyCode >= 0x41 && aNativeKeyCode <= 0x5A) ||
      (aNativeKeyCode >= 0x60 && aNativeKeyCode <= 0x87)) {
    return static_cast<PRUint32>(aNativeKeyCode);
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
      return PRUint32(aNativeKeyCode);

    case VK_HELP:
      return NS_VK_HELP;

    
    
    case VK_LWIN:
    case VK_RWIN:
      return NS_VK_WIN;

    
    case VK_BROWSER_BACK:
    case VK_BROWSER_FORWARD:
    case VK_BROWSER_REFRESH:
    case VK_BROWSER_STOP:
    case VK_BROWSER_SEARCH:
    case VK_BROWSER_FAVORITES:
    case VK_BROWSER_HOME:
    case VK_VOLUME_MUTE:
    case VK_VOLUME_DOWN:
    case VK_VOLUME_UP:
    case VK_MEDIA_NEXT_TRACK:
    case VK_MEDIA_STOP:
    case VK_MEDIA_PLAY_PAUSE:
    case VK_LAUNCH_MAIL:
    case VK_LAUNCH_MEDIA_SELECT:
    case VK_LAUNCH_APP1:
    case VK_LAUNCH_APP2:
    case VK_ATTN: 
    case VK_CRSEL: 
    case VK_EXSEL: 
    case VK_EREOF: 
    case VK_PLAY:
    case VK_ZOOM:
    case VK_PA1: 
    case VK_OEM_CLEAR:
      return 0;

    
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
    case 0xE1: 
    case VK_OEM_102:
    case 0xE3: 
    case 0xE4: 
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

    
    case VK_PROCESSKEY:
      return 0;
    
    
    case VK_PACKET:
      return 0;
  }
  NS_WARNING("Unknown key code comes, please check latest MSDN document,"
             " there may be some new keycodes we have not known.");
  return 0;
}





PRUnichar
DeadKeyTable::GetCompositeChar(PRUnichar aBaseChar) const
{
  
  

  for (PRUint32 index = 0; index < mEntries; index++) {
    if (mTable[index].BaseChar == aBaseChar) {
      return mTable[index].CompositeChar;
    }
    if (mTable[index].BaseChar > aBaseChar) {
      break;
    }
  }

  return 0;
}

} 
} 

