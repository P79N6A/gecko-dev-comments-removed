




#ifndef MOZILLA_MEDIASEGMENT_H_
#define MOZILLA_MEDIASEGMENT_H_

#include "nsTArray.h"
#ifdef MOZILLA_INTERNAL_API
#include "mozilla/TimeStamp.h"
#endif
#include <algorithm>
#include "Latency.h"

namespace mozilla {






typedef int32_t TrackRate;
const int64_t TRACK_RATE_MAX_BITS = 20;
const TrackRate TRACK_RATE_MAX = 1 << TRACK_RATE_MAX_BITS;









typedef int64_t TrackTicks;
const int64_t TRACK_TICKS_MAX = INT64_MAX >> TRACK_RATE_MAX_BITS;





typedef int64_t MediaTime;
const int64_t MEDIA_TIME_MAX = TRACK_TICKS_MAX;




typedef MediaTime StreamTime;
const StreamTime STREAM_TIME_MAX = MEDIA_TIME_MAX;













class MediaSegment {
public:
  virtual ~MediaSegment()
  {
    MOZ_COUNT_DTOR(MediaSegment);
  }

  enum Type {
    AUDIO,
    VIDEO,
    TYPE_COUNT
  };

  


  StreamTime GetDuration() const { return mDuration; }
  Type GetType() const { return mType; }

  


  virtual MediaSegment* CreateEmptyClone() const = 0;
  


  virtual void AppendFrom(MediaSegment* aSource) = 0;
  


  virtual void AppendSlice(const MediaSegment& aSource,
                           StreamTime aStart, StreamTime aEnd) = 0;
  


  virtual void ForgetUpTo(StreamTime aDuration) = 0;
  


  virtual void FlushAfter(StreamTime aNewEnd) = 0;
  


  virtual void InsertNullDataAtStart(StreamTime aDuration) = 0;
  


  virtual void AppendNullData(StreamTime aDuration) = 0;
  


  virtual void ReplaceWithDisabled() = 0;
  


  virtual void Clear() = 0;

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return 0;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  explicit MediaSegment(Type aType) : mDuration(0), mType(aType)
  {
    MOZ_COUNT_CTOR(MediaSegment);
  }

