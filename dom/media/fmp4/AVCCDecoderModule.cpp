





#include "AVCCDecoderModule.h"
#include "ImageContainer.h"
#include "MediaTaskQueue.h"
#include "mp4_demuxer/DecoderData.h"
#include "mp4_demuxer/AnnexB.h"

namespace mozilla
{

class AVCCMediaDataDecoder : public MediaDataDecoder {
public:

  AVCCMediaDataDecoder(PlatformDecoderModule* aPDM,
                       const mp4_demuxer::VideoDecoderConfig& aConfig,
                       layers::LayersBackend aLayersBackend,
                       layers::ImageContainer* aImageContainer,
                       FlushableMediaTaskQueue* aVideoTaskQueue,
                       MediaDataDecoderCallback* aCallback);

  virtual ~AVCCMediaDataDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;
  virtual bool IsWaitingMediaResources() MOZ_OVERRIDE;
  virtual bool IsDormantNeeded() MOZ_OVERRIDE;
  virtual void AllocateMediaResources() MOZ_OVERRIDE;
  virtual void ReleaseMediaResources() MOZ_OVERRIDE;
  virtual void ReleaseDecoder() MOZ_OVERRIDE;
  virtual bool IsHardwareAccelerated() const MOZ_OVERRIDE;

private:
  
  
  
  nsresult CreateDecoder();
  nsresult CreateDecoderAndInit(mp4_demuxer::MP4Sample* aSample);

