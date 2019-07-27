




#ifndef MOZILLA_STREAMBUFFER_H_
#define MOZILLA_STREAMBUFFER_H_

#include "MediaSegment.h"
#include "nsAutoPtr.h"

namespace mozilla {






typedef int32_t TrackID;
const TrackID TRACK_NONE = 0;
const TrackID TRACK_INVALID = -1;

inline TrackTicks RateConvertTicksRoundDown(TrackRate aOutRate,
                                            TrackRate aInRate,
                                            TrackTicks aTicks)
{
  NS_ASSERTION(0 < aOutRate && aOutRate <= TRACK_RATE_MAX, "Bad out rate");
  NS_ASSERTION(0 < aInRate && aInRate <= TRACK_RATE_MAX, "Bad in rate");
  NS_WARN_IF_FALSE(0 <= aTicks && aTicks <= TRACK_TICKS_MAX, "Bad ticks"); 
  return (aTicks * aOutRate) / aInRate;
}
inline TrackTicks RateConvertTicksRoundUp(TrackRate aOutRate,
                                          TrackRate aInRate, TrackTicks aTicks)
{
  NS_ASSERTION(0 < aOutRate && aOutRate <= TRACK_RATE_MAX, "Bad out rate");
  NS_ASSERTION(0 < aInRate && aInRate <= TRACK_RATE_MAX, "Bad in rate");
  NS_ASSERTION(0 <= aTicks && aTicks <= TRACK_TICKS_MAX, "Bad ticks");
  return (aTicks * aOutRate + aInRate - 1) / aInRate;
}













class StreamBuffer {
public:
  










  class Track {
    Track(TrackID aID, StreamTime aStart, MediaSegment* aSegment)
      : mStart(aStart),
        mSegment(aSegment),
        mID(aID),
        mEnded(false)
    {
      MOZ_COUNT_CTOR(Track);

      NS_ASSERTION(aID > TRACK_NONE, "Bad track ID");
      NS_ASSERTION(0 <= aStart && aStart <= aSegment->GetDuration(), "Bad start position");
    }
  public:
    ~Track()
    {
      MOZ_COUNT_DTOR(Track);
    }
    template <class T> T* Get() const
    {
      if (mSegment->GetType() == T::StaticType()) {
        return static_cast<T*>(mSegment.get());
      }
      return nullptr;
    }
    MediaSegment* GetSegment() const { return mSegment; }
    TrackID GetID() const { return mID; }
    bool IsEnded() const { return mEnded; }
    StreamTime GetStart() const { return mStart; }
    StreamTime GetEnd() const { return mSegment->GetDuration(); }
    MediaSegment::Type GetType() const { return mSegment->GetType(); }

    void SetEnded() { mEnded = true; }
    void AppendFrom(Track* aTrack)
    {
      NS_ASSERTION(!mEnded, "Can't append to ended track");
      NS_ASSERTION(aTrack->mID == mID, "IDs must match");
      NS_ASSERTION(aTrack->mStart == 0, "Source track must start at zero");
      NS_ASSERTION(aTrack->mSegment->GetType() == GetType(), "Track types must match");

      mSegment->AppendFrom(aTrack->mSegment);
      mEnded = aTrack->mEnded;
    }
    MediaSegment* RemoveSegment()
    {
      return mSegment.forget();
    }
    void ForgetUpTo(StreamTime aTime)
    {
      mSegment->ForgetUpTo(aTime);
    }
    void FlushAfter(StreamTime aNewEnd)
    {
      
      
      mSegment->FlushAfter(aNewEnd);
    }

    size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
    {
      size_t amount = aMallocSizeOf(this);
      if (mSegment) {
        amount += mSegment->SizeOfIncludingThis(aMallocSizeOf);
      }
      return amount;
    }

  protected:
    friend class StreamBuffer;

    
    StreamTime mStart;
    
    
    nsAutoPtr<MediaSegment> mSegment;
    
    TrackID mID;
    
    bool mEnded;
  };

