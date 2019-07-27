



#include "MediaEngineDefault.h"

#include "nsCOMPtr.h"
#include "mozilla/dom/File.h"
#include "nsILocalFile.h"
#include "Layers.h"
#include "ImageContainer.h"
#include "ImageTypes.h"
#include "prmem.h"
#include "nsContentUtils.h"

#include "nsIFilePicker.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#include "nsISupportsUtils.h"
#endif

#ifdef MOZ_WEBRTC
#include "YuvStamper.h"
#endif

#define AUDIO_RATE 16000
#define AUDIO_FRAME_LENGTH ((AUDIO_RATE * MediaEngine::DEFAULT_AUDIO_TIMER_MS) / 1000)
namespace mozilla {

using namespace mozilla::gfx;



static const int kFakeVideoTrackCount = 2;
static const int kFakeAudioTrackCount = 3;

NS_IMPL_ISUPPORTS(MediaEngineDefaultVideoSource, nsITimerCallback)




MediaEngineDefaultVideoSource::MediaEngineDefaultVideoSource()
  : MediaEngineVideoSource(kReleased)
  , mTimer(nullptr)
  , mMonitor("Fake video")
  , mCb(16), mCr(16)
{
  mImageContainer = layers::LayerManager::CreateImageContainer();
}

MediaEngineDefaultVideoSource::~MediaEngineDefaultVideoSource()
{}

void
MediaEngineDefaultVideoSource::GetName(nsAString& aName)
{
  aName.AssignLiteral(MOZ_UTF16("Default Video Device"));
  return;
}

void
MediaEngineDefaultVideoSource::GetUUID(nsAString& aUUID)
{
  aUUID.AssignLiteral(MOZ_UTF16("1041FCBD-3F12-4F7B-9E9B-1EC556DD5676"));
  return;
}

nsresult
MediaEngineDefaultVideoSource::Allocate(const VideoTrackConstraintsN &aConstraints,
                                        const MediaEnginePrefs &aPrefs)
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

  mOpts = aPrefs;
  mOpts.mWidth = mOpts.mWidth ? mOpts.mWidth : MediaEngine::DEFAULT_43_VIDEO_WIDTH;
  mOpts.mHeight = mOpts.mHeight ? mOpts.mHeight : MediaEngine::DEFAULT_43_VIDEO_HEIGHT;
  mState = kAllocated;
  return NS_OK;
}

nsresult
MediaEngineDefaultVideoSource::Deallocate()
{
  if (mState != kStopped && mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }
  mState = kReleased;
  return NS_OK;
}

static void AllocateSolidColorFrame(layers::PlanarYCbCrData& aData,
                                    int aWidth, int aHeight,
                                    int aY, int aCb, int aCr)
{
  MOZ_ASSERT(!(aWidth&1));
  MOZ_ASSERT(!(aHeight&1));
  
  int yLen = aWidth*aHeight;
  int cbLen = yLen>>2;
  int crLen = cbLen;
  uint8_t* frame = (uint8_t*) PR_Malloc(yLen+cbLen+crLen);
  memset(frame, aY, yLen);
  memset(frame+yLen, aCb, cbLen);
  memset(frame+yLen+cbLen, aCr, crLen);

  aData.mYChannel = frame;
  aData.mYSize = IntSize(aWidth, aHeight);
  aData.mYStride = aWidth;
  aData.mCbCrStride = aWidth>>1;
  aData.mCbChannel = frame + yLen;
  aData.mCrChannel = aData.mCbChannel + cbLen;
  aData.mCbCrSize = IntSize(aWidth>>1, aHeight>>1);
  aData.mPicX = 0;
  aData.mPicY = 0;
  aData.mPicSize = IntSize(aWidth, aHeight);
  aData.mStereoMode = StereoMode::MONO;
}

static void ReleaseFrame(layers::PlanarYCbCrData& aData)
{
  PR_Free(aData.mYChannel);
}

nsresult
MediaEngineDefaultVideoSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  if (mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!mTimer) {
    return NS_ERROR_FAILURE;
  }

  aStream->AddTrack(aID, 0, new VideoSegment());
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  if (mHasFakeTracks) {
    for (int i = 0; i < kFakeVideoTrackCount; ++i) {
      aStream->AddTrack(kTrackCount + i, 0, new VideoSegment());
    }
  }

  
  mTrackID = aID;

  
#if defined(MOZ_WIDGET_GONK) && defined(DEBUG)

  mTimer->InitWithCallback(this, (1000 / mOpts.mFPS)*10, nsITimer::TYPE_REPEATING_SLACK);
#else
  mTimer->InitWithCallback(this, 1000 / mOpts.mFPS, nsITimer::TYPE_REPEATING_SLACK);
#endif
  mState = kStarted;

  return NS_OK;
}

