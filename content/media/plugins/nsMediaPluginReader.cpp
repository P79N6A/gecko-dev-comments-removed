




#include "nsMediaPluginReader.h"
#include "mozilla/TimeStamp.h"
#include "nsTimeRanges.h"
#include "MediaResource.h"
#include "VideoUtils.h"
#include "nsMediaPluginDecoder.h"
#include "nsMediaPluginHost.h"
#include "nsBuiltinDecoderStateMachine.h"

using namespace mozilla;

nsMediaPluginReader::nsMediaPluginReader(nsBuiltinDecoder *aDecoder) :
  nsBuiltinDecoderReader(aDecoder),
  mPlugin(NULL),
  mHasAudio(false),
  mHasVideo(false),
  mVideoSeekTimeUs(-1),
  mAudioSeekTimeUs(-1),
  mLastVideoFrame(NULL)
{
  reinterpret_cast<nsMediaPluginDecoder *>(aDecoder)->GetContentType(mType);
}

nsMediaPluginReader::~nsMediaPluginReader()
{
  ResetDecode();
}

nsresult nsMediaPluginReader::Init(nsBuiltinDecoderReader* aCloneDonor)
{
  return NS_OK;
}

nsresult nsMediaPluginReader::ReadMetadata(nsVideoInfo* aInfo,
                                           MetadataTags** aTags)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  if (!mPlugin) {
    mPlugin = GetMediaPluginHost()->CreateDecoder(mDecoder->GetResource(), mType);
    if (!mPlugin) {
      return NS_ERROR_FAILURE;
    }
  }

  
  int64_t durationUs;
  mPlugin->GetDuration(mPlugin, &durationUs);
  if (durationUs) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->GetStateMachine()->SetDuration(durationUs);
  }

  if (mPlugin->HasVideo(mPlugin)) {
    int32_t width, height;
    mPlugin->GetVideoParameters(mPlugin, &width, &height);
    nsIntRect pictureRect(0, 0, width, height);

    
    
    nsIntSize displaySize(width, height);
    nsIntSize frameSize(width, height);
    if (!nsVideoInfo::ValidateVideoRegion(frameSize, pictureRect, displaySize)) {
      return NS_ERROR_FAILURE;
    }

    
    mHasVideo = mInfo.mHasVideo = true;
    mInfo.mDisplay = displaySize;
    mPicture = pictureRect;
    mInitialFrame = frameSize;
    VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
    if (container) {
      container->SetCurrentFrame(gfxIntSize(displaySize.width, displaySize.height),
                                 nullptr,
                                 mozilla::TimeStamp::Now());
    }
  }

  if (mPlugin->HasAudio(mPlugin)) {
    int32_t numChannels, sampleRate;
    mPlugin->GetAudioParameters(mPlugin, &numChannels, &sampleRate);
    mHasAudio = mInfo.mHasAudio = true;
    mInfo.mAudioChannels = numChannels;
    mInfo.mAudioRate = sampleRate;
  }

 *aInfo = mInfo;
 *aTags = nullptr;
  return NS_OK;
}


nsresult nsMediaPluginReader::ResetDecode()
{
  if (mLastVideoFrame) {
    delete mLastVideoFrame;
    mLastVideoFrame = NULL;
  }
  if (mPlugin) {
    GetMediaPluginHost()->DestroyDecoder(mPlugin);
    mPlugin = NULL;
  }

  return NS_OK;
}

