















#include "GonkCameraParameters.h"
#include "camera/CameraParameters.h"
#include "CameraPreferences.h"
#include "ICameraControl.h"
#include "CameraCommon.h"
#include "mozilla/Hal.h"

using namespace mozilla;
using namespace android;

 bool
GonkCameraParameters::IsLowMemoryPlatform()
{
  bool testIsLowMem = false;
  CameraPreferences::GetPref("camera.control.test.is_low_memory", testIsLowMem);
  if (testIsLowMem) {
    NS_WARNING("Forcing low-memory platform camera preferences");
    return true;
  }

  uint32_t lowMemoryThresholdBytes = 0;
  CameraPreferences::GetPref("camera.control.low_memory_thresholdMB",
                             lowMemoryThresholdBytes);
  lowMemoryThresholdBytes *= 1024 * 1024;
  if (lowMemoryThresholdBytes) {
    uint32_t totalMemoryBytes = hal::GetTotalSystemMemory();
    if (totalMemoryBytes < lowMemoryThresholdBytes) {
      DOM_CAMERA_LOGI("Low-memory platform with %d bytes of RAM (threshold: <%d bytes)\n",
        totalMemoryBytes, lowMemoryThresholdBytes);
      return true;
    }
  }

  return false;
}

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
    case CAMERA_PARAM_ISOMODE:
      
      
      return "iso";
    case CAMERA_PARAM_LUMINANCE:
      return "luminance-condition";
    case CAMERA_PARAM_SCENEMODE_HDR_RETURNNORMALPICTURE:
      
      
      return "hdr-need-1x";
    case CAMERA_PARAM_RECORDINGHINT:
      return KEY_RECORDING_HINT;
    case CAMERA_PARAM_PICTURE_QUALITY:
      return KEY_JPEG_QUALITY;
    case CAMERA_PARAM_PREFERRED_PREVIEWSIZE_FOR_VIDEO:
      return KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO;

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
    case CAMERA_PARAM_SUPPORTED_MAXDETECTEDFACES:
      return KEY_MAX_NUM_DETECTED_FACES_HW;
    case CAMERA_PARAM_SUPPORTED_JPEG_THUMBNAIL_SIZES:
      return KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES;
    case CAMERA_PARAM_SUPPORTED_ISOMODES:
      
      
      return "iso-values";
    default:
      DOM_CAMERA_LOGE("Unhandled camera parameter value %u\n", aKey);
      return nullptr;
  }
}

GonkCameraParameters::GonkCameraParameters()
  : mLock(PR_NewRWLock(PR_RWLOCK_RANK_NONE, "GonkCameraParameters.Lock"))
  , mDirty(false)
  , mInitialized(false)
  , mExposureCompensationStep(0.0)
{
  MOZ_COUNT_CTOR(GonkCameraParameters);
  if (!mLock) {
    MOZ_CRASH("Out of memory getting new PRRWLock");
  }
}

GonkCameraParameters::~GonkCameraParameters()
{
  MOZ_COUNT_DTOR(GonkCameraParameters);
  mIsoModeMap.Clear();
  MOZ_ASSERT(mLock, "mLock missing in ~GonkCameraParameters()");
  if (mLock) {
    PR_DestroyRWLock(mLock);
    mLock = nullptr;
  }
}

