




#include "OMXCodecWrapper.h"
#include "OMXCodecDescriptorUtil.h"
#include "TrackEncoder.h"

#include <binder/ProcessState.h>
#include <media/ICrypto.h>
#include <media/IOMX.h>
#include <OMX_Component.h>
#include <stagefright/MediaDefs.h>
#include <stagefright/MediaErrors.h>

#include "AudioChannelFormat.h"
#include <mozilla/Monitor.h>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;

#define ENCODER_CONFIG_BITRATE 2000000 // bps

#define ENCODER_CONFIG_I_FRAME_INTERVAL 1

#define INPUT_BUFFER_TIMEOUT_US (5 * 1000ll)

#define CODEC_ERROR(args...)                                                   \
  do {                                                                         \
    __android_log_print(ANDROID_LOG_ERROR, "OMXCodecWrapper", ##args);         \
  } while (0)

namespace android {

OMXAudioEncoder*
OMXCodecWrapper::CreateAACEncoder()
{
  nsAutoPtr<OMXAudioEncoder> aac(new OMXAudioEncoder(CodecType::AAC_ENC));
  
  NS_ENSURE_TRUE(aac->IsValid(), nullptr);

  return aac.forget();
}

OMXVideoEncoder*
OMXCodecWrapper::CreateAVCEncoder()
{
  nsAutoPtr<OMXVideoEncoder> avc(new OMXVideoEncoder(CodecType::AVC_ENC));
  
  NS_ENSURE_TRUE(avc->IsValid(), nullptr);

  return avc.forget();
}

OMXCodecWrapper::OMXCodecWrapper(CodecType aCodecType)
  : mStarted(false)
{
  ProcessState::self()->startThreadPool();

  mLooper = new ALooper();
  mLooper->start();

  if (aCodecType == CodecType::AVC_ENC) {
    mCodec = MediaCodec::CreateByType(mLooper, MEDIA_MIMETYPE_VIDEO_AVC, true);
  } else if (aCodecType == CodecType::AAC_ENC) {
    mCodec = MediaCodec::CreateByType(mLooper, MEDIA_MIMETYPE_AUDIO_AAC, true);
  } else {
    NS_ERROR("Unknown codec type.");
  }
}

OMXCodecWrapper::~OMXCodecWrapper()
{
  if (mCodec.get()) {
    Stop();
    mCodec->release();
  }
  mLooper->stop();
}

status_t
OMXCodecWrapper::Start()
{
  
  NS_ENSURE_FALSE(mStarted, OK);

  status_t result = mCodec->start();
  mStarted = (result == OK);

  
  if (result == OK) {
    mCodec->getInputBuffers(&mInputBufs);
    mCodec->getOutputBuffers(&mOutputBufs);
  }

  return result;
}

status_t
OMXCodecWrapper::Stop()
{
  
  NS_ENSURE_TRUE(mStarted, OK);

  status_t result = mCodec->stop();
  mStarted = !(result == OK);

  return result;
}

nsresult
OMXVideoEncoder::Configure(int aWidth, int aHeight, int aFrameRate)
{
  MOZ_ASSERT(!mStarted, "Configure() was called already.");

  NS_ENSURE_TRUE(aWidth > 0 && aHeight > 0 && aFrameRate > 0,
                 NS_ERROR_INVALID_ARG);

  
  sp<AMessage> format = new AMessage;
  
  format->setString("mime", MEDIA_MIMETYPE_VIDEO_AVC);
  format->setInt32("bitrate", ENCODER_CONFIG_BITRATE);
  format->setInt32("i-frame-interval", ENCODER_CONFIG_I_FRAME_INTERVAL);
  
  
  format->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);
  format->setInt32("profile", OMX_VIDEO_AVCProfileBaseline);
  format->setInt32("level", OMX_VIDEO_AVCLevel3);
  format->setInt32("bitrate-mode", OMX_Video_ControlRateConstant);
  format->setInt32("store-metadata-in-buffers", 0);
  format->setInt32("prepend-sps-pps-to-idr-frames", 0);
  
  format->setInt32("width", aWidth);
  format->setInt32("height", aHeight);
  format->setInt32("stride", aWidth);
  format->setInt32("slice-height", aHeight);
  format->setInt32("frame-rate", aFrameRate);

  status_t result = mCodec->configure(format, nullptr, nullptr,
                                      MediaCodec::CONFIGURE_FLAG_ENCODE);
  NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);

  mWidth = aWidth;
  mHeight = aHeight;

  result = Start();

  return result == OK ? NS_OK : NS_ERROR_FAILURE;
}










