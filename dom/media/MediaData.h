




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

class MediaLargeByteBuffer;
class MediaByteBuffer;


class MediaData {
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaData)

  enum Type {
    AUDIO_DATA = 0,
    VIDEO_DATA,
    RAW_DATA
  };

  MediaData(Type aType,
            int64_t aOffset,
            int64_t aTimestamp,
            int64_t aDuration)
    : mType(aType)
    , mOffset(aOffset)
    , mTime(aTimestamp)
    , mTimecode(aTimestamp)
    , mDuration(aDuration)
    , mKeyframe(false)
    , mDiscontinuity(false)
  {}

  
  const Type mType;

  
  int64_t mOffset;

  
  int64_t mTime;

  
  
  int64_t mTimecode;

  
  int64_t mDuration;

  bool mKeyframe;

  
  
  bool mDiscontinuity;

  int64_t GetEndTime() const { return mTime + mDuration; }

protected:
  explicit MediaData(Type aType)
    : mType(aType)
    , mOffset(0)
    , mTime(0)
    , mTimecode(0)
    , mDuration(0)
    , mKeyframe(false)
    , mDiscontinuity(false)
  {}

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

  
  
  
  
  
  
  
  
  static already_AddRefed<VideoData> Create(const VideoInfo& aInfo,
                                            ImageContainer* aContainer,
                                            Image* aImage,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            const YCbCrBuffer &aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  
  static already_AddRefed<VideoData> Create(const VideoInfo& aInfo,
                                            ImageContainer* aContainer,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            const YCbCrBuffer &aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  
  static already_AddRefed<VideoData> Create(const VideoInfo& aInfo,
                                            Image* aImage,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            const YCbCrBuffer &aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  static already_AddRefed<VideoData> Create(const VideoInfo& aInfo,
                                            ImageContainer* aContainer,
                                            int64_t aOffset,
                                            int64_t aTime,
                                            int64_t aDuration,
                                            layers::TextureClient* aBuffer,
                                            bool aKeyframe,
                                            int64_t aTimecode,
                                            const IntRect& aPicture);

  static already_AddRefed<VideoData> CreateFromImage(const VideoInfo& aInfo,
                                                     ImageContainer* aContainer,
                                                     int64_t aOffset,
                                                     int64_t aTime,
                                                     int64_t aDuration,
                                                     const nsRefPtr<Image>& aImage,
                                                     bool aKeyframe,
                                                     int64_t aTimecode,
                                                     const IntRect& aPicture);

  
  
  
  
  
  
  
  static already_AddRefed<VideoData> ShallowCopyUpdateDuration(const VideoData* aOther,
                                                               int64_t aDuration);

  
  
  
  static already_AddRefed<VideoData> ShallowCopyUpdateTimestamp(const VideoData* aOther,
                                                                int64_t aTimestamp);

  
  
  
  static already_AddRefed<VideoData>
  ShallowCopyUpdateTimestampAndDuration(const VideoData* aOther, int64_t aTimestamp,
                                        int64_t aDuration);

  
  
  static void SetVideoDataToImage(PlanarYCbCrImage* aVideoImage,
                                  const VideoInfo& aInfo,
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

  
  nsRefPtr<Image> mImage;

  
  
  const bool mDuplicate;

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

class CryptoTrack
{
public:
  CryptoTrack() : mValid(false) {}
  bool mValid;
  int32_t mMode;
  int32_t mIVSize;
  nsTArray<uint8_t> mKeyId;
};

class CryptoSample : public CryptoTrack
{
public:
  nsTArray<uint16_t> mPlainSizes;
  nsTArray<uint32_t> mEncryptedSizes;
  nsTArray<uint8_t> mIV;
  nsTArray<nsCString> mSessionIds;
};






















class MediaRawData;

class MediaRawDataWriter
{
public:
  
  uint8_t* mData;
  
  size_t mSize;
  
  CryptoSample& mCrypto;

  

  
  
  bool SetSize(size_t aSize);
  
  bool Prepend(const uint8_t* aData, size_t aSize);
  
  bool Replace(const uint8_t* aData, size_t aSize);
  
  void Clear();

private:
  friend class MediaRawData;
  explicit MediaRawDataWriter(MediaRawData* aMediaRawData);
  bool EnsureSize(size_t aSize);
  MediaRawData* mTarget;
  nsRefPtr<MediaLargeByteBuffer> mBuffer;
};

class MediaRawData : public MediaData {
public:
  MediaRawData();
  MediaRawData(const uint8_t* aData, size_t mSize);

  
  const uint8_t* mData;
  
  size_t mSize;

  const CryptoSample& mCrypto;
  nsRefPtr<MediaByteBuffer> mExtraData;

  
  virtual already_AddRefed<MediaRawData> Clone() const;
  
  
  virtual MediaRawDataWriter* CreateWriter();
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

protected:
  ~MediaRawData();

private:
  friend class MediaRawDataWriter;
  
  
  
  
  
  bool EnsureCapacity(size_t aSize);
  nsRefPtr<MediaLargeByteBuffer> mBuffer;
  CryptoSample mCryptoInternal;
  uint32_t mPadding;
  MediaRawData(const MediaRawData&); 
};

  
  
class MediaLargeByteBuffer : public FallibleTArray<uint8_t> {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaLargeByteBuffer);
  MediaLargeByteBuffer() = default;
  explicit MediaLargeByteBuffer(size_t aCapacity) : FallibleTArray<uint8_t>(aCapacity) {}

private:
  ~MediaLargeByteBuffer() {}
};

  
class MediaByteBuffer : public nsTArray<uint8_t> {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaByteBuffer);

private:
  ~MediaByteBuffer() {}
};

} 

#endif 
