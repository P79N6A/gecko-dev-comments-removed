




#include "MediaPluginReader.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaResource.h"
#include "VideoUtils.h"
#include "MediaPluginDecoder.h"
#include "MediaPluginHost.h"
#include "MediaDecoderStateMachine.h"
#include "ImageContainer.h"
#include "AbstractMediaDecoder.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {

typedef mozilla::layers::Image Image;

MediaPluginReader::MediaPluginReader(AbstractMediaDecoder *aDecoder,
                                     const nsACString& aContentType) :
  MediaDecoderReader(aDecoder),
  mType(aContentType),
  mPlugin(nullptr),
  mHasAudio(false),
  mHasVideo(false),
  mVideoSeekTimeUs(-1),
  mAudioSeekTimeUs(-1)
{
}

MediaPluginReader::~MediaPluginReader()
{
  ResetDecode();
}

nsresult MediaPluginReader::Init(MediaDecoderReader* aCloneDonor)
{
  return NS_OK;
}

nsresult MediaPluginReader::ReadMetadata(MediaInfo* aInfo,
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
    mDecoder->SetMediaDuration(durationUs);
  }

  if (mPlugin->HasVideo(mPlugin)) {
    int32_t width, height;
    mPlugin->GetVideoParameters(mPlugin, &width, &height);
    nsIntRect pictureRect(0, 0, width, height);

    
    
    nsIntSize displaySize(width, height);
    nsIntSize frameSize(width, height);
    if (!VideoInfo::ValidateVideoRegion(frameSize, pictureRect, displaySize)) {
      return NS_ERROR_FAILURE;
    }

    
    mHasVideo = mInfo.mVideo.mHasVideo = true;
    mInfo.mVideo.mDisplay = displaySize;
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
    mHasAudio = mInfo.mAudio.mHasAudio = true;
    mInfo.mAudio.mChannels = numChannels;
    mInfo.mAudio.mRate = sampleRate;
  }

 *aInfo = mInfo;
 *aTags = nullptr;
  return NS_OK;
}


nsresult MediaPluginReader::ResetDecode()
{
  if (mLastVideoFrame) {
    mLastVideoFrame = nullptr;
  }
  if (mPlugin) {
    GetMediaPluginHost()->DestroyDecoder(mPlugin);
    mPlugin = nullptr;
  }

  return NS_OK;
}

bool MediaPluginReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                         int64_t aTimeThreshold)
{
  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  
  if (mLastVideoFrame && mVideoSeekTimeUs != -1) {
    mLastVideoFrame = nullptr;
  }

  ImageBufferCallback bufferCallback(mDecoder->GetImageContainer());
  nsRefPtr<Image> currentImage;

  
  while (true) {
    MPAPI::VideoFrame frame;
    if (!mPlugin->ReadVideo(mPlugin, &frame, mVideoSeekTimeUs, &bufferCallback)) {
      
      
      
      if (mLastVideoFrame) {
        int64_t durationUs;
        mPlugin->GetDuration(mPlugin, &durationUs);
        if (durationUs < mLastVideoFrame->mTime) {
          durationUs = 0;
        }
        mVideoQueue.Push(VideoData::ShallowCopyUpdateDuration(mLastVideoFrame,
                                                              durationUs));
        mLastVideoFrame = nullptr;
      }
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

    if (frame.mSize == 0)
      return true;

    currentImage = bufferCallback.GetImage();
    int64_t pos = mDecoder->GetResource()->Tell();
    nsIntRect picture = mPicture;

    nsAutoPtr<VideoData> v;
    if (currentImage) {
      gfx::IntSize frameSize = currentImage->GetSize();
      if (frameSize.width != mInitialFrame.width ||
          frameSize.height != mInitialFrame.height) {
        
        
        
        picture.x = (mPicture.x * frameSize.width) / mInitialFrame.width;
        picture.y = (mPicture.y * frameSize.height) / mInitialFrame.height;
        picture.width = (frameSize.width * mPicture.width) / mInitialFrame.width;
        picture.height = (frameSize.height * mPicture.height) / mInitialFrame.height;
      }

      v = VideoData::CreateFromImage(mInfo.mVideo,
                                     mDecoder->GetImageContainer(),
                                     pos,
                                     frame.mTimeUs,
                                     1, 
                                     currentImage,
                                     frame.mKeyFrame,
                                     -1,
                                     picture);
    } else {
      
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

      if (frame.Y.mWidth != mInitialFrame.width ||
          frame.Y.mHeight != mInitialFrame.height) {

        
        
        
        picture.x = (mPicture.x * frame.Y.mWidth) / mInitialFrame.width;
        picture.y = (mPicture.y * frame.Y.mHeight) / mInitialFrame.height;
        picture.width = (frame.Y.mWidth * mPicture.width) / mInitialFrame.width;
        picture.height = (frame.Y.mHeight * mPicture.height) / mInitialFrame.height;
      }

      
      v = VideoData::Create(mInfo.mVideo,
                            mDecoder->GetImageContainer(),
                            pos,
                            frame.mTimeUs,
                            1, 
                            b,
                            frame.mKeyFrame,
                            -1,
                            picture);
    }
 
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

    
    
    
    int64_t duration = v->mTime - mLastVideoFrame->mTime;
    mLastVideoFrame = VideoData::ShallowCopyUpdateDuration(mLastVideoFrame, duration);

    
    
    
    if (mLastVideoFrame->GetEndTime() < aTimeThreshold) {
      mLastVideoFrame = nullptr;
      continue;
    }

    mVideoQueue.Push(mLastVideoFrame.forget());

    
    mLastVideoFrame = v;

    break;
  }

  return true;
}

bool MediaPluginReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  int64_t pos = mDecoder->GetResource()->Tell();

  
  MPAPI::AudioFrame frame;
  if (!mPlugin->ReadAudio(mPlugin, &frame, mAudioSeekTimeUs)) {
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

nsresult MediaPluginReader::Seek(int64_t aTarget, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  mVideoQueue.Erase();
  mAudioQueue.Erase();

  mAudioSeekTimeUs = mVideoSeekTimeUs = aTarget;

  return DecodeToTarget(aTarget);
}

MediaPluginReader::ImageBufferCallback::ImageBufferCallback(mozilla::layers::ImageContainer *aImageContainer) :
  mImageContainer(aImageContainer)
{
}

void *
MediaPluginReader::ImageBufferCallback::operator()(size_t aWidth, size_t aHeight,
                                                     MPAPI::ColorFormat aColorFormat)
{
  if (!mImageContainer) {
    NS_WARNING("No image container to construct an image");
    return nullptr;
  }

  nsRefPtr<Image> rgbImage;
  switch(aColorFormat) {
    case MPAPI::RGB565:
      rgbImage = mozilla::layers::CreateSharedRGBImage(mImageContainer,
                                                       nsIntSize(aWidth, aHeight),
                                                       gfxImageFormatRGB16_565);
      if (!rgbImage) {
        NS_WARNING("Could not create rgb image");
        return nullptr;
      }

      mImage = rgbImage;
      return rgbImage->AsSharedImage()->GetBuffer();
    case MPAPI::YCbCr:
    default:
      NS_NOTREACHED("Color format not supported");
      return nullptr;
  }
}

already_AddRefed<Image>
MediaPluginReader::ImageBufferCallback::GetImage()
{
  return mImage.forget();
}

} 
