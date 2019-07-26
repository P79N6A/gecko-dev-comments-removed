



#include "nsDebug.h"
#include "nsPIDOMWindow.h"
#include "mozilla/Services.h"
#include "nsObserverService.h"
#include "nsIPermissionManager.h"
#include "DOMCameraControl.h"
#include "DOMCameraManager.h"
#include "nsDOMClassInfo.h"
#include "DictionaryHelpers.h"
#include "CameraCommon.h"
#include "mozilla/dom/CameraManagerBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(nsDOMCameraManager, mWindow)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMCameraManager)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCameraManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCameraManager)







PRLogModuleInfo*
GetCameraLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("Camera");
  return sLog;
}







WindowTable* nsDOMCameraManager::sActiveWindows = nullptr;

nsDOMCameraManager::nsDOMCameraManager(nsPIDOMWindow* aWindow)
  : mWindowId(aWindow->WindowID())
  , mCameraThread(nullptr)
  , mWindow(aWindow)
{
  
  DOM_CAMERA_LOGT("%s:%d : this=%p, windowId=%llx\n", __func__, __LINE__, this, mWindowId);
  SetIsDOMBinding();
}

nsDOMCameraManager::~nsDOMCameraManager()
{
  
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this, "xpcom-shutdown");
  }
}

bool
nsDOMCameraManager::CheckPermission(nsPIDOMWindow* aWindow)
{
  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromWindow(aWindow, "camera", &permission);
  if (permission != nsIPermissionManager::ALLOW_ACTION) {
    return false;
  }

  return true;
}


already_AddRefed<nsDOMCameraManager>
nsDOMCameraManager::CreateInstance(nsPIDOMWindow* aWindow)
{
  
  if (!sActiveWindows) {
    sActiveWindows = new WindowTable();
  }

  nsRefPtr<nsDOMCameraManager> cameraManager =
    new nsDOMCameraManager(aWindow);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  obs->AddObserver(cameraManager, "xpcom-shutdown", true);

  return cameraManager.forget();
}

void
nsDOMCameraManager::GetCamera(const CameraSelector& aOptions,
                              nsICameraGetCameraCallback* onSuccess,
                              const Optional<nsICameraErrorCallback*>& onError,
                              ErrorResult& aRv)
{
  uint32_t cameraId = 0;  
  if (aOptions.mCamera.EqualsLiteral("front")) {
    cameraId = 1;
  }

  
  if (!mCameraThread) {
    aRv = NS_NewThread(getter_AddRefs(mCameraThread));
    if (aRv.Failed()) {
      return;
    }
  }

  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

  
  nsRefPtr<nsDOMCameraControl> cameraControl =
    new nsDOMCameraControl(cameraId, mCameraThread,
                           onSuccess, onError.WasPassed() ? onError.Value() : nullptr, mWindow);

  Register(cameraControl);
}

void
nsDOMCameraManager::Register(nsDOMCameraControl* aDOMCameraControl)
{
  DOM_CAMERA_LOGI(">>> Register( aDOMCameraControl = %p ) mWindowId = 0x%llx\n", aDOMCameraControl, mWindowId);
  MOZ_ASSERT(NS_IsMainThread());

  
  CameraControls* controls = sActiveWindows->Get(mWindowId);
  if (!controls) {
    controls = new CameraControls;
    sActiveWindows->Put(mWindowId, controls);
  }
  controls->AppendElement(aDOMCameraControl);
}

void
nsDOMCameraManager::Shutdown(uint64_t aWindowId)
{
  DOM_CAMERA_LOGI(">>> Shutdown( aWindowId = 0x%llx )\n", aWindowId);
  MOZ_ASSERT(NS_IsMainThread());

  CameraControls* controls = sActiveWindows->Get(aWindowId);
  if (!controls) {
    return;
  }

  uint32_t length = controls->Length();
  for (uint32_t i = 0; i < length; i++) {
    nsRefPtr<nsDOMCameraControl> cameraControl = controls->ElementAt(i);
    cameraControl->Shutdown();
  }
  controls->Clear();

  sActiveWindows->Remove(aWindowId);
}

void
nsDOMCameraManager::XpComShutdown()
{
  DOM_CAMERA_LOGI(">>> XPCOM Shutdown\n");
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  obs->RemoveObserver(this, "xpcom-shutdown");

  delete sActiveWindows;
  sActiveWindows = nullptr;
}

nsresult
nsDOMCameraManager::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  if (strcmp(aTopic, "xpcom-shutdown") == 0) {
    XpComShutdown();
  }
  return NS_OK;
}

void
nsDOMCameraManager::OnNavigation(uint64_t aWindowId)
{
  DOM_CAMERA_LOGI(">>> OnNavigation event\n");
  Shutdown(aWindowId);
}

bool
nsDOMCameraManager::IsWindowStillActive(uint64_t aWindowId)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!sActiveWindows) {
    return false;
  }

  return !!sActiveWindows->Get(aWindowId);
}

JSObject*
nsDOMCameraManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return CameraManagerBinding::Wrap(aCx, aScope, this);
}
