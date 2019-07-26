















#include <camera/Camera.h>

#include "jsapi.h"
#include "GonkCameraControl.h"
#include "DOMCameraManager.h"
#include "CameraCommon.h"


nsresult
nsDOMCameraManager::GetNumberOfCameras(int32_t& aDeviceCount)
{
  aDeviceCount = android::Camera::getNumberOfCameras();
  return NS_OK;
}

nsresult
nsDOMCameraManager::GetCameraName(uint32_t aDeviceNum, nsCString& aDeviceName)
{
  int32_t count = android::Camera::getNumberOfCameras();
  DOM_CAMERA_LOGI("getListOfCameras : getNumberOfCameras() returned %d\n", count);
  if (aDeviceNum > count) {
    DOM_CAMERA_LOGE("GetCameraName : invalid device number");
    return NS_ERROR_NOT_AVAILABLE;
  }

  android::CameraInfo info;
  int rv = android::Camera::getCameraInfo(aDeviceNum, &info);
  if (rv != 0) {
    DOM_CAMERA_LOGE("GetCameraName : get_camera_info(%d) failed: %d\n", aDeviceNum, rv);
    return NS_ERROR_NOT_AVAILABLE;
  }

  switch (info.facing) {
    case CAMERA_FACING_BACK:
      aDeviceName.Assign("back");
      break;

    case CAMERA_FACING_FRONT:
      aDeviceName.Assign("front");
      break;

    default:
      aDeviceName.Assign("extra-camera-");
      aDeviceName.AppendInt(aDeviceNum);
      break;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsDOMCameraManager::GetListOfCameras(JSContext* cx, JS::Value* _retval)
{
  JSObject* a = JS_NewArrayObject(cx, 0, nullptr);
  uint32_t index = 0;
  int32_t count;

  if (!a) {
    DOM_CAMERA_LOGE("getListOfCameras : Could not create array object");
    return NS_ERROR_OUT_OF_MEMORY;
  }
  count = android::Camera::getNumberOfCameras();
  if (count <= 0) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  DOM_CAMERA_LOGI("getListOfCameras : getNumberOfCameras() returned %d\n", count);
  while (count--) {
    nsCString cameraName;
    nsresult result = GetCameraName(count, cameraName);
    if (result != NS_OK) {
      continue;
    }

    JSString* v = JS_NewStringCopyZ(cx, cameraName.get());
    JS::Value jv;
    if (!cameraName.Compare("back")) {
      index = 0;
    } else if (!cameraName.Compare("front")) {
      index = 1;
    } else {
      static uint32_t extraIndex = 2;
      index = extraIndex++;
    }

    if (!v) {
      DOM_CAMERA_LOGE("getListOfCameras : out of memory populating camera list");
      return NS_ERROR_NOT_AVAILABLE;
    }
    jv = STRING_TO_JSVAL(v);
    if (!JS_SetElement(cx, a, index, &jv)) {
      DOM_CAMERA_LOGE("getListOfCameras : failed building list of cameras");
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  *_retval = OBJECT_TO_JSVAL(a);
  return NS_OK;
}
