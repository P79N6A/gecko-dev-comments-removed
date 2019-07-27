




#include <unistd.h>
#include <fcntl.h>

#include "base/basictypes.h"
#include <cutils/properties.h>
#include <stagefright/foundation/ADebug.h>
#include <stagefright/foundation/AMessage.h>
#include <stagefright/MediaExtractor.h>
#include <stagefright/MetaData.h>
#include <stagefright/OMXClient.h>
#include <stagefright/OMXCodec.h>
#include <OMX.h>
#if MOZ_WIDGET_GONK && ANDROID_VERSION >= 17
#include <ui/Fence.h>
#endif

#include "mozilla/layers/GrallocTextureClient.h"
#include "mozilla/layers/TextureClient.h"
#include "mozilla/Preferences.h"
#include "mozilla/Types.h"
#include "mozilla/Monitor.h"
#include "nsMimeTypes.h"
#include "MPAPI.h"
#include "mozilla/Logging.h"

#include "GonkNativeWindow.h"
#include "GonkNativeWindowClient.h"
#include "OMXCodecProxy.h"
#include "OmxDecoder.h"

#include <android/log.h>
#define OD_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "OmxDecoder", __VA_ARGS__)

#undef LOG
PRLogModuleInfo *gOmxDecoderLog;
#define LOG(type, msg...) MOZ_LOG(gOmxDecoderLog, type, (msg))

using namespace MPAPI;
using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using namespace android;

OmxDecoder::OmxDecoder(MediaResource *aResource,
                       AbstractMediaDecoder *aDecoder) :
  mDecoder(aDecoder),
  mResource(aResource),
  mDisplayWidth(0),
  mDisplayHeight(0),
  mVideoWidth(0),
  mVideoHeight(0),
  mVideoColorFormat(0),
  mVideoStride(0),
  mVideoSliceHeight(0),
  mVideoRotation(0),
  mAudioChannels(-1),
  mAudioSampleRate(-1),
  mDurationUs(-1),
  mLastSeekTime(-1),
  mVideoBuffer(nullptr),
  mAudioBuffer(nullptr),
  mIsVideoSeeking(false),
  mAudioMetadataRead(false),
  mAudioPaused(false),
  mVideoPaused(false)
{
  mLooper = new ALooper;
  mLooper->setName("OmxDecoder");

  mReflector = new AHandlerReflector<OmxDecoder>(this);
  
  mLooper->registerHandler(mReflector);
  
  mLooper->start();
}

OmxDecoder::~OmxDecoder()
{
  MOZ_ASSERT(NS_IsMainThread());

  ReleaseMediaResources();

  
  mLooper->unregisterHandler(mReflector->id());
  
  mLooper->stop();
}

void OmxDecoder::statusChanged()
{
  sp<AMessage> notify =
           new AMessage(kNotifyStatusChanged, mReflector->id());
 
 notify->post();
}

static sp<IOMX> sOMX = nullptr;
static sp<IOMX> GetOMX()
{
  if(sOMX.get() == nullptr) {
    sOMX = new OMX;
    }
  return sOMX;
}

bool OmxDecoder::Init(sp<MediaExtractor>& extractor) {
  if (!gOmxDecoderLog) {
    gOmxDecoderLog = PR_NewLogModule("OmxDecoder");
  }

  sp<MetaData> meta = extractor->getMetaData();

  ssize_t audioTrackIndex = -1;
  ssize_t videoTrackIndex = -1;

  for (size_t i = 0; i < extractor->countTracks(); ++i) {
    sp<MetaData> meta = extractor->getTrackMetaData(i);

    int32_t bitRate;
    if (!meta->findInt32(kKeyBitRate, &bitRate))
      bitRate = 0;

    const char *mime;
    if (!meta->findCString(kKeyMIMEType, &mime)) {
      continue;
    }

    if (videoTrackIndex == -1 && !strncasecmp(mime, "video/", 6)) {
      videoTrackIndex = i;
    } else if (audioTrackIndex == -1 && !strncasecmp(mime, "audio/", 6)) {
      audioTrackIndex = i;
    }
  }

  if (videoTrackIndex == -1 && audioTrackIndex == -1) {
    NS_WARNING("OMX decoder could not find video or audio tracks");
    return false;
  }

  mResource->SetReadMode(MediaCacheStream::MODE_PLAYBACK);

  if (videoTrackIndex != -1 && mDecoder->GetImageContainer()) {
    mVideoTrack = extractor->getTrack(videoTrackIndex);
  }

  if (audioTrackIndex != -1) {
    mAudioTrack = extractor->getTrack(audioTrackIndex);

#ifdef MOZ_AUDIO_OFFLOAD
    
    
    mAudioOffloadTrack = extractor->getTrack(audioTrackIndex);
#endif
  }
  return true;
}