static
void
ConvertPlanarYCbCrToNV12(const PlanarYCbCrData* aSource, uint8_t* aDestination)
{
  
  uint8_t* y = aSource->mYChannel;
  IntSize ySize = aSource->mYSize;

  
  for (int i = 0; i < ySize.height; i++) {
    memcpy(aDestination, y, ySize.width);
    aDestination += ySize.width;
    y += aSource->mYStride;
  }

  
  uint8_t* u = aSource->mCbChannel;
  uint8_t* v = aSource->mCrChannel;
  IntSize uvSize = aSource->mCbCrSize;
  
  
  MOZ_ASSERT(ySize.width % uvSize.width == 0 &&
             ySize.height % uvSize.height == 0);
  size_t uvWidth = ySize.width / 2;
  size_t uvHeight = ySize.height / 2;
  size_t horiSubsample = uvSize.width / uvWidth;
  size_t uPixStride = horiSubsample * (1 + aSource->mCbSkip);
  size_t vPixStride = horiSubsample * (1 + aSource->mCrSkip);
  size_t lineStride = uvSize.height / uvHeight * aSource->mCbCrStride;

  for (int i = 0; i < uvHeight; i++) {
    
    uint8_t* uSrc = u;
    uint8_t* vSrc = v;
    for (int j = 0; j < uvWidth; j++) {
      *aDestination++ = *uSrc;
      *aDestination++ = *vSrc;
      
      uSrc += uPixStride;
      vSrc += vPixStride;
    }
    
    u += lineStride;
    v += lineStride;
  }
}

nsresult
OMXVideoEncoder::Encode(const Image* aImage, int aWidth, int aHeight,
                        int64_t aTimestamp, int aInputFlags)
{
  MOZ_ASSERT(mStarted, "Configure() should be called before Encode().");

  NS_ENSURE_TRUE(aWidth == mWidth && aHeight == mHeight && aTimestamp >= 0,
                 NS_ERROR_INVALID_ARG);

  status_t result;

  
  uint32_t index;
  result = mCodec->dequeueInputBuffer(&index, INPUT_BUFFER_TIMEOUT_US);
  NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);

  const sp<ABuffer>& inBuf = mInputBufs.itemAt(index);
  uint8_t* dst = inBuf->data();
  size_t dstSize = inBuf->capacity();

  size_t yLen = aWidth * aHeight;
  size_t uvLen = yLen / 2;

  
  MOZ_ASSERT(dstSize >= yLen + uvLen);

  inBuf->setRange(0, yLen + uvLen);

  if (!aImage) {
    
    dstSize = yLen + uvLen;
    
    memset(dst, 0x10, yLen);
    
    memset(dst + yLen, 0x80, uvLen);
  } else {
    Image* img = const_cast<Image*>(aImage);
    ImageFormat format = img->GetFormat();

    MOZ_ASSERT(aWidth == img->GetSize().width &&
               aHeight == img->GetSize().height);

    if (format == GRALLOC_PLANAR_YCBCR) {
      
      void* imgPtr = nullptr;
      GrallocImage* nativeImage = static_cast<GrallocImage*>(img);
      SurfaceDescriptor handle = nativeImage->GetSurfaceDescriptor();
      SurfaceDescriptorGralloc gralloc = handle.get_SurfaceDescriptorGralloc();
      sp<GraphicBuffer> graphicBuffer = GrallocBufferActor::GetFrom(gralloc);
      graphicBuffer->lock(GraphicBuffer::USAGE_SW_READ_MASK, &imgPtr);
      uint8_t* src = static_cast<uint8_t*>(imgPtr);

      
      MOZ_ASSERT(graphicBuffer->getPixelFormat() ==
                 HAL_PIXEL_FORMAT_YCrCb_420_SP);

      
      PlanarYCbCrData nv21;
      
      nv21.mYChannel = src;
      nv21.mYSize.width = aWidth;
      nv21.mYSize.height = aHeight;
      nv21.mYStride = aWidth;
      nv21.mYSkip = 0;
      
      nv21.mCrChannel = src + yLen;
      nv21.mCrSkip = 1;
      nv21.mCbChannel = nv21.mCrChannel + 1;
      nv21.mCbSkip = 1;
      nv21.mCbCrStride = aWidth;
      
      nv21.mCbCrSize.width = aWidth / 2;
      nv21.mCbCrSize.height = aHeight / 2;

      ConvertPlanarYCbCrToNV12(&nv21, dst);

      graphicBuffer->unlock();
    } else if (format == PLANAR_YCBCR) {
      ConvertPlanarYCbCrToNV12(static_cast<PlanarYCbCrImage*>(img)->GetData(),
                             dst);
    } else {
      
      NS_ERROR("Unsupported input image type.");
    }
  }

  
  result = mCodec->queueInputBuffer(index, 0, dstSize, aTimestamp, aInputFlags);

  return result == OK ? NS_OK : NS_ERROR_FAILURE;
}

