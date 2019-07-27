





#include "MediaOmxReader.h"

#include "MediaDecoderStateMachine.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaResource.h"
#include "VideoUtils.h"
#include "MediaOmxDecoder.h"
#include "AbstractMediaDecoder.h"
#include "AudioChannelService.h"
#include "OmxDecoder.h"
#include "MPAPI.h"
#include "gfx2DGlue.h"
#include "MediaStreamSource.h"

#define MAX_DROPPED_FRAMES 25

#define MAX_VIDEO_DECODE_SECONDS 0.1

using namespace mozilla::gfx;
using namespace android;

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define DECODER_LOG(type, msg)
#endif

class MediaOmxReader::ProcessCachedDataTask : public Task
{
public:
  ProcessCachedDataTask(MediaOmxReader* aOmxReader, int64_t aOffset)
  : mOmxReader(aOmxReader),
    mOffset(aOffset)
  { }

  void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(mOmxReader.get());
    mOmxReader->ProcessCachedData(mOffset, false);
  }

private:
  nsRefPtr<MediaOmxReader> mOmxReader;
  int64_t                  mOffset;
};
















class MediaOmxReader::NotifyDataArrivedRunnable : public nsRunnable
{
public:
  NotifyDataArrivedRunnable(MediaOmxReader* aOmxReader,
                                     const char* aBuffer, uint64_t aLength,
                                     int64_t aOffset, uint64_t aFullLength)
  : mOmxReader(aOmxReader),
    mBuffer(aBuffer),
    mLength(aLength),
    mOffset(aOffset),
    mFullLength(aFullLength)
  {
    MOZ_ASSERT(mOmxReader.get());
    MOZ_ASSERT(mBuffer.get() || !mLength);
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

    NotifyDataArrived();

    return NS_OK;
  }

private:
  void NotifyDataArrived()
  {
    if (mOmxReader->IsShutdown()) {
      return;
    }
    const char* buffer = mBuffer.get();

    while (mLength) {
      uint32_t length = std::min<uint64_t>(mLength, UINT32_MAX);
      mOmxReader->NotifyDataArrived(buffer, length,
                                    mOffset);
      buffer  += length;
      mLength -= length;
      mOffset += length;
    }

    if (static_cast<uint64_t>(mOffset) < mFullLength) {
      
      
      
      XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
          new ProcessCachedDataTask(mOmxReader.get(), mOffset));
    }
  }

  nsRefPtr<MediaOmxReader>         mOmxReader;
  nsAutoArrayPtr<const char>       mBuffer;
  uint64_t                         mLength;
  int64_t                          mOffset;
  uint64_t                         mFullLength;
};

MediaOmxReader::MediaOmxReader(AbstractMediaDecoder *aDecoder)
  : MediaOmxCommonReader(aDecoder)
  , mShutdownMutex("MediaOmxReader.Shutdown")
  , mHasVideo(false)
  , mHasAudio(false)
  , mVideoSeekTimeUs(-1)
  , mAudioSeekTimeUs(-1)
  , mLastParserDuration(-1)
  , mSkipCount(0)
  , mUseParserDuration(false)
  , mIsShutdown(false)
  , mMP3FrameParser(-1)
  , mIsWaitingResources(false)
{
#ifdef PR_LOGGING
  if (!gMediaDecoderLog) {
    gMediaDecoderLog = PR_NewLogModule("MediaDecoder");
  }
#endif

  mAudioChannel = dom::AudioChannelService::GetDefaultAudioChannel();
}

MediaOmxReader::~MediaOmxReader()
{
}

nsresult MediaOmxReader::Init(MediaDecoderReader* aCloneDonor)
{
  return NS_OK;
}

already_AddRefed<AbstractMediaDecoder>
MediaOmxReader::SafeGetDecoder() {
  nsRefPtr<AbstractMediaDecoder> decoder;
  MutexAutoLock lock(mShutdownMutex);
  if (!mIsShutdown) {
    decoder = mDecoder;
  }
  return decoder.forget();
}

