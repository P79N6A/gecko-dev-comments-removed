















#include <camera/Camera.h>

#include "jsapi.h"
#include "GonkCameraControl.h"
#include "DOMCameraManager.h"
#include "CameraCommon.h"




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

  DOM_CAMERA_LOGI("getListOfCameras : get_number_of_cameras() returned %d\n", count);
  while (count--) {
    android::CameraInfo info;
    int rv = android::Camera::getCameraInfo(count, &info);
    if (rv != 0) {
      DOM_CAMERA_LOGE("getListOfCameras : get_camera_info(%d) failed: %d\n", count, rv);
      continue;
    }

    JSString* v;
    jsval jv;

    switch (info.facing) {
      case CAMERA_FACING_BACK:
        v = JS_NewStringCopyZ(cx, "back");
        index = 0;
        break;

      case CAMERA_FACING_FRONT:
        v = JS_NewStringCopyZ(cx, "front");
        index = 1;
        break;

      default:
        
        {
          static uint32_t extraIndex = 2;
          nsCString s;
          s.AppendPrintf("extra-camera-%d", count);
          v = JS_NewStringCopyZ(cx, s.get());
          index = extraIndex++;
        }
        break;
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