status_t
OMXVideoEncoder::AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                     ABuffer* aData)
{
  
  
  return GenerateAVCDescriptorBlob(aData, aOutputBuf);
}



void OMXVideoEncoder::AppendFrame(nsTArray<uint8_t>* aOutputBuf,
                                  const uint8_t* aData, size_t aSize)
{
  uint8_t length[] = {
    (aSize >> 24) & 0xFF,
    (aSize >> 16) & 0xFF,
    (aSize >> 8) & 0xFF,
    aSize & 0xFF,
  };
  aOutputBuf->SetCapacity(aSize);
  aOutputBuf->AppendElements(length, sizeof(length));
  aOutputBuf->AppendElements(aData + sizeof(length), aSize);
}

nsresult
OMXAudioEncoder::Configure(int aChannels, int aSamplingRate)
{
  MOZ_ASSERT(!mStarted);

  NS_ENSURE_TRUE(aChannels > 0 && aSamplingRate > 0, NS_ERROR_INVALID_ARG);

  
  sp<AMessage> format = new AMessage;
  
  format->setString("mime", MEDIA_MIMETYPE_AUDIO_AAC);
  format->setInt32("bitrate", kAACBitrate);
  format->setInt32("aac-profile", OMX_AUDIO_AACObjectLC);
  
  format->setInt32("channel-count", aChannels);
  format->setInt32("sample-rate", aSamplingRate);

  status_t result = mCodec->configure(format, nullptr, nullptr,
                                      MediaCodec::CONFIGURE_FLAG_ENCODE);
  NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);

  mChannels = aChannels;
  mSampleDuration = 1000000 / aSamplingRate;
  result = Start();

  return result == OK ? NS_OK : NS_ERROR_FAILURE;
}

class InputBufferHelper MOZ_FINAL {
public:
  InputBufferHelper(sp<MediaCodec>& aCodec, Vector<sp<ABuffer> >& aBuffers)
    : mCodec(aCodec)
    , mBuffers(aBuffers)
    , mIndex(0)
    , mData(nullptr)
    , mOffset(0)
    , mCapicity(0)
  {}

  ~InputBufferHelper()
  {
    
    MOZ_ASSERT(!mData);
  }

  status_t Dequeue()
  {
    
    MOZ_ASSERT(!mData);

    status_t result = mCodec->dequeueInputBuffer(&mIndex,
                                                 INPUT_BUFFER_TIMEOUT_US);
    NS_ENSURE_TRUE(result == OK, result);
    sp<ABuffer> inBuf = mBuffers.itemAt(mIndex);
    mData = inBuf->data();
    mCapicity = inBuf->capacity();
    mOffset = 0;

    return OK;
  }

  uint8_t* GetPointer() { return mData + mOffset; }

  const size_t AvailableSize() { return mCapicity - mOffset; }

  void IncreaseOffset(size_t aValue)
  {
    
    MOZ_ASSERT(mOffset + aValue <= mCapicity);
    mOffset += aValue;
  }

  status_t Enqueue(int64_t aTimestamp, int aFlags)
  {
    
    MOZ_ASSERT(mData);

    
    status_t result = mCodec->queueInputBuffer(mIndex, 0, mOffset, aTimestamp,
                                               aFlags);
    NS_ENSURE_TRUE(result == OK, result);
    mData = nullptr;

    return OK;
  }

private:
  sp<MediaCodec>& mCodec;
  Vector<sp<ABuffer> >& mBuffers;
  size_t mIndex;
  uint8_t* mData;
  size_t mCapicity;
  size_t mOffset;
};

