





#include "MediaCodecProxy.h"
#include <string.h>
#include <binder/IPCThreadState.h>
#include <stagefright/foundation/ABuffer.h>
#include <stagefright/foundation/ADebug.h>
#include <stagefright/MetaData.h>
#include "stagefright/MediaErrors.h"

#define LOG_TAG "MediaCodecProxy"
#include <android/log.h>
#define ALOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define TIMEOUT_DEQUEUE_INPUTBUFFER_MS 1000000ll
namespace android {


template <typename T, bool InterfaceSupported>
struct OutputGraphicBufferStub
{
  static status_t GetOutputGraphicBuffer(T *aMediaCodec,
                                         size_t aIndex,
                                         sp<GraphicBuffer> *aGraphicBuffer)
  {
    return ERROR_UNSUPPORTED;
  }
};


template <typename T>
struct OutputGraphicBufferStub<T, true>
{
  static status_t GetOutputGraphicBuffer(T *aMediaCodec,
                                         size_t aIndex,
                                         sp<GraphicBuffer> *aGraphicBuffer)
  {
    if (aMediaCodec == nullptr || aGraphicBuffer == nullptr) {
      return BAD_VALUE;
    }
    *aGraphicBuffer = aMediaCodec->getOutputGraphicBufferFromIndex(aIndex);
    return OK;
  }
};


struct MediaCodecInterfaceWrapper
{
  typedef int8_t Supported;
  typedef int16_t Unsupported;

  template <typename T>
  static auto TestOutputGraphicBuffer(T *aMediaCodec) -> decltype(aMediaCodec->getOutputGraphicBufferFromIndex(0), Supported());

  template <typename T>
  static auto TestOutputGraphicBuffer(...) -> Unsupported;

  
  static const bool OutputGraphicBufferSupported = sizeof(TestOutputGraphicBuffer<MediaCodec>(nullptr)) == sizeof(Supported);

  
  static OutputGraphicBufferStub<MediaCodec, OutputGraphicBufferSupported> sOutputGraphicBufferStub;

  
  static status_t GetOutputGraphicBuffer(MediaCodec *aMediaCodec,
                                         size_t aIndex,
                                         sp<GraphicBuffer> *aGraphicBuffer)
  {
    return sOutputGraphicBufferStub.GetOutputGraphicBuffer(aMediaCodec, aIndex, aGraphicBuffer);
  }

};

sp<MediaCodecProxy>
MediaCodecProxy::CreateByType(sp<ALooper> aLooper,
                              const char *aMime,
                              bool aEncoder,
                              bool aAsync,
                              wp<CodecResourceListener> aListener)
{
  sp<MediaCodecProxy> codec = new MediaCodecProxy(aLooper, aMime, aEncoder, aAsync, aListener);
  if ((!aAsync && codec->allocated()) || codec->requestResource()) {
    return codec;
  }
  return nullptr;
}

MediaCodecProxy::MediaCodecProxy(sp<ALooper> aLooper,
                                 const char *aMime,
                                 bool aEncoder,
                                 bool aAsync,
                                 wp<CodecResourceListener> aListener)
  : mCodecLooper(aLooper)
  , mCodecMime(aMime)
  , mCodecEncoder(aEncoder)
  , mListener(aListener)
{
  MOZ_ASSERT(mCodecLooper != nullptr, "ALooper should not be nullptr.");
  if (aAsync) {
    mResourceHandler = new MediaResourceHandler(this);
  } else {
    allocateCodec();
  }
}

MediaCodecProxy::~MediaCodecProxy()
{
  releaseCodec();

  
  IPCThreadState::self()->flushCommands();

  cancelResource();
}

bool
MediaCodecProxy::requestResource()
{
  if (mResourceHandler == nullptr) {
    return false;
  }

  if (strncasecmp(mCodecMime.get(), "video/", 6) == 0) {
    mResourceHandler->requestResource(mCodecEncoder
        ? IMediaResourceManagerService::HW_VIDEO_ENCODER
        : IMediaResourceManagerService::HW_VIDEO_DECODER);
  } else if (strncasecmp(mCodecMime.get(), "audio/", 6) == 0) {
    mResourceHandler->requestResource(mCodecEncoder
        ? IMediaResourceManagerService::HW_AUDIO_ENCODER
        : IMediaResourceManagerService::HW_AUDIO_DECODER);
  } else {
    return false;
  }

  return true;
}

void
MediaCodecProxy::cancelResource()
{
  if (mResourceHandler == nullptr) {
    return;
  }

  mResourceHandler->cancelResource();
}

bool
MediaCodecProxy::allocateCodec()
{
  if (mCodecLooper == nullptr) {
    return false;
  }

  
  RWLock::AutoWLock awl(mCodecLock);

  
  mCodec = MediaCodec::CreateByType(mCodecLooper, mCodecMime.get(), mCodecEncoder);
  if (mCodec == nullptr) {
    return false;
  }

  return true;
}

void
MediaCodecProxy::releaseCodec()
{
  wp<MediaCodec> codec;

  {
    
    RWLock::AutoWLock awl(mCodecLock);

    codec = mCodec;

    
    if (mCodec != nullptr) {
      mCodec->release();
      mCodec = nullptr;
    }
  }

  while (codec.promote() != nullptr) {
    
    usleep(1000);
  }
}

bool
MediaCodecProxy::allocated() const
{
  
  RWLock::AutoRLock arl(mCodecLock);

  return mCodec != nullptr;
}

status_t
MediaCodecProxy::configure(const sp<AMessage> &aFormat,
                           const sp<Surface> &aNativeWindow,
                           const sp<ICrypto> &aCrypto,
                           uint32_t aFlags)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->configure(aFormat, aNativeWindow, aCrypto, aFlags);
}