bool nsMediaPluginReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                           int64_t aTimeThreshold)
{
  
  
  uint32_t parsed = 0, decoded = 0;
  nsMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  
  if (mLastVideoFrame && mVideoSeekTimeUs != -1) {
    delete mLastVideoFrame;
    mLastVideoFrame = NULL;
  }

  
  while (true) {
    MPAPI::VideoFrame frame;
    if (!mPlugin->ReadVideo(mPlugin, &frame, mVideoSeekTimeUs)) {
      
      
      
      if (mLastVideoFrame) {
        int64_t durationUs;
        mPlugin->GetDuration(mPlugin, &durationUs);
        mLastVideoFrame->mEndTime = (durationUs > mLastVideoFrame->mTime)
                                  ? durationUs
                                  : mLastVideoFrame->mTime;
        mVideoQueue.Push(mLastVideoFrame);
        mLastVideoFrame = NULL;
      }
      mVideoQueue.Finish();
      return false;
    }
    mVideoSeekTimeUs = -1;

    if (aKeyframeSkip) {
      
      
      
#if 0
      if (!frame.mKeyFrame) {
        ++parsed;
        continue;
      }
#endif
      aKeyframeSkip = false;
    }

    VideoData::YCbCrBuffer b;
    b.mPlanes[0].mData = static_cast<uint8_t *>(frame.Y.mData);
    b.mPlanes[0].mStride = frame.Y.mStride;
    b.mPlanes[0].mHeight = frame.Y.mHeight;
    b.mPlanes[0].mWidth = frame.Y.mWidth;
    b.mPlanes[0].mOffset = frame.Y.mOffset;
    b.mPlanes[0].mSkip = frame.Y.mSkip;

    b.mPlanes[1].mData = static_cast<uint8_t *>(frame.Cb.mData);
    b.mPlanes[1].mStride = frame.Cb.mStride;
    b.mPlanes[1].mHeight = frame.Cb.mHeight;
    b.mPlanes[1].mWidth = frame.Cb.mWidth;
    b.mPlanes[1].mOffset = frame.Cb.mOffset;
    b.mPlanes[1].mSkip = frame.Cb.mSkip;

    b.mPlanes[2].mData = static_cast<uint8_t *>(frame.Cr.mData);
    b.mPlanes[2].mStride = frame.Cr.mStride;
    b.mPlanes[2].mHeight = frame.Cr.mHeight;
    b.mPlanes[2].mWidth = frame.Cr.mWidth;
    b.mPlanes[2].mOffset = frame.Cr.mOffset;
    b.mPlanes[2].mSkip = frame.Cr.mSkip;

    nsIntRect picture = mPicture;
    if (frame.Y.mWidth != mInitialFrame.width ||
        frame.Y.mHeight != mInitialFrame.height) {

      
      
      
      picture.x = (mPicture.x * frame.Y.mWidth) / mInitialFrame.width;
      picture.y = (mPicture.y * frame.Y.mHeight) / mInitialFrame.height;
      picture.width = (frame.Y.mWidth * mPicture.width) / mInitialFrame.width;
      picture.height = (frame.Y.mHeight * mPicture.height) / mInitialFrame.height;
    }

    
    int64_t pos = mDecoder->GetResource()->Tell();

    VideoData *v = VideoData::Create(mInfo,
                                     mDecoder->GetImageContainer(),
                                     pos,
                                     frame.mTimeUs,
                                     frame.mTimeUs+1, 
                                     b,
                                     frame.mKeyFrame,
                                     -1,
                                     picture);

    if (!v) {
      return false;
    }
    parsed++;
    decoded++;
    NS_ASSERTION(decoded <= parsed, "Expect to decode fewer frames than parsed in MediaPlugin...");

    
    
    
    
    if (!mLastVideoFrame) {
      mLastVideoFrame = v;
      continue;
    }

    mLastVideoFrame->mEndTime = v->mTime;

    
    
    
    if (mLastVideoFrame->mEndTime < aTimeThreshold) {
      delete mLastVideoFrame;
      mLastVideoFrame = NULL;
      continue;
    }

    mVideoQueue.Push(mLastVideoFrame);

    
    mLastVideoFrame = v;

    break;
  }

  return true;
}

bool nsMediaPluginReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  int64_t pos = mDecoder->GetResource()->Tell();

  
  MPAPI::AudioFrame frame;
  if (!mPlugin->ReadAudio(mPlugin, &frame, mAudioSeekTimeUs)) {
    mAudioQueue.Finish();
    return false;
  }
  mAudioSeekTimeUs = -1;

  
  if (frame.mSize == 0)
    return true;

  nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[frame.mSize/2] );
  memcpy(buffer.get(), frame.mData, frame.mSize);

  uint32_t frames = frame.mSize / (2 * frame.mAudioChannels);
  CheckedInt64 duration = FramesToUsecs(frames, frame.mAudioSampleRate);
  if (!duration.isValid()) {
    return false;
  }

  mAudioQueue.Push(new AudioData(pos,
                                 frame.mTimeUs,
                                 duration.value(),
                                 frames,
                                 buffer.forget(),
                                 frame.mAudioChannels));
  return true;
}

nsresult nsMediaPluginReader::Seek(int64_t aTarget, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  mVideoQueue.Erase();
  mAudioQueue.Erase();

  mAudioSeekTimeUs = mVideoSeekTimeUs = aTarget;

  return DecodeToTarget(aTarget);
}

nsresult nsMediaPluginReader::GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime)
{
  if (!mPlugin)
    return NS_OK;

  MediaResource* stream = mDecoder->GetResource();

  int64_t durationUs = 0;
  mPlugin->GetDuration(mPlugin, &durationUs);

  GetEstimatedBufferedTimeRanges(stream, durationUs, aBuffered);
  
  return NS_OK;
}
