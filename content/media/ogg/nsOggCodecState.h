





































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

class OggPageDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* aPage) {
    ogg_page* p = static_cast<ogg_page*>(aPage);
    delete [] p->header;
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

  
  static PRInt64 Time(vorbis_info* aInfo, PRInt64 aGranulePos); 
 
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

  
  static PRInt64 Time(th_info* aInfo, PRInt64 aGranulePos); 
  
  th_info mInfo;
  th_comment mComment;
  th_setup_info *mSetup;
  th_dec_ctx* mCtx;

  float mPixelAspectRatio;
};



#define SKELETON_VERSION(major, minor) (((major)<<16)|(minor))

class nsSkeletonState : public nsOggCodecState {
public:
  nsSkeletonState(ogg_page* aBosPage);
  virtual ~nsSkeletonState();
  virtual CodecType GetType() { return TYPE_SKELETON; }
  virtual PRBool DecodeHeader(ogg_packet* aPacket);
  virtual PRInt64 Time(PRInt64 granulepos) { return -1; }
  virtual PRBool Init() { return PR_TRUE; }

  
  
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
