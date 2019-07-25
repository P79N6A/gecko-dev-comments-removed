





































#include "nsMemory.h"
#include "nsKeyboardLayout.h"
#include "nsToolkit.h"
#include "nsQuickSort.h"

#include <winuser.h>

#ifndef WINABLEAPI
#include <winable.h>
#endif

struct DeadKeyEntry
{
  PRUnichar BaseChar;
  PRUnichar CompositeChar;
};


class DeadKeyTable
{
  friend class KeyboardLayout;

  PRUint16 mEntries;
  DeadKeyEntry mTable [1];    
                              

  void Init (const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries)
  {
    mEntries = aEntries;
    memcpy (mTable, aDeadKeyArray, aEntries * sizeof (DeadKeyEntry));
  }

  static PRUint32 SizeInBytes (PRUint32 aEntries)
  {
    return offsetof (DeadKeyTable, mTable) + aEntries * sizeof (DeadKeyEntry);
  }

public:
  PRUint32 Entries () const
  {
    return mEntries;
  }

  PRBool IsEqual (const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries) const
  {
    return (mEntries == aEntries &&
            memcmp (mTable, aDeadKeyArray, aEntries * sizeof (DeadKeyEntry)) == 0);
  }

  PRUnichar GetCompositeChar (PRUnichar aBaseChar) const;
};



inline PRUnichar VirtualKey::GetCompositeChar (PRUint8 aShiftState, PRUnichar aBaseChar) const
{
  return mShiftStates [aShiftState].DeadKey.Table->GetCompositeChar (aBaseChar);
}

const DeadKeyTable* VirtualKey::MatchingDeadKeyTable (const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries) const
{
  if (!mIsDeadKey)
    return nsnull;

  for (PRUint32 shiftState = 0; shiftState < 16; shiftState++)
  {
    if (IsDeadKey (shiftState))
    {
      const DeadKeyTable* dkt = mShiftStates [shiftState].DeadKey.Table;

      if (dkt && dkt->IsEqual (aDeadKeyArray, aEntries))
        return dkt;
    }
  }

  return nsnull;
}

void VirtualKey::SetNormalChars (PRUint8 aShiftState, const PRUnichar* aChars, PRUint32 aNumOfChars)
{
  NS_ASSERTION (aShiftState < NS_ARRAY_LENGTH (mShiftStates), "invalid index");

  SetDeadKey (aShiftState, PR_FALSE);
  
  for (PRUint32 c1 = 0; c1 < aNumOfChars; c1++)
  {
    
    mShiftStates [aShiftState].Normal.Chars [c1] = (aChars [c1] >= 0x20) ? aChars [c1] : 0;
  }

  for (PRUint32 c2 = aNumOfChars; c2 < NS_ARRAY_LENGTH (mShiftStates [aShiftState].Normal.Chars); c2++)
    mShiftStates [aShiftState].Normal.Chars [c2] = 0;
}

void VirtualKey::SetDeadChar (PRUint8 aShiftState, PRUnichar aDeadChar)
{
  NS_ASSERTION (aShiftState < NS_ARRAY_LENGTH (mShiftStates), "invalid index");
  
  SetDeadKey (aShiftState, PR_TRUE);
  
  mShiftStates [aShiftState].DeadKey.DeadChar = aDeadChar;
  mShiftStates [aShiftState].DeadKey.Table = nsnull;
}

PRUint32 VirtualKey::GetUniChars (PRUint8 aShiftState, PRUnichar* aUniChars, PRUint8* aFinalShiftState) const
{
  *aFinalShiftState = aShiftState;
  PRUint32 numOfChars = GetNativeUniChars (aShiftState, aUniChars);
  
  if (aShiftState & (eAlt | eCtrl))
  {
    PRUnichar unshiftedChars [5];
    PRUint32 numOfUnshiftedChars = GetNativeUniChars (aShiftState & ~(eAlt | eCtrl), unshiftedChars);

    if (numOfChars)
    {
      if ((aShiftState & (eAlt | eCtrl)) == (eAlt | eCtrl)) {
        
        
        
        
        
        *aFinalShiftState &= ~(eAlt | eCtrl);
      } else if (!(numOfChars == numOfUnshiftedChars &&
                   memcmp (aUniChars, unshiftedChars,
                           numOfChars * sizeof (PRUnichar)) == 0)) {
        
        
        *aFinalShiftState &= ~(eAlt | eCtrl);
      }
    } else
    {
      if (numOfUnshiftedChars)
      {
        memcpy (aUniChars, unshiftedChars, numOfUnshiftedChars * sizeof (PRUnichar));
        numOfChars = numOfUnshiftedChars;
      }
    }
  }

  return numOfChars;
}


