




































#ifndef KeyboardLayout_h__
#define KeyboardLayout_h__

#include "nscore.h"
#include <windows.h>

#define NS_NUM_OF_KEYS          50

#define VK_OEM_1                0xBA   // ';:' for US
#define VK_OEM_PLUS             0xBB   // '+' any country
#define VK_OEM_MINUS            0xBD   // '-' any country

namespace mozilla {
namespace widget {


















enum eKeyShiftFlags
{
  eShift    = 0x01,
  eCtrl     = 0x02,
  eAlt      = 0x04,
  eCapsLock = 0x08
};

struct DeadKeyEntry;
class DeadKeyTable;


class VirtualKey
{
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

  void SetDeadKey(PRUint8 aShiftState, bool aIsDeadKey)
  {
    if (aIsDeadKey) {
      mIsDeadKey |= 1 << aShiftState;
    } else {
      mIsDeadKey &= ~(1 << aShiftState);
    }
  }

public:
  bool IsDeadKey(PRUint8 aShiftState) const
  {
    return (mIsDeadKey & (1 << aShiftState)) != 0;
  }

  void AttachDeadKeyTable(PRUint8 aShiftState,
                          const DeadKeyTable* aDeadKeyTable)
  {
    mShiftStates[aShiftState].DeadKey.Table = aDeadKeyTable;
  }

  void SetNormalChars(PRUint8 aShiftState, const PRUnichar* aChars,
                      PRUint32 aNumOfChars);
  void SetDeadChar(PRUint8 aShiftState, PRUnichar aDeadChar);
  const DeadKeyTable* MatchingDeadKeyTable(const DeadKeyEntry* aDeadKeyArray,
                                           PRUint32 aEntries) const;
  inline PRUnichar GetCompositeChar(PRUint8 aShiftState,
                                    PRUnichar aBaseChar) const;
  PRUint32 GetNativeUniChars(PRUint8 aShiftState,
                             PRUnichar* aUniChars = nsnull) const;
  PRUint32 GetUniChars(PRUint8 aShiftState, PRUnichar* aUniChars,
                       PRUint8* aFinalShiftState) const;
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
  PRUint8 mDeadKeyShiftState;
  PRInt32 mLastVirtualKeyIndex;
  PRUint8 mLastShiftState;
  PRUnichar mChars[5];                    
  PRUint8 mShiftStates[5];
  PRUint8 mNumOfChars;

  static PRUint8 GetShiftState(const PBYTE aKbdState);
  static void SetShiftState(PBYTE aKbdState, PRUint8 aShiftState);
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

  bool IsDeadKey() const
  {
    return (mLastVirtualKeyIndex >= 0) ?
      mVirtualKeys[mLastVirtualKeyIndex].IsDeadKey(mLastShiftState) : false;
  }

  void LoadLayout(HKL aLayout);
  void OnKeyDown(PRUint8 aVirtualKey);
  PRUint32 GetUniChars(PRUnichar* aUniChars, PRUint8* aShiftStates,
                       PRUint32 aMaxChars) const;
  PRUint32 GetUniCharsWithShiftState(PRUint8 aVirtualKey, PRUint8 aShiftStates,
                                     PRUnichar* aUniChars,
                                     PRUint32 aMaxChars) const;

  HKL GetLayout() { return mKeyboardLayout; }
};

} 
} 

#endif
