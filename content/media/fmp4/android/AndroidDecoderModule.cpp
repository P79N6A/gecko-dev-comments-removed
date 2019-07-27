



#include "AndroidDecoderModule.h"
#include "PlatformDecoderModule.h"
#include "GeneratedJNIWrappers.h"
#include "GeneratedSDKWrappers.h"
#include "AndroidBridge.h"
#include "MediaTaskQueue.h"
#include "SharedThreadPool.h"
#include "TexturePoolOGL.h"
#include "GLImages.h"

#include "MediaData.h"

#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/DecoderData.h"

#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

#include <jni.h>

using namespace mozilla;
using namespace mozilla::gl;
using namespace mozilla::widget::android;

static MediaCodec* CreateDecoder(JNIEnv* aEnv, const char* aMimeType)
{
  if (!aMimeType) {
    return nullptr;
  }

  nsAutoString mimeType;
  mimeType.AssignASCII(aMimeType);

  jobject decoder = MediaCodec::CreateDecoderByType(mimeType);

  return new MediaCodec(decoder, aEnv);
}

class VideoDataDecoder : public MediaCodecDataDecoder {
public:
  VideoDataDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                   MediaFormat* aFormat, MediaDataDecoderCallback* aCallback,
                   layers::ImageContainer* aImageContainer)
    : MediaCodecDataDecoder(MediaData::Type::VIDEO_FRAME, aConfig.mime_type, aFormat, aCallback)
    , mImageContainer(aImageContainer)
    , mConfig(aConfig)
  {

  }

  nsresult Init() MOZ_OVERRIDE {
    mSurfaceTexture = AndroidSurfaceTexture::Create();
    if (!mSurfaceTexture) {
      printf_stderr("Failed to create SurfaceTexture for video decode\n");
      return NS_ERROR_FAILURE;
    }

    return InitDecoder(mSurfaceTexture->JavaSurface());
  }

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE {
    mp4_demuxer::AnnexB::ConvertSample(aSample, mConfig.annex_b);
    return MediaCodecDataDecoder::Input(aSample);
  }

  virtual nsresult PostOutput(BufferInfo* aInfo, Microseconds aDuration) MOZ_OVERRIDE {
    VideoInfo videoInfo;
    videoInfo.mDisplay = nsIntSize(mConfig.display_width, mConfig.display_height);

    bool isSync = false;
    if (MediaCodec::getBUFFER_FLAG_SYNC_FRAME() & aInfo->getFlags()) {
      isSync = true;
    }

    nsRefPtr<layers::Image> img = mImageContainer->CreateImage(ImageFormat::SURFACE_TEXTURE);
    layers::SurfaceTextureImage::Data data;
    data.mSurfTex = mSurfaceTexture.get();
    data.mSize = gfx::IntSize(mConfig.display_width, mConfig.display_height);
    data.mInverted = true;

    layers::SurfaceTextureImage* typedImg = static_cast<layers::SurfaceTextureImage*>(img.get());
    typedImg->SetData(data);

    mCallback->Output(VideoData::CreateFromImage(videoInfo, mImageContainer, aInfo->getOffset(),
                                                 aInfo->getPresentationTimeUs(),
                                                 aDuration,
                                                 img, isSync,
                                                 aInfo->getPresentationTimeUs(),
                                                 gfx::IntRect(0, 0,
                                                   mConfig.display_width,
                                                   mConfig.display_height)));
    return NS_OK;
  }

protected:
  layers::ImageContainer* mImageContainer;
  const mp4_demuxer::VideoDecoderConfig& mConfig;
  nsRefPtr<AndroidSurfaceTexture> mSurfaceTexture;
};

class AudioDataDecoder : public MediaCodecDataDecoder {
public:
  AudioDataDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                   MediaFormat* aFormat, MediaDataDecoderCallback* aCallback)
  : MediaCodecDataDecoder(MediaData::Type::AUDIO_SAMPLES, aConfig.mime_type, aFormat, aCallback)
  , mConfig(aConfig)
  {
    MOZ_ASSERT(mConfig.bits_per_sample == 16, "We only support 16-bit audio");
  }

  nsresult Output(BufferInfo* aInfo, void* aBuffer, Microseconds aDuration) {
    

    uint32_t numChannels = mConfig.channel_count;
    uint32_t numFrames = (aInfo->getSize() / numChannels) / 2;

    AudioDataValue* audio = new AudioDataValue[aInfo->getSize()];
    PodCopy(audio, static_cast<AudioDataValue*>(aBuffer), aInfo->getSize());

    mCallback->Output(new AudioData(aInfo->getOffset(), aInfo->getPresentationTimeUs(),
                                    aDuration,
                                    numFrames,
                                    audio,
                                    numChannels,
                                    mConfig.samples_per_second));
    return NS_OK;
  }

protected:
  const mp4_demuxer::AudioDecoderConfig& mConfig;
};


