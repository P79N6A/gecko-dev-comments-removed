






































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





#define MAX_VIDEO_WIDTH  4000
#define MAX_VIDEO_HEIGHT 3000

using mozilla::layers::PlanarYCbCrImage;



PR_STATIC_ASSERT(MAX_VIDEO_WIDTH < PlanarYCbCrImage::MAX_DIMENSION);
PR_STATIC_ASSERT(MAX_VIDEO_HEIGHT < PlanarYCbCrImage::MAX_DIMENSION);
PR_STATIC_ASSERT(PlanarYCbCrImage::MAX_DIMENSION < PR_UINT32_MAX / PlanarYCbCrImage::MAX_DIMENSION);




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
         aPlane.mWidth * aPlane.mHeight < MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT &&
         aPlane.mStride > 0;
}

PRBool
nsVideoInfo::ValidateVideoRegion(const nsIntSize& aFrame,
                                 const nsIntRect& aPicture,
                                 const nsIntSize& aDisplay)
{
  return
    aFrame.width <= PlanarYCbCrImage::MAX_DIMENSION &&
    aFrame.height <= PlanarYCbCrImage::MAX_DIMENSION &&
    aFrame.width * aFrame.height <= MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT &&
    aFrame.width * aFrame.height != 0 &&
    aPicture.width <= PlanarYCbCrImage::MAX_DIMENSION &&
    aPicture.x < PlanarYCbCrImage::MAX_DIMENSION &&
    aPicture.x + aPicture.width < PlanarYCbCrImage::MAX_DIMENSION &&
    aPicture.height <= PlanarYCbCrImage::MAX_DIMENSION &&
    aPicture.y < PlanarYCbCrImage::MAX_DIMENSION &&
    aPicture.y + aPicture.height < PlanarYCbCrImage::MAX_DIMENSION &&
    aPicture.width * aPicture.height <= MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT &&
    aPicture.width * aPicture.height != 0 &&
    aDisplay.width <= PlanarYCbCrImage::MAX_DIMENSION &&
    aDisplay.height <= PlanarYCbCrImage::MAX_DIMENSION &&
    aDisplay.width * aDisplay.height <= MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT &&
    aDisplay.width * aDisplay.height != 0;
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
  if (!ValidatePlane(aBuffer.mPlanes[0]) || !ValidatePlane(aBuffer.mPlanes[1]) ||
      !ValidatePlane(aBuffer.mPlanes[2])) {
    NS_WARNING("Invalid plane size");
    return nsnull;
  }

  PRUint32 picX = aInfo.mPicture.x;
  PRUint32 picY = aInfo.mPicture.y;
  gfxIntSize picSize = gfxIntSize(aInfo.mPicture.width, aInfo.mPicture.height);

  if (aInfo.mFrame.width != aBuffer.mPlanes[0].mWidth ||
      aInfo.mFrame.height != aBuffer.mPlanes[0].mHeight)
  {
    
    
    
    picX = (aInfo.mPicture.x * aBuffer.mPlanes[0].mWidth) / aInfo.mFrame.width;
    picY = (aInfo.mPicture.y * aBuffer.mPlanes[0].mHeight) / aInfo.mFrame.height;
    picSize = gfxIntSize((aBuffer.mPlanes[0].mWidth * aInfo.mPicture.width) / aInfo.mFrame.width,
                         (aBuffer.mPlanes[0].mHeight * aInfo.mPicture.height) / aInfo.mFrame.height);
  }

  
  
  PRUint32 picXLimit;
  PRUint32 picYLimit;
  if (!AddOverflow32(picX, picSize.width, picXLimit) ||
      picXLimit > aBuffer.mPlanes[0].mStride ||
      !AddOverflow32(picY, picSize.height, picYLimit) ||
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
  data.mPicX = picX;
  data.mPicY = picY;
  data.mPicSize = picSize;
  data.mStereoMode = aInfo.mStereoMode;

  videoImage->SetData(data); 
  return v.forget();
}

nsBuiltinDecoderReader::nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder)
  : mReentrantMonitor("media.decoderreader"),
    mDecoder(aDecoder)
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
      ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
      if (mDecoder->GetDecodeState() == nsDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return nsnull;
      }
    }
    eof = !(this->*aDecodeFn)();
  }
  Data* d = nsnull;
  return (d = aQueue.PeekFront()) ? d : nsnull;
}

nsresult nsBuiltinDecoderReader::DecodeToTarget(PRInt64 aTarget)
{
  
  if (HasVideo()) {
    PRBool eof = PR_FALSE;
    PRInt64 startTime = -1;
    while (HasVideo() && !eof) {
      while (mVideoQueue.GetSize() == 0 && !eof) {
        PRBool skip = PR_FALSE;
        eof = !DecodeVideoFrame(skip, 0);
        {
          ReentrantMonitorAutoExit exitReaderMon(mReentrantMonitor);
          ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
          if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      if (mVideoQueue.GetSize() == 0) {
        break;
      }
      nsAutoPtr<VideoData> video(mVideoQueue.PeekFront());
      
      
      if (video && video->mEndTime <= aTarget) {
        if (startTime == -1) {
          startTime = video->mTime;
        }
        mVideoQueue.PopFront();
        video = nsnull;
      } else {
        video.forget();
        break;
      }
    }
    {
      ReentrantMonitorAutoExit exitReaderMon(mReentrantMonitor);
      ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
      if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return NS_ERROR_FAILURE;
      }
    }
    LOG(PR_LOG_DEBUG, ("First video frame after decode is %lld", startTime));
  }

  if (HasAudio()) {
    
    PRBool eof = PR_FALSE;
    while (HasAudio() && !eof) {
      while (!eof && mAudioQueue.GetSize() == 0) {
        eof = !DecodeAudioData();
        {
          ReentrantMonitorAutoExit exitReaderMon(mReentrantMonitor);
          ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
          if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      nsAutoPtr<SoundData> audio(mAudioQueue.PeekFront());
      if (audio && audio->mTime + audio->mDuration <= aTarget) {
        mAudioQueue.PopFront();
        audio = nsnull;
      } else {
        audio.forget();
        break;
      }
    }
  }
  return NS_OK;
}


