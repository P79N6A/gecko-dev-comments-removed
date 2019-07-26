



#ifndef DOM_CAMERA_DOMCAMERACAPABILITIES_H
#define DOM_CAMERA_DOMCAMERACAPABILITIES_H

#include "ICameraControl.h"
#include "nsAutoPtr.h"
#include "CameraCommon.h"

namespace mozilla {

typedef nsresult (*ParseItemAndAddFunc)(JSContext* aCx, JSObject* aArray, uint32_t aIndex, const char* aStart, char** aEnd);

class DOMCameraCapabilities MOZ_FINAL : public nsICameraCapabilities
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERACAPABILITIES

  DOMCameraCapabilities(ICameraControl* aCamera)
    : mCamera(aCamera)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  nsresult ParameterListToNewArray(
    JSContext* cx,
    JSObject** aArray,
    uint32_t aKey,
    ParseItemAndAddFunc aParseItemAndAdd
  );
  nsresult StringListToNewObject(JSContext* aCx, JS::Value* aArray, uint32_t aKey);
  nsresult DimensionListToNewObject(JSContext* aCx, JS::Value* aArray, uint32_t aKey);

private:
  DOMCameraCapabilities(const DOMCameraCapabilities&) MOZ_DELETE;
  DOMCameraCapabilities& operator=(const DOMCameraCapabilities&) MOZ_DELETE;

protected:
  
  ~DOMCameraCapabilities()
  {
    
    DOM_CAMERA_LOGT("%s:%d : this=%p, mCamera=%p\n", __func__, __LINE__, this, mCamera.get());
  }

  nsRefPtr<ICameraControl> mCamera;
};

} 

#endif 
