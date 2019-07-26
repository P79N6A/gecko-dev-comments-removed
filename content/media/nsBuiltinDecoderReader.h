




#if !defined(nsBuiltinDecoderReader_h_)
#define nsBuiltinDecoderReader_h_

#include <nsDeque.h>
#include "nsSize.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaStreamGraph.h"
#include "SharedBuffer.h"
#include "ImageLayers.h"


class nsVideoInfo {
public:
  nsVideoInfo()
    : mAudioRate(44100),
      mAudioChannels(2),
      mDisplay(0,0),
      mStereoMode(mozilla::STEREO_MODE_MONO),
      mHasAudio(false),
      mHasVideo(false)
  {}

  
  
  
  
  static bool ValidateVideoRegion(const nsIntSize& aFrame,
                                    const nsIntRect& aPicture,
                                    const nsIntSize& aDisplay);

  
  uint32_t mAudioRate;

  
  uint32_t mAudioChannels;

  
  
  nsIntSize mDisplay;

  
  mozilla::StereoMode mStereoMode;

  
  bool mHasAudio;

  
  bool mHasVideo;
};

#ifdef MOZ_SAMPLE_TYPE_S16LE
#include <ogg/os_types.h>
typedef ogg_int32_t VorbisPCMValue;
typedef short AudioDataValue;

#define MOZ_CLIP_TO_15(x) ((x)<-32768?-32768:(x)<=32767?(x):32767)

#define MOZ_CONVERT_VORBIS_SAMPLE(x) \
 (static_cast<AudioDataValue>(MOZ_CLIP_TO_15((x)>>9)))

#define MOZ_CONVERT_AUDIO_SAMPLE(x) ((x)*(1.F/32768))

#else 

typedef float VorbisPCMValue;
typedef float AudioDataValue;

#define MOZ_CONVERT_VORBIS_SAMPLE(x) (x)
#define MOZ_CONVERT_AUDIO_SAMPLE(x) (x)

#endif


class AudioData {
public:
  typedef mozilla::SharedBuffer SharedBuffer;

  AudioData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            uint32_t aFrames,
            AudioDataValue* aData,
            uint32_t aChannels)
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

  
  void EnsureAudioBuffer();

  int64_t GetEnd() { return mTime + mDuration; }

  
  
  const int64_t mOffset;

  int64_t mTime; 
  const int64_t mDuration; 
  const uint32_t mFrames;
  const uint32_t mChannels;
  
  
  nsRefPtr<SharedBuffer> mAudioBuffer;
  
  nsAutoArrayPtr<AudioDataValue> mAudioData;
};


class VideoData {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::Image Image;

  
  
  
  
  struct YCbCrBuffer {
    struct Plane {
      uint8_t* mData;
      uint32_t mWidth;
      uint32_t mHeight;
      uint32_t mStride;
      uint32_t mOffset;
      uint32_t mSkip;
    };

    Plane mPlanes[3];
  };

  
  
  
  
  
  
  static VideoData* Create(nsVideoInfo& aInfo,
                           ImageContainer* aContainer,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aEndTime,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  
  
  
  static VideoData* CreateDuplicate(int64_t aOffset,
                                    int64_t aTime,
                                    int64_t aEndTime,
                                    int64_t aTimecode)
  {
    return new VideoData(aOffset, aTime, aEndTime, aTimecode);
  }

  ~VideoData();

  int64_t GetEnd() { return mEndTime; }

  
  
  
  nsIntSize mDisplay;

  
  int64_t mOffset;

  
  int64_t mTime;

  
  int64_t mEndTime;

  
  
  int64_t mTimecode;

  
  nsRefPtr<Image> mImage;

  
  
  bool mDuplicate;
  bool mKeyframe;

public:
  VideoData(int64_t aOffset, int64_t aTime, int64_t aEndTime, int64_t aTimecode);

  VideoData(int64_t aOffset,
            int64_t aTime,
            int64_t aEndTime,
            bool aKeyframe,
            int64_t aTimecode,
            nsIntSize aDisplay);

};


template <class T>
class MediaQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* anObject) {
    delete static_cast<T*>(anObject);
    return nullptr;
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

  inline int32_t GetSize() { 
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
    mEndOfStream = false;
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
    mEndOfStream = true;
  }

  
  int64_t Duration() {
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

  
  
  void GetElementsAfter(int64_t aTime, nsTArray<T*>* aResult) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    if (!GetSize())
      return;
    int32_t i;
    for (i = GetSize() - 1; i > 0; --i) {
      T* v = static_cast<T*>(ObjectAt(i));
      if (v->GetEnd() < aTime)
        break;
    }
    
    
    for (; i < GetSize(); ++i) {
      aResult->AppendElement(static_cast<T*>(ObjectAt(i)));
    }
  }

private:
  mutable ReentrantMonitor mReentrantMonitor;

  
  
  bool mEndOfStream;
};





class nsBuiltinDecoderReader : public nsRunnable {
public:
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
  typedef mozilla::ReentrantMonitorAutoEnter ReentrantMonitorAutoEnter;
  typedef mozilla::VideoFrameContainer VideoFrameContainer;

  nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder);
  virtual ~nsBuiltinDecoderReader();

  
  
  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor) = 0;

  
  virtual nsresult ResetDecode();

  
  
  
  
  virtual bool DecodeAudioData() = 0;

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) = 0;

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  
  
  
  virtual nsresult ReadMetadata(nsVideoInfo* aInfo,
                                nsHTMLMediaElement::MetadataTags** aTags) = 0;

  
  
  
  VideoData* FindStartTime(int64_t& aOutStartTime);

  
  
  
  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime) = 0;

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered,
                               int64_t aStartTime) = 0;

  
  virtual bool IsSeekableInBufferedRanges() = 0;

  class VideoQueueMemoryFunctor : public nsDequeFunctor {
  public:
    VideoQueueMemoryFunctor() : mResult(0) {}

    virtual void* operator()(void* anObject);

    int64_t mResult;
  };

  int64_t VideoQueueMemoryInUse() {
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
      return nullptr;
    }

    int64_t mResult;
  };

  int64_t AudioQueueMemoryInUse() {
    AudioQueueMemoryFunctor functor;
    mAudioQueue.LockedForEach(functor);
    return functor.mResult;
  }

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {}

protected:

  
  
  nsresult DecodeToTarget(int64_t aTarget);

  
  
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
