




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

#include "IMediaResourceManagerService.h"
#include "MediaResourceManagerClient.h"

#include <speex/speex_resampler.h>

namespace android {


class OMXCodecReservation : public MediaResourceManagerClient::EventListener
{
public:
  OMXCodecReservation(bool aEncoder)
  {
    mType = aEncoder ? IMediaResourceManagerService::HW_VIDEO_ENCODER :
            IMediaResourceManagerService::HW_VIDEO_DECODER;
  }

  virtual ~OMXCodecReservation()
  {
    ReleaseOMXCodec();
  }

  
  virtual bool ReserveOMXCodec();

  
  virtual void ReleaseOMXCodec();

  
  virtual void statusChanged(int event) {}

private:
  IMediaResourceManagerService::ResourceType mType;

  sp<MediaResourceManagerClient> mClient;
  sp<IMediaResourceManagerService> mManagerService;
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
  
  OMXCodecWrapper() MOZ_DELETE;
  OMXCodecWrapper(const OMXCodecWrapper&) MOZ_DELETE;
  OMXCodecWrapper& operator=(const OMXCodecWrapper&) MOZ_DELETE;

  




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




class OMXAudioEncoder MOZ_FINAL : public OMXCodecWrapper
{
public:
  




  nsresult Configure(int aChannelCount, int aInputSampleRate, int aEncodedSampleRate);

  





  nsresult Encode(mozilla::AudioSegment& aSegment, int aInputFlags = 0);

  ~OMXAudioEncoder();
protected:
  virtual status_t AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                       ABuffer* aData) MOZ_OVERRIDE;
private:
  
  OMXAudioEncoder() MOZ_DELETE;
  OMXAudioEncoder(const OMXAudioEncoder&) MOZ_DELETE;
  OMXAudioEncoder& operator=(const OMXAudioEncoder&) MOZ_DELETE;

  



  OMXAudioEncoder(CodecType aCodecType)
    : OMXCodecWrapper(aCodecType)
    , mResampler(nullptr)
    , mChannels(0)
    , mTimestamp(0)
    , mSampleDuration(0)
    , mResamplingRatio(0) {}

  
  friend class OMXCodecWrapper;

  



  SpeexResamplerState* mResampler;
  
  size_t mChannels;

  float mResamplingRatio;
  
  int64_t mTimestamp;
  
  int64_t mSampleDuration;
};




class OMXVideoEncoder MOZ_FINAL : public OMXCodecWrapper
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(OMXVideoEncoder)
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
                                       ABuffer* aData) MOZ_OVERRIDE;

  
  
  
  virtual void AppendFrame(nsTArray<uint8_t>* aOutputBuf,
                           const uint8_t* aData, size_t aSize) MOZ_OVERRIDE;

private:
  
  OMXVideoEncoder() MOZ_DELETE;
  OMXVideoEncoder(const OMXVideoEncoder&) MOZ_DELETE;
  OMXVideoEncoder& operator=(const OMXVideoEncoder&) MOZ_DELETE;

  



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
