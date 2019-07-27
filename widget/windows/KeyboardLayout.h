




#ifndef KeyboardLayout_h__
#define KeyboardLayout_h__

#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsWindowBase.h"
#include "nsWindowDefs.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include <windows.h>

#define NS_NUM_OF_KEYS          70

#define VK_OEM_1                0xBA   // ';:' for US
#define VK_OEM_PLUS             0xBB   // '+' any country
#define VK_OEM_COMMA            0xBC
#define VK_OEM_MINUS            0xBD   // '-' any country
#define VK_OEM_PERIOD           0xBE
#define VK_OEM_2                0xBF
#define VK_OEM_3                0xC0

#define VK_ABNT_C1              0xC1

#define VK_ABNT_C2              0xC2
#define VK_OEM_4                0xDB
#define VK_OEM_5                0xDC
#define VK_OEM_6                0xDD
#define VK_OEM_7                0xDE
#define VK_OEM_8                0xDF
#define VK_OEM_102              0xE2
#define VK_OEM_CLEAR            0xFE

class nsIIdleServiceInternal;

namespace mozilla {
namespace widget {

static const uint32_t sModifierKeyMap[][3] = {
  { nsIWidget::CAPS_LOCK, VK_CAPITAL, 0 },
  { nsIWidget::NUM_LOCK,  VK_NUMLOCK, 0 },
  { nsIWidget::SHIFT_L,   VK_SHIFT,   VK_LSHIFT },
  { nsIWidget::SHIFT_R,   VK_SHIFT,   VK_RSHIFT },
  { nsIWidget::CTRL_L,    VK_CONTROL, VK_LCONTROL },
  { nsIWidget::CTRL_R,    VK_CONTROL, VK_RCONTROL },
  { nsIWidget::ALT_L,     VK_MENU,    VK_LMENU },
  { nsIWidget::ALT_R,     VK_MENU,    VK_RMENU }
};

class KeyboardLayout;

class ModifierKeyState
{
public:
  ModifierKeyState();
  ModifierKeyState(bool aIsShiftDown, bool aIsControlDown, bool aIsAltDown);
  ModifierKeyState(Modifiers aModifiers);

  void Update();

  void Unset(Modifiers aRemovingModifiers);
  void Set(Modifiers aAddingModifiers);

  void InitInputEvent(WidgetInputEvent& aInputEvent) const;

  bool IsShift() const;
  bool IsControl() const;
  bool IsAlt() const;
  bool IsAltGr() const;
  bool IsWin() const;

  bool IsCapsLocked() const;
  bool IsNumLocked() const;
  bool IsScrollLocked() const;

  MOZ_ALWAYS_INLINE Modifiers GetModifiers() const
  {
    return mModifiers;
  }

private:
  Modifiers mModifiers;

  MOZ_ALWAYS_INLINE void EnsureAltGr();

  void InitMouseEvent(WidgetInputEvent& aMouseEvent) const;
};

struct UniCharsAndModifiers
{
  
  char16_t mChars[5];
  Modifiers mModifiers[5];
  uint32_t  mLength;

  UniCharsAndModifiers() : mLength(0) {}
  UniCharsAndModifiers operator+(const UniCharsAndModifiers& aOther) const;
  UniCharsAndModifiers& operator+=(const UniCharsAndModifiers& aOther);

  


  void Append(char16_t aUniChar, Modifiers aModifiers);
  void Clear() { mLength = 0; }
  bool IsEmpty() const { return !mLength; }

  void FillModifiers(Modifiers aModifiers);

  bool UniCharsEqual(const UniCharsAndModifiers& aOther) const;
  bool UniCharsCaseInsensitiveEqual(const UniCharsAndModifiers& aOther) const;

  nsString ToString() const { return nsString(mChars, mLength); }
};

struct DeadKeyEntry;
class DeadKeyTable;


class VirtualKey
{
public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  enum ShiftStateFlag
  {
    STATE_SHIFT    = 0x01,
    STATE_CONTROL  = 0x02,
    STATE_ALT      = 0x04,
    STATE_CAPSLOCK = 0x08
  };

  typedef uint8_t ShiftState;

  static ShiftState ModifiersToShiftState(Modifiers aModifiers);
  static Modifiers ShiftStateToModifiers(ShiftState aShiftState);

private:
  union KeyShiftState
  {
    struct
    {
      char16_t Chars[4];
    } Normal;
    struct
    {
      const DeadKeyTable* Table;
      char16_t DeadChar;
    } DeadKey;
  };

