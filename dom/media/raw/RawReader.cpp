




#include "MediaDecoderStateMachine.h"
#include "AbstractMediaDecoder.h"
#include "RawReader.h"
#include "RawDecoder.h"
#include "VideoUtils.h"
#include "nsISeekableStream.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::media;

RawReader::RawReader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder),
    mCurrentFrame(0), mFrameSize(0)
{
  MOZ_COUNT_CTOR(RawReader);
}

RawReader::~RawReader()
{
  MOZ_COUNT_DTOR(RawReader);
}

nsresult RawReader::Init(MediaDecoderReader* aCloneDonor)
{
  return NS_OK;
}

nsresult RawReader::ResetDecode()
{
  mCurrentFrame = 0;
  return MediaDecoderReader::ResetDecode();
}

nsresult RawReader::ReadMetadata(MediaInfo* aInfo,
                                 MetadataTags** aTags)
{
  MOZ_ASSERT(OnTaskQueue());

  MediaResource* resource = mDecoder->GetResource();
  NS_ASSERTION(resource, "Decoder has no media resource");

  if (!ReadFromResource(resource, reinterpret_cast<uint8_t*>(&mMetadata),
                        sizeof(mMetadata)))
    return NS_ERROR_FAILURE;

  
  if (!(mMetadata.headerPacketID == 0  &&
        mMetadata.codecID == RAW_ID  &&
        mMetadata.majorVersion == 0 &&
        mMetadata.minorVersion == 1))
    return NS_ERROR_FAILURE;

  CheckedUint32 dummy = CheckedUint32(static_cast<uint32_t>(mMetadata.frameWidth)) *
                          static_cast<uint32_t>(mMetadata.frameHeight);
  NS_ENSURE_TRUE(dummy.isValid(), NS_ERROR_FAILURE);

  if (mMetadata.aspectDenominator == 0 ||
      mMetadata.framerateDenominator == 0)
    return NS_ERROR_FAILURE; 

  
  float pixelAspectRatio = static_cast<float>(mMetadata.aspectNumerator) /
                            mMetadata.aspectDenominator;
  nsIntSize display(mMetadata.frameWidth, mMetadata.frameHeight);
  ScaleDisplayByAspectRatio(display, pixelAspectRatio);
  mPicture = nsIntRect(0, 0, mMetadata.frameWidth, mMetadata.frameHeight);
  nsIntSize frameSize(mMetadata.frameWidth, mMetadata.frameHeight);
  if (!IsValidVideoRegion(frameSize, mPicture, display)) {
    
    return NS_ERROR_FAILURE;
  }

  mInfo.mVideo.mDisplay = display;

  mFrameRate = static_cast<float>(mMetadata.framerateNumerator) /
               mMetadata.framerateDenominator;

  
  if (mFrameRate > 45 ||
      mFrameRate == 0 ||
      pixelAspectRatio == 0 ||
      mMetadata.frameWidth > 2000 ||
      mMetadata.frameHeight > 2000 ||
      mMetadata.chromaChannelBpp != 4 ||
      mMetadata.lumaChannelBpp != 8 ||
      mMetadata.colorspace != 1 )
    return NS_ERROR_FAILURE;

  mFrameSize = mMetadata.frameWidth * mMetadata.frameHeight *
    (mMetadata.lumaChannelBpp + mMetadata.chromaChannelBpp) / 8.0 +
    sizeof(RawPacketHeader);

  int64_t length = resource->GetLength();
  if (length != -1) {
    mInfo.mMetadataDuration.emplace(TimeUnit::FromSeconds((length - sizeof(RawVideoHeader)) /
                                                          (mFrameSize * mFrameRate)));
  }

  *aInfo = mInfo;

  *aTags = nullptr;

  return NS_OK;
}

bool
RawReader::IsMediaSeekable()
{
  
  return true;
}

 bool RawReader::DecodeAudioData()
{
  MOZ_ASSERT(OnTaskQueue() || mDecoder->OnStateMachineTaskQueue());
  return false;
}



bool RawReader::ReadFromResource(MediaResource *aResource, uint8_t* aBuf,
                                   uint32_t aLength)
{
  while (aLength > 0) {
    uint32_t bytesRead = 0;
    nsresult rv;

    rv = aResource->Read(reinterpret_cast<char*>(aBuf), aLength, &bytesRead);
    NS_ENSURE_SUCCESS(rv, false);

    if (bytesRead == 0) {
      return false;
    }

    aLength -= bytesRead;
    aBuf += bytesRead;
  }

  return true;
}

