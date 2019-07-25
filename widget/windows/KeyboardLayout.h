




#ifndef KeyboardLayout_h__
#define KeyboardLayout_h__

#include "nscore.h"
#include "nsEvent.h"
#include <windows.h>

#define NS_NUM_OF_KEYS          54

#define VK_OEM_1                0xBA   // ';:' for US
#define VK_OEM_PLUS             0xBB   // '+' any country
#define VK_OEM_COMMA            0xBC
#define VK_OEM_MINUS            0xBD   // '-' any country
#define VK_OEM_PERIOD           0xBE
#define VK_OEM_2                0xBF
#define VK_OEM_3                0xC0
#define VK_OEM_4                0xDB
#define VK_OEM_5                0xDC
#define VK_OEM_6                0xDD
#define VK_OEM_7                0xDE
#define VK_OEM_8                0xDF
#define VK_OEM_102              0xE2
#define VK_OEM_CLEAR            0xFE

class nsWindow;
struct nsModifierKeyState;

namespace mozilla {
namespace widget {

class KeyboardLayout;

class ModifierKeyState {
public:
  ModifierKeyState()
  {
    Update();
  }

  ModifierKeyState(bool aIsShiftDown, bool aIsControlDown, bool aIsAltDown)
  {
    Update();
    Unset(MODIFIER_SHIFT | MODIFIER_CONTROL | MODIFIER_ALT | MODIFIER_ALTGRAPH);
    Modifiers modifiers = 0;
    if (aIsShiftDown) {
      modifiers |= MODIFIER_SHIFT;
    }
    if (aIsControlDown) {
      modifiers |= MODIFIER_CONTROL;
    }
    if (aIsAltDown) {
      modifiers |= MODIFIER_ALT;
    }
    if (modifiers) {
      Set(modifiers);
    }
  }

  void Update();

  void Unset(Modifiers aRemovingModifiers)
  {
    mModifiers &= ~aRemovingModifiers;
    
    
    
  }

  void Set(Modifiers aAddingModifiers)
  {
    mModifiers |= aAddingModifiers;
    EnsureAltGr();
  }

  void InitInputEvent(nsInputEvent& aInputEvent) const;

  bool IsShift() const { return (mModifiers & MODIFIER_SHIFT) != 0; }
  bool IsControl() const { return (mModifiers & MODIFIER_CONTROL) != 0; }
  bool IsAlt() const { return (mModifiers & MODIFIER_ALT) != 0; }
  bool IsAltGr() const { return IsControl() && IsAlt(); }
  bool IsWin() const { return (mModifiers & MODIFIER_WIN) != 0; }

  bool IsCapsLocked() const { return (mModifiers & MODIFIER_CAPSLOCK) != 0; }
  bool IsNumLocked() const { return (mModifiers & MODIFIER_NUMLOCK) != 0; }
  bool IsScrollLocked() const { return (mModifiers & MODIFIER_SCROLL) != 0; }

  Modifiers GetModifiers() const { return mModifiers; }

private:
  Modifiers mModifiers;

  void EnsureAltGr()
  {
    
    
    
    
    if (IsAltGr()) {
      mModifiers |= MODIFIER_ALTGRAPH;
    }
  }

  void InitMouseEvent(nsInputEvent& aMouseEvent) const;
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

  typedef PRUint8 ShiftState;

  static ShiftState ModifiersToShiftState(Modifiers aModifiers)
  {
    ShiftState state = 0;
    if (aModifiers & MODIFIER_SHIFT) {
      state |= STATE_SHIFT;
    }
    if (aModifiers & MODIFIER_CONTROL) {
      state |= STATE_CONTROL;
    }
    if (aModifiers & MODIFIER_ALT) {
      state |= STATE_ALT;
    }
    if (aModifiers & MODIFIER_CAPSLOCK) {
      state |= STATE_CAPSLOCK;
    }
    return state;
  }

  static Modifiers ShiftStateToModifiers(ShiftState aShiftState)
  {
    Modifiers modifiers = 0;
    if (aShiftState & STATE_SHIFT) {
      modifiers |= MODIFIER_SHIFT;
    }
    if (aShiftState & STATE_CONTROL) {
      modifiers |= MODIFIER_CONTROL;
    }
    if (aShiftState & STATE_ALT) {
      modifiers |= MODIFIER_ALT;
    }
    if (aShiftState & STATE_CAPSLOCK) {
      modifiers |= MODIFIER_CAPSLOCK;
    }
    if ((modifiers & (MODIFIER_ALT | MODIFIER_CONTROL)) ==
           (MODIFIER_ALT | MODIFIER_CONTROL)) {
      modifiers |= MODIFIER_ALTGRAPH;
    }
    return modifiers;
  }

private:
  union KeyShiftState
  {
    struct
    {
      PRUnichar Chars[4];
    } Normal;
    struct
    {
      const DeadKeyTable* Table;
      PRUnichar DeadChar;
    } DeadKey;
  };

  KeyShiftState mShiftStates[16];
  PRUint16 mIsDeadKey;

