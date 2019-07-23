




































#ifndef nsKeyboardLayout_h__
#define nsKeyboardLayout_h__

#include "nscore.h"
#include <windows.h>


#define VK_OEM_1                0xBA   // ';:' for US
#define VK_OEM_PLUS             0xBB   // '+' any country
#define VK_OEM_MINUS            0xBD   // '-' any country




















enum eKeyShiftFlags
{
  eShift    = 0x01,
  eCtrl     = 0x02,
  eAlt      = 0x04,
  eCapsLock = 0x08
};

#ifndef WINCE
struct DeadKeyEntry;
class DeadKeyTable;


class VirtualKey
{
  union KeyShiftState
  {
    struct
    {
      PRUint16 Chars [4];
    } Normal;
    struct
    {
      const DeadKeyTable* Table;
      PRUint16 DeadChar;
    } DeadKey;
  };

  KeyShiftState mShiftStates [16];
  PRUint16 mIsDeadKey;

  void SetDeadKey (PRUint8 aShiftState, PRBool aIsDeadKey)
  {
    if (aIsDeadKey)
      mIsDeadKey |= 1 << aShiftState;
    else
      mIsDeadKey &= ~(1 << aShiftState);
  }

public:
  PRBool IsDeadKey (PRUint8 aShiftState) const
  {
    return (mIsDeadKey & (1 << aShiftState)) != 0;
  }

  void AttachDeadKeyTable (PRUint8 aShiftState, const DeadKeyTable* aDeadKeyTable)
  {
    mShiftStates [aShiftState].DeadKey.Table = aDeadKeyTable;
  }

  void SetNormalChars (PRUint8 aShiftState, const PRUint16* aChars, PRUint32 aNumOfChars);
  void SetDeadChar (PRUint8 aShiftState, PRUint16 aDeadChar);
  const DeadKeyTable* MatchingDeadKeyTable (const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries) const;
  inline PRUint16 GetCompositeChar (PRUint8 aShiftState, PRUint16 aBaseChar) const;
  PRUint32 GetNativeUniChars (PRUint8 aShiftState, PRUint16* aUniChars = nsnull) const;
  PRUint32 GetUniChars (PRUint8 aShiftState, PRUint16* aUniChars, PRUint8* aFinalShiftState) const;
};
#endif


class KeyboardLayout
{
#ifndef WINCE
  struct DeadKeyTableListEntry
  {
    DeadKeyTableListEntry* next;
    PRUint8 data [1];
  };

  #define NUM_OF_KEYS   50

  HKL mKeyboardLayout;

  VirtualKey mVirtualKeys [NUM_OF_KEYS];
  DeadKeyTableListEntry* mDeadKeyTableListHead;
  PRInt32 mActiveDeadKey;                 
  PRUint8 mDeadKeyShiftState;
  PRInt32 mLastVirtualKeyIndex;
  PRUint8 mLastShiftState;
  PRUint16 mChars [5];                    
  PRUint8 mShiftStates [5];
  PRUint8 mNumOfChars;

  static PRUint8 GetShiftState (const PBYTE aKbdState);
  static void SetShiftState (PBYTE aKbdState, PRUint8 aShiftState);
  static inline PRInt32 GetKeyIndex (PRUint8 aVirtualKey);
  static int PR_CALLBACK CompareDeadKeyEntries (const void* aArg1, const void* aArg2, void* aData);
  static PRBool AddDeadKeyEntry (PRUint16 aBaseChar, PRUint16 aCompositeChar, DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries);
  PRBool EnsureDeadKeyActive (PRBool aIsActive, PRUint8 aDeadKey, const PBYTE aDeadKeyKbdState);
  PRUint32 GetDeadKeyCombinations (PRUint8 aDeadKey, const PBYTE aDeadKeyKbdState, PRUint16 aShiftStatesWithBaseChars,
                                   DeadKeyEntry* aDeadKeyArray, PRUint32 aMaxEntries);
  void DeactivateDeadKeyState ();
  const DeadKeyTable* AddDeadKeyTable (const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries);
  void ReleaseDeadKeyTables ();
#endif

public:
  KeyboardLayout ();
  ~KeyboardLayout ();

  static PRBool IsPrintableCharKey (PRUint8 aVirtualKey);
  static PRBool IsNumpadKey (PRUint8 aVirtualKey);

  PRBool IsDeadKey () const
  {
#ifndef WINCE
    return (mLastVirtualKeyIndex >= 0) ? mVirtualKeys [mLastVirtualKeyIndex].IsDeadKey (mLastShiftState) : PR_FALSE;
#else
    return PR_FALSE;
#endif
  }

  void LoadLayout ();
  void OnKeyDown (PRUint8 aVirtualKey);
  PRUint32 GetUniChars (PRUint16* aUniChars, PRUint8* aShiftStates, PRUint32 aMaxChars) const;
};

#endif