status_t
MediaCodecProxy::start()
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->start();
}

status_t
MediaCodecProxy::stop()
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->stop();
}

status_t
MediaCodecProxy::release()
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->release();
}

status_t
MediaCodecProxy::flush()
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->flush();
}

status_t
MediaCodecProxy::queueInputBuffer(size_t aIndex,
                                  size_t aOffset,
                                  size_t aSize,
                                  int64_t aPresentationTimeUs,
                                  uint32_t aFlags,
                                  AString *aErrorDetailMessage)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->queueInputBuffer(aIndex, aOffset, aSize,
      aPresentationTimeUs, aFlags, aErrorDetailMessage);
}

status_t
MediaCodecProxy::queueSecureInputBuffer(size_t aIndex,
                                        size_t aOffset,
                                        const CryptoPlugin::SubSample *aSubSamples,
                                        size_t aNumSubSamples,
                                        const uint8_t aKey[16],
                                        const uint8_t aIV[16],
                                        CryptoPlugin::Mode aMode,
                                        int64_t aPresentationTimeUs,
                                        uint32_t aFlags,
                                        AString *aErrorDetailMessage)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->queueSecureInputBuffer(aIndex, aOffset,
      aSubSamples, aNumSubSamples, aKey, aIV, aMode,
      aPresentationTimeUs, aFlags, aErrorDetailMessage);
}

status_t
MediaCodecProxy::dequeueInputBuffer(size_t *aIndex,
                                    int64_t aTimeoutUs)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->dequeueInputBuffer(aIndex, aTimeoutUs);
}

status_t
MediaCodecProxy::dequeueOutputBuffer(size_t *aIndex,
                                     size_t *aOffset,
                                     size_t *aSize,
                                     int64_t *aPresentationTimeUs,
                                     uint32_t *aFlags,
                                     int64_t aTimeoutUs)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->dequeueOutputBuffer(aIndex, aOffset, aSize,
      aPresentationTimeUs, aFlags, aTimeoutUs);
}

status_t
MediaCodecProxy::renderOutputBufferAndRelease(size_t aIndex)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->renderOutputBufferAndRelease(aIndex);
}

status_t
MediaCodecProxy::releaseOutputBuffer(size_t aIndex)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->releaseOutputBuffer(aIndex);
}

status_t
MediaCodecProxy::signalEndOfInputStream()
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->signalEndOfInputStream();
}

status_t
MediaCodecProxy::getOutputFormat(sp<AMessage> *aFormat) const
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->getOutputFormat(aFormat);
}

status_t
MediaCodecProxy::getInputBuffers(Vector<sp<ABuffer>> *aBuffers) const
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->getInputBuffers(aBuffers);
}

status_t
MediaCodecProxy::getOutputBuffers(Vector<sp<ABuffer>> *aBuffers) const
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return mCodec->getOutputBuffers(aBuffers);
}

void
MediaCodecProxy::requestActivityNotification(const sp<AMessage> &aNotify)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return;
  }
  mCodec->requestActivityNotification(aNotify);
}