void MediaOmxReader::ReleaseDecoder()
{
  if (mOmxDecoder.get()) {
    mOmxDecoder->ReleaseDecoder();
  }
  mOmxDecoder.clear();
}

nsRefPtr<ShutdownPromise>
MediaOmxReader::Shutdown()
{
  {
    MutexAutoLock lock(mShutdownMutex);
    mIsShutdown = true;
  }

  nsRefPtr<ShutdownPromise> p = MediaDecoderReader::Shutdown();

  
  
  p->Then(AbstractThread::MainThread(), __func__, this, &MediaOmxReader::ReleaseDecoder, &MediaOmxReader::ReleaseDecoder);

  return p;
}

bool MediaOmxReader::IsWaitingMediaResources()
{
  return mIsWaitingResources;
}

void MediaOmxReader::UpdateIsWaitingMediaResources()
{
  if (mOmxDecoder.get()) {
    mIsWaitingResources = mOmxDecoder->IsWaitingMediaResources();
  } else {
    mIsWaitingResources = false;
  }
}

void MediaOmxReader::ReleaseMediaResources()
{
  ResetDecode();
  
  
  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->ClearCurrentFrame();
  }
  if (mOmxDecoder.get()) {
    mOmxDecoder->ReleaseMediaResources();
  }
}

