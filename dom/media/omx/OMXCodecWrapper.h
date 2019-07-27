




#ifndef OMXCodecWrapper_h_
#define OMXCodecWrapper_h_

#include <gui/Surface.h>
#include <utils/RefBase.h>
#include <stagefright/foundation/ABuffer.h>
#include <stagefright/foundation/AMessage.h>
#include <stagefright/MediaCodec.h>

#include "AudioSegment.h"
#include "GonkNativeWindow.h"
#include "GonkNativeWindowClient.h"
#include "mozilla/media/MediaSystemResourceClient.h"
#include "nsRefPtr.h"

#include <speex/speex_resampler.h>

namespace android {


class OMXCodecReservation : public RefBase
{
public:
  OMXCodecReservation(bool aEncoder)
  {
    mType = aEncoder ? mozilla::MediaSystemResourceType::VIDEO_ENCODER :
            mozilla::MediaSystemResourceType::VIDEO_DECODER;
  }

  virtual ~OMXCodecReservation()
  {
    ReleaseOMXCodec();
  }

  
  virtual bool ReserveOMXCodec();

  
  virtual void ReleaseOMXCodec();

private:
  mozilla::MediaSystemResourceType mType;

  nsRefPtr<mozilla::MediaSystemResourceClient> mClient;
};


class OMXAudioEncoder;
class OMXVideoEncoder;




























class OMXCodecWrapper
{
public:
  
  enum CodecType {
    AAC_ENC, 
    AMR_NB_ENC, 
    AVC_ENC, 
    TYPE_COUNT
  };

  
  enum {
    
    
    
    BUFFER_EOS = MediaCodec::BUFFER_FLAG_EOS,
    
    BUFFER_SYNC_FRAME = MediaCodec::BUFFER_FLAG_SYNCFRAME,
    
    
    
    BUFFER_CODEC_CONFIG = MediaCodec::BUFFER_FLAG_CODECCONFIG,
  };

  
  
  
  enum {
    kAACBitrate = 96000,      
    kAACFrameSize = 768,      
    kAACFrameDuration = 1024, 
  };

  
  static OMXAudioEncoder* CreateAACEncoder();

  
  static OMXAudioEncoder* CreateAMRNBEncoder();

  
  static OMXVideoEncoder* CreateAVCEncoder();

  virtual ~OMXCodecWrapper();

  





  nsresult GetNextEncodedFrame(nsTArray<uint8_t>* aOutputBuf,
                               int64_t* aOutputTimestamp, int* aOutputFlags,
                               int64_t aTimeOut);
  


  int GetCodecType() { return mCodecType; }
protected:
  



  virtual bool IsValid() { return mCodec != nullptr; }

  





  virtual status_t AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                       ABuffer* aData) = 0;

  




  virtual void AppendFrame(nsTArray<uint8_t>* aOutputBuf,
                           const uint8_t* aData, size_t aSize)
  {
    aOutputBuf->AppendElements(aData, aSize);
  }

private:
  
  OMXCodecWrapper() = delete;
  OMXCodecWrapper(const OMXCodecWrapper&) = delete;
  OMXCodecWrapper& operator=(const OMXCodecWrapper&) = delete;

  




  OMXCodecWrapper(CodecType aCodecType);

  
  friend class OMXAudioEncoder;
  friend class OMXVideoEncoder;

  


  status_t Start();

  


  status_t Stop();

  
  sp<MediaCodec> mCodec;

  
  sp<ALooper> mLooper;

  Vector<sp<ABuffer> > mInputBufs;  
  Vector<sp<ABuffer> > mOutputBufs; 

  int mCodecType;
  bool mStarted; 
  bool mAMRCSDProvided;
};




class OMXAudioEncoder final : public OMXCodecWrapper
{
public:
  




  nsresult Configure(int aChannelCount, int aInputSampleRate, int aEncodedSampleRate);

  





  nsresult Encode(mozilla::AudioSegment& aSegment, int aInputFlags = 0);

  ~OMXAudioEncoder();
protected:
  virtual status_t AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                       ABuffer* aData) override;
private:
  
  OMXAudioEncoder() = delete;
  OMXAudioEncoder(const OMXAudioEncoder&) = delete;
  OMXAudioEncoder& operator=(const OMXAudioEncoder&) = delete;

  



  OMXAudioEncoder(CodecType aCodecType)
    : OMXCodecWrapper(aCodecType)
    , mResampler(nullptr)
    , mChannels(0)
    , mResamplingRatio(0)
    , mTimestamp(0)
    , mSampleDuration(0) {}

  
  friend class OMXCodecWrapper;
  friend class InputBufferHelper;

  



  SpeexResamplerState* mResampler;
  
  size_t mChannels;

  float mResamplingRatio;
  
  int64_t mTimestamp;
  
  int64_t mSampleDuration;
};




class OMXVideoEncoder final : public OMXCodecWrapper
{
public:
  
  enum BlobFormat {
    AVC_MP4, 
    AVC_NAL  
  };

  







  nsresult Configure(int aWidth, int aHeight, int aFrameRate,
                     BlobFormat aBlobFormat = BlobFormat::AVC_MP4);
  nsresult ConfigureDirect(sp<AMessage>& aFormat,
                           BlobFormat aBlobFormat = BlobFormat::AVC_MP4);

  





  nsresult Encode(const mozilla::layers::Image* aImage, int aWidth, int aHeight,
                  int64_t aTimestamp, int aInputFlags = 0);

#if ANDROID_VERSION >= 18
  
  nsresult SetBitrate(int32_t aKbps);
#endif

  



  nsresult RequestIDRFrame();

protected:
  virtual status_t AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                       ABuffer* aData) override;

  
  
  
  virtual void AppendFrame(nsTArray<uint8_t>* aOutputBuf,
                           const uint8_t* aData, size_t aSize) override;

private:
  
  OMXVideoEncoder() = delete;
  OMXVideoEncoder(const OMXVideoEncoder&) = delete;
  OMXVideoEncoder& operator=(const OMXVideoEncoder&) = delete;

  



  OMXVideoEncoder(CodecType aCodecType)
    : OMXCodecWrapper(aCodecType)
    , mWidth(0)
    , mHeight(0)
    , mBlobFormat(BlobFormat::AVC_MP4)
  {}

  
  friend class OMXCodecWrapper;

  int mWidth;
  int mHeight;
  BlobFormat mBlobFormat;
};

} 
#endif