PRUint32 VirtualKey::GetNativeUniChars (PRUint8 aShiftState, PRUnichar* aUniChars) const
{
  if (IsDeadKey (aShiftState))
  {
    if (aUniChars)
      aUniChars [0] = mShiftStates [aShiftState].DeadKey.DeadChar;

    return 1;
  } else
  {
    PRUint32 cnt = 0;

    while (cnt < NS_ARRAY_LENGTH (mShiftStates [aShiftState].Normal.Chars) &&
           mShiftStates [aShiftState].Normal.Chars [cnt])
    {
      if (aUniChars)
        aUniChars [cnt] = mShiftStates [aShiftState].Normal.Chars [cnt];

      cnt++;
    }

    return cnt;
  }
}

KeyboardLayout::KeyboardLayout () :
  mKeyboardLayout(0)
{
  mDeadKeyTableListHead = nsnull;

  
  
  
}

KeyboardLayout::~KeyboardLayout ()
{
  ReleaseDeadKeyTables ();
}

PRBool KeyboardLayout::IsPrintableCharKey (PRUint8 aVirtualKey)
{
  return GetKeyIndex (aVirtualKey) >= 0;
}

PRBool KeyboardLayout::IsNumpadKey (PRUint8 aVirtualKey)
{
  return VK_NUMPAD0 <= aVirtualKey && aVirtualKey <= VK_DIVIDE;
}

void KeyboardLayout::OnKeyDown (PRUint8 aVirtualKey)
{
  mLastVirtualKeyIndex = GetKeyIndex (aVirtualKey);

  if (mLastVirtualKeyIndex < 0)
  {
    
    mNumOfChars = 0;
  } else
  {
    BYTE kbdState [256];

    if (::GetKeyboardState (kbdState))
    {
      mLastShiftState = GetShiftState (kbdState);

      if (mVirtualKeys [mLastVirtualKeyIndex].IsDeadKey (mLastShiftState))
      {
        if (mActiveDeadKey >= 0)
        {
          
          PRInt32 activeDeadKeyIndex = GetKeyIndex (mActiveDeadKey);
          mVirtualKeys [activeDeadKeyIndex].GetUniChars (mDeadKeyShiftState, mChars, mShiftStates);
          mVirtualKeys [mLastVirtualKeyIndex].GetUniChars (mLastShiftState, &mChars [1], &mShiftStates [1]);
          mNumOfChars = 2;
          
          DeactivateDeadKeyState ();
        } else
        {
          
          mActiveDeadKey = aVirtualKey;
          mDeadKeyShiftState = mLastShiftState;
          mNumOfChars = 0;
        }
      } else
      {
        PRUint8 finalShiftState;
        PRUnichar uniChars [5];
        PRUint32 numOfBaseChars = mVirtualKeys [mLastVirtualKeyIndex].GetUniChars (mLastShiftState, uniChars, &finalShiftState);

        if (mActiveDeadKey >= 0)
        {
          PRInt32 activeDeadKeyIndex = GetKeyIndex (mActiveDeadKey);
          
          
          PRUnichar compositeChar = (numOfBaseChars == 1 && uniChars [0]) ?
            mVirtualKeys [activeDeadKeyIndex].GetCompositeChar (mDeadKeyShiftState, uniChars [0]) : 0;

          if (compositeChar)
          {
            
            mChars [0] = compositeChar;
            mShiftStates [0] = finalShiftState;
            mNumOfChars = 1;
          } else
          {
            
            mVirtualKeys [activeDeadKeyIndex].GetUniChars (mDeadKeyShiftState, mChars, mShiftStates);
            memcpy (&mChars [1], uniChars, numOfBaseChars * sizeof (PRUnichar));
            memset (&mShiftStates [1], finalShiftState, numOfBaseChars);
            mNumOfChars = numOfBaseChars + 1;
          }
        
          DeactivateDeadKeyState ();
        } else
        {
          
          memcpy (mChars, uniChars, numOfBaseChars * sizeof (PRUnichar));
          memset (mShiftStates, finalShiftState, numOfBaseChars);
          mNumOfChars = numOfBaseChars;
        }
      }
    }
  }
}

