





#ifndef DOM_CAMERA_DOMCAMERAMANAGER_H
#define DOM_CAMERA_DOMCAMERAMANAGER_H

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Promise.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIObserver.h"
#include "nsHashKeys.h"
#include "nsWrapperCache.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class nsPIDOMWindow;

namespace mozilla {
  class ErrorResult;
  class nsDOMCameraControl;
  namespace dom {
    struct CameraConfiguration;
  } 
} 

typedef nsTArray<nsWeakPtr> CameraControls;
typedef nsClassHashtable<nsUint64HashKey, CameraControls> WindowTable;

class nsDOMCameraManager final
  : public nsIObserver
  , public nsSupportsWeakReference
  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMCameraManager,
                                                         nsIObserver)
  NS_DECL_NSIOBSERVER

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  static bool CheckPermission(nsPIDOMWindow* aWindow);
  static already_AddRefed<nsDOMCameraManager>
    CreateInstance(nsPIDOMWindow* aWindow);
  static bool IsWindowStillActive(uint64_t aWindowId);

  void Register(mozilla::nsDOMCameraControl* aDOMCameraControl);
  void OnNavigation(uint64_t aWindowId);

  void PermissionAllowed(uint32_t aCameraId,
                         const mozilla::dom::CameraConfiguration& aOptions,
                         mozilla::dom::Promise* aPromise);

  void PermissionCancelled(uint32_t aCameraId,
                           const mozilla::dom::CameraConfiguration& aOptions,
                           mozilla::dom::Promise* aPromise);

  
  already_AddRefed<mozilla::dom::Promise>
  GetCamera(const nsAString& aCamera,
            const mozilla::dom::CameraConfiguration& aOptions,
            mozilla::ErrorResult& aRv);
  void GetListOfCameras(nsTArray<nsString>& aList, mozilla::ErrorResult& aRv);

  nsPIDOMWindow* GetParentObject() const { return mWindow; }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

#ifdef MOZ_WIDGET_GONK
  static void PreinitCameraHardware();
#endif

protected:
  void XpComShutdown();
  void Shutdown(uint64_t aWindowId);
  ~nsDOMCameraManager();

private:
  nsDOMCameraManager() = delete;
  explicit nsDOMCameraManager(nsPIDOMWindow* aWindow);
  nsDOMCameraManager(const nsDOMCameraManager&) = delete;
  nsDOMCameraManager& operator=(const nsDOMCameraManager&) = delete;

protected:
  uint64_t mWindowId;
  uint32_t mPermission;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  



  static ::WindowTable* sActiveWindows;
};

#endif 
