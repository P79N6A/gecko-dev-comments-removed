



#include "AndroidDecoderModule.h"
#include "AndroidBridge.h"
#include "GLImages.h"

#include "MediaData.h"

#include "mp4_demuxer/Adts.h"
#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/DecoderData.h"

#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

#include <jni.h>
#include <string.h>

using namespace mozilla;
using namespace mozilla::gl;
using namespace mozilla::widget::android::sdk;

namespace mozilla {

static MediaCodec* CreateDecoder(JNIEnv* aEnv, const char* aMimeType)
{
  if (!aMimeType) {
    return nullptr;
  }

  jobject decoder = MediaCodec::CreateDecoderByType(nsCString(aMimeType));

  return new MediaCodec(decoder, aEnv);
}

class VideoDataDecoder : public MediaCodecDataDecoder {
public:
  VideoDataDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                   MediaFormat* aFormat, MediaDataDecoderCallback* aCallback,
                   layers::ImageContainer* aImageContainer)
    : MediaCodecDataDecoder(MediaData::Type::VIDEO_DATA, aConfig.mime_type, aFormat, aCallback)
    , mImageContainer(aImageContainer)
    , mConfig(aConfig)
  {

  }

  nsresult Init() MOZ_OVERRIDE {
    mSurfaceTexture = AndroidSurfaceTexture::Create();
    if (!mSurfaceTexture) {
      NS_WARNING("Failed to create SurfaceTexture for video decode\n");
      return NS_ERROR_FAILURE;
    }

    return InitDecoder(mSurfaceTexture->JavaSurface());
  }

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE {
    mp4_demuxer::AnnexB::ConvertSample(aSample);
    return MediaCodecDataDecoder::Input(aSample);
  }

  virtual nsresult PostOutput(BufferInfo* aInfo, MediaFormat* aFormat, Microseconds aDuration) MOZ_OVERRIDE {
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
  RefPtr<AndroidSurfaceTexture> mSurfaceTexture;
};

class AudioDataDecoder : public MediaCodecDataDecoder {
public:
  AudioDataDecoder(const char* aMimeType, MediaFormat* aFormat, MediaDataDecoderCallback* aCallback)
  : MediaCodecDataDecoder(MediaData::Type::AUDIO_DATA, aMimeType, aFormat, aCallback)
  {
  }

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE {
    if (!strcmp(mMimeType, "audio/mp4a-latm")) {
      uint32_t numChannels = mFormat->GetInteger(NS_LITERAL_CSTRING("channel-count"));
      uint32_t sampleRate = mFormat->GetInteger(NS_LITERAL_CSTRING("sample-rate"));
      uint8_t frequencyIndex =
          mp4_demuxer::Adts::GetFrequencyIndex(sampleRate);
      uint32_t aacProfile = mFormat->GetInteger(NS_LITERAL_CSTRING("aac-profile"));
      bool rv = mp4_demuxer::Adts::ConvertSample(numChannels,
                                                 frequencyIndex,
                                                 aacProfile,
                                                 aSample);
      if (!rv) {
        NS_WARNING("Failed to prepend ADTS header\n");
        return NS_ERROR_FAILURE;
      }
    }
    return MediaCodecDataDecoder::Input(aSample);
  }