PRUint32 KeyboardLayout::GetUniChars (PRUnichar* aUniChars, PRUint8* aShiftStates, PRUint32 aMaxChars) const
{
  PRUint32 chars = PR_MIN (mNumOfChars, aMaxChars);

  memcpy (aUniChars, mChars, chars * sizeof (PRUnichar));
  memcpy (aShiftStates, mShiftStates, chars);

  return chars;
}

PRUint32
KeyboardLayout::GetUniCharsWithShiftState(PRUint8 aVirtualKey,
                                          PRUint8 aShiftStates,
                                          PRUnichar* aUniChars,
                                          PRUint32 aMaxChars) const
{
  PRInt32 key = GetKeyIndex(aVirtualKey);
  if (key < 0)
    return 0;
  PRUint8 finalShiftState;
  PRUnichar uniChars[5];
  PRUint32 numOfBaseChars =
    mVirtualKeys[key].GetUniChars(aShiftStates, uniChars, &finalShiftState);
  PRUint32 chars = PR_MIN(numOfBaseChars, aMaxChars);

  memcpy(aUniChars, uniChars, chars * sizeof (PRUnichar));

  return chars;
}

void KeyboardLayout::LoadLayout (HKL aLayout)
{
  if (mKeyboardLayout == aLayout)
    return;

  mKeyboardLayout = aLayout;

  PRUint32 shiftState;
  BYTE kbdState [256];
  BYTE originalKbdState [256];
  PRUint16 shiftStatesWithDeadKeys = 0;     
  PRUint16 shiftStatesWithBaseChars = 0;    

  memset (kbdState, 0, sizeof (kbdState));

  mActiveDeadKey = -1;
  mNumOfChars = 0;

  ReleaseDeadKeyTables ();

  ::GetKeyboardState (originalKbdState);

  
  
  
  for (shiftState = 0; shiftState < 16; shiftState++)
  {
    SetShiftState (kbdState, shiftState);

    for (PRUint32 virtualKey = 0; virtualKey < 256; virtualKey++)
    {
      PRInt32 vki = GetKeyIndex (virtualKey);
      if (vki < 0)
        continue;

      NS_ASSERTION (vki < NS_ARRAY_LENGTH (mVirtualKeys), "invalid index"); 

      PRUnichar uniChars [5];
      PRInt32 rv;

      rv = ::ToUnicodeEx (virtualKey, 0, kbdState, (LPWSTR)uniChars, NS_ARRAY_LENGTH (uniChars), 0, mKeyboardLayout);

      if (rv < 0)   
      {      
        shiftStatesWithDeadKeys |= 1 << shiftState;
        
        
        PRUnichar deadChar [2];

        rv = ::ToUnicodeEx (virtualKey, 0, kbdState, (LPWSTR)deadChar, NS_ARRAY_LENGTH (deadChar), 0, mKeyboardLayout);

        NS_ASSERTION (rv == 2, "Expecting twice repeated dead-key character");

        mVirtualKeys [vki].SetDeadChar (shiftState, deadChar [0]);
      } else
      {
        if (rv == 1)  
          shiftStatesWithBaseChars |= 1 << shiftState;

        mVirtualKeys [vki].SetNormalChars (shiftState, uniChars, rv);
      }
    }
  }

  
  

  for (shiftState = 0; shiftState < 16; shiftState++)
  {
    if (!(shiftStatesWithDeadKeys & (1 << shiftState)))
      continue;

    SetShiftState (kbdState, shiftState);

    for (PRUint32 virtualKey = 0; virtualKey < 256; virtualKey++)
    {
      PRInt32 vki = GetKeyIndex (virtualKey);
  
      if (vki >= 0 && mVirtualKeys [vki].IsDeadKey (shiftState))
      {      
        DeadKeyEntry deadKeyArray [256];

        PRInt32 n = GetDeadKeyCombinations (virtualKey, kbdState, shiftStatesWithBaseChars, 
                                            deadKeyArray, NS_ARRAY_LENGTH (deadKeyArray));

        const DeadKeyTable* dkt = mVirtualKeys [vki].MatchingDeadKeyTable (deadKeyArray, n);
          
        if (!dkt)
          dkt = AddDeadKeyTable (deadKeyArray, n);
        
        mVirtualKeys [vki].AttachDeadKeyTable (shiftState, dkt);
      }
    }
  }

  ::SetKeyboardState (originalKbdState);
}


