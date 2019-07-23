




































#include "gfxSkipChars.h"

#include <stdlib.h>

#define SHORTCUT_FREQUENCY 256


static PRBool
IsKeepEntry(PRUint32 aEntry)
{
    return !(aEntry & 1);
}

void
gfxSkipChars::BuildShortcuts()
{
    if (!mList || mCharCount < SHORTCUT_FREQUENCY)
        return;
  
    mShortcuts = new Shortcut[mCharCount/SHORTCUT_FREQUENCY];
    if (!mShortcuts)
        return;
  
    PRUint32 i;
    PRUint32 nextShortcutIndex = 0;
    PRUint32 originalCharOffset = 0;
    PRUint32 skippedCharOffset = 0;
    for (i = 0; i < mListLength; ++i) {
        PRUint8 len = mList[i];
    
        
        
        
        
        
        
        
        while (originalCharOffset + len >= (nextShortcutIndex + 1)*SHORTCUT_FREQUENCY) {
            mShortcuts[nextShortcutIndex] =
                Shortcut(i, originalCharOffset, skippedCharOffset);
            ++nextShortcutIndex;
        }
    
        originalCharOffset += len;
        if (IsKeepEntry(i)) {
            skippedCharOffset += len;
        }
    }
}

void
gfxSkipCharsIterator::SetOffsets(PRUint32 aOffset, PRBool aInOriginalString)
{
    if (mSkipChars->mListLength == 0) {
        
        if (aOffset < 0) {
            aOffset = 0;
        } else if (aOffset > mSkipChars->mCharCount) {
            aOffset = mSkipChars->mCharCount;
        }
        mOriginalStringOffset = mSkippedStringOffset = aOffset;
        return;
    }
  
    if (aOffset == 0) {
        
        mSkippedStringOffset = 0;
        mOriginalStringOffset = 0;
        mListPrefixLength = 0;
        mListPrefixKeepCharCount = 0;
        mListPrefixCharCount = 0;
        if (aInOriginalString) {
            
            return;
        }
    }
  
    if (aInOriginalString && mSkipChars->mShortcuts &&
        abs(PRInt32(aOffset) - PRInt32(mListPrefixCharCount)) > SHORTCUT_FREQUENCY) {
        
        
        PRUint32 shortcutIndex = aOffset/SHORTCUT_FREQUENCY;
        if (shortcutIndex == 0) {
            mListPrefixLength = 0;
            mListPrefixKeepCharCount = 0;
            mListPrefixCharCount = 0;
        } else {
            const gfxSkipChars::Shortcut& shortcut = mSkipChars->mShortcuts[shortcutIndex - 1];
            mListPrefixLength = shortcut.mListPrefixLength;
            mListPrefixKeepCharCount = shortcut.mListPrefixKeepCharCount;
            mListPrefixCharCount = shortcut.mListPrefixCharCount;
        }
    }
  
    PRInt32 currentRunLength = mSkipChars->mList[mListPrefixLength];
    for (;;) {
        
        
        PRUint32 segmentOffset = aInOriginalString ? mListPrefixCharCount : mListPrefixKeepCharCount;
        if ((aInOriginalString || IsKeepEntry(mListPrefixLength)) &&
            aOffset >= segmentOffset && aOffset < segmentOffset + currentRunLength) {
            PRInt32 offsetInSegment = aOffset - segmentOffset;
            mOriginalStringOffset = mListPrefixCharCount + offsetInSegment;
            mSkippedStringOffset = mListPrefixKeepCharCount;
            if (IsKeepEntry(mListPrefixLength)) {
                mSkippedStringOffset += offsetInSegment;
            }
            return;
        }
        
        if (aOffset < segmentOffset) {
            
            if (mListPrefixLength <= 0) {
                
                mOriginalStringOffset = mSkippedStringOffset = 0;
                return;
            }
            
            --mListPrefixLength;
            currentRunLength = mSkipChars->mList[mListPrefixLength];
            mListPrefixCharCount -= currentRunLength;
            if (IsKeepEntry(mListPrefixLength)) {
                mListPrefixKeepCharCount -= currentRunLength;
            }
        } else {
            
            if (mListPrefixLength >= mSkipChars->mListLength - 1) {
                
                mOriginalStringOffset = mListPrefixCharCount + currentRunLength;
                mSkippedStringOffset = mListPrefixKeepCharCount;
                if (IsKeepEntry(mListPrefixLength)) {
                    mSkippedStringOffset += currentRunLength;
                }
                return;
            }
            
            mListPrefixCharCount += currentRunLength;
            if (IsKeepEntry(mListPrefixLength)) {
                mListPrefixKeepCharCount += currentRunLength;
            }
            ++mListPrefixLength;
            currentRunLength = mSkipChars->mList[mListPrefixLength];
        }
    }
}

PRBool
gfxSkipCharsIterator::IsOriginalCharSkipped(PRInt32* aRunLength) const
{
    if (mSkipChars->mListLength == 0) {
        if (aRunLength) {
            *aRunLength = mSkipChars->mCharCount - mOriginalStringOffset;
        }
        return mSkipChars->mCharCount == PRUint32(mOriginalStringOffset);
    }
  
    PRUint32 listPrefixLength = mListPrefixLength;
    
    PRUint32 currentRunLength = mSkipChars->mList[listPrefixLength];
    
    
    
    while (currentRunLength == 0 && listPrefixLength < mSkipChars->mListLength - 1) {
        ++listPrefixLength;
        
        
        currentRunLength = mSkipChars->mList[listPrefixLength];
    }
    NS_ASSERTION(PRUint32(mOriginalStringOffset) >= mListPrefixCharCount,
                 "Invariant violation");
    PRUint32 offsetIntoCurrentRun =
      PRUint32(mOriginalStringOffset) - mListPrefixCharCount;
    if (listPrefixLength >= mSkipChars->mListLength - 1 &&
        offsetIntoCurrentRun >= currentRunLength) {
        NS_ASSERTION(listPrefixLength == mSkipChars->mListLength - 1 &&
                     offsetIntoCurrentRun == currentRunLength,
                     "Overran end of string");
        
        if (aRunLength) {
            *aRunLength = 0;
        }
        return PR_TRUE;
    }
  
    PRBool isSkipped = !IsKeepEntry(listPrefixLength);
    if (aRunLength) {
        
        
        
        PRUint32 runLength = currentRunLength - offsetIntoCurrentRun;
        for (PRUint32 i = listPrefixLength + 2; i < mSkipChars->mListLength; i += 2) {
            if (mSkipChars->mList[i - 1] != 0)
                break;
            runLength += mSkipChars->mList[i];
        }
        *aRunLength = runLength;
    }
    return isSkipped;
}

void
gfxSkipCharsBuilder::FlushRun()
{
    NS_ASSERTION((mBuffer.Length() & 1) == mRunSkipped,
                 "out of sync?");
    
    PRUint32 charCount = mRunCharCount;
    for (;;) {
        PRUint32 chars = PR_MIN(255, charCount);
        if (!mBuffer.AppendElement(chars)) {
            mInErrorState = PR_TRUE;
            return;
        }
        charCount -= chars;
        if (charCount == 0)
            break;
        if (!mBuffer.AppendElement(0)) {
            mInErrorState = PR_TRUE;
            return;
        }
    }
  
    NS_ASSERTION(mCharCount + mRunCharCount >= mCharCount,
                 "String length overflow");
    mCharCount += mRunCharCount;
    mRunCharCount = 0;
    mRunSkipped = !mRunSkipped;
}