nsresult
GonkCameraParameters::MapIsoToGonk(const nsAString& aIso, nsACString& aIsoOut)
{
  nsCString* s;
  if (mIsoModeMap.Get(aIso, &s)) {
    if (!s) {
      DOM_CAMERA_LOGE("ISO mode '%s' maps to null Gonk ISO value\n",
        NS_LossyConvertUTF16toASCII(aIso).get());
      return NS_ERROR_FAILURE;
    }

    aIsoOut = *s;
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

nsresult
GonkCameraParameters::MapIsoFromGonk(const char* aIso, nsAString& aIsoOut)
{
  if (strcmp(aIso, "ISO_HJR") == 0) {
    aIsoOut.AssignASCII("hjr");
  } else if (strcmp(aIso, "auto") == 0) {
    aIsoOut.AssignASCII("auto");
  } else {
    unsigned int iso;
    char ignored;
    
    if (sscanf(aIso, "ISO%u%c", &iso, &ignored) != 1 &&
        sscanf(aIso, "%u%c", &iso, &ignored) != 1) {
      return NS_ERROR_INVALID_ARG;
    }
    aIsoOut.Truncate(0);
    aIsoOut.AppendInt(iso);
  }

  return NS_OK;
}



nsresult
GonkCameraParameters::Initialize()
{
  nsresult rv;

  rv = GetImpl(Parameters::KEY_EXPOSURE_COMPENSATION_STEP, mExposureCompensationStep);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to initialize exposure compensation step size");
    mExposureCompensationStep = 0.0;
  }
  rv = GetImpl(Parameters::KEY_MIN_EXPOSURE_COMPENSATION, mExposureCompensationMinIndex);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to initialize minimum exposure compensation index");
    mExposureCompensationMinIndex = 0;
  }
  rv = GetImpl(Parameters::KEY_MAX_EXPOSURE_COMPENSATION, mExposureCompensationMaxIndex);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to initialize maximum exposure compensation index");
    mExposureCompensationMaxIndex = 0;
  }

  rv = GetListAsArray(CAMERA_PARAM_SUPPORTED_ZOOMRATIOS, mZoomRatios);
  if (NS_FAILED(rv)) {
    
    mZoomRatios.Clear();
  }
  for (uint32_t i = 1; i < mZoomRatios.Length(); ++i) {
    
    if (mZoomRatios[i] < mZoomRatios[i - 1]) {
      NS_WARNING("Zoom ratios list is out of order, discarding");
      DOM_CAMERA_LOGE("zoom[%d]=%fx < zoom[%d]=%fx is out of order\n",
        i, mZoomRatios[i] / 100.0, i - 1, mZoomRatios[i - 1] / 100.0);
      mZoomRatios.Clear();
      break;
    }
  }
  if (mZoomRatios.Length() == 0) {
    
    *mZoomRatios.AppendElement() = 100;
  }

  
  
  
  nsString s;
  nsTArray<nsCString> isoModes;
  GetListAsArray(CAMERA_PARAM_SUPPORTED_ISOMODES, isoModes);
  for (nsTArray<nsCString>::size_type i = 0; i < isoModes.Length(); ++i) {
    rv = MapIsoFromGonk(isoModes[i].get(), s);
    if (NS_FAILED(rv)) {
      DOM_CAMERA_LOGW("Unrecognized ISO mode value '%s'\n", isoModes[i].get());
      continue;
    }
    *mIsoModes.AppendElement() = s;
    mIsoModeMap.Put(s, new nsCString(isoModes[i]));
  }

  GetListAsArray(CAMERA_PARAM_SUPPORTED_SCENEMODES, mSceneModes);
  if (IsLowMemoryPlatform()) {
    bool hdrRemoved = false;
    while (mSceneModes.RemoveElement(NS_LITERAL_STRING("hdr"))) {
      hdrRemoved = true;
    }
    if (hdrRemoved) {
      DOM_CAMERA_LOGI("Disabling HDR support due to low memory\n");
    }
  }

  mInitialized = true;
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const nsAString& aValue)
{
  switch (aKey) {
    case CAMERA_PARAM_ISOMODE:
      {
        nsAutoCString v;
        nsresult rv = MapIsoToGonk(aValue, v);
        if (NS_FAILED(rv)) {
          return rv;
        }
        return SetImpl(aKey, v.get());
      }

    case CAMERA_PARAM_SCENEMODE:
      if (mSceneModes.IndexOf(aValue) == nsTArray<nsString>::NoIndex) {
        return NS_ERROR_INVALID_ARG;
      }
      

    default:
      return SetImpl(aKey, NS_ConvertUTF16toUTF8(aValue).get());
  }
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsAString& aValue)
{
  const char* val;
  nsresult rv = GetImpl(aKey, val);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (aKey == CAMERA_PARAM_ISOMODE) {
    rv = MapIsoFromGonk(val, aValue);
  } else if(val) {
    aValue.AssignASCII(val);
  } else {
    aValue.Truncate(0);
  }
  return rv;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const ICameraControl::Size& aSize)
{
  if (aSize.width > INT_MAX || aSize.height > INT_MAX) {
    
    DOM_CAMERA_LOGE("Camera parameter aKey=%d out of bounds (width=%u, height=%u)\n",
      aKey, aSize.width, aSize.height);
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
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (width < 0) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    rv = GetImpl(Parameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (height < 0) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    aSize.width = static_cast<uint32_t>(width);
    aSize.height = static_cast<uint32_t>(height);
    return NS_OK;
  }

  const char* value;
  rv = GetImpl(aKey, value);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!value || *value == '\0') {
    DOM_CAMERA_LOGW("Camera parameter aKey=%d not available\n", aKey);
    return NS_ERROR_NOT_AVAILABLE;
  }
  if (sscanf(value, "%ux%u", &aSize.width, &aSize.height) != 2) {
    DOM_CAMERA_LOGE("Camera parameter aKey=%d size tuple '%s' is invalid\n", aKey, value);
    return NS_ERROR_NOT_AVAILABLE;
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
    s.AppendPrintf("(%d,%d,%d,%d,%d),", r->left, r->top, r->right, r->bottom, r->weight);
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
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!value || *value == '\0') {
    DOM_CAMERA_LOGW("Camera parameter aKey=%d not available\n", aKey);
    return NS_ERROR_NOT_AVAILABLE;
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
    if (sscanf(p, "(%d,%d,%d,%d,%u)", &r->left, &r->top, &r->right, &r->bottom, &r->weight) != 5) {
      DOM_CAMERA_LOGE("Camera parameter aKey=%d region tuple has bad format: '%s'\n", aKey, p);
      aRegions.Clear();
      return NS_ERROR_NOT_AVAILABLE;
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
  } else {
    ClearImpl(Parameters::KEY_GPS_LATITUDE);
  }
  if (!isnan(aPosition.longitude)) {
    DOM_CAMERA_LOGI("setting picture longitude to %lf\n", aPosition.longitude);
    SetImpl(Parameters::KEY_GPS_LONGITUDE, nsPrintfCString("%lf", aPosition.longitude).get());
  } else {
    ClearImpl(Parameters::KEY_GPS_LONGITUDE);
  }
  if (!isnan(aPosition.altitude)) {
    DOM_CAMERA_LOGI("setting picture altitude to %lf\n", aPosition.altitude);
    SetImpl(Parameters::KEY_GPS_ALTITUDE, nsPrintfCString("%lf", aPosition.altitude).get());
  } else {
    ClearImpl(Parameters::KEY_GPS_ALTITUDE);
  }
  if (!isnan(aPosition.timestamp)) {
    DOM_CAMERA_LOGI("setting picture timestamp to %lf\n", aPosition.timestamp);
    SetImpl(Parameters::KEY_GPS_TIMESTAMP, nsPrintfCString("%lf", aPosition.timestamp).get());
  } else {
    ClearImpl(Parameters::KEY_GPS_TIMESTAMP);
  }

  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const int64_t& aValue)
{
  switch (aKey) {
    case CAMERA_PARAM_PICTURE_DATETIME:
      {
        
        
        
        
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

    case CAMERA_PARAM_ISOMODE:
      {
        if (aValue > INT32_MAX) {
          DOM_CAMERA_LOGW("Can't set ISO mode = %lld, too big\n", aValue);
          return NS_ERROR_INVALID_ARG;
        }

        nsString s;
        s.AppendInt(aValue);
        return SetTranslated(CAMERA_PARAM_ISOMODE, s);
      }
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
  int index;
  int value;

  switch (aKey) {
    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      if (mExposureCompensationStep == 0.0) {
        DOM_CAMERA_LOGE("Exposure compensation not supported, can't set EV=%f\n", aValue);
        return NS_ERROR_NOT_AVAILABLE;
      }

      



      {
        double i = round(aValue / mExposureCompensationStep);
        if (i < mExposureCompensationMinIndex) {
          index = mExposureCompensationMinIndex;
        } else if (i > mExposureCompensationMaxIndex) {
          index = mExposureCompensationMaxIndex;
        } else {
          index = i;
        }
      }
      DOM_CAMERA_LOGI("Exposure compensation = %f --> index = %d\n", aValue, index);
      return SetImpl(CAMERA_PARAM_EXPOSURECOMPENSATION, index);

    case CAMERA_PARAM_ZOOM:
      {
        



        value = aValue * 100.0;

        if (value <= mZoomRatios[0]) {
          index = 0;
        } else if (value >= mZoomRatios.LastElement()) {
          index = mZoomRatios.Length() - 1;
        } else {
          
          int bottom = 0;
          int top = mZoomRatios.Length() - 1;

          while (top >= bottom) {
            index = (top + bottom) / 2;
            if (value == mZoomRatios[index]) {
              
              break;
            }
            if (value > mZoomRatios[index] && value < mZoomRatios[index + 1]) {
              
              break;
            }
            if (value > mZoomRatios[index]) {
              bottom = index + 1;
            } else {
              top = index - 1;
            }
          }
        }
        DOM_CAMERA_LOGI("Zoom = %fx --> index = %d\n", aValue, index);
      }
      return SetImpl(CAMERA_PARAM_ZOOM, index);

    case CAMERA_PARAM_PICTURE_QUALITY:
      {
        
        index = (aValue + 0.005) * 99.0 + 1.0;
        if (aValue < 0.0) {
          index = 1;
        } else if (aValue > 1.0) {
          index = 100;
        }
        DOM_CAMERA_LOGI("Picture quality = %f --> index = %d\n", aValue, index);
      }
      return SetImpl(CAMERA_PARAM_PICTURE_QUALITY, index);
  }

  return SetImpl(aKey, aValue);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, double& aValue)
{
  double val = 0.0; 
  int index = 0;
  double focusDistance[3];
  const char* s;
  nsresult rv;

  switch (aKey) {
    case CAMERA_PARAM_ZOOM:
      rv = GetImpl(aKey, index);
      if (NS_SUCCEEDED(rv) && index >= 0) {
        val = mZoomRatios[index] / 100.0;
      } else {
        
        val = 1.0;
        rv = NS_OK;
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
    case CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION:
    case CAMERA_PARAM_SUPPORTED_MAXEXPOSURECOMPENSATION:
      if (mExposureCompensationStep == 0.0) {
        DOM_CAMERA_LOGE("Exposure compensation not supported, can't get EV\n");
        return NS_ERROR_NOT_AVAILABLE;
      }
      rv = GetImpl(aKey, index);
      if (NS_SUCCEEDED(rv)) {
        val = index * mExposureCompensationStep;
        DOM_CAMERA_LOGI("exposure compensation (aKey=%d): index=%d --> EV=%f\n", aKey, index, val);
      }
      break;

    case CAMERA_PARAM_PICTURE_QUALITY:
      
      rv = GetImpl(aKey, index);
      if (index < 2) {
        val = 0.0;
      } else if (index > 99) {
        val = 1.0;
      } else {
        val = static_cast<double>(index - 1) / 99.0;
      }
      DOM_CAMERA_LOGI("index = %d --> picture quality = %f\n", index, val);
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
    return NS_ERROR_NOT_AVAILABLE;
  }

  aValue = val;
  return NS_OK;
}


nsresult
GonkCameraParameters::SetTranslated(uint32_t aKey, const bool& aValue)
{
  return SetImpl(aKey, aValue);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, bool& aValue)
{
  return GetImpl(aKey, aValue);
}

nsresult
ParseItem(const char* aStart, const char* aEnd, ICameraControl::Size* aItem)
{
  if (sscanf(aStart, "%ux%u", &aItem->width, &aItem->height) == 2) {
    return NS_OK;
  }

  DOM_CAMERA_LOGE("Size tuple has bad format: '%s'\n", aStart);
  return NS_ERROR_NOT_AVAILABLE;
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
ParseItem(const char* aStart, const char* aEnd, nsACString* aItem)
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

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult
ParseItem(const char* aStart, const char* aEnd, int* aItem)
{
  if (sscanf(aStart, "%d", aItem) == 1) {
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}

template<class T> nsresult
GonkCameraParameters::GetListAsArray(uint32_t aKey, nsTArray<T>& aArray)
{
  const char* p;
  nsresult rv = GetImpl(aKey, p);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aArray.Clear();

  
  if (!p) {
    DOM_CAMERA_LOGI("Camera parameter %d not available (value is null)\n", aKey);
    return NS_OK;
  }
  if (*p == '\0') {
    DOM_CAMERA_LOGI("Camera parameter %d not available (value is empty string)\n", aKey);
    return NS_OK;
  }

  const char* comma;

  while (p) {
    
    T* v = aArray.AppendElement();
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
  switch (aKey) {
    case CAMERA_PARAM_SUPPORTED_ISOMODES:
      aValues = mIsoModes;
      return NS_OK;

    case CAMERA_PARAM_SUPPORTED_SCENEMODES:
      aValues = mSceneModes;
      return NS_OK;

    default:
      return GetListAsArray(aKey, aValues);
  }
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsTArray<double>& aValues)
{
  if (aKey == CAMERA_PARAM_SUPPORTED_ZOOMRATIOS) {
    aValues.Clear();
    for (uint32_t i = 0; i < mZoomRatios.Length(); ++i) {
      *aValues.AppendElement() = mZoomRatios[i] / 100.0;
    }
    return NS_OK;
  }

  return GetListAsArray(aKey, aValues);
}

nsresult
GonkCameraParameters::GetTranslated(uint32_t aKey, nsTArray<ICameraControl::Size>& aSizes)
{
  return GetListAsArray(aKey, aSizes);
}

