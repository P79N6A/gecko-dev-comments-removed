




#if !defined(MediaData_h)
#define MediaData_h

#include "nsSize.h"
#include "mozilla/gfx/Rect.h"
#include "nsRect.h"
#include "AudioSampleFormat.h"
#include "nsIMemoryReporter.h"
#include "SharedBuffer.h"
#include "nsRefPtr.h"
#include "nsTArray.h"

namespace mozilla {

namespace layers {
class Image;
class ImageContainer;
}


class MediaData {
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaData)

  enum Type {
    AUDIO_DATA = 0,
    VIDEO_DATA
  };

  MediaData(Type aType,
            int64_t aOffset,
            int64_t aTimestamp,
            int64_t aDuration)
    : mType(aType)
    , mOffset(aOffset)
    , mTime(aTimestamp)
    , mDuration(aDuration)
    , mDiscontinuity(false)
  {}

  
  const Type mType;

  
  const int64_t mOffset;

  
  const int64_t mTime;

  
  const int64_t mDuration;

  
  
  bool mDiscontinuity;

  int64_t GetEndTime() const { return mTime + mDuration; }

protected:
  virtual ~MediaData() {}

};


class AudioData : public MediaData {
public:

  AudioData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            uint32_t aFrames,
            AudioDataValue* aData,
            uint32_t aChannels,
            uint32_t aRate)
    : MediaData(AUDIO_DATA, aOffset, aTime, aDuration)
    , mFrames(aFrames)
    , mChannels(aChannels)
    , mRate(aRate)
    , mAudioData(aData) {}

  
  
  
  
  static already_AddRefed<AudioData>
  TransferAndUpdateTimestampAndDuration(AudioData* aOther,
                                        int64_t aTimestamp,
                                        int64_t aDuration);

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  
  void EnsureAudioBuffer();

  const uint32_t mFrames;
  const uint32_t mChannels;
  const uint32_t mRate;
  
  
  nsRefPtr<SharedBuffer> mAudioBuffer;
  
  nsAutoArrayPtr<AudioDataValue> mAudioData;

protected:
  ~AudioData() {}
};

namespace layers {
class TextureClient;
class PlanarYCbCrImage;
}

class VideoInfo;


class VideoData : public MediaData {
public:
  typedef gfx::IntRect IntRect;
  typedef gfx::IntSize IntSize;
  typedef layers::ImageContainer ImageContainer;
  typedef layers::Image Image;
  typedef layers::PlanarYCbCrImage PlanarYCbCrImage;

  
  
  
  
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

  
  
  
  
  
  
  
  
  static already_AddRefed<VideoData> Create(VideoInfo& aInfo,
                                            ImageContainer* aContainer,
                                            Image* aImage,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            const YCbCrBuffer &aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  
  static already_AddRefed<VideoData> Create(VideoInfo& aInfo,
                                            ImageContainer* aContainer,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            const YCbCrBuffer &aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  
  static already_AddRefed<VideoData> Create(VideoInfo& aInfo,
                                            Image* aImage,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            const YCbCrBuffer &aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  static already_AddRefed<VideoData> Create(VideoInfo& aInfo,
                                             ImageContainer* aContainer,
                                             int64_t aOffset,
                                             int64_t aTime,
                                             int64_t aDuration,
                                             layers::TextureClient* aBuffer,
                                             bool aKeyframe,
                                             int64_t aTimecode,
                                             const IntRect& aPicture);

  static already_AddRefed<VideoData> CreateFromImage(VideoInfo& aInfo,
                                                     ImageContainer* aContainer,
                                                     int64_t aOffset,
                                                     int64_t aTime,
                                                     int64_t aDuration,
                                                     const nsRefPtr<Image>& aImage,
                                                     bool aKeyframe,
                                                     int64_t aTimecode,
                                                     const IntRect& aPicture);

  
  
  
  
  
  
  
  static already_AddRefed<VideoData> ShallowCopyUpdateDuration(VideoData* aOther,
                                                               int64_t aDuration);

  
  
  
  static already_AddRefed<VideoData> ShallowCopyUpdateTimestamp(VideoData* aOther,
                                                                int64_t aTimestamp);

  
  
  
  static already_AddRefed<VideoData>
  ShallowCopyUpdateTimestampAndDuration(VideoData* aOther, int64_t aTimestamp,
                                        int64_t aDuration);

  
  
  static void SetVideoDataToImage(PlanarYCbCrImage* aVideoImage,
                                  VideoInfo& aInfo,
                                  const YCbCrBuffer &aBuffer,
                                  const IntRect& aPicture,
                                  bool aCopyData);

  
  
  
  static already_AddRefed<VideoData> CreateDuplicate(int64_t aOffset,
                                                     int64_t aTime,
                                                     int64_t aDuration,
                                                     int64_t aTimecode)
  {
    nsRefPtr<VideoData> rv = new VideoData(aOffset, aTime, aDuration, aTimecode);
    return rv.forget();
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  
  
  
  const IntSize mDisplay;

  
  
  const int64_t mTimecode;

  
  nsRefPtr<Image> mImage;

  
  
  const bool mDuplicate;
  const bool mKeyframe;

  VideoData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            int64_t aTimecode);

  VideoData(int64_t aOffset,
            int64_t aTime,
            int64_t aDuration,
            bool aKeyframe,
            int64_t aTimecode,
            IntSize aDisplay);

protected:
  ~VideoData();
};

  
  
class LargeDataBuffer : public FallibleTArray<uint8_t> {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(LargeDataBuffer);

private:
  ~LargeDataBuffer() {}
};

} 

#endif 