bool RawReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                     int64_t aTimeThreshold)
{
  MOZ_ASSERT(OnTaskQueue());

  
  
  AbstractMediaDecoder::AutoNotifyDecoded a(mDecoder);

  if (!mFrameSize)
    return false; 

  int64_t currentFrameTime = USECS_PER_S * mCurrentFrame / mFrameRate;
  uint32_t length = mFrameSize - sizeof(RawPacketHeader);

  nsAutoArrayPtr<uint8_t> buffer(new uint8_t[length]);
  MediaResource* resource = mDecoder->GetResource();
  NS_ASSERTION(resource, "Decoder has no media resource");

  
  while(true) {
    RawPacketHeader header;

    
    if (!(ReadFromResource(resource, reinterpret_cast<uint8_t*>(&header),
                           sizeof(header))) ||
        !(header.packetID == 0xFF && header.codecID == RAW_ID )) {
      return false;
    }

    if (!ReadFromResource(resource, buffer, length)) {
      return false;
    }

    a.mParsed++;

    if (currentFrameTime >= aTimeThreshold)
      break;

    mCurrentFrame++;
    currentFrameTime += static_cast<double>(USECS_PER_S) / mFrameRate;
  }

  VideoData::YCbCrBuffer b;
  b.mPlanes[0].mData = buffer;
  b.mPlanes[0].mStride = mMetadata.frameWidth * mMetadata.lumaChannelBpp / 8.0;
  b.mPlanes[0].mHeight = mMetadata.frameHeight;
  b.mPlanes[0].mWidth = mMetadata.frameWidth;
  b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

  uint32_t cbcrStride = mMetadata.frameWidth * mMetadata.chromaChannelBpp / 8.0;

  b.mPlanes[1].mData = buffer + mMetadata.frameHeight * b.mPlanes[0].mStride;
  b.mPlanes[1].mStride = cbcrStride;
  b.mPlanes[1].mHeight = mMetadata.frameHeight / 2;
  b.mPlanes[1].mWidth = mMetadata.frameWidth / 2;
  b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

  b.mPlanes[2].mData = b.mPlanes[1].mData + mMetadata.frameHeight * cbcrStride / 2;
  b.mPlanes[2].mStride = cbcrStride;
  b.mPlanes[2].mHeight = mMetadata.frameHeight / 2;
  b.mPlanes[2].mWidth = mMetadata.frameWidth / 2;
  b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;

  nsRefPtr<VideoData> v = VideoData::Create(mInfo.mVideo,
                                            mDecoder->GetImageContainer(),
                                            -1,
                                            currentFrameTime,
                                            (USECS_PER_S / mFrameRate),
                                            b,
                                            1, 
                                            -1,
                                            mPicture);
  if (!v)
    return false;

  mVideoQueue.Push(v);
  mCurrentFrame++;
  a.mDecoded++;

  return true;
}

nsRefPtr<MediaDecoderReader::SeekPromise>
RawReader::Seek(int64_t aTime, int64_t aEndTime)
{
  MOZ_ASSERT(OnTaskQueue());

  MediaResource *resource = mDecoder->GetResource();
  NS_ASSERTION(resource, "Decoder has no media resource");

  uint32_t frame = mCurrentFrame;
  if (aTime >= UINT_MAX)
    return SeekPromise::CreateAndReject(NS_ERROR_FAILURE, __func__);
  mCurrentFrame = aTime * mFrameRate / USECS_PER_S;

  CheckedUint32 offset = CheckedUint32(mCurrentFrame) * mFrameSize;
  offset += sizeof(RawVideoHeader);
  NS_ENSURE_TRUE(offset.isValid(), SeekPromise::CreateAndReject(NS_ERROR_FAILURE, __func__));

  nsresult rv = resource->Seek(nsISeekableStream::NS_SEEK_SET, offset.value());
  NS_ENSURE_SUCCESS(rv, SeekPromise::CreateAndReject(rv, __func__));

  mVideoQueue.Reset();
  nsRefPtr<SeekPromise::Private> p = new SeekPromise::Private(__func__);
  nsRefPtr<RawReader> self = this;
  InvokeUntil([self] () {
    MOZ_ASSERT(self->OnTaskQueue());
    NS_ENSURE_TRUE(!self->mShutdown, false);
    bool skip = false;
    return self->DecodeVideoFrame(skip, 0);
  }, [self, aTime] () {
    MOZ_ASSERT(self->OnTaskQueue());
    return self->mVideoQueue.Peek() &&
           self->mVideoQueue.Peek()->GetEndTime() >= aTime;
  })->Then(OwnerThread(), __func__, [self, p, aTime] () {
    while (self->mVideoQueue.GetSize() >= 2) {
      nsRefPtr<VideoData> releaseMe = self->mVideoQueue.PopFront();
    }
    p->Resolve(aTime, __func__);
  }, [self, p, frame] {
    self->mCurrentFrame = frame;
    self->mVideoQueue.Reset();
    p->Reject(NS_ERROR_FAILURE, __func__);
  });

  return p.forget();
}

media::TimeIntervals RawReader::GetBuffered()
{
  MOZ_ASSERT(OnTaskQueue());
  return media::TimeIntervals();
}
