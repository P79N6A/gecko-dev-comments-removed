



#include "CSFLog.h"

#include "WebrtcOMXH264VideoCodec.h"


#include <avc_utils.h>
#include <binder/ProcessState.h>
#include <foundation/ABuffer.h>
#include <foundation/AMessage.h>
#include <gui/Surface.h>
#include <media/ICrypto.h>
#include <MediaCodec.h>
#include <MediaDefs.h>
#include <MediaErrors.h>
#include <MetaData.h>
#include <OMX_Component.h>
using namespace android;


#include "common_video/interface/texture_video_frame.h"
#include "video_engine/include/vie_external_codec.h"


#include "GonkNativeWindow.h"
#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"
#include "nsThreadUtils.h"
#include "OMXCodecWrapper.h"
#include "TextureClient.h"

#define DEQUEUE_BUFFER_TIMEOUT_US (100 * 1000ll) // 100ms.
#define START_DEQUEUE_BUFFER_TIMEOUT_US (10 * DEQUEUE_BUFFER_TIMEOUT_US) // 1s.
#define DRAIN_THREAD_TIMEOUT_US  (1000 * 1000ll) // 1s.

#define LOG_TAG "WebrtcOMXH264VideoCodec"
#define CODEC_LOGV(...) CSFLogInfo(LOG_TAG, __VA_ARGS__)
#define CODEC_LOGD(...) CSFLogDebug(LOG_TAG, __VA_ARGS__)
#define CODEC_LOGI(...) CSFLogInfo(LOG_TAG, __VA_ARGS__)
#define CODEC_LOGW(...) CSFLogWarn(LOG_TAG, __VA_ARGS__)
#define CODEC_LOGE(...) CSFLogError(LOG_TAG, __VA_ARGS__)

namespace mozilla {

static uint8_t kNALStartCode[] = { 0x00, 0x00, 0x00, 0x01 };
enum {
  kNALTypeSPS = 7,
  kNALTypePPS = 8,
};






class DummyRefCountBase {
public:
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageNativeHandle)
  
  virtual ~DummyRefCountBase() {}
};






class ImageNativeHandle MOZ_FINAL
  : public webrtc::NativeHandle
  , public DummyRefCountBase
{
public:
  ImageNativeHandle(layers::Image* aImage)
    : mImage(aImage)
  {}

  
  virtual void* GetHandle() MOZ_OVERRIDE { return mImage.get(); }

  virtual int AddRef() MOZ_OVERRIDE
  {
    return DummyRefCountBase::AddRef();
  }

  virtual int Release() MOZ_OVERRIDE
  {
    return DummyRefCountBase::Release();
  }

private:
  RefPtr<layers::Image> mImage;
};

struct EncodedFrame
{
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mTimestamp;
  int64_t mRenderTimeMs;
};












class OMXOutputDrain : public nsRunnable
{
public:
  void Start() {
    MonitorAutoLock lock(mMonitor);
    if (mThread == nullptr) {
      NS_NewNamedThread("OMXOutputDrain", getter_AddRefs(mThread));
    }
    CODEC_LOGD("OMXOutputDrain started");
    mEnding = false;
    mThread->Dispatch(this, NS_DISPATCH_NORMAL);
  }

  void Stop() {
    MonitorAutoLock lock(mMonitor);
    mEnding = true;
    lock.NotifyAll(); 

    if (mThread != nullptr) {
      mThread->Shutdown();
      mThread = nullptr;
    }
    CODEC_LOGD("OMXOutputDrain stopped");
  }

  void QueueInput(const EncodedFrame& aFrame)
  {
    MonitorAutoLock lock(mMonitor);

    MOZ_ASSERT(mThread);

    mInputFrames.push(aFrame);
    
    lock.NotifyAll();
  }

  NS_IMETHODIMP Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mThread);

    MonitorAutoLock lock(mMonitor);
    while (true) {
      if (mInputFrames.empty()) {
        
        lock.Wait();
      }

      if (mEnding) {
        
        break;
      }

      MOZ_ASSERT(!mInputFrames.empty());
      EncodedFrame frame = mInputFrames.front();
      bool shouldPop = false;
      {
        
        MonitorAutoUnlock unlock(mMonitor);
        
        shouldPop = DrainOutput(frame);
      }
      if (shouldPop) {
        mInputFrames.pop();
      }
    }

    CODEC_LOGD("OMXOutputDrain Ended");
    return NS_OK;
  }

