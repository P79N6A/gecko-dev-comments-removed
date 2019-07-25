



#ifndef DOM_CAMERA_NSCAMERACAPABILITIES_H
#define DOM_CAMERA_NSCAMERACAPABILITIES_H

#include "CameraControl.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

namespace mozilla {

typedef nsresult (*ParseItemAndAddFunc)(JSContext* aCx, JSObject* aArray, PRUint32 aIndex, const char* aStart, char** aEnd);

class nsCameraCapabilities MOZ_FINAL : public nsICameraCapabilities
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERACAPABILITIES

  nsCameraCapabilities(nsCameraControl* aCamera);

  nsresult ParameterListToNewArray(
    JSContext* cx,
    JSObject** aArray,
    PRUint32 aKey,
    ParseItemAndAddFunc aParseItemAndAdd
  );
  nsresult StringListToNewObject(JSContext* aCx, JS::Value* aArray, PRUint32 aKey);
  nsresult DimensionListToNewObject(JSContext* aCx, JS::Value* aArray, PRUint32 aKey);

private:
  nsCameraCapabilities(const nsCameraCapabilities&) MOZ_DELETE;
  nsCameraCapabilities& operator=(const nsCameraCapabilities&) MOZ_DELETE;

protected:
  
  ~nsCameraCapabilities();
  nsCOMPtr<nsCameraControl> mCamera;
};

} 

#endif 
