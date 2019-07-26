















#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include "base/basictypes.h"
#include "camera/CameraParameters.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "nsMemory.h"
#include "nsThread.h"
#include <media/MediaProfiles.h>
#include "mozilla/FileUtils.h"
#include "mozilla/Services.h"
#include "nsAlgorithm.h"
#include <media/mediaplayer.h>
#include "nsPrintfCString.h"
#include "nsIObserverService.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "DOMCameraManager.h"
#include "GonkCameraHwMgr.h"
#include "DOMCameraCapabilities.h"
#include "DOMCameraControl.h"
#include "GonkRecorderProfiles.h"
#include "GonkCameraControl.h"
#include "CameraCommon.h"
#include "DeviceStorageFileDescriptor.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;
using namespace android;
using mozilla::gfx::IntSize;









#define FORCE_PREVIEW_FORMAT_YUV420SP   1

#define RETURN_IF_NO_CAMERA_HW()                                          \
  do {                                                                    \
    if (!mCameraHw.get()) {                                               \
      DOM_CAMERA_LOGE("%s:%d : mCameraHw is null\n", __func__, __LINE__); \
      return NS_ERROR_NOT_AVAILABLE;                                      \
    }                                                                     \
  } while(0)

static const char* getKeyText(uint32_t aKey)
{
  switch (aKey) {
    case CAMERA_PARAM_EFFECT:
      return CameraParameters::KEY_EFFECT;
    case CAMERA_PARAM_WHITEBALANCE:
      return CameraParameters::KEY_WHITE_BALANCE;
    case CAMERA_PARAM_SCENEMODE:
      return CameraParameters::KEY_SCENE_MODE;
    case CAMERA_PARAM_FLASHMODE:
      return CameraParameters::KEY_FLASH_MODE;
    case CAMERA_PARAM_FOCUSMODE:
      return CameraParameters::KEY_FOCUS_MODE;
    case CAMERA_PARAM_ZOOM:
      return CameraParameters::KEY_ZOOM;
    case CAMERA_PARAM_METERINGAREAS:
      return CameraParameters::KEY_METERING_AREAS;
    case CAMERA_PARAM_FOCUSAREAS:
      return CameraParameters::KEY_FOCUS_AREAS;
    case CAMERA_PARAM_FOCALLENGTH:
      return CameraParameters::KEY_FOCAL_LENGTH;
    case CAMERA_PARAM_FOCUSDISTANCENEAR:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_FOCUSDISTANCEFAR:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      return CameraParameters::KEY_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_PICTURESIZE:
      return CameraParameters::KEY_PICTURE_SIZE;
    case CAMERA_PARAM_THUMBNAILQUALITY:
      return CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY;

    case CAMERA_PARAM_SUPPORTED_PREVIEWSIZES:
      return CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES;
    case CAMERA_PARAM_SUPPORTED_VIDEOSIZES:
      return CameraParameters::KEY_SUPPORTED_VIDEO_SIZES;
    case CAMERA_PARAM_SUPPORTED_PICTURESIZES:
      return CameraParameters::KEY_SUPPORTED_PICTURE_SIZES;
    case CAMERA_PARAM_SUPPORTED_PICTUREFORMATS:
      return CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS;
    case CAMERA_PARAM_SUPPORTED_WHITEBALANCES:
      return CameraParameters::KEY_SUPPORTED_WHITE_BALANCE;
    case CAMERA_PARAM_SUPPORTED_SCENEMODES:
      return CameraParameters::KEY_SUPPORTED_SCENE_MODES;
    case CAMERA_PARAM_SUPPORTED_EFFECTS:
      return CameraParameters::KEY_SUPPORTED_EFFECTS;
    case CAMERA_PARAM_SUPPORTED_FLASHMODES:
      return CameraParameters::KEY_SUPPORTED_FLASH_MODES;
    case CAMERA_PARAM_SUPPORTED_FOCUSMODES:
      return CameraParameters::KEY_SUPPORTED_FOCUS_MODES;
    case CAMERA_PARAM_SUPPORTED_MAXFOCUSAREAS:
      return CameraParameters::KEY_MAX_NUM_FOCUS_AREAS;
    case CAMERA_PARAM_SUPPORTED_MAXMETERINGAREAS:
      return CameraParameters::KEY_MAX_NUM_METERING_AREAS;
    case CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION:
      return CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_MAXEXPOSURECOMPENSATION:
      return CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_EXPOSURECOMPENSATIONSTEP:
      return CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP;
    case CAMERA_PARAM_SUPPORTED_ZOOM:
      return CameraParameters::KEY_ZOOM_SUPPORTED;
    case CAMERA_PARAM_SUPPORTED_ZOOMRATIOS:
      return CameraParameters::KEY_ZOOM_RATIOS;
    case CAMERA_PARAM_SUPPORTED_JPEG_THUMBNAIL_SIZES:
      return CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES;
    default:
      return nullptr;
  }
}


nsDOMCameraControl::nsDOMCameraControl(uint32_t aCameraId, nsIThread* aCameraThread, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, nsPIDOMWindow* aWindow)
  : mDOMCapabilities(nullptr), mWindow(aWindow)
{
  MOZ_ASSERT(aWindow, "shouldn't be created with null window!");
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  SetIsDOMBinding();

  










  NS_ADDREF_THIS();
  nsRefPtr<nsGonkCameraControl> control = new nsGonkCameraControl(aCameraId, aCameraThread, this, onSuccess, onError, aWindow->WindowID());
  control->DispatchInit(this, onSuccess, onError, aWindow->WindowID());
  mCameraControl = control;
}




class InitGonkCameraControl : public nsRunnable
{
public:
  InitGonkCameraControl(nsGonkCameraControl* aCameraControl, nsDOMCameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, uint64_t aWindowId)
    : mCameraControl(aCameraControl)
    , mDOMCameraControl(aDOMCameraControl)
    , mOnSuccessCb(new nsMainThreadPtrHolder<nsICameraGetCameraCallback>(onSuccess))
    , mOnErrorCb(new nsMainThreadPtrHolder<nsICameraErrorCallback>(onError))
    , mWindowId(aWindowId)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  ~InitGonkCameraControl()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    nsresult rv = mCameraControl->Init();
    return mDOMCameraControl->Result(rv, mOnSuccessCb, mOnErrorCb, mWindowId);
  }

  nsRefPtr<nsGonkCameraControl> mCameraControl;
  
  nsDOMCameraControl* mDOMCameraControl;
  nsMainThreadPtrHandle<nsICameraGetCameraCallback> mOnSuccessCb;
  nsMainThreadPtrHandle<nsICameraErrorCallback> mOnErrorCb;
  uint64_t mWindowId;
};


