






































#include "nsISeekableStream.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"

using namespace mozilla;
using mozilla::layers::ImageContainer;
using mozilla::layers::PlanarYCbCrImage;




#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#ifdef SEEK_LOGGING
#define SEEK_LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define SEEK_LOG(type, msg)
#endif
#else
#define LOG(type, msg)
#define SEEK_LOG(type, msg)
#endif

static PRBool
ValidatePlane(const VideoData::YCbCrBuffer::Plane& aPlane)
{
  return aPlane.mWidth <= PlanarYCbCrImage::MAX_DIMENSION &&
         aPlane.mHeight <= PlanarYCbCrImage::MAX_DIMENSION &&
         aPlane.mStride > 0;
}

VideoData* VideoData::Create(nsVideoInfo& aInfo,
                             ImageContainer* aContainer,
                             PRInt64 aOffset,
                             PRInt64 aTime,
                             PRInt64 aEndTime,
                             const YCbCrBuffer& aBuffer,
                             PRBool aKeyframe,
                             PRInt64 aTimecode)
{
  if (!aContainer) {
    return nsnull;
  }

  
  
  if (aBuffer.mPlanes[1].mWidth != aBuffer.mPlanes[2].mWidth ||
      aBuffer.mPlanes[1].mHeight != aBuffer.mPlanes[2].mHeight) {
    NS_ERROR("C planes with different sizes");
    return nsnull;
  }

  
  if (aInfo.mPicture.width <= 0 || aInfo.mPicture.height <= 0) {
    NS_WARNING("Empty picture rect");
    return nsnull;
  }
  if (aBuffer.mPlanes[0].mWidth != PRUint32(aInfo.mFrame.width) ||
      aBuffer.mPlanes[0].mHeight != PRUint32(aInfo.mFrame.height)) {
    NS_WARNING("Unexpected frame size");
    return nsnull;
  }
  if (!ValidatePlane(aBuffer.mPlanes[0]) || !ValidatePlane(aBuffer.mPlanes[1]) ||
      !ValidatePlane(aBuffer.mPlanes[2])) {
    NS_WARNING("Invalid plane size");
    return nsnull;
  }
  
  
  PRUint32 picXLimit;
  PRUint32 picYLimit;
  if (!AddOverflow32(aInfo.mPicture.x, aInfo.mPicture.width, picXLimit) ||
      picXLimit > aBuffer.mPlanes[0].mStride ||
      !AddOverflow32(aInfo.mPicture.y, aInfo.mPicture.height, picYLimit) ||
      picYLimit > aBuffer.mPlanes[0].mHeight)
  {
    
    
    NS_WARNING("Overflowing picture rect");
    return nsnull;
  }

  nsAutoPtr<VideoData> v(new VideoData(aOffset, aTime, aEndTime, aKeyframe, aTimecode));
  
  
  Image::Format format = Image::PLANAR_YCBCR;
  v->mImage = aContainer->CreateImage(&format, 1);
  if (!v->mImage) {
    return nsnull;
  }
  NS_ASSERTION(v->mImage->GetFormat() == Image::PLANAR_YCBCR,
               "Wrong format?");
  PlanarYCbCrImage* videoImage = static_cast<PlanarYCbCrImage*>(v->mImage.get());

  PlanarYCbCrImage::Data data;
  data.mYChannel = aBuffer.mPlanes[0].mData;
  data.mYSize = gfxIntSize(aBuffer.mPlanes[0].mWidth, aBuffer.mPlanes[0].mHeight);
  data.mYStride = aBuffer.mPlanes[0].mStride;
  data.mCbChannel = aBuffer.mPlanes[1].mData;
  data.mCrChannel = aBuffer.mPlanes[2].mData;
  data.mCbCrSize = gfxIntSize(aBuffer.mPlanes[1].mWidth, aBuffer.mPlanes[1].mHeight);
  data.mCbCrStride = aBuffer.mPlanes[1].mStride;
  data.mPicX = aInfo.mPicture.x;
  data.mPicY = aInfo.mPicture.y;
  data.mPicSize = gfxIntSize(aInfo.mPicture.width, aInfo.mPicture.height);

  videoImage->SetData(data); 
  return v.forget();
}

nsBuiltinDecoderReader::nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder)
  : mMonitor("media.decoderreader"),
    mDecoder(aDecoder),
    mDataOffset(0)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoderReader);
}