nsresult
MediaEngineDefaultVideoSource::Stop(SourceMediaStream *aSource, TrackID aID)
{
  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }
  if (!mTimer) {
    return NS_ERROR_FAILURE;
  }

  mTimer->Cancel();
  mTimer = nullptr;

  aSource->EndTrack(aID);
  if (mHasFakeTracks) {
    for (int i = 0; i < kFakeVideoTrackCount; ++i) {
      aSource->EndTrack(kTrackCount + i);
    }
  }
  aSource->Finish();

  mState = kStopped;
  return NS_OK;
}

NS_IMETHODIMP
MediaEngineDefaultVideoSource::Notify(nsITimer* aTimer)
{
  
  if (mCr <= 16) {
    if (mCb < 240) {
      mCb++;
    } else {
      mCr++;
    }
  } else if (mCb >= 240) {
    if (mCr < 240) {
      mCr++;
    } else {
      mCb--;
    }
  } else if (mCr >= 240) {
    if (mCb > 16) {
      mCb--;
    } else {
      mCr--;
    }
  } else {
    mCr--;
  }

  
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(ImageFormat::PLANAR_YCBCR);
  nsRefPtr<layers::PlanarYCbCrImage> ycbcr_image =
      static_cast<layers::PlanarYCbCrImage*>(image.get());
  layers::PlanarYCbCrData data;
  AllocateSolidColorFrame(data, mOpts.mWidth, mOpts.mHeight, 0x80, mCb, mCr);

#ifdef MOZ_WEBRTC
  uint64_t timestamp = PR_Now();
  YuvStamper::Encode(mOpts.mWidth, mOpts.mHeight, mOpts.mWidth,
		     data.mYChannel,
		     reinterpret_cast<unsigned char*>(&timestamp), sizeof(timestamp),
		     0, 0);
#endif

  ycbcr_image->SetData(data);
  
  ReleaseFrame(data);

  MonitorAutoLock lock(mMonitor);

  
  mImage = ycbcr_image.forget();

  return NS_OK;
}

void
MediaEngineDefaultVideoSource::NotifyPull(MediaStreamGraph* aGraph,
                                          SourceMediaStream *aSource,
                                          TrackID aID,
                                          StreamTime aDesiredTime)
{
  
  VideoSegment segment;
  MonitorAutoLock lock(mMonitor);
  if (mState != kStarted) {
    return;
  }

  
  nsRefPtr<layers::Image> image = mImage;
  StreamTime delta = aDesiredTime - aSource->GetEndOfAppendedData(aID);

  if (delta > 0) {
    
    IntSize size(image ? mOpts.mWidth : 0, image ? mOpts.mHeight : 0);
    segment.AppendFrame(image.forget(), delta, size);
    
    
    aSource->AppendToTrack(aID, &segment);
    
    if (mHasFakeTracks) {
      for (int i = 0; i < kFakeVideoTrackCount; ++i) {
        VideoSegment nullSegment;
        nullSegment.AppendNullData(delta);
        aSource->AppendToTrack(kTrackCount + i, &nullSegment);
      }
    }
  }
}


class SineWaveGenerator
{
public:
  static const int bytesPerSample = 2;
  static const int millisecondsPerSecond = 1000;
  static const int frequency = 1000;

  explicit SineWaveGenerator(int aSampleRate) :
    mTotalLength(aSampleRate / frequency),
    mReadLength(0) {
    MOZ_ASSERT(mTotalLength * frequency == aSampleRate);
    mAudioBuffer = new int16_t[mTotalLength];
    for(int i = 0; i < mTotalLength; i++) {
      
      mAudioBuffer[i] = (3276.8f * sin(2 * M_PI * i / mTotalLength));
    }
  }

  
  void generate(int16_t* aBuffer, int16_t aLengthInSamples) {
    int16_t remaining = aLengthInSamples;

    while (remaining) {
      int16_t processSamples = 0;

      if (mTotalLength - mReadLength >= remaining) {
        processSamples = remaining;
      } else {
        processSamples = mTotalLength - mReadLength;
      }
      memcpy(aBuffer, mAudioBuffer + mReadLength, processSamples * bytesPerSample);
      aBuffer += processSamples;
      mReadLength += processSamples;
      remaining -= processSamples;
      if (mReadLength == mTotalLength) {
        mReadLength = 0;
      }
    }
  }

private:
  nsAutoArrayPtr<int16_t> mAudioBuffer;
  int16_t mTotalLength;
  int16_t mReadLength;
};




NS_IMPL_ISUPPORTS(MediaEngineDefaultAudioSource, nsITimerCallback)

MediaEngineDefaultAudioSource::MediaEngineDefaultAudioSource()
  : MediaEngineAudioSource(kReleased)
  , mTimer(nullptr)
{
}

MediaEngineDefaultAudioSource::~MediaEngineDefaultAudioSource()
{}

void
MediaEngineDefaultAudioSource::GetName(nsAString& aName)
{
  aName.AssignLiteral(MOZ_UTF16("Default Audio Device"));
  return;
}