protected:
  OMXOutputDrain()
    : mMonitor("OMXOutputDrain monitor")
    , mEnding(false)
  {}

  
  
  
  
  
  
  
  virtual bool DrainOutput(const EncodedFrame& aFrame) = 0;

private:
  
  
  Monitor mMonitor;
  nsCOMPtr<nsIThread> mThread;
  std::queue<EncodedFrame> mInputFrames;
  bool mEnding;
};




class WebrtcOMXDecoder MOZ_FINAL : public GonkNativeWindowNewFrameCallback
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WebrtcOMXDecoder)
public:
  WebrtcOMXDecoder(const char* aMimeType,
                   webrtc::DecodedImageCallback* aCallback)
    : mWidth(0)
    , mHeight(0)
    , mStarted(false)
    , mDecodedFrameLock("WebRTC decoded frame lock")
    , mCallback(aCallback)
  {
    
    android::ProcessState::self()->startThreadPool();

    mLooper = new ALooper;
    mLooper->start();
    mCodec = MediaCodec::CreateByType(mLooper, aMimeType, false );
  }

  virtual ~WebrtcOMXDecoder()
  {
    if (mStarted) {
      Stop();
    }
    if (mCodec != nullptr) {
      mCodec->release();
      mCodec.clear();
    }
    mLooper.clear();
  }

  
  static status_t ExtractPicDimensions(uint8_t* aData, size_t aSize,
                                       int32_t* aWidth, int32_t* aHeight)
  {
    MOZ_ASSERT(aData && aSize > 1);
    if ((aData[0] & 0x1f) != kNALTypeSPS) {
      return ERROR_MALFORMED;
    }
    sp<ABuffer> sps = new ABuffer(aData, aSize);
    FindAVCDimensions(sps, aWidth, aHeight);
    return OK;
  }

  
  status_t ConfigureWithPicDimensions(int32_t aWidth, int32_t aHeight)
  {
    MOZ_ASSERT(mCodec != nullptr);
    if (mCodec == nullptr) {
      return INVALID_OPERATION;
    }

    CODEC_LOGD("OMX:%p decoder width:%d height:%d", this, aWidth, aHeight);

    sp<AMessage> config = new AMessage();
    config->setString("mime", MEDIA_MIMETYPE_VIDEO_AVC);
    config->setInt32("width", aWidth);
    config->setInt32("height", aHeight);
    mWidth = aWidth;
    mHeight = aHeight;

    sp<Surface> surface = nullptr;
    mNativeWindow = new GonkNativeWindow();
    if (mNativeWindow.get()) {
      
      mNativeWindow->setNewFrameCallback(this);
      surface = new Surface(mNativeWindow->getBufferQueue());
    }
    status_t result = mCodec->configure(config, surface, nullptr, 0);
    if (result == OK) {
      result = Start();
    }
    return result;
  }

  status_t
  FillInput(const webrtc::EncodedImage& aEncoded, bool aIsFirstFrame,
            int64_t& aRenderTimeMs)
  {
    MOZ_ASSERT(mCodec != nullptr && aEncoded._buffer && aEncoded._length > 0);
    if (mCodec == nullptr || !aEncoded._buffer || aEncoded._length == 0) {
      return INVALID_OPERATION;
    }

    size_t index;
    status_t err = mCodec->dequeueInputBuffer(&index,
      aIsFirstFrame ? START_DEQUEUE_BUFFER_TIMEOUT_US : DEQUEUE_BUFFER_TIMEOUT_US);
    if (err != OK) {
      CODEC_LOGE("decode dequeue input buffer error:%d", err);
      return err;
    }

    
    size_t size = aEncoded._length + sizeof(kNALStartCode);
    const sp<ABuffer>& omxIn = mInputBuffers.itemAt(index);
    MOZ_ASSERT(omxIn->capacity() >= size);
    omxIn->setRange(0, size);
    
    
    uint8_t* dst = omxIn->data();
    memcpy(dst, kNALStartCode, sizeof(kNALStartCode));
    memcpy(dst + sizeof(kNALStartCode), aEncoded._buffer, aEncoded._length);
    int64_t inputTimeUs = aEncoded._timeStamp * 1000 / 90; 
    
    uint32_t flags = 0;
    if (aEncoded._frameType == webrtc::kKeyFrame) {
      int nalType = dst[sizeof(kNALStartCode)] & 0x1f;
      flags = (nalType == kNALTypeSPS || nalType == kNALTypePPS) ?
              MediaCodec::BUFFER_FLAG_CODECCONFIG : MediaCodec::BUFFER_FLAG_SYNCFRAME;
    }
    err = mCodec->queueInputBuffer(index, 0, size, inputTimeUs, flags);
    if (err == OK && !(flags & MediaCodec::BUFFER_FLAG_CODECCONFIG)) {
      if (mOutputDrain == nullptr) {
        mOutputDrain = new OutputDrain(this);
        mOutputDrain->Start();
      }
      EncodedFrame frame;
      frame.mWidth = mWidth;
      frame.mHeight = mHeight;
      frame.mTimestamp = aEncoded._timeStamp;
      frame.mRenderTimeMs = aRenderTimeMs;
      mOutputDrain->QueueInput(frame);
    }

    return err;
  }

  status_t
  DrainOutput(const EncodedFrame& aFrame)
  {
    MOZ_ASSERT(mCodec != nullptr);
    if (mCodec == nullptr) {
      return INVALID_OPERATION;
    }

    size_t index = 0;
    size_t outOffset = 0;
    size_t outSize = 0;
    int64_t outTime = -1ll;
    uint32_t outFlags = 0;
    status_t err = mCodec->dequeueOutputBuffer(&index, &outOffset, &outSize,
                                               &outTime, &outFlags,
                                               DRAIN_THREAD_TIMEOUT_US);
    switch (err) {
      case OK:
        break;
      case -EAGAIN:
        
        CODEC_LOGI("decode dequeue OMX output buffer timed out. Try later.");
        return err;
      case INFO_FORMAT_CHANGED:
        
        
        CODEC_LOGD("decode dequeue OMX output buffer format change");
        return err;
      case INFO_OUTPUT_BUFFERS_CHANGED:
        
        
        CODEC_LOGD("decode dequeue OMX output buffer change");
        err = mCodec->getOutputBuffers(&mOutputBuffers);
        MOZ_ASSERT(err == OK);
        return INFO_OUTPUT_BUFFERS_CHANGED;
      default:
        CODEC_LOGE("decode dequeue OMX output buffer error:%d", err);
        
        return OK;
    }

    if (mCallback) {
      {
        
        MutexAutoLock lock(mDecodedFrameLock);
        mDecodedFrames.push(aFrame);
      }
      
      
      mCodec->renderOutputBufferAndRelease(index);
      
      
    } else {
      mCodec->releaseOutputBuffer(index);
    }

    return err;
  }

  
  
  void OnNewFrame() MOZ_OVERRIDE
  {
    RefPtr<layers::TextureClient> buffer = mNativeWindow->getCurrentBuffer();
    MOZ_ASSERT(buffer != nullptr);

    layers::GrallocImage::GrallocData grallocData;
    grallocData.mPicSize = buffer->GetSize();
    grallocData.mGraphicBuffer = buffer;

    nsAutoPtr<layers::GrallocImage> grallocImage(new layers::GrallocImage());
    grallocImage->SetData(grallocData);

    
    int64_t timestamp = -1;
    int64_t renderTimeMs = -1;
    {
      MutexAutoLock lock(mDecodedFrameLock);
      if (mDecodedFrames.empty()) {
        return;
      }
      EncodedFrame decoded = mDecodedFrames.front();
      timestamp = decoded.mTimestamp;
      renderTimeMs = decoded.mRenderTimeMs;
      mDecodedFrames.pop();
    }
    MOZ_ASSERT(timestamp >= 0 && renderTimeMs >= 0);

    nsAutoPtr<webrtc::I420VideoFrame> videoFrame(
      new webrtc::TextureVideoFrame(new ImageNativeHandle(grallocImage.forget()),
                                    grallocData.mPicSize.width,
                                    grallocData.mPicSize.height,
                                    timestamp,
                                    renderTimeMs));
    if (videoFrame != nullptr) {
      mCallback->Decoded(*videoFrame);
    }
  }