  nsresult Output(BufferInfo* aInfo, void* aBuffer, MediaFormat* aFormat, Microseconds aDuration) {
    

    uint32_t numChannels = aFormat->GetInteger(NS_LITERAL_CSTRING("channel-count"));
    uint32_t sampleRate = aFormat->GetInteger(NS_LITERAL_CSTRING("sample-rate"));
    uint32_t numFrames = (aInfo->getSize() / numChannels) / 2;

    AudioDataValue* audio = new AudioDataValue[aInfo->getSize()];
    PodCopy(audio, static_cast<AudioDataValue*>(aBuffer), aInfo->getSize());

    mCallback->Output(new AudioData(aInfo->getOffset(), aInfo->getPresentationTimeUs(),
                                    aDuration,
                                    numFrames,
                                    audio,
                                    numChannels,
                                    sampleRate));
    return NS_OK;
  }
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
  jobject jFormat = MediaFormat::CreateVideoFormat(nsCString(aConfig.mime_type),
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
  MOZ_ASSERT(aConfig.bits_per_sample == 16, "We only handle 16-bit audio!");

  jobject jFormat = MediaFormat::CreateAudioFormat(nsCString(aConfig.mime_type),
                                                   aConfig.samples_per_second,
                                                   aConfig.channel_count);

  if (jFormat == nullptr)
    return nullptr;

  MediaFormat* format = MediaFormat::Wrap(jFormat);

  if(format == nullptr)
    return nullptr;

  JNIEnv* env = GetJNIForThread();

  if (!format->GetByteBuffer(NS_LITERAL_CSTRING("csd-0"))) {
    uint8_t* csd0 = new uint8_t[2];

    csd0[0] = aConfig.audio_specific_config[0];
    csd0[1] = aConfig.audio_specific_config[1];

    jobject buffer = env->NewDirectByteBuffer(csd0, 2);
    format->SetByteBuffer(NS_LITERAL_CSTRING("csd-0"), buffer);

    env->DeleteLocalRef(buffer);
  }

  if (strcmp(aConfig.mime_type, "audio/mp4a-latm") == 0) {
    format->SetInteger(NS_LITERAL_CSTRING("is-adts"), 1);
    format->SetInteger(NS_LITERAL_CSTRING("aac-profile"), aConfig.aac_profile);
  }

  nsRefPtr<MediaDataDecoder> decoder =
    new AudioDataDecoder(aConfig.mime_type, format, aCallback);

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
  , mFlushing(false)
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

  nsresult res;
  mDecoder->Configure(mFormat->wrappedObject(), aSurface, nullptr, 0, &res);
  if (NS_FAILED(res)) {
    return res;
  }

  mDecoder->Start(&res);
  if (NS_FAILED(res)) {
    return res;
  }

  res = ResetInputBuffers();
  if (NS_FAILED(res)) {
    return res;
  }

  res = ResetOutputBuffers();
  if (NS_FAILED(res)) {
    return res;
  }

  NS_NewNamedThread("MC Decoder", getter_AddRefs(mThread),
                    NS_NewRunnableMethod(this, &MediaCodecDataDecoder::DecoderLoop));

  return NS_OK;
}


#define DECODER_TIMEOUT 10000

#define HANDLE_DECODER_ERROR() \
  if (NS_FAILED(res)) { \
    NS_WARNING("exiting decoder loop due to exception"); \
    mCallback->Error(); \
    break; \
  }

void MediaCodecDataDecoder::DecoderLoop()
{
  bool outputDone = false;

  bool draining = false;
  bool waitingEOF = false;

  JNIEnv* env = GetJNIForThread();
  mp4_demuxer::MP4Sample* sample = nullptr;

  nsAutoPtr<MediaFormat> outputFormat;
  nsresult res;

  for (;;) {
    {
      MonitorAutoLock lock(mMonitor);
      while (!mStopping && !mDraining && !mFlushing && mQueue.empty()) {
        if (mQueue.empty()) {
          
          mCallback->InputExhausted();
        }
        lock.Wait();
      }

      if (mStopping) {
        
        break;
      }

      if (mFlushing) {
        mDecoder->Flush();
        ClearQueue();
        mFlushing =  false;
        lock.Notify();
        continue;
      }

      if (mDraining && !sample && !waitingEOF) {
        draining = true;
      }

      
      if (!mQueue.empty()) {
        sample = mQueue.front();
      }
    }

    if (draining && !waitingEOF) {
      MOZ_ASSERT(!sample, "Shouldn't have a sample when pushing EOF frame");

      int inputIndex = mDecoder->DequeueInputBuffer(DECODER_TIMEOUT, &res);
      HANDLE_DECODER_ERROR();

      if (inputIndex >= 0) {
        mDecoder->QueueInputBuffer(inputIndex, 0, 0, 0, MediaCodec::getBUFFER_FLAG_END_OF_STREAM(), &res);
        waitingEOF = true;
      }
    }

    if (sample) {
      
      int inputIndex = mDecoder->DequeueInputBuffer(DECODER_TIMEOUT, &res);
      HANDLE_DECODER_ERROR();

      if (inputIndex >= 0) {
        jobject buffer = env->GetObjectArrayElement(mInputBuffers, inputIndex);
        void* directBuffer = env->GetDirectBufferAddress(buffer);

        
        mMonitor.Lock();
        mQueue.pop();
        mMonitor.Unlock();

        MOZ_ASSERT(env->GetDirectBufferCapacity(buffer) >= sample->size,
          "Decoder buffer is not large enough for sample");

        PodCopy((uint8_t*)directBuffer, sample->data, sample->size);

        mDecoder->QueueInputBuffer(inputIndex, 0, sample->size, sample->composition_timestamp, 0, &res);
        HANDLE_DECODER_ERROR();

        mDurations.push(sample->duration);

        delete sample;
        sample = nullptr;

        outputDone = false;
        env->DeleteLocalRef(buffer);
      }
    }

    if (!outputDone) {
      BufferInfo bufferInfo;

      int outputStatus = mDecoder->DequeueOutputBuffer(bufferInfo.wrappedObject(), DECODER_TIMEOUT, &res);
      HANDLE_DECODER_ERROR();

      if (outputStatus == MediaCodec::getINFO_TRY_AGAIN_LATER()) {
        
        
      } else if (outputStatus == MediaCodec::getINFO_OUTPUT_BUFFERS_CHANGED()) {
        res = ResetOutputBuffers();
        HANDLE_DECODER_ERROR();
      } else if (outputStatus == MediaCodec::getINFO_OUTPUT_FORMAT_CHANGED()) {
        outputFormat = new MediaFormat(mDecoder->GetOutputFormat(), GetJNIForThread());
      } else if (outputStatus < 0) {
        NS_WARNING("unknown error from decoder!");
        mCallback->Error();

        
        
      } else {
        
        if (bufferInfo.getFlags() & MediaCodec::getBUFFER_FLAG_END_OF_STREAM()) {
          if (draining) {
            draining = false;
            waitingEOF = false;

            mMonitor.Lock();
            mDraining = false;
            mMonitor.Notify();
            mMonitor.Unlock();

            mCallback->DrainComplete();
          }

          mDecoder->ReleaseOutputBuffer(outputStatus, false);
          outputDone = true;

          
          continue;
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
          Output(&bufferInfo, directBuffer, outputFormat, duration);
        }

        
        mDecoder->ReleaseOutputBuffer(outputStatus, true);

        PostOutput(&bufferInfo, outputFormat, duration);

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

nsresult MediaCodecDataDecoder::ResetInputBuffers()
{
  JNIEnv* env = GetJNIForThread();

  if (mInputBuffers) {
    env->DeleteGlobalRef(mInputBuffers);
  }

  nsresult res;
  mInputBuffers = (jobjectArray) env->NewGlobalRef(mDecoder->GetInputBuffers(&res));
  if (NS_FAILED(res)) {
    return res;
  }

  return NS_OK;
}

nsresult MediaCodecDataDecoder::ResetOutputBuffers()
{
  JNIEnv* env = GetJNIForThread();

  if (mOutputBuffers) {
    env->DeleteGlobalRef(mOutputBuffers);
  }

  nsresult res;
  mOutputBuffers = (jobjectArray) env->NewGlobalRef(mDecoder->GetOutputBuffers(&res));
  if (NS_FAILED(res)) {
    return res;
  }

  return NS_OK;
}

nsresult MediaCodecDataDecoder::Flush() {
  MonitorAutoLock lock(mMonitor);
  mFlushing = true;
  lock.Notify();

  while (mFlushing) {
    lock.Wait();
  }

  return NS_OK;
}

nsresult MediaCodecDataDecoder::Drain() {
  MonitorAutoLock lock(mMonitor);
  if (mDraining) {
    return NS_OK;
  }

  mDraining = true;
  lock.Notify();

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

} 
