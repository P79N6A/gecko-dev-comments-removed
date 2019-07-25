





































#if !defined(nsBuiltinDecoderReader_h_)
#define nsBuiltinDecoderReader_h_

#include <nsDeque.h>
#include "Layers.h"
#include "ImageLayers.h"
#include "nsClassHashtable.h"
#include "mozilla/TimeStamp.h"
#include "nsSize.h"
#include "nsRect.h"
#include "mozilla/ReentrantMonitor.h"

class nsBuiltinDecoderStateMachine;


class nsVideoInfo {
public:
  nsVideoInfo()
    : mAudioRate(0),
      mAudioChannels(0),
      mDisplay(0,0),
      mStereoMode(mozilla::layers::STEREO_MODE_MONO),
      mHasAudio(PR_FALSE),
      mHasVideo(PR_FALSE)
  {}

  
  
  
  
  static bool ValidateVideoRegion(const nsIntSize& aFrame,
                                    const nsIntRect& aPicture,
                                    const nsIntSize& aDisplay);

  
  PRUint32 mAudioRate;

  
  PRUint32 mAudioChannels;

  
  
  nsIntSize mDisplay;

  
  mozilla::layers::StereoMode mStereoMode;

  
  bool mHasAudio;

  
  bool mHasVideo;
};

#ifdef MOZ_TREMOR
#include <ogg/os_types.h>
typedef ogg_int32_t VorbisPCMValue;
typedef short AudioDataValue;

#define MOZ_AUDIO_DATA_FORMAT (nsAudioStream::FORMAT_S16_LE)
#define MOZ_CLIP_TO_15(x) ((x)<-32768?-32768:(x)<=32767?(x):32767)

#define MOZ_CONVERT_VORBIS_SAMPLE(x) \
 (static_cast<AudioDataValue>(MOZ_CLIP_TO_15((x)>>9)))

#define MOZ_CONVERT_AUDIO_SAMPLE(x) ((x)*(1.F/32768))
#define MOZ_SAMPLE_TYPE_S16LE 1

#else 

typedef float VorbisPCMValue;
typedef float AudioDataValue;

#define MOZ_AUDIO_DATA_FORMAT (nsAudioStream::FORMAT_FLOAT32)
#define MOZ_CONVERT_VORBIS_SAMPLE(x) (x)
#define MOZ_CONVERT_AUDIO_SAMPLE(x) (x)
#define MOZ_SAMPLE_TYPE_FLOAT32 1

#endif


class AudioData {
public:
  AudioData(PRInt64 aOffset,
            PRInt64 aTime,
            PRInt64 aDuration,
            PRUint32 aFrames,
            AudioDataValue* aData,
            PRUint32 aChannels)
  : mOffset(aOffset),
    mTime(aTime),
    mDuration(aDuration),
    mFrames(aFrames),
    mChannels(aChannels),
    mAudioData(aData)
  {
    MOZ_COUNT_CTOR(AudioData);
  }

  ~AudioData()
  {
    MOZ_COUNT_DTOR(AudioData);
  }

  
  
  const PRInt64 mOffset;

  PRInt64 mTime; 
  const PRInt64 mDuration; 
  const PRUint32 mFrames;
  const PRUint32 mChannels;
  nsAutoArrayPtr<AudioDataValue> mAudioData;
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
                           bool aKeyframe,
                           PRInt64 aTimecode,
                           nsIntRect aPicture);

  
  
  
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

  
  
  
  nsIntSize mDisplay;

  
  PRInt64 mOffset;

  
  PRInt64 mTime;

  
  PRInt64 mEndTime;

  
  
  PRInt64 mTimecode;

  
  nsRefPtr<Image> mImage;

  
  
  bool mDuplicate;
  bool mKeyframe;

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
            bool aKeyframe,
            PRInt64 aTimecode,
            nsIntSize aDisplay)
    : mDisplay(aDisplay),
      mOffset(aOffset),
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
   typedef mozilla::ReentrantMonitorAutoEnter ReentrantMonitorAutoEnter;
   typedef mozilla::ReentrantMonitor ReentrantMonitor;

   MediaQueue()
     : nsDeque(new MediaQueueDeallocator<T>()),
       mReentrantMonitor("mediaqueue"),
       mEndOfStream(0)
   {}
  
  ~MediaQueue() {
    Reset();
  }

  inline PRInt32 GetSize() { 
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return nsDeque::GetSize();
  }
  
  inline void Push(T* aItem) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsDeque::Push(aItem);
  }
  
  inline void PushFront(T* aItem) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsDeque::PushFront(aItem);
  }
  
  inline T* Pop() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return static_cast<T*>(nsDeque::Pop());
  }

  inline T* PopFront() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return static_cast<T*>(nsDeque::PopFront());
  }
  
  inline T* Peek() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return static_cast<T*>(nsDeque::Peek());
  }
  
  inline T* PeekFront() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return static_cast<T*>(nsDeque::PeekFront());
  }

  inline void Empty() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsDeque::Empty();
  }

  inline void Erase() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsDeque::Erase();
  }

  void Reset() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (GetSize() > 0) {
      T* x = PopFront();
      delete x;
    }
    mEndOfStream = PR_FALSE;
  }

  bool AtEndOfStream() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return GetSize() == 0 && mEndOfStream;
  }

  
  
  
  bool IsFinished() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mEndOfStream;
  }

  
  void Finish() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mEndOfStream = PR_TRUE;
  }

  
  PRInt64 Duration() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    if (GetSize() < 2) {
      return 0;
    }
    T* last = Peek();
    T* first = PeekFront();
    return last->mTime - first->mTime;
  }

  void LockedForEach(nsDequeFunctor& aFunctor) const {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    ForEach(aFunctor);
  }