  void SetDeadKey(ShiftState aShiftState, bool aIsDeadKey)
  {
    if (aIsDeadKey) {
      mIsDeadKey |= 1 << aShiftState;
    } else {
      mIsDeadKey &= ~(1 << aShiftState);
    }
  }

public:
  bool IsDeadKey(ShiftState aShiftState) const
  {
    return (mIsDeadKey & (1 << aShiftState)) != 0;
  }

  void AttachDeadKeyTable(ShiftState aShiftState,
                          const DeadKeyTable* aDeadKeyTable)
  {
    mShiftStates[aShiftState].DeadKey.Table = aDeadKeyTable;
  }

  void SetNormalChars(ShiftState aShiftState, const PRUnichar* aChars,
                      PRUint32 aNumOfChars);
  void SetDeadChar(ShiftState aShiftState, PRUnichar aDeadChar);
  const DeadKeyTable* MatchingDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                           PRUint32 aEntries) const;
  inline PRUnichar GetCompositeChar(ShiftState aShiftState,
                                    PRUnichar aBaseChar) const;
  PRUint32 GetNativeUniChars(ShiftState aShiftState,
                             PRUnichar* aUniChars = nsnull) const;
  PRUint32 GetUniChars(ShiftState aShiftState, PRUnichar* aUniChars,
                       Modifiers* aFinalModifiers) const;
};

class NativeKey {
public:
  NativeKey() :
    mDOMKeyCode(0), mVirtualKeyCode(0), mOriginalVirtualKeyCode(0),
    mScanCode(0), mIsExtended(false)
  {
  }

  NativeKey(const KeyboardLayout& aKeyboardLayout,
            nsWindow* aWindow,
            const MSG& aKeyOrCharMessage);

  PRUint32 GetDOMKeyCode() const { return mDOMKeyCode; }

  
  PRUint32 GetKeyLocation() const;
  WORD GetScanCode() const { return mScanCode; }
  PRUint8 GetVirtualKeyCode() const { return mVirtualKeyCode; }
  PRUint8 GetOriginalVirtualKeyCode() const { return mOriginalVirtualKeyCode; }

private:
  PRUint32 mDOMKeyCode;
  
  PRUint8 mVirtualKeyCode;
  
  
  
  PRUint8 mOriginalVirtualKeyCode;
  WORD    mScanCode;
  bool    mIsExtended;

  UINT GetScanCodeWithExtendedFlag() const;
};

class KeyboardLayout
{
  struct DeadKeyTableListEntry
  {
    DeadKeyTableListEntry* next;
    PRUint8 data[1];
  };

  HKL mKeyboardLayout;

  VirtualKey mVirtualKeys[NS_NUM_OF_KEYS];
  DeadKeyTableListEntry* mDeadKeyTableListHead;
  PRInt32 mActiveDeadKey;                 
  VirtualKey::ShiftState mDeadKeyShiftState;
  PRUnichar mChars[5];                    
  Modifiers mModifiersOfChars[5];
  PRUint8 mNumOfChars;

  static VirtualKey::ShiftState GetShiftState(const PBYTE aKbdState);
  static void SetShiftState(PBYTE aKbdState,
                            VirtualKey::ShiftState aShiftState);
  static inline PRInt32 GetKeyIndex(PRUint8 aVirtualKey);
  static int CompareDeadKeyEntries(const void* aArg1, const void* aArg2,
                                   void* aData);
  static bool AddDeadKeyEntry(PRUnichar aBaseChar, PRUnichar aCompositeChar,
                                DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries);
  bool EnsureDeadKeyActive(bool aIsActive, PRUint8 aDeadKey,
                             const PBYTE aDeadKeyKbdState);
  PRUint32 GetDeadKeyCombinations(PRUint8 aDeadKey,
                                  const PBYTE aDeadKeyKbdState,
                                  PRUint16 aShiftStatesWithBaseChars,
                                  DeadKeyEntry* aDeadKeyArray,
                                  PRUint32 aMaxEntries);
  void DeactivateDeadKeyState();
  const DeadKeyTable* AddDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                      PRUint32 aEntries);
  void ReleaseDeadKeyTables();

public:
  KeyboardLayout();
  ~KeyboardLayout();

  static bool IsPrintableCharKey(PRUint8 aVirtualKey);
  static bool IsNumpadKey(PRUint8 aVirtualKey);

  


  bool IsDeadKey(PRUint8 aVirtualKey,
                 const ModifierKeyState& aModKeyState) const;

  void LoadLayout(HKL aLayout);
  void OnKeyDown(PRUint8 aVirtualKey);
  PRUint32 GetUniChars(PRUnichar* aUniChars, Modifiers* aModifiersOfUniChars,
                       PRUint32 aMaxChars) const;
  PRUint32 GetUniCharsWithShiftState(PRUint8 aVirtualKey, Modifiers aModifiers,
                                     PRUnichar* aUniChars,
                                     PRUint32 aMaxChars) const;
  PRUint32 ConvertNativeKeyCodeToDOMKeyCode(UINT aNativeKeyCode) const;

  HKL GetLayout() const { return mKeyboardLayout; }
};

} 
} 

#endif
