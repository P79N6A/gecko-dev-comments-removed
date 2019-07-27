




#ifndef WebMWriter_h_
#define WebMWriter_h_

#include "ContainerWriter.h"

namespace mozilla {

class EbmlComposer;


class VorbisMetadata : public TrackMetadataBase
{
public:
  nsTArray<uint8_t> mData;
  int32_t mChannels;
  int32_t mBitDepth;
  float mSamplingFrequency;
  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_VORBIS; }
};


class VP8Metadata : public TrackMetadataBase
{
public:
  int32_t mWidth;
  int32_t mHeight;
  int32_t mDisplayWidth;
  int32_t mDisplayHeight;
  int32_t mEncodedFrameRate;
  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_VP8; }
};







class WebMWriter : public ContainerWriter
{
public:
  
  
  explicit WebMWriter(uint32_t aTrackTypes);
  virtual ~WebMWriter();

  
  nsresult WriteEncodedTrack(const EncodedFrameContainer &aData,
                             uint32_t aFlags = 0) MOZ_OVERRIDE;

  
  
  
  nsresult GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                            uint32_t aFlags = 0) MOZ_OVERRIDE;

  
  nsresult SetMetadata(TrackMetadataBase* aMetadata) MOZ_OVERRIDE;

private:
  nsAutoPtr<EbmlComposer> mEbmlComposer;

  
  
  uint8_t mMetadataRequiredFlag;
};

}
#endif
