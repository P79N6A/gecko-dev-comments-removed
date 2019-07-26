




#if !defined(WebMReader_h_)
#define WebMReader_h_

#include "mozilla/StandardInteger.h"

#include "nsDeque.h"
#include "MediaDecoderReader.h"
#include "nsAutoRef.h"
#include "nestegg/nestegg.h"

#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vpx_codec.h"

#ifdef MOZ_TREMOR
#include "tremor/ivorbiscodec.h"
#else
#include "vorbis/codec.h"
#endif

#ifdef MOZ_DASH
#include "DASHRepReader.h"
#endif

namespace mozilla {

class WebMBufferedState;





class NesteggPacketHolder {
public:
  NesteggPacketHolder(nestegg_packet* aPacket, int64_t aOffset)
    : mPacket(aPacket), mOffset(aOffset)
  {
    MOZ_COUNT_CTOR(NesteggPacketHolder);
  }
  ~NesteggPacketHolder() {
    MOZ_COUNT_DTOR(NesteggPacketHolder);
    nestegg_free_packet(mPacket);
  }
  nestegg_packet* mPacket;
  
  
  int64_t mOffset;
private:
  
  NesteggPacketHolder(const NesteggPacketHolder &aOther);
  NesteggPacketHolder& operator= (NesteggPacketHolder const& aOther);
};


class PacketQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* anObject) {
    delete static_cast<NesteggPacketHolder*>(anObject);
    return nullptr;
  }
};




class WebMPacketQueue : private nsDeque {
 public:
   WebMPacketQueue()
     : nsDeque(new PacketQueueDeallocator())
   {}
  
  ~WebMPacketQueue() {
    Reset();
  }

  inline int32_t GetSize() { 
    return nsDeque::GetSize();
  }
  
  inline void Push(NesteggPacketHolder* aItem) {
    NS_ASSERTION(aItem, "NULL pushed to WebMPacketQueue");
    nsDeque::Push(aItem);
  }
  
  inline void PushFront(NesteggPacketHolder* aItem) {
    NS_ASSERTION(aItem, "NULL pushed to WebMPacketQueue");
    nsDeque::PushFront(aItem);
  }

  inline NesteggPacketHolder* PopFront() {
    return static_cast<NesteggPacketHolder*>(nsDeque::PopFront());
  }
  
  void Reset() {
    while (GetSize() > 0) {
      delete PopFront();
    }
  }
};

#ifdef MOZ_DASH
class WebMReader : public DASHRepReader
#else
class WebMReader : public MediaDecoderReader
#endif
{
public:
  WebMReader(AbstractMediaDecoder* aDecoder);
  ~WebMReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  int64_t aTimeThreshold);

  virtual bool HasAudio()
  {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return mHasAudio;
  }

  virtual bool HasVideo()
  {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return mHasVideo;
  }

  virtual nsresult ReadMetadata(VideoInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(TimeRanges* aBuffered, int64_t aStartTime);
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

#ifdef MOZ_DASH
  virtual void SetMainReader(DASHReader *aMainReader) MOZ_OVERRIDE {
    NS_ASSERTION(aMainReader, "aMainReader is null.");
    mMainReader = aMainReader;
  }

  
  
  
  
  
  void PrepareToDecode() MOZ_OVERRIDE;

  
  
  
  MediaQueue<AudioData>& AudioQueue() MOZ_OVERRIDE {
    if (mMainReader) {
      return mMainReader->AudioQueue();
    } else {
      return MediaDecoderReader::AudioQueue();
    }
  }

  MediaQueue<VideoData>& VideoQueue() MOZ_OVERRIDE {
    if (mMainReader) {
      return mMainReader->VideoQueue();
    } else {
      return MediaDecoderReader::VideoQueue();
    }
  }

  
  void SetInitByteRange(MediaByteRange &aByteRange) MOZ_OVERRIDE {
    mInitByteRange = aByteRange;
  }

  
  void SetIndexByteRange(MediaByteRange &aByteRange) MOZ_OVERRIDE {
    mCuesByteRange = aByteRange;
  }

  
  int64_t GetSubsegmentForSeekTime(int64_t aSeekToTime) MOZ_OVERRIDE;

  
  nsresult GetSubsegmentByteRanges(nsTArray<MediaByteRange>& aByteRanges)
                                                                  MOZ_OVERRIDE;

  
  
  
  bool HasReachedSubsegment(uint32_t aSubsegmentIndex) MOZ_OVERRIDE;

  
  
  
  
  void RequestSeekToSubsegment(uint32_t aIdx) MOZ_OVERRIDE;

  
  
  
  void RequestSwitchAtSubsegment(int32_t aSubsegmentIdx,
                                 MediaDecoderReader* aNextReader) MOZ_OVERRIDE;

  
  
  void SeekToCluster(uint32_t aIdx);

  
  bool IsDataCachedAtEndOfSubsegments() MOZ_OVERRIDE;
#endif

protected:
  
  
  enum TrackType {
    VIDEO = 0,
    AUDIO = 1
  };

  
  
  
#ifdef MOZ_DASH
  nsReturnRef<NesteggPacketHolder> NextPacketInternal(TrackType aTrackType);

  
  
  
  
#endif
  nsReturnRef<NesteggPacketHolder> NextPacket(TrackType aTrackType);

  
  virtual void PushVideoPacket(NesteggPacketHolder* aItem);

  
  ogg_packet InitOggPacket(unsigned char* aData,
                           size_t aLength,
                           bool aBOS,
                           bool aEOS,
                           int64_t aGranulepos);

  
  
  
  
  
  
  bool DecodeAudioPacket(nestegg_packet* aPacket, int64_t aOffset);

  
  
  void Cleanup();

private:
  
  
  nestegg* mContext;

  
  vpx_codec_ctx_t mVP8;

  
  vorbis_info mVorbisInfo;
  vorbis_comment mVorbisComment;
  vorbis_dsp_state mVorbisDsp;
  vorbis_block mVorbisBlock;
  uint32_t mPacketCount;
  uint32_t mChannels;

  
  
  WebMPacketQueue mVideoPackets;
  WebMPacketQueue mAudioPackets;

  
  uint32_t mVideoTrack;
  uint32_t mAudioTrack;

  
  int64_t mAudioStartUsec;

  
  uint64_t mAudioFrames;

  
  
  nsRefPtr<WebMBufferedState> mBufferedState;

  
  
  nsIntSize mInitialFrame;

  
  nsIntRect mPicture;

  
  bool mHasVideo;
  bool mHasAudio;

#ifdef MOZ_DASH
  
  MediaByteRange mInitByteRange;

  
  MediaByteRange mCuesByteRange;

  
  nsTArray<TimestampedMediaByteRange> mClusterByteRanges;

  
  DASHReader* mMainReader;

  
  
  int32_t mSwitchingCluster;

  
  
  
  nsRefPtr<WebMReader> mNextReader;

  
  
  
  int32_t mSeekToCluster;

  
  
  
  int64_t mCurrentOffset;

  
  
  
  
  bool mPushVideoPacketToNextReader;

  
  
  
  bool mReachedSwitchAccessPoint;
#endif
};

} 

#endif
