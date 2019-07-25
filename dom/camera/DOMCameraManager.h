





#ifndef DOM_CAMERA_DOMCAMERAMANAGER_H
#define DOM_CAMERA_DOMCAMERAMANAGER_H

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "nsIDOMCameraManager.h"
#include "mozilla/Attributes.h"

class nsDOMCameraManager MOZ_FINAL : public nsIDOMCameraManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMCAMERAMANAGER

  static already_AddRefed<nsDOMCameraManager> Create(uint64_t aWindowId);

  void OnNavigation(uint64_t aWindowId);

private:
  nsDOMCameraManager();
  nsDOMCameraManager(uint64_t aWindowId);
  nsDOMCameraManager(const nsDOMCameraManager&) MOZ_DELETE;
  nsDOMCameraManager& operator=(const nsDOMCameraManager&) MOZ_DELETE;
  ~nsDOMCameraManager();

protected:
  uint64_t mWindowId;
  nsCOMPtr<nsIThread> mCameraThread;
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

  NS_IMETHOD Run();

protected:
  uint32_t mCameraId;
  nsCOMPtr<nsICameraGetCameraCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
  nsCOMPtr<nsIThread> mCameraThread;
};

class GetCameraResult : public nsRunnable
{
public:
  GetCameraResult(nsICameraControl* aCameraControl, nsICameraGetCameraCallback* onSuccess)
    : mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mCameraControl);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsICameraControl> mCameraControl;
  nsCOMPtr<nsICameraGetCameraCallback> mOnSuccessCb;
};

#endif 