  StreamTime mDuration; 
  Type mType;
};





template <class C, class Chunk> class MediaSegmentBase : public MediaSegment {
public:
  virtual MediaSegment* CreateEmptyClone() const override
  {
    return new C();
  }
  virtual void AppendFrom(MediaSegment* aSource) override
  {
    NS_ASSERTION(aSource->GetType() == C::StaticType(), "Wrong type");
    AppendFromInternal(static_cast<C*>(aSource));
  }
  void AppendFrom(C* aSource)
  {
    AppendFromInternal(aSource);
  }
  virtual void AppendSlice(const MediaSegment& aSource,
                           StreamTime aStart, StreamTime aEnd) override
  {
    NS_ASSERTION(aSource.GetType() == C::StaticType(), "Wrong type");
    AppendSliceInternal(static_cast<const C&>(aSource), aStart, aEnd);
  }
  void AppendSlice(const C& aOther, StreamTime aStart, StreamTime aEnd)
  {
    AppendSliceInternal(aOther, aStart, aEnd);
  }
  



  virtual void ForgetUpTo(StreamTime aDuration) override
  {
    if (mChunks.IsEmpty() || aDuration <= 0) {
      return;
    }
    if (mChunks[0].IsNull()) {
      StreamTime extraToForget = std::min(aDuration, mDuration) - mChunks[0].GetDuration();
      if (extraToForget > 0) {
        RemoveLeading(extraToForget, 1);
        mChunks[0].mDuration += extraToForget;
        mDuration += extraToForget;
      }
      return;
    }
    RemoveLeading(aDuration, 0);
    mChunks.InsertElementAt(0)->SetNull(aDuration);
    mDuration += aDuration;
  }
  virtual void FlushAfter(StreamTime aNewEnd) override
  {
    if (mChunks.IsEmpty()) {
      return;
    }

    if (mChunks[0].IsNull()) {
      StreamTime extraToKeep = aNewEnd - mChunks[0].GetDuration();
      if (extraToKeep < 0) {
        
        mChunks[0].SetNull(aNewEnd);
        extraToKeep = 0;
      }
      RemoveTrailing(extraToKeep, 1);
    } else {
      if (aNewEnd > mDuration) {
        NS_ASSERTION(aNewEnd <= mDuration, "can't add data in FlushAfter");
        return;
      }
      RemoveTrailing(aNewEnd, 0);
    }
    mDuration = aNewEnd;
  }
  virtual void InsertNullDataAtStart(StreamTime aDuration) override
  {
    if (aDuration <= 0) {
      return;
    }
    if (!mChunks.IsEmpty() && mChunks[0].IsNull()) {
      mChunks[0].mDuration += aDuration;
    } else {
      mChunks.InsertElementAt(0)->SetNull(aDuration);
    }
#ifdef MOZILLA_INTERNAL_API
    mChunks[0].mTimeStamp = mozilla::TimeStamp::Now();
#endif
    mDuration += aDuration;
  }
  virtual void AppendNullData(StreamTime aDuration) override
  {
    if (aDuration <= 0) {
      return;
    }
    if (!mChunks.IsEmpty() && mChunks[mChunks.Length() - 1].IsNull()) {
      mChunks[mChunks.Length() - 1].mDuration += aDuration;
    } else {
      mChunks.AppendElement()->SetNull(aDuration);
    }
    mDuration += aDuration;
  }
  virtual void ReplaceWithDisabled() override
  {
    if (GetType() != AUDIO) {
      MOZ_CRASH("Disabling unknown segment type");
    }
    StreamTime duration = GetDuration();
    Clear();
    AppendNullData(duration);
  }
  virtual void Clear() override
  {
    mDuration = 0;
    mChunks.Clear();
  }

  class ChunkIterator {
  public:
    explicit ChunkIterator(MediaSegmentBase<C, Chunk>& aSegment)
      : mSegment(aSegment), mIndex(0) {}
    bool IsEnded() { return mIndex >= mSegment.mChunks.Length(); }
    void Next() { ++mIndex; }
    Chunk& operator*() { return mSegment.mChunks[mIndex]; }
    Chunk* operator->() { return &mSegment.mChunks[mIndex]; }
  private:
    MediaSegmentBase<C, Chunk>& mSegment;
    uint32_t mIndex;
  };
  class ConstChunkIterator {
  public:
    explicit ConstChunkIterator(const MediaSegmentBase<C, Chunk>& aSegment)
      : mSegment(aSegment), mIndex(0) {}
    bool IsEnded() { return mIndex >= mSegment.mChunks.Length(); }
    void Next() { ++mIndex; }
    const Chunk& operator*() { return mSegment.mChunks[mIndex]; }
    const Chunk* operator->() { return &mSegment.mChunks[mIndex]; }
  private:
    const MediaSegmentBase<C, Chunk>& mSegment;
    uint32_t mIndex;
  };

  void RemoveLeading(StreamTime aDuration)
  {
    RemoveLeading(aDuration, 0);
  }

#ifdef MOZILLA_INTERNAL_API
  void GetStartTime(TimeStamp &aTime) {
    aTime = mChunks[0].mTimeStamp;
  }
#endif

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = mChunks.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < mChunks.Length(); i++) {
      amount += mChunks[i].SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    }
    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  explicit MediaSegmentBase(Type aType) : MediaSegment(aType) {}

  