private:
  class OutputDrain : public OMXOutputDrain
  {
  public:
    OutputDrain(WebrtcOMXDecoder* aOMX)
      : OMXOutputDrain()
      , mOMX(aOMX)
    {}

  protected:
    virtual bool DrainOutput(const EncodedFrame& aFrame) MOZ_OVERRIDE
    {
      return (mOMX->DrainOutput(aFrame) == OK);
    }

  private:
    WebrtcOMXDecoder* mOMX;
  };

  status_t Start()
  {
    MOZ_ASSERT(!mStarted);
    if (mStarted) {
      return OK;
    }

    status_t err = mCodec->start();
    if (err == OK) {
      mStarted = true;
      mCodec->getInputBuffers(&mInputBuffers);
      mCodec->getOutputBuffers(&mOutputBuffers);
    }

    return err;
  }

  status_t Stop()
  {
    MOZ_ASSERT(mStarted);
    if (!mStarted) {
      return OK;
    }

    
    {
      MutexAutoLock lock(mDecodedFrameLock);
      while (!mDecodedFrames.empty()) {
        mDecodedFrames.pop();
      }
    }

    if (mOutputDrain != nullptr) {
      mOutputDrain->Stop();
      mOutputDrain = nullptr;
    }

    status_t err = mCodec->stop();
    if (err == OK) {
      mInputBuffers.clear();
      mOutputBuffers.clear();
      mStarted = false;
    } else {
      MOZ_ASSERT(false);
    }

    return err;
  }

  sp<ALooper> mLooper;
  sp<MediaCodec> mCodec; 
  int mWidth;
  int mHeight;
  android::Vector<sp<ABuffer> > mInputBuffers;
  android::Vector<sp<ABuffer> > mOutputBuffers;
  bool mStarted;

  sp<GonkNativeWindow> mNativeWindow;

  RefPtr<OutputDrain> mOutputDrain;
  webrtc::DecodedImageCallback* mCallback;

  Mutex mDecodedFrameLock; 
  std::queue<EncodedFrame> mDecodedFrames;
};