bool AndroidDecoderModule::SupportsAudioMimeType(const char* aMimeType) {
  JNIEnv* env = GetJNIForThread();
  MediaCodec* decoder = CreateDecoder(env, aMimeType);
  bool supports = (decoder != nullptr);
  delete decoder;
  return supports;
}

already_AddRefed<MediaDataDecoder>
AndroidDecoderModule::CreateH264Decoder(
                                const mp4_demuxer::VideoDecoderConfig& aConfig,
                                layers::LayersBackend aLayersBackend,
                                layers::ImageContainer* aImageContainer,
                                MediaTaskQueue* aVideoTaskQueue,
                                MediaDataDecoderCallback* aCallback)
{
  nsAutoString mimeType;
  mimeType.AssignASCII(aConfig.mime_type);

  jobject jFormat = MediaFormat::CreateVideoFormat(mimeType,
                                                   aConfig.display_width,
                                                   aConfig.display_height);

  if (!jFormat) {
    return nullptr;
  }

  MediaFormat* format = MediaFormat::Wrap(jFormat);

  if (!format) {
    return nullptr;
  }

  nsRefPtr<MediaDataDecoder> decoder =
    new VideoDataDecoder(aConfig, format, aCallback, aImageContainer);

  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
AndroidDecoderModule::CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                         MediaTaskQueue* aAudioTaskQueue,
                                         MediaDataDecoderCallback* aCallback)
{

  nsAutoString mimeType;
  mimeType.AssignASCII(aConfig.mime_type);

  jobject jFormat = MediaFormat::CreateAudioFormat(mimeType,
                                                   aConfig.samples_per_second,
                                                   aConfig.channel_count);

  if (jFormat == nullptr)
    return nullptr;

  MediaFormat* format = MediaFormat::Wrap(jFormat);

  if(format == nullptr)
    return nullptr;

  JNIEnv* env = GetJNIForThread();

  if (!format->GetByteBuffer(NS_LITERAL_STRING("csd-0"))) {
    uint8_t* csd0 = new uint8_t[2];

    csd0[0] = aConfig.audio_specific_config[0];
    csd0[1] = aConfig.audio_specific_config[1];

    jobject buffer = env->NewDirectByteBuffer(csd0, 2);
    format->SetByteBuffer(NS_LITERAL_STRING("csd-0"), buffer);

    env->DeleteLocalRef(buffer);
  }

  if (mimeType.EqualsLiteral("audio/mp4a-latm")) {
    format->SetInteger(NS_LITERAL_STRING("is-adts"), 1);
  }

  nsRefPtr<MediaDataDecoder> decoder =
    new AudioDataDecoder(aConfig, format, aCallback);

  return decoder.forget();

}


nsresult AndroidDecoderModule::Shutdown()
{
  return NS_OK;
}

MediaCodecDataDecoder::MediaCodecDataDecoder(MediaData::Type aType,
                                             const char* aMimeType,
                                             MediaFormat* aFormat,
                                             MediaDataDecoderCallback* aCallback)
  : mType(aType)
  , mMimeType(strdup(aMimeType))
  , mFormat(aFormat)
  , mCallback(aCallback)
  , mInputBuffers(nullptr)
  , mOutputBuffers(nullptr)
  , mMonitor("MediaCodecDataDecoder::mMonitor")
  , mDraining(false)
  , mStopping(false)
{

}

MediaCodecDataDecoder::~MediaCodecDataDecoder()
{
  JNIEnv* env = GetJNIForThread();

  Shutdown();

  if (mInputBuffers) {
    env->DeleteGlobalRef(mInputBuffers);
    mInputBuffers = nullptr;
  }

  if (mOutputBuffers) {
    env->DeleteGlobalRef(mOutputBuffers);
    mOutputBuffers = nullptr;
  }
}

nsresult MediaCodecDataDecoder::Init()
{
  return InitDecoder();
}