bool OmxDecoder::EnsureMetadata() {
  
  int64_t totalDurationUs = 0;
  int64_t durationUs = 0;
  if (mVideoTrack.get() && mVideoTrack->getFormat()->findInt64(kKeyDuration, &durationUs)) {
    if (durationUs > totalDurationUs)
      totalDurationUs = durationUs;
  }
  if (mAudioTrack.get()) {
    durationUs = -1;
    sp<MetaData> meta = mAudioTrack->getFormat();

    if ((durationUs == -1) && meta->findInt64(kKeyDuration, &durationUs)) {
      if (durationUs > totalDurationUs) {
        totalDurationUs = durationUs;
      }
    }
  }
  mDurationUs = totalDurationUs;

  
  if (mVideoSource.get() && !SetVideoFormat()) {
    NS_WARNING("Couldn't set OMX video format");
    return false;
  }

  
  if (mAudioSource.get()) {
    
    
    status_t err = mAudioSource->read(&mAudioBuffer);
    if (err != INFO_FORMAT_CHANGED) {
      if (err != OK) {
        NS_WARNING("Couldn't read audio buffer from OMX decoder");
        return false;
      }
      sp<MetaData> meta = mAudioSource->getFormat();
      if (!meta->findInt32(kKeyChannelCount, &mAudioChannels) ||
          !meta->findInt32(kKeySampleRate, &mAudioSampleRate)) {
        NS_WARNING("Couldn't get audio metadata from OMX decoder");
        return false;
      }
      mAudioMetadataRead = true;
    }
    else if (!SetAudioFormat()) {
      NS_WARNING("Couldn't set audio format");
      return false;
    }
  }

  return true;
}

bool OmxDecoder::IsWaitingMediaResources()
{
  if (mVideoSource.get()) {
    return mVideoSource->IsWaitingResources();
  }
  return false;
}

static bool isInEmulator()
{
  char propQemu[PROPERTY_VALUE_MAX];
  property_get("ro.kernel.qemu", propQemu, "");
  return !strncmp(propQemu, "1", 1);
}

bool OmxDecoder::AllocateMediaResources()
{
  if ((mVideoTrack != nullptr) && (mVideoSource == nullptr)) {
    
    
    OMXClient client;
    DebugOnly<status_t> err = client.connect();
    NS_ASSERTION(err == OK, "Failed to connect to OMX in mediaserver.");
    sp<IOMX> omx = client.interface();

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
    sp<IGraphicBufferProducer> producer;
    sp<IGonkGraphicBufferConsumer> consumer;
    GonkBufferQueue::createBufferQueue(&producer, &consumer);
    mNativeWindow = new GonkNativeWindow(consumer);
#else
    mNativeWindow = new GonkNativeWindow();
#endif

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
    mNativeWindowClient = new GonkNativeWindowClient(producer);
#elif defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
    mNativeWindowClient = new GonkNativeWindowClient(mNativeWindow->getBufferQueue());
#else
    mNativeWindowClient = new GonkNativeWindowClient(mNativeWindow);
#endif

    
    
    
    
    
#ifdef MOZ_OMX_WEBM_DECODER
    int flags = 0;
#else
    int flags = kHardwareCodecsOnly;
#endif

    if (isInEmulator()) {
      
      flags = 0;
    }
    mVideoSource =
          OMXCodecProxy::Create(omx,
                                mVideoTrack->getFormat(),
                                false, 
                                mVideoTrack,
                                nullptr,
                                flags,
                                mNativeWindowClient);
    if (mVideoSource == nullptr) {
      NS_WARNING("Couldn't create OMX video source");
      return false;
    } else {
      sp<OMXCodecProxy::EventListener> listener = this;
      mVideoSource->setEventListener(listener);
      mVideoSource->requestResource();
    }
  }

  if ((mAudioTrack != nullptr) && (mAudioSource == nullptr)) {
    
    
    OMXClient client;
    DebugOnly<status_t> err = client.connect();
    NS_ASSERTION(err == OK, "Failed to connect to OMX in mediaserver.");
    sp<IOMX> omx = client.interface();

    const char *audioMime = nullptr;
    sp<MetaData> meta = mAudioTrack->getFormat();
    if (!meta->findCString(kKeyMIMEType, &audioMime)) {
      return false;
    }
    if (!strcasecmp(audioMime, "audio/raw")) {
      mAudioSource = mAudioTrack;
    } else {
      
      int flags = kHardwareCodecsOnly;
      mAudioSource = OMXCodec::Create(omx,
                                     mAudioTrack->getFormat(),
                                     false, 
                                     mAudioTrack,
                                     nullptr,
                                     flags);
    }

    if (mAudioSource == nullptr) {
      
      int flags = kSoftwareCodecsOnly;
      mAudioSource = OMXCodec::Create(GetOMX(),
                                     mAudioTrack->getFormat(),
                                     false, 
                                     mAudioTrack,
                                     nullptr,
                                     flags);
      if (mAudioSource == nullptr) {
        NS_WARNING("Couldn't create OMX audio source");
        return false;
      }
    }
    if (mAudioSource->start() != OK) {
      NS_WARNING("Couldn't start OMX audio source");
      mAudioSource.clear();
      return false;
    }
  }
  return true;
}


