





#include "MediaDecoderReader.h"
#include "PlatformDecoderModule.h"
#include "nsRect.h"
#include "mozilla/RefPtr.h"
#include "mozilla/CheckedInt.h"
#include "VideoUtils.h"
#include "ImageContainer.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "MediaTaskQueue.h"

namespace mozilla {



template<class BlankMediaDataCreator>
class BlankMediaDataDecoder : public MediaDataDecoder {
public:

  BlankMediaDataDecoder(BlankMediaDataCreator* aCreator,
                        MediaTaskQueue* aTaskQueue,
                        MediaDataDecoderCallback* aCallback)
    : mCreator(aCreator)
    , mTaskQueue(aTaskQueue)
    , mCallback(aCallback)
  {
  }

  virtual nsresult Init() MOZ_OVERRIDE {
    return NS_OK;
  }

  virtual nsresult Shutdown() MOZ_OVERRIDE {
    return NS_OK;
  }

  class OutputEvent : public nsRunnable {
  public:
    OutputEvent(mp4_demuxer::MP4Sample* aSample,
                MediaDataDecoderCallback* aCallback,
                BlankMediaDataCreator* aCreator)
      : mSample(aSample)
      , mCreator(aCreator)
      , mCallback(aCallback)
    {
    }
    NS_IMETHOD Run() MOZ_OVERRIDE
    {
      mCallback->Output(mCreator->Create(mSample->composition_timestamp,
                                         mSample->duration,
                                         mSample->byte_offset));
      return NS_OK;
    }
  private:
    nsAutoPtr<mp4_demuxer::MP4Sample> mSample;
    BlankMediaDataCreator* mCreator;
    MediaDataDecoderCallback* mCallback;
  };

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE
  {
    
    
    
    RefPtr<nsIRunnable> r(new OutputEvent(aSample, mCallback, mCreator));
    mTaskQueue->Dispatch(r);
    return NS_OK;
  }

  virtual nsresult Flush() MOZ_OVERRIDE {
    return NS_OK;
  }

  virtual nsresult Drain() MOZ_OVERRIDE {
    return NS_OK;
  }

private:
  nsAutoPtr<BlankMediaDataCreator> mCreator;
  nsAutoPtr<MediaData> mOutput;
  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
};

class BlankVideoDataCreator {
public:
  BlankVideoDataCreator(uint32_t aFrameWidth,
                        uint32_t aFrameHeight,
                        layers::ImageContainer* aImageContainer)
    : mFrameWidth(aFrameWidth)
    , mFrameHeight(aFrameHeight)
    , mImageContainer(aImageContainer)
  {
    mInfo.mDisplay = nsIntSize(mFrameWidth, mFrameHeight);
    mPicture = gfx::IntRect(0, 0, mFrameWidth, mFrameHeight);
  }

  MediaData* Create(Microseconds aDTS,
                    Microseconds aDuration,
                    int64_t aOffsetInStream)
  {
    
    
    
    
    uint8_t* frame = new uint8_t[mFrameWidth * mFrameHeight];
    memset(frame, 0, mFrameWidth * mFrameHeight);
    VideoData::YCbCrBuffer buffer;

    
    buffer.mPlanes[0].mData = frame;
    buffer.mPlanes[0].mStride = mFrameWidth;
    buffer.mPlanes[0].mHeight = mFrameHeight;
    buffer.mPlanes[0].mWidth = mFrameWidth;
    buffer.mPlanes[0].mOffset = 0;
    buffer.mPlanes[0].mSkip = 0;

    
    buffer.mPlanes[1].mData = frame;
    buffer.mPlanes[1].mStride = mFrameWidth / 2;
    buffer.mPlanes[1].mHeight = mFrameHeight / 2;
    buffer.mPlanes[1].mWidth = mFrameWidth / 2;
    buffer.mPlanes[1].mOffset = 0;
    buffer.mPlanes[1].mSkip = 0;

    
    buffer.mPlanes[2].mData = frame;
    buffer.mPlanes[2].mStride = mFrameWidth / 2;
    buffer.mPlanes[2].mHeight = mFrameHeight / 2;
    buffer.mPlanes[2].mWidth = mFrameWidth / 2;
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
  gfx::IntRect mPicture;
  uint32_t mFrameWidth;
  uint32_t mFrameHeight;
  RefPtr<layers::ImageContainer> mImageContainer;
};


class BlankAudioDataCreator {
public:
  BlankAudioDataCreator(uint32_t aChannelCount,
                        uint32_t aSampleRate,
                        uint16_t aBitsPerSample)
    : mFrameSum(0)
    , mChannelCount(aChannelCount)
    , mSampleRate(aSampleRate)
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
};

class BlankDecoderModule : public PlatformDecoderModule {
public:

  
  virtual nsresult Shutdown() MOZ_OVERRIDE {
    return NS_OK;
  }

  
  virtual MediaDataDecoder* CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                              layers::LayersBackend aLayersBackend,
                                              layers::ImageContainer* aImageContainer,
                                              MediaTaskQueue* aVideoTaskQueue,
                                              MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE {
    BlankVideoDataCreator* decoder = new BlankVideoDataCreator(aConfig.visible_rect().width(),
                                                               aConfig.visible_rect().height(),
                                                               aImageContainer);
    return new BlankMediaDataDecoder<BlankVideoDataCreator>(decoder,
                                                            aVideoTaskQueue,
                                                            aCallback);
  }

  
  virtual MediaDataDecoder* CreateAACDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                             MediaTaskQueue* aAudioTaskQueue,
                                             MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE {
    BlankAudioDataCreator* decoder =
      new BlankAudioDataCreator(ChannelLayoutToChannelCount(aConfig.channel_layout()),
                                aConfig.samples_per_second(),
                                aConfig.bits_per_channel());
    return new BlankMediaDataDecoder<BlankAudioDataCreator>(decoder,
                                                            aAudioTaskQueue,
                                                            aCallback);
  }
};

PlatformDecoderModule* CreateBlankDecoderModule()
{
  return new BlankDecoderModule();
}

} 
