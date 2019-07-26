




#if !defined(MediaDecoderReader_h_)
#define MediaDecoderReader_h_

#include <nsDeque.h>
#include "nsSize.h"
#include "mozilla/ReentrantMonitor.h"
#include "MediaStreamGraph.h"
#include "SharedBuffer.h"
#include "ImageLayers.h"
#include "AudioSampleFormat.h"
#include "MediaResource.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {

class AbstractMediaDecoder;


class VideoInfo {
public:
  VideoInfo()
    : mAudioRate(44100),
      mAudioChannels(2),
      mDisplay(0,0),
      mStereoMode(STEREO_MODE_MONO),
      mHasAudio(false),
      mHasVideo(false)
  {}

  
  
  
  
  static bool ValidateVideoRegion(const nsIntSize& aFrame,
                                  const nsIntRect& aPicture,
                                  const nsIntSize& aDisplay);

  
  uint32_t mAudioRate;

  
  uint32_t mAudioChannels;

  
  
  nsIntSize mDisplay;

  
  StereoMode mStereoMode;

  
  bool mHasAudio;

  
  bool mHasVideo;
};


class AudioData {
public:

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

namespace layers {
class GraphicBufferLocked;
}


class VideoData {
public:
  typedef layers::ImageContainer ImageContainer;
  typedef layers::Image Image;

  
  
  
  
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

  
  
  
  
  
  
  
  
  static VideoData* Create(VideoInfo& aInfo,
                           ImageContainer* aContainer,
                           Image* aImage,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aEndTime,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  
  static VideoData* Create(VideoInfo& aInfo,
                           ImageContainer* aContainer,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aEndTime,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  
  static VideoData* Create(VideoInfo& aInfo,
                           Image* aImage,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aEndTime,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  static VideoData* Create(VideoInfo& aInfo,
                           ImageContainer* aContainer,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aEndTime,
                           layers::GraphicBufferLocked* aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  static VideoData* CreateFromImage(VideoInfo& aInfo,
                                    ImageContainer* aContainer,
                                    int64_t aOffset,
                                    int64_t aTime,
                                    int64_t aEndTime,
                                    const nsRefPtr<Image>& aImage,
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

   MediaQueue()
     : nsDeque(new MediaQueueDeallocator<T>()),
       mReentrantMonitor("mediaqueue"),
       mEndOfStream(false)
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

  uint32_t FrameCount() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    uint32_t frames = 0;
    for (int32_t i = 0; i < GetSize(); ++i) {
      T* v = static_cast<T*>(ObjectAt(i));
      frames += v->mFrames;
    }
    return frames;
  }

private:
  mutable ReentrantMonitor mReentrantMonitor;

  
  
  bool mEndOfStream;
};





class MediaDecoderReader {
public:
  MediaDecoderReader(AbstractMediaDecoder* aDecoder);
  virtual ~MediaDecoderReader();

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) = 0;

  
  virtual nsresult ResetDecode();

  
  
  
  
  virtual bool DecodeAudioData() = 0;

  
  virtual void PrepareToDecode() { }

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) = 0;

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  
  
  
  virtual nsresult ReadMetadata(VideoInfo* aInfo,
                                MetadataTags** aTags) = 0;

  
  
  
  virtual VideoData* FindStartTime(int64_t& aOutStartTime);

  
  
  
  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime) = 0;
  
  
  
  
  virtual void OnDecodeThreadStart() {}
  
  
  
  
  
  virtual void OnDecodeThreadFinish() {}

protected:
  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

public:
  
  
  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered,
                               int64_t aStartTime) = 0;

  class VideoQueueMemoryFunctor : public nsDequeFunctor {
  public:
    VideoQueueMemoryFunctor() : mResult(0) {}

    virtual void* operator()(void* anObject);

    int64_t mResult;
  };

  virtual int64_t VideoQueueMemoryInUse() {
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

  virtual int64_t AudioQueueMemoryInUse() {
    AudioQueueMemoryFunctor functor;
    mAudioQueue.LockedForEach(functor);
    return functor.mResult;
  }

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {}

  virtual MediaQueue<AudioData>& AudioQueue() { return mAudioQueue; }
  virtual MediaQueue<VideoData>& VideoQueue() { return mVideoQueue; }

  
  AbstractMediaDecoder* GetDecoder() {
    return mDecoder;
  }

  AudioData* DecodeToFirstAudioData();
  VideoData* DecodeToFirstVideoData();

protected:
  
  
  nsresult DecodeToTarget(int64_t aTarget);

  
  AbstractMediaDecoder* mDecoder;

  
  VideoInfo mInfo;
};

} 

#endif
