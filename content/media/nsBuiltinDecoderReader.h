





































#if !defined(nsBuiltinDecoderReader_h_)
#define nsBuiltinDecoderReader_h_

#include <nsDeque.h>
#include "Layers.h"
#include "ImageLayers.h"
#include "nsAutoLock.h"
#include "nsClassHashtable.h"
#include "mozilla/TimeStamp.h"
#include "nsSize.h"
#include "nsRect.h"
#include "mozilla/Monitor.h"

class nsBuiltinDecoderStateMachine;


class nsVideoInfo {
public:
  nsVideoInfo()
    : mPixelAspectRatio(1.0),
      mAudioRate(0),
      mAudioChannels(0),
      mFrame(0,0),
      mHasAudio(PR_FALSE),
      mHasVideo(PR_FALSE)
  {}

  
  float mPixelAspectRatio;

  
  PRUint32 mAudioRate;

  
  PRUint32 mAudioChannels;

  
  nsIntSize mFrame;

  
  nsIntRect mPicture;

  
  
  PRInt64 mDataOffset;

  
  PRPackedBool mHasAudio;

  
  PRPackedBool mHasVideo;
};


class SoundData {
public:
  SoundData(PRInt64 aOffset,
            PRInt64 aTime,
            PRInt64 aDuration,
            PRUint32 aSamples,
            float* aData,
            PRUint32 aChannels)
  : mOffset(aOffset),
    mTime(aTime),
    mDuration(aDuration),
    mSamples(aSamples),
    mChannels(aChannels),
    mAudioData(aData)
  {
    MOZ_COUNT_CTOR(SoundData);
  }

  SoundData(PRInt64 aOffset,
            PRInt64 aDuration,
            PRUint32 aSamples,
            float* aData,
            PRUint32 aChannels)
  : mOffset(aOffset),
    mTime(-1),
    mDuration(aDuration),
    mSamples(aSamples),
    mChannels(aChannels),
    mAudioData(aData)
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

  
  
  const PRInt64 mOffset;

  PRInt64 mTime; 
  const PRInt64 mDuration; 
  const PRUint32 mSamples;
  const PRUint32 mChannels;
  nsAutoArrayPtr<float> mAudioData;
};


class VideoData {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::Image Image;

  
  
  
  
  struct YCbCrBuffer {
    struct Plane {
      PRUint8* mData;
      PRUint32 mWidth;
      PRUint32 mHeight;
      PRUint32 mStride;
    };

    Plane mPlanes[3];
  };

  
  
  
  
  
  
  static VideoData* Create(nsVideoInfo& aInfo,
                           ImageContainer* aContainer,
                           PRInt64 aOffset,
                           PRInt64 aTime,
                           PRInt64 aEndTime,
                           const YCbCrBuffer &aBuffer,
                           PRBool aKeyframe,
                           PRInt64 aTimecode);

  
  
  
  static VideoData* CreateDuplicate(PRInt64 aOffset,
                                    PRInt64 aTime,
                                    PRInt64 aEndTime,
                                    PRInt64 aTimecode)
  {
    return new VideoData(aOffset, aTime, aEndTime, aTimecode);
  }

  ~VideoData()
  {
    MOZ_COUNT_DTOR(VideoData);
  }

  
  PRInt64 mOffset;

  
  PRInt64 mTime;

  
  PRInt64 mEndTime;

  
  
  PRInt64 mTimecode;

  
  nsRefPtr<Image> mImage;

  
  
  PRPackedBool mDuplicate;
  PRPackedBool mKeyframe;

public:
  VideoData(PRInt64 aOffset, PRInt64 aTime, PRInt64 aEndTime, PRInt64 aTimecode)
    : mOffset(aOffset),
      mTime(aTime),
      mEndTime(aEndTime),
      mTimecode(aTimecode),
      mDuplicate(PR_TRUE),
      mKeyframe(PR_FALSE)
  {
    MOZ_COUNT_CTOR(VideoData);
    NS_ASSERTION(aEndTime >= aTime, "Frame must start before it ends.");
  }

  VideoData(PRInt64 aOffset,
            PRInt64 aTime,
            PRInt64 aEndTime,
            PRBool aKeyframe,
            PRInt64 aTimecode)
    : mOffset(aOffset),
      mTime(aTime),
      mEndTime(aEndTime),
      mTimecode(aTimecode),
      mDuplicate(PR_FALSE),
      mKeyframe(aKeyframe)
  {
    MOZ_COUNT_CTOR(VideoData);
    NS_ASSERTION(aEndTime >= aTime, "Frame must start before it ends.");
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
   typedef mozilla::MonitorAutoEnter MonitorAutoEnter;
   typedef mozilla::Monitor Monitor;

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
  ByteRange()
    : mOffsetStart(0),
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







class nsBuiltinDecoderReader : public nsRunnable {
public:
  typedef mozilla::Monitor Monitor;

  nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder);
  ~nsBuiltinDecoderReader();

  
  
  virtual nsresult Init() = 0;

  
  virtual nsresult ResetDecode();

  
  
  
  
  virtual PRBool DecodeAudioData() = 0;

  
  
  
  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold) = 0;

  virtual PRBool HasAudio() = 0;
  virtual PRBool HasVideo() = 0;

  
  
  
  virtual nsresult ReadMetadata() = 0;


  
  
  virtual VideoData* FindStartTime(PRInt64 aOffset,
                                   PRInt64& aOutStartTime);

  
  
  virtual PRInt64 FindEndTime(PRInt64 aEndOffset);

  
  
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime) = 0;

  
  const nsVideoInfo& GetInfo() {
    return mInfo;
  }

  
  MediaQueue<SoundData> mAudioQueue;

  
  MediaQueue<VideoData> mVideoQueue;

protected:

  
  
  typedef PRBool (nsBuiltinDecoderReader::*DecodeFn)();

  
  
  template<class Data>
  Data* DecodeToFirstData(DecodeFn aDecodeFn,
                          MediaQueue<Data>& aQueue);

  
  
  PRBool DecodeVideoFrame() {
    PRBool f = PR_FALSE;
    return DecodeVideoFrame(f, 0);
  }

  
  
  
  
  nsresult GetBufferedBytes(nsTArray<ByteRange>& aRanges);

  
  
  
  
  
  
  
  
  ByteRange GetSeekRange(const nsTArray<ByteRange>& aRanges,
                         PRInt64 aTarget,
                         PRInt64 aStartTime,
                         PRInt64 aEndTime,
                         PRBool aExact);

  
  
  Monitor mMonitor;

  
  
  nsBuiltinDecoder* mDecoder;

  
  
  PRInt64 mDataOffset;

  
  nsVideoInfo mInfo;
};

#endif
