



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

NS_IMPL_THREADSAFE_ISUPPORTS1(MediaEngineDefaultVideoSource, nsITimerCallback)




MediaEngineDefaultVideoSource::MediaEngineDefaultVideoSource(int32_t aWidth,
                                                             int32_t aHeight,
                                                             int32_t aFPS)
  : mTimer(nullptr)
{
  mOpts.mWidth = aWidth;
  mOpts.mHeight = aHeight;
  mOpts.mMaxFPS = aFPS;
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
MediaEngineDefaultVideoSource::Allocate()
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

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

const MediaEngineVideoOptions *
MediaEngineDefaultVideoSource::GetOptions()
{
  return &mOpts;
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

  mSource = aStream;

  
  ImageFormat format = PLANAR_YCBCR;
  mImageContainer = layers::LayerManager::CreateImageContainer();

  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(&format, 1);
  mImage = static_cast<layers::PlanarYCbCrImage*>(image.get());

  layers::PlanarYCbCrImage::Data data;
  
  mCb = 16;
  mCr = 16;
  AllocateSolidColorFrame(data, mOpts.mWidth, mOpts.mHeight, 0x80, mCb, mCr);
  
  mImage->SetData(data);
  ReleaseFrame(data);

  
  VideoSegment *segment = new VideoSegment();
  segment->AppendFrame(image.forget(), USECS_PER_S / mOpts.mMaxFPS,
                       gfxIntSize(mOpts.mWidth, mOpts.mHeight));
  mSource->AddTrack(aID, VIDEO_RATE, 0, segment);

  
  mSource->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  
  mTrackID = aID;

  
  mTimer->InitWithCallback(this, 1000 / mOpts.mMaxFPS, nsITimer::TYPE_REPEATING_SLACK);
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

  
  VideoSegment segment;
  segment.AppendFrame(ycbcr_image.forget(), USECS_PER_S / mOpts.mMaxFPS,
                      gfxIntSize(mOpts.mWidth, mOpts.mHeight));
  mSource->AppendToTrack(mTrackID, &segment);

  return NS_OK;
}

void
MediaEngineDefaultVideoSource::NotifyPull(MediaStreamGraph* aGraph,
                                          StreamTime aDesiredTime)
{
  
}





NS_IMPL_THREADSAFE_ISUPPORTS1(MediaEngineDefaultAudioSource, nsITimerCallback)

MediaEngineDefaultAudioSource::MediaEngineDefaultAudioSource()
  : mTimer(nullptr)
{
  mState = kReleased;
}

MediaEngineDefaultAudioSource::~MediaEngineDefaultAudioSource()
{}

void
MediaEngineDefaultAudioSource::NotifyPull(MediaStreamGraph* aGraph,
                                          StreamTime aDesiredTime)
{
  
}

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
MediaEngineDefaultAudioSource::Allocate()
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

  
  mTimer->InitWithCallback(this, 1000 / MediaEngineDefaultVideoSource::DEFAULT_VIDEO_FPS,
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
  segment.InsertNullDataAtStart(AUDIO_RATE/100); 

  mSource->AppendToTrack(mTrackID, &segment);

  return NS_OK;
}

void
MediaEngineDefault::EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >* aVSources) {
  MutexAutoLock lock(mMutex);
  int32_t found = false;
  int32_t len = mVSources.Length();

  int32_t width  = MediaEngineDefaultVideoSource::DEFAULT_VIDEO_WIDTH;
  int32_t height = MediaEngineDefaultVideoSource::DEFAULT_VIDEO_HEIGHT;
  int32_t fps    = MediaEngineDefaultVideoSource::DEFAULT_VIDEO_FPS;

  
  
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

    if (branch) {
      
      branch->GetIntPref("media.navigator.video.default_width", &width);
      branch->GetIntPref("media.navigator.video.default_height", &height);
      branch->GetIntPref("media.navigator.video.default_fps", &fps);
    }
  }

  for (int32_t i = 0; i < len; i++) {
    nsRefPtr<MediaEngineVideoSource> source = mVSources.ElementAt(i);
    aVSources->AppendElement(source);
    const MediaEngineVideoOptions *opts = source->GetOptions();
    if (source->IsAvailable() &&
        opts->mWidth == width && opts->mHeight == height && opts->mMaxFPS == fps) {
      found = true;
    }
  }

  
  if (!found) {
    nsRefPtr<MediaEngineVideoSource> newSource =
      new MediaEngineDefaultVideoSource(width, height, fps);
    mVSources.AppendElement(newSource);
    aVSources->AppendElement(newSource);
  }
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
