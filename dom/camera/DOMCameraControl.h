



#ifndef DOM_CAMERA_DOMCAMERACONTROL_H
#define DOM_CAMERA_DOMCAMERACONTROL_H

#include "base/basictypes.h"
#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "DictionaryHelpers.h"
#include "ICameraControl.h"
#include "DOMCameraPreview.h"
#include "nsIDOMCameraManager.h"
#include "CameraCommon.h"

namespace mozilla {

using namespace mozilla;
using namespace dom;


class nsDOMCameraControl : public nsICameraControl
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMCameraControl)
  NS_DECL_NSICAMERACONTROL

  nsDOMCameraControl(uint32_t aCameraId, nsIThread* aCameraThread, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult Result(nsresult aResult, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError);

protected:
  virtual ~nsDOMCameraControl();

private:
  nsDOMCameraControl(const nsDOMCameraControl&) MOZ_DELETE;
  nsDOMCameraControl& operator=(const nsDOMCameraControl&) MOZ_DELETE;

protected:
  
  nsRefPtr<ICameraControl>        mCameraControl; 
  nsCOMPtr<nsICameraCapabilities> mDOMCapabilities;
};

} 

#endif 