void
MediaEngineDefaultAudioSource::GetUUID(nsAString& aUUID)
{
  aUUID.AssignLiteral(MOZ_UTF16("B7CBD7C1-53EF-42F9-8353-73F61C70C092"));
  return;
}

nsresult
MediaEngineDefaultAudioSource::Allocate(const AudioTrackConstraintsN &aConstraints,
                                        const MediaEnginePrefs &aPrefs)
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

  mState = kAllocated;
  
  mSineGenerator = new SineWaveGenerator(AUDIO_RATE);
  return NS_OK;
}

nsresult
MediaEngineDefaultAudioSource::Deallocate()
{
  if (mState != kStopped && mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }
  mState = kReleased;
  return NS_OK;
}

nsresult
MediaEngineDefaultAudioSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  if (mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!mTimer) {
    return NS_ERROR_FAILURE;
  }

  mSource = aStream;

  
  AudioSegment* segment = new AudioSegment();
  mSource->AddAudioTrack(aID, AUDIO_RATE, 0, segment);

  if (mHasFakeTracks) {
    for (int i = 0; i < kFakeAudioTrackCount; ++i) {
      mSource->AddAudioTrack(kTrackCount + kFakeVideoTrackCount+i,
                             AUDIO_RATE, 0, new AudioSegment());
    }
  }

  
  mSource->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  
  mTrackID = aID;

  
#if defined(MOZ_WIDGET_GONK) && defined(DEBUG)

  mTimer->InitWithCallback(this, MediaEngine::DEFAULT_AUDIO_TIMER_MS*10,
                           nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
#else
  mTimer->InitWithCallback(this, MediaEngine::DEFAULT_AUDIO_TIMER_MS,
                           nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
#endif
  mState = kStarted;

  return NS_OK;
}

nsresult
MediaEngineDefaultAudioSource::Stop(SourceMediaStream *aSource, TrackID aID)
{
  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }
  if (!mTimer) {
    return NS_ERROR_FAILURE;
  }

  mTimer->Cancel();
  mTimer = nullptr;

  aSource->EndTrack(aID);
  if (mHasFakeTracks) {
    for (int i = 0; i < kFakeAudioTrackCount; ++i) {
      aSource->EndTrack(kTrackCount + kFakeVideoTrackCount+i);
    }
  }
  aSource->Finish();

  mState = kStopped;
  return NS_OK;
}

NS_IMETHODIMP
MediaEngineDefaultAudioSource::Notify(nsITimer* aTimer)
{
  AudioSegment segment;
  nsRefPtr<SharedBuffer> buffer = SharedBuffer::Create(AUDIO_FRAME_LENGTH * sizeof(int16_t));
  int16_t* dest = static_cast<int16_t*>(buffer->Data());

  mSineGenerator->generate(dest, AUDIO_FRAME_LENGTH);
  nsAutoTArray<const int16_t*,1> channels;
  channels.AppendElement(dest);
  segment.AppendFrames(buffer.forget(), channels, AUDIO_FRAME_LENGTH);
  mSource->AppendToTrack(mTrackID, &segment);

  
  if (mHasFakeTracks) {
    for (int i = 0; i < kFakeAudioTrackCount; ++i) {
      AudioSegment nullSegment;
      nullSegment.AppendNullData(AUDIO_FRAME_LENGTH);
      mSource->AppendToTrack(kTrackCount + kFakeVideoTrackCount+i, &nullSegment);
    }
  }
  return NS_OK;
}

void
MediaEngineDefault::EnumerateVideoDevices(MediaSourceType aMediaSource,
                                          nsTArray<nsRefPtr<MediaEngineVideoSource> >* aVSources) {
  MutexAutoLock lock(mMutex);

  
  if (aMediaSource != MediaSourceType::Camera) {
    return;
  }

  
  

  nsRefPtr<MediaEngineVideoSource> newSource = new MediaEngineDefaultVideoSource();
  newSource->SetHasFakeTracks(mHasFakeTracks);
  mVSources.AppendElement(newSource);
  aVSources->AppendElement(newSource);

  return;
}

void
MediaEngineDefault::EnumerateAudioDevices(MediaSourceType aMediaSource,
                                          nsTArray<nsRefPtr<MediaEngineAudioSource> >* aASources) {
  MutexAutoLock lock(mMutex);
  int32_t len = mASources.Length();

  

  for (int32_t i = 0; i < len; i++) {
    nsRefPtr<MediaEngineAudioSource> source = mASources.ElementAt(i);
    if (source->IsAvailable()) {
      aASources->AppendElement(source);
    }
  }

  
  if (aASources->Length() == 0) {
    nsRefPtr<MediaEngineAudioSource> newSource =
      new MediaEngineDefaultAudioSource();
    newSource->SetHasFakeTracks(mHasFakeTracks);
    mASources.AppendElement(newSource);
    aASources->AppendElement(newSource);
  }
  return;
}

} 
