




#if !defined(WebMReader_h_)
#define WebMReader_h_

#include <stdint.h>

#include "MediaDecoderReader.h"
#include "nsAutoRef.h"
#include "nestegg/nestegg.h"

#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vpx_codec.h"

#include "mozilla/layers/LayersTypes.h"

namespace mozilla {
static const unsigned NS_PER_USEC = 1000;
static const double NS_PER_S = 1e9;





class NesteggPacketHolder {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NesteggPacketHolder)
  NesteggPacketHolder() : mPacket(nullptr), mOffset(-1), mTimestamp(-1), mIsKeyframe(false) {}

  bool Init(nestegg_packet* aPacket, int64_t aOffset, unsigned aTrack, bool aIsKeyframe)
  {
    uint64_t timestamp_ns;
    if (nestegg_packet_tstamp(aPacket, &timestamp_ns) == -1) {
      return false;
    }

    
    
    mTimestamp = timestamp_ns / 1000;
    mPacket = aPacket;
    mOffset = aOffset;
    mTrack = aTrack;
    mIsKeyframe = aIsKeyframe;

    return true;
  }

  nestegg_packet* Packet() { MOZ_ASSERT(IsInitialized()); return mPacket; }
  int64_t Offset() { MOZ_ASSERT(IsInitialized()); return mOffset; }
  int64_t Timestamp() { MOZ_ASSERT(IsInitialized()); return mTimestamp; }
  unsigned Track() { MOZ_ASSERT(IsInitialized()); return mTrack; }
  bool IsKeyframe() { MOZ_ASSERT(IsInitialized()); return mIsKeyframe; }

private:
  ~NesteggPacketHolder()
  {
    nestegg_free_packet(mPacket);
  }

  bool IsInitialized() { return mOffset >= 0; }

  nestegg_packet* mPacket;

  
  
  int64_t mOffset;

  
  int64_t mTimestamp;

  
  unsigned mTrack;

  
  bool mIsKeyframe;

  
  NesteggPacketHolder(const NesteggPacketHolder &aOther);
  NesteggPacketHolder& operator= (NesteggPacketHolder const& aOther);
};

class WebMBufferedState;


class WebMPacketQueue {
 public:
  int32_t GetSize() {
    return mQueue.size();
  }

  void Push(already_AddRefed<NesteggPacketHolder> aItem) {
    mQueue.push_back(Move(aItem));
  }

  void PushFront(already_AddRefed<NesteggPacketHolder> aItem) {
    mQueue.push_front(Move(aItem));
  }

  already_AddRefed<NesteggPacketHolder> PopFront() {
    nsRefPtr<NesteggPacketHolder> result = mQueue.front().forget();
    mQueue.pop_front();
    return result.forget();
  }

  void Reset() {
    while (!mQueue.empty()) {
      mQueue.pop_front();
    }
  }

private:
  std::deque<nsRefPtr<NesteggPacketHolder>> mQueue;
};

class WebMReader;


class WebMVideoDecoder
{
public:
  virtual nsresult Init(unsigned int aWidth = 0, unsigned int aHeight = 0) = 0;
  virtual nsresult Flush() { return NS_OK; }
  virtual void Shutdown() = 0;
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) = 0;
  WebMVideoDecoder() {}
  virtual ~WebMVideoDecoder() {}
};


class WebMAudioDecoder
{
public:
  virtual nsresult Init() = 0;
  virtual void Shutdown() = 0;
  virtual nsresult ResetDecode() = 0;
  virtual nsresult DecodeHeader(const unsigned char* aData, size_t aLength) = 0;
  virtual nsresult FinishInit(AudioInfo& aInfo) = 0;
  virtual bool Decode(const unsigned char* aData, size_t aLength,
                      int64_t aOffset, uint64_t aTstampUsecs,
                      int64_t aDiscardPadding, int32_t* aTotalFrames) = 0;
  virtual ~WebMAudioDecoder() {}
};

class WebMReader : public MediaDecoderReader
{
public:
  explicit WebMReader(AbstractMediaDecoder* aDecoder, MediaTaskQueue* aBorrowedTaskQueue = nullptr);

protected:
  ~WebMReader();

public:
  virtual nsRefPtr<ShutdownPromise> Shutdown() override;
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) override;
  virtual nsresult ResetDecode() override;
  virtual bool DecodeAudioData() override;

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) override;

  virtual bool HasAudio() override
  {
    MOZ_ASSERT(OnTaskQueue());
    return mHasAudio;
  }

  virtual bool HasVideo() override
  {
    MOZ_ASSERT(OnTaskQueue());
    return mHasVideo;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) override;
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  virtual media::TimeIntervals GetBuffered() override;
  virtual int64_t GetEvictionOffset(double aTime) override;

  virtual bool IsMediaSeekable() override;

  
  
  enum TrackType {
    VIDEO = 0,
    AUDIO = 1
  };

  
  
  
  already_AddRefed<NesteggPacketHolder> NextPacket(TrackType aTrackType);

  
  virtual void PushVideoPacket(already_AddRefed<NesteggPacketHolder> aItem);

  int GetVideoCodec();
  nsIntRect GetPicture();
  nsIntSize GetInitialFrame();
  int64_t GetLastVideoFrameTime();
  void SetLastVideoFrameTime(int64_t aFrameTime);
  layers::LayersBackend GetLayersBackendType() { return mLayersBackendType; }
  FlushableMediaTaskQueue* GetVideoTaskQueue() { return mVideoTaskQueue; }
  uint64_t GetCodecDelay() { return mCodecDelay; }

protected:

  virtual void NotifyDataArrivedInternal(uint32_t aLength, int64_t aOffset) override;

  
  
  
  
  
  
  bool DecodeAudioPacket(NesteggPacketHolder* aHolder);

  
  
  void Cleanup();

  virtual nsresult SeekInternal(int64_t aTime);

  
  void InitLayersBackendType();

  bool ShouldSkipVideoFrame(int64_t aTimeThreshold);

private:
  
  int64_t GetNextKeyframeTime(int64_t aTimeThreshold);
  
  
  bool FilterPacketByTime(int64_t aEndTime, WebMPacketQueue& aOutput);

  
  
  already_AddRefed<NesteggPacketHolder> DemuxPacket();

  
  
  nestegg* mContext;

  nsAutoPtr<WebMAudioDecoder> mAudioDecoder;
  nsAutoPtr<WebMVideoDecoder> mVideoDecoder;

  
  
  WebMPacketQueue mVideoPackets;
  WebMPacketQueue mAudioPackets;

  
  uint32_t mVideoTrack;
  uint32_t mAudioTrack;

  
  int64_t mAudioStartUsec;

  
  uint64_t mAudioFrames;

  
  uint64_t mCodecDelay;

  
  uint64_t mSeekPreroll;

  
  
  int64_t mLastVideoFrameTime;

  
  
  nsRefPtr<WebMBufferedState> mBufferedState;

  
  
  nsIntSize mInitialFrame;

  
  nsIntRect mPicture;

  
  int mAudioCodec;
  
  int mVideoCodec;

  layers::LayersBackend mLayersBackendType;

  
  nsRefPtr<FlushableMediaTaskQueue> mVideoTaskQueue;

  
  bool mHasVideo;
  bool mHasAudio;

};

} 

#endif
