



#include "DOMCameraControl.h"
#include "DOMCameraManager.h"
#include "nsDOMClassInfo.h"
#include "DictionaryHelpers.h"
#include "CameraCommon.h"

using namespace mozilla;

DOMCI_DATA(CameraManager, nsIDOMCameraManager)

NS_INTERFACE_MAP_BEGIN(nsDOMCameraManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCameraManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraManager)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMCameraManager)
NS_IMPL_RELEASE(nsDOMCameraManager)







#ifdef PR_LOGGING
PRLogModuleInfo* gCameraLog;
#endif







nsDOMCameraManager::nsDOMCameraManager(uint64_t aWindowId)
  : mWindowId(aWindowId)
{
  
  DOM_CAMERA_LOGT("%s:%d : this=%p, windowId=%llx\n", __func__, __LINE__, this, mWindowId);
}

nsDOMCameraManager::~nsDOMCameraManager()
{
  
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
}

void
nsDOMCameraManager::OnNavigation(uint64_t aWindowId)
{
  
}


already_AddRefed<nsDOMCameraManager>
nsDOMCameraManager::Create(uint64_t aWindowId)
{
  

#ifdef PR_LOGGING
  if (!gCameraLog) {
    gCameraLog = PR_LOG_DEFINE("Camera");
  }
#endif
  nsRefPtr<nsDOMCameraManager> cameraManager = new nsDOMCameraManager(aWindowId);
  return cameraManager.forget();
}


NS_IMETHODIMP
nsDOMCameraManager::GetCamera(const JS::Value& aOptions, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  uint32_t cameraId = 0;  
  CameraSelector selector;

  nsresult rv = selector.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  if (selector.camera.EqualsASCII("front")) {
    cameraId = 1;
  }

  
  if (!mCameraThread) {
    rv = NS_NewThread(getter_AddRefs(mCameraThread));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

  
  nsCOMPtr<nsICameraControl> cameraControl = new nsDOMCameraControl(cameraId, mCameraThread, onSuccess, onError);

  return NS_OK;
}