void OmxDecoder::ReleaseMediaResources() {
  ReleaseVideoBuffer();
  ReleaseAudioBuffer();

  {
    Mutex::Autolock autoLock(mPendingVideoBuffersLock);
    MOZ_ASSERT(mPendingRecycleTexutreClients.empty());
    
    
    if (!mPendingRecycleTexutreClients.empty()) {
      printf_stderr("OmxDecoder::ReleaseMediaResources -- TextureClients are not recycled yet\n");
      for (std::set<TextureClient*>::iterator it=mPendingRecycleTexutreClients.begin();
           it!=mPendingRecycleTexutreClients.end(); it++)
      {
        GrallocTextureClientOGL* client = static_cast<GrallocTextureClientOGL*>(*it);
        client->ClearRecycleCallback();
        if (client->GetMediaBuffer()) {
          mPendingVideoBuffers.push(BufferItem(client->GetMediaBuffer(), client->GetAndResetReleaseFenceHandle()));
        }
      }
      mPendingRecycleTexutreClients.clear();
    }
  }

  {
    
    Mutex::Autolock autoLock(mSeekLock);
    ReleaseAllPendingVideoBuffersLocked();
  }

  if (mVideoSource.get()) {
    mVideoSource->stop();
    mVideoSource.clear();
  }

  if (mAudioSource.get()) {
    mAudioSource->stop();
    mAudioSource.clear();
  }

  mNativeWindowClient.clear();
  mNativeWindow.clear();

  
  
  mLastSeekTime = -1;
}

bool OmxDecoder::SetVideoFormat() {
  const char *componentName;

  if (!mVideoSource->getFormat()->findInt32(kKeyWidth, &mVideoWidth) ||
      !mVideoSource->getFormat()->findInt32(kKeyHeight, &mVideoHeight) ||
      !mVideoSource->getFormat()->findCString(kKeyDecoderComponent, &componentName) ||
      !mVideoSource->getFormat()->findInt32(kKeyColorFormat, &mVideoColorFormat) ) {
    return false;
  }

  if (!mVideoTrack.get() || !mVideoTrack->getFormat()->findInt32(kKeyDisplayWidth, &mDisplayWidth)) {
    mDisplayWidth = mVideoWidth;
    NS_WARNING("display width not available, assuming width");
  }

  if (!mVideoTrack.get() || !mVideoTrack->getFormat()->findInt32(kKeyDisplayHeight, &mDisplayHeight)) {
    mDisplayHeight = mVideoHeight;
    NS_WARNING("display height not available, assuming height");
  }

  if (!mVideoSource->getFormat()->findInt32(kKeyStride, &mVideoStride)) {
    mVideoStride = mVideoWidth;
    NS_WARNING("stride not available, assuming width");
  }

  if (!mVideoSource->getFormat()->findInt32(kKeySliceHeight, &mVideoSliceHeight)) {
    mVideoSliceHeight = mVideoHeight;
    NS_WARNING("slice height not available, assuming height");
  }

  
  
  
  
  int32_t crop_left, crop_top, crop_right, crop_bottom;
  if (mVideoSource->getFormat()->findRect(kKeyCropRect,
                                          &crop_left,
                                          &crop_top,
                                          &crop_right,
                                          &crop_bottom)) {
    mVideoWidth = crop_right - crop_left + 1;
    mVideoHeight = crop_bottom - crop_top + 1;
  }

  if (!mVideoSource->getFormat()->findInt32(kKeyRotation, &mVideoRotation)) {
    mVideoRotation = 0;
    NS_WARNING("rotation not available, assuming 0");
  }

  LOG(PR_LOG_DEBUG, "display width: %d display height %d width: %d height: %d component: %s format: %d stride: %d sliceHeight: %d rotation: %d",
      mDisplayWidth, mDisplayHeight, mVideoWidth, mVideoHeight, componentName,
      mVideoColorFormat, mVideoStride, mVideoSliceHeight, mVideoRotation);

  return true;
}