  KeyShiftState mShiftStates[16];
  uint16_t mIsDeadKey;

  void SetDeadKey(ShiftState aShiftState, bool aIsDeadKey)
  {
    if (aIsDeadKey) {
      mIsDeadKey |= 1 << aShiftState;
    } else {
      mIsDeadKey &= ~(1 << aShiftState);
    }
  }

public:
  static void FillKbdState(PBYTE aKbdState, const ShiftState aShiftState);

  bool IsDeadKey(ShiftState aShiftState) const
  {
    return (mIsDeadKey & (1 << aShiftState)) != 0;
  }

  void AttachDeadKeyTable(ShiftState aShiftState,
                          const DeadKeyTable* aDeadKeyTable)
  {
    mShiftStates[aShiftState].DeadKey.Table = aDeadKeyTable;
  }

  void SetNormalChars(ShiftState aShiftState, const char16_t* aChars,
                      uint32_t aNumOfChars);
  void SetDeadChar(ShiftState aShiftState, char16_t aDeadChar);
  const DeadKeyTable* MatchingDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                           uint32_t aEntries) const;
  inline char16_t GetCompositeChar(ShiftState aShiftState,
                                    char16_t aBaseChar) const;
  UniCharsAndModifiers GetNativeUniChars(ShiftState aShiftState) const;
  UniCharsAndModifiers GetUniChars(ShiftState aShiftState) const;
};

class MOZ_STACK_CLASS NativeKey
{
  friend class KeyboardLayout;

public:
  struct FakeCharMsg
  {
    UINT mCharCode;
    UINT mScanCode;
    bool mIsDeadKey;
    bool mConsumed;

    FakeCharMsg() :
      mCharCode(0), mScanCode(0), mIsDeadKey(false), mConsumed(false)
    {
    }

    MSG GetCharMsg(HWND aWnd) const
    {
      MSG msg;
      msg.hwnd = aWnd;
      msg.message = mIsDeadKey ? WM_DEADCHAR : WM_CHAR;
      msg.wParam = static_cast<WPARAM>(mCharCode);
      msg.lParam = static_cast<LPARAM>(mScanCode << 16);
      msg.time = 0;
      msg.pt.x = msg.pt.y = 0;
      return msg;
    }
  };

  NativeKey(nsWindowBase* aWidget,
            const MSG& aMessage,
            const ModifierKeyState& aModKeyState,
            nsTArray<FakeCharMsg>* aFakeCharMsgs = nullptr);

  





  bool HandleKeyDownMessage(bool* aEventDispatched = nullptr) const;

  




  bool HandleCharMessage(const MSG& aCharMsg,
                         bool* aEventDispatched = nullptr) const;

  



  bool HandleKeyUpMessage(bool* aEventDispatched = nullptr) const;

  



  bool HandleAppCommandMessage() const;

private:
  nsRefPtr<nsWindowBase> mWidget;
  HKL mKeyboardLayout;
  MSG mMsg;

  uint32_t mDOMKeyCode;
  KeyNameIndex mKeyNameIndex;
  CodeNameIndex mCodeNameIndex;

  ModifierKeyState mModKeyState;

  
  uint8_t mVirtualKeyCode;
  
  
  
  uint8_t mOriginalVirtualKeyCode;

  
  
  
  UniCharsAndModifiers mCommittedCharsAndModifiers;

  WORD    mScanCode;
  bool    mIsExtended;
  bool    mIsDeadKey;
  
  
  
  
  bool    mIsPrintableKey;

  nsTArray<FakeCharMsg>* mFakeCharMsgs;

  
  
  
  
  static uint8_t sDispatchedKeyOfAppCommand;

  NativeKey()
  {
    MOZ_CRASH("The default constructor of NativeKey isn't available");
  }

  void InitWithAppCommand();

  


  bool IsRepeat() const
  {
    switch (mMsg.message) {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      case WM_CHAR:
      case WM_SYSCHAR:
      case WM_DEADCHAR:
      case WM_SYSDEADCHAR:
        return ((mMsg.lParam & (1 << 30)) != 0);
      case WM_APPCOMMAND:
        if (mVirtualKeyCode) {
          
          
          BYTE kbdState[256];
          memset(kbdState, 0, sizeof(kbdState));
          ::GetKeyboardState(kbdState);
          return !!kbdState[mVirtualKeyCode];
        }
        
        
        
        
        
        return false;
      default:
        return false;
    }
  }

