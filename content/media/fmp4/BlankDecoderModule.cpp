





#include "MediaDecoderReader.h"
#include "PlatformDecoderModule.h"
#include "nsRect.h"
#include "mozilla/RefPtr.h"
#include "mozilla/CheckedInt.h"
#include "VideoUtils.h"
#include "ImageContainer.h"

namespace mozilla {



template<class BlankMediaDataCreator>
class BlankMediaDataDecoder : public MediaDataDecoder {
public:

  BlankMediaDataDecoder(BlankMediaDataCreator* aCreator)
    : mCreator(aCreator),
      mNextDTS(-1),
      mNextOffset(-1)
  {
  }

  virtual nsresult Shutdown() MOZ_OVERRIDE {
    return NS_OK;
  }

  virtual DecoderStatus Input(const uint8_t* aData,
                              uint32_t aLength,
                              Microseconds aDTS,
                              Microseconds aPTS,
                              int64_t aOffsetInStream) MOZ_OVERRIDE
  {
    
    
    if (mOutput) {
      return DECODE_STATUS_NOT_ACCEPTING;
    }
    if (mNextDTS != -1 && mNextOffset != -1) {
      Microseconds duration = aDTS - mNextDTS;
      mOutput = mCreator->Create(mNextDTS, duration, mNextOffset);
    }

    mNextDTS = aDTS;
    mNextOffset = aOffsetInStream;
    return DECODE_STATUS_OK;
  }

  virtual DecoderStatus Output(nsAutoPtr<MediaData>& aOutData) MOZ_OVERRIDE
  {
    if (!mOutput) {
      return DECODE_STATUS_NEED_MORE_INPUT;
    }
    aOutData = mOutput.forget();
    return DECODE_STATUS_OK;
  }

  virtual DecoderStatus Flush()  MOZ_OVERRIDE {
    return DECODE_STATUS_OK;
  }
private:
  nsAutoPtr<BlankMediaDataCreator> mCreator;
  Microseconds mNextDTS;
  int64_t mNextOffset;
  nsAutoPtr<MediaData> mOutput;
  bool mHasInput;
};

static const uint32_t sFrameWidth = 320;
static const uint32_t sFrameHeight = 240;

class BlankVideoDataCreator {
public:
  BlankVideoDataCreator(layers::ImageContainer* aImageContainer)
    : mImageContainer(aImageContainer)
  {
    mInfo.mDisplay = nsIntSize(sFrameWidth, sFrameHeight);
    mPicture = nsIntRect(0, 0, sFrameWidth, sFrameHeight);
  }

  MediaData* Create(Microseconds aDTS,
                    Microseconds aDuration,
                    int64_t aOffsetInStream)
  {
    
    
    
    
    uint8_t* frame = new uint8_t[sFrameWidth * sFrameHeight];
    memset(frame, 0, sFrameWidth * sFrameHeight);
    VideoData::YCbCrBuffer buffer;

    
    buffer.mPlanes[0].mData = frame;
    buffer.mPlanes[0].mStride = sFrameWidth;
    buffer.mPlanes[0].mHeight = sFrameHeight;
    buffer.mPlanes[0].mWidth = sFrameWidth;
    buffer.mPlanes[0].mOffset = 0;
    buffer.mPlanes[0].mSkip = 0;

    
    buffer.mPlanes[1].mData = frame;
    buffer.mPlanes[1].mStride = sFrameWidth / 2;
    buffer.mPlanes[1].mHeight = sFrameHeight / 2;
    buffer.mPlanes[1].mWidth = sFrameWidth / 2;
    buffer.mPlanes[1].mOffset = 0;
    buffer.mPlanes[1].mSkip = 0;

    
    buffer.mPlanes[2].mData = frame;
    buffer.mPlanes[2].mStride = sFrameWidth / 2;
    buffer.mPlanes[2].mHeight = sFrameHeight / 2;
    buffer.mPlanes[2].mWidth = sFrameWidth / 2;
    buffer.mPlanes[2].mOffset = 0;
    buffer.mPlanes[2].mSkip = 0;

    return VideoData::Create(mInfo,
                             mImageContainer,
                             nullptr,
                             aOffsetInStream,
                             aDTS,
                             aDuration,
                             buffer,
                             true,
                             aDTS,
                             mPicture);
  }
private:
  VideoInfo mInfo;
  nsIntRect mPicture;
  RefPtr<layers::ImageContainer> mImageContainer;
};


class BlankAudioDataCreator {
public:
  BlankAudioDataCreator(uint32_t aChannelCount,
                        uint32_t aSampleRate,
                        uint16_t aBitsPerSample)
    : mFrameSum(0),
      mChannelCount(aChannelCount),
      mSampleRate(aSampleRate),
      mBitsPerSample(aBitsPerSample)
  {
  }

  MediaData* Create(Microseconds aDTS,
                    Microseconds aDuration,
                    int64_t aOffsetInStream)
  {
    
    
    CheckedInt64 frames = UsecsToFrames(aDuration+1, mSampleRate);
    if (!frames.isValid() ||
        !mChannelCount ||
        !mSampleRate ||
        frames.value() > (UINT32_MAX / mChannelCount)) {
      return nullptr;
    }
    AudioDataValue* samples = new AudioDataValue[frames.value() * mChannelCount];
    
    static const float pi = 3.14159265f;
    static const float noteHz = 440.0f;
    for (int i = 0; i < frames.value(); i++) {
      float f = sin(2 * pi * noteHz * mFrameSum / mSampleRate);
      for (unsigned c = 0; c < mChannelCount; c++) {
        samples[i * mChannelCount + c] = AudioDataValue(f);
      }
      mFrameSum++;
    }
    return new AudioData(aOffsetInStream,
                         aDTS,
                         aDuration,
                         uint32_t(frames.value()),
                         samples,
                         mChannelCount);
  }

private:
  int64_t mFrameSum;
  uint32_t mChannelCount;
  uint32_t mSampleRate;
  uint16_t mBitsPerSample;
};

class BlankDecoderModule : public PlatformDecoderModule {
public:

  
  virtual nsresult Shutdown() MOZ_OVERRIDE {
    return NS_OK;
  }

  
  virtual MediaDataDecoder* CreateVideoDecoder(layers::LayersBackend aLayersBackend,
                                               layers::ImageContainer* aImageContainer) MOZ_OVERRIDE {
    BlankVideoDataCreator* decoder = new BlankVideoDataCreator(aImageContainer);
    return new BlankMediaDataDecoder<BlankVideoDataCreator>(decoder);
  }

  
  virtual MediaDataDecoder* CreateAudioDecoder(uint32_t aChannelCount,
                                               uint32_t aSampleRate,
                                               uint16_t aBitsPerSample,
                                               const uint8_t* aUserData,
                                               uint32_t aUserDataLength) MOZ_OVERRIDE {
    BlankAudioDataCreator* decoder = new BlankAudioDataCreator(aChannelCount,
                                                               aSampleRate,
                                                               aBitsPerSample);
    return new BlankMediaDataDecoder<BlankAudioDataCreator>(decoder);
  }
};

PlatformDecoderModule* CreateBlankDecoderModule()
{
  return new BlankDecoderModule();
}

} 