bool OmxDecoder::SetAudioFormat() {
  
  if (!mAudioSource->getFormat()->findInt32(kKeyChannelCount, &mAudioChannels) ||
      !mAudioSource->getFormat()->findInt32(kKeySampleRate, &mAudioSampleRate)) {
    return false;
  }

  LOG(PR_LOG_DEBUG, "channelCount: %d sampleRate: %d",
      mAudioChannels, mAudioSampleRate);

  return true;
}

void OmxDecoder::ReleaseDecoder()
{
  mDecoder = nullptr;
}

void OmxDecoder::ReleaseVideoBuffer() {
  if (mVideoBuffer) {
    mVideoBuffer->release();
    mVideoBuffer = nullptr;
  }
}

void OmxDecoder::ReleaseAudioBuffer() {
  if (mAudioBuffer) {
    mAudioBuffer->release();
    mAudioBuffer = nullptr;
  }
}

void OmxDecoder::PlanarYUV420Frame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame) {
  void *y = aData;
  void *u = static_cast<uint8_t *>(y) + mVideoStride * mVideoSliceHeight;
  void *v = static_cast<uint8_t *>(u) + mVideoStride/2 * mVideoSliceHeight/2;

  aFrame->Set(aTimeUs, aKeyFrame,
              aData, aSize, mVideoStride, mVideoSliceHeight, mVideoRotation,
              y, mVideoStride, mVideoWidth, mVideoHeight, 0, 0,
              u, mVideoStride/2, mVideoWidth/2, mVideoHeight/2, 0, 0,
              v, mVideoStride/2, mVideoWidth/2, mVideoHeight/2, 0, 0);
}

void OmxDecoder::CbYCrYFrame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame) {
  aFrame->Set(aTimeUs, aKeyFrame,
              aData, aSize, mVideoStride, mVideoSliceHeight, mVideoRotation,
              aData, mVideoStride, mVideoWidth, mVideoHeight, 1, 1,
              aData, mVideoStride, mVideoWidth/2, mVideoHeight/2, 0, 3,
              aData, mVideoStride, mVideoWidth/2, mVideoHeight/2, 2, 3);
}

void OmxDecoder::SemiPlanarYUV420Frame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame) {
  void *y = aData;
  void *uv = static_cast<uint8_t *>(y) + (mVideoStride * mVideoSliceHeight);

  aFrame->Set(aTimeUs, aKeyFrame,
              aData, aSize, mVideoStride, mVideoSliceHeight, mVideoRotation,
              y, mVideoStride, mVideoWidth, mVideoHeight, 0, 0,
              uv, mVideoStride, mVideoWidth/2, mVideoHeight/2, 0, 1,
              uv, mVideoStride, mVideoWidth/2, mVideoHeight/2, 1, 1);
}

void OmxDecoder::SemiPlanarYVU420Frame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame) {
  SemiPlanarYUV420Frame(aFrame, aTimeUs, aData, aSize, aKeyFrame);
  aFrame->Cb.mOffset = 1;
  aFrame->Cr.mOffset = 0;
}

