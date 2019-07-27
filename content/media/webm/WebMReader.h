




#if !defined(WebMReader_h_)
#define WebMReader_h_

#include <stdint.h>

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

#ifdef MOZ_OPUS
#include "OpusParser.h"
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
  virtual void* operator() (void* aObject) {
    delete static_cast<NesteggPacketHolder*>(aObject);
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

class WebMReader : public MediaDecoderReader
{
public:
  explicit WebMReader(AbstractMediaDecoder* aDecoder);

protected:
  ~WebMReader();

public:
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

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered, int64_t aStartTime);
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);
  virtual int64_t GetEvictionOffset(double aTime);

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

protected:
  
  
  enum TrackType {
    VIDEO = 0,
    AUDIO = 1
  };

  
  
  
  nsReturnRef<NesteggPacketHolder> NextPacket(TrackType aTrackType);

  
  virtual void PushVideoPacket(NesteggPacketHolder* aItem);

  
  ogg_packet InitOggPacket(unsigned char* aData,
                           size_t aLength,
                           bool aBOS,
                           bool aEOS,
                           int64_t aGranulepos);

#ifdef MOZ_OPUS
  
  bool InitOpusDecoder();
#endif

  
  
  
  
  
  
  bool DecodeAudioPacket(nestegg_packet* aPacket, int64_t aOffset);

  
  
  void Cleanup();

private:
  
  
  nestegg* mContext;

  
  vpx_codec_ctx_t mVPX;

  
  vorbis_info mVorbisInfo;
  vorbis_comment mVorbisComment;
  vorbis_dsp_state mVorbisDsp;
  vorbis_block mVorbisBlock;
  uint32_t mPacketCount;
  uint32_t mChannels;


#ifdef MOZ_OPUS
  
  nsAutoPtr<OpusParser> mOpusParser;
  OpusMSDecoder *mOpusDecoder;
  int mSkip;        
  uint64_t mSeekPreroll; 
#endif

  
  
  WebMPacketQueue mVideoPackets;
  WebMPacketQueue mAudioPackets;

  
  uint32_t mVideoTrack;
  uint32_t mAudioTrack;

  
  int64_t mAudioStartUsec;

  
  uint64_t mAudioFrames;

  
  uint64_t mCodecDelay;

  
  
  uint64_t mLastVideoFrameTime;

  
  
  nsRefPtr<WebMBufferedState> mBufferedState;

  
  
  nsIntSize mInitialFrame;

  
  nsIntRect mPicture;

  
  int mAudioCodec;
  
  int mVideoCodec;

  
  bool mHasVideo;
  bool mHasAudio;
};

} 

#endif
