















#include "GonkCameraParameters.h"
#include "camera/CameraParameters.h"
#include "ICameraControl.h"
#include "CameraCommon.h"

using namespace mozilla;
using namespace android;

 const char*
GonkCameraParameters::Parameters::GetTextKey(uint32_t aKey)
{
  switch (aKey) {
    case CAMERA_PARAM_PREVIEWSIZE:
      return KEY_PREVIEW_SIZE;
    case CAMERA_PARAM_PREVIEWFORMAT:
      return KEY_PREVIEW_FORMAT;
    case CAMERA_PARAM_PREVIEWFRAMERATE:
      return KEY_PREVIEW_FRAME_RATE;
    case CAMERA_PARAM_EFFECT:
      return KEY_EFFECT;
    case CAMERA_PARAM_WHITEBALANCE:
      return KEY_WHITE_BALANCE;
    case CAMERA_PARAM_SCENEMODE:
      return KEY_SCENE_MODE;
    case CAMERA_PARAM_FLASHMODE:
      return KEY_FLASH_MODE;
    case CAMERA_PARAM_FOCUSMODE:
      return KEY_FOCUS_MODE;
    case CAMERA_PARAM_ZOOM:
      return KEY_ZOOM;
    case CAMERA_PARAM_METERINGAREAS:
      return KEY_METERING_AREAS;
    case CAMERA_PARAM_FOCUSAREAS:
      return KEY_FOCUS_AREAS;
    case CAMERA_PARAM_FOCALLENGTH:
      return KEY_FOCAL_LENGTH;
    case CAMERA_PARAM_FOCUSDISTANCENEAR:
      return KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      return KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_FOCUSDISTANCEFAR:
      return KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      return KEY_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_PICTURESIZE:
      return KEY_PICTURE_SIZE;
    case CAMERA_PARAM_THUMBNAILQUALITY:
      return KEY_JPEG_THUMBNAIL_QUALITY;
    case CAMERA_PARAM_PICTURE_SIZE:
      return KEY_PICTURE_SIZE;
    case CAMERA_PARAM_PICTURE_FILEFORMAT:
      return KEY_PICTURE_FORMAT;
    case CAMERA_PARAM_PICTURE_ROTATION:
      return KEY_ROTATION;
    case CAMERA_PARAM_PICTURE_DATETIME:
      
      
      
      
      
      return "exif-datetime";
    case CAMERA_PARAM_VIDEOSIZE:
      return KEY_VIDEO_SIZE;

    case CAMERA_PARAM_SUPPORTED_PREVIEWSIZES:
      return KEY_SUPPORTED_PREVIEW_SIZES;
    case CAMERA_PARAM_SUPPORTED_PICTURESIZES:
      return KEY_SUPPORTED_PICTURE_SIZES;
    case CAMERA_PARAM_SUPPORTED_VIDEOSIZES:
      return KEY_SUPPORTED_VIDEO_SIZES;
    case CAMERA_PARAM_SUPPORTED_PICTUREFORMATS:
      return KEY_SUPPORTED_PICTURE_FORMATS;
    case CAMERA_PARAM_SUPPORTED_WHITEBALANCES:
      return KEY_SUPPORTED_WHITE_BALANCE;
    case CAMERA_PARAM_SUPPORTED_SCENEMODES:
      return KEY_SUPPORTED_SCENE_MODES;
    case CAMERA_PARAM_SUPPORTED_EFFECTS:
      return KEY_SUPPORTED_EFFECTS;
    case CAMERA_PARAM_SUPPORTED_FLASHMODES:
      return KEY_SUPPORTED_FLASH_MODES;
    case CAMERA_PARAM_SUPPORTED_FOCUSMODES:
      return KEY_SUPPORTED_FOCUS_MODES;
    case CAMERA_PARAM_SUPPORTED_MAXFOCUSAREAS:
      return KEY_MAX_NUM_FOCUS_AREAS;
    case CAMERA_PARAM_SUPPORTED_MAXMETERINGAREAS:
      return KEY_MAX_NUM_METERING_AREAS;
    case CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION:
      return KEY_MIN_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_MAXEXPOSURECOMPENSATION:
      return KEY_MAX_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_EXPOSURECOMPENSATIONSTEP:
      return KEY_EXPOSURE_COMPENSATION_STEP;
    case CAMERA_PARAM_SUPPORTED_ZOOM:
      return KEY_ZOOM_SUPPORTED;
    case CAMERA_PARAM_SUPPORTED_ZOOMRATIOS:
      return KEY_ZOOM_RATIOS;
    case CAMERA_PARAM_SUPPORTED_JPEG_THUMBNAIL_SIZES:
      return KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES;
    default:
      DOM_CAMERA_LOGE("Unhandled camera parameter value %u\n", aKey);
      return nullptr;
  }
}

