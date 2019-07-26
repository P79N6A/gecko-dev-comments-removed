




#ifndef GFX_SKIP_CHARS_H
#define GFX_SKIP_CHARS_H

#include "prtypes.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "gfxTypes.h"


























class THEBES_API gfxSkipCharsBuilder {
public:
    gfxSkipCharsBuilder() :
        mCharCount(0), mRunCharCount(0), mRunSkipped(false), mInErrorState(false)
    {}
  
    void SkipChars(uint32_t aChars) {
        DoChars(aChars, true);
    }
    void KeepChars(uint32_t aChars) {
        DoChars(aChars, false);
    }
    void SkipChar() {
        SkipChars(1);
    }
    void KeepChar() {
        KeepChars(1);
    }
    void DoChars(uint32_t aChars, bool aSkipped) {
        if (aSkipped != mRunSkipped && aChars > 0) {
            FlushRun();
        }
        NS_ASSERTION(mRunCharCount + aChars > mRunCharCount,
                     "Character count overflow");
        mRunCharCount += aChars;
    }

    bool IsOK() { return !mInErrorState; }

    uint32_t GetCharCount() { return mCharCount + mRunCharCount; }
    bool GetAllCharsKept() { return mBuffer.Length() == 0; }

    friend class gfxSkipChars;

private:
    typedef AutoFallibleTArray<uint8_t,256> Buffer;

    



    void FlushRun();
  
    Buffer       mBuffer;
    uint32_t     mCharCount;
    uint32_t     mRunCharCount;
    bool mRunSkipped; 
    bool mInErrorState;
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
            mList = nullptr;
            mListLength = 0;
        } else {
            aSkipCharsBuilder->FlushRun();
            mCharCount = aSkipCharsBuilder->mCharCount;
            mList = new uint8_t[aSkipCharsBuilder->mBuffer.Length()];
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
        aSkipCharsBuilder->mRunSkipped = false;
        BuildShortcuts();
    }
  
    void SetAllKeep(uint32_t aLength) {
        mCharCount = aLength;
        mList = nullptr;
        mListLength = 0;
    }
  
    int32_t GetOriginalCharCount() const { return mCharCount; }

    friend class gfxSkipCharsIterator;

private:
    struct Shortcut {
        uint32_t mListPrefixLength;
        uint32_t mListPrefixCharCount;
        uint32_t mListPrefixKeepCharCount;
    
        Shortcut() {}
        Shortcut(uint32_t aListPrefixLength, uint32_t aListPrefixCharCount,
                 uint32_t aListPrefixKeepCharCount) :
            mListPrefixLength(aListPrefixLength),
            mListPrefixCharCount(aListPrefixCharCount),
            mListPrefixKeepCharCount(aListPrefixKeepCharCount) {}
    };
  
    void BuildShortcuts();

    nsAutoArrayPtr<uint8_t>  mList;
    nsAutoArrayPtr<Shortcut> mShortcuts;
    uint32_t                 mListLength;
    uint32_t                 mCharCount;
};


















class THEBES_API gfxSkipCharsIterator {
public:
    



    gfxSkipCharsIterator(const gfxSkipChars& aSkipChars,
                         int32_t aOriginalStringToSkipCharsOffset,
                         int32_t aOriginalStringOffset)
        : mSkipChars(&aSkipChars),
          mOriginalStringToSkipCharsOffset(aOriginalStringToSkipCharsOffset),
          mListPrefixLength(0), mListPrefixCharCount(0), mListPrefixKeepCharCount(0) {
          SetOriginalOffset(aOriginalStringOffset);
    }

    gfxSkipCharsIterator(const gfxSkipChars& aSkipChars,
                         int32_t aOriginalStringToSkipCharsOffset = 0)
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
  
    


    gfxSkipCharsIterator() : mSkipChars(nullptr) {}

    

  
    bool IsInitialized() { return mSkipChars != nullptr; }

    




    void SetOriginalOffset(int32_t aOriginalStringOffset) {
        SetOffsets(aOriginalStringOffset + mOriginalStringToSkipCharsOffset, true);
    }
    
    




    void SetSkippedOffset(uint32_t aSkippedStringOffset) {
        SetOffsets(aSkippedStringOffset, false);
    }
    
    uint32_t ConvertOriginalToSkipped(int32_t aOriginalStringOffset) {
        SetOriginalOffset(aOriginalStringOffset);
        return GetSkippedOffset();
    }
    uint32_t ConvertSkippedToOriginal(int32_t aSkippedStringOffset) {
        SetSkippedOffset(aSkippedStringOffset);
        return GetOriginalOffset();
    }
  
    






    bool IsOriginalCharSkipped(int32_t* aRunLength = nullptr) const;
    
    void AdvanceOriginal(int32_t aDelta) {
        SetOffsets(mOriginalStringOffset + aDelta, true);
    }
    void AdvanceSkipped(int32_t aDelta) {
        SetOffsets(mSkippedStringOffset + aDelta, false);
    }
  
    


    int32_t GetOriginalOffset() const {
        return mOriginalStringOffset - mOriginalStringToSkipCharsOffset;
    }
    







    uint32_t GetSkippedOffset() const { return mSkippedStringOffset; }

    int32_t GetOriginalEnd() const {
        return mSkipChars->GetOriginalCharCount() -
            mOriginalStringToSkipCharsOffset;
    }

private:
    void SetOffsets(uint32_t aOffset, bool aInOriginalString);
  
    const gfxSkipChars* mSkipChars;
    int32_t mOriginalStringOffset;
    uint32_t mSkippedStringOffset;

    
    
    
    int32_t mOriginalStringToSkipCharsOffset;

    









    uint32_t mListPrefixLength;
    uint32_t mListPrefixCharCount;
    uint32_t mListPrefixKeepCharCount;
};

#endif 