  class CompareTracksByID {
  public:
    bool Equals(Track* aA, Track* aB) const {
      return aA->GetID() == aB->GetID();
    }
    bool LessThan(Track* aA, Track* aB) const {
      return aA->GetID() < aB->GetID();
    }
  };

  StreamBuffer()
    : mTracksKnownTime(0), mForgottenTime(0)
#ifdef DEBUG
    , mGraphRateIsSet(false)
#endif
  {
    MOZ_COUNT_CTOR(StreamBuffer);
  }
  ~StreamBuffer()
  {
    MOZ_COUNT_DTOR(StreamBuffer);
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t amount = 0;
    amount += mTracks.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < mTracks.Length(); i++) {
      amount += mTracks[i]->SizeOfIncludingThis(aMallocSizeOf);
    }
    return amount;
  }

  



  void InitGraphRate(TrackRate aGraphRate)
  {
    mGraphRate = aGraphRate;
#if DEBUG
    MOZ_ASSERT(!mGraphRateIsSet);
    mGraphRateIsSet = true;
#endif
  }

  TrackRate GraphRate() const
  {
    MOZ_ASSERT(mGraphRateIsSet);
    return mGraphRate;
  }

  




  Track& AddTrack(TrackID aID, StreamTime aStart, MediaSegment* aSegment)
  {
    NS_ASSERTION(!FindTrack(aID), "Track with this ID already exists");

    Track* track = new Track(aID, aStart, aSegment);
    mTracks.InsertElementSorted(track, CompareTracksByID());

    if (mTracksKnownTime == STREAM_TIME_MAX) {
      
      
      NS_WARNING("Adding track to StreamBuffer that should have no more tracks");
    } else {
      NS_ASSERTION(mTracksKnownTime <= aStart, "Start time too early");
    }
    return *track;
  }
  void AdvanceKnownTracksTime(StreamTime aKnownTime)
  {
    NS_ASSERTION(aKnownTime >= mTracksKnownTime, "Can't move tracks-known time earlier");
    mTracksKnownTime = aKnownTime;
  }

  



  StreamTime GetEnd() const;

  




  StreamTime GetAllTracksEnd() const;

#ifdef DEBUG
  void DumpTrackInfo() const;
#endif

  Track* FindTrack(TrackID aID);

  class TrackIter {
  public:
    


    explicit TrackIter(const StreamBuffer& aBuffer) :
      mBuffer(&aBuffer.mTracks), mIndex(0), mMatchType(false) {}
    


    TrackIter(const StreamBuffer& aBuffer, MediaSegment::Type aType) :
      mBuffer(&aBuffer.mTracks), mIndex(0), mType(aType), mMatchType(true) { FindMatch(); }
    bool IsEnded() { return mIndex >= mBuffer->Length(); }
    void Next()
    {
      ++mIndex;
      FindMatch();
    }
    Track* get() { return mBuffer->ElementAt(mIndex); }
    Track& operator*() { return *mBuffer->ElementAt(mIndex); }
    Track* operator->() { return mBuffer->ElementAt(mIndex); }
  private:
    void FindMatch()
    {
      if (!mMatchType)
        return;
      while (mIndex < mBuffer->Length() &&
             mBuffer->ElementAt(mIndex)->GetType() != mType) {
        ++mIndex;
      }
    }

    const nsTArray<nsAutoPtr<Track> >* mBuffer;
    uint32_t mIndex;
    MediaSegment::Type mType;
    bool mMatchType;
  };
  friend class TrackIter;

  




  void ForgetUpTo(StreamTime aTime);
  


  StreamTime GetForgottenDuration()
  {
    return mForgottenTime;
  }

protected:
  TrackRate mGraphRate; 
  
  
  StreamTime mTracksKnownTime;
  StreamTime mForgottenTime;
  
  nsTArray<nsAutoPtr<Track> > mTracks;
#ifdef DEBUG
  bool mGraphRateIsSet;
#endif
};

}

#endif 

