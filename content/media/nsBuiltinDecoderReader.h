





































#if !defined(nsBuiltinDecoderReader_h_)
#define nsBuiltinDecoderReader_h_

#include <nsDeque.h>
#include "Layers.h"
#include "ImageLayers.h"
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
      mDisplay(0,0),
      mStereoMode(mozilla::layers::STEREO_MODE_MONO),
      mHasAudio(PR_FALSE),
      mHasVideo(PR_FALSE)
  {}

  
  
  
  
  
  static PRBool ValidateVideoRegion(const nsIntSize& aFrame,
                                    const nsIntRect& aPicture,
                                    const nsIntSize& aDisplay);

  
  float mPixelAspectRatio;

  
  PRUint32 mAudioRate;

  
  PRUint32 mAudioChannels;

  
  nsIntSize mFrame;

  
  nsIntRect mPicture;

  
  
  nsIntSize mDisplay;

  
  mozilla::layers::StereoMode mStereoMode;

  
  PRPackedBool mHasAudio;

  
  PRPackedBool mHasVideo;
};

#ifdef MOZ_TREMOR
#include <ogg/os_types.h>
typedef ogg_int32_t VorbisPCMValue;
typedef short SoundDataValue;

#define MOZ_SOUND_DATA_FORMAT (nsAudioStream::FORMAT_S16_LE)
#define MOZ_CLIP_TO_15(x) ((x)<-32768?-32768:(x)<=32767?(x):32767)

#define MOZ_CONVERT_VORBIS_SAMPLE(x) \
 (static_cast<SoundDataValue>(MOZ_CLIP_TO_15((x)>>9)))

#define MOZ_CONVERT_SOUND_SAMPLE(x) ((x)*(1.F/32768))
#define MOZ_SAMPLE_TYPE_S16LE 1

#else 

typedef float VorbisPCMValue;
typedef float SoundDataValue;

#define MOZ_SOUND_DATA_FORMAT (nsAudioStream::FORMAT_FLOAT32)
#define MOZ_CONVERT_VORBIS_SAMPLE(x) (x)
#define MOZ_CONVERT_SOUND_SAMPLE(x) (x)
#define MOZ_SAMPLE_TYPE_FLOAT32 1

#endif


class SoundData {
public:
  SoundData(PRInt64 aOffset,
            PRInt64 aTime,
            PRInt64 aDuration,
            PRUint32 aSamples,
            SoundDataValue* aData,
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
            SoundDataValue* aData,
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
  nsAutoArrayPtr<SoundDataValue> mAudioData;
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

  
  
  
  PRBool IsFinished() {
    MonitorAutoEnter mon(mMonitor);
    return mEndOfStream;    
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







class nsBuiltinDecoderReader : public nsRunnable {
public:
  typedef mozilla::Monitor Monitor;
  typedef mozilla::MonitorAutoEnter MonitorAutoEnter;

  nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder);
  ~nsBuiltinDecoderReader();

  
  
  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor) = 0;

  
  virtual nsresult ResetDecode();

  
  
  
  
  virtual PRBool DecodeAudioData() = 0;

  
  
  
  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold) = 0;

  virtual PRBool HasAudio() = 0;
  virtual PRBool HasVideo() = 0;

  
  
  
  virtual nsresult ReadMetadata(nsVideoInfo* aInfo) = 0;

  
  
  
  virtual VideoData* FindStartTime(PRInt64 aOffset,
                                   PRInt64& aOutStartTime);

  
  
  virtual PRInt64 FindEndTime(PRInt64 aEndOffset);

  
  
  
  virtual nsresult Seek(PRInt64 aTime,
                        PRInt64 aStartTime,
                        PRInt64 aEndTime,
                        PRInt64 aCurrentTime) = 0;

  
  MediaQueue<SoundData> mAudioQueue;

  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered,
                               PRInt64 aStartTime) = 0;

  
  
  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) {}

protected:

  
  
  nsresult DecodeToTarget(PRInt64 aTarget);

  
  
  typedef PRBool (nsBuiltinDecoderReader::*DecodeFn)();

  
  
  template<class Data>
  Data* DecodeToFirstData(DecodeFn aDecodeFn,
                          MediaQueue<Data>& aQueue);

  
  
  PRBool DecodeVideoFrame() {
    PRBool f = PR_FALSE;
    return DecodeVideoFrame(f, 0);
  }

  
  
  Monitor mMonitor;

  
  
  nsBuiltinDecoder* mDecoder;

  
  
  nsVideoInfo mInfo;
};

#endif
