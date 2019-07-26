



#include "MediaEngineDefault.h"

#include "nsCOMPtr.h"
#include "nsDOMFile.h"
#include "nsILocalFile.h"
#include "Layers.h"
#include "ImageContainer.h"
#include "ImageTypes.h"
#include "prmem.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#include "nsISupportsUtils.h"
#endif

#define VIDEO_RATE USECS_PER_S
#define AUDIO_RATE 16000

namespace mozilla {

NS_IMPL_ISUPPORTS1(MediaEngineDefaultVideoSource, nsITimerCallback)




MediaEngineDefaultVideoSource::MediaEngineDefaultVideoSource()
  : mTimer(nullptr), mMonitor("Fake video")
{
  mImageContainer = layers::LayerManager::CreateImageContainer();
  mState = kReleased;
}

MediaEngineDefaultVideoSource::~MediaEngineDefaultVideoSource()
{}

void
MediaEngineDefaultVideoSource::GetName(nsAString& aName)
{
  aName.Assign(NS_LITERAL_STRING("Default Video Device"));
  return;
}

void
MediaEngineDefaultVideoSource::GetUUID(nsAString& aUUID)
{
  aUUID.Assign(NS_LITERAL_STRING("1041FCBD-3F12-4F7B-9E9B-1EC556DD5676"));
  return;
}

nsresult
MediaEngineDefaultVideoSource::Allocate(const MediaEnginePrefs &aPrefs)
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

  mOpts = aPrefs;
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

static void AllocateSolidColorFrame(layers::PlanarYCbCrImage::Data& aData,
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
  aData.mYSize = gfxIntSize(aWidth, aHeight);
  aData.mYStride = aWidth;
  aData.mCbCrStride = aWidth>>1;
  aData.mCbChannel = frame + yLen;
  aData.mCrChannel = aData.mCbChannel + cbLen;
  aData.mCbCrSize = gfxIntSize(aWidth>>1, aHeight>>1);
  aData.mPicX = 0;
  aData.mPicY = 0;
  aData.mPicSize = gfxIntSize(aWidth, aHeight);
  aData.mStereoMode = STEREO_MODE_MONO;
}

static void ReleaseFrame(layers::PlanarYCbCrImage::Data& aData)
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

  aStream->AddTrack(aID, VIDEO_RATE, 0, new VideoSegment());
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  
  mTrackID = aID;

  
  mTimer->InitWithCallback(this, 1000 / mOpts.mFPS, nsITimer::TYPE_REPEATING_SLACK);
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
  mTimer = NULL;

  aSource->EndTrack(aID);
  aSource->Finish();

  mState = kStopped;
  return NS_OK;
}

nsresult
MediaEngineDefaultVideoSource::Snapshot(uint32_t aDuration, nsIDOMFile** aFile)
{
  *aFile = nullptr;

#ifndef MOZ_WIDGET_ANDROID
  return NS_ERROR_NOT_IMPLEMENTED;
#else
  if (!AndroidBridge::Bridge()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoString filePath;
  AndroidBridge::Bridge()->ShowFilePickerForMimeType(filePath, NS_LITERAL_STRING("image/*"));

  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_NewLocalFile(filePath, false, getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aFile = new nsDOMFileFile(file));
  return NS_OK;
#endif
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

  
  ImageFormat format = PLANAR_YCBCR;
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(&format, 1);
  nsRefPtr<layers::PlanarYCbCrImage> ycbcr_image =
      static_cast<layers::PlanarYCbCrImage*>(image.get());
  layers::PlanarYCbCrImage::Data data;
  AllocateSolidColorFrame(data, mOpts.mWidth, mOpts.mHeight, 0x80, mCb, mCr);
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
                                          StreamTime aDesiredTime,
                                          TrackTicks &aLastEndTime)
{
  
  VideoSegment segment;
  MonitorAutoLock lock(mMonitor);
  if (mState != kStarted) {
    return;
  }

  
  nsRefPtr<layers::Image> image = mImage;
  TrackTicks target = TimeToTicksRoundUp(USECS_PER_S, aDesiredTime);
  TrackTicks delta = target - aLastEndTime;

  if (delta > 0) {
    
    if (image) {
      segment.AppendFrame(image.forget(), delta,
                          gfxIntSize(mOpts.mWidth, mOpts.mHeight));
    } else {
      segment.AppendFrame(nullptr, delta, gfxIntSize(0,0));
    }
    
    
    if (aSource->AppendToTrack(aID, &segment)) {
      aLastEndTime = target;
    }
  }
}





NS_IMPL_ISUPPORTS1(MediaEngineDefaultAudioSource, nsITimerCallback)

MediaEngineDefaultAudioSource::MediaEngineDefaultAudioSource()
  : mTimer(nullptr)
{
  mState = kReleased;
}

MediaEngineDefaultAudioSource::~MediaEngineDefaultAudioSource()
{}

void
MediaEngineDefaultAudioSource::GetName(nsAString& aName)
{
  aName.Assign(NS_LITERAL_STRING("Default Audio Device"));
  return;
}

void
MediaEngineDefaultAudioSource::GetUUID(nsAString& aUUID)
{
  aUUID.Assign(NS_LITERAL_STRING("B7CBD7C1-53EF-42F9-8353-73F61C70C092"));
  return;
}

nsresult
MediaEngineDefaultAudioSource::Allocate(const MediaEnginePrefs &aPrefs)
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

  mState = kAllocated;
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
  mSource->AddTrack(aID, AUDIO_RATE, 0, segment);

  
  mSource->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  
  mTrackID = aID;

  
  mTimer->InitWithCallback(this, MediaEngine::DEFAULT_AUDIO_TIMER_MS,
                           nsITimer::TYPE_REPEATING_SLACK);
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
  mTimer = NULL;

  aSource->EndTrack(aID);
  aSource->Finish();

  mState = kStopped;
  return NS_OK;
}

nsresult
MediaEngineDefaultAudioSource::Snapshot(uint32_t aDuration, nsIDOMFile** aFile)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MediaEngineDefaultAudioSource::Notify(nsITimer* aTimer)
{
  AudioSegment segment;

  
  segment.InsertNullDataAtStart((AUDIO_RATE * MediaEngine::DEFAULT_AUDIO_TIMER_MS) / 1000);

  mSource->AppendToTrack(mTrackID, &segment);

  return NS_OK;
}

void
MediaEngineDefault::EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >* aVSources) {
  MutexAutoLock lock(mMutex);

  
  

  nsRefPtr<MediaEngineVideoSource> newSource = new MediaEngineDefaultVideoSource();
  mVSources.AppendElement(newSource);
  aVSources->AppendElement(newSource);

  return;
}

void
MediaEngineDefault::EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >* aASources) {
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
    mASources.AppendElement(newSource);
    aASources->AppendElement(newSource);
  }
  return;
}

} 
