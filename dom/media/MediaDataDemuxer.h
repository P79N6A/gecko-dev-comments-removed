





#if !defined(MediaDataDemuxer_h)
#define MediaDataDemuxer_h

#include "MediaData.h"
#include "MediaInfo.h"
#include "MediaPromise.h"
#include "TimeUnits.h"
#include "mozilla/UniquePtr.h"
#include "nsISupportsImpl.h"
#include "nsRefPtr.h"
#include "nsTArray.h"

namespace mozilla {

class MediaTrackDemuxer;
class TrackMetadataHolder;

enum class DemuxerFailureReason : int8_t
{
  WAITING_FOR_DATA,
  END_OF_STREAM,
  DEMUXER_ERROR,
  CANCELED,
  SHUTDOWN,
};






class MediaDataDemuxer
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDataDemuxer)

  typedef MediaPromise<nsresult, DemuxerFailureReason,  true> InitPromise;

  
  
  
  
  
  
  virtual nsRefPtr<InitPromise> Init() = 0;

  
  
  
  
  
  
  virtual bool IsThreadSafe() { return false; }

  
  
  
  virtual already_AddRefed<MediaDataDemuxer> Clone() const = 0;

  
  virtual bool HasTrackType(TrackInfo::TrackType aType) const = 0;

  
  
  virtual uint32_t GetNumberTracks(TrackInfo::TrackType aType) const = 0;

  
  
  
  
  
  virtual already_AddRefed<MediaTrackDemuxer> GetTrackDemuxer(TrackInfo::TrackType aType,
                                                              uint32_t aTrackNumber) = 0;

  
  virtual bool IsSeekable() const = 0;

  
  
  virtual UniquePtr<EncryptionInfo> GetCrypto()
  {
    return nullptr;
  }

  
  
  
  virtual void NotifyDataArrived(uint32_t aLength, int64_t aOffset) { }

  
  
  
  
  virtual void NotifyDataRemoved() { }

  
  
  
  virtual bool ShouldComputeStartTime() const { return true; }

protected:
  virtual ~MediaDataDemuxer()
  {
  }
};

class MediaTrackDemuxer
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaTrackDemuxer)

  class SamplesHolder {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SamplesHolder)
    nsTArray<nsRefPtr<MediaRawData>> mSamples;
  private:
    ~SamplesHolder() {}
  };

  class SkipFailureHolder {
  public:
    SkipFailureHolder(DemuxerFailureReason aFailure, uint32_t aSkipped)
      : mFailure(aFailure)
      , mSkipped(aSkipped)
    {}
    DemuxerFailureReason mFailure;
    uint32_t mSkipped;
  };

  typedef MediaPromise<media::TimeUnit, DemuxerFailureReason,  true> SeekPromise;
  typedef MediaPromise<nsRefPtr<SamplesHolder>, DemuxerFailureReason,  true> SamplesPromise;
  typedef MediaPromise<uint32_t, SkipFailureHolder,  true> SkipAccessPointPromise;

  
  
  
  
  
  virtual UniquePtr<TrackInfo> GetInfo() const = 0;

  
  
  virtual nsRefPtr<SeekPromise> Seek(media::TimeUnit aTime) = 0;

  
  
  
  
  
  
  virtual nsRefPtr<SamplesPromise> GetSamples(int32_t aNumSamples = 1) = 0;

  
  
  
  
  virtual bool GetSamplesMayBlock() const
  {
    return true;
  }

  
  
  
  
  virtual void Reset() = 0;

  
  
  virtual nsresult GetNextRandomAccessPoint(media::TimeUnit* aTime)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  virtual nsresult GetPreviousRandomAccessPoint(media::TimeUnit* aTime)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  
  
  virtual nsRefPtr<SkipAccessPointPromise> SkipToNextRandomAccessPoint(media::TimeUnit aTimeThreshold) = 0;

  
  
  virtual int64_t GetResourceOffset() const
  {
    return -1;
  }

  virtual TrackInfo::TrackType GetType() const
  {
    return GetInfo()->GetType();
  }

  virtual media::TimeIntervals GetBuffered() = 0;

  virtual int64_t GetEvictionOffset(media::TimeUnit aTime) = 0;

  
  
  virtual void BreakCycles()
  {
  }

protected:
  virtual ~MediaTrackDemuxer() {}
};

} 

#endif
