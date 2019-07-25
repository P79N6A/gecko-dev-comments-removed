





































#if !defined(nsOggCodecState_h_)
#define nsOggCodecState_h_

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#ifdef MOZ_TREMOR
#include <tremor/ivorbiscodec.h>
#else
#include <vorbis/codec.h>
#endif
#include <nsDeque.h>
#include <nsTArray.h>
#include <nsClassHashtable.h>
#include "VideoUtils.h"



#define VALIDATE_VORBIS_SAMPLE_CALCULATION
#ifdef  VALIDATE_VORBIS_SAMPLE_CALCULATION
#include <map>
#endif


class OggPacketDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* aPacket) {
    ogg_packet* p = static_cast<ogg_packet*>(aPacket);
    delete [] p->packet;
    delete p;
    return nsnull;
  }
};











class nsPacketQueue : private nsDeque {
public:
  nsPacketQueue() : nsDeque(new OggPacketDeallocator()) {}
  ~nsPacketQueue() { Erase(); }
  PRBool IsEmpty() { return nsDeque::GetSize() == 0; }
  void Append(ogg_packet* aPacket);
  ogg_packet* PopFront() { return static_cast<ogg_packet*>(nsDeque::PopFront()); }
  ogg_packet* PeekFront() { return static_cast<ogg_packet*>(nsDeque::PeekFront()); }
  void PushFront(ogg_packet* aPacket) { nsDeque::PushFront(aPacket); }
  void PushBack(ogg_packet* aPacket) { nsDeque::PushFront(aPacket); }
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

  
  
  void Deactivate() {
    mActive = PR_FALSE;
    mDoneReadingHeaders = PR_TRUE;
    Reset();
  }

  
  virtual nsresult Reset();

  
  
  
  
  
  
  virtual PRBool IsHeader(ogg_packet* aPacket) { return PR_FALSE; }

  
  
  
  
  
  ogg_packet* PacketOut();

  
  
  static void ReleasePacket(ogg_packet* aPacket);

  
  
  
  
  
  
  virtual nsresult PageIn(ogg_page* aPage);

  
  PRUint64 mPacketCount;

  
  PRUint32 mSerial;

  
  ogg_stream_state mState;

  
  
  nsPacketQueue mPackets;

  
  PRPackedBool mActive;
  
  
  PRPackedBool mDoneReadingHeaders;

protected:
  
  
  
  nsOggCodecState(ogg_page* aBosPage, PRBool aActive);

  
  void ClearUnstamped();

  
  
  
  
  
  
  
  PRBool PacketOutUntilGranulepos();

  
  
  nsTArray<ogg_packet*> mUnstamped;
};

class nsVorbisState : public nsOggCodecState {
public:
  nsVorbisState(ogg_page* aBosPage);
  virtual ~nsVorbisState();

  CodecType GetType() { return TYPE_VORBIS; }
  PRBool DecodeHeader(ogg_packet* aPacket);
  PRInt64 Time(PRInt64 granulepos);
  PRBool Init();
  nsresult Reset();
  PRBool IsHeader(ogg_packet* aPacket);
  nsresult PageIn(ogg_page* aPage); 

  
  static PRInt64 Time(vorbis_info* aInfo, PRInt64 aGranulePos); 

  vorbis_info mInfo;
  vorbis_comment mComment;
  vorbis_dsp_state mDsp;
  vorbis_block mBlock;

private:

  
  
  nsresult ReconstructVorbisGranulepos();

  
  
  
  
  long mPrevVorbisBlockSize;

  
  
  
  PRInt64 mGranulepos;

#ifdef VALIDATE_VORBIS_SAMPLE_CALCULATION
  
  
  
  std::map<ogg_packet*, long> mVorbisPacketSamples;
#endif

  
  
  
  void RecordVorbisPacketSamples(ogg_packet* aPacket, long aSamples);

  
  
  
  void AssertHasRecordedPacketSamples(ogg_packet* aPacket);

public:
  
  
  
  void ValidateVorbisPacketSamples(ogg_packet* aPacket, long aSamples);

};



int TheoraVersion(th_info* info,
                  unsigned char maj,
                  unsigned char min,
                  unsigned char sub);