nsGonkCameraControl::nsGonkCameraControl(uint32_t aCameraId, nsIThread* aCameraThread, nsDOMCameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, uint64_t aWindowId)
  : CameraControlImpl(aCameraId, aCameraThread, aWindowId)
  , mExposureCompensationMin(0.0)
  , mExposureCompensationStep(0.0)
  , mDeferConfigUpdate(false)
  , mWidth(0)
  , mHeight(0)
  , mLastPictureWidth(0)
  , mLastPictureHeight(0)
  , mLastThumbnailWidth(0)
  , mLastThumbnailHeight(0)
#if !FORCE_PREVIEW_FORMAT_YUV420SP
  , mFormat(PREVIEW_FORMAT_UNKNOWN)
#else
  , mFormat(PREVIEW_FORMAT_YUV420SP)
#endif
  , mFps(30)
  , mDiscardedFrameCount(0)
  , mMediaProfiles(nullptr)
  , mRecorder(nullptr)
  , mProfileManager(nullptr)
  , mRecorderProfile(nullptr)
  , mVideoFile(nullptr)
{
  
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  mRwLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE, "GonkCameraControl.Parameters.Lock");
}

void nsGonkCameraControl::DispatchInit(nsDOMCameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, uint64_t aWindowId)
{
  
  nsCOMPtr<nsIRunnable> init = new InitGonkCameraControl(this, aDOMCameraControl, onSuccess, onError, aWindowId);
  mCameraThread->Dispatch(init, NS_DISPATCH_NORMAL);
}

nsresult
nsGonkCameraControl::Init()
{
  mCameraHw = GonkCameraHardware::Connect(this, mCameraId);
  if (!mCameraHw.get()) {
    DOM_CAMERA_LOGE("Failed to connect to camera %d (this=%p)\n", mCameraId, this);
    return NS_ERROR_FAILURE;
  }

  DOM_CAMERA_LOGI("Initializing camera %d (this=%p, mCameraHw=%p)\n", mCameraId, this, mCameraHw.get());

  
  PullParametersImpl();

  
#if !FORCE_PREVIEW_FORMAT_YUV420SP
  DOM_CAMERA_LOGI("Camera preview formats: %s\n", mParams.get(mParams.KEY_SUPPORTED_PREVIEW_FORMATS));
  const char* const PREVIEW_FORMAT = "yuv420p";
  const char* const BAD_PREVIEW_FORMAT = "yuv420sp";
  mParams.setPreviewFormat(PREVIEW_FORMAT);
  mParams.setPreviewFrameRate(mFps);
#else
  mParams.setPreviewFormat("yuv420sp");
  mParams.setPreviewFrameRate(mFps);
#endif
  PushParametersImpl();

  
  PullParametersImpl();
#if !FORCE_PREVIEW_FORMAT_YUV420SP
  const char* format = mParams.getPreviewFormat();
  if (strcmp(format, PREVIEW_FORMAT) == 0) {
    mFormat = PREVIEW_FORMAT_YUV420P;  
  } else if (strcmp(format, BAD_PREVIEW_FORMAT) == 0) {
    mFormat = PREVIEW_FORMAT_YUV420SP;
    DOM_CAMERA_LOGA("Camera ignored our request for '%s' preview, will have to convert (from %d)\n", PREVIEW_FORMAT, mFormat);
  } else {
    mFormat = PREVIEW_FORMAT_UNKNOWN;
    DOM_CAMERA_LOGE("Camera ignored our request for '%s' preview, returned UNSUPPORTED format '%s'\n", PREVIEW_FORMAT, format);
  }
#endif

  
  uint32_t fps = mParams.getPreviewFrameRate();
  if (fps != mFps) {
    DOM_CAMERA_LOGA("We asked for %d fps but camera returned %d fps, using that", mFps, fps);
    mFps = fps;
  }

  
  mExposureCompensationMin = mParams.getFloat(mParams.KEY_MIN_EXPOSURE_COMPENSATION);
  mExposureCompensationStep = mParams.getFloat(mParams.KEY_EXPOSURE_COMPENSATION_STEP);
  mMaxMeteringAreas = mParams.getInt(mParams.KEY_MAX_NUM_METERING_AREAS);
  mMaxFocusAreas = mParams.getInt(mParams.KEY_MAX_NUM_FOCUS_AREAS);
  mLastThumbnailWidth = mParams.getInt(mParams.KEY_JPEG_THUMBNAIL_WIDTH);
  mLastThumbnailHeight = mParams.getInt(mParams.KEY_JPEG_THUMBNAIL_HEIGHT);

  int w;
  int h;
  mParams.getPictureSize(&w, &h);
  MOZ_ASSERT(w > 0 && h > 0); 
  mLastPictureWidth = static_cast<uint32_t>(w);
  mLastPictureHeight = static_cast<uint32_t>(h);

  DOM_CAMERA_LOGI(" - minimum exposure compensation: %f\n", mExposureCompensationMin);
  DOM_CAMERA_LOGI(" - exposure compensation step:    %f\n", mExposureCompensationStep);
  DOM_CAMERA_LOGI(" - maximum metering areas:        %d\n", mMaxMeteringAreas);
  DOM_CAMERA_LOGI(" - maximum focus areas:           %d\n", mMaxFocusAreas);
  DOM_CAMERA_LOGI(" - default picture size:          %u x %u\n", mLastPictureWidth, mLastPictureHeight);
  DOM_CAMERA_LOGI(" - default thumbnail size:        %u x %u\n", mLastThumbnailWidth, mLastThumbnailHeight);

  return NS_OK;
}

nsGonkCameraControl::~nsGonkCameraControl()
{
  DOM_CAMERA_LOGT("%s:%d : this=%p, mCameraHw = %p\n", __func__, __LINE__, this, mCameraHw.get());

  ReleaseHardwareImpl(nullptr);
  if (mRwLock) {
    PRRWLock* lock = mRwLock;
    mRwLock = nullptr;
    PR_DestroyRWLock(lock);
  }

  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
}

class RwAutoLockRead
{
public:
  RwAutoLockRead(PRRWLock* aRwLock)
    : mRwLock(aRwLock)
  {
    PR_RWLock_Rlock(mRwLock);
  }

