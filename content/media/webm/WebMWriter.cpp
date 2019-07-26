




#include "WebMWriter.h"
#include "EbmlComposer.h"

namespace mozilla {

WebMWriter::WebMWriter(uint32_t aTrackTypes) : ContainerWriter()
{
  mMetadataRequiredFlag = aTrackTypes;
  mEbmlComposer = new EbmlComposer();
}

WebMWriter::~WebMWriter()
{
  
}

nsresult
WebMWriter::WriteEncodedTrack(const EncodedFrameContainer& aData,
                              uint32_t aFlags)
{
  for (uint32_t i = 0 ; i < aData.GetEncodedFrames().Length(); i++) {
    mEbmlComposer->WriteSimpleBlock(aData.GetEncodedFrames().ElementAt(i).get());
  }
  return NS_OK;
}

nsresult
WebMWriter::GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                             uint32_t aFlags)
{
  mEbmlComposer->ExtractBuffer(aOutputBufs, aFlags);
  if (aFlags & ContainerWriter::FLUSH_NEEDED) {
    mIsWritingComplete = true;
  }
  return NS_OK;
}

nsresult
WebMWriter::SetMetadata(TrackMetadataBase* aMetadata)
{
  MOZ_ASSERT(aMetadata);
  if (aMetadata->GetKind() == TrackMetadataBase::METADATA_VP8) {
    VP8Metadata* meta = static_cast<VP8Metadata*>(aMetadata);
    MOZ_ASSERT(meta, "Cannot find vp8 encoder metadata");
    mEbmlComposer->SetVideoConfig(meta->mWidth, meta->mHeight,
                                  meta->mDisplayWidth, meta->mDisplayHeight,
                                  meta->mEncodedFrameRate);
    mMetadataRequiredFlag = mMetadataRequiredFlag & ~ContainerWriter::CREATE_VIDEO_TRACK;
  }

  if (aMetadata->GetKind() == TrackMetadataBase::METADATA_VORBIS) {
    VorbisMetadata* meta = static_cast<VorbisMetadata*>(aMetadata);
    MOZ_ASSERT(meta, "Cannot find vorbis encoder metadata");
    mEbmlComposer->SetAudioConfig(meta->mSamplingFrequency, meta->mChannels, meta->mBitDepth);
    mEbmlComposer->SetAudioCodecPrivateData(meta->mData);
    mMetadataRequiredFlag = mMetadataRequiredFlag & ~ContainerWriter::CREATE_AUDIO_TRACK;
  }

  if (!mMetadataRequiredFlag) {
    mEbmlComposer->GenerateHeader();
  }
  return NS_OK;
}

} 
