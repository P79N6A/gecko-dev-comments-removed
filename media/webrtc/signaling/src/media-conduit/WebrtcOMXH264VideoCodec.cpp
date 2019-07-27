



#include "CSFLog.h"

#include "WebrtcOMXH264VideoCodec.h"


#include <avc_utils.h>
#include <binder/ProcessState.h>
#include <foundation/ABuffer.h>
#include <foundation/AMessage.h>
#include <gui/Surface.h>
#include <media/ICrypto.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <OMX_Component.h>
using namespace android;


#include "webrtc/common_video/interface/texture_video_frame.h"
#include "webrtc/video_engine/include/vie_external_codec.h"
#include "runnable_utils.h"


#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
#include "GonkBufferQueueProducer.h"
#endif
#include "GonkNativeWindow.h"
#include "GrallocImages.h"
#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"
#include "nsThreadUtils.h"
#include "OMXCodecWrapper.h"
#include "TextureClient.h"
#include "mozilla/IntegerPrintfMacros.h"

#define DEQUEUE_BUFFER_TIMEOUT_US (100 * 1000ll) // 100ms.
#define START_DEQUEUE_BUFFER_TIMEOUT_US (10 * DEQUEUE_BUFFER_TIMEOUT_US) // 1s.
#define DRAIN_THREAD_TIMEOUT_US  (1000 * 1000ll) // 1s.

#define WOHVC_LOG_TAG "WebrtcOMXH264VideoCodec"
#define CODEC_LOGV(...) CSFLogInfo(WOHVC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGD(...) CSFLogDebug(WOHVC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGI(...) CSFLogInfo(WOHVC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGW(...) CSFLogWarn(WOHVC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGE(...) CSFLogError(WOHVC_LOG_TAG, __VA_ARGS__)

namespace mozilla {

static const uint8_t kNALStartCode[] = { 0x00, 0x00, 0x00, 0x01 };
enum {
  kNALTypeIDR = 5,
  kNALTypeSPS = 7,
  kNALTypePPS = 8,
};






class DummyRefCountBase {
public:
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DummyRefCountBase)
protected:
  
  virtual ~DummyRefCountBase() {}
};