status_t
MediaCodecProxy::getOutputGraphicBufferFromIndex(size_t aIndex,
                                                 sp<GraphicBuffer> *aGraphicBuffer)
{
  
  RWLock::AutoRLock arl(mCodecLock);

  if (mCodec == nullptr) {
    return NO_INIT;
  }
  return MediaCodecInterfaceWrapper::GetOutputGraphicBuffer(mCodec.get(), aIndex, aGraphicBuffer);
}

status_t
MediaCodecProxy::getCapability(uint32_t *aCapability)
{
  if (aCapability == nullptr) {
    return BAD_VALUE;
  }

  uint32_t capability = kEmptyCapability;

  if (MediaCodecInterfaceWrapper::OutputGraphicBufferSupported) {
    capability |= kCanExposeGraphicBuffer;
  }

  *aCapability = capability;

  return OK;
}


void
MediaCodecProxy::resourceReserved()
{
  
  releaseCodec();
  if (!allocateCodec()) {
    cancelResource();
    return;
  }

  
  sp<CodecResourceListener> listener = mListener.promote();
  if (listener != nullptr) {
    listener->codecReserved();
  }
}


void
MediaCodecProxy::resourceCanceled()
{
  
  releaseCodec();

  
  sp<CodecResourceListener> listener = mListener.promote();
  if (listener != nullptr) {
    listener->codecCanceled();
  }
}

bool MediaCodecProxy::Prepare()
{

  status_t err;
  if (start() != OK) {
    ALOG("Couldn't start MediaCodec");
    return false;
  }
  if (getInputBuffers(&mInputBuffers) != OK) {
    ALOG("Couldn't get input buffers from MediaCodec");
    return false;
  }
  if (getOutputBuffers(&mOutputBuffers) != OK) {
    ALOG("Couldn't get output buffers from MediaCodec");
    return false;
  }

  return true;
}

status_t MediaCodecProxy::Input(const uint8_t* aData, uint32_t aDataSize,
                                int64_t aTimestampUsecs, uint64_t aflags)
{
  if (mCodec == nullptr) {
    ALOG("MediaCodec has not been inited from input!");
    return NO_INIT;
  }

  size_t index;
  status_t err = dequeueInputBuffer(&index, TIMEOUT_DEQUEUE_INPUTBUFFER_MS);
  if (err != OK) {
    ALOG("dequeueInputBuffer returned %d", err);
    return err;
  }

  if (aData) {
    const sp<ABuffer> &dstBuffer = mInputBuffers.itemAt(index);

    CHECK_LE(aDataSize, dstBuffer->capacity());
    dstBuffer->setRange(0, aDataSize);

    memcpy(dstBuffer->data(), aData, aDataSize);
    err = queueInputBuffer(index, 0, dstBuffer->size(), aTimestampUsecs, aflags);
  } else {
    err = queueInputBuffer(index, 0, 0, 0ll, MediaCodec::BUFFER_FLAG_EOS);
  }

  if (err != OK) {
    ALOG("queueInputBuffer returned %d", err);
    return err;
  }
  return err;
}

status_t MediaCodecProxy::Output(MediaBuffer** aBuffer, int64_t aTimeoutUs)
{

  if (mCodec == nullptr) {
    ALOG("MediaCodec has not been inited from output!");
    return NO_INIT;
  }

  size_t index = 0;
  size_t offset = 0;
  size_t size = 0;
  int64_t timeUs = 0;
  uint32_t flags = 0;

  *aBuffer = nullptr;

  status_t err = dequeueOutputBuffer(&index, &offset, &size,
                                      &timeUs, &flags, aTimeoutUs);
  if (err != OK) {
    ALOG("Output returned %d", err);
    return err;
  }

  MediaBuffer *buffer;

  buffer = new MediaBuffer(mOutputBuffers.itemAt(index));
  sp<MetaData> metaData = buffer->meta_data();
  metaData->setInt32(kKeyBufferIndex, index);
  metaData->setInt64(kKeyTime, timeUs);
  buffer->set_range(buffer->range_offset(), size);
  *aBuffer = buffer;
  if (flags & MediaCodec::BUFFER_FLAG_EOS) {
    return ERROR_END_OF_STREAM;
  }
  return err;
}

bool MediaCodecProxy::IsWaitingResources()
{
  return mCodec == nullptr;
}

bool MediaCodecProxy::IsDormantNeeded()
{
  return mCodecLooper.get() ? true : false;
}

void MediaCodecProxy::ReleaseMediaResources()
{
  if (mCodec.get()) {
    mCodec->stop();
    mCodec->release();
    mCodec.clear();
  }
}

} 
