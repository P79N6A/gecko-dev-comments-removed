



#include "nsDOMClassInfoID.h"
#include "CameraControl.h"
#include "CameraCapabilities.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

DOMCI_DATA(CameraCapabilities, nsICameraCapabilities)

NS_INTERFACE_MAP_BEGIN(nsCameraCapabilities)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraCapabilities)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraCapabilities)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsCameraCapabilities)
NS_IMPL_RELEASE(nsCameraCapabilities)

nsCameraCapabilities::nsCameraCapabilities(nsCameraControl* aCamera)
  : mCamera(aCamera)
{
  
  DOM_CAMERA_LOGI("%s:%d : FALLBACK CAMERA CAPABILITIES\n", __func__, __LINE__);
}

nsCameraCapabilities::~nsCameraCapabilities()
{
  
}


NS_IMETHODIMP
nsCameraCapabilities::GetPreviewSizes(JSContext* cx, JS::Value* aPreviewSizes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetPictureSizes(JSContext* cx, JS::Value* aPictureSizes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetFileFormats(JSContext* cx, JS::Value* aFileFormats)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetWhiteBalanceModes(JSContext* cx, JS::Value* aWhiteBalanceModes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetSceneModes(JSContext* cx, JS::Value* aSceneModes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetEffects(JSContext* cx, JS::Value* aEffects)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetFlashModes(JSContext* cx, JS::Value* aFlashModes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetFocusModes(JSContext* cx, JS::Value* aFocusModes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetMaxFocusAreas(JSContext* cx, int32_t* aMaxFocusAreas)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetMinExposureCompensation(JSContext* cx, double* aMinExposureCompensation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetMaxExposureCompensation(JSContext* cx, double* aMaxExposureCompensation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetStepExposureCompensation(JSContext* cx, double* aStepExposureCompensation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetMaxMeteringAreas(JSContext* cx, int32_t* aMaxMeteringAreas)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetZoomRatios(JSContext* cx, JS::Value* aZoomRatios)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCameraCapabilities::GetVideoSizes(JSContext* cx, JS::Value* aVideoSizes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