class ImageNativeHandle final
  : public webrtc::NativeHandle
  , public DummyRefCountBase
{
public:
  ImageNativeHandle(layers::Image* aImage)
    : mImage(aImage)
  {}

  
  virtual void* GetHandle() override { return mImage.get(); }

  virtual int AddRef() override
  {
    return DummyRefCountBase::AddRef();
  }

  virtual int Release() override
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

static void
ShutdownThread(nsCOMPtr<nsIThread>& aThread)
{
  aThread->Shutdown();
}












class OMXOutputDrain : public nsRunnable
{
public:
  void Start() {
    CODEC_LOGD("OMXOutputDrain starting");
    MonitorAutoLock lock(mMonitor);
    if (mThread == nullptr) {
      NS_NewNamedThread("OMXOutputDrain", getter_AddRefs(mThread));
    }
    CODEC_LOGD("OMXOutputDrain started");
    mEnding = false;
    mThread->Dispatch(this, NS_DISPATCH_NORMAL);
  }

  void Stop() {
    CODEC_LOGD("OMXOutputDrain stopping");
    MonitorAutoLock lock(mMonitor);
    mEnding = true;
    lock.NotifyAll(); 

    if (mThread != nullptr) {
      MonitorAutoUnlock unlock(mMonitor);
      CODEC_LOGD("OMXOutputDrain thread shutdown");
      NS_DispatchToMainThread(
        WrapRunnableNM<decltype(&ShutdownThread),
                       nsCOMPtr<nsIThread> >(&ShutdownThread, mThread));
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

  NS_IMETHODIMP Run() override
  {
    MOZ_ASSERT(mThread);

    MonitorAutoLock lock(mMonitor);
    while (true) {
      if (mInputFrames.empty()) {
        
        lock.Wait();
      }

      if (mEnding) {
        CODEC_LOGD("OMXOutputDrain Run() ending");
        
        break;
      }

      MOZ_ASSERT(!mInputFrames.empty());
      {
        
        MonitorAutoUnlock unlock(mMonitor);
        DrainOutput();
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

  
  
  
  
  

  
  
  virtual bool DrainOutput() = 0;

protected:
  
  
  Monitor mMonitor;
  std::queue<EncodedFrame> mInputFrames;

private:
  
  nsCOMPtr<nsIThread> mThread;
  bool mEnding;
};


static bool IsParamSets(uint8_t* aData, size_t aSize)
{
  MOZ_ASSERT(aData && aSize > sizeof(kNALStartCode));
  return (aData[sizeof(kNALStartCode)] & 0x1f) == kNALTypeSPS;
}


static size_t ParamSetLength(uint8_t* aData, size_t aSize)
{
  const uint8_t* data = aData;
  size_t size = aSize;
  const uint8_t* nalStart = nullptr;
  size_t nalSize = 0;
  while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
    if ((*nalStart & 0x1f) != kNALTypeSPS &&
        (*nalStart & 0x1f) != kNALTypePPS) {
      MOZ_ASSERT(nalStart - sizeof(kNALStartCode) >= aData);
      return (nalStart - sizeof(kNALStartCode)) - aData; 
    }
  }
  return aSize; 
}




class WebrtcOMXDecoder final : public GonkNativeWindowNewFrameCallback
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WebrtcOMXDecoder)

private:
  virtual ~WebrtcOMXDecoder()
  {
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p OMX destructor", this);
    if (mStarted) {
      Stop();
    }
    if (mCodec != nullptr) {
      mCodec->release();
      mCodec.clear();
    }
    mLooper.clear();
  }

public:
  WebrtcOMXDecoder(const char* aMimeType,
                   webrtc::DecodedImageCallback* aCallback)
    : mWidth(0)
    , mHeight(0)
    , mStarted(false)
    , mCallback(aCallback)
    , mDecodedFrameLock("WebRTC decoded frame lock")
    , mEnding(false)
  {
    
    android::ProcessState::self()->startThreadPool();

    mLooper = new ALooper;
    mLooper->start();
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p creating decoder", this);
    mCodec = MediaCodec::CreateByType(mLooper, aMimeType, false );
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p OMX created", this);
  }

  
  static status_t ExtractPicDimensions(uint8_t* aData, size_t aSize,
                                       int32_t* aWidth, int32_t* aHeight)
  {
    MOZ_ASSERT(aData && aSize > sizeof(kNALStartCode));
    if ((aData[sizeof(kNALStartCode)] & 0x1f) != kNALTypeSPS) {
      return ERROR_MALFORMED;
    }
    sp<ABuffer> sps = new ABuffer(&aData[sizeof(kNALStartCode)], aSize - sizeof(kNALStartCode));
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
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
    sp<IGraphicBufferProducer> producer;
    sp<IGonkGraphicBufferConsumer> consumer;
    GonkBufferQueue::createBufferQueue(&producer, &consumer);
    mNativeWindow = new GonkNativeWindow(consumer);
#else
    mNativeWindow = new GonkNativeWindow();
#endif
    if (mNativeWindow.get()) {
      
      mNativeWindow->setNewFrameCallback(this);
      
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
      static_cast<GonkBufferQueueProducer*>(producer.get())->setSynchronousMode(false);
      
      consumer->setMaxAcquiredBufferCount(WEBRTC_OMX_H264_MIN_DECODE_BUFFERS);
      surface = new Surface(producer);
#else
      sp<GonkBufferQueue> bq = mNativeWindow->getBufferQueue();
      bq->setSynchronousMode(false);
      
      bq->setMaxAcquiredBufferCount(WEBRTC_OMX_H264_MIN_DECODE_BUFFERS);
      surface = new Surface(bq);
#endif
    }
    status_t result = mCodec->configure(config, surface, nullptr, 0);
    if (result == OK) {
      CODEC_LOGD("OMX:%p decoder configured", this);
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

    
    
    
    const uint8_t* data = aEncoded._buffer;
    size_t size = aEncoded._length;
    const uint8_t* nalStart = nullptr;
    size_t nalSize = 0;
    status_t err = OK;

    
    while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
      
      webrtc::EncodedImage nalu(aEncoded);

      nalu._buffer = const_cast<uint8_t*>(nalStart) - sizeof(kNALStartCode);
      MOZ_ASSERT(nalu._buffer >= aEncoded._buffer);
      nalu._length = nalSize + sizeof(kNALStartCode);
      MOZ_ASSERT(nalu._buffer + nalu._length <= aEncoded._buffer + aEncoded._length);

      size_t index;
      err = mCodec->dequeueInputBuffer(&index,
                                       aIsFirstFrame ? START_DEQUEUE_BUFFER_TIMEOUT_US : DEQUEUE_BUFFER_TIMEOUT_US);
      if (err != OK) {
        if (err != -EAGAIN) {
          CODEC_LOGE("decode dequeue input buffer error:%d", err);
        } else {
          CODEC_LOGE("decode dequeue 100ms without a buffer (EAGAIN)");
        }
        return err;
      }

      
      MOZ_ASSERT(memcmp(nalu._buffer, kNALStartCode, sizeof(kNALStartCode)) == 0);
      const sp<ABuffer>& omxIn = mInputBuffers.itemAt(index);
      MOZ_ASSERT(omxIn->capacity() >= nalu._length);
      omxIn->setRange(0, nalu._length);
      
      
      uint8_t* dst = omxIn->data();
      memcpy(dst, nalu._buffer, nalu._length);
      int64_t inputTimeUs = (nalu._timeStamp * 1000ll) / 90; 
      
      uint32_t flags;
      int nalType = dst[sizeof(kNALStartCode)] & 0x1f;
      switch (nalType) {
        case kNALTypeSPS:
        case kNALTypePPS:
          flags = MediaCodec::BUFFER_FLAG_CODECCONFIG;
          break;
        case kNALTypeIDR:
          flags = MediaCodec::BUFFER_FLAG_SYNCFRAME;
          break;
        default:
          flags = 0;
          break;
      }
      CODEC_LOGD("Decoder input: %d bytes (NAL 0x%02x), time %lld (%u), flags 0x%x",
                 nalu._length, dst[sizeof(kNALStartCode)], inputTimeUs, nalu._timeStamp, flags);
      err = mCodec->queueInputBuffer(index, 0, nalu._length, inputTimeUs, flags);
      if (err == OK && !(flags & MediaCodec::BUFFER_FLAG_CODECCONFIG)) {
        if (mOutputDrain == nullptr) {
          mOutputDrain = new OutputDrain(this);
          mOutputDrain->Start();
        }
        EncodedFrame frame;
        frame.mWidth = mWidth;
        frame.mHeight = mHeight;
        frame.mTimestamp = nalu._timeStamp;
        frame.mRenderTimeMs = aRenderTimeMs;
        mOutputDrain->QueueInput(frame);
      }
    }

    return err;
  }

  status_t
  DrainOutput(std::queue<EncodedFrame>& aInputFrames, Monitor& aMonitor)
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
        
        MonitorAutoLock lock(aMonitor);
        aInputFrames.pop();
        return OK;
    }

    CODEC_LOGD("Decoder output: %d bytes, offset %u, time %lld, flags 0x%x",
               outSize, outOffset, outTime, outFlags);
    if (mCallback) {
      EncodedFrame frame;
      {
        MonitorAutoLock lock(aMonitor);
        frame = aInputFrames.front();
        aInputFrames.pop();
      }
      {
        
        MutexAutoLock lock(mDecodedFrameLock);
        if (mEnding) {
          mCodec->releaseOutputBuffer(index);
          return err;
        }
        mDecodedFrames.push(frame);
      }
      
      
      mCodec->renderOutputBufferAndRelease(index);
      
      
    } else {
      mCodec->releaseOutputBuffer(index);
    }

    return err;
  }

  
  
  void OnNewFrame() override
  {
    RefPtr<layers::TextureClient> buffer = mNativeWindow->getCurrentBuffer();
    if (!buffer) {
      CODEC_LOGE("Decoder NewFrame: Get null buffer");
      return;
    }

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

    CODEC_LOGD("Decoder NewFrame: %dx%d, timestamp %lld, renderTimeMs %lld",
               grallocData.mPicSize.width, grallocData.mPicSize.height, timestamp, renderTimeMs);

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
    virtual bool DrainOutput() override
    {
      return (mOMX->DrainOutput(mInputFrames, mMonitor) == OK);
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

    {
      MutexAutoLock lock(mDecodedFrameLock);
      mEnding = false;
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

    CODEC_LOGD("OMXOutputDrain decoder stopping");
    
    {
      MutexAutoLock lock(mDecodedFrameLock);
      mEnding = true;
      while (!mDecodedFrames.empty()) {
        mDecodedFrames.pop();
      }
    }

    if (mOutputDrain != nullptr) {
      CODEC_LOGD("decoder's OutputDrain stopping");
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
    CODEC_LOGD("OMXOutputDrain decoder stopped");
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
  bool mEnding;
};

class EncOutputDrain : public OMXOutputDrain
{
public:
  EncOutputDrain(OMXVideoEncoder* aOMX, webrtc::EncodedImageCallback* aCallback)
    : OMXOutputDrain()
    , mOMX(aOMX)
    , mCallback(aCallback)
    , mIsPrevFrameParamSets(false)
  {}

protected:
  virtual bool DrainOutput() override
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
      
      CODEC_LOGD("OMX: (encode no output available this time)");
      return false;
    }

    
    uint32_t target_timestamp = (timeUs * 90ll + 999) / 1000; 
    
    
    
    
    bool isParamSets = IsParamSets(output.Elements(), output.Length());
    bool isIFrame = (flags & MediaCodec::BUFFER_FLAG_SYNCFRAME);
    CODEC_LOGD("OMX: encoded frame (%d): time %lld (%u), flags x%x",
               output.Length(), timeUs, target_timestamp, flags);
    
    
    

    if (mCallback) {
      
      
      
      
      webrtc::EncodedImage encoded(output.Elements(), output.Length(),
                                   output.Capacity());
      encoded._frameType = (isParamSets || isIFrame) ?
                           webrtc::kKeyFrame : webrtc::kDeltaFrame;
      EncodedFrame input_frame;
      {
        MonitorAutoLock lock(mMonitor);
        
        
        if (isParamSets) {
          
          
          input_frame = mInputFrames.front();
        } else {
          do {
            if (mInputFrames.empty()) {
              
              mInputFrames.push(input_frame);
              CODEC_LOGE("OMX: encoded timestamp %u which doesn't match input queue!! (head %u)",
                         target_timestamp, input_frame.mTimestamp);
              break;
            }

            input_frame = mInputFrames.front();
            mInputFrames.pop();
            if (input_frame.mTimestamp != target_timestamp) {
              CODEC_LOGD("OMX: encoder skipped frame timestamp %u", input_frame.mTimestamp);
            }
          } while (input_frame.mTimestamp != target_timestamp);
        }
      }

      encoded._encodedWidth = input_frame.mWidth;
      encoded._encodedHeight = input_frame.mHeight;
      encoded._timeStamp = input_frame.mTimestamp;
      encoded.capture_time_ms_ = input_frame.mRenderTimeMs;
      encoded._completeFrame = true;

      CODEC_LOGD("Encoded frame: %d bytes, %dx%d, is_param %d, is_iframe %d, timestamp %u, captureTimeMs %" PRIu64,
                 encoded._length, encoded._encodedWidth, encoded._encodedHeight,
                 isParamSets, isIFrame, encoded._timeStamp, encoded.capture_time_ms_);
      
      SendEncodedDataToCallback(encoded, isIFrame && !mIsPrevFrameParamSets && !isParamSets);
      
      
      
      mIsPrevFrameParamSets = isParamSets && !isIFrame;
      if (isParamSets) {
        
        mParamSets.Clear();
        
        size_t length = ParamSetLength(encoded._buffer, encoded._length);
        MOZ_ASSERT(length > 0);
        mParamSets.AppendElements(encoded._buffer, length);
      }
    }

    return !isParamSets; 
  }

private:
  
  
  
  void SendEncodedDataToCallback(webrtc::EncodedImage& aEncodedImage,
                                 bool aPrependParamSets)
  {
    if (aPrependParamSets) {
      webrtc::EncodedImage prepend(aEncodedImage);
      
      MOZ_ASSERT(mParamSets.Length() > sizeof(kNALStartCode)); 
      prepend._length = mParamSets.Length();
      prepend._buffer = mParamSets.Elements();
      
      CODEC_LOGD("Prepending SPS/PPS: %d bytes, timestamp %u, captureTimeMs %" PRIu64,
                 prepend._length, prepend._timeStamp, prepend.capture_time_ms_);
      SendEncodedDataToCallback(prepend, false);
    }

    struct nal_entry {
      uint32_t offset;
      uint32_t size;
    };
    nsAutoTArray<nal_entry, 1> nals;

    
    const uint8_t* data = aEncodedImage._buffer;
    size_t size = aEncodedImage._length;
    const uint8_t* nalStart = nullptr;
    size_t nalSize = 0;
    while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
      
      nal_entry nal = {((uint32_t) (nalStart - aEncodedImage._buffer)), (uint32_t) nalSize};
      nals.AppendElement(nal);
    }

    size_t num_nals = nals.Length();
    if (num_nals > 0) {
      webrtc::RTPFragmentationHeader fragmentation;
      fragmentation.VerifyAndAllocateFragmentationHeader(num_nals);
      for (size_t i = 0; i < num_nals; i++) {
        fragmentation.fragmentationOffset[i] = nals[i].offset;
        fragmentation.fragmentationLength[i] = nals[i].size;
      }
      webrtc::EncodedImage unit(aEncodedImage);
      unit._completeFrame = true;

      mCallback->Encoded(unit, nullptr, &fragmentation);
    }
  }

  OMXVideoEncoder* mOMX;
  webrtc::EncodedImageCallback* mCallback;
  bool mIsPrevFrameParamSets;
  nsTArray<uint8_t> mParamSets;
};


WebrtcOMXH264VideoEncoder::WebrtcOMXH264VideoEncoder()
  : mOMX(nullptr)
  , mCallback(nullptr)
  , mWidth(0)
  , mHeight(0)
  , mFrameRate(0)
  , mBitRateKbps(0)
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
  , mBitRateAtLastIDR(0)
#endif
  , mOMXConfigured(false)
  , mOMXReconfigure(false)
{
  mReservation = new OMXCodecReservation(true);
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
    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p OMX created", this);
  }

  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p Encoder in use", this);
    mOMX = nullptr;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  
  
  
  mWidth = aCodecSettings->width;
  mHeight = aCodecSettings->height;
  mFrameRate = aCodecSettings->maxFramerate;
  mBitRateKbps = aCodecSettings->startBitrate;
  

  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p OMX Encoder reserved", this);
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

  
  
  
  if (aInputImage.width() < 0 || (uint32_t)aInputImage.width() != mWidth ||
      aInputImage.height() < 0 || (uint32_t)aInputImage.height() != mHeight) {
    mWidth = aInputImage.width();
    mHeight = aInputImage.height();
    mOMXReconfigure = true;
  }

  if (!mOMXConfigured || mOMXReconfigure) {
    if (mOMXConfigured) {
      CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p reconfiguring encoder %dx%d @ %u fps",
                 this, mWidth, mHeight, mFrameRate);
      mOMXConfigured = false;
    }
    mOMXReconfigure = false;
    
    

    
    OMX_VIDEO_AVCLEVELTYPE level = OMX_VIDEO_AVCLevel3;
    
    OMX_VIDEO_CONTROLRATETYPE bitrateMode = OMX_Video_ControlRateConstantSkipFrames;

    
    sp<AMessage> format = new AMessage;
    
    format->setString("mime", MEDIA_MIMETYPE_VIDEO_AVC);
    
    
    

    
    
    format->setInt32("i-frame-interval", 4 );
    
    
    format->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);
    format->setInt32("profile", OMX_VIDEO_AVCProfileBaseline);
    format->setInt32("level", level);
    format->setInt32("bitrate-mode", bitrateMode);
    format->setInt32("store-metadata-in-buffers", 0);
    
    format->setInt32("prepend-sps-pps-to-idr-frames", 1);
    
    format->setInt32("width", mWidth);
    format->setInt32("height", mHeight);
    format->setInt32("stride", mWidth);
    format->setInt32("slice-height", mHeight);
    format->setInt32("frame-rate", mFrameRate);
    format->setInt32("bitrate", mBitRateKbps*1000);

    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p configuring encoder %dx%d @ %d fps, rate %d kbps",
               this, mWidth, mHeight, mFrameRate, mBitRateKbps);
    nsresult rv = mOMX->ConfigureDirect(format,
                                        OMXVideoEncoder::BlobFormat::AVC_NAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      CODEC_LOGE("WebrtcOMXH264VideoEncoder:%p FAILED configuring encoder %d", this, int(rv));
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    mOMXConfigured = true;
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
    mLastIDRTime = TimeStamp::Now();
    mBitRateAtLastIDR = mBitRateKbps;
#endif
  }

  if (aFrameTypes && aFrameTypes->size() &&
      ((*aFrameTypes)[0] == webrtc::kKeyFrame)) {
    mOMX->RequestIDRFrame();
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
    mLastIDRTime = TimeStamp::Now();
    mBitRateAtLastIDR = mBitRateKbps;
  } else if (mBitRateKbps != mBitRateAtLastIDR) {
    
    TimeStamp now = TimeStamp::Now();
    if (mLastIDRTime.IsNull()) {
      
      mLastIDRTime = now;
    }
    int32_t timeSinceLastIDR = (now - mLastIDRTime).ToMilliseconds();

    

    
    
    
    if ((timeSinceLastIDR > 3000) ||
        (mBitRateKbps < (mBitRateAtLastIDR * 8)/10) ||
        (timeSinceLastIDR < 300 && mBitRateKbps < (mBitRateAtLastIDR * 9)/10) ||
        (timeSinceLastIDR < 1000 && mBitRateKbps < (mBitRateAtLastIDR * 97)/100) ||
        (timeSinceLastIDR >= 1000 && mBitRateKbps < mBitRateAtLastIDR) ||
        (mBitRateKbps > (mBitRateAtLastIDR * 15)/10) ||
        (timeSinceLastIDR < 500 && mBitRateKbps > (mBitRateAtLastIDR * 13)/10) ||
        (timeSinceLastIDR < 1000 && mBitRateKbps > (mBitRateAtLastIDR * 11)/10) ||
        (timeSinceLastIDR >= 1000 && mBitRateKbps > mBitRateAtLastIDR)) {
      CODEC_LOGD("Requesting IDR for bitrate change from %u to %u (time since last idr %dms)",
                 mBitRateAtLastIDR, mBitRateKbps, timeSinceLastIDR);

      mOMX->RequestIDRFrame();
      mLastIDRTime = now;
      mBitRateAtLastIDR = mBitRateKbps;
    }
#endif
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

  CODEC_LOGD("Encode frame: %dx%d, timestamp %u (%lld), renderTimeMs %" PRIu64,
             aInputImage.width(), aInputImage.height(),
             aInputImage.timestamp(), aInputImage.timestamp() * 1000ll / 90,
             aInputImage.render_time_ms());

  nsresult rv = mOMX->Encode(&img,
                             yuvData.mYSize.width,
                             yuvData.mYSize.height,
                             aInputImage.timestamp() * 1000ll / 90, 
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
  mOMXConfigured = false;
  bool hadOMX = !!mOMX;
  mOMX = nullptr;
  if (hadOMX) {
    mReservation->ReleaseOMXCodec();
  }
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p released", this);

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
WebrtcOMXH264VideoEncoder::SetRates(uint32_t aBitRateKbps, uint32_t aFrameRate)
{
  CODEC_LOGE("WebrtcOMXH264VideoEncoder:%p set bitrate:%u, frame rate:%u (%u))",
             this, aBitRateKbps, aFrameRate, mFrameRate);
  MOZ_ASSERT(mOMX != nullptr);
  if (mOMX == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  

  
  
  
  
  
  
#if !defined(TEST_OMX_FRAMERATE_CHANGES)
  if (aFrameRate > mFrameRate ||
      aFrameRate < mFrameRate/2) {
    uint32_t old_rate = mFrameRate;
    if (aFrameRate >= 15) {
      mFrameRate = 30;
    } else if (aFrameRate >= 10) {
      mFrameRate = 20;
    } else if (aFrameRate >= 8) {
      mFrameRate = 15;
    } else  {
      
      mFrameRate = 10;
    }
    if (mFrameRate < aFrameRate) { 
      mFrameRate = aFrameRate;
    }
    if (old_rate != mFrameRate) {
      mOMXReconfigure = true;  
    }
  }
#else
  
  if (aFrameRate != mFrameRate) {
    mFrameRate = aFrameRate;
    mOMXReconfigure = true;  
  }
#endif

  
  
  
  if (aBitRateKbps > 700) {
    aBitRateKbps = 700;
  }
  mBitRateKbps = aBitRateKbps;
  nsresult rv = mOMX->SetBitrate(mBitRateKbps);
  NS_WARN_IF(NS_FAILED(rv));
  return NS_FAILED(rv) ? WEBRTC_VIDEO_CODEC_OK : WEBRTC_VIDEO_CODEC_ERROR;
}


WebrtcOMXH264VideoDecoder::WebrtcOMXH264VideoDecoder()
  : mCallback(nullptr)
  , mOMX(nullptr)
{
  mReservation = new OMXCodecReservation(false);
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be constructed", this);
}

int32_t
WebrtcOMXH264VideoDecoder::InitDecode(const webrtc::VideoCodec* aCodecSettings,
                                      int32_t aNumOfCores)
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p init OMX:%p", this, mOMX.get());

  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p Decoder in use", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  
  

  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p OMX Decoder reserved", this);
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
      
      CODEC_LOGI("WebrtcOMXH264VideoDecoder:%p missing SPS in input (nal 0x%02x, len %d)",
                 this, aInputImage._buffer[sizeof(kNALStartCode)] & 0x1f, aInputImage._length);
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
  mReservation->ReleaseOMXCodec();

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
