




































#ifndef GFX_SKIP_CHARS_H
#define GFX_SKIP_CHARS_H

#include "prtypes.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "gfxTypes.h"


























class THEBES_API gfxSkipCharsBuilder {
public:
    gfxSkipCharsBuilder() :
        mCharCount(0), mRunCharCount(0), mRunSkipped(PR_FALSE), mInErrorState(PR_FALSE)
    {}
  
    void SkipChars(PRUint32 aChars) {
        DoChars(aChars, PR_TRUE);
    }
    void KeepChars(PRUint32 aChars) {
        DoChars(aChars, PR_FALSE);
    }
    void SkipChar() {
        SkipChars(1);
    }
    void KeepChar() {
        KeepChars(1);
    }
    void DoChars(PRUint32 aChars, PRBool aSkipped) {
        if (aSkipped != mRunSkipped && aChars > 0) {
            FlushRun();
        }
        NS_ASSERTION(mRunCharCount + aChars > mRunCharCount,
                     "Character count overflow");
        mRunCharCount += aChars;
    }

    PRBool IsOK() { return !mInErrorState; }

    PRUint32 GetCharCount() { return mCharCount + mRunCharCount; }
    PRBool GetAllCharsKept() { return mBuffer.Length() == 0; }

    friend class gfxSkipChars;

private:
    typedef nsAutoTArray<PRUint8,256> Buffer;

    



    void FlushRun();
  
    Buffer       mBuffer;
    PRUint32     mCharCount;
    PRUint32     mRunCharCount;
    PRPackedBool mRunSkipped; 
    PRPackedBool mInErrorState;
};









class THEBES_API gfxSkipChars {
public:
    gfxSkipChars() : mListLength(0), mCharCount(0) {}
  
    void TakeFrom(gfxSkipChars* aSkipChars) {
        mList = aSkipChars->mList.forget();
        mListLength = aSkipChars->mListLength;
        mCharCount = aSkipChars->mCharCount;
        aSkipChars->mCharCount = 0;
        aSkipChars->mListLength = 0;
        BuildShortcuts();
    }
  
    void TakeFrom(gfxSkipCharsBuilder* aSkipCharsBuilder) {
        if (!aSkipCharsBuilder->mBuffer.Length()) {
            NS_ASSERTION(!aSkipCharsBuilder->mRunSkipped, "out of sync");
            
            mCharCount = aSkipCharsBuilder->mRunCharCount;
            mList = nsnull;
            mListLength = 0;
        } else {
            aSkipCharsBuilder->FlushRun();
            mCharCount = aSkipCharsBuilder->mCharCount;
            mList = new PRUint8[aSkipCharsBuilder->mBuffer.Length()];
            if (!mList) {
                mListLength = 0;
            } else {
                mListLength = aSkipCharsBuilder->mBuffer.Length();
                memcpy(mList, aSkipCharsBuilder->mBuffer.Elements(), mListLength);
            }
        }
        aSkipCharsBuilder->mBuffer.Clear();
        aSkipCharsBuilder->mCharCount = 0;
        aSkipCharsBuilder->mRunCharCount = 0;    
        aSkipCharsBuilder->mRunSkipped = PR_FALSE;
        BuildShortcuts();
    }
  
    void SetAllKeep(PRUint32 aLength) {
        mCharCount = aLength;
        mList = nsnull;
        mListLength = 0;
    }
  
      PRInt32 GetOriginalCharCount() const { return mCharCount; }

      friend class gfxSkipCharsIterator;

private:
    struct Shortcut {
        PRUint32 mListPrefixLength;
        PRUint32 mListPrefixCharCount;
        PRUint32 mListPrefixKeepCharCount;
    
        Shortcut() {}
        Shortcut(PRUint32 aListPrefixLength, PRUint32 aListPrefixCharCount,
                 PRUint32 aListPrefixKeepCharCount) :
            mListPrefixLength(aListPrefixLength),
            mListPrefixCharCount(aListPrefixCharCount),
            mListPrefixKeepCharCount(aListPrefixKeepCharCount) {}
    };
  