nsresult
OMXAudioEncoder::Encode(const AudioSegment& aSegment, int aInputFlags)
{
#ifndef MOZ_SAMPLE_TYPE_S16
#error MediaCodec accepts only 16-bit PCM data.
#endif

  MOZ_ASSERT(mStarted, "Configure() should be called before Encode().");

  size_t numSamples = aSegment.GetDuration();

  
  InputBufferHelper buffer(mCodec, mInputBufs);
  status_t result = buffer.Dequeue();
  NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);

  size_t samplesCopied = 0; 

  if (numSamples > 0) {
    
    AudioSegment::ChunkIterator iter(const_cast<AudioSegment&>(aSegment));
    while (!iter.IsEnded()) {
      AudioChunk chunk = *iter;
      size_t samplesToCopy = chunk.GetDuration(); 
      size_t bytesToCopy = samplesToCopy * mChannels * sizeof(AudioDataValue);

      if (bytesToCopy > buffer.AvailableSize()) {
        
        
        
        result = buffer.Enqueue(mTimestamp, aInputFlags & ~BUFFER_EOS);
        NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);

        mTimestamp += samplesCopied * mSampleDuration;
        samplesCopied = 0;

        result = buffer.Dequeue();
        NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);
      }

      AudioDataValue* dst = reinterpret_cast<AudioDataValue*>(buffer.GetPointer());
      if (!chunk.IsNull()) {
        
        AudioTrackEncoder::InterleaveTrackData(chunk, samplesToCopy, mChannels,
                                               dst);
      } else {
        
        memset(dst, 0, bytesToCopy);
      }

      samplesCopied += samplesToCopy;
      buffer.IncreaseOffset(bytesToCopy);
      iter.Next();
    }
  } else if (aInputFlags & BUFFER_EOS) {
    
    
    size_t bytesToCopy = mChannels * sizeof(AudioDataValue);
    memset(buffer.GetPointer(), 0, bytesToCopy);
    buffer.IncreaseOffset(bytesToCopy);
    samplesCopied = 1;
  }

  if (samplesCopied > 0) {
    result = buffer.Enqueue(mTimestamp, aInputFlags);
    NS_ENSURE_TRUE(result == OK, NS_ERROR_FAILURE);

    mTimestamp += samplesCopied * mSampleDuration;
  }

  return NS_OK;
}




status_t
OMXAudioEncoder::AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                     ABuffer* aData)
{
  MOZ_ASSERT(aData);

  const size_t csdSize = aData->size();

  
  
  
  NS_ENSURE_TRUE(csdSize == 2, ERROR_MALFORMED);
  
  
  NS_ENSURE_TRUE((aData->data()[1] & 0x04) == 0, ERROR_MALFORMED);

  
  const uint8_t decConfig[] = {
    0x04,                   
    15 + csdSize,           
    0x40,                   
    0x15,                   
    0x00, 0x03, 0x00,       
    0x00, 0x01, 0x77, 0x00, 
    0x00, 0x01, 0x77, 0x00, 
    0x05,                   
    csdSize,                
  };
  
  const uint8_t slConfig[] = {
    0x06, 
    0x01, 
    0x02, 
  };

  aOutputBuf->SetCapacity(sizeof(decConfig) + csdSize + sizeof(slConfig));
  aOutputBuf->AppendElements(decConfig, sizeof(decConfig));
  aOutputBuf->AppendElements(aData->data(), csdSize);
  aOutputBuf->AppendElements(slConfig, sizeof(slConfig));

  return OK;
}

nsresult
OMXCodecWrapper::GetNextEncodedFrame(nsTArray<uint8_t>* aOutputBuf,
                                     int64_t* aOutputTimestamp,
                                     int* aOutputFlags, int64_t aTimeOut)
{
  MOZ_ASSERT(mStarted,
             "Configure() should be called before GetNextEncodedFrame().");

  
  size_t index = 0;
  size_t outOffset = 0;
  size_t outSize = 0;
  int64_t outTimeUs = 0;
  uint32_t outFlags = 0;
  bool retry = false;
  do {
    status_t result = mCodec->dequeueOutputBuffer(&index, &outOffset, &outSize,
                                                  &outTimeUs, &outFlags,
                                                  aTimeOut);
    switch (result) {
      case OK:
        break;
      case INFO_OUTPUT_BUFFERS_CHANGED:
        
        result = mCodec->getOutputBuffers(&mOutputBufs);
        
        retry = true;
        break;
      case INFO_FORMAT_CHANGED:
        
        
        return NS_OK;
      case -EAGAIN:
        
        return NS_OK;
      default:
        CODEC_ERROR("MediaCodec error:%d", result);
        MOZ_ASSERT(false, "MediaCodec error.");
        return NS_ERROR_FAILURE;
    }
  } while (retry);

  if (aOutputBuf) {
    aOutputBuf->Clear();
    const sp<ABuffer> omxBuf = mOutputBufs.itemAt(index);
    if (outFlags & MediaCodec::BUFFER_FLAG_CODECCONFIG) {
      
      if (AppendDecoderConfig(aOutputBuf, omxBuf.get()) != OK) {
        mCodec->releaseOutputBuffer(index);
        return NS_ERROR_FAILURE;
      }
    } else {
      AppendFrame(aOutputBuf, omxBuf->data(), omxBuf->size());
    }
  }
  mCodec->releaseOutputBuffer(index);

  if (aOutputTimestamp) {
    *aOutputTimestamp = outTimeUs;
  }

  if (aOutputFlags) {
    *aOutputFlags = outFlags;
  }

  return NS_OK;
}

}
