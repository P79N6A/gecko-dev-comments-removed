




#ifndef MOZILLA_STREAMBUFFER_H_
#define MOZILLA_STREAMBUFFER_H_

#include "mozilla/DebugOnly.h"

#include "MediaSegment.h"
#include "nsAutoPtr.h"
#include <algorithm>

namespace mozilla {




typedef MediaTime StreamTime;
const StreamTime STREAM_TIME_MAX = MEDIA_TIME_MAX;





typedef int32_t TrackRate;
const TrackRate TRACK_RATE_MAX = 1 << MEDIA_TIME_FRAC_BITS;






typedef int32_t TrackID;
const TrackID TRACK_NONE = 0;

inline TrackTicks TimeToTicksRoundUp(TrackRate aRate, StreamTime aTime)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aTime && aTime <= STREAM_TIME_MAX, "Bad time");
  return (aTime*aRate + (1 << MEDIA_TIME_FRAC_BITS) - 1) >> MEDIA_TIME_FRAC_BITS;
}

inline TrackTicks TimeToTicksRoundDown(TrackRate aRate, StreamTime aTime)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aTime && aTime <= STREAM_TIME_MAX, "Bad time");
  return (aTime*aRate) >> MEDIA_TIME_FRAC_BITS;
}

inline StreamTime TicksToTimeRoundUp(TrackRate aRate, TrackTicks aTicks)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aTicks && aTicks <= TRACK_TICKS_MAX, "Bad samples");
  return ((aTicks << MEDIA_TIME_FRAC_BITS) + aRate - 1)/aRate;
}

inline StreamTime TicksToTimeRound(TrackRate aRate, TrackTicks aTicks)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aTicks && aTicks <= TRACK_TICKS_MAX, "Bad samples");
  return ((aTicks << MEDIA_TIME_FRAC_BITS) + aRate/2)/aRate;
}

inline StreamTime TicksToTimeRoundDown(TrackRate aRate, TrackTicks aTicks)
{
  NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Bad rate");
  NS_ASSERTION(0 <= aTicks && aTicks <= TRACK_TICKS_MAX, "Bad samples");
  return (aTicks << MEDIA_TIME_FRAC_BITS)/aRate;
}













class StreamBuffer {
public:
  










  class Track {
  public:
    Track(TrackID aID, TrackRate aRate, TrackTicks aStart, MediaSegment* aSegment)
      : mStart(aStart),
        mSegment(aSegment),
        mRate(aRate),
        mID(aID),
        mEnded(false)
    {
      MOZ_COUNT_CTOR(Track);

      NS_ASSERTION(aID > TRACK_NONE, "Bad track ID");
      NS_ASSERTION(0 < aRate && aRate <= TRACK_RATE_MAX, "Invalid rate");
      NS_ASSERTION(0 <= aStart && aStart <= aSegment->GetDuration(), "Bad start position");
    }
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
      return mozilla::TicksToTimeRoundDown(mRate, mSegment->GetDuration());
    }
    StreamTime GetStartTimeRoundDown() const
    {
      return mozilla::TicksToTimeRoundDown(mRate, mStart);
    }
    TrackTicks TimeToTicksRoundDown(StreamTime aTime) const
    {
      return mozilla::TimeToTicksRoundDown(mRate, aTime);
    }
    StreamTime TicksToTimeRoundDown(TrackTicks aTicks) const
    {
      return mozilla::TicksToTimeRoundDown(mRate, aTicks);
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
#ifdef DEBUG
      mForgottenUpTo = std::max<TrackTicks>(mForgottenUpTo, aTime);
#endif
    }
#ifdef DEBUG
    TrackTicks GetForgottenUpTo() { return mForgottenUpTo; }
#endif

  protected:
    friend class StreamBuffer;

    
    TrackTicks mStart;
    
    
    nsAutoPtr<MediaSegment> mSegment;
    TrackRate mRate; 
    
    TrackID mID;
    
    bool mEnded;
    DebugOnly<TrackTicks> mForgottenUpTo;
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
  {
    MOZ_COUNT_CTOR(StreamBuffer);
  }
  ~StreamBuffer()
  {
    MOZ_COUNT_DTOR(StreamBuffer);
  }

  




  Track& AddTrack(TrackID aID, TrackRate aRate, TrackTicks aStart, MediaSegment* aSegment)
  {
    NS_ASSERTION(TimeToTicksRoundDown(aRate, mTracksKnownTime) <= aStart,
                 "Start time too early");
    NS_ASSERTION(!FindTrack(aID), "Track with this ID already exists");

    return **mTracks.InsertElementSorted(new Track(aID, aRate, aStart, aSegment),
                                         CompareTracksByID());
  }
  void AdvanceKnownTracksTime(StreamTime aKnownTime)
  {
    NS_ASSERTION(aKnownTime >= mTracksKnownTime, "Can't move tracks-known time earlier");
    mTracksKnownTime = aKnownTime;
  }

  



  StreamTime GetEnd() const;

  Track* FindTrack(TrackID aID);

  class TrackIter {
  public:
    


    TrackIter(const StreamBuffer& aBuffer) :
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
  
  
  StreamTime mTracksKnownTime;
  StreamTime mForgottenTime;
  
  nsTArray<nsAutoPtr<Track> > mTracks;
};

}

#endif 

