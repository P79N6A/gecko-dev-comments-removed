















#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPermissionController.h>
#include <private/android_filesystem_config.h>
#include "GonkPermission.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/SyncRunnable.h"
#include "nsIAppsService.h"
#include "mozIApplication.h"
#include "nsThreadUtils.h"

#undef LOG
#include <android/log.h>
#define ALOGE(args...)  __android_log_print(ANDROID_LOG_ERROR, "gonkperm" , ## args)

using namespace android;
using namespace mozilla;




class GonkPermissionChecker : public nsRunnable {
  int32_t mPid;
  bool mCanUseCamera;

  explicit GonkPermissionChecker(int32_t pid)
    : mPid(pid)
    , mCanUseCamera(false)
  {
  }

public:
  static already_AddRefed<GonkPermissionChecker> Inspect(int32_t pid)
  {
    nsRefPtr<GonkPermissionChecker> that = new GonkPermissionChecker(pid);
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    MOZ_ASSERT(mainThread);
    SyncRunnable::DispatchToThread(mainThread, that);
    return that.forget();
  }

  bool CanUseCamera()
  {
    return mCanUseCamera;
  }

  NS_IMETHOD Run();
};

NS_IMETHODIMP
GonkPermissionChecker::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  dom::ContentParent *contentParent = nullptr;
  {
    nsTArray<dom::ContentParent*> parents;
    dom::ContentParent::GetAll(parents);
    for (uint32_t i = 0; i < parents.Length(); ++i) {
      if (parents[i]->Pid() == mPid) {
	contentParent = parents[i];
	break;
      }
    }
  }
  if (!contentParent) {
    ALOGE("pid=%d denied: can't find ContentParent", mPid);
    return NS_OK;
  }

  
  for (uint32_t i = 0; i < contentParent->ManagedPBrowserParent().Length(); i++) {
    dom::TabParent *tabParent =
      static_cast<dom::TabParent*>(contentParent->ManagedPBrowserParent()[i]);
    nsCOMPtr<mozIApplication> mozApp = tabParent->GetOwnOrContainingApp();
    if (!mozApp) {
      continue;
    }

    
    bool appCanUseCamera;
    nsresult rv = mozApp->HasPermission("camera", &appCanUseCamera);
    if (NS_SUCCEEDED(rv) && appCanUseCamera) {
      mCanUseCamera = true;
      return NS_OK;
    }
  }
  return NS_OK;
}

bool
GonkPermissionService::checkPermission(const String16& permission, int32_t pid,
                                     int32_t uid)
{
  
  if (0 == uid) {
    return true;
  }

  String8 perm8(permission);

  
  if (uid == AID_RADIO &&
      perm8 == "android.permission.MODIFY_AUDIO_SETTINGS") {
    return true;
  }

  
  if (uid < AID_APP) {
    ALOGE("%s for pid=%d,uid=%d denied: not an app",
      String8(permission).string(), pid, uid);
    return false;
  }

  
  if (perm8 != "android.permission.CAMERA" &&
    perm8 != "android.permission.RECORD_AUDIO") {
    ALOGE("%s for pid=%d,uid=%d denied: unsupported permission",
      String8(permission).string(), pid, uid);
    return false;
  }

  
  
  
  PermissionGrant permGrant(perm8.string(), pid);
  if (nsTArray<PermissionGrant>::NoIndex != mGrantArray.IndexOf(permGrant)) {
    mGrantArray.RemoveElement(permGrant);
    return true;
  }

  
  
  nsRefPtr<GonkPermissionChecker> checker =
    GonkPermissionChecker::Inspect(pid);
  bool canUseCamera = checker->CanUseCamera();
  if (!canUseCamera) {
    ALOGE("%s for pid=%d,uid=%d denied: not granted by user or app manifest",
      String8(permission).string(), pid, uid);
  }
  return canUseCamera;
}

static GonkPermissionService* gGonkPermissionService = NULL;


void
GonkPermissionService::instantiate()
{
  defaultServiceManager()->addService(String16(getServiceName()),
    GetInstance());
}


GonkPermissionService*
GonkPermissionService::GetInstance()
{
  if (!gGonkPermissionService) {
    gGonkPermissionService = new GonkPermissionService();
  }
  return gGonkPermissionService;
}

void
GonkPermissionService::addGrantInfo(const char* permission, int32_t pid)
{
  mGrantArray.AppendElement(PermissionGrant(permission, pid));
}
