





#include "MediaOmxReader.h"

#include "MediaDecoderStateMachine.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaResource.h"
#include "VideoUtils.h"
#include "MediaOmxDecoder.h"
#include "AbstractMediaDecoder.h"
#include "OmxDecoder.h"
#include "MPAPI.h"

#define MAX_DROPPED_FRAMES 25

#define MAX_VIDEO_DECODE_SECONDS 3.0

using namespace android;

namespace mozilla {

MediaOmxReader::MediaOmxReader(AbstractMediaDecoder *aDecoder) :
  MediaDecoderReader(aDecoder),
  mHasVideo(false),
  mHasAudio(false),
  mVideoSeekTimeUs(-1),
  mAudioSeekTimeUs(-1),
  mSkipCount(0)
{
}

MediaOmxReader::~MediaOmxReader()
{
  ResetDecode();
  mOmxDecoder.clear();
}

nsresult MediaOmxReader::Init(MediaDecoderReader* aCloneDonor)
{
  return NS_OK;
}

bool MediaOmxReader::IsWaitingMediaResources()
{
  if (!mOmxDecoder.get()) {
    return false;
  }
  return mOmxDecoder->IsWaitingMediaResources();
}

bool MediaOmxReader::IsDormantNeeded()
{
  if (!mOmxDecoder.get()) {
    return false;
  }
  return mOmxDecoder->IsDormantNeeded();
}

void MediaOmxReader::ReleaseMediaResources()
{
  ResetDecode();
  if (mOmxDecoder.get()) {
    mOmxDecoder->ReleaseMediaResources();
  }
}

void MediaOmxReader::ReleaseDecoder()
{
  if (mOmxDecoder.get()) {
    mOmxDecoder->ReleaseDecoder();
  }
}

nsresult MediaOmxReader::InitOmxDecoder()
{
  if (!mOmxDecoder.get()) {
    
    DataSource::RegisterDefaultSniffers();
    mDecoder->GetResource()->SetReadMode(MediaCacheStream::MODE_METADATA);

    sp<DataSource> dataSource = new MediaStreamSource(mDecoder->GetResource(), mDecoder);
    dataSource->initCheck();

    sp<MediaExtractor> extractor = MediaExtractor::Create(dataSource);

    mOmxDecoder = new OmxDecoder(mDecoder->GetResource(), mDecoder);
    if (!mOmxDecoder->Init(extractor)) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

nsresult MediaOmxReader::ReadMetadata(MediaInfo* aInfo,
                                      MetadataTags** aTags)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  *aTags = nullptr;

  
  nsresult rv = InitOmxDecoder();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!mOmxDecoder->TryLoad()) {
    return NS_ERROR_FAILURE;
  }

  if (IsWaitingMediaResources()) {
    return NS_OK;
  }

  
  int64_t durationUs;
  mOmxDecoder->GetDuration(&durationUs);
  if (durationUs) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(durationUs);
  }

  if (mOmxDecoder->HasVideo()) {
    int32_t width, height;
    mOmxDecoder->GetVideoParameters(&width, &height);
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

  if (mOmxDecoder->HasAudio()) {
    int32_t numChannels, sampleRate;
    mOmxDecoder->GetAudioParameters(&numChannels, &sampleRate);
    mHasAudio = mInfo.mAudio.mHasAudio = true;
    mInfo.mAudio.mChannels = numChannels;
    mInfo.mAudio.mRate = sampleRate;
  }

 *aInfo = mInfo;

  return NS_OK;
}


nsresult MediaOmxReader::ResetDecode()
{
  MediaDecoderReader::ResetDecode();

  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->ClearCurrentFrame();
  }
  return NS_OK;
}

