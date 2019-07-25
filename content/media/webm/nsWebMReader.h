





































#if !defined(nsWebMReader_h_)
#define nsWebMReader_h_

#include "nsDeque.h"
#include "nsBuiltinDecoderReader.h"
#include "nestegg/nestegg.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "vorbis/codec.h"

class nsMediaDecoder;


class PacketQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* anObject) {
    nestegg_free_packet(static_cast<nestegg_packet*>(anObject));
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
  
  inline void Push(nestegg_packet* aItem) {
    NS_ASSERTION(aItem, "NULL pushed to PacketQueue");
    nsDeque::Push(aItem);
  }
  
  inline void PushFront(nestegg_packet* aItem) {
    NS_ASSERTION(aItem, "NULL pushed to PacketQueue");
    nsDeque::PushFront(aItem);
  }

  inline nestegg_packet* PopFront() {
    return static_cast<nestegg_packet*>(nsDeque::PopFront());
  }
  
  void Reset() {
    while (GetSize() > 0) {
      nestegg_free_packet(PopFront());
    }
  }
};


class nsWebMReader : public nsBuiltinDecoderReader
{
public:
  nsWebMReader(nsBuiltinDecoder* aDecoder);
  ~nsWebMReader();

  virtual nsresult Init();
  virtual nsresult ResetDecode();
  virtual PRBool DecodeAudioData();

  
  
  
  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold);

  virtual PRBool HasAudio()
  {
    mozilla::MonitorAutoEnter mon(mMonitor);
    return mHasAudio;
  }

  virtual PRBool HasVideo()
  {
    mozilla::MonitorAutoEnter mon(mMonitor);
    return mHasVideo;
  }

  virtual nsresult ReadMetadata();
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime);
  virtual nsresult GetBuffered(nsHTMLTimeRanges* aBuffered, PRInt64 aStartTime);

private:
  
  
  enum TrackType {
    VIDEO = 0,
    AUDIO = 1
  };

  
  
  
  nestegg_packet* NextPacket(TrackType aTrackType);

  
  ogg_packet InitOggPacket(unsigned char* aData,
                           size_t aLength,
                           PRBool aBOS,
                           PRBool aEOS,
                           PRInt64 aGranulepos);
                     
  
  
  
  
  
  
  PRBool DecodeAudioPacket(nestegg_packet* aPacket);

  
  
  void Cleanup();

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

  
  PRInt64 mAudioStartMs;

  
  PRUint64 mAudioSamples;

  
  PRPackedBool mHasVideo;
  PRPackedBool mHasAudio;
};

#endif