bool OmxDecoder::ToVideoFrame(VideoFrame *aFrame, int64_t aTimeUs, void *aData, size_t aSize, bool aKeyFrame) {
  const int OMX_QCOM_COLOR_FormatYVU420SemiPlanar = 0x7FA30C00;

  aFrame->mGraphicBuffer = nullptr;

  switch (mVideoColorFormat) {
  case OMX_COLOR_FormatYUV420Planar:
    PlanarYUV420Frame(aFrame, aTimeUs, aData, aSize, aKeyFrame);
    break;
  case OMX_COLOR_FormatCbYCrY:
    CbYCrYFrame(aFrame, aTimeUs, aData, aSize, aKeyFrame);
    break;
  case OMX_COLOR_FormatYUV420SemiPlanar:
    SemiPlanarYUV420Frame(aFrame, aTimeUs, aData, aSize, aKeyFrame);
    break;
  case OMX_QCOM_COLOR_FormatYVU420SemiPlanar:
    SemiPlanarYVU420Frame(aFrame, aTimeUs, aData, aSize, aKeyFrame);
    break;
  default:
    LOG(PR_LOG_DEBUG, "Unknown video color format %08x", mVideoColorFormat);
    return false;
  }
  return true;
}

bool OmxDecoder::ToAudioFrame(AudioFrame *aFrame, int64_t aTimeUs, void *aData, size_t aDataOffset, size_t aSize, int32_t aAudioChannels, int32_t aAudioSampleRate)
{
  aFrame->Set(aTimeUs, static_cast<char *>(aData) + aDataOffset, aSize, aAudioChannels, aAudioSampleRate);
  return true;
}

bool OmxDecoder::ReadVideo(VideoFrame *aFrame, int64_t aTimeUs,
                           bool aKeyframeSkip, bool aDoSeek)
{
  if (!mVideoSource.get())
    return false;

  ReleaseVideoBuffer();

  status_t err;

  if (aDoSeek) {
    {
      Mutex::Autolock autoLock(mSeekLock);
      ReleaseAllPendingVideoBuffersLocked();
      mIsVideoSeeking = true;
    }
    MediaSource::ReadOptions options;
    MediaSource::ReadOptions::SeekMode seekMode;
    
    
    OD_LOG("SeekTime: %lld, mLastSeekTime:%lld", aTimeUs, mLastSeekTime);
    if (mLastSeekTime == -1 || mLastSeekTime > aTimeUs) {
      seekMode = MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC;
    } else {
      seekMode = MediaSource::ReadOptions::SEEK_NEXT_SYNC;
    }
    mLastSeekTime = aTimeUs;
    bool findNextBuffer = true;
    while (findNextBuffer) {
      options.setSeekTo(aTimeUs, seekMode);
      findNextBuffer = false;
      if (mIsVideoSeeking) {
        err = mVideoSource->read(&mVideoBuffer, &options);
        Mutex::Autolock autoLock(mSeekLock);
        mIsVideoSeeking = false;
        PostReleaseVideoBuffer(nullptr, FenceHandle());
      }
      else {
	err = mVideoSource->read(&mVideoBuffer);
      }

      
      if (err == ERROR_END_OF_STREAM && seekMode == MediaSource::ReadOptions::SEEK_NEXT_SYNC) {
        seekMode = MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC;
        findNextBuffer = true;
        {
          Mutex::Autolock autoLock(mSeekLock);
          mIsVideoSeeking = true;
        }
        continue;
      } else if (err != OK) {
        OD_LOG("Unexpected error when seeking to %lld", aTimeUs);
        break;
      }
      
      
      if (mVideoBuffer->range_length() == 0) {
        PostReleaseVideoBuffer(mVideoBuffer, FenceHandle());
        findNextBuffer = true;
      }
    }
    aDoSeek = false;
  } else {
    err = mVideoSource->read(&mVideoBuffer);
  }

  aFrame->mSize = 0;

  if (err == OK) {
    int64_t timeUs;
    int32_t unreadable;
    int32_t keyFrame;

    size_t length = mVideoBuffer->range_length();

    if (!mVideoBuffer->meta_data()->findInt64(kKeyTime, &timeUs) ) {
      NS_WARNING("OMX decoder did not return frame time");
      return false;
    }

    if (!mVideoBuffer->meta_data()->findInt32(kKeyIsSyncFrame, &keyFrame)) {
      keyFrame = 0;
    }

    if (!mVideoBuffer->meta_data()->findInt32(kKeyIsUnreadable, &unreadable)) {
      unreadable = 0;
    }

    RefPtr<mozilla::layers::TextureClient> textureClient;
    if ((mVideoBuffer->graphicBuffer().get())) {
      textureClient = mNativeWindow->getTextureClientFromBuffer(mVideoBuffer->graphicBuffer().get());
    }

    if (textureClient) {
      
      
      mVideoBuffer->add_ref();
      GrallocTextureClientOGL* grallocClient = static_cast<GrallocTextureClientOGL*>(textureClient.get());
      grallocClient->SetMediaBuffer(mVideoBuffer);
      
      textureClient->SetRecycleCallback(OmxDecoder::RecycleCallback, this);
      {
        Mutex::Autolock autoLock(mPendingVideoBuffersLock);
        
        MOZ_ASSERT(mPendingRecycleTexutreClients.find(textureClient) == mPendingRecycleTexutreClients.end());
        mPendingRecycleTexutreClients.insert(textureClient);
      }

      aFrame->mGraphicBuffer = textureClient;
      aFrame->mRotation = mVideoRotation;
      aFrame->mTimeUs = timeUs;
      aFrame->mKeyFrame = keyFrame;
      aFrame->Y.mWidth = mVideoWidth;
      aFrame->Y.mHeight = mVideoHeight;
      
      
      ReleaseVideoBuffer();
    } else if (length > 0) {
      char *data = static_cast<char *>(mVideoBuffer->data()) + mVideoBuffer->range_offset();

      if (unreadable) {
        LOG(PR_LOG_DEBUG, "video frame is unreadable");
      }

      if (!ToVideoFrame(aFrame, timeUs, data, length, keyFrame)) {
        return false;
      }
    }
    
    if ((aKeyframeSkip && timeUs < aTimeUs) || length == 0) {
      aFrame->mShouldSkip = true;
    }
  }
  else if (err == INFO_FORMAT_CHANGED) {
    
    if (!SetVideoFormat()) {
      return false;
    } else {
      return ReadVideo(aFrame, aTimeUs, aKeyframeSkip, aDoSeek);
    }
  }
  else if (err == ERROR_END_OF_STREAM) {
    return false;
  }
  else if (err == -ETIMEDOUT) {
    LOG(PR_LOG_DEBUG, "OmxDecoder::ReadVideo timed out, will retry");
    return true;
  }
  else {
    
    
    LOG(PR_LOG_DEBUG, "OmxDecoder::ReadVideo failed, err=%d", err);
    return false;
  }

  return true;
}

