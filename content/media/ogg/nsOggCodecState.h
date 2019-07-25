





































#if !defined(nsOggCodecState_h_)
#define nsOggCodecState_h_

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>
#include <nsDeque.h>

class OggPageDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* aPage) {
    ogg_page* p = static_cast<ogg_page*>(aPage);
    delete p->header;
    delete p;
    return nsnull;
  }
};








class nsPageQueue : private nsDeque {
public:
  nsPageQueue() : nsDeque(new OggPageDeallocator()) {}
  ~nsPageQueue() { Erase(); }
  PRBool IsEmpty() { return nsDeque::GetSize() == 0; }
  void Append(ogg_page* aPage);
  ogg_page* PopFront() { return static_cast<ogg_page*>(nsDeque::PopFront()); }
  ogg_page* PeekFront() { return static_cast<ogg_page*>(nsDeque::PeekFront()); }
  void Erase() { nsDeque::Erase(); }
};



class nsOggCodecState {
 public:
  
  enum CodecType {
    TYPE_VORBIS=0,
    TYPE_THEORA=1,
    TYPE_SKELETON=2,
    TYPE_UNKNOWN=3
  };

 public:
  nsOggCodecState(ogg_page* aBosPage);
  virtual ~nsOggCodecState();
  
  
  static nsOggCodecState* Create(ogg_page* aPage);
  
  virtual CodecType GetType() { return TYPE_UNKNOWN; }
  
  
  virtual PRBool DecodeHeader(ogg_packet* aPacket) {
    return (mDoneReadingHeaders = PR_TRUE);
  }

  
  virtual PRInt64 Time(PRInt64 granulepos) { return -1; }

  
  virtual PRInt64 StartTime(PRInt64 granulepos) { return -1; }

  
  virtual PRBool Init();

  
  
  PRBool DoneReadingHeaders() { return mDoneReadingHeaders; }

  
  
  void Deactivate() { mActive = PR_FALSE; }

  
  virtual nsresult Reset();

  
  
  
  inline void AddToBuffer(ogg_page* aPage) { mBuffer.Append(aPage); }

  
  
  PRBool PageInFromBuffer();

public:

  
  PRUint64 mPacketCount;

  
  PRUint32 mSerial;

  
  ogg_stream_state mState;

  
  nsPageQueue mBuffer;

  
  PRPackedBool mActive;
  
  
  PRPackedBool mDoneReadingHeaders;
};

class nsVorbisState : public nsOggCodecState {
public:
  nsVorbisState(ogg_page* aBosPage);
  virtual ~nsVorbisState();

  virtual CodecType GetType() { return TYPE_VORBIS; }
  virtual PRBool DecodeHeader(ogg_packet* aPacket);
  virtual PRInt64 Time(PRInt64 granulepos);
  virtual PRBool Init();
  virtual nsresult Reset();

  vorbis_info mInfo;
  vorbis_comment mComment;
  vorbis_dsp_state mDsp;
  vorbis_block mBlock;
};

class nsTheoraState : public nsOggCodecState {
public:
  nsTheoraState(ogg_page* aBosPage);
  virtual ~nsTheoraState();

  virtual CodecType GetType() { return TYPE_THEORA; }
  virtual PRBool DecodeHeader(ogg_packet* aPacket);
  virtual PRInt64 Time(PRInt64 granulepos);
  virtual PRInt64 StartTime(PRInt64 granulepos);
  virtual PRBool Init();

  
  
  PRInt64 MaxKeyframeOffset();

  th_info mInfo;
  th_comment mComment;
  th_setup_info *mSetup;
  th_dec_ctx* mCtx;

  
  PRUint32 mFrameDuration;

  
  float mFrameRate;

  float mPixelAspectRatio;
};

class nsSkeletonState : public nsOggCodecState {
public:
  nsSkeletonState(ogg_page* aBosPage);
  virtual ~nsSkeletonState();
  virtual CodecType GetType() { return TYPE_SKELETON; }
  virtual PRBool DecodeHeader(ogg_packet* aPacket);
  virtual PRInt64 Time(PRInt64 granulepos) { return -1; }
  virtual PRBool Init() { return PR_TRUE; }
};

#endif
