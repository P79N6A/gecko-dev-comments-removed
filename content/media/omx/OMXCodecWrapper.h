




#ifndef OMXCodecWrapper_h_
#define OMXCodecWrapper_h_

#include <gui/Surface.h>
#include <stagefright/foundation/ABuffer.h>
#include <stagefright/foundation/AMessage.h>
#include <stagefright/MediaCodec.h>

#include "AudioSegment.h"
#include "GonkNativeWindow.h"
#include "GonkNativeWindowClient.h"

namespace android {

class OMXAudioEncoder;
class OMXVideoEncoder;




























class OMXCodecWrapper
{
public:
  
  enum CodecType {
    AAC_ENC, 
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

  
  static OMXVideoEncoder* CreateAVCEncoder();

  virtual ~OMXCodecWrapper();

  





  nsresult GetNextEncodedFrame(nsTArray<uint8_t>* aOutputBuf,
                               int64_t* aOutputTimestamp, int* aOutputFlags,
                               int64_t aTimeOut);

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

  bool mStarted; 
};




class OMXAudioEncoder MOZ_FINAL : public OMXCodecWrapper
{
public:
  



  nsresult Configure(int aChannelCount, int aSampleRate);

  





  nsresult Encode(mozilla::AudioSegment& aSegment, int aInputFlags = 0);

protected:
  virtual status_t AppendDecoderConfig(nsTArray<uint8_t>* aOutputBuf,
                                       ABuffer* aData) MOZ_OVERRIDE;

private:
  
  OMXAudioEncoder() MOZ_DELETE;
  OMXAudioEncoder(const OMXAudioEncoder&) MOZ_DELETE;
  OMXAudioEncoder& operator=(const OMXAudioEncoder&) MOZ_DELETE;

  



  OMXAudioEncoder(CodecType aCodecType)
    : OMXCodecWrapper(aCodecType)
    , mChannels(0)
    , mTimestamp(0)
    , mSampleDuration(0) {}

  
  friend class OMXCodecWrapper;

  
  size_t mChannels;
  
  int64_t mTimestamp;
  
  int64_t mSampleDuration;
};




class OMXVideoEncoder MOZ_FINAL : public OMXCodecWrapper
{
public:
  



  nsresult Configure(int aWidth, int aHeight, int aFrameRate);

  





  nsresult Encode(const mozilla::layers::Image* aImage, int aWidth, int aHeight,
                  int64_t aTimestamp, int aInputFlags = 0);

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
    : OMXCodecWrapper(aCodecType), mWidth(0), mHeight(0) {}

  
  friend class OMXCodecWrapper;

  int mWidth;
  int mHeight;
};

} 
#endif