bool OmxDecoder::ReadAudio(AudioFrame *aFrame, int64_t aSeekTimeUs)
{
  status_t err;

  if (mAudioMetadataRead && aSeekTimeUs == -1) {
    
    err = OK;
  }
  else {
    ReleaseAudioBuffer();
    if (aSeekTimeUs != -1) {
      MediaSource::ReadOptions options;
      options.setSeekTo(aSeekTimeUs);
      err = mAudioSource->read(&mAudioBuffer, &options);
    } else {
      err = mAudioSource->read(&mAudioBuffer);
    }
  }
  mAudioMetadataRead = false;

  aSeekTimeUs = -1;
  aFrame->mSize = 0;

  if (err == OK && mAudioBuffer && mAudioBuffer->range_length() != 0) {
    int64_t timeUs;
    if (!mAudioBuffer->meta_data()->findInt64(kKeyTime, &timeUs))
      return false;

    return ToAudioFrame(aFrame, timeUs,
                        mAudioBuffer->data(),
                        mAudioBuffer->range_offset(),
                        mAudioBuffer->range_length(),
                        mAudioChannels, mAudioSampleRate);
  }
  else if (err == INFO_FORMAT_CHANGED) {
    
    if (!SetAudioFormat()) {
      return false;
    } else {
      return ReadAudio(aFrame, aSeekTimeUs);
    }
  }
  else if (err == ERROR_END_OF_STREAM) {
    if (aFrame->mSize == 0) {
      return false;
    }
  }
  else if (err == -ETIMEDOUT) {
    LOG(PR_LOG_DEBUG, "OmxDecoder::ReadAudio timed out, will retry");
    return true;
  }
  else if (err != OK) {
    LOG(PR_LOG_DEBUG, "OmxDecoder::ReadAudio failed, err=%d", err);
    return false;
  }

  return true;
}

