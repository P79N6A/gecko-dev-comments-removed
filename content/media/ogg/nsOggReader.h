





































#if !defined(nsOggReader_h_)
#define nsOggReader_h_

#include <nsDeque.h>
#include "nsOggCodecState.h"
#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>
#include "prmon.h"
#include "nsAutoLock.h"
#include "nsClassHashtable.h"
#include "mozilla/TimeStamp.h"
#include "nsSize.h"
#include "nsRect.h"
#include "mozilla/Monitor.h"

class nsOggPlayStateMachine;

using mozilla::Monitor;
using mozilla::MonitorAutoEnter;
using mozilla::TimeDuration;
using mozilla::TimeStamp;


class SoundData {
public:
  SoundData(PRInt64 aTime,
            PRInt64 aDuration,
            PRUint32 aSamples,
            float* aData,
            PRUint32 aChannels)
  : mTime(aTime),
    mDuration(aDuration),
    mSamples(aSamples),
    mAudioData(aData),
    mChannels(aChannels)
  {
    MOZ_COUNT_CTOR(SoundData);
  }

  SoundData(PRInt64 aDuration,
            PRUint32 aSamples,
            float* aData,
            PRUint32 aChannels)
  : mTime(-1),
    mDuration(aDuration),
    mSamples(aSamples),
    mAudioData(aData),
    mChannels(aChannels)
  {
    MOZ_COUNT_CTOR(SoundData);
  }

  ~SoundData()
  {
    MOZ_COUNT_DTOR(SoundData);
  }

  PRUint32 AudioDataLength() {
    return mChannels * mSamples;
  }

  PRInt64 mTime; 
  const PRInt64 mDuration; 
  const PRUint32 mSamples;
  const PRUint32 mChannels;
  nsAutoArrayPtr<float> mAudioData;
};


class VideoData {
public:

  
  
  
  static VideoData* Create(PRInt64 aTime,
                           th_ycbcr_buffer aBuffer,
                           PRBool aKeyframe,
                           PRInt64 aGranulepos);

  
  
  
  static VideoData* CreateDuplicate(PRInt64 aTime,
                                    PRInt64 aGranulepos)
  {
    return new VideoData(aTime, aGranulepos);
  }

  ~VideoData()
  {
    MOZ_COUNT_DTOR(VideoData);
    for (PRUint32 i = 0; i < 3; ++i) {
      delete mBuffer[i].data;
    }
  }

  
  PRInt64 mTime;
  PRInt64 mGranulepos;

  th_ycbcr_buffer mBuffer;

  
  
  PRPackedBool mDuplicate;
  PRPackedBool mKeyframe;

private:
  VideoData(PRInt64 aTime, PRInt64 aGranulepos) :
    mTime(aTime),
    mGranulepos(aGranulepos),
    mDuplicate(PR_TRUE),
    mKeyframe(PR_FALSE)
  {
    MOZ_COUNT_CTOR(VideoData);
    memset(&mBuffer, 0, sizeof(th_ycbcr_buffer));
  }

  VideoData(PRInt64 aTime,
            PRBool aKeyframe,
            PRInt64 aGranulepos)
  : mTime(aTime),
    mGranulepos(aGranulepos),
    mDuplicate(PR_FALSE),
    mKeyframe(aKeyframe)
  {
    MOZ_COUNT_CTOR(VideoData);
  }

};


template <class T>
class MediaQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* anObject) {
    delete static_cast<T*>(anObject);
    return nsnull;
  }
};

template <class T> class MediaQueue : private nsDeque {
 public:
  
   MediaQueue()
     : nsDeque(new MediaQueueDeallocator<T>()),
       mMonitor("mediaqueue"),
       mEndOfStream(0)
   {}
  
  ~MediaQueue() {
    Reset();
  }

  inline PRInt32 GetSize() { 
    MonitorAutoEnter mon(mMonitor);
    return nsDeque::GetSize();
  }
  
  inline void Push(T* aItem) {
    MonitorAutoEnter mon(mMonitor);
    nsDeque::Push(aItem);
  }
  
  inline void PushFront(T* aItem) {
    MonitorAutoEnter mon(mMonitor);
    nsDeque::PushFront(aItem);
  }
  
  inline T* Pop() {
    MonitorAutoEnter mon(mMonitor);
    return static_cast<T*>(nsDeque::Pop());
  }

  inline T* PopFront() {
    MonitorAutoEnter mon(mMonitor);
    return static_cast<T*>(nsDeque::PopFront());
  }
  
  inline T* Peek() {
    MonitorAutoEnter mon(mMonitor);
    return static_cast<T*>(nsDeque::Peek());
  }
  
  inline T* PeekFront() {
    MonitorAutoEnter mon(mMonitor);
    return static_cast<T*>(nsDeque::PeekFront());
  }

  inline void Empty() {
    MonitorAutoEnter mon(mMonitor);
    nsDeque::Empty();
  }

  inline void Erase() {
    MonitorAutoEnter mon(mMonitor);
    nsDeque::Erase();
  }

  void Reset() {
    MonitorAutoEnter mon(mMonitor);
    while (GetSize() > 0) {
      T* x = PopFront();
      delete x;
    }
    mEndOfStream = PR_FALSE;
  }

  PRBool AtEndOfStream() {
    MonitorAutoEnter mon(mMonitor);
    return GetSize() == 0 && mEndOfStream;    
  }

  void Finish() {
    MonitorAutoEnter mon(mMonitor);
    mEndOfStream = PR_TRUE;    
  }

  
  PRInt64 Duration() {
    MonitorAutoEnter mon(mMonitor);
    if (GetSize() < 2) {
      return 0;
    }
    T* last = Peek();
    T* first = PeekFront();
    return last->mTime - first->mTime;
  }

private:
  Monitor mMonitor;

  
  
  PRBool mEndOfStream;
};




