




#ifndef MOZILLA_STREAMBUFFER_H_
#define MOZILLA_STREAMBUFFER_H_

#include "MediaSegment.h"
#include "nsAutoPtr.h"

namespace mozilla {




typedef MediaTime StreamTime;
const StreamTime STREAM_TIME_MAX = MEDIA_TIME_MAX;






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

inline TrackTicks SecondsToTicksRoundDown(TrackRate aRate, double aSeconds)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aSeconds && aSeconds <= TRACK_TICKS_MAX/TRACK_RATE_MAX,
               "Bad seconds");
  return aSeconds * aRate;
}
inline double TrackTicksToSeconds(TrackRate aRate, TrackTicks aTicks)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aTicks && aTicks <= TRACK_TICKS_MAX, "Bad ticks");
  return static_cast<double>(aTicks)/aRate;
}













class StreamBuffer {
public:
  










  class Track {
    Track(TrackID aID, TrackRate aRate, TrackTicks aStart, MediaSegment* aSegment, TrackRate aGraphRate)
      : mStart(aStart),
        mSegment(aSegment),
        mRate(aRate),
        mGraphRate(aGraphRate),
        mID(aID),
        mEnded(false)
    {
      MOZ_COUNT_CTOR(Track);

      NS_ASSERTION(aID > TRACK_NONE, "Bad track ID");
      NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Invalid rate");
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
    TrackRate GetRate() const { return mRate; }
    TrackID GetID() const { return mID; }
    bool IsEnded() const { return mEnded; }
    TrackTicks GetStart() const { return mStart; }
    TrackTicks GetEnd() const { return mSegment->GetDuration(); }
    StreamTime GetEndTimeRoundDown() const
    {
      return TicksToTimeRoundDown(mSegment->GetDuration());
    }
    StreamTime GetStartTimeRoundDown() const
    {
      return TicksToTimeRoundDown(mStart);
    }
    TrackTicks TimeToTicksRoundDown(StreamTime aTime) const
    {
      return RateConvertTicksRoundDown(mRate, mGraphRate, aTime);
    }
    StreamTime TicksToTimeRoundDown(TrackTicks aTicks) const
    {
      return RateConvertTicksRoundDown(mGraphRate, mRate, aTicks);
    }
    MediaSegment::Type GetType() const { return mSegment->GetType(); }

    void SetEnded() { mEnded = true; }
    void AppendFrom(Track* aTrack)
    {
      NS_ASSERTION(!mEnded, "Can't append to ended track");
      NS_ASSERTION(aTrack->mID == mID, "IDs must match");
      NS_ASSERTION(aTrack->mStart == 0, "Source track must start at zero");
      NS_ASSERTION(aTrack->mSegment->GetType() == GetType(), "Track types must match");
      NS_ASSERTION(aTrack->mRate == mRate, "Track rates must match");

      mSegment->AppendFrom(aTrack->mSegment);
      mEnded = aTrack->mEnded;
    }
    MediaSegment* RemoveSegment()
    {
      return mSegment.forget();
    }
    void ForgetUpTo(TrackTicks aTime)
    {
      mSegment->ForgetUpTo(aTime);
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

    
    TrackTicks mStart;
    
    
    nsAutoPtr<MediaSegment> mSegment;
    TrackRate mRate; 
    TrackRate mGraphRate; 
    
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

  




  Track& AddTrack(TrackID aID, TrackRate aRate, TrackTicks aStart, MediaSegment* aSegment)
  {
    NS_ASSERTION(!FindTrack(aID), "Track with this ID already exists");

    Track* track = new Track(aID, aRate, aStart, aSegment, GraphRate());
    mTracks.InsertElementSorted(track, CompareTracksByID());

    if (mTracksKnownTime == STREAM_TIME_MAX) {
      
      
      NS_WARNING("Adding track to StreamBuffer that should have no more tracks");
    } else {
      NS_ASSERTION(track->TimeToTicksRoundDown(mTracksKnownTime) <= aStart,
                   "Start time too early");
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