class nsTheoraState : public nsOggCodecState {
public:
  nsTheoraState(ogg_page* aBosPage);
  virtual ~nsTheoraState();

  CodecType GetType() { return TYPE_THEORA; }
  PRBool DecodeHeader(ogg_packet* aPacket);
  PRInt64 Time(PRInt64 granulepos);
  PRInt64 StartTime(PRInt64 granulepos);
  PRBool Init();
  PRBool IsHeader(ogg_packet* aPacket);
  nsresult PageIn(ogg_page* aPage); 

  
  
  PRInt64 MaxKeyframeOffset();

  
  static PRInt64 Time(th_info* aInfo, PRInt64 aGranulePos); 

  th_info mInfo;
  th_comment mComment;
  th_setup_info *mSetup;
  th_dec_ctx* mCtx;

  float mPixelAspectRatio;

private:

  
  
  
  
  
  void ReconstructTheoraGranulepos();

};



#define SKELETON_VERSION(major, minor) (((major)<<16)|(minor))

class nsSkeletonState : public nsOggCodecState {
public:
  nsSkeletonState(ogg_page* aBosPage);
  ~nsSkeletonState();
  CodecType GetType() { return TYPE_SKELETON; }
  PRBool DecodeHeader(ogg_packet* aPacket);
  PRInt64 Time(PRInt64 granulepos) { return -1; }
  PRBool Init() { return PR_TRUE; }
  PRBool IsHeader(ogg_packet* aPacket) { return PR_TRUE; }

  
  
  PRBool IsPresentable(PRInt64 aTime) { return aTime >= mPresentationTime; }

  
  
  class nsKeyPoint {
  public:
    nsKeyPoint()
      : mOffset(PR_INT64_MAX),
        mTime(PR_INT64_MAX) {}

    nsKeyPoint(PRInt64 aOffset, PRInt64 aTime)
      : mOffset(aOffset),
        mTime(aTime) {}

    
    PRInt64 mOffset;

    
    PRInt64 mTime;

    PRBool IsNull() {
      return mOffset == PR_INT64_MAX &&
             mTime == PR_INT64_MAX;
    }
  };

  
  
  class nsSeekTarget {
  public:
    nsSeekTarget() : mSerial(0) {}
    nsKeyPoint mKeyPoint;
    PRUint32 mSerial;
    PRBool IsNull() {
      return mKeyPoint.IsNull() &&
             mSerial == 0;
    }
  };

  
  
  
  nsresult IndexedSeekTarget(PRInt64 aTarget,
                             nsTArray<PRUint32>& aTracks,
                             nsSeekTarget& aResult);

  PRBool HasIndex() const {
    return mIndex.IsInitialized() && mIndex.Count() > 0;
  }

  
  
  
  
  nsresult GetDuration(const nsTArray<PRUint32>& aTracks, PRInt64& aDuration);

private:

  
  PRBool DecodeIndex(ogg_packet* aPacket);

  
  
  nsresult IndexedSeekTargetForTrack(PRUint32 aSerialno,
                                     PRInt64 aTarget,
                                     nsKeyPoint& aResult);

  
  PRUint32 mVersion;

  
  PRInt64 mPresentationTime;

  
  PRInt64 mLength;

  
  
  class nsKeyFrameIndex {
  public:

    nsKeyFrameIndex(PRInt64 aStartTime, PRInt64 aEndTime) 
      : mStartTime(aStartTime),
        mEndTime(aEndTime)
    {
      MOZ_COUNT_CTOR(nsKeyFrameIndex);
    }

    ~nsKeyFrameIndex() {
      MOZ_COUNT_DTOR(nsKeyFrameIndex);
    }

    void Add(PRInt64 aOffset, PRInt64 aTimeMs) {
      mKeyPoints.AppendElement(nsKeyPoint(aOffset, aTimeMs));
    }

    const nsKeyPoint& Get(PRUint32 aIndex) const {
      return mKeyPoints[aIndex];
    }

    PRUint32 Length() const {
      return mKeyPoints.Length();
    }

    
    const PRInt64 mStartTime;

    
    const PRInt64 mEndTime;

  private:
    nsTArray<nsKeyPoint> mKeyPoints;
  };

  
  nsClassHashtable<nsUint32HashKey, nsKeyFrameIndex> mIndex;
};

#endif
