




#if !defined(WebMDemuxer_h_)
#define WebMDemuxer_h_

#include "nsTArray.h"
#include "MediaDataDemuxer.h"

typedef struct nestegg nestegg;

namespace mozilla {

class NesteggPacketHolder;
class WebMBufferedState;
class WebMPacketQueue;


class MediaRawDataQueue {
 public:
  uint32_t GetSize() {
    return mQueue.size();
  }

  void Push(MediaRawData* aItem) {
    mQueue.push_back(aItem);
  }

  void PushFront(MediaRawData* aItem) {
    mQueue.push_front(aItem);
  }

  nsRefPtr<MediaRawData> PopFront() {
    nsRefPtr<MediaRawData> result = mQueue.front();
    mQueue.pop_front();
    return result;
  }

  void Reset() {
    while (!mQueue.empty()) {
      mQueue.pop_front();
    }
  }

private:
  std::deque<nsRefPtr<MediaRawData>> mQueue;
};

class WebMTrackDemuxer;

class WebMDemuxer : public MediaDataDemuxer
{
public:
  explicit WebMDemuxer(MediaResource* aResource);

  nsRefPtr<InitPromise> Init() override;

  already_AddRefed<MediaDataDemuxer> Clone() const override;

  bool HasTrackType(TrackInfo::TrackType aType) const override;

  uint32_t GetNumberTracks(TrackInfo::TrackType aType) const override;

  UniquePtr<TrackInfo> GetTrackInfo(TrackInfo::TrackType aType, size_t aTrackNumber) const;

  already_AddRefed<MediaTrackDemuxer> GetTrackDemuxer(TrackInfo::TrackType aType,
                                                      uint32_t aTrackNumber) override;

  bool IsSeekable() const override;

  UniquePtr<EncryptionInfo> GetCrypto() override;

  bool GetOffsetForTime(uint64_t aTime, int64_t* aOffset);

  
  bool GetNextPacket(TrackInfo::TrackType aType, MediaRawDataQueue *aSamples);

  nsresult Reset();

  
  virtual void PushAudioPacket(NesteggPacketHolder* aItem);

  
  virtual void PushVideoPacket(NesteggPacketHolder* aItem);

  nsresult Read(char* aBuffer, uint32_t aCount, uint32_t * aBytes);
  nsresult Seek(int32_t aWhence, int64_t aOffset);
  int64_t Tell();

private:
  friend class WebMTrackDemuxer;

  ~WebMDemuxer();
  void Cleanup();
  nsresult InitBufferedState();
  nsresult ReadMetadata();
  void NotifyDataArrived(uint32_t aLength, int64_t aOffset) override;
  void NotifyDataRemoved() override;
  void EnsureUpToDateIndex();
  media::TimeIntervals GetBuffered();
  virtual nsresult SeekInternal(const media::TimeUnit& aTarget);
  
  int64_t GetNextKeyframeTime();

  
  
  
  nsRefPtr<NesteggPacketHolder> NextPacket(TrackInfo::TrackType aType);

  
  
  nsRefPtr<NesteggPacketHolder> DemuxPacket();

  nsRefPtr<MediaResource> mResource;
  MediaInfo mInfo;
  nsTArray<nsRefPtr<WebMTrackDemuxer>> mDemuxers;

  
  
  nsRefPtr<WebMBufferedState> mBufferedState;
  nsRefPtr<MediaByteBuffer> mInitData;

  
  
  
  nestegg* mContext;
  int64_t mOffset;

  
  WebMPacketQueue mVideoPackets;
  WebMPacketQueue mAudioPackets;

  
  uint32_t mVideoTrack;
  uint32_t mAudioTrack;

  
  uint64_t mCodecDelay;

  
  uint64_t mSeekPreroll;

  int64_t mLastAudioFrameTime;

  
  
  int64_t mLastVideoFrameTime;

  
  nsIntRect mPicture;

  
  int mAudioCodec;
  
  int mVideoCodec;

  
  bool mHasVideo;
  bool mHasAudio;
  bool mNeedReIndex;
};

class WebMTrackDemuxer : public MediaTrackDemuxer
{
public:
  WebMTrackDemuxer(WebMDemuxer* aParent,
                  TrackInfo::TrackType aType,
                  uint32_t aTrackNumber);

  UniquePtr<TrackInfo> GetInfo() const override;

  nsRefPtr<SeekPromise> Seek(media::TimeUnit aTime) override;

  nsRefPtr<SamplesPromise> GetSamples(int32_t aNumSamples = 1) override;

  void Reset() override;

  nsresult GetNextRandomAccessPoint(media::TimeUnit* aTime) override;

  nsRefPtr<SkipAccessPointPromise> SkipToNextRandomAccessPoint(media::TimeUnit aTimeThreshold) override;

  media::TimeIntervals GetBuffered() override;

  int64_t GetEvictionOffset(media::TimeUnit aTime) override;

  void BreakCycles() override;

private:
  friend class WebMDemuxer;
  ~WebMTrackDemuxer();
  void UpdateSamples(nsTArray<nsRefPtr<MediaRawData>>& aSamples);
  void SetNextKeyFrameTime();
  nsRefPtr<MediaRawData> NextSample ();
  nsRefPtr<WebMDemuxer> mParent;
  TrackInfo::TrackType mType;
  UniquePtr<TrackInfo> mInfo;
  Maybe<media::TimeUnit> mNextKeyframeTime;

  
  MediaRawDataQueue mSamples;
};

} 

#endif