nsresult MediaOmxReader::InitOmxDecoder()
{
  if (!mOmxDecoder.get()) {
    
    DataSource::RegisterDefaultSniffers();
    mDecoder->GetResource()->SetReadMode(MediaCacheStream::MODE_METADATA);

    sp<DataSource> dataSource = new MediaStreamSource(mDecoder->GetResource());
    dataSource->initCheck();

    mExtractor = MediaExtractor::Create(dataSource);
    if (!mExtractor.get()) {
      return NS_ERROR_FAILURE;
    }
    mOmxDecoder = new OmxDecoder(mDecoder->GetResource(), mDecoder);
    if (!mOmxDecoder->Init(mExtractor)) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

void MediaOmxReader::PreReadMetadata()
{
  UpdateIsWaitingMediaResources();
}

nsresult MediaOmxReader::ReadMetadata(MediaInfo* aInfo,
                                      MetadataTags** aTags)
{
  MOZ_ASSERT(OnTaskQueue());
  EnsureActive();

  *aTags = nullptr;

  
  nsresult rv = InitOmxDecoder();
  if (NS_FAILED(rv)) {
    return rv;
  }

  bool isMP3 = mDecoder->GetResource()->GetContentType().EqualsASCII(AUDIO_MP3);
  if (isMP3) {
    
    
    
    mMP3FrameParser.SetLength(mDecoder->GetResource()->GetLength());
    ProcessCachedData(0, true);
  }

  if (!mOmxDecoder->AllocateMediaResources()) {
    return NS_ERROR_FAILURE;
  }
  
  
  
  
  UpdateIsWaitingMediaResources();
  if (IsWaitingMediaResources()) {
    return NS_OK;
  }
  
  if (!mOmxDecoder->EnsureMetadata()) {
    return NS_ERROR_FAILURE;
  }

  if (isMP3 && mMP3FrameParser.IsMP3()) {
    int64_t duration = mMP3FrameParser.GetDuration();
    
    
    if (duration >= 0) {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mUseParserDuration = true;
      mLastParserDuration = duration;
      mDecoder->SetMediaDuration(mLastParserDuration);
    }
  } else {
    
    int64_t durationUs;
    mOmxDecoder->GetDuration(&durationUs);
    if (durationUs) {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mDecoder->SetMediaDuration(durationUs);
    }
  }

  if (mOmxDecoder->HasVideo()) {
    int32_t displayWidth, displayHeight, width, height;
    mOmxDecoder->GetVideoParameters(&displayWidth, &displayHeight,
                                    &width, &height);
    nsIntRect pictureRect(0, 0, width, height);

    
    
    nsIntSize displaySize(displayWidth, displayHeight);
    nsIntSize frameSize(width, height);
    if (!IsValidVideoRegion(frameSize, pictureRect, displaySize)) {
      return NS_ERROR_FAILURE;
    }

    
    mHasVideo = true;
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
    mHasAudio = true;
    mInfo.mAudio.mChannels = numChannels;
    mInfo.mAudio.mRate = sampleRate;
  }

 *aInfo = mInfo;

#ifdef MOZ_AUDIO_OFFLOAD
  CheckAudioOffload();
#endif

  return NS_OK;
}

bool
MediaOmxReader::IsMediaSeekable()
{
  
  return (mExtractor->flags() & MediaExtractor::CAN_SEEK);
}

bool MediaOmxReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                      int64_t aTimeThreshold)
{
  MOZ_ASSERT(OnTaskQueue());
  EnsureActive();

  
  
  AbstractMediaDecoder::AutoNotifyDecoded a(mDecoder);

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
    mVideoSeekTimeUs = -1;

    
    if (frame.mSize == 0 && !frame.mGraphicBuffer) {
      continue;
    }

    a.mParsed++;
    if (frame.mShouldSkip && mSkipCount < MAX_DROPPED_FRAMES) {
      mSkipCount++;
      continue;
    }

    mSkipCount = 0;

    aKeyframeSkip = false;

    IntRect picture = mPicture;
    if (frame.Y.mWidth != mInitialFrame.width ||
        frame.Y.mHeight != mInitialFrame.height) {

      
      
      
      picture.x = (mPicture.x * frame.Y.mWidth) / mInitialFrame.width;
      picture.y = (mPicture.y * frame.Y.mHeight) / mInitialFrame.height;
      picture.width = (frame.Y.mWidth * mPicture.width) / mInitialFrame.width;
      picture.height = (frame.Y.mHeight * mPicture.height) / mInitialFrame.height;
    }

    
    int64_t pos = mDecoder->GetResource()->Tell();

    nsRefPtr<VideoData> v;
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
                            1, 
                            b,
                            frame.mKeyFrame,
                            -1,
                            picture);
    } else {
      v = VideoData::Create(mInfo.mVideo,
                            mDecoder->GetImageContainer(),
                            pos,
                            frame.mTimeUs,
                            1, 
                            frame.mGraphicBuffer,
                            frame.mKeyFrame,
                            -1,
                            picture);
    }

    if (!v) {
      NS_WARNING("Unable to create VideoData");
      return false;
    }

    a.mDecoded++;
    NS_ASSERTION(a.mDecoded <= a.mParsed, "Expect to decode fewer frames than parsed in OMX decoder...");

    mVideoQueue.Push(v);

    break;
  }

  return true;
}

void MediaOmxReader::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsRefPtr<AbstractMediaDecoder> decoder = SafeGetDecoder();
  if (!decoder) { 
    return;
  }
  if (HasVideo()) {
    return;
  }
  if (!mMP3FrameParser.NeedsData()) {
    return;
  }

  mMP3FrameParser.Parse(aBuffer, aLength, aOffset);
  if (!mMP3FrameParser.IsMP3()) {
    return;
  }

  int64_t duration = mMP3FrameParser.GetDuration();
  if (duration != mLastParserDuration && mUseParserDuration) {
    ReentrantMonitorAutoEnter mon(decoder->GetReentrantMonitor());
    mLastParserDuration = duration;
    decoder->UpdateEstimatedMediaDuration(mLastParserDuration);
  }
}