class EncOutputDrain : public OMXOutputDrain
{
public:
  EncOutputDrain(OMXVideoEncoder* aOMX, webrtc::EncodedImageCallback* aCallback)
    : OMXOutputDrain()
    , mOMX(aOMX)
    , mCallback(aCallback)
    , mIsPrevOutputParamSets(false)
  {}

protected:
  virtual bool DrainOutput(const EncodedFrame& aInputFrame) MOZ_OVERRIDE
  {
    nsTArray<uint8_t> output;
    int64_t timeUs = -1ll;
    int flags = 0;
    nsresult rv = mOMX->GetNextEncodedFrame(&output, &timeUs, &flags,
                                            DRAIN_THREAD_TIMEOUT_US);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      
      return true;
    }

    if (output.Length() == 0) {
      
      CODEC_LOGD("OMX:%p (encode no output available this time)", mOMX);
      return false;
    }

    bool isParamSets = (flags & MediaCodec::BUFFER_FLAG_CODECCONFIG);
    bool isIFrame = (flags & MediaCodec::BUFFER_FLAG_SYNCFRAME);
    
    MOZ_ASSERT(!(isParamSets && isIFrame));

    if (mCallback) {
      
      
      
      
      webrtc::EncodedImage encoded(output.Elements(), output.Length(),
                                   output.Capacity());
      encoded._frameType = (isParamSets || isIFrame) ?
                           webrtc::kKeyFrame : webrtc::kDeltaFrame;
      encoded._encodedWidth = aInputFrame.mWidth;
      encoded._encodedHeight = aInputFrame.mHeight;
      encoded._timeStamp = aInputFrame.mTimestamp;
      encoded.capture_time_ms_ = aInputFrame.mRenderTimeMs;
      encoded._completeFrame = true;

      
      SendEncodedDataToCallback(encoded, isIFrame && !mIsPrevOutputParamSets);
      mIsPrevOutputParamSets = isParamSets;
    }

    
    
