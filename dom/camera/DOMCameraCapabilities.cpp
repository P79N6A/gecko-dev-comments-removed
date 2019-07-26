



#include <cstring>
#include <cstdlib>
#include "base/basictypes.h"
#include "nsDOMClassInfo.h"
#include "jsapi.h"
#include "CameraRecorderProfiles.h"
#include "DOMCameraControl.h"
#include "DOMCameraCapabilities.h"
#include "CameraCommon.h"

using namespace mozilla;
using namespace mozilla::dom;

DOMCI_DATA(CameraCapabilities, nsICameraCapabilities)

NS_INTERFACE_MAP_BEGIN(DOMCameraCapabilities)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraCapabilities)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraCapabilities)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(DOMCameraCapabilities)
NS_IMPL_RELEASE(DOMCameraCapabilities)

static nsresult
ParseZoomRatioItemAndAdd(JSContext* aCx, JSObject* aArray, uint32_t aIndex, const char* aStart, char** aEnd)
{
  if (!*aEnd) {
    
    aEnd = nullptr;
  }

  



  double d = strtod(aStart, aEnd);
#if MOZ_WIDGET_GONK
  d /= 100;
#endif

  JS::Value v = JS_NumberValue(d);

  if (!JS_SetElement(aCx, aArray, aIndex, &v)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

static nsresult
ParseStringItemAndAdd(JSContext* aCx, JSObject* aArray, uint32_t aIndex, const char* aStart, char** aEnd)
{
  JSString* s;

  if (*aEnd) {
    s = JS_NewStringCopyN(aCx, aStart, *aEnd - aStart);
  } else {
    s = JS_NewStringCopyZ(aCx, aStart);
  }
  if (!s) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JS::Value v = STRING_TO_JSVAL(s);
  if (!JS_SetElement(aCx, aArray, aIndex, &v)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

static nsresult
ParseDimensionItemAndAdd(JSContext* aCx, JSObject* aArray, uint32_t aIndex, const char* aStart, char** aEnd)
{
  char* x;

  if (!*aEnd) {
    
    aEnd = nullptr;
  }

  JS::Value w = INT_TO_JSVAL(strtol(aStart, &x, 10));
  JS::Value h = INT_TO_JSVAL(strtol(x + 1, aEnd, 10));

  JSObject* o = JS_NewObject(aCx, nullptr, nullptr, nullptr);
  if (!o) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!JS_SetProperty(aCx, o, "width", &w)) {
    return NS_ERROR_FAILURE;
  }
  if (!JS_SetProperty(aCx, o, "height", &h)) {
    return NS_ERROR_FAILURE;
  }

  JS::Value v = OBJECT_TO_JSVAL(o);
  if (!JS_SetElement(aCx, aArray, aIndex, &v)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
DOMCameraCapabilities::ParameterListToNewArray(JSContext* aCx, JSObject** aArray, uint32_t aKey, ParseItemAndAddFunc aParseItemAndAdd)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(aKey);
  if (!value) {
    
    *aArray = nullptr;
    return NS_OK;
  }

  *aArray = JS_NewArrayObject(aCx, 0, nullptr);
  if (!*aArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  const char* p = value;
  uint32_t index = 0;
  nsresult rv;
  char* q;

  while (p) {
    








    q = const_cast<char*>(strchr(p, ','));
    if (q != p) { 
      rv = aParseItemAndAdd(aCx, *aArray, index, p, &q);
      NS_ENSURE_SUCCESS(rv, rv);
      ++index;
    }
    p = q;
    if (p) {
      ++p;
    }
  }

  return JS_FreezeObject(aCx, *aArray) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
DOMCameraCapabilities::StringListToNewObject(JSContext* aCx, JS::Value* aArray, uint32_t aKey)
{
  JSObject* array;

  nsresult rv = ParameterListToNewArray(aCx, &array, aKey, ParseStringItemAndAdd);
  NS_ENSURE_SUCCESS(rv, rv);

  *aArray = OBJECT_TO_JSVAL(array);
  return NS_OK;
}

nsresult
DOMCameraCapabilities::DimensionListToNewObject(JSContext* aCx, JS::Value* aArray, uint32_t aKey)
{
  JSObject* array;
  nsresult rv;

  rv = ParameterListToNewArray(aCx, &array, aKey, ParseDimensionItemAndAdd);
  NS_ENSURE_SUCCESS(rv, rv);

  *aArray = OBJECT_TO_JSVAL(array);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetPreviewSizes(JSContext* cx, JS::Value* aPreviewSizes)
{
  return DimensionListToNewObject(cx, aPreviewSizes, CAMERA_PARAM_SUPPORTED_PREVIEWSIZES);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetPictureSizes(JSContext* cx, JS::Value* aPictureSizes)
{
  return DimensionListToNewObject(cx, aPictureSizes, CAMERA_PARAM_SUPPORTED_PICTURESIZES);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetFileFormats(JSContext* cx, JS::Value* aFileFormats)
{
  return StringListToNewObject(cx, aFileFormats, CAMERA_PARAM_SUPPORTED_PICTUREFORMATS);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetWhiteBalanceModes(JSContext* cx, JS::Value* aWhiteBalanceModes)
{
  return StringListToNewObject(cx, aWhiteBalanceModes, CAMERA_PARAM_SUPPORTED_WHITEBALANCES);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetSceneModes(JSContext* cx, JS::Value* aSceneModes)
{
  return StringListToNewObject(cx, aSceneModes, CAMERA_PARAM_SUPPORTED_SCENEMODES);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetEffects(JSContext* cx, JS::Value* aEffects)
{
  return StringListToNewObject(cx, aEffects, CAMERA_PARAM_SUPPORTED_EFFECTS);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetFlashModes(JSContext* cx, JS::Value* aFlashModes)
{
  return StringListToNewObject(cx, aFlashModes, CAMERA_PARAM_SUPPORTED_FLASHMODES);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetFocusModes(JSContext* cx, JS::Value* aFocusModes)
{
  return StringListToNewObject(cx, aFocusModes, CAMERA_PARAM_SUPPORTED_FOCUSMODES);
}


NS_IMETHODIMP
DOMCameraCapabilities::GetMaxFocusAreas(JSContext* cx, int32_t* aMaxFocusAreas)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(CAMERA_PARAM_SUPPORTED_MAXFOCUSAREAS);
  if (!value) {
    
    *aMaxFocusAreas = 0;
    return NS_OK;
  }

  *aMaxFocusAreas = atoi(value);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetMinExposureCompensation(JSContext* cx, double* aMinExposureCompensation)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION);
  if (!value) {
    
    *aMinExposureCompensation = 0;
    return NS_OK;
  }

  *aMinExposureCompensation = atof(value);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetMaxExposureCompensation(JSContext* cx, double* aMaxExposureCompensation)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(CAMERA_PARAM_SUPPORTED_MAXEXPOSURECOMPENSATION);
  if (!value) {
    
    *aMaxExposureCompensation = 0;
    return NS_OK;
  }

  *aMaxExposureCompensation = atof(value);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetStepExposureCompensation(JSContext* cx, double* aStepExposureCompensation)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(CAMERA_PARAM_SUPPORTED_EXPOSURECOMPENSATIONSTEP);
  if (!value) {
    
    *aStepExposureCompensation = 0;
    return NS_OK;
  }

  *aStepExposureCompensation = atof(value);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetMaxMeteringAreas(JSContext* cx, int32_t* aMaxMeteringAreas)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(CAMERA_PARAM_SUPPORTED_MAXMETERINGAREAS);
  if (!value) {
    
    *aMaxMeteringAreas = 0;
    return NS_OK;
  }

  *aMaxMeteringAreas = atoi(value);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetZoomRatios(JSContext* cx, JS::Value* aZoomRatios)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  const char* value = mCamera->GetParameterConstChar(CAMERA_PARAM_SUPPORTED_ZOOM);
  if (!value || strcmp(value, "true") != 0) {
    
    *aZoomRatios = JSVAL_NULL;
    return NS_OK;
  }

  JSObject* array;

  nsresult rv = ParameterListToNewArray(cx, &array, CAMERA_PARAM_SUPPORTED_ZOOMRATIOS, ParseZoomRatioItemAndAdd);
  NS_ENSURE_SUCCESS(rv, rv);

  *aZoomRatios = OBJECT_TO_JSVAL(array);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetVideoSizes(JSContext* cx, JS::Value* aVideoSizes)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  nsTArray<mozilla::idl::CameraSize> sizes;
  nsresult rv = mCamera->GetVideoSizes(sizes);
  NS_ENSURE_SUCCESS(rv, rv);
  if (sizes.Length() == 0) {
    
    *aVideoSizes = JSVAL_NULL;
    return NS_OK;
  }

  JSObject* array = JS_NewArrayObject(cx, 0, nullptr);
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < sizes.Length(); ++i) {
    JSObject* o = JS_NewObject(cx, nullptr, nullptr, nullptr);
    JS::Value v = INT_TO_JSVAL(sizes[i].width);
    if (!JS_SetProperty(cx, o, "width", &v)) {
      return NS_ERROR_FAILURE;
    }
    v = INT_TO_JSVAL(sizes[i].height);
    if (!JS_SetProperty(cx, o, "height", &v)) {
      return NS_ERROR_FAILURE;
    }

    v = OBJECT_TO_JSVAL(o);
    if (!JS_SetElement(cx, array, i, &v)) {
      return NS_ERROR_FAILURE;
    }
  }

  *aVideoSizes = OBJECT_TO_JSVAL(array);
  return NS_OK;
}


NS_IMETHODIMP
DOMCameraCapabilities::GetRecorderProfiles(JSContext* cx, JS::Value* aRecorderProfiles)
{
  NS_ENSURE_TRUE(mCamera, NS_ERROR_NOT_AVAILABLE);

  nsRefPtr<RecorderProfileManager> profileMgr = mCamera->GetRecorderProfileManager();
  if (!profileMgr) {
    *aRecorderProfiles = JSVAL_NULL;
    return NS_OK;
  }

  JSObject* o = nullptr;  
  nsresult rv = profileMgr->GetJsObject(cx, &o);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRecorderProfiles = OBJECT_TO_JSVAL(o);
  return NS_OK;
}