  ~RwAutoLockRead()
  {
    PR_RWLock_Unlock(mRwLock);
  }

protected:
  PRRWLock* mRwLock;
};

class RwAutoLockWrite
{
public:
  RwAutoLockWrite(PRRWLock* aRwLock)
    : mRwLock(aRwLock)
  {
    PR_RWLock_Wlock(mRwLock);
  }

  ~RwAutoLockWrite()
  {
    PR_RWLock_Unlock(mRwLock);
  }

protected:
  PRRWLock* mRwLock;
};

const char*
nsGonkCameraControl::GetParameter(const char* aKey)
{
  RwAutoLockRead lock(mRwLock);
  return mParams.get(aKey);
}

const char*
nsGonkCameraControl::GetParameterConstChar(uint32_t aKey)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return nullptr;
  }

  RwAutoLockRead lock(mRwLock);
  return mParams.get(key);
}

double
nsGonkCameraControl::GetParameterDouble(uint32_t aKey)
{
  double val;
  int index = 0;
  double focusDistance[3];
  const char* s;

  const char* key = getKeyText(aKey);
  if (!key) {
    
    return aKey == CAMERA_PARAM_ZOOM ? 1.0 : 0.0;
  }

  RwAutoLockRead lock(mRwLock);
  switch (aKey) {
    case CAMERA_PARAM_ZOOM:
      val = mParams.getInt(key);
      return val / 100;

    




    case CAMERA_PARAM_FOCUSDISTANCEFAR:
      ++index;
      

    case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      ++index;
      

    case CAMERA_PARAM_FOCUSDISTANCENEAR:
      s = mParams.get(key);
      if (sscanf(s, "%lf,%lf,%lf", &focusDistance[0], &focusDistance[1], &focusDistance[2]) == 3) {
        return focusDistance[index];
      }
      return 0.0;

    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      index = mParams.getInt(key);
      if (!index) {
        
        return NAN;
      }
      val = (index - 1) * mExposureCompensationStep + mExposureCompensationMin;
      DOM_CAMERA_LOGI("index = %d --> compensation = %f\n", index, val);
      return val;

    default:
      return mParams.getFloat(key);
  }
}

int32_t
nsGonkCameraControl::GetParameterInt32(uint32_t aKey)
{
  if (aKey == CAMERA_PARAM_SENSORANGLE) {
    if (!mCameraHw.get()) {
      return 0;
    }
    return mCameraHw->GetSensorOrientation();
  }

  const char* key = getKeyText(aKey);
  if (!key) {
    return 0;
  }

  RwAutoLockRead lock(mRwLock);
  return mParams.getInt(key);
}

void
nsGonkCameraControl::GetParameter(uint32_t aKey,
                                  nsTArray<idl::CameraRegion>& aRegions)
{
  aRegions.Clear();

  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  RwAutoLockRead lock(mRwLock);

  const char* value = mParams.get(key);
  DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, value);
  if (!value) {
    return;
  }

  const char* p = value;
  uint32_t count = 1;

  
  while ((p = strstr(p, "),("))) {
    ++count;
    p += 3;
  }

  aRegions.SetCapacity(count);
  idl::CameraRegion* r;

  
  uint32_t i;
  for (i = 0, p = value; p && i < count; ++i, p = strchr(p + 1, '(')) {
    r = aRegions.AppendElement();
    if (sscanf(p, "(%d,%d,%d,%d,%u)", &r->top, &r->left, &r->bottom, &r->right, &r->weight) != 5) {
      DOM_CAMERA_LOGE("%s:%d : region tuple has bad format: '%s'\n", __func__, __LINE__, p);
      aRegions.Clear();
      return;
    }
  }

  return;
}

void
nsGonkCameraControl::GetParameter(uint32_t aKey,
                                  nsTArray<idl::CameraSize>& aSizes)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  RwAutoLockRead lock(mRwLock);

  const char* value = mParams.get(key);
  DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, value);
  if (!value) {
    return;
  }

  const char* p = value;
  idl::CameraSize* s;

  
  while (p) {
    s = aSizes.AppendElement();
    if (sscanf(p, "%dx%d", &s->width, &s->height) != 2) {
      DOM_CAMERA_LOGE("%s:%d : size tuple has bad format: '%s'\n", __func__, __LINE__, p);
      aSizes.Clear();
      return;
    }
    
    p = strchr(p, ',');
    if (p) {
      
      ++p;
    }
  }

  return;
}

void
nsGonkCameraControl::GetParameter(uint32_t aKey, idl::CameraSize& aSize)
{
  if (aKey == CAMERA_PARAM_THUMBNAILSIZE) {
    
    
    RwAutoLockRead lock(mRwLock);

    aSize.width = mParams.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    aSize.height = mParams.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    DOM_CAMERA_LOGI("thumbnail size --> value='%ux%u'\n", aSize.width, aSize.height);
    return;
  }

  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  RwAutoLockRead lock(mRwLock);

  const char* value = mParams.get(key);
  DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, value);
  if (!value) {
    return;
  }

  if (sscanf(value, "%ux%u", &aSize.width, &aSize.height) != 2) {
    DOM_CAMERA_LOGE("%s:%d : size tuple has bad format: '%s'\n", __func__, __LINE__, value);
    aSize.width = 0;
    aSize.height = 0;
  }
}

nsresult
nsGonkCameraControl::PushParameters()
{
  if (mDeferConfigUpdate) {
    DOM_CAMERA_LOGT("%s:%d - defering config update\n", __func__, __LINE__);
    return NS_OK;
  }

  





  if (NS_IsMainThread()) {
    DOM_CAMERA_LOGT("%s:%d - dispatching to camera thread\n", __func__, __LINE__);
    nsCOMPtr<nsIRunnable> pushParametersTask = NS_NewRunnableMethod(this, &nsGonkCameraControl::PushParametersImpl);
    return mCameraThread->Dispatch(pushParametersTask, NS_DISPATCH_NORMAL);
  }

  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
  return PushParametersImpl();
}

