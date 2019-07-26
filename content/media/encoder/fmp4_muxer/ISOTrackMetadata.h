




#ifndef ISOTrackMetadata_h_
#define ISOTrackMetadata_h_

#include "TrackMetadataBase.h"

namespace mozilla {

class AACTrackMetadata : public AudioTrackMetadata {
public:
  
  uint32_t GetAudioFrameDuration() MOZ_OVERRIDE { return mFrameDuration; }
  uint32_t GetAudioFrameSize() MOZ_OVERRIDE { return mFrameSize; }
  uint32_t GetAudioSampleRate() MOZ_OVERRIDE { return mSampleRate; }
  uint32_t GetAudioChannels() MOZ_OVERRIDE { return mChannels; }

  
  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_AAC; }

  
  AACTrackMetadata()
    : mSampleRate(0)
    , mFrameDuration(0)
    , mFrameSize(0)
    , mChannels(0) {
    MOZ_COUNT_CTOR(AACTrackMetadata);
  }
  ~AACTrackMetadata() { MOZ_COUNT_DTOR(AACTrackMetadata); }

  uint32_t mSampleRate;     
  uint32_t mFrameDuration;  
  uint32_t mFrameSize;      
  uint32_t mChannels;       
};


#define AVC_CLOCK_RATE 90000

class AVCTrackMetadata : public VideoTrackMetadata {
public:
  
  uint32_t GetVideoHeight() MOZ_OVERRIDE { return mHeight; }
  uint32_t GetVideoWidth() MOZ_OVERRIDE {return mWidth; }
  uint32_t GetVideoClockRate() MOZ_OVERRIDE { return AVC_CLOCK_RATE; }
  uint32_t GetVideoFrameRate() MOZ_OVERRIDE { return mFrameRate; }

  
  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_AVC; }

  
  AVCTrackMetadata()
    : mHeight(0)
    , mWidth(0)
    , mFrameRate(0) {
    MOZ_COUNT_CTOR(AVCTrackMetadata);
  }
  ~AVCTrackMetadata() { MOZ_COUNT_DTOR(AVCTrackMetadata); }

  uint32_t mHeight;
  uint32_t mWidth;
  uint32_t mFrameRate;       
};

}

#endif 