    void BuildShortcuts();

    nsAutoArrayPtr<PRUint8>  mList;
    nsAutoArrayPtr<Shortcut> mShortcuts;
    PRUint32                 mListLength;
    PRUint32                 mCharCount;
};














class THEBES_API gfxSkipCharsIterator {
public:
    



    gfxSkipCharsIterator(const gfxSkipChars& aSkipChars,
                         PRInt32 aOriginalStringToSkipCharsOffset,
                         PRInt32 aOriginalStringOffset)
        : mSkipChars(&aSkipChars),
          mOriginalStringToSkipCharsOffset(aOriginalStringToSkipCharsOffset),
          mListPrefixLength(0), mListPrefixCharCount(0), mListPrefixKeepCharCount(0) {
          SetOriginalOffset(aOriginalStringOffset);
    }

    gfxSkipCharsIterator(const gfxSkipChars& aSkipChars,
                         PRInt32 aOriginalStringToSkipCharsOffset = 0)
        : mSkipChars(&aSkipChars),
          mOriginalStringOffset(0), mSkippedStringOffset(0),
          mOriginalStringToSkipCharsOffset(aOriginalStringToSkipCharsOffset),
          mListPrefixLength(0), mListPrefixCharCount(0), mListPrefixKeepCharCount(0) {
    }

    gfxSkipCharsIterator(const gfxSkipCharsIterator& aIterator)
        : mSkipChars(aIterator.mSkipChars),
          mOriginalStringOffset(aIterator.mOriginalStringOffset),
          mSkippedStringOffset(aIterator.mSkippedStringOffset),
          mOriginalStringToSkipCharsOffset(aIterator.mOriginalStringToSkipCharsOffset),
          mListPrefixLength(aIterator.mListPrefixLength),
          mListPrefixCharCount(aIterator.mListPrefixCharCount),
          mListPrefixKeepCharCount(aIterator.mListPrefixKeepCharCount)
    {}
  
    


    gfxSkipCharsIterator() : mSkipChars(nsnull) {}

    

  
    PRBool IsInitialized() { return mSkipChars != nsnull; }

    




    void SetOriginalOffset(PRInt32 aOriginalStringOffset) {
        SetOffsets(aOriginalStringOffset + mOriginalStringToSkipCharsOffset, PR_TRUE);
    }
    
    




    void SetSkippedOffset(PRUint32 aSkippedStringOffset) {
        SetOffsets(aSkippedStringOffset, PR_FALSE);
    }
    
    PRUint32 ConvertOriginalToSkipped(PRInt32 aOriginalStringOffset) {
        SetOriginalOffset(aOriginalStringOffset);
        return GetSkippedOffset();
    }
    PRUint32 ConvertSkippedToOriginal(PRInt32 aSkippedStringOffset) {
        SetSkippedOffset(aSkippedStringOffset);
        return GetOriginalOffset();
    }
  
    






    PRBool IsOriginalCharSkipped(PRInt32* aRunLength = nsnull) const;
    
    void AdvanceOriginal(PRInt32 aDelta) {
        SetOffsets(mOriginalStringOffset + aDelta, PR_TRUE);
    }
    void AdvanceSkipped(PRInt32 aDelta) {
        SetOffsets(mSkippedStringOffset + aDelta, PR_FALSE);
    }
  
    


    PRInt32 GetOriginalOffset() const {
        return mOriginalStringOffset - mOriginalStringToSkipCharsOffset;
    }
    







    PRUint32 GetSkippedOffset() const { return mSkippedStringOffset; }
  
private:
    void SetOffsets(PRUint32 aOffset, PRBool aInOriginalString);
  
    const gfxSkipChars* mSkipChars;
    PRInt32 mOriginalStringOffset;
    PRUint32 mSkippedStringOffset;
    
    PRUint32 mOriginalStringToSkipCharsOffset;
    
    









    PRUint32 mListPrefixLength;
    PRUint32 mListPrefixCharCount;
    PRUint32 mListPrefixKeepCharCount;
};

#endif 
