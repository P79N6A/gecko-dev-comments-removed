




#if !defined(MediaData_h)
#define MediaData_h

#include "nsSize.h"
#include "nsRect.h"
#include "AudioSampleFormat.h"
#include "nsIMemoryReporter.h"
#include "SharedBuffer.h"

namespace mozilla {

namespace layers {
class Image;
class ImageContainer;
}


class MediaData {
public:

  enum Type {
    AUDIO_SAMPLES = 0,
    VIDEO_FRAME = 1
  };

  MediaData(Type aType,
            int64_t aOffset,
            int64_t aTimestamp,
            int64_t aDuration)
    : mType(aType)
    , mOffset(aOffset)
    , mTime(aTimestamp)
    , mDuration(aDuration)
  {}

  virtual ~MediaData() {}

  
  const Type mType;

  
  const int64_t mOffset;

  
  const int64_t mTime;

  
  const int64_t mDuration;

  int64_t GetEndTime() const { return mTime + mDuration; }

};


class AudioData : public MediaData {
public:

  AudioData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            uint32_t aFrames,
            AudioDataValue* aData,
            uint32_t aChannels)
    : MediaData(AUDIO_SAMPLES, aOffset, aTime, aDuration)
    , mFrames(aFrames)
    , mChannels(aChannels)
    , mAudioData(aData)
  {
    MOZ_COUNT_CTOR(AudioData);
  }

  ~AudioData()
  {
    MOZ_COUNT_DTOR(AudioData);
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    size_t size = aMallocSizeOf(this) + aMallocSizeOf(mAudioData);
    if (mAudioBuffer) {
      size += mAudioBuffer->SizeOfIncludingThis(aMallocSizeOf);
    }
    return size;
  }

  
  void EnsureAudioBuffer();

  const uint32_t mFrames;
  const uint32_t mChannels;
  
  
  nsRefPtr<SharedBuffer> mAudioBuffer;
  
  nsAutoArrayPtr<AudioDataValue> mAudioData;
};

namespace layers {
class GraphicBufferLocked;
}

class VideoInfo;


class VideoData : public MediaData {
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
                           int64_t aDuration,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  
  static VideoData* Create(VideoInfo& aInfo,
                           ImageContainer* aContainer,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aDuration,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  
  static VideoData* Create(VideoInfo& aInfo,
                           Image* aImage,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aDuration,
                           const YCbCrBuffer &aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  static VideoData* Create(VideoInfo& aInfo,
                           ImageContainer* aContainer,
                           int64_t aOffset,
                           int64_t aTime,
                           int64_t aDuration,
                           layers::GraphicBufferLocked* aBuffer,
                           bool aKeyframe,
                           int64_t aTimecode,
                           nsIntRect aPicture);

  static VideoData* CreateFromImage(VideoInfo& aInfo,
                                    ImageContainer* aContainer,
                                    int64_t aOffset,
                                    int64_t aTime,
                                    int64_t aDuration,
                                    const nsRefPtr<Image>& aImage,
                                    bool aKeyframe,
                                    int64_t aTimecode,
                                    nsIntRect aPicture);

  
  
  
  
  
  
  
  static VideoData* ShallowCopyUpdateDuration(VideoData* aOther,
                                              int64_t aDuration);

  
  
  
  static VideoData* CreateDuplicate(int64_t aOffset,
                                    int64_t aTime,
                                    int64_t aDuration,
                                    int64_t aTimecode)
  {
    return new VideoData(aOffset, aTime, aDuration, aTimecode);
  }

  ~VideoData();

  
  
  
  const nsIntSize mDisplay;

  
  
  const int64_t mTimecode;

  
  nsRefPtr<Image> mImage;

  
  
  const bool mDuplicate;
  const bool mKeyframe;

public:
  VideoData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            int64_t aTimecode);

  VideoData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            bool aKeyframe,
            int64_t aTimecode,
            nsIntSize aDisplay);

};

} 

#endif