PRUint8 KeyboardLayout::GetShiftState (const PBYTE aKbdState)
{
  PRBool isShift = (aKbdState [VK_SHIFT] & 0x80) != 0;
  PRBool isCtrl  = (aKbdState [VK_CONTROL] & 0x80) != 0;
  PRBool isAlt   = (aKbdState [VK_MENU] & 0x80) != 0;
  PRBool isCaps  = (aKbdState [VK_CAPITAL] & 0x01) != 0;

  return ((isCaps << 3) | (isAlt << 2) | (isCtrl << 1) | isShift);
}

void KeyboardLayout::SetShiftState (PBYTE aKbdState, PRUint8 aShiftState)
{
  NS_ASSERTION (aShiftState < 16, "aShiftState out of range");

  if (aShiftState & eShift)
    aKbdState [VK_SHIFT] |= 0x80;
  else
  {
    aKbdState [VK_SHIFT]  &= ~0x80;
    aKbdState [VK_LSHIFT] &= ~0x80;
    aKbdState [VK_RSHIFT] &= ~0x80;
  }

  if (aShiftState & eCtrl)
    aKbdState [VK_CONTROL] |= 0x80;
  else
  {
    aKbdState [VK_CONTROL]  &= ~0x80;
    aKbdState [VK_LCONTROL] &= ~0x80;
    aKbdState [VK_RCONTROL] &= ~0x80;
  }

  if (aShiftState & eAlt)
    aKbdState [VK_MENU] |= 0x80;
  else
  {
    aKbdState [VK_MENU]  &= ~0x80;
    aKbdState [VK_LMENU] &= ~0x80;
    aKbdState [VK_RMENU] &= ~0x80;
  }

  if (aShiftState & eCapsLock)
    aKbdState [VK_CAPITAL] |= 0x01;
  else
    aKbdState [VK_CAPITAL] &= ~0x01;
}

inline PRInt32 KeyboardLayout::GetKeyIndex (PRUint8 aVirtualKey)
{



















  static const PRInt8 xlat [256] =
  {
  
  
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
     0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, -1, -1, -1, -1, -1, -1,   
    -1, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,   
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 37, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 38, 39, 40, 41, 42, 43,   
    44, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 45, 46, 47, 48, 49,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1    
  };

  return xlat [aVirtualKey];
}

int KeyboardLayout::CompareDeadKeyEntries (const void* aArg1, const void* aArg2, void*)
{
  const DeadKeyEntry* arg1 = static_cast<const DeadKeyEntry*>(aArg1);
  const DeadKeyEntry* arg2 = static_cast<const DeadKeyEntry*>(aArg2);

  return arg1->BaseChar - arg2->BaseChar;
}

const DeadKeyTable* KeyboardLayout::AddDeadKeyTable (const DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries)
{
  DeadKeyTableListEntry* next = mDeadKeyTableListHead;
  
  const size_t bytes = offsetof (DeadKeyTableListEntry, data) + DeadKeyTable::SizeInBytes (aEntries);
  PRUint8* p = new PRUint8 [bytes];

  mDeadKeyTableListHead = reinterpret_cast<DeadKeyTableListEntry*>(p);
  mDeadKeyTableListHead->next = next;

  DeadKeyTable* dkt = reinterpret_cast<DeadKeyTable*>(mDeadKeyTableListHead->data);
  
  dkt->Init (aDeadKeyArray, aEntries);

  return dkt;
}

void KeyboardLayout::ReleaseDeadKeyTables ()
{
  while (mDeadKeyTableListHead)
  {
    PRUint8* p = reinterpret_cast<PRUint8*>(mDeadKeyTableListHead);
    mDeadKeyTableListHead = mDeadKeyTableListHead->next;

    delete [] p;
  }
}