nsresult OmxDecoder::Play()
{
  if (!mVideoPaused && !mAudioPaused) {
    return NS_OK;
  }

  if (mVideoPaused && mVideoSource.get() && mVideoSource->start() != OK) {
    return NS_ERROR_UNEXPECTED;
  }
  mVideoPaused = false;

  if (mAudioPaused && mAudioSource.get() && mAudioSource->start() != OK) {
    return NS_ERROR_UNEXPECTED;
  }
  mAudioPaused = false;

  return NS_OK;
}









void OmxDecoder::Pause()
{
  





  if (isInEmulator()) {
    return;
  }

  if (mVideoPaused || mAudioPaused) {
    return;
  }

  if (mVideoSource.get() && mVideoSource->pause() == OK) {
    mVideoPaused = true;
  }

  if (mAudioSource.get() && mAudioSource->pause() == OK) {
    mAudioPaused = true;
  }
}


void OmxDecoder::onMessageReceived(const sp<AMessage> &msg)
{
  switch (msg->what()) {
    case kNotifyPostReleaseVideoBuffer:
    {
      Mutex::Autolock autoLock(mSeekLock);
      
      
      if (!mIsVideoSeeking) {
        ReleaseAllPendingVideoBuffersLocked();
      }
      break;
    }

    case kNotifyStatusChanged:
    {
      
      
      mDecoder->NotifyWaitingForResourcesStatusChanged();
      break;
    }

    default:
      TRESPASS();
      break;
  }
}

void OmxDecoder::PostReleaseVideoBuffer(MediaBuffer *aBuffer, const FenceHandle& aReleaseFenceHandle)
{
  {
    Mutex::Autolock autoLock(mPendingVideoBuffersLock);
    if (aBuffer) {
      mPendingVideoBuffers.push(BufferItem(aBuffer, aReleaseFenceHandle));
    }
  }

  sp<AMessage> notify =
            new AMessage(kNotifyPostReleaseVideoBuffer, mReflector->id());
  
  notify->post();
}

void OmxDecoder::ReleaseAllPendingVideoBuffersLocked()
{
  Vector<BufferItem> releasingVideoBuffers;
  {
    Mutex::Autolock autoLock(mPendingVideoBuffersLock);

    int size = mPendingVideoBuffers.size();
    for (int i = 0; i < size; i++) {
      releasingVideoBuffers.push(mPendingVideoBuffers[i]);
    }
    mPendingVideoBuffers.clear();
  }
  
  int size = releasingVideoBuffers.size();
  for (int i = 0; i < size; i++) {
    MediaBuffer *buffer;
    buffer = releasingVideoBuffers[i].mMediaBuffer;
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
    nsRefPtr<FenceHandle::FdObj> fdObj = releasingVideoBuffers.editItemAt(i).mReleaseFenceHandle.GetAndResetFdObj();
    int fenceFd = fdObj->GetAndResetFd();

    MOZ_ASSERT(buffer->refcount() == 1);
    
    
    ANativeWindow* window = static_cast<ANativeWindow*>(mNativeWindowClient.get());
    window->cancelBuffer(window,
                         buffer->graphicBuffer().get(),
                         fenceFd);
    
    
    
    sp<MetaData> metaData = buffer->meta_data();
    metaData->setInt32(kKeyRendered, 1);
#endif
    
    buffer->release();
  }
  releasingVideoBuffers.clear();
}

void OmxDecoder::RecycleCallbackImp(TextureClient* aClient)
{
  aClient->ClearRecycleCallback();
  {
    Mutex::Autolock autoLock(mPendingVideoBuffersLock);
    if (mPendingRecycleTexutreClients.find(aClient) == mPendingRecycleTexutreClients.end()) {
      printf_stderr("OmxDecoder::RecycleCallbackImp -- TextureClient is not pending recycle\n");
      return;
    }
    mPendingRecycleTexutreClients.erase(aClient);
    GrallocTextureClientOGL* client = static_cast<GrallocTextureClientOGL*>(aClient);
    if (client->GetMediaBuffer()) {
      mPendingVideoBuffers.push(BufferItem(client->GetMediaBuffer(), client->GetAndResetReleaseFenceHandle()));
    }
  }
  sp<AMessage> notify =
            new AMessage(kNotifyPostReleaseVideoBuffer, mReflector->id());
  
  notify->post();
}

 void
OmxDecoder::RecycleCallback(TextureClient* aClient, void* aClosure)
{
  OmxDecoder* decoder = static_cast<OmxDecoder*>(aClosure);
  decoder->RecycleCallbackImp(aClient);
}
