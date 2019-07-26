





#ifndef DOM_CAMERA_DOMCAMERAMANAGER_H
#define DOM_CAMERA_DOMCAMERAMANAGER_H

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIThread.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"
#include "nsHashKeys.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsIDOMCameraManager.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class nsPIDOMWindow;

namespace mozilla {
class nsDOMCameraControl;
}

typedef nsTArray<nsRefPtr<mozilla::nsDOMCameraControl> > CameraControls;
typedef nsClassHashtable<nsUint64HashKey, CameraControls> WindowTable;

class nsDOMCameraManager MOZ_FINAL
  : public nsIDOMCameraManager
  , public nsIObserver
  , public nsSupportsWeakReference
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMCameraManager, nsIObserver)
  NS_DECL_NSIDOMCAMERAMANAGER
  NS_DECL_NSIOBSERVER

  static already_AddRefed<nsDOMCameraManager>
    CheckPermissionAndCreateInstance(nsPIDOMWindow* aWindow);
  static bool IsWindowStillActive(uint64_t aWindowId);

  void Register(mozilla::nsDOMCameraControl* aDOMCameraControl);
  void OnNavigation(uint64_t aWindowId);

  nsresult GetNumberOfCameras(int32_t& aDeviceCount);
  nsresult GetCameraName(uint32_t aDeviceNum, nsCString& aDeviceName);

protected:
  void XpComShutdown();
  void Shutdown(uint64_t aWindowId);
  ~nsDOMCameraManager();

private:
  nsDOMCameraManager() MOZ_DELETE;
  nsDOMCameraManager(uint64_t aWindowId);
  nsDOMCameraManager(const nsDOMCameraManager&) MOZ_DELETE;
  nsDOMCameraManager& operator=(const nsDOMCameraManager&) MOZ_DELETE;

protected:
  uint64_t mWindowId;
  nsCOMPtr<nsIThread> mCameraThread;
  



  static WindowTable sActiveWindows;
  static bool sActiveWindowsInitialized;
};

class GetCameraTask : public nsRunnable
{
public:
  GetCameraTask(uint32_t aCameraId, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, nsIThread* aCameraThread)
    : mCameraId(aCameraId)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
    , mCameraThread(aCameraThread)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE;

protected:
  uint32_t mCameraId;
  nsCOMPtr<nsICameraGetCameraCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
  nsCOMPtr<nsIThread> mCameraThread;
};

#endif 