bool MediaOmxReader::DecodeAudioData()
{
  MOZ_ASSERT(OnTaskQueue());
  EnsureActive();

  
  int64_t pos = mDecoder->GetResource()->Tell();

  
  MPAPI::AudioFrame source;
  if (!mOmxDecoder->ReadAudio(&source, mAudioSeekTimeUs)) {
    return false;
  }
  mAudioSeekTimeUs = -1;

  
  if (source.mSize == 0) {
    return true;
  }

  uint32_t frames = source.mSize / (source.mAudioChannels *
                                    sizeof(AudioDataValue));

  typedef AudioCompactor::NativeCopy OmxCopy;
  return mAudioCompactor.Push(pos,
                              source.mTimeUs,
                              source.mAudioSampleRate,
                              frames,
                              source.mAudioChannels,
                              OmxCopy(static_cast<uint8_t *>(source.mData),
                                      source.mSize,
                                      source.mAudioChannels));
}

nsRefPtr<MediaDecoderReader::SeekPromise>
MediaOmxReader::Seek(int64_t aTarget, int64_t aEndTime)
{
  MOZ_ASSERT(OnTaskQueue());
  EnsureActive();

  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container && container->GetImageContainer()) {
    container->GetImageContainer()->ClearAllImagesExceptFront();
  }

  if (mHasAudio && mHasVideo) {
    
    
    
    
    
    
    
    
    mVideoSeekTimeUs = aTarget;
    const VideoData* v = DecodeToFirstVideoData();
    mAudioSeekTimeUs = v ? v->mTime : aTarget;
  } else {
    mAudioSeekTimeUs = mVideoSeekTimeUs = aTarget;
  }

  return SeekPromise::CreateAndResolve(mAudioSeekTimeUs, __func__);
}

void MediaOmxReader::SetIdle() {
  if (!mOmxDecoder.get()) {
    return;
  }
  mOmxDecoder->Pause();
}

void MediaOmxReader::EnsureActive() {
  if (!mOmxDecoder.get()) {
    return;
  }
  DebugOnly<nsresult> result = mOmxDecoder->Play();
  NS_ASSERTION(result == NS_OK, "OmxDecoder should be in play state to continue decoding");
}

int64_t MediaOmxReader::ProcessCachedData(int64_t aOffset, bool aWaitForCompletion)
{
  
  nsRefPtr<AbstractMediaDecoder> decoder = SafeGetDecoder();
  if (!decoder) { 
    return -1;
  }
  
  
  
  
  static const int64_t sReadSize = 32 * 1024;

  NS_ASSERTION(!NS_IsMainThread(), "Should not be on main thread.");

  MOZ_ASSERT(decoder->GetResource());
  int64_t resourceLength = decoder->GetResource()->GetCachedDataEnd(0);
  NS_ENSURE_TRUE(resourceLength >= 0, -1);

  if (aOffset >= resourceLength) {
    return 0; 
  }

  int64_t bufferLength = std::min<int64_t>(resourceLength-aOffset, sReadSize);

  nsAutoArrayPtr<char> buffer(new char[bufferLength]);

  nsresult rv = decoder->GetResource()->ReadFromCache(buffer.get(),
                                                       aOffset, bufferLength);
  NS_ENSURE_SUCCESS(rv, -1);

  nsRefPtr<NotifyDataArrivedRunnable> runnable(
    new NotifyDataArrivedRunnable(this, buffer.forget(), bufferLength,
                                  aOffset, resourceLength));
  if (aWaitForCompletion) {
    rv = NS_DispatchToMainThread(runnable.get(), NS_DISPATCH_SYNC);
  } else {
    rv = NS_DispatchToMainThread(runnable.get());
  }
  NS_ENSURE_SUCCESS(rv, -1);

  return resourceLength - aOffset - bufferLength;
}

android::sp<android::MediaSource> MediaOmxReader::GetAudioOffloadTrack()
{
  if (!mOmxDecoder.get()) {
    return nullptr;
  }
  return mOmxDecoder->GetAudioOffloadTrack();
}

} 