nsBuiltinDecoderReader::~nsBuiltinDecoderReader()
{
  ResetDecode();
  MOZ_COUNT_DTOR(nsBuiltinDecoderReader);
}

nsresult nsBuiltinDecoderReader::ResetDecode()
{
  nsresult res = NS_OK;

  mVideoQueue.Reset();
  mAudioQueue.Reset();

  return res;
}

nsresult nsBuiltinDecoderReader::GetBufferedBytes(nsTArray<ByteRange>& aRanges)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  mMonitor.AssertCurrentThreadIn();
  PRInt64 startOffset = mDataOffset;
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  while (PR_TRUE) {
    PRInt64 endOffset = stream->GetCachedDataEnd(startOffset);
    if (endOffset == startOffset) {
      
      endOffset = stream->GetNextCachedData(startOffset);
      if (endOffset == -1) {
        
        
        break;
      }
    } else {
      
      PRInt64 startTime = -1;
      PRInt64 endTime = -1;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      FindStartTime(startOffset, startTime);
      if (startTime != -1 &&
          ((endTime = FindEndTime(endOffset)) != -1))
      {
        NS_ASSERTION(startOffset < endOffset,
                     "Start offset must be before end offset");
        NS_ASSERTION(startTime < endTime,
                     "Start time must be before end time");
        aRanges.AppendElement(ByteRange(startOffset,
                                        endOffset,
                                        startTime,
                                        endTime));
      }
    }
    startOffset = endOffset;
  }
  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

ByteRange
nsBuiltinDecoderReader::GetSeekRange(const nsTArray<ByteRange>& ranges,
                                     PRInt64 aTarget,
                                     PRInt64 aStartTime,
                                     PRInt64 aEndTime,
                                     PRBool aExact)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  PRInt64 so = mDataOffset;
  PRInt64 eo = mDecoder->GetCurrentStream()->GetLength();
  PRInt64 st = aStartTime;
  PRInt64 et = aEndTime;
  for (PRUint32 i = 0; i < ranges.Length(); i++) {
    const ByteRange &r = ranges[i];
    if (r.mTimeStart < aTarget) {
      so = r.mOffsetStart;
      st = r.mTimeStart;
    }
    if (r.mTimeEnd >= aTarget && r.mTimeEnd < et) {
      eo = r.mOffsetEnd;
      et = r.mTimeEnd;
    }

    if (r.mTimeStart < aTarget && aTarget <= r.mTimeEnd) {
      
      return ranges[i];
    }
  }
  return aExact ? ByteRange() : ByteRange(so, eo, st, et);
}

VideoData* nsBuiltinDecoderReader::FindStartTime(PRInt64 aOffset,
                                      PRInt64& aOutStartTime)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(), "Should be on state machine thread.");

  if (NS_FAILED(ResetDecode())) {
    return nsnull;
  }

  
  
  PRInt64 videoStartTime = PR_INT64_MAX;
  PRInt64 audioStartTime = PR_INT64_MAX;
  VideoData* videoData = nsnull;

  if (HasVideo()) {
    videoData = DecodeToFirstData(&nsBuiltinDecoderReader::DecodeVideoFrame,
                                  mVideoQueue);
    if (videoData) {
      videoStartTime = videoData->mTime;
    }
  }
  if (HasAudio()) {
    SoundData* soundData = DecodeToFirstData(&nsBuiltinDecoderReader::DecodeAudioData,
                                             mAudioQueue);
    if (soundData) {
      audioStartTime = soundData->mTime;
    }
  }

  PRInt64 startTime = PR_MIN(videoStartTime, audioStartTime);
  if (startTime != PR_INT64_MAX) {
    aOutStartTime = startTime;
  }

  return videoData;
}

PRInt64 nsBuiltinDecoderReader::FindEndTime(PRInt64 aEndOffset)
{
  return -1;
}

template<class Data>
Data* nsBuiltinDecoderReader::DecodeToFirstData(DecodeFn aDecodeFn,
                                                MediaQueue<Data>& aQueue)
{
  PRBool eof = PR_FALSE;
  while (!eof && aQueue.GetSize() == 0) {
    {
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      if (mDecoder->GetDecodeState() == nsDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return nsnull;
      }
    }
    eof = !(this->*aDecodeFn)();
  }
  Data* d = nsnull;
  return (d = aQueue.PeekFront()) ? d : nsnull;
}


