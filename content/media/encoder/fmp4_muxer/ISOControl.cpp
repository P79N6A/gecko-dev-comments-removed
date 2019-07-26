




#include <time.h>
#include "nsAutoPtr.h"
#include "ISOControl.h"
#include "ISOMediaBoxes.h"
#include "EncodedFrameContainer.h"

namespace mozilla {



#define iso_time_offset 2082844800

const static uint32_t MUXING_BUFFER_SIZE = 512*1024;

FragmentBuffer::FragmentBuffer(uint32_t aTrackType, uint32_t aFragDuration,
                               TrackMetadataBase* aMetadata)
  : mTrackType(aTrackType)
  , mFragDuration(aFragDuration)
  , mMediaStartTime(0)
  , mFragmentNumber(0)
  , mEOS(false)
{
  mFragArray.AppendElement();
  if (mTrackType == Audio_Track) {
    nsRefPtr<AACTrackMetadata> audMeta = static_cast<AACTrackMetadata*>(aMetadata);
    MOZ_ASSERT(audMeta);
  } else {
    nsRefPtr<AVCTrackMetadata> vidMeta = static_cast<AVCTrackMetadata*>(aMetadata);
    MOZ_ASSERT(vidMeta);
  }
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
  if (type == EncodedFrame::AAC_CSD || type == EncodedFrame::AVC_CSD) {
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

  
  if ((aFrame->GetTimeStamp() - mMediaStartTime) > (mFragDuration * mFragmentNumber)) {
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

ISOControl::ISOControl()
  : mAudioFragmentBuffer(nullptr)
  , mVideoFragmentBuffer(nullptr)
  , mFragNum(0)
  , mOutputSize(0)
  , mBitCount(0)
  , mBit(0)
{
  mOutBuffer.SetCapacity(MUXING_BUFFER_SIZE);
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
ISOControl::GetTrackID(uint32_t aTrackType)
{
  TrackMetadataBase::MetadataKind kind;
  if (aTrackType == Audio_Track) {
    kind = TrackMetadataBase::METADATA_AAC;
  } else {
    kind = TrackMetadataBase::METADATA_AVC;
  }

  for (uint32_t i = 0; i < mMetaArray.Length(); i++) {
    if (mMetaArray[i]->GetKind() == kind) {
      return (i + 1);
    }
  }

  return 0;
}

nsresult
ISOControl::SetMetadata(TrackMetadataBase* aTrackMeta)
{
  if (aTrackMeta->GetKind() == TrackMetadataBase::METADATA_AAC ||
      aTrackMeta->GetKind() == TrackMetadataBase::METADATA_AVC) {
    mMetaArray.AppendElement(aTrackMeta);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
ISOControl::GetAudioMetadata(nsRefPtr<AACTrackMetadata>& aAudMeta)
{
  for (uint32_t i = 0; i < mMetaArray.Length() ; i++) {
    if (mMetaArray[i]->GetKind() == TrackMetadataBase::METADATA_AAC) {
      aAudMeta = static_cast<AACTrackMetadata*>(mMetaArray[i].get());
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult
ISOControl::GetVideoMetadata(nsRefPtr<AVCTrackMetadata>& aVidMeta)
{
  for (uint32_t i = 0; i < mMetaArray.Length() ; i++) {
    if (mMetaArray[i]->GetKind() == TrackMetadataBase::METADATA_AVC) {
      aVidMeta = static_cast<AVCTrackMetadata*>(mMetaArray[i].get());
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

bool
ISOControl::HasAudioTrack()
{
  nsRefPtr<AACTrackMetadata> audMeta;
  GetAudioMetadata(audMeta);
  return audMeta;
}

bool
ISOControl::HasVideoTrack()
{
  nsRefPtr<AVCTrackMetadata> vidMeta;
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
ISOControl::GetBuf(nsTArray<uint8_t>& aOutBuf)
{
  mOutputSize += mOutBuffer.Length();
  aOutBuf.SwapElements(mOutBuffer);
  return FlushBuf();
}

nsresult
ISOControl::FlushBuf()
{
  mOutBuffer.SetCapacity(MUXING_BUFFER_SIZE);
  mLastWrittenBoxPos = 0;
  return NS_OK;
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
  mOutBuffer.AppendElements(aBuf, aSize);
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
  uint64_t first_sample_offset = mOutputSize + mLastWrittenBoxPos;
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