bool MediaOmxReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                      int64_t aTimeThreshold)
{
  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  bool doSeek = mVideoSeekTimeUs != -1;
  if (doSeek) {
    aTimeThreshold = mVideoSeekTimeUs;
  }

  TimeStamp start = TimeStamp::Now();

  
  while ((TimeStamp::Now() - start) < TimeDuration::FromSeconds(MAX_VIDEO_DECODE_SECONDS)) {
    MPAPI::VideoFrame frame;
    frame.mGraphicBuffer = nullptr;
    frame.mShouldSkip = false;
    if (!mOmxDecoder->ReadVideo(&frame, aTimeThreshold, aKeyframeSkip, doSeek)) {
      return false;
    }
    doSeek = false;

    
    if (frame.mSize == 0 && !frame.mGraphicBuffer) {
      continue;
    }

    parsed++;
    if (frame.mShouldSkip && mSkipCount < MAX_DROPPED_FRAMES) {
      mSkipCount++;
      continue;
    }

    mSkipCount = 0;

    mVideoSeekTimeUs = -1;
    aKeyframeSkip = false;

    nsIntRect picture = mPicture;
    if (frame.Y.mWidth != mInitialFrame.width ||
        frame.Y.mHeight != mInitialFrame.height) {

      
      
      
      picture.x = (mPicture.x * frame.Y.mWidth) / mInitialFrame.width;
      picture.y = (mPicture.y * frame.Y.mHeight) / mInitialFrame.height;
      picture.width = (frame.Y.mWidth * mPicture.width) / mInitialFrame.width;
      picture.height = (frame.Y.mHeight * mPicture.height) / mInitialFrame.height;
    }

    
    int64_t pos = mDecoder->GetResource()->Tell();

    VideoData *v;
    if (!frame.mGraphicBuffer) {

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

      v = VideoData::Create(mInfo.mVideo,
                            mDecoder->GetImageContainer(),
                            pos,
                            frame.mTimeUs,
                            frame.mTimeUs+1, 
                            b,
                            frame.mKeyFrame,
                            -1,
                            picture);
    } else {
      v = VideoData::Create(mInfo.mVideo,
                            mDecoder->GetImageContainer(),
                            pos,
                            frame.mTimeUs,
                            frame.mTimeUs+1, 
                            frame.mGraphicBuffer,
                            frame.mKeyFrame,
                            -1,
                            picture);
    }

    if (!v) {
      NS_WARNING("Unable to create VideoData");
      return false;
    }

    decoded++;
    NS_ASSERTION(decoded <= parsed, "Expect to decode fewer frames than parsed in MediaPlugin...");

    mVideoQueue.Push(v);

    break;
  }

  return true;
}

void MediaOmxReader::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  android::OmxDecoder *omxDecoder = mOmxDecoder.get();

  if (omxDecoder) {
    omxDecoder->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
}

bool MediaOmxReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  int64_t pos = mDecoder->GetResource()->Tell();

  
  MPAPI::AudioFrame frame;
  if (!mOmxDecoder->ReadAudio(&frame, mAudioSeekTimeUs)) {
    return false;
  }
  mAudioSeekTimeUs = -1;

  
  if (frame.mSize == 0) {
    return true;
  }

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

nsresult MediaOmxReader::Seek(int64_t aTarget, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  mVideoQueue.Reset();
  mAudioQueue.Reset();

  mAudioSeekTimeUs = mVideoSeekTimeUs = aTarget;

  return DecodeToTarget(aTarget);
}

static uint64_t BytesToTime(int64_t offset, uint64_t length, uint64_t durationUs) {
  double perc = double(offset) / double(length);
  if (perc > 1.0)
    perc = 1.0;
  return uint64_t(double(durationUs) * perc);
}

nsresult MediaOmxReader::GetBuffered(mozilla::dom::TimeRanges* aBuffered, int64_t aStartTime)
{
  if (!mOmxDecoder.get())
    return NS_OK;

  MediaResource* stream = mOmxDecoder->GetResource();

  int64_t durationUs = 0;
  mOmxDecoder->GetDuration(&durationUs);

  
  if (!durationUs)
    return NS_OK;

  
  if (stream->IsDataCachedToEndOfResource(0)) {
    aBuffered->Add(0, durationUs);
    return NS_OK;
  }

  int64_t totalBytes = stream->GetLength();

  
  
  
  if (totalBytes == -1)
    return NS_OK;

  int64_t startOffset = stream->GetNextCachedData(0);
  while (startOffset >= 0) {
    int64_t endOffset = stream->GetCachedDataEnd(startOffset);
    
    NS_ASSERTION(startOffset >= 0, "Integer underflow in GetBuffered");
    NS_ASSERTION(endOffset >= 0, "Integer underflow in GetBuffered");

    uint64_t startUs = BytesToTime(startOffset, totalBytes, durationUs);
    uint64_t endUs = BytesToTime(endOffset, totalBytes, durationUs);
    if (startUs != endUs) {
      aBuffered->Add((double)startUs / USECS_PER_S, (double)endUs / USECS_PER_S);
    }
    startOffset = stream->GetNextCachedData(endOffset);
  }
  return NS_OK;
}

void MediaOmxReader::OnDecodeThreadFinish() {
  if (mOmxDecoder.get()) {
    mOmxDecoder->Pause();
  }
}

void MediaOmxReader::OnDecodeThreadStart() {
  if (mOmxDecoder.get()) {
    DebugOnly<nsresult> result = mOmxDecoder->Play();
    NS_ASSERTION(result == NS_OK, "OmxDecoder should be in play state to continue decoding");
  }
}

} 