class ByteRange {
public:
  ByteRange() :
      mOffsetStart(0),
      mOffsetEnd(0),
      mTimeStart(0),
      mTimeEnd(0)
  {}

  ByteRange(PRInt64 aOffsetStart,
            PRInt64 aOffsetEnd,
            PRInt64 aTimeStart,
            PRInt64 aTimeEnd)
    : mOffsetStart(aOffsetStart),
      mOffsetEnd(aOffsetEnd),
      mTimeStart(aTimeStart),
      mTimeEnd(aTimeEnd)
  {}

  PRBool IsNull() {
    return mOffsetStart == 0 &&
           mOffsetEnd == 0 &&
           mTimeStart == 0 &&
           mTimeEnd == 0;
  }

  PRInt64 mOffsetStart, mOffsetEnd; 
  PRInt64 mTimeStart, mTimeEnd; 
};


class nsOggInfo {
public:
  nsOggInfo()
    : mFramerate(0.0),
      mAspectRatio(1.0),
      mCallbackPeriod(1),
      mAudioRate(0),
      mAudioChannels(0),
      mFrame(0,0),
      mHasAudio(PR_FALSE),
      mHasVideo(PR_FALSE)
  {}

  
  float mFramerate;

  
  float mAspectRatio;

  
  
  PRUint32 mCallbackPeriod;

  
  PRUint32 mAudioRate;

  
  PRUint32 mAudioChannels;

  
  nsIntSize mFrame;

  
  nsIntRect mPicture;

  
  
  PRInt64 mDataOffset;

  
  PRPackedBool mHasAudio;

  
  PRPackedBool mHasVideo;
};







class nsOggReader : public nsRunnable {
public:
  nsOggReader(nsOggPlayStateMachine* aStateMachine);
  ~nsOggReader();

  PRBool HasAudio()
  {
    MonitorAutoEnter mon(mMonitor);
    return mVorbisState != 0 && mVorbisState->mActive;
  }

  PRBool HasVideo()
  {
    MonitorAutoEnter mon(mMonitor);
    return mTheoraState != 0 && mTheoraState->mActive;
  }

  
  
  
  nsresult ReadOggHeaders(nsOggInfo& aInfo);

  
  
  VideoData* FindStartTime(PRInt64 aOffset,
                           PRInt64& aOutStartTime);

  
  
  PRInt64 FindEndTime(PRInt64 aEndOffset);

  
  
  
  
  PRBool DecodeAudioPage();
  
  
  
  
  
  
  
  PRBool DecodeVideoPage(PRBool &aKeyframeSkip,
                         PRInt64 aTimeThreshold);

  
  
  nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime);

  
  MediaQueue<SoundData> mAudioQueue;

  
  MediaQueue<VideoData> mVideoQueue;

  
  
  nsresult Init();

private:

  
  
  typedef PRBool (nsOggReader::*DecodeFn)();

  
  
  template<class Data>
  Data* DecodeToFirstData(DecodeFn aDecodeFn,
                          MediaQueue<Data>& aQueue);

  
  
  PRBool DecodeVideoPage() {
    PRBool f = PR_FALSE;
    return DecodeVideoPage(f, 0);
  }

  
  
  nsresult DecodeVorbis(nsTArray<SoundData*>& aChunks,
                        ogg_packet* aPacket);

  
  nsresult DecodeTheora(nsTArray<VideoData*>& aFrames,
                        ogg_packet* aPacket);

  
  nsresult ResetDecode();

  
  
  PRInt64 ReadOggPage(ogg_page* aPage);

  
  
  PRBool ReadOggPacket(nsOggCodecState* aCodecState, ogg_packet* aPacket);

  
  
  
  
  
  
  nsresult SeekBisection(PRInt64 aTarget,
                         const ByteRange& aRange,
                         PRUint32 aFuzz);

  
  
  
  
  nsresult GetBufferedBytes(nsTArray<ByteRange>& aRanges);

  
  
  
  
  
  
  
  
  ByteRange GetSeekRange(const nsTArray<ByteRange>& aRanges,
                         PRInt64 aTarget,
                         PRInt64 aStartTime,
                         PRInt64 aEndTime,
                         PRBool aExact);

  
  
  Monitor mMonitor;

  
  
  nsOggPlayStateMachine* mPlayer;

  
  nsClassHashtable<nsUint32HashKey, nsOggCodecState> mCodecStates;

  
  nsTheoraState* mTheoraState;

  
  nsVorbisState* mVorbisState;

  
  ogg_sync_state mOggState;

  
  
  PRInt64 mPageOffset;

  
  
  PRInt64 mDataOffset;

  
  PRInt64 mTheoraGranulepos;

  
  PRInt64 mVorbisGranulepos;

  
  PRUint32 mCallbackPeriod;

};

#endif