private:
  mutable ReentrantMonitor mReentrantMonitor;

  
  
  bool mEndOfStream;
};





class nsBuiltinDecoderReader : public nsRunnable {
public:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
  typedef mozilla::ReentrantMonitorAutoEnter ReentrantMonitorAutoEnter;

  nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder);
  ~nsBuiltinDecoderReader();

  
  
  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor) = 0;

  
  virtual nsresult ResetDecode();

  
  
  
  
  virtual bool DecodeAudioData() = 0;

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold) = 0;

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  
  
  virtual nsresult ReadMetadata(nsVideoInfo* aInfo) = 0;

  
  
  
  VideoData* FindStartTime(PRInt64& aOutStartTime);

  
  
  
  virtual nsresult Seek(PRInt64 aTime,
                        PRInt64 aStartTime,
                        PRInt64 aEndTime,
                        PRInt64 aCurrentTime) = 0;

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered,
                               PRInt64 aStartTime) = 0;

  class VideoQueueMemoryFunctor : public nsDequeFunctor {
  public:
    VideoQueueMemoryFunctor() : mResult(0) {}

    virtual void* operator()(void* anObject) {
      const VideoData* v = static_cast<const VideoData*>(anObject);
      if (!v->mImage) {
        return nsnull;
      }
      NS_ASSERTION(v->mImage->GetFormat() == mozilla::layers::Image::PLANAR_YCBCR,
                   "Wrong format?");
      mozilla::layers::PlanarYCbCrImage* vi = static_cast<mozilla::layers::PlanarYCbCrImage*>(v->mImage.get());

      mResult += vi->GetDataSize();
      return nsnull;
    }

    PRInt64 mResult;
  };

  PRInt64 VideoQueueMemoryInUse() {
    VideoQueueMemoryFunctor functor;
    mVideoQueue.LockedForEach(functor);
    return functor.mResult;
  }

  class AudioQueueMemoryFunctor : public nsDequeFunctor {
  public:
    AudioQueueMemoryFunctor() : mResult(0) {}

    virtual void* operator()(void* anObject) {
      const AudioData* audioData = static_cast<const AudioData*>(anObject);
      mResult += audioData->mFrames * audioData->mChannels * sizeof(AudioDataValue);
      return nsnull;
    }

    PRInt64 mResult;
  };

  PRInt64 AudioQueueMemoryInUse() {
    AudioQueueMemoryFunctor functor;
    mAudioQueue.LockedForEach(functor);
    return functor.mResult;
  }

  
  
  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) {}

protected:

  
  
  nsresult DecodeToTarget(PRInt64 aTarget);

  
  
  typedef bool (nsBuiltinDecoderReader::*DecodeFn)();

  
  
  template<class Data>
  Data* DecodeToFirstData(DecodeFn aDecodeFn,
                          MediaQueue<Data>& aQueue);

  
  
  bool DecodeVideoFrame() {
    bool f = false;
    return DecodeVideoFrame(f, 0);
  }

  
  nsBuiltinDecoder* mDecoder;

  
  nsVideoInfo mInfo;
};

#endif
