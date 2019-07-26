




#ifndef ISOTrackMetadata_h_
#define ISOTrackMetadata_h_

#include "TrackMetadataBase.h"

namespace mozilla {




#define Audio_Track 0x01
#define Video_Track 0x02

class AACTrackMetadata : public TrackMetadataBase {
public:
  uint32_t SampleRate;     
  uint32_t FrameDuration;  
  uint32_t FrameSize;      
  uint32_t Channels;       

  AACTrackMetadata()
    : SampleRate(0)
    , FrameDuration(0)
    , FrameSize(0)
    , Channels(0) {
    MOZ_COUNT_CTOR(AACTrackMetadata);
  }
  ~AACTrackMetadata() { MOZ_COUNT_DTOR(AACTrackMetadata); }
  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_AAC; }
};

class AVCTrackMetadata : public TrackMetadataBase {
public:
  uint32_t Height;
  uint32_t Width;
  uint32_t VideoFrequency;  
  uint32_t FrameRate;       

  AVCTrackMetadata()
    : Height(0)
    , Width(0)
    , VideoFrequency(0)
    , FrameRate(0) {
    MOZ_COUNT_CTOR(AVCTrackMetadata);
  }
  ~AVCTrackMetadata() { MOZ_COUNT_DTOR(AVCTrackMetadata); }
  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_AVC; }
};

}

#endif 