GonkCameraParameters::GonkCameraParameters()
  : mLock(PR_NewRWLock(PR_RWLOCK_RANK_NONE, "GonkCameraParameters.Lock"))
  , mDirty(false)
  , mInitialized(false)
{
  MOZ_COUNT_CTOR(GonkCameraParameters);
  if (!mLock) {
    MOZ_CRASH("OOM getting new PRRWLock");
  }
}

GonkCameraParameters::~GonkCameraParameters()
{
  MOZ_COUNT_DTOR(GonkCameraParameters);
  if (mLock) {
    PR_DestroyRWLock(mLock);
    mLock = nullptr;
  }
}



nsresult
GonkCameraParameters::Initialize()
{
  nsresult rv;

  rv = GetImpl(CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION, mExposureCompensationMin);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = GetImpl(CAMERA_PARAM_SUPPORTED_EXPOSURECOMPENSATIONSTEP, mExposureCompensationStep);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mInitialized = true;
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const nsAString& aValue)
{
  return SetImpl(aKey, NS_ConvertUTF16toUTF8(aValue).get());
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsAString& aValue)
{
  const char* val;
  nsresult rv = GetImpl(aKey, val);
  if (NS_FAILED(rv)) {
    return rv;
  }
  aValue.AssignASCII(val);
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const ICameraControl::Size& aSize)
{
  if (aSize.width > INT_MAX || aSize.height > INT_MAX) {
    
    DOM_CAMERA_LOGE("Camera parameter aKey=%d out of bounds (width=%u, height=%u)\n",
      aSize.width, aSize.height);
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;

  switch (aKey) {
    case CAMERA_PARAM_THUMBNAILSIZE:
      
      
      
      rv = SetImpl(Parameters::KEY_JPEG_THUMBNAIL_WIDTH, static_cast<int>(aSize.width));
      if (NS_SUCCEEDED(rv)) {
        rv = SetImpl(Parameters::KEY_JPEG_THUMBNAIL_HEIGHT, static_cast<int>(aSize.height));
      }
      break;

    case CAMERA_PARAM_VIDEOSIZE:
      
      
      
      rv = SetImpl("record-size", nsPrintfCString("%ux%u", aSize.width, aSize.height).get());
      if (NS_FAILED(rv)) {
        break;
      }
      

    default:
      rv = SetImpl(aKey, nsPrintfCString("%ux%u", aSize.width, aSize.height).get());
      break;
  }

  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Camera parameter aKey=%d failed to set (0x%x)\n", aKey, rv);
  }
  return rv;
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, ICameraControl::Size& aSize)
{
  nsresult rv;

  if (aKey == CAMERA_PARAM_THUMBNAILSIZE) {
    int width;
    int height;

    rv = GetImpl(Parameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
    if (NS_FAILED(rv) || width < 0) {
      return NS_ERROR_FAILURE;
    }
    rv = GetImpl(Parameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
    if (NS_FAILED(rv) || height < 0) {
      return NS_ERROR_FAILURE;
    }

    aSize.width = static_cast<uint32_t>(width);
    aSize.height = static_cast<uint32_t>(height);
    return NS_OK;
  }

  const char* value;
  rv = GetImpl(aKey, value);
  if (NS_FAILED(rv) || !value || *value == '\0') {
    DOM_CAMERA_LOGW("Camera parameter aKey=%d not available (0x%x)\n", aKey, rv);
    return NS_ERROR_NOT_AVAILABLE;
  }
  if (sscanf(value, "%ux%u", &aSize.width, &aSize.height) != 2) {
    DOM_CAMERA_LOGE("Camera parameter aKey=%d size tuple '%s' is invalid\n", aKey, value);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const nsTArray<ICameraControl::Region>& aRegions)
{
  uint32_t length = aRegions.Length();

  if (!length) {
    
    return SetImpl(aKey, "(0,0,0,0,0)");
  }

  nsCString s;

  for (uint32_t i = 0; i < length; ++i) {
    const ICameraControl::Region* r = &aRegions[i];
    s.AppendPrintf("(%d,%d,%d,%d,%d),", r->top, r->left, r->bottom, r->right, r->weight);
  }

  
  s.Trim(",", false, true, true);

  return SetImpl(aKey, s.get());
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsTArray<ICameraControl::Region>& aRegions)
{
  aRegions.Clear();

  const char* value;
  nsresult rv = GetImpl(aKey, value);
  if (NS_FAILED(rv) || !value || *value == '\0') {
    return NS_ERROR_FAILURE;
  }

  const char* p = value;
  uint32_t count = 1;

  
  while ((p = strstr(p, "),("))) {
    ++count;
    p += 3;
  }

  aRegions.SetCapacity(count);
  ICameraControl::Region* r;

  
  uint32_t i;
  for (i = 0, p = value; p && i < count; ++i, p = strchr(p + 1, '(')) {
    r = aRegions.AppendElement();
    if (sscanf(p, "(%d,%d,%d,%d,%u)", &r->top, &r->left, &r->bottom, &r->right, &r->weight) != 5) {
      DOM_CAMERA_LOGE("%s:%d : region tuple has bad format: '%s'\n", __func__, __LINE__, p);
      aRegions.Clear();
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const ICameraControl::Position& aPosition)
{
  MOZ_ASSERT(aKey == CAMERA_PARAM_PICTURE_LOCATION);

  
  if (!isnan(aPosition.latitude)) {
    DOM_CAMERA_LOGI("setting picture latitude to %lf\n", aPosition.latitude);
    SetImpl(Parameters::KEY_GPS_LATITUDE, nsPrintfCString("%lf", aPosition.latitude).get());
  }
  if (!isnan(aPosition.longitude)) {
    DOM_CAMERA_LOGI("setting picture longitude to %lf\n", aPosition.longitude);
    SetImpl(Parameters::KEY_GPS_LONGITUDE, nsPrintfCString("%lf", aPosition.longitude).get());
  }
  if (!isnan(aPosition.altitude)) {
    DOM_CAMERA_LOGI("setting picture altitude to %lf\n", aPosition.altitude);
    SetImpl(Parameters::KEY_GPS_ALTITUDE, nsPrintfCString("%lf", aPosition.altitude).get());
  }
  if (!isnan(aPosition.timestamp)) {
    DOM_CAMERA_LOGI("setting picture timestamp to %lf\n", aPosition.timestamp);
    SetImpl(Parameters::KEY_GPS_TIMESTAMP, nsPrintfCString("%lf", aPosition.timestamp).get());
  }
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const int64_t& aValue)
{
  if (aKey == CAMERA_PARAM_PICTURE_DATETIME) {
    
    
    
    
    time_t time = aValue;
    if (time != aValue) {
      DOM_CAMERA_LOGE("picture date/time '%llu' is too far in the future\n", aValue);
      return NS_ERROR_INVALID_ARG;
    }

    struct tm t;
    if (!localtime_r(&time, &t)) {
      DOM_CAMERA_LOGE("picture date/time couldn't be converted to local time: (%d) %s\n", errno, strerror(errno));
      return NS_ERROR_FAILURE;
    }

    char dateTime[20];
    if (!strftime(dateTime, sizeof(dateTime), "%Y:%m:%d %T", &t)) {
      DOM_CAMERA_LOGE("picture date/time couldn't be converted to string\n");
      return NS_ERROR_FAILURE;
    }

    DOM_CAMERA_LOGI("setting picture date/time to %s\n", dateTime);

    return SetImpl(CAMERA_PARAM_PICTURE_DATETIME, dateTime);
  }

  
  int32_t v = static_cast<int32_t>(aValue);
  if (static_cast<int64_t>(v) != aValue) {
    return NS_ERROR_INVALID_ARG;;
  }
  return SetImpl(aKey, v);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, int64_t& aValue)
{
  int val;
  nsresult rv = GetImpl(aKey, val);
  if (NS_FAILED(rv)) {
    return rv;
  }
  aValue = val;
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const double& aValue)
{
  if (aKey == CAMERA_PARAM_EXPOSURECOMPENSATION) {
    



    int index =
      (aValue - mExposureCompensationMin + mExposureCompensationStep / 2) /
      mExposureCompensationStep + 1;
    DOM_CAMERA_LOGI("Exposure compensation = %f --> index = %d\n", aValue, index);
    return SetImpl(CAMERA_PARAM_EXPOSURECOMPENSATION, index);
  }

  return SetImpl(aKey, aValue);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, double& aValue)
{
  double val;
  int index = 0;
  double focusDistance[3];
  const char* s;
  nsresult rv;

  switch (aKey) {
    case CAMERA_PARAM_ZOOM:
      rv = GetImpl(CAMERA_PARAM_ZOOM, val);
      if (NS_SUCCEEDED(rv)) {
        val /= 100.0;
      } else {
        
        val = 1.0;
      }
      break;

    




    case CAMERA_PARAM_FOCUSDISTANCEFAR:
      ++index;
      

    case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      ++index;
      

    case CAMERA_PARAM_FOCUSDISTANCENEAR:
      rv = GetImpl(aKey, s);
      if (NS_SUCCEEDED(rv)) {
        if (sscanf(s, "%lf,%lf,%lf", &focusDistance[0], &focusDistance[1], &focusDistance[2]) == 3) {
          val = focusDistance[index];
        } else {
          val = 0.0;
        }
      }
      break;

    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      rv = GetImpl(aKey, index);
      if (NS_SUCCEEDED(rv)) {
        if (!index) {
          
          val = NAN;
        } else {
          val = (index - 1) * mExposureCompensationStep + mExposureCompensationMin;
          DOM_CAMERA_LOGI("index = %d --> compensation = %f\n", index, val);
        }
      }
      break;

    default:
      rv = GetImpl(aKey, val);
      break;
  }

  if (NS_SUCCEEDED(rv)) {
    aValue = val;
  }
  return rv;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const int& aValue)
{
  return SetImpl(aKey, aValue);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, int& aValue)
{
  return GetImpl(aKey, aValue);
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const uint32_t& aValue)
{
  if (aValue > INT_MAX) {
    return NS_ERROR_INVALID_ARG;
  }

  int val = static_cast<int>(aValue);
  return SetImpl(aKey, val);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, uint32_t& aValue)
{
  int val;
  nsresult rv = GetImpl(aKey, val);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (val < 0) {
    return NS_ERROR_FAILURE;
  }

  aValue = val;
  return NS_OK;
}

nsresult
ParseItem(const char* aStart, const char* aEnd, ICameraControl::Size* aItem)
{
  if (sscanf(aStart, "%ux%u", &aItem->width, &aItem->height) == 2) {
    return NS_OK;
  }

  DOM_CAMERA_LOGE("Size tuple has bad format: '%s'\n", __func__, __LINE__, aStart);
  return NS_ERROR_FAILURE;
}

nsresult
ParseItem(const char* aStart, const char* aEnd, nsAString* aItem)
{
  if (aEnd) {
    aItem->AssignASCII(aStart, aEnd - aStart);
  } else {
    aItem->AssignASCII(aStart);
  }
  return NS_OK;
}

nsresult
ParseItem(const char* aStart, const char* aEnd, double* aItem)
{
  if (sscanf(aStart, "%lf", aItem) == 1) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

template<class T> nsresult
GonkCameraParameters::GetListAsArray(uint32_t aKey, nsTArray<T>& aArray)
{
  const char* p;
  nsresult rv = GetImpl(aKey, p);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!p) {
    DOM_CAMERA_LOGW("Camera parameter %d not available (value is null)\n", aKey);
    return NS_ERROR_NOT_AVAILABLE;
  }
  if (*p == '\0') {
    DOM_CAMERA_LOGW("Camera parameter %d not available (value is empty string)\n", aKey);
    return NS_ERROR_NOT_AVAILABLE;
  }

  aArray.Clear();
  const char* comma;

  while (p) {
    T* v = aArray.AppendElement();
    if (!v) {
      aArray.Clear();
      return NS_ERROR_OUT_OF_MEMORY;
    }
    comma = strchr(p, ',');
    if (comma != p) {
      rv = ParseItem(p, comma, v);
      if (NS_FAILED(rv)) {
        aArray.Clear();
        return rv;
      }
      p = comma;
    }
    if (p) {
      ++p;
    }
  }

  return NS_OK;
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsTArray<nsString>& aValues)
{
  return GetListAsArray(aKey, aValues);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsTArray<double>& aValues)
{
  return GetListAsArray(aKey, aValues);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsTArray<ICameraControl::Size>& aSizes)
{
  return GetListAsArray(aKey, aSizes);
}

