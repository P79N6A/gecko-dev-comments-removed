




#if !defined(WebMReader_h_)
#define WebMReader_h_

#include <stdint.h>

#include "nsDeque.h"
#include "MediaDecoderReader.h"
#include "nsAutoRef.h"
#include "nestegg/nestegg.h"

#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vpx_codec.h"

#include "mozilla/layers/LayersTypes.h"

#ifdef MOZ_TREMOR
#include "tremor/ivorbiscodec.h"
#else
#include "vorbis/codec.h"
#endif

#include "OpusParser.h"





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

template <>
class nsAutoRefTraits<NesteggPacketHolder> : public nsPointerRefTraits<NesteggPacketHolder>
{
public:
  static void Release(NesteggPacketHolder* aHolder) { delete aHolder; }
};

namespace mozilla {
class WebMBufferedState;
static const unsigned NS_PER_USEC = 1000;
static const double NS_PER_S = 1e9;


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

class WebMReader : public MediaDecoderReader
{
public:
  explicit WebMReader(AbstractMediaDecoder* aDecoder);

protected:
  ~WebMReader();

public:
  virtual nsRefPtr<ShutdownPromise> Shutdown() MOZ_OVERRIDE;
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;
  virtual nsresult ResetDecode() MOZ_OVERRIDE;
  virtual bool DecodeAudioData() MOZ_OVERRIDE;

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual bool HasAudio() MOZ_OVERRIDE
  {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return mHasAudio;
  }

  virtual bool HasVideo() MOZ_OVERRIDE
  {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return mHasVideo;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) MOZ_OVERRIDE;

  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered) MOZ_OVERRIDE;
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength,
                                 int64_t aOffset) MOZ_OVERRIDE;
  virtual int64_t GetEvictionOffset(double aTime) MOZ_OVERRIDE;

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

  
  
  enum TrackType {
    VIDEO = 0,
    AUDIO = 1
  };

  
  
  
  nsReturnRef<NesteggPacketHolder> NextPacket(TrackType aTrackType);

  
  virtual void PushVideoPacket(NesteggPacketHolder* aItem);

  int GetVideoCodec();
  nsIntRect GetPicture();
  nsIntSize GetInitialFrame();
  uint64_t GetLastVideoFrameTime();
  void SetLastVideoFrameTime(uint64_t aFrameTime);
  layers::LayersBackend GetLayersBackendType() { return mLayersBackendType; }
  FlushableMediaTaskQueue* GetVideoTaskQueue() { return mVideoTaskQueue; }

protected:
  
  bool InitOpusDecoder();

  
  
  
  
  
  
  bool DecodeAudioPacket(nestegg_packet* aPacket, int64_t aOffset);
  bool DecodeVorbis(const unsigned char* aData, size_t aLength,
                    int64_t aOffset, uint64_t aTstampUsecs,
                    int32_t* aTotalFrames);
  bool DecodeOpus(const unsigned char* aData, size_t aLength,
                  int64_t aOffset, uint64_t aTstampUsecs,
                  nestegg_packet* aPacket);

  
  
  void Cleanup();

  virtual nsresult SeekInternal(int64_t aTime);

  
  void InitLayersBackendType();

private:
  
  
  nestegg* mContext;

  
  nsAutoPtr<WebMVideoDecoder> mVideoDecoder;

  
  vorbis_info mVorbisInfo;
  vorbis_comment mVorbisComment;
  vorbis_dsp_state mVorbisDsp;
  vorbis_block mVorbisBlock;
  int64_t mPacketCount;

  
  nsAutoPtr<OpusParser> mOpusParser;
  OpusMSDecoder *mOpusDecoder;
  uint16_t mSkip;        
  uint64_t mSeekPreroll; 

  
  
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

  layers::LayersBackend mLayersBackendType;

  
  nsRefPtr<FlushableMediaTaskQueue> mVideoTaskQueue;

  
  bool mHasVideo;
  bool mHasAudio;

  
  
  
  bool mPaddingDiscarded;
};

} 

#endif
