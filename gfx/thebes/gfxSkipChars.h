




#ifndef GFX_SKIP_CHARS_H
#define GFX_SKIP_CHARS_H

#include "nsTArray.h"
















class gfxSkipChars
{
    friend struct SkippedRangeStartComparator;
    friend struct SkippedRangeOffsetComparator;

private:
    class SkippedRange
    {
    public:
        SkippedRange(uint32_t aOffset, uint32_t aLength, uint32_t aDelta)
            : mOffset(aOffset), mLength(aLength), mDelta(aDelta)
        { }

        uint32_t Start() const
        {
            return mOffset;
        }

        uint32_t End() const
        {
            return mOffset + mLength;
        }

        uint32_t Length() const
        {
            return mLength;
        }

        uint32_t SkippedOffset() const
        {
            return mOffset - mDelta;
        }

        uint32_t Delta() const
        {
            return mDelta;
        }

        uint32_t NextDelta() const
        {
            return mDelta + mLength;
        }

        void Extend(uint32_t aChars)
        {
            mLength += aChars;
        }

    private:
        uint32_t mOffset; 
        uint32_t mLength; 
        uint32_t mDelta;  
    };

public:
    gfxSkipChars()
        : mCharCount(0)
    { }

    void SkipChars(uint32_t aChars)
    {
        NS_ASSERTION(mCharCount + aChars > mCharCount,
                     "Character count overflow");
        uint32_t rangeCount = mRanges.Length();
        uint32_t delta = 0;
        if (rangeCount > 0) {
            SkippedRange& lastRange = mRanges[rangeCount - 1];
            if (lastRange.End() == mCharCount) {
                lastRange.Extend(aChars);
                mCharCount += aChars;
                return;
            }
            delta = lastRange.NextDelta();
        }
        mRanges.AppendElement(SkippedRange(mCharCount, aChars, delta));
        mCharCount += aChars;
    }

    void KeepChars(uint32_t aChars)
    {
        NS_ASSERTION(mCharCount + aChars > mCharCount,
                     "Character count overflow");
        mCharCount += aChars;
    }

    void SkipChar()
    {
        SkipChars(1);
    }

    void KeepChar()
    {
        KeepChars(1);
    }

    void TakeFrom(gfxSkipChars* aSkipChars)
    {
        mRanges.SwapElements(aSkipChars->mRanges);
        mCharCount = aSkipChars->mCharCount;
        aSkipChars->mCharCount = 0;
    }

    int32_t GetOriginalCharCount() const
    {
        return mCharCount;
    }

    const SkippedRange& LastRange() const
    {
        
        
        return mRanges[mRanges.Length() - 1];
    }

    friend class gfxSkipCharsIterator;

private:
    nsTArray<SkippedRange> mRanges;
    uint32_t               mCharCount;
};


















class gfxSkipCharsIterator
{
public:
    



    gfxSkipCharsIterator(const gfxSkipChars& aSkipChars,
                         int32_t aOriginalStringToSkipCharsOffset,
                         int32_t aOriginalStringOffset)
        : mSkipChars(&aSkipChars),
          mOriginalStringOffset(0),
          mSkippedStringOffset(0),
          mCurrentRangeIndex(-1),
          mOriginalStringToSkipCharsOffset(aOriginalStringToSkipCharsOffset)
    {
        SetOriginalOffset(aOriginalStringOffset);
    }

    explicit gfxSkipCharsIterator(const gfxSkipChars& aSkipChars,
                                  int32_t aOriginalStringToSkipCharsOffset = 0)
        : mSkipChars(&aSkipChars),
          mOriginalStringOffset(0),
          mSkippedStringOffset(0),
          mOriginalStringToSkipCharsOffset(aOriginalStringToSkipCharsOffset)
    {
        mCurrentRangeIndex =
            mSkipChars->mRanges.IsEmpty() ||
            mSkipChars->mRanges[0].Start() > 0 ? -1 : 0;
    }

    gfxSkipCharsIterator(const gfxSkipCharsIterator& aIterator)
        : mSkipChars(aIterator.mSkipChars),
          mOriginalStringOffset(aIterator.mOriginalStringOffset),
          mSkippedStringOffset(aIterator.mSkippedStringOffset),
          mCurrentRangeIndex(aIterator.mCurrentRangeIndex),
          mOriginalStringToSkipCharsOffset(aIterator.mOriginalStringToSkipCharsOffset)
    { }

    


    gfxSkipCharsIterator()
        : mSkipChars(nullptr)
    { }

    


    bool IsInitialized()
    {
        return mSkipChars != nullptr;
    }

    




    void SetOriginalOffset(int32_t aOriginalStringOffset);

    




    void SetSkippedOffset(uint32_t aSkippedStringOffset);

    uint32_t ConvertOriginalToSkipped(int32_t aOriginalStringOffset)
    {
        SetOriginalOffset(aOriginalStringOffset);
        return GetSkippedOffset();
    }

    uint32_t ConvertSkippedToOriginal(int32_t aSkippedStringOffset)
    {
        SetSkippedOffset(aSkippedStringOffset);
        return GetOriginalOffset();
    }

    






    bool IsOriginalCharSkipped(int32_t* aRunLength = nullptr) const;

    void AdvanceOriginal(int32_t aDelta)
    {
        SetOriginalOffset(GetOriginalOffset() + aDelta);
    }

    void AdvanceSkipped(int32_t aDelta)
    {
        SetSkippedOffset(GetSkippedOffset() + aDelta);
    }

    


    int32_t GetOriginalOffset() const
    {
        return mOriginalStringOffset - mOriginalStringToSkipCharsOffset;
    }

    







    uint32_t GetSkippedOffset() const
    {
        return mSkippedStringOffset;
    }

    int32_t GetOriginalEnd() const
    {
        return mSkipChars->GetOriginalCharCount() -
            mOriginalStringToSkipCharsOffset;
    }

private:
    const gfxSkipChars* mSkipChars;

    
    int32_t mOriginalStringOffset;
    uint32_t mSkippedStringOffset;

    
    
    
    int32_t mCurrentRangeIndex;

    
    
    
    int32_t mOriginalStringToSkipCharsOffset;
};

#endif 