PRBool KeyboardLayout::EnsureDeadKeyActive (PRBool aIsActive, PRUint8 aDeadKey, const PBYTE aDeadKeyKbdState)
{
  PRInt32 rv;

  do
  {
    PRUnichar dummyChars [5];

    rv = ::ToUnicodeEx (aDeadKey, 0, (PBYTE)aDeadKeyKbdState, (LPWSTR)dummyChars, NS_ARRAY_LENGTH (dummyChars), 0, mKeyboardLayout);
    
    
    
    
    
  } while ((rv < 0) != aIsActive);

  return (rv < 0);
}

void KeyboardLayout::DeactivateDeadKeyState ()
{
  if (mActiveDeadKey < 0)
    return;
  
  BYTE kbdState [256];

  memset (kbdState, 0, sizeof (kbdState));
  SetShiftState (kbdState, mDeadKeyShiftState);

  EnsureDeadKeyActive (PR_FALSE, mActiveDeadKey, kbdState);
  mActiveDeadKey = -1;
}

PRBool KeyboardLayout::AddDeadKeyEntry (PRUnichar aBaseChar, PRUnichar aCompositeChar,
                                        DeadKeyEntry* aDeadKeyArray, PRUint32 aEntries)
{
  for (PRUint32 cnt = 0; cnt < aEntries; cnt++)
    if (aDeadKeyArray [cnt].BaseChar == aBaseChar)
      return PR_FALSE;

  aDeadKeyArray [aEntries].BaseChar = aBaseChar;
  aDeadKeyArray [aEntries].CompositeChar = aCompositeChar;

  return PR_TRUE;
}

PRUint32 KeyboardLayout::GetDeadKeyCombinations (PRUint8 aDeadKey, const PBYTE aDeadKeyKbdState,
                                                 PRUint16 aShiftStatesWithBaseChars,
                                                 DeadKeyEntry* aDeadKeyArray, PRUint32 aMaxEntries)
{
  PRBool deadKeyActive = PR_FALSE;
  PRUint32 entries = 0;
  BYTE kbdState [256];
  
  memset (kbdState, 0, sizeof (kbdState));
  
  for (PRUint32 shiftState = 0; shiftState < 16; shiftState++)
  {
    if (!(aShiftStatesWithBaseChars & (1 << shiftState)))
      continue;

    SetShiftState (kbdState, shiftState);

    for (PRUint32 virtualKey = 0; virtualKey < 256; virtualKey++)
    {
      PRInt32 vki = GetKeyIndex (virtualKey);
      
      
      if (vki >= 0 && mVirtualKeys [vki].GetNativeUniChars (shiftState) == 1)
      {
        
        if (!deadKeyActive)
          deadKeyActive = EnsureDeadKeyActive (PR_TRUE, aDeadKey, aDeadKeyKbdState);

        
        
        PRUnichar compositeChars [5];
        PRInt32 rv;

        rv = ::ToUnicodeEx (virtualKey, 0, kbdState, (LPWSTR)compositeChars, NS_ARRAY_LENGTH (compositeChars), 0, mKeyboardLayout);

        switch (rv)
        {
          case 0:
            
            break;

          case 1:
          {
            
            
            PRUnichar baseChars [5];

            rv = ::ToUnicodeEx (virtualKey, 0, kbdState, (LPWSTR)baseChars, NS_ARRAY_LENGTH (baseChars), 0, mKeyboardLayout);

            NS_ASSERTION (rv == 1, "One base character expected");

            if (rv == 1 && entries < aMaxEntries)
              if (AddDeadKeyEntry (baseChars [0], compositeChars [0], aDeadKeyArray, entries))
                entries++;
            
            deadKeyActive = PR_FALSE;
            break;
          }

          default:
            
            
            deadKeyActive = PR_FALSE;
            break;
        }
      }
    }
  }

  if (deadKeyActive)
    deadKeyActive = EnsureDeadKeyActive (PR_FALSE, aDeadKey, aDeadKeyKbdState);

  NS_QuickSort (aDeadKeyArray, entries, sizeof (DeadKeyEntry), CompareDeadKeyEntries, nsnull);

  return entries;
}


PRUnichar DeadKeyTable::GetCompositeChar (PRUnichar aBaseChar) const
{
  
  

  for (PRUint32 cnt = 0; cnt < mEntries; cnt++)
  {
    if (mTable [cnt].BaseChar == aBaseChar)
      return mTable [cnt].CompositeChar;
    else if (mTable [cnt].BaseChar > aBaseChar)
      break;
  }

  return 0;
}