void
nsGonkCameraControl::SetParameter(const char* aKey, const char* aValue)
{
  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(aKey, aValue);
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(uint32_t aKey, const char* aValue)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(key, aValue);
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(uint32_t aKey, double aValue)
{
  uint32_t index;

  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  {
    RwAutoLockWrite lock(mRwLock);
    if (aKey == CAMERA_PARAM_EXPOSURECOMPENSATION) {
      



      index = (aValue - mExposureCompensationMin + mExposureCompensationStep / 2) / mExposureCompensationStep + 1;
      DOM_CAMERA_LOGI("compensation = %f --> index = %d\n", aValue, index);
      mParams.set(key, index);
    } else {
      mParams.setFloat(key, aValue);
    }
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(uint32_t aKey,
                                  const nsTArray<idl::CameraRegion>& aRegions)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  uint32_t length = aRegions.Length();

  if (!length) {
    
    {
      RwAutoLockWrite lock(mRwLock);
      mParams.set(key, "(0,0,0,0,0)");
    }
    PushParameters();
    return;
  }

  nsCString s;

  for (uint32_t i = 0; i < length; ++i) {
    const idl::CameraRegion* r = &aRegions[i];
    s.AppendPrintf("(%d,%d,%d,%d,%d),", r->top, r->left, r->bottom, r->right, r->weight);
  }

  
  s.Trim(",", false, true, true);

  DOM_CAMERA_LOGI("camera region string '%s'\n", s.get());

  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(key, s.get());
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(uint32_t aKey, int aValue)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }
  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(key, aValue);
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(uint32_t aKey, const idl::CameraSize& aSize)
{
  switch (aKey) {
    case CAMERA_PARAM_PICTURESIZE:
      DOM_CAMERA_LOGI("setting picture size to %ux%u\n", aSize.width, aSize.height);
      SetPictureSize(aSize.width, aSize.height);
      break;

    case CAMERA_PARAM_THUMBNAILSIZE:
      DOM_CAMERA_LOGI("setting thumbnail size to %ux%u\n", aSize.width, aSize.height);
      SetThumbnailSize(aSize.width, aSize.height);
      break;

    default:
      {
        const char* key = getKeyText(aKey);
        if (!key) {
          return;
        }

        nsCString s;
        s.AppendPrintf("%ux%u", aSize.width, aSize.height);
        DOM_CAMERA_LOGI("setting '%s' to %s\n", key, s.get());

        RwAutoLockWrite lock(mRwLock);
        mParams.set(key, s.get());
      }
      break;
  }
  PushParameters();
}

nsresult
nsGonkCameraControl::GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream)
{
  
  StopPreviewInternal(true );

  
  mRecorderProfile = nullptr;

  SetPreviewSize(aGetPreviewStream->mSize.width, aGetPreviewStream->mSize.height);
  DOM_CAMERA_LOGI("picture preview: wanted %d x %d, got %d x %d (%d fps, format %d)\n", aGetPreviewStream->mSize.width, aGetPreviewStream->mSize.height, mWidth, mHeight, mFps, mFormat);

  nsMainThreadPtrHandle<nsICameraPreviewStreamCallback> onSuccess = aGetPreviewStream->mOnSuccessCb;
  nsCOMPtr<GetPreviewStreamResult> getPreviewStreamResult = new GetPreviewStreamResult(this, mWidth, mHeight, mFps, onSuccess, mWindowId);
  return NS_DispatchToMainThread(getPreviewStreamResult);
}

nsresult
nsGonkCameraControl::StartPreviewImpl(StartPreviewTask* aStartPreview)
{
  




  if (aStartPreview->mDOMPreview) {
    StopPreviewInternal(true );
    mDOMPreview = aStartPreview->mDOMPreview;
  } else if (!mDOMPreview) {
    return NS_ERROR_INVALID_ARG;
  }

  DOM_CAMERA_LOGI("%s: starting preview (mDOMPreview=%p)\n", __func__, mDOMPreview);

  RETURN_IF_NO_CAMERA_HW();
  if (mCameraHw->StartPreview() != OK) {
    DOM_CAMERA_LOGE("%s: failed to start preview\n", __func__);
    return NS_ERROR_FAILURE;
  }

  if (aStartPreview->mDOMPreview) {
    mDOMPreview->Started();
  }

  OnPreviewStateChange(PREVIEW_STARTED);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StopPreviewInternal(bool aForced)
{
  DOM_CAMERA_LOGI("%s: stopping preview (mDOMPreview=%p)\n", __func__, mDOMPreview);

  
  
  if (mDOMPreview) {
    if (mCameraHw.get()) {
      mCameraHw->StopPreview();
    }
    mDOMPreview->Stopped(aForced);
    mDOMPreview = nullptr;
  }

  OnPreviewStateChange(PREVIEW_STOPPED);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StopPreviewImpl(StopPreviewTask* aStopPreview)
{
  return StopPreviewInternal();
}

nsresult
nsGonkCameraControl::AutoFocusImpl(AutoFocusTask* aAutoFocus)
{
  if (aAutoFocus->mCancel) {
    if (mCameraHw.get()) {
      mCameraHw->CancelAutoFocus();
    }
  }

  mAutoFocusOnSuccessCb = aAutoFocus->mOnSuccessCb;
  mAutoFocusOnErrorCb = aAutoFocus->mOnErrorCb;

  RETURN_IF_NO_CAMERA_HW();
  if (mCameraHw->AutoFocus() != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

void
nsGonkCameraControl::SetThumbnailSize(uint32_t aWidth, uint32_t aHeight)
{
  




  mLastThumbnailWidth = aWidth;
  mLastThumbnailHeight = aHeight;

  



  if (!aWidth || !aHeight) {
    DOM_CAMERA_LOGW("Requested thumbnail size %ux%u, disabling thumbnail\n", aWidth, aHeight);
    RwAutoLockWrite write(mRwLock);
    mParams.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, 0);
    mParams.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, 0);
    return;
  }

  





  int smallestDelta = INT_MAX;
  uint32_t smallestDeltaIndex = UINT32_MAX;
  int targetArea = aWidth * aHeight;

  nsAutoTArray<idl::CameraSize, 8> supportedSizes;
  GetParameter(CAMERA_PARAM_SUPPORTED_JPEG_THUMBNAIL_SIZES, supportedSizes);

  for (uint32_t i = 0; i < supportedSizes.Length(); ++i) {
    int area = supportedSizes[i].width * supportedSizes[i].height;
    int delta = abs(area - targetArea);

    if (area != 0
      && delta < smallestDelta
      && supportedSizes[i].width * mLastPictureHeight / supportedSizes[i].height == mLastPictureWidth
    ) {
      smallestDelta = delta;
      smallestDeltaIndex = i;
    }
  }

  if (smallestDeltaIndex == UINT32_MAX) {
    DOM_CAMERA_LOGW("Unable to find a thumbnail size close to %ux%u\n", aWidth, aHeight);
    return;
  }

  uint32_t w = supportedSizes[smallestDeltaIndex].width;
  uint32_t h = supportedSizes[smallestDeltaIndex].height;
  DOM_CAMERA_LOGI("Requested thumbnail size %ux%u --> using supported size %ux%u\n", aWidth, aHeight, w, h);
  if (w > INT32_MAX || h > INT32_MAX) {
    DOM_CAMERA_LOGE("Supported thumbnail size is too big, no change\n");
    return;
  }

  RwAutoLockWrite write(mRwLock);
  mParams.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, static_cast<int>(w));
  mParams.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, static_cast<int>(h));
}

void
nsGonkCameraControl::UpdateThumbnailSize()
{
  SetThumbnailSize(mLastThumbnailWidth, mLastThumbnailHeight);
}

void
nsGonkCameraControl::SetPictureSize(uint32_t aWidth, uint32_t aHeight)
{
  




  if (!aWidth || !aHeight) {
    DOM_CAMERA_LOGW("Ignoring requested picture size of %ux%u\n", aWidth, aHeight);
    return;
  }

  if (aWidth == mLastPictureWidth && aHeight == mLastPictureHeight) {
    DOM_CAMERA_LOGI("Requested picture size %ux%u unchanged\n", aWidth, aHeight);
    return;
  }

  





  int smallestDelta = INT_MAX;
  uint32_t smallestDeltaIndex = UINT32_MAX;
  int targetArea = aWidth * aHeight;
  
  nsAutoTArray<idl::CameraSize, 8> supportedSizes;
  GetParameter(CAMERA_PARAM_SUPPORTED_PICTURESIZES, supportedSizes);

  for (uint32_t i = 0; i < supportedSizes.Length(); ++i) {
    int area = supportedSizes[i].width * supportedSizes[i].height;
    int delta = abs(area - targetArea);

    if (area != 0 && delta < smallestDelta) {
      smallestDelta = delta;
      smallestDeltaIndex = i;
    }
  }

  if (smallestDeltaIndex == UINT32_MAX) {
    DOM_CAMERA_LOGW("Unable to find a picture size close to %ux%u\n", aWidth, aHeight);
    return;
  }

  uint32_t w = supportedSizes[smallestDeltaIndex].width;
  uint32_t h = supportedSizes[smallestDeltaIndex].height;
  DOM_CAMERA_LOGI("Requested picture size %ux%u --> using supported size %ux%u\n", aWidth, aHeight, w, h);
  if (w > INT32_MAX || h > INT32_MAX) {
    DOM_CAMERA_LOGE("Supported picture size is too big, no change\n");
    return;
  }

  mLastPictureWidth = w;
  mLastPictureHeight = h;

  {
    
    RwAutoLockWrite write(mRwLock);
    mParams.setPictureSize(static_cast<int>(w), static_cast<int>(h));
  }

  
  UpdateThumbnailSize();
}

nsresult
nsGonkCameraControl::TakePictureImpl(TakePictureTask* aTakePicture)
{
  if (aTakePicture->mCancel) {
    if (mCameraHw.get()) {
      mCameraHw->CancelTakePicture();
    }
  }

  mTakePictureOnSuccessCb = aTakePicture->mOnSuccessCb;
  mTakePictureOnErrorCb = aTakePicture->mOnErrorCb;

  RETURN_IF_NO_CAMERA_HW();

  
  mDeferConfigUpdate = true;

  SetPictureSize(aTakePicture->mSize.width, aTakePicture->mSize.height);

  
  mFileFormat = aTakePicture->mFileFormat;
  SetParameter(CameraParameters::KEY_PICTURE_FORMAT, NS_ConvertUTF16toUTF8(mFileFormat).get());

  
  uint32_t r = static_cast<uint32_t>(aTakePicture->mRotation);
  r += mCameraHw->GetSensorOrientation(GonkCameraHardware::OFFSET_SENSOR_ORIENTATION);
  r %= 360;
  r += 45;
  r /= 90;
  r *= 90;
  DOM_CAMERA_LOGI("setting picture rotation to %d degrees (mapped from %d)\n", r, aTakePicture->mRotation);
  SetParameter(CameraParameters::KEY_ROTATION, nsPrintfCString("%u", r).get());

  
  if (!isnan(aTakePicture->mPosition.latitude)) {
    DOM_CAMERA_LOGI("setting picture latitude to %lf\n", aTakePicture->mPosition.latitude);
    SetParameter(CameraParameters::KEY_GPS_LATITUDE, nsPrintfCString("%lf", aTakePicture->mPosition.latitude).get());
  }
  if (!isnan(aTakePicture->mPosition.longitude)) {
    DOM_CAMERA_LOGI("setting picture longitude to %lf\n", aTakePicture->mPosition.longitude);
    SetParameter(CameraParameters::KEY_GPS_LONGITUDE, nsPrintfCString("%lf", aTakePicture->mPosition.longitude).get());
  }
  if (!isnan(aTakePicture->mPosition.altitude)) {
    DOM_CAMERA_LOGI("setting picture altitude to %lf\n", aTakePicture->mPosition.altitude);
    SetParameter(CameraParameters::KEY_GPS_ALTITUDE, nsPrintfCString("%lf", aTakePicture->mPosition.altitude).get());
  }
  if (!isnan(aTakePicture->mPosition.timestamp)) {
    DOM_CAMERA_LOGI("setting picture timestamp to %lf\n", aTakePicture->mPosition.timestamp);
    SetParameter(CameraParameters::KEY_GPS_TIMESTAMP, nsPrintfCString("%lf", aTakePicture->mPosition.timestamp).get());
  }

  
  
  
  
  time_t time = aTakePicture->mDateTime;
  if ((uint64_t)time != aTakePicture->mDateTime) {
    DOM_CAMERA_LOGE("picture date/time '%llu' is too far in the future\n", aTakePicture->mDateTime);
  } else {
    struct tm t;
    if (localtime_r(&time, &t)) {
      char dateTime[20];
      if (strftime(dateTime, sizeof(dateTime), "%Y:%m:%d %T", &t)) {
        DOM_CAMERA_LOGI("setting picture date/time to %s\n", dateTime);
        
        
        
        
        
        SetParameter("exif-datetime", dateTime);
      } else {
        DOM_CAMERA_LOGE("picture date/time couldn't be converted to string\n");
      }
    } else {
      DOM_CAMERA_LOGE("picture date/time couldn't be converted to local time: (%d) %s\n", errno, strerror(errno));
    }
  }

  mDeferConfigUpdate = false;
  PushParameters();

  if (mCameraHw->TakePicture() != OK) {
    return NS_ERROR_FAILURE;
  }
  
  
  
  OnPreviewStateChange(PREVIEW_STOPPED);
  return NS_OK;
}

nsresult
nsGonkCameraControl::PushParametersImpl()
{
  DOM_CAMERA_LOGI("Pushing camera parameters\n");
  RETURN_IF_NO_CAMERA_HW();

  RwAutoLockRead lock(mRwLock);
  if (mCameraHw->PushParameters(mParams) != OK) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::PullParametersImpl()
{
  DOM_CAMERA_LOGI("Pulling camera parameters\n");
  RETURN_IF_NO_CAMERA_HW();

  RwAutoLockWrite lock(mRwLock);
  mCameraHw->PullParameters(mParams);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StartRecordingImpl(StartRecordingTask* aStartRecording)
{
  NS_ENSURE_TRUE(mRecorderProfile, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mRecorder, NS_ERROR_FAILURE);

  







  nsRefPtr<DeviceStorageFileDescriptor> dsfd = aStartRecording->mDSFileDescriptor;
  NS_ENSURE_TRUE(dsfd, NS_ERROR_FAILURE);
  nsAutoString fullPath;
  mVideoFile = dsfd->mDSFile;
  mVideoFile->GetFullPath(fullPath);
  DOM_CAMERA_LOGI("Video filename is '%s'\n",
                  NS_LossyConvertUTF16toASCII(fullPath).get());

  if (!mVideoFile->IsSafePath()) {
    DOM_CAMERA_LOGE("Invalid video file name\n");
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  rv = SetupRecording(dsfd->mFileDescriptor.PlatformHandle(),
                      aStartRecording->mOptions.rotation,
                      aStartRecording->mOptions.maxFileSizeBytes,
                      aStartRecording->mOptions.maxVideoLengthMs);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mRecorder->start() != OK) {
    DOM_CAMERA_LOGE("mRecorder->start() failed\n");
    
    mRecorder = nullptr;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

class RecordingComplete : public nsRunnable
{
public:
  RecordingComplete(DeviceStorageFile* aFile)
    : mFile(aFile)
  { }

  ~RecordingComplete() { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    obs->NotifyObservers(mFile, "file-watcher-notify", MOZ_UTF16("modified"));
    return NS_OK;
  }

private:
  nsRefPtr<DeviceStorageFile> mFile;
};

nsresult
nsGonkCameraControl::StopRecordingImpl(StopRecordingTask* aStopRecording)
{
  
  NS_ENSURE_TRUE(mRecorder, NS_OK);

  mRecorder->stop();
  mRecorder = nullptr;

  
  nsCOMPtr<nsIRunnable> recordingComplete = new RecordingComplete(mVideoFile);
  return NS_DispatchToMainThread(recordingComplete, NS_DISPATCH_NORMAL);
}

void
nsGonkCameraControl::AutoFocusComplete(bool aSuccess)
{
  




  PullParametersImpl();

  





  nsCOMPtr<nsIRunnable> autoFocusResult = new AutoFocusResult(aSuccess, mAutoFocusOnSuccessCb, mWindowId);
  



  mAutoFocusOnSuccessCb = nullptr;
  mAutoFocusOnErrorCb = nullptr;
  nsresult rv = NS_DispatchToMainThread(autoFocusResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch autoFocus() onSuccess callback to main thread!");
  }
}

void
nsGonkCameraControl::TakePictureComplete(uint8_t* aData, uint32_t aLength)
{
  uint8_t* data = new uint8_t[aLength];

  memcpy(data, aData, aLength);

  
  nsCOMPtr<nsIRunnable> takePictureResult = new TakePictureResult(data, aLength, NS_LITERAL_STRING("image/jpeg"), mTakePictureOnSuccessCb, mWindowId);
  



  mTakePictureOnSuccessCb = nullptr;
  mTakePictureOnErrorCb = nullptr;
  nsresult rv = NS_DispatchToMainThread(takePictureResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch takePicture() onSuccess callback to main thread!");
  }
}

void
nsGonkCameraControl::TakePictureError()
{
  nsCOMPtr<nsIRunnable> takePictureError = new CameraErrorResult(mTakePictureOnErrorCb, NS_LITERAL_STRING("FAILURE"), mWindowId);
  mTakePictureOnSuccessCb = nullptr;
  mTakePictureOnErrorCb = nullptr;
  nsresult rv = NS_DispatchToMainThread(takePictureError);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch takePicture() onError callback to main thread!");
  }
}

void
nsGonkCameraControl::SetPreviewSize(uint32_t aWidth, uint32_t aHeight)
{
  android::Vector<Size> previewSizes;
  uint32_t bestWidth = aWidth;
  uint32_t bestHeight = aHeight;
  uint32_t minSizeDelta = UINT32_MAX;
  uint32_t delta;
  Size size;

  {
    RwAutoLockRead lock(mRwLock);
    mParams.getSupportedPreviewSizes(previewSizes);
  }

  if (!aWidth && !aHeight) {
    
    size = previewSizes[0];
    bestWidth = size.width;
    bestHeight = size.height;
  } else if (aWidth && aHeight) {
    
    for (uint32_t i = 0; i < previewSizes.size(); i++) {
      Size size = previewSizes[i];
      uint32_t delta = abs((long int)(size.width * size.height - aWidth * aHeight));
      if (delta < minSizeDelta) {
        minSizeDelta = delta;
        bestWidth = size.width;
        bestHeight = size.height;
      }
    }
  } else if (!aWidth) {
    
    for (uint32_t i = 0; i < previewSizes.size(); i++) {
      size = previewSizes[i];
      delta = abs((long int)(size.height - aHeight));
      if (delta < minSizeDelta) {
        minSizeDelta = delta;
        bestWidth = size.width;
        bestHeight = size.height;
      }
    }
  } else if (!aHeight) {
    
    for (uint32_t i = 0; i < previewSizes.size(); i++) {
      size = previewSizes[i];
      delta = abs((long int)(size.width - aWidth));
      if (delta < minSizeDelta) {
        minSizeDelta = delta;
        bestWidth = size.width;
        bestHeight = size.height;
      }
    }
  }

  mWidth = bestWidth;
  mHeight = bestHeight;
  {
    RwAutoLockWrite lock(mRwLock);
    mParams.setPreviewSize(mWidth, mHeight);
  }
  PushParameters();
}

nsresult
nsGonkCameraControl::SetupVideoMode(const nsAString& aProfile)
{
  
  mMediaProfiles = MediaProfiles::getInstance();

  nsAutoCString profile = NS_ConvertUTF16toUTF8(aProfile);
  mRecorderProfile = GetGonkRecorderProfileManager().get()->Get(profile.get());
  if (!mRecorderProfile) {
    DOM_CAMERA_LOGE("Recorder profile '%s' is not supported\n", profile.get());
    return NS_ERROR_INVALID_ARG;
  }

  const GonkRecorderVideoProfile* video = mRecorderProfile->GetGonkVideoProfile();
  int width = video->GetWidth();
  int height = video->GetHeight();
  int fps = video->GetFramerate();
  if (fps == -1 || width == -1 || height == -1) {
    DOM_CAMERA_LOGE("Can't configure preview with fps=%d, width=%d, height=%d\n", fps, width, height);
    return NS_ERROR_FAILURE;
  }

  PullParametersImpl();

  
  const size_t SIZE = 256;
  char buffer[SIZE];

  {
    RwAutoLockWrite lock(mRwLock);
    mParams.setPreviewSize(width, height);
    mParams.setPreviewFrameRate(fps);

    




    snprintf(buffer, SIZE, "%dx%d", width, height);
    mParams.set("record-size", buffer);
  }

  
  PushParameters();
  return NS_OK;
}

class GonkRecorderListener : public IMediaRecorderClient
{
public:
  GonkRecorderListener(nsGonkCameraControl* aCameraControl)
    : mCameraControl(aCameraControl)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p, aCameraControl=%p\n", __func__, __LINE__, this, mCameraControl.get());
  }

  void notify(int msg, int ext1, int ext2)
  {
    if (mCameraControl) {
      mCameraControl->HandleRecorderEvent(msg, ext1, ext2);
    }
  }

  IBinder* onAsBinder()
  {
    DOM_CAMERA_LOGE("onAsBinder() called, should NEVER get called!\n");
    return nullptr;
  }

protected:
  ~GonkRecorderListener() { }
  nsRefPtr<nsGonkCameraControl> mCameraControl;
};

void
nsGonkCameraControl::HandleRecorderEvent(int msg, int ext1, int ext2)
{
  













































  int trackNum = -1;  

  switch (msg) {
    
    case MEDIA_RECORDER_EVENT_INFO:
      switch (ext1) {
        case MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED:
          DOM_CAMERA_LOGI("recorder-event : info: maximum file size reached\n");
          OnRecorderStateChange(NS_LITERAL_STRING("FileSizeLimitReached"), ext2, trackNum);
          return;

        case MEDIA_RECORDER_INFO_MAX_DURATION_REACHED:
          DOM_CAMERA_LOGI("recorder-event : info: maximum video duration reached\n");
          OnRecorderStateChange(NS_LITERAL_STRING("VideoLengthLimitReached"), ext2, trackNum);
          return;

        case MEDIA_RECORDER_TRACK_INFO_COMPLETION_STATUS:
          DOM_CAMERA_LOGI("recorder-event : info: track completed\n");
          OnRecorderStateChange(NS_LITERAL_STRING("TrackCompleted"), ext2, trackNum);
          return;
      }
      break;

    case MEDIA_RECORDER_EVENT_ERROR:
      switch (ext1) {
        case MEDIA_RECORDER_ERROR_UNKNOWN:
          DOM_CAMERA_LOGE("recorder-event : recorder-error: %d (0x%08x)\n", ext2, ext2);
          OnRecorderStateChange(NS_LITERAL_STRING("MediaRecorderFailed"), ext2, trackNum);
          return;

        case MEDIA_ERROR_SERVER_DIED:
          DOM_CAMERA_LOGE("recorder-event : recorder-error: server died\n");
          OnRecorderStateChange(NS_LITERAL_STRING("MediaServerFailed"), ext2, trackNum);
          return;
      }
      break;

    
    case MEDIA_RECORDER_TRACK_EVENT_INFO:
      trackNum = (ext1 & 0xF0000000) >> 28;
      ext1 &= 0xFFFF;
      switch (ext1) {
        case MEDIA_RECORDER_TRACK_INFO_COMPLETION_STATUS:
          if (ext2 == OK) {
            DOM_CAMERA_LOGI("recorder-event : track-complete: track %d, %d (0x%08x)\n", trackNum, ext2, ext2);
            OnRecorderStateChange(NS_LITERAL_STRING("TrackCompleted"), ext2, trackNum);
            return;
          }
          DOM_CAMERA_LOGE("recorder-event : track-error: track %d, %d (0x%08x)\n", trackNum, ext2, ext2);
          OnRecorderStateChange(NS_LITERAL_STRING("TrackFailed"), ext2, trackNum);
          return;

        case MEDIA_RECORDER_TRACK_INFO_PROGRESS_IN_TIME:
          DOM_CAMERA_LOGI("recorder-event : track-info: progress in time: %d ms\n", ext2);
          return;
      }
      break;

    case MEDIA_RECORDER_TRACK_EVENT_ERROR:
      trackNum = (ext1 & 0xF0000000) >> 28;
      ext1 &= 0xFFFF;
      DOM_CAMERA_LOGE("recorder-event : track-error: track %d, %d (0x%08x)\n", trackNum, ext2, ext2);
      OnRecorderStateChange(NS_LITERAL_STRING("TrackFailed"), ext2, trackNum);
      return;
  }

  
  DOM_CAMERA_LOGW("recorder-event : unhandled: msg=%d, ext1=%d, ext2=%d\n", msg, ext1, ext2);
}

nsresult
nsGonkCameraControl::SetupRecording(int aFd, int aRotation, int64_t aMaxFileSizeBytes, int64_t aMaxVideoLengthMs)
{
  RETURN_IF_NO_CAMERA_HW();

  
  const size_t SIZE = 256;
  char buffer[SIZE];

  mRecorder = new GonkRecorder();
  CHECK_SETARG(mRecorder->init());

  nsresult rv = mRecorderProfile->ConfigureRecorder(mRecorder);
  NS_ENSURE_SUCCESS(rv, rv);

  CHECK_SETARG(mRecorder->setCamera(mCameraHw));

  DOM_CAMERA_LOGI("maxVideoLengthMs=%lld\n", aMaxVideoLengthMs);
  if (aMaxVideoLengthMs == 0) {
    aMaxVideoLengthMs = -1;
  }
  snprintf(buffer, SIZE, "max-duration=%lld", aMaxVideoLengthMs);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));

  DOM_CAMERA_LOGI("maxFileSizeBytes=%lld\n", aMaxFileSizeBytes);
  if (aMaxFileSizeBytes == 0) {
    aMaxFileSizeBytes = -1;
  }
  snprintf(buffer, SIZE, "max-filesize=%lld", aMaxFileSizeBytes);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));

  
  int r = aRotation;
  r += mCameraHw->GetSensorOrientation();
  r %= 360;
  r += 45;
  r /= 90;
  r *= 90;
  if (r < 0) {
    
    r += 360;
  }
  DOM_CAMERA_LOGI("setting video rotation to %d degrees (mapped from %d)\n", r, aRotation);
  snprintf(buffer, SIZE, "video-param-rotation-angle-degrees=%d", r);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));

  CHECK_SETARG(mRecorder->setListener(new GonkRecorderListener(this)));

  
  CHECK_SETARG(mRecorder->setOutputFile(aFd, 0, 0));
  CHECK_SETARG(mRecorder->prepare());
  return NS_OK;
}

