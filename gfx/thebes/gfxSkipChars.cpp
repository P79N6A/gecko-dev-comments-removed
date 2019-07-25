




#include "gfxSkipChars.h"

#include <stdlib.h>

#define SHORTCUT_FREQUENCY 256


static bool
IsKeepEntry(uint32_t aEntry)
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
  
    uint32_t i;
    uint32_t nextShortcutIndex = 0;
    uint32_t originalCharOffset = 0;
    uint32_t skippedCharOffset = 0;
    for (i = 0; i < mListLength; ++i) {
        uint8_t len = mList[i];
    
        
        
        
        
        
        
        
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
gfxSkipCharsIterator::SetOffsets(uint32_t aOffset, bool aInOriginalString)
{
    NS_ASSERTION(aOffset <= mSkipChars->mCharCount,
                 "Invalid offset");

    if (mSkipChars->mListLength == 0) {
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
        abs(int32_t(aOffset) - int32_t(mListPrefixCharCount)) > SHORTCUT_FREQUENCY) {
        
        
        uint32_t shortcutIndex = aOffset/SHORTCUT_FREQUENCY;
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
  
    int32_t currentRunLength = mSkipChars->mList[mListPrefixLength];
    for (;;) {
        
        
        uint32_t segmentOffset = aInOriginalString ? mListPrefixCharCount : mListPrefixKeepCharCount;
        if ((aInOriginalString || IsKeepEntry(mListPrefixLength)) &&
            aOffset >= segmentOffset && aOffset < segmentOffset + currentRunLength) {
            int32_t offsetInSegment = aOffset - segmentOffset;
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

bool
gfxSkipCharsIterator::IsOriginalCharSkipped(int32_t* aRunLength) const
{
    if (mSkipChars->mListLength == 0) {
        if (aRunLength) {
            *aRunLength = mSkipChars->mCharCount - mOriginalStringOffset;
        }
        return mSkipChars->mCharCount == uint32_t(mOriginalStringOffset);
    }
  
    uint32_t listPrefixLength = mListPrefixLength;
    
    uint32_t currentRunLength = mSkipChars->mList[listPrefixLength];
    
    
    
    while (currentRunLength == 0 && listPrefixLength < mSkipChars->mListLength - 1) {
        ++listPrefixLength;
        
        
        currentRunLength = mSkipChars->mList[listPrefixLength];
    }
    NS_ASSERTION(uint32_t(mOriginalStringOffset) >= mListPrefixCharCount,
                 "Invariant violation");
    uint32_t offsetIntoCurrentRun =
      uint32_t(mOriginalStringOffset) - mListPrefixCharCount;
    if (listPrefixLength >= mSkipChars->mListLength - 1 &&
        offsetIntoCurrentRun >= currentRunLength) {
        NS_ASSERTION(listPrefixLength == mSkipChars->mListLength - 1 &&
                     offsetIntoCurrentRun == currentRunLength,
                     "Overran end of string");
        
        if (aRunLength) {
            *aRunLength = 0;
        }
        return true;
    }
  
    bool isSkipped = !IsKeepEntry(listPrefixLength);
    if (aRunLength) {
        
        
        
        uint32_t runLength = currentRunLength - offsetIntoCurrentRun;
        for (uint32_t i = listPrefixLength + 2; i < mSkipChars->mListLength; i += 2) {
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
    
    uint32_t charCount = mRunCharCount;
    for (;;) {
        uint32_t chars = NS_MIN<uint32_t>(255, charCount);
        if (!mBuffer.AppendElement(chars)) {
            mInErrorState = true;
            return;
        }
        charCount -= chars;
        if (charCount == 0)
            break;
        if (!mBuffer.AppendElement(0)) {
            mInErrorState = true;
            return;
        }
    }
  
    NS_ASSERTION(mCharCount + mRunCharCount >= mCharCount,
                 "String length overflow");
    mCharCount += mRunCharCount;
    mRunCharCount = 0;
    mRunSkipped = !mRunSkipped;
}