  UINT GetScanCodeWithExtendedFlag() const;

  
  uint32_t GetKeyLocation() const;

  




  bool IsIMEDoingKakuteiUndo() const;

  bool IsKeyDownMessage() const
  {
    return (mMsg.message == WM_KEYDOWN || mMsg.message == WM_SYSKEYDOWN);
  }
  bool IsKeyUpMessage() const
  {
    return (mMsg.message == WM_KEYUP || mMsg.message == WM_SYSKEYUP);
  }
  bool IsPrintableCharMessage(const MSG& aMSG) const
  {
    return IsPrintableCharMessage(aMSG.message);
  }
  bool IsPrintableCharMessage(UINT aMessage) const
  {
    return (aMessage == WM_CHAR || aMessage == WM_SYSCHAR);
  }
  bool IsCharMessage(const MSG& aMSG) const
  {
    return IsCharMessage(aMSG.message);
  }
  bool IsCharMessage(UINT aMessage) const
  {
    return (IsPrintableCharMessage(aMessage) || IsDeadCharMessage(aMessage));
  }
  bool IsDeadCharMessage(const MSG& aMSG) const
  {
    return IsDeadCharMessage(aMSG.message);
  }
  bool IsDeadCharMessage(UINT aMessage) const
  {
    return (aMessage == WM_DEADCHAR || aMessage == WM_SYSDEADCHAR);
  }
  bool IsSysCharMessage(const MSG& aMSG) const
  {
    return IsSysCharMessage(aMSG.message);
  }
  bool IsSysCharMessage(UINT aMessage) const
  {
    return (aMessage == WM_SYSCHAR || aMessage == WM_SYSDEADCHAR);
  }
  bool MayBeSameCharMessage(const MSG& aCharMsg1, const MSG& aCharMsg2) const;
  bool IsFollowedByDeadCharMessage() const;

  







  bool GetFollowingCharMessage(MSG& aCharMsg) const;

  


  bool CanComputeVirtualKeyCodeFromScanCode() const;

  


  uint8_t ComputeVirtualKeyCodeFromScanCode() const;

  


  uint8_t ComputeVirtualKeyCodeFromScanCodeEx() const;

  


  uint16_t ComputeScanCodeExFromVirtualKeyCode(UINT aVirtualKeyCode) const;

  


  char16_t ComputeUnicharFromScanCode() const;

  


  void InitKeyEvent(WidgetKeyboardEvent& aKeyEvent,
                    const ModifierKeyState& aModKeyState) const;
  void InitKeyEvent(WidgetKeyboardEvent& aKeyEvent) const;

  



  bool DispatchCommandEvent(uint32_t aEventCommand) const;

  



  bool DispatchKeyEvent(WidgetKeyboardEvent& aKeyEvent,
                        const MSG* aMsgSentToPlugin = nullptr) const;

  



  bool DispatchKeyPressEventsWithKeyboardLayout() const;

  





  bool DispatchPluginEventsAndDiscardsCharMessages() const;

  




  bool DispatchKeyPressEventForFollowingCharMessage(const MSG& aCharMsg) const;

  






  bool NeedsToHandleWithoutFollowingCharMessages() const;
};

class KeyboardLayout
{
  friend class NativeKey;

private:
  KeyboardLayout();
  ~KeyboardLayout();

  static KeyboardLayout* sInstance;
  static nsIIdleServiceInternal* sIdleService;

  struct DeadKeyTableListEntry
  {
    DeadKeyTableListEntry* next;
    uint8_t data[1];
  };

  HKL mKeyboardLayout;

  VirtualKey mVirtualKeys[NS_NUM_OF_KEYS];
  DeadKeyTableListEntry* mDeadKeyTableListHead;
  int32_t mActiveDeadKey;                 
  VirtualKey::ShiftState mDeadKeyShiftState;

  bool mIsOverridden : 1;
  bool mIsPendingToRestoreKeyboardLayout : 1;