nsresult MediaCodecDataDecoder::InitDecoder(jobject aSurface)
{
  JNIEnv* env = GetJNIForThread();
  mDecoder = CreateDecoder(env, mMimeType);
  if (!mDecoder) {
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  mDecoder->Configure(mFormat->wrappedObject(), aSurface, nullptr, 0);
  mDecoder->Start();

  ResetInputBuffers();
  ResetOutputBuffers();

  NS_NewNamedThread("MC Decoder", getter_AddRefs(mThread),
                    NS_NewRunnableMethod(this, &MediaCodecDataDecoder::DecoderLoop));

  return NS_OK;
}


#define DECODER_TIMEOUT 10000

void MediaCodecDataDecoder::DecoderLoop()
{
  bool outputDone = false;

  JNIEnv* env = GetJNIForThread();
  mp4_demuxer::MP4Sample* sample = nullptr;

  for (;;) {
    {
      MonitorAutoLock lock(mMonitor);
      while (!mStopping && !mDraining && mQueue.empty()) {
        if (mQueue.empty()) {
          
          mCallback->InputExhausted();
        }
        lock.Wait();
      }

      if (mStopping) {
        
        break;
      }

      if (mDraining) {
        mDecoder->Flush();
        ClearQueue();
        mDraining =  false;
        lock.Notify();
        continue;
      }

      
      if (!mQueue.empty()) {
        sample = mQueue.front();
      }
    }

    if (sample) {
      
      int inputIndex = mDecoder->DequeueInputBuffer(DECODER_TIMEOUT);
      if (inputIndex >= 0) {
        jobject buffer = env->GetObjectArrayElement(mInputBuffers, inputIndex);
        void* directBuffer = env->GetDirectBufferAddress(buffer);

        
        mMonitor.Lock();
        mQueue.pop();
        mMonitor.Unlock();

        MOZ_ASSERT(env->GetDirectBufferCapacity(buffer) >= sample->size,
          "Decoder buffer is not large enough for sample");

        PodCopy((uint8_t*)directBuffer, sample->data, sample->size);

        mDecoder->QueueInputBuffer(inputIndex, 0, sample->size, sample->composition_timestamp, 0);
        mDurations.push(sample->duration);

        delete sample;
        sample = nullptr;

        outputDone = false;
        env->DeleteLocalRef(buffer);
      }
    }

    if (!outputDone) {
      BufferInfo bufferInfo;

      int outputStatus = mDecoder->DequeueOutputBuffer(bufferInfo.wrappedObject(), DECODER_TIMEOUT);
      if (outputStatus == MediaCodec::getINFO_TRY_AGAIN_LATER()) {
        
        
      } else if (outputStatus == MediaCodec::getINFO_OUTPUT_BUFFERS_CHANGED()) {
        ResetOutputBuffers();
      } else if (outputStatus == MediaCodec::getINFO_OUTPUT_FORMAT_CHANGED()) {
        
      } else if (outputStatus < 0) {
        printf_stderr("unknown error from decoder! %d\n", outputStatus);
        mCallback->Error();
      } else {
        
        if (bufferInfo.getFlags() & MediaCodec::getBUFFER_FLAG_END_OF_STREAM()) {
          outputDone = true;
        }

        MOZ_ASSERT(!mDurations.empty(), "Should have had a duration queued");

        Microseconds duration = 0;
        if (!mDurations.empty()) {
          duration = mDurations.front();
          mDurations.pop();
        }

        jobject buffer = env->GetObjectArrayElement(mOutputBuffers, outputStatus);
        if (buffer) {
          
          void* directBuffer = env->GetDirectBufferAddress(buffer);
          Output(&bufferInfo, directBuffer, duration);
        }

        
        mDecoder->ReleaseOutputBuffer(outputStatus, true);

        PostOutput(&bufferInfo, duration);

        if (buffer) {
          env->DeleteLocalRef(buffer);
        }
      }
    }
  }

  
  mMonitor.Lock();
  mStopping = false;
  mMonitor.Notify();
  mMonitor.Unlock();
}

void MediaCodecDataDecoder::ClearQueue()
{
  mMonitor.AssertCurrentThreadOwns();
  while (!mQueue.empty()) {
    delete mQueue.front();
    mQueue.pop();
  }
  while (!mDurations.empty()) {
    mDurations.pop();
  }
}

nsresult MediaCodecDataDecoder::Input(mp4_demuxer::MP4Sample* aSample) {
  MonitorAutoLock lock(mMonitor);
  mQueue.push(aSample);
  lock.NotifyAll();

  return NS_OK;
}

void MediaCodecDataDecoder::ResetInputBuffers()
{
  JNIEnv* env = GetJNIForThread();

  if (mInputBuffers) {
    env->DeleteGlobalRef(mInputBuffers);
  }

  mInputBuffers = (jobjectArray) env->NewGlobalRef(mDecoder->GetInputBuffers());
}

void MediaCodecDataDecoder::ResetOutputBuffers()
{
  JNIEnv* env = GetJNIForThread();

  if (mOutputBuffers) {
    env->DeleteGlobalRef(mOutputBuffers);
  }

  mOutputBuffers = (jobjectArray) env->NewGlobalRef(mDecoder->GetOutputBuffers());
}

nsresult MediaCodecDataDecoder::Flush() {
  Drain();
  return NS_OK;
}

nsresult MediaCodecDataDecoder::Drain() {
  MonitorAutoLock lock(mMonitor);
  mDraining = true;
  lock.Notify();

  while (mDraining) {
    lock.Wait();
  }

  mCallback->DrainComplete();
  return NS_OK;
}


nsresult MediaCodecDataDecoder::Shutdown() {
  MonitorAutoLock lock(mMonitor);

  if (!mThread || mStopping) {
    
    return NS_OK;
  }

  mStopping = true;
  lock.Notify();

  while (mStopping) {
    lock.Wait();
  }

  mThread->Shutdown();
  mThread = nullptr;

  mDecoder->Stop();
  mDecoder->Release();
  return NS_OK;

}
