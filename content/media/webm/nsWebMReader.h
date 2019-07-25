





































#if !defined(nsWebMReader_h_)
#define nsWebMReader_h_

#include "nsDeque.h"
#include "nsBuiltinDecoderReader.h"
#include "nsWebMBufferedParser.h"
#include "nsAutoRef.h"
#include "nestegg/nestegg.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#ifdef MOZ_TREMOR
#include "tremor/ivorbiscodec.h"
#else
#include "vorbis/codec.h"
#endif

class nsMediaDecoder;





class NesteggPacketHolder {
public:
  NesteggPacketHolder(nestegg_packet* aPacket, PRInt64 aOffset)
    : mPacket(aPacket), mOffset(aOffset)
  {
    MOZ_COUNT_CTOR(NesteggPacketHolder);
  }
  ~NesteggPacketHolder() {
    MOZ_COUNT_DTOR(NesteggPacketHolder);
    nestegg_free_packet(mPacket);
  }
  nestegg_packet* mPacket;
  
  
  PRInt64 mOffset;
private:
  
  NesteggPacketHolder(const NesteggPacketHolder &aOther);
  NesteggPacketHolder& operator= (NesteggPacketHolder const& aOther);
};


class PacketQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* anObject) {
    delete static_cast<NesteggPacketHolder*>(anObject);
    return nsnull;
  }
};




class PacketQueue : private nsDeque {
 public:
   PacketQueue()
     : nsDeque(new PacketQueueDeallocator())
   {}
  
  ~PacketQueue() {
    Reset();
  }

  inline PRInt32 GetSize() { 
    return nsDeque::GetSize();
  }
  
  inline void Push(NesteggPacketHolder* aItem) {
    NS_ASSERTION(aItem, "NULL pushed to PacketQueue");
    nsDeque::Push(aItem);
  }
  
  inline void PushFront(NesteggPacketHolder* aItem) {
    NS_ASSERTION(aItem, "NULL pushed to PacketQueue");
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

class nsWebMReader : public nsBuiltinDecoderReader
{
public:
  nsWebMReader(nsBuiltinDecoder* aDecoder);
  ~nsWebMReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual PRBool DecodeAudioData();

  
  
  
  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold);

  virtual PRBool HasAudio()
  {
    mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mHasAudio;
  }

  virtual PRBool HasVideo()
  {
    mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mHasVideo;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);
  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset);

private:
  
  
  enum TrackType {
    VIDEO = 0,
    AUDIO = 1
  };

  
  
  
  nsReturnRef<NesteggPacketHolder> NextPacket(TrackType aTrackType);

  
  ogg_packet InitOggPacket(unsigned char* aData,
                           size_t aLength,
                           PRBool aBOS,
                           PRBool aEOS,
                           PRInt64 aGranulepos);
                     
  
  
  
  
  
  
  PRBool DecodeAudioPacket(nestegg_packet* aPacket, PRInt64 aOffset);

  
  
  void Cleanup();

  
  
  
  
  PRBool CanDecodeToTarget(PRInt64 aTarget, PRInt64 aCurrentTime);

private:
  
  
  nestegg* mContext;

  
  vpx_codec_ctx_t  mVP8;

  
  vorbis_info mVorbisInfo;
  vorbis_comment mVorbisComment;
  vorbis_dsp_state mVorbisDsp;
  vorbis_block mVorbisBlock;
  PRUint32 mPacketCount;
  PRUint32 mChannels;

  
  
  PacketQueue mVideoPackets;
  PacketQueue mAudioPackets;

  
  PRUint32 mVideoTrack;
  PRUint32 mAudioTrack;

  
  PRInt64 mAudioStartUsec;

  
  PRUint64 mAudioSamples;

  
  
  nsRefPtr<nsWebMBufferedState> mBufferedState;

  
  
  nsIntSize mInitialFrame;

  
  nsIntRect mPicture;

  
  PRPackedBool mHasVideo;
  PRPackedBool mHasAudio;
};

#endif
