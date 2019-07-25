



#include "CameraControl.h"
#include "DOMCameraManager.h"
#include "nsDOMClassInfo.h"
#include "DictionaryHelpers.h"

#undef DOM_CAMERA_LOG_LEVEL
#define DOM_CAMERA_LOG_LEVEL  DOM_CAMERA_LOG_NOTHING
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







nsDOMCameraManager::nsDOMCameraManager(uint64_t aWindowId)
  : mWindowId(aWindowId)
{
  
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

nsDOMCameraManager::~nsDOMCameraManager()
{
  
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

void
nsDOMCameraManager::OnNavigation(uint64_t aWindowId)
{
  
}


already_AddRefed<nsDOMCameraManager>
nsDOMCameraManager::Create(uint64_t aWindowId)
{
  

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

  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

  nsCOMPtr<nsIRunnable> getCameraTask = new GetCameraTask(cameraId, onSuccess, onError, mCameraThread);
  mCameraThread->Dispatch(getCameraTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}