  void AppendFromInternal(MediaSegmentBase<C, Chunk>* aSource)
  {
    MOZ_ASSERT(aSource->mDuration >= 0);
    mDuration += aSource->mDuration;
    aSource->mDuration = 0;
    if (!mChunks.IsEmpty() && !aSource->mChunks.IsEmpty() &&
        mChunks[mChunks.Length() - 1].CanCombineWithFollowing(aSource->mChunks[0])) {
      mChunks[mChunks.Length() - 1].mDuration += aSource->mChunks[0].mDuration;
      aSource->mChunks.RemoveElementAt(0);
    }
    mChunks.MoveElementsFrom(aSource->mChunks);
  }

  void AppendSliceInternal(const MediaSegmentBase<C, Chunk>& aSource,
                           StreamTime aStart, StreamTime aEnd)
  {
    MOZ_ASSERT(aStart <= aEnd, "Endpoints inverted");
    NS_WARN_IF_FALSE(aStart >= 0 && aEnd <= aSource.mDuration, "Slice out of range");
    mDuration += aEnd - aStart;
    StreamTime offset = 0;
    for (uint32_t i = 0; i < aSource.mChunks.Length() && offset < aEnd; ++i) {
      const Chunk& c = aSource.mChunks[i];
      StreamTime start = std::max(aStart, offset);
      StreamTime nextOffset = offset + c.GetDuration();
      StreamTime end = std::min(aEnd, nextOffset);
      if (start < end) {
        mChunks.AppendElement(c)->SliceTo(start - offset, end - offset);
      }
      offset = nextOffset;
    }
  }

  Chunk* AppendChunk(StreamTime aDuration)
  {
    MOZ_ASSERT(aDuration >= 0);
    Chunk* c = mChunks.AppendElement();
    c->mDuration = aDuration;
    mDuration += aDuration;
    return c;
  }

  Chunk* FindChunkContaining(StreamTime aOffset, StreamTime* aStart = nullptr)
  {
    if (aOffset < 0) {
      return nullptr;
    }
    StreamTime offset = 0;
    for (uint32_t i = 0; i < mChunks.Length(); ++i) {
      Chunk& c = mChunks[i];
      StreamTime nextOffset = offset + c.GetDuration();
      if (aOffset < nextOffset) {
        if (aStart) {
          *aStart = offset;
        }
        return &c;
      }
      offset = nextOffset;
    }
    return nullptr;
  }

  Chunk* GetLastChunk()
  {
    if (mChunks.IsEmpty()) {
      return nullptr;
    }
    return &mChunks[mChunks.Length() - 1];
  }

  void RemoveLeading(StreamTime aDuration, uint32_t aStartIndex)
  {
    NS_ASSERTION(aDuration >= 0, "Can't remove negative duration");
    StreamTime t = aDuration;
    uint32_t chunksToRemove = 0;
    for (uint32_t i = aStartIndex; i < mChunks.Length() && t > 0; ++i) {
      Chunk* c = &mChunks[i];
      if (c->GetDuration() > t) {
        c->SliceTo(t, c->GetDuration());
        t = 0;
        break;
      }
      t -= c->GetDuration();
      chunksToRemove = i + 1 - aStartIndex;
    }
    mChunks.RemoveElementsAt(aStartIndex, chunksToRemove);
    mDuration -= aDuration - t;
  }

  void RemoveTrailing(StreamTime aKeep, uint32_t aStartIndex)
  {
    NS_ASSERTION(aKeep >= 0, "Can't keep negative duration");
    StreamTime t = aKeep;
    uint32_t i;
    for (i = aStartIndex; i < mChunks.Length(); ++i) {
      Chunk* c = &mChunks[i];
      if (c->GetDuration() > t) {
        c->SliceTo(0, t);
        break;
      }
      t -= c->GetDuration();
      if (t == 0) {
        break;
      }
    }
    if (i+1 < mChunks.Length()) {
      mChunks.RemoveElementsAt(i+1, mChunks.Length() - (i+1));
    }
    
  }

  nsTArray<Chunk> mChunks;
#ifdef MOZILLA_INTERNAL_API
  mozilla::TimeStamp mTimeStamp;
#endif
};

}

#endif 