  static inline int32_t GetKeyIndex(uint8_t aVirtualKey);
  static int CompareDeadKeyEntries(const void* aArg1, const void* aArg2,
                                   void* aData);
  static bool AddDeadKeyEntry(char16_t aBaseChar, char16_t aCompositeChar,
                                DeadKeyEntry* aDeadKeyArray, uint32_t aEntries);
  bool EnsureDeadKeyActive(bool aIsActive, uint8_t aDeadKey,
                             const PBYTE aDeadKeyKbdState);
  uint32_t GetDeadKeyCombinations(uint8_t aDeadKey,
                                  const PBYTE aDeadKeyKbdState,
                                  uint16_t aShiftStatesWithBaseChars,
                                  DeadKeyEntry* aDeadKeyArray,
                                  uint32_t aMaxEntries);
  void DeactivateDeadKeyState();
  const DeadKeyTable* AddDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                      uint32_t aEntries);
  void ReleaseDeadKeyTables();

  



  void LoadLayout(HKL aLayout);

  





  void InitNativeKey(NativeKey& aNativeKey,
                     const ModifierKeyState& aModKeyState);

public:
  static KeyboardLayout* GetInstance();
  static void Shutdown();
  static void NotifyIdleServiceOfUserActivity();

  static bool IsPrintableCharKey(uint8_t aVirtualKey);

  



  bool IsDeadKey(uint8_t aVirtualKey,
                 const ModifierKeyState& aModKeyState) const;

  



  UniCharsAndModifiers GetUniCharsAndModifiers(
                         uint8_t aVirtualKey,
                         const ModifierKeyState& aModKeyState) const;

  





  void OnLayoutChange(HKL aKeyboardLayout)
  {
    MOZ_ASSERT(!mIsOverridden);
    LoadLayout(aKeyboardLayout);
  }

  


  void OverrideLayout(HKL aLayout)
  {
    mIsOverridden = true;
    LoadLayout(aLayout);
  }

  


  void RestoreLayout()
  {
    mIsOverridden = false;
    mIsPendingToRestoreKeyboardLayout = true;
  }

  uint32_t ConvertNativeKeyCodeToDOMKeyCode(UINT aNativeKeyCode) const;

  



  KeyNameIndex ConvertNativeKeyCodeToKeyNameIndex(uint8_t aVirtualKey) const;

  




  static CodeNameIndex ConvertScanCodeToCodeNameIndex(UINT aScanCode);

  HKL GetLayout() const
  {
    return mIsPendingToRestoreKeyboardLayout ? ::GetKeyboardLayout(0) :
                                               mKeyboardLayout;
  }

  


  WORD ComputeScanCodeForVirtualKeyCode(uint8_t aVirtualKeyCode) const;

  


  nsresult SynthesizeNativeKeyEvent(nsWindowBase* aWidget,
                                    int32_t aNativeKeyboardLayout,
                                    int32_t aNativeKeyCode,
                                    uint32_t aModifierFlags,
                                    const nsAString& aCharacters,
                                    const nsAString& aUnmodifiedCharacters);
};

class RedirectedKeyDownMessageManager
{
public:
  









  class MOZ_STACK_CLASS AutoFlusher final
  {
  public:
    AutoFlusher(nsWindowBase* aWidget, const MSG &aMsg) :
      mCancel(!RedirectedKeyDownMessageManager::IsRedirectedMessage(aMsg)),
      mWidget(aWidget), mMsg(aMsg)
    {
    }

    ~AutoFlusher()
    {
      if (mCancel) {
        return;
      }
      
      if (!mWidget->Destroyed()) {
        RedirectedKeyDownMessageManager::RemoveNextCharMessage(mMsg.hwnd);
      }
      
      RedirectedKeyDownMessageManager::Forget();
    }

    void Cancel() { mCancel = true; }

  private:
    bool mCancel;
    nsRefPtr<nsWindowBase> mWidget;
    const MSG &mMsg;
  };

  static void WillRedirect(const MSG& aMsg, bool aDefualtPrevented)
  {
    sRedirectedKeyDownMsg = aMsg;
    sDefaultPreventedOfRedirectedMsg = aDefualtPrevented;
  }

  static void Forget()
  {
    sRedirectedKeyDownMsg.message = WM_NULL;
  }

  static void PreventDefault() { sDefaultPreventedOfRedirectedMsg = true; }
  static bool DefaultPrevented() { return sDefaultPreventedOfRedirectedMsg; }

  static bool IsRedirectedMessage(const MSG& aMsg);

  








  static void RemoveNextCharMessage(HWND aWnd);

private:
  
  
  
  static MSG sRedirectedKeyDownMsg;
  static bool sDefaultPreventedOfRedirectedMsg;
};

} 
} 

#endif