    return !isParamSets;
  }

private:
  
  
  
  void SendEncodedDataToCallback(webrtc::EncodedImage& aEncodedImage,
                                 bool aPrependParamSets)
  {
    
    webrtc::EncodedImage nalu(aEncodedImage);

    if (aPrependParamSets) {
      
      nsTArray<uint8_t> paramSets;
      mOMX->GetCodecConfig(&paramSets);
      MOZ_ASSERT(paramSets.Length() > 4); 
      
      nalu._buffer = paramSets.Elements();
      nalu._length = paramSets.Length();
      
      SendEncodedDataToCallback(nalu, false);
    }

    
    const uint8_t* data = aEncodedImage._buffer;
    size_t size = aEncodedImage._length;
    const uint8_t* nalStart = nullptr;
    size_t nalSize = 0;
    while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
      nalu._buffer = const_cast<uint8_t*>(nalStart);
      nalu._length = nalSize;
      mCallback->Encoded(nalu, nullptr, nullptr);
    }
  }

  OMXVideoEncoder* mOMX;
  webrtc::EncodedImageCallback* mCallback;
  bool mIsPrevOutputParamSets;
};


WebrtcOMXH264VideoEncoder::WebrtcOMXH264VideoEncoder()
  : mOMX(nullptr)
  , mCallback(nullptr)
  , mWidth(0)
  , mHeight(0)
  , mFrameRate(0)
  , mOMXConfigured(false)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p constructed", this);
}

int32_t
WebrtcOMXH264VideoEncoder::InitEncode(const webrtc::VideoCodec* aCodecSettings,
                                      int32_t aNumOfCores,
                                      uint32_t aMaxPayloadSize)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p init", this);

  if (mOMX == nullptr) {
    nsAutoPtr<OMXVideoEncoder> omx(OMXCodecWrapper::CreateAVCEncoder());
    if (NS_WARN_IF(omx == nullptr)) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    mOMX = omx.forget();
  }

  
  
  
  mWidth = aCodecSettings->width;
  mHeight = aCodecSettings->height;
  mFrameRate = aCodecSettings->maxFramerate;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoEncoder::Encode(const webrtc::I420VideoFrame& aInputImage,
                                  const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                                  const std::vector<webrtc::VideoFrameType>* aFrameTypes)
{
  MOZ_ASSERT(mOMX != nullptr);
  if (mOMX == nullptr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (!mOMXConfigured) {
    mOMX->Configure(mWidth, mHeight, mFrameRate,
                    OMXVideoEncoder::BlobFormat::AVC_NAL);
    mOMXConfigured = true;
    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p start OMX with image size:%ux%u",
               this, mWidth, mHeight);
  }

  
  layers::PlanarYCbCrData yuvData;
  yuvData.mYChannel = const_cast<uint8_t*>(aInputImage.buffer(webrtc::kYPlane));
  yuvData.mYSize = gfx::IntSize(aInputImage.width(), aInputImage.height());
  yuvData.mYStride = aInputImage.stride(webrtc::kYPlane);
  MOZ_ASSERT(aInputImage.stride(webrtc::kUPlane) == aInputImage.stride(webrtc::kVPlane));
  yuvData.mCbCrStride = aInputImage.stride(webrtc::kUPlane);
  yuvData.mCbChannel = const_cast<uint8_t*>(aInputImage.buffer(webrtc::kUPlane));
  yuvData.mCrChannel = const_cast<uint8_t*>(aInputImage.buffer(webrtc::kVPlane));
  yuvData.mCbCrSize = gfx::IntSize((yuvData.mYSize.width + 1) / 2,
                                   (yuvData.mYSize.height + 1) / 2);
  yuvData.mPicSize = yuvData.mYSize;
  yuvData.mStereoMode = StereoMode::MONO;
  layers::PlanarYCbCrImage img(nullptr);
  img.SetDataNoCopy(yuvData);

  nsresult rv = mOMX->Encode(&img,
                             yuvData.mYSize.width,
                             yuvData.mYSize.height,
                             aInputImage.timestamp() * 1000 / 90, 
                             0);
  if (rv == NS_OK) {
    if (mOutputDrain == nullptr) {
      mOutputDrain = new EncOutputDrain(mOMX, mCallback);
      mOutputDrain->Start();
    }
    EncodedFrame frame;
    frame.mWidth = mWidth;
    frame.mHeight = mHeight;
    frame.mTimestamp = aInputImage.timestamp();
    frame.mRenderTimeMs = aInputImage.render_time_ms();
    mOutputDrain->QueueInput(frame);
  }

  return (rv == NS_OK) ? WEBRTC_VIDEO_CODEC_OK : WEBRTC_VIDEO_CODEC_ERROR;
}

