




#ifndef TrackMetadataBase_h_
#define TrackMetadataBase_h_

#include "nsTArray.h"
#include "nsCOMPtr.h"
namespace mozilla {


class TrackMetadataBase
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TrackMetadataBase)
  enum MetadataKind {
    METADATA_OPUS,    
    METADATA_VP8,
    METADATA_VORBIS,
    METADATA_AVC,
    METADATA_AAC,
    METADATA_AMR,
    METADATA_UNKNOWN  
  };
  
  virtual MetadataKind GetKind() const = 0;

protected:
  
  virtual ~TrackMetadataBase() {}
};


class AudioTrackMetadata : public TrackMetadataBase {
public:
  
  
  virtual uint32_t GetAudioFrameDuration() = 0;

  
  
  virtual uint32_t GetAudioFrameSize() = 0;

  
  virtual uint32_t GetAudioSampleRate() = 0;

  virtual uint32_t GetAudioChannels() = 0;
};


class VideoTrackMetadata : public TrackMetadataBase {
public:
  
  virtual uint32_t GetVideoHeight() = 0;
  virtual uint32_t GetVideoWidth() = 0;

  
  virtual uint32_t GetVideoDisplayHeight() = 0;
  virtual uint32_t GetVideoDisplayWidth() = 0;

  
  
  
  
  virtual uint32_t GetVideoClockRate() = 0;

  
  virtual uint32_t GetVideoFrameRate() = 0;
};

}
#endif