nsresult
nsGonkCameraControl::GetPreviewStreamVideoModeImpl(GetPreviewStreamVideoModeTask* aGetPreviewStreamVideoMode)
{
  
  StopPreviewInternal(true );

  
  nsresult rv = SetupVideoMode(aGetPreviewStreamVideoMode->mOptions.profile);
  NS_ENSURE_SUCCESS(rv, rv);
  
  const RecorderVideoProfile* video = mRecorderProfile->GetVideoProfile();
  int width = video->GetWidth();
  int height = video->GetHeight();
  int fps = video->GetFramerate();
  DOM_CAMERA_LOGI("recording preview format: %d x %d (%d fps)\n", width, height, fps);

  
  nsCOMPtr<GetPreviewStreamResult> getPreviewStreamResult = new GetPreviewStreamResult(this, width, height, fps, aGetPreviewStreamVideoMode->mOnSuccessCb, mWindowId);
  rv = NS_DispatchToMainThread(getPreviewStreamResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch GetPreviewStreamVideoMode() onSuccess callback to main thread!");
    return rv;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::ReleaseHardwareImpl(ReleaseHardwareTask* aReleaseHardware)
{
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);

  
  if (mRecorder) {
    DOM_CAMERA_LOGI("shutting down existing video recorder\n");
    mRecorder->stop();
    mRecorder = nullptr;
  }

  
  StopPreviewInternal(true );

  
  if (mCameraHw.get()){
     mCameraHw->Close();
     mCameraHw.clear();
  }

  if (aReleaseHardware) {
    nsCOMPtr<nsIRunnable> releaseHardwareResult = new ReleaseHardwareResult(aReleaseHardware->mOnSuccessCb, mWindowId);
    return NS_DispatchToMainThread(releaseHardwareResult);
  }

  return NS_OK;
}

already_AddRefed<GonkRecorderProfileManager>
nsGonkCameraControl::GetGonkRecorderProfileManager()
{
  if (!mProfileManager) {
    nsTArray<idl::CameraSize> sizes;
    nsresult rv = GetVideoSizes(sizes);
    NS_ENSURE_SUCCESS(rv, nullptr);

    mProfileManager = new GonkRecorderProfileManager(mCameraId);
    mProfileManager->SetSupportedResolutions(sizes);
  }

  nsRefPtr<GonkRecorderProfileManager> profileMgr = mProfileManager;
  return profileMgr.forget();
}

already_AddRefed<RecorderProfileManager>
nsGonkCameraControl::GetRecorderProfileManagerImpl()
{
  nsRefPtr<RecorderProfileManager> profileMgr = GetGonkRecorderProfileManager();
  return profileMgr.forget();
}

nsresult
nsGonkCameraControl::GetVideoSizes(nsTArray<idl::CameraSize>& aVideoSizes)
{
  aVideoSizes.Clear();

  android::Vector<Size> sizes;
  {
    RwAutoLockRead lock(mRwLock);

    mParams.getSupportedVideoSizes(sizes);
    if (sizes.size() == 0) {
      DOM_CAMERA_LOGI("Camera doesn't support video independent of the preview\n");
      mParams.getSupportedPreviewSizes(sizes);
    }
  }

  if (sizes.size() == 0) {
    DOM_CAMERA_LOGW("Camera doesn't report any supported video sizes at all\n");
    return NS_OK;
  }

  for (size_t i = 0; i < sizes.size(); ++i) {
    idl::CameraSize size;
    size.width = sizes[i].width;
    size.height = sizes[i].height;
    aVideoSizes.AppendElement(size);
  }
  return NS_OK;
}


namespace mozilla {

void
ReceiveImage(nsGonkCameraControl* gc, uint8_t* aData, uint32_t aLength)
{
  gc->TakePictureComplete(aData, aLength);
}

void
ReceiveImageError(nsGonkCameraControl* gc)
{
  gc->TakePictureError();
}

void
AutoFocusComplete(nsGonkCameraControl* gc, bool aSuccess)
{
  gc->AutoFocusComplete(aSuccess);
}

static void
GonkFrameBuilder(Image* aImage, void* aBuffer, uint32_t aWidth, uint32_t aHeight)
{
  



  GrallocImage* videoImage = static_cast<GrallocImage*>(aImage);
  GrallocImage::GrallocData data;
  data.mGraphicBuffer = static_cast<layers::GraphicBufferLocked*>(aBuffer);
  data.mPicSize = IntSize(aWidth, aHeight);
  videoImage->SetData(data);
}

void
ReceiveFrame(nsGonkCameraControl* gc, layers::GraphicBufferLocked* aBuffer)
{
  if (!gc->ReceiveFrame(aBuffer, ImageFormat::GRALLOC_PLANAR_YCBCR, GonkFrameBuilder)) {
    aBuffer->Unlock();
  }
}

void
OnShutter(nsGonkCameraControl* gc)
{
  gc->OnShutter();
}

void
OnClosed(nsGonkCameraControl* gc)
{
  gc->OnClosed();
}

} 
