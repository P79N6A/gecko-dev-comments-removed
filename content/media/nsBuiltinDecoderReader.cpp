






































#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "VideoUtils.h"

#include "mozilla/mozalloc.h"
#include "mozilla/StandardInteger.h"

using namespace mozilla;
using mozilla::layers::ImageContainer;
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

static bool
ValidatePlane(const VideoData::YCbCrBuffer::Plane& aPlane)
{
  return aPlane.mWidth <= PlanarYCbCrImage::MAX_DIMENSION &&
         aPlane.mHeight <= PlanarYCbCrImage::MAX_DIMENSION &&
         aPlane.mWidth * aPlane.mHeight < MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT &&
         aPlane.mStride > 0;
}

bool
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
                             bool aKeyframe,
                             PRInt64 aTimecode,
                             nsIntRect aPicture)
{
  if (!aContainer) {
    return nsnull;
  }

  
  
  if (aBuffer.mPlanes[1].mWidth != aBuffer.mPlanes[2].mWidth ||
      aBuffer.mPlanes[1].mHeight != aBuffer.mPlanes[2].mHeight) {
    NS_ERROR("C planes with different sizes");
    return nsnull;
  }

  
  if (aPicture.width <= 0 || aPicture.height <= 0) {
    NS_WARNING("Empty picture rect");
    return nsnull;
  }
  if (!ValidatePlane(aBuffer.mPlanes[0]) || !ValidatePlane(aBuffer.mPlanes[1]) ||
      !ValidatePlane(aBuffer.mPlanes[2])) {
    NS_WARNING("Invalid plane size");
    return nsnull;
  }

  
  
  CheckedUint32 xLimit = aPicture.x + CheckedUint32(aPicture.width);
  CheckedUint32 yLimit = aPicture.y + CheckedUint32(aPicture.height);
  if (!xLimit.valid() || xLimit.value() > aBuffer.mPlanes[0].mStride ||
      !yLimit.valid() || yLimit.value() > aBuffer.mPlanes[0].mHeight)
  {
    
    
    NS_WARNING("Overflowing picture rect");
    return nsnull;
  }

  nsAutoPtr<VideoData> v(new VideoData(aOffset,
                                       aTime,
                                       aEndTime,
                                       aKeyframe,
                                       aTimecode,
                                       aInfo.mDisplay));
  
  
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
  data.mPicX = aPicture.x;
  data.mPicY = aPicture.y;
  data.mPicSize = gfxIntSize(aPicture.width, aPicture.height);
  data.mStereoMode = aInfo.mStereoMode;

  videoImage->SetData(data); 
  return v.forget();
}

nsBuiltinDecoderReader::nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder)
  : mDecoder(aDecoder)
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

VideoData* nsBuiltinDecoderReader::FindStartTime(PRInt64& aOutStartTime)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or decode thread.");

  
  
  PRInt64 videoStartTime = INT64_MAX;
  PRInt64 audioStartTime = INT64_MAX;
  VideoData* videoData = nsnull;

  if (HasVideo()) {
    videoData = DecodeToFirstData(&nsBuiltinDecoderReader::DecodeVideoFrame,
                                  mVideoQueue);
    if (videoData) {
      videoStartTime = videoData->mTime;
    }
  }
  if (HasAudio()) {
    AudioData* audioData = DecodeToFirstData(&nsBuiltinDecoderReader::DecodeAudioData,
                                             mAudioQueue);
    if (audioData) {
      audioStartTime = audioData->mTime;
    }
  }

  PRInt64 startTime = NS_MIN(videoStartTime, audioStartTime);
  if (startTime != INT64_MAX) {
    aOutStartTime = startTime;
  }

  return videoData;
}

template<class Data>
Data* nsBuiltinDecoderReader::DecodeToFirstData(DecodeFn aDecodeFn,
                                                MediaQueue<Data>& aQueue)
{
  bool eof = false;
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
    bool eof = false;
    PRInt64 startTime = -1;
    nsAutoPtr<VideoData> video;
    while (HasVideo() && !eof) {
      while (mVideoQueue.GetSize() == 0 && !eof) {
        bool skip = false;
        eof = !DecodeVideoFrame(skip, 0);
        {
          ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
          if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      if (mVideoQueue.GetSize() == 0) {
        
        if (video) {
          mVideoQueue.PushFront(video.forget());
        }
        break;
      }
      video = mVideoQueue.PeekFront();
      
      
      if (video && video->mEndTime <= aTarget) {
        if (startTime == -1) {
          startTime = video->mTime;
        }
        mVideoQueue.PopFront();
      } else {
        video.forget();
        break;
      }
    }
    {
      ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
      if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return NS_ERROR_FAILURE;
      }
    }
    LOG(PR_LOG_DEBUG, ("First video frame after decode is %lld", startTime));
  }

  if (HasAudio()) {
    
    bool eof = false;
    while (HasAudio() && !eof) {
      while (!eof && mAudioQueue.GetSize() == 0) {
        eof = !DecodeAudioData();
        {
          ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
          if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      const AudioData* audio = mAudioQueue.PeekFront();
      if (!audio)
        break;
      CheckedInt64 startFrame = UsecsToFrames(audio->mTime, mInfo.mAudioRate);
      CheckedInt64 targetFrame = UsecsToFrames(aTarget, mInfo.mAudioRate);
      if (!startFrame.valid() || !targetFrame.valid()) {
        return NS_ERROR_FAILURE;
      }
      if (startFrame.value() + audio->mFrames <= targetFrame.value()) {
        
        
        delete mAudioQueue.PopFront();
        audio = nsnull;
        continue;
      }
      if (startFrame.value() > targetFrame.value()) {
        
        
        
        
        
        
        
        NS_WARNING("Audio not synced after seek, maybe a poorly muxed file?");
        break;
      }

      
      
      
      NS_ASSERTION(targetFrame.value() >= startFrame.value(),
                   "Target must at or be after data start.");
      NS_ASSERTION(targetFrame.value() < startFrame.value() + audio->mFrames,
                   "Data must end after target.");

      PRInt64 framesToPrune = targetFrame.value() - startFrame.value();
      if (framesToPrune > audio->mFrames) {
        
        
        NS_WARNING("Can't prune more frames that we have!");
        break;
      }
      PRUint32 frames = audio->mFrames - static_cast<PRUint32>(framesToPrune);
      PRUint32 channels = audio->mChannels;
      nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[frames * channels]);
      memcpy(audioData.get(),
             audio->mAudioData.get() + (framesToPrune * channels),
             frames * channels * sizeof(AudioDataValue));
      CheckedInt64 duration = FramesToUsecs(frames, mInfo.mAudioRate);
      if (!duration.valid()) {
        return NS_ERROR_FAILURE;
      }
      nsAutoPtr<AudioData> data(new AudioData(audio->mOffset,
                                              aTarget,
                                              duration.value(),
                                              frames,
                                              audioData.forget(),
                                              channels));
      delete mAudioQueue.PopFront();
      mAudioQueue.PushFront(data.forget());
      break;
    }
  }
  return NS_OK;
}