int32_t
WebrtcOMXH264VideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* aCallback)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoEncoder::Release()
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p will be released", this);

  if (mOutputDrain != nullptr) {
    mOutputDrain->Stop();
    mOutputDrain = nullptr;
  }

  mOMX = nullptr;

  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcOMXH264VideoEncoder::~WebrtcOMXH264VideoEncoder()
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p will be destructed", this);

  Release();
}





int32_t
WebrtcOMXH264VideoEncoder::SetChannelParameters(uint32_t aPacketLossRate,
                                                int aRoundTripTimeMs)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p set channel packet loss:%u, rtt:%d",
             this, aPacketLossRate, aRoundTripTimeMs);

  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t
WebrtcOMXH264VideoEncoder::SetRates(uint32_t aBitRate, uint32_t aFrameRate)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p set bitrate:%u, frame rate:%u)",
             this, aBitRate, aFrameRate);
  MOZ_ASSERT(mOMX != nullptr);
  if (mOMX == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  mOMX->SetBitrate(aBitRate);

  return WEBRTC_VIDEO_CODEC_OK;
}


WebrtcOMXH264VideoDecoder::WebrtcOMXH264VideoDecoder()
  : mCallback(nullptr)
  , mOMX(nullptr)
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be constructed", this);
}

int32_t
WebrtcOMXH264VideoDecoder::InitDecode(const webrtc::VideoCodec* aCodecSettings,
                                      int32_t aNumOfCores)
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p init OMX:%p", this, mOMX.get());

  
  

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoDecoder::Decode(const webrtc::EncodedImage& aInputImage,
                                  bool aMissingFrames,
                                  const webrtc::RTPFragmentationHeader* aFragmentation,
                                  const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                                  int64_t aRenderTimeMs)
{
  if (aInputImage._length== 0 || !aInputImage._buffer) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  bool configured = !!mOMX;
  if (!configured) {
    
    int32_t width;
    int32_t height;
    status_t result = WebrtcOMXDecoder::ExtractPicDimensions(aInputImage._buffer,
                                                             aInputImage._length,
                                                             &width, &height);
    if (result != OK) {
      
      CODEC_LOGI("WebrtcOMXH264VideoDecoder:%p missing SPS in input");
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    RefPtr<WebrtcOMXDecoder> omx = new WebrtcOMXDecoder(MEDIA_MIMETYPE_VIDEO_AVC,
                                                        mCallback);
    result = omx->ConfigureWithPicDimensions(width, height);
    if (NS_WARN_IF(result != OK)) {
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p start OMX", this);
    mOMX = omx;
  }

  bool feedFrame = true;
  while (feedFrame) {
    int64_t timeUs;
    status_t err = mOMX->FillInput(aInputImage, !configured, aRenderTimeMs);
    feedFrame = (err == -EAGAIN); 
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* aCallback)
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoDecoder::Release()
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be released", this);

  mOMX = nullptr;

  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcOMXH264VideoDecoder::~WebrtcOMXH264VideoDecoder()
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be destructed", this);
  Release();
}

int32_t
WebrtcOMXH264VideoDecoder::Reset()
{
  CODEC_LOGW("WebrtcOMXH264VideoDecoder::Reset() will NOT reset decoder");
  return WEBRTC_VIDEO_CODEC_OK;
}

}
