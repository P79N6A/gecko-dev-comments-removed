




#include <time.h>
#include "nsAutoPtr.h"
#include "ISOControl.h"
#include "ISOMediaBoxes.h"
#include "EncodedFrameContainer.h"

namespace mozilla {



#define iso_time_offset 2082844800

FragmentBuffer::FragmentBuffer(uint32_t aTrackType, uint32_t aFragDuration)
  : mTrackType(aTrackType)
  , mFragDuration(aFragDuration)
  , mMediaStartTime(0)
  , mFragmentNumber(0)
  , mLastFrameTimeOfLastFragment(0)
  , mEOS(false)
{
  mFragArray.AppendElement();
  MOZ_COUNT_CTOR(FragmentBuffer);
}

FragmentBuffer::~FragmentBuffer()
{
  MOZ_COUNT_DTOR(FragmentBuffer);
}

bool
FragmentBuffer::HasEnoughData()
{
  
  return (mFragArray.Length() > 1);
}

nsresult
FragmentBuffer::GetCSD(nsTArray<uint8_t>& aCSD)
{
  if (!mCSDFrame) {
    return NS_ERROR_FAILURE;
  }
  aCSD.AppendElements(mCSDFrame->GetFrameData().Elements(),
                      mCSDFrame->GetFrameData().Length());

  return NS_OK;
}

nsresult
FragmentBuffer::AddFrame(EncodedFrame* aFrame)
{
  
  if (mEOS) {
    MOZ_ASSERT(0);
    return NS_OK;
  }

  EncodedFrame::FrameType type = aFrame->GetFrameType();
  if (type == EncodedFrame::AAC_CSD || type == EncodedFrame::AVC_CSD ||
      type == EncodedFrame::AMR_AUDIO_CSD) {
    mCSDFrame = aFrame;
    
    
    mMediaStartTime = aFrame->GetTimeStamp();
    mFragmentNumber = 1;
    return NS_OK;
  }

  
  if (aFrame->GetTimeStamp() < mMediaStartTime) {
    MOZ_ASSERT(false);
    return NS_ERROR_FAILURE;
  }

  mFragArray.LastElement().AppendElement(aFrame);

  
  if ((aFrame->GetTimeStamp() - mMediaStartTime) >= (mFragDuration * mFragmentNumber)) {
    mFragArray.AppendElement();
    mFragmentNumber++;
  }

  return NS_OK;
}

nsresult
FragmentBuffer::GetFirstFragment(nsTArray<nsRefPtr<EncodedFrame>>& aFragment,
                                 bool aFlush)
{
  
  if (mFragArray.Length() <= 1 && !mEOS) {
    MOZ_ASSERT(false);
    return NS_ERROR_FAILURE;
  }

  if (aFlush) {
    aFragment.SwapElements(mFragArray.ElementAt(0));
    mFragArray.RemoveElementAt(0);
  } else {
    aFragment.AppendElements(mFragArray.ElementAt(0));
  }
  return NS_OK;
}

uint32_t
FragmentBuffer::GetFirstFragmentSampleNumber()
{
  return mFragArray.ElementAt(0).Length();
}

uint32_t
FragmentBuffer::GetFirstFragmentSampleSize()
{
  uint32_t size = 0;
  uint32_t len = mFragArray.ElementAt(0).Length();
  for (uint32_t i = 0; i < len; i++) {
    size += mFragArray.ElementAt(0).ElementAt(i)->GetFrameData().Length();
  }
  return size;
}

ISOControl::ISOControl(uint32_t aMuxingType)
  : mMuxingType(aMuxingType)
  , mAudioFragmentBuffer(nullptr)
  , mVideoFragmentBuffer(nullptr)
  , mFragNum(0)
  , mOutputSize(0)
  , mBitCount(0)
  , mBit(0)
{
  
  mOutBuffers.SetLength(1);
  MOZ_COUNT_CTOR(ISOControl);
}

ISOControl::~ISOControl()
{
  MOZ_COUNT_DTOR(ISOControl);
}

uint32_t
ISOControl::GetNextTrackID()
{
  return (mMetaArray.Length() + 1);
}

uint32_t
ISOControl::GetTrackID(TrackMetadataBase::MetadataKind aKind)
{
  for (uint32_t i = 0; i < mMetaArray.Length(); i++) {
    if (mMetaArray[i]->GetKind() == aKind) {
      return (i + 1);
    }
  }

  
  MOZ_ASSERT(0);
  return 0;
}

nsresult
ISOControl::SetMetadata(TrackMetadataBase* aTrackMeta)
{
  if (aTrackMeta->GetKind() == TrackMetadataBase::METADATA_AAC ||
      aTrackMeta->GetKind() == TrackMetadataBase::METADATA_AMR ||
      aTrackMeta->GetKind() == TrackMetadataBase::METADATA_AVC) {
    mMetaArray.AppendElement(aTrackMeta);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
ISOControl::GetAudioMetadata(nsRefPtr<AudioTrackMetadata>& aAudMeta)
{
  for (uint32_t i = 0; i < mMetaArray.Length() ; i++) {
    if (mMetaArray[i]->GetKind() == TrackMetadataBase::METADATA_AAC ||
        mMetaArray[i]->GetKind() == TrackMetadataBase::METADATA_AMR) {
      aAudMeta = static_cast<AudioTrackMetadata*>(mMetaArray[i].get());
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult
ISOControl::GetVideoMetadata(nsRefPtr<VideoTrackMetadata>& aVidMeta)
{
  for (uint32_t i = 0; i < mMetaArray.Length() ; i++) {
    if (mMetaArray[i]->GetKind() == TrackMetadataBase::METADATA_AVC) {
      aVidMeta = static_cast<VideoTrackMetadata*>(mMetaArray[i].get());
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

bool
ISOControl::HasAudioTrack()
{
  nsRefPtr<AudioTrackMetadata> audMeta;
  GetAudioMetadata(audMeta);
  return audMeta;
}

bool
ISOControl::HasVideoTrack()
{
  nsRefPtr<VideoTrackMetadata> vidMeta;
  GetVideoMetadata(vidMeta);
  return vidMeta;
}

nsresult
ISOControl::SetFragment(FragmentBuffer* aFragment)
{
  if (aFragment->GetType() == Audio_Track) {
    mAudioFragmentBuffer = aFragment;
  } else {
    mVideoFragmentBuffer = aFragment;
  }
  return NS_OK;
}

FragmentBuffer*
ISOControl::GetFragment(uint32_t aType)
{
  if (aType == Audio_Track) {
    return mAudioFragmentBuffer;
  } else if (aType == Video_Track){
    return mVideoFragmentBuffer;
  }
  MOZ_ASSERT(0);
  return nullptr;
}

nsresult
ISOControl::GetBufs(nsTArray<nsTArray<uint8_t>>* aOutputBufs)
{
  uint32_t len = mOutBuffers.Length();
  for (uint32_t i = 0; i < len; i++) {
    mOutBuffers[i].SwapElements(*aOutputBufs->AppendElement());
  }
  return FlushBuf();
}

nsresult
ISOControl::FlushBuf()
{
  mOutBuffers.SetLength(1);
  return NS_OK;
}

uint32_t
ISOControl::WriteAVData(nsTArray<uint8_t>& aArray)
{
  MOZ_ASSERT(!mBitCount);

  uint32_t len = aArray.Length();
  if (!len) {
    return 0;
  }

  mOutputSize += len;

  
  
  if (mOutBuffers.LastElement().Length()) {
    mOutBuffers.AppendElement();
  }
  
  mOutBuffers.LastElement().SwapElements(aArray);
  
  mOutBuffers.AppendElement();

  return len;
}

uint32_t
ISOControl::WriteBits(uint64_t aBits, size_t aNumBits)
{
  uint8_t output_byte = 0;

  MOZ_ASSERT(aNumBits <= 64);
  
  for (size_t i = aNumBits; i > 0; i--) {
    mBit |= (((aBits >> (i - 1)) & 1) << (8 - ++mBitCount));
    if (mBitCount == 8) {
      Write(&mBit, sizeof(uint8_t));
      mBit = 0;
      mBitCount = 0;
      output_byte++;
    }
  }
  return output_byte;
}

uint32_t
ISOControl::Write(uint8_t* aBuf, uint32_t aSize)
{
  mOutBuffers.LastElement().AppendElements(aBuf, aSize);
  mOutputSize += aSize;
  return aSize;
}

uint32_t
ISOControl::Write(uint8_t aData)
{
  MOZ_ASSERT(!mBitCount);
  Write((uint8_t*)&aData, sizeof(uint8_t));
  return sizeof(uint8_t);
}

uint32_t
ISOControl::GetBufPos()
{
  uint32_t len = mOutBuffers.Length();
  uint32_t pos = 0;
  for (uint32_t i = 0; i < len; i++) {
    pos += mOutBuffers.ElementAt(i).Length();
  }
  return pos;
}

uint32_t
ISOControl::WriteFourCC(const char* aType)
{
  
  MOZ_ASSERT(!mBitCount);

  uint32_t size = strlen(aType);
  if (size == 4) {
    return Write((uint8_t*)aType, size);
  }

  return 0;
}

nsresult
ISOControl::GenerateFtyp()
{
  nsresult rv;
  uint32_t size;
  nsAutoPtr<FileTypeBox> type_box(new FileTypeBox(this));
  rv = type_box->Generate(&size);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = type_box->Write();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
ISOControl::GenerateMoov()
{
  nsresult rv;
  uint32_t size;
  nsAutoPtr<MovieBox> moov_box(new MovieBox(this));
  rv = moov_box->Generate(&size);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moov_box->Write();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
ISOControl::GenerateMoof(uint32_t aTrackType)
{
  mFragNum++;

  nsresult rv;
  uint32_t size;
  uint64_t first_sample_offset = mOutputSize;
  nsAutoPtr<MovieFragmentBox> moof_box(new MovieFragmentBox(aTrackType, this));
  nsAutoPtr<MediaDataBox> mdat_box(new MediaDataBox(aTrackType, this));

  rv = moof_box->Generate(&size);
  NS_ENSURE_SUCCESS(rv, rv);
  first_sample_offset += size;
  rv = mdat_box->Generate(&size);
  NS_ENSURE_SUCCESS(rv, rv);
  first_sample_offset += mdat_box->FirstSampleOffsetInMediaDataBox();

  
  nsTArray<nsRefPtr<MuxerOperation>> tfhds;
  rv = moof_box->Find(NS_LITERAL_CSTRING("tfhd"), tfhds);
  NS_ENSURE_SUCCESS(rv, rv);
  uint32_t len = tfhds.Length();
  for (uint32_t i = 0; i < len; i++) {
    TrackFragmentHeaderBox* tfhd = (TrackFragmentHeaderBox*) tfhds.ElementAt(i).get();
    rv = tfhd->UpdateBaseDataOffset(first_sample_offset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = moof_box->Write();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mdat_box->Write();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

uint32_t
ISOControl::GetTime()
{
  return (uint64_t)time(nullptr) + iso_time_offset;
}

}