  nsRefPtr<PlatformDecoderModule> mPDM;
  mp4_demuxer::VideoDecoderConfig mCurrentConfig;
  layers::LayersBackend mLayersBackend;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  nsRefPtr<FlushableMediaTaskQueue> mVideoTaskQueue;
  MediaDataDecoderCallback* mCallback;
  nsRefPtr<MediaDataDecoder> mDecoder;
  nsresult mLastError;
};

AVCCMediaDataDecoder::AVCCMediaDataDecoder(PlatformDecoderModule* aPDM,
                                           const mp4_demuxer::VideoDecoderConfig& aConfig,
                                           layers::LayersBackend aLayersBackend,
                                           layers::ImageContainer* aImageContainer,
                                           FlushableMediaTaskQueue* aVideoTaskQueue,
                                           MediaDataDecoderCallback* aCallback)
  : mPDM(aPDM)
  , mCurrentConfig(aConfig)
  , mLayersBackend(aLayersBackend)
  , mImageContainer(aImageContainer)
  , mVideoTaskQueue(aVideoTaskQueue)
  , mCallback(aCallback)
  , mDecoder(nullptr)
  , mLastError(NS_OK)
{
  CreateDecoder();
}

AVCCMediaDataDecoder::~AVCCMediaDataDecoder()
{
}

nsresult
AVCCMediaDataDecoder::Init()
{
  if (mDecoder) {
    return mDecoder->Init();
  }
  return mLastError;
}

nsresult
AVCCMediaDataDecoder::Input(mp4_demuxer::MP4Sample* aSample)
{
  if (!mp4_demuxer::AnnexB::ConvertSampleToAVCC(aSample)) {
    return NS_ERROR_FAILURE;
  }
  if (!mDecoder) {
    
    
    
    nsresult rv = CreateDecoderAndInit(aSample);
    if (rv == NS_ERROR_NOT_INITIALIZED) {
      
      
      return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  aSample->extra_data = mCurrentConfig.extra_data;

  return mDecoder->Input(aSample);
}

nsresult
AVCCMediaDataDecoder::Flush()
{
  if (mDecoder) {
    return mDecoder->Flush();
  }
  return mLastError;
}

nsresult
AVCCMediaDataDecoder::Drain()
{
  if (mDecoder) {
    return mDecoder->Drain();
  }
  return mLastError;
}

nsresult
AVCCMediaDataDecoder::Shutdown()
{
  if (mDecoder) {
    return mDecoder->Shutdown();
  }
  return NS_OK;
}

bool
AVCCMediaDataDecoder::IsWaitingMediaResources()
{
  if (mDecoder) {
    return mDecoder->IsWaitingMediaResources();
  }
  return MediaDataDecoder::IsWaitingMediaResources();
}

bool
AVCCMediaDataDecoder::IsDormantNeeded()
{
  if (mDecoder) {
    return mDecoder->IsDormantNeeded();
  }
  return MediaDataDecoder::IsDormantNeeded();
}

void
AVCCMediaDataDecoder::AllocateMediaResources()
{
  if (mDecoder) {
    mDecoder->AllocateMediaResources();
  }
}

void
AVCCMediaDataDecoder::ReleaseMediaResources()
{
  if (mDecoder) {
    mDecoder->ReleaseMediaResources();
  }
}

void
AVCCMediaDataDecoder::ReleaseDecoder()
{
  if (mDecoder) {
    mDecoder->ReleaseDecoder();
  }
}

nsresult
AVCCMediaDataDecoder::CreateDecoder()
{
  if (!mp4_demuxer::AnnexB::HasSPS(mCurrentConfig.extra_data)) {
    
    return NS_ERROR_NOT_INITIALIZED;
  }
  mDecoder = mPDM->CreateVideoDecoder(mCurrentConfig,
                                      mLayersBackend,
                                      mImageContainer,
                                      mVideoTaskQueue,
                                      mCallback);
  if (!mDecoder) {
    mLastError = NS_ERROR_FAILURE;
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
AVCCMediaDataDecoder::CreateDecoderAndInit(mp4_demuxer::MP4Sample* aSample)
{
  nsRefPtr<mp4_demuxer::ByteBuffer> extra_data =
    mp4_demuxer::AnnexB::ExtractExtraData(aSample);
  if (!mp4_demuxer::AnnexB::HasSPS(extra_data)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  mCurrentConfig.extra_data = extra_data;

  nsresult rv = CreateDecoder();
  NS_ENSURE_SUCCESS(rv, rv);
  return Init();
}

bool
AVCCMediaDataDecoder::IsHardwareAccelerated() const
{
  if (mDecoder) {
    return mDecoder->IsHardwareAccelerated();
  }
  return MediaDataDecoder::IsHardwareAccelerated();
}



AVCCDecoderModule::AVCCDecoderModule(PlatformDecoderModule* aPDM)
: mPDM(aPDM)
{
  MOZ_ASSERT(aPDM);
}

AVCCDecoderModule::~AVCCDecoderModule()
{
}

nsresult
AVCCDecoderModule::Startup()
{
  return mPDM->Startup();
}

nsresult
AVCCDecoderModule::Shutdown()
{
  return mPDM->Shutdown();
}

already_AddRefed<MediaDataDecoder>
AVCCDecoderModule::CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                      layers::LayersBackend aLayersBackend,
                                      layers::ImageContainer* aImageContainer,
                                      FlushableMediaTaskQueue* aVideoTaskQueue,
                                      MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder;

  if (strcmp(aConfig.mime_type, "video/avc") ||
      !mPDM->DecoderNeedsAVCC(aConfig)) {
    
    decoder = mPDM->CreateVideoDecoder(aConfig,
                                       aLayersBackend,
                                       aImageContainer,
                                       aVideoTaskQueue,
                                       aCallback);
  } else {
    decoder = new AVCCMediaDataDecoder(mPDM,
                                       aConfig,
                                       aLayersBackend,
                                       aImageContainer,
                                       aVideoTaskQueue,
                                       aCallback);
  }
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
AVCCDecoderModule::CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                      FlushableMediaTaskQueue* aAudioTaskQueue,
                                      MediaDataDecoderCallback* aCallback)
{
  return mPDM->CreateAudioDecoder(aConfig,
                                  aAudioTaskQueue,
                                  aCallback);
}

bool
AVCCDecoderModule::SupportsAudioMimeType(const char* aMimeType)
{
  return mPDM->SupportsAudioMimeType(aMimeType);
}

bool
AVCCDecoderModule::SupportsVideoMimeType(const char* aMimeType)
{
  return mPDM->SupportsVideoMimeType(aMimeType);
}

} 
