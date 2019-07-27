





#include "mozilla/dom/DeviceStorageAreaListener.h"
#include "mozilla/dom/DeviceStorageAreaListenerBinding.h"
#include "mozilla/Attributes.h"
#include "mozilla/Services.h"
#include "DeviceStorage.h"
#include "nsIObserverService.h"
#ifdef MOZ_WIDGET_GONK
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#endif

namespace mozilla {
namespace dom {

class VolumeStateObserver final : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  explicit VolumeStateObserver(DeviceStorageAreaListener* aListener)
    : mDeviceStorageAreaListener(aListener) {}
  void ForgetListener() { mDeviceStorageAreaListener = nullptr; }

private:
  ~VolumeStateObserver() {};

  
  
  DeviceStorageAreaListener* MOZ_NON_OWNING_REF mDeviceStorageAreaListener;
};

NS_IMPL_ISUPPORTS(VolumeStateObserver, nsIObserver)

NS_IMETHODIMP
VolumeStateObserver::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const char16_t *aData)
{
  if (!mDeviceStorageAreaListener) {
    return NS_OK;
  }

#ifdef MOZ_WIDGET_GONK
  if (!strcmp(aTopic, NS_VOLUME_STATE_CHANGED)) {
    nsCOMPtr<nsIVolume> vol = do_QueryInterface(aSubject);
    MOZ_ASSERT(vol);

    int32_t state;
    nsresult rv = vol->GetState(&state);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString volName;
    vol->GetName(volName);

    switch (state) {
      case nsIVolume::STATE_MOUNTED:
        mDeviceStorageAreaListener->DispatchStorageAreaChangedEvent(
          volName,
          DeviceStorageAreaChangedEventOperation::Added);
        break;
      default:
        mDeviceStorageAreaListener->DispatchStorageAreaChangedEvent(
          volName,
          DeviceStorageAreaChangedEventOperation::Removed);
        break;
    }
  }
#endif
  return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(DeviceStorageAreaListener, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(DeviceStorageAreaListener, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN(DeviceStorageAreaListener)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

DeviceStorageAreaListener::DeviceStorageAreaListener(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
  MOZ_ASSERT(aWindow);

  MOZ_ASSERT(NS_IsMainThread());

  mVolumeStateObserver = new VolumeStateObserver(this);
#ifdef MOZ_WIDGET_GONK
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(mVolumeStateObserver, NS_VOLUME_STATE_CHANGED, false);
  }
#endif
}

DeviceStorageAreaListener::~DeviceStorageAreaListener()
{
#ifdef MOZ_WIDGET_GONK
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(mVolumeStateObserver, NS_VOLUME_STATE_CHANGED);
  }
#endif
  mVolumeStateObserver->ForgetListener();
}

JSObject*
DeviceStorageAreaListener::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return DeviceStorageAreaListenerBinding::Wrap(aCx, this, aGivenProto);
}

void
DeviceStorageAreaListener::DispatchStorageAreaChangedEvent(
  const nsString& aStorageName,
  DeviceStorageAreaChangedEventOperation aOperation)
{
  StateMapType::const_iterator iter = mStorageAreaStateMap.find(aStorageName);
  if (iter == mStorageAreaStateMap.end() &&
      aOperation != DeviceStorageAreaChangedEventOperation::Added) {
    
    return;
  }
  if (iter != mStorageAreaStateMap.end() &&
      iter->second == aOperation) {
    
    return;
  }

  DeviceStorageAreaChangedEventInit init;
  init.mOperation = aOperation;
  init.mStorageName = aStorageName;

  nsRefPtr<DeviceStorageAreaChangedEvent> event =
    DeviceStorageAreaChangedEvent::Constructor(this,
                                               NS_LITERAL_STRING("storageareachanged"),
                                               init);
  event->SetTrusted(true);

  mStorageAreaStateMap[aStorageName] = aOperation;

  nsDOMDeviceStorage::InvalidateVolumeCaches();

  bool ignore;
  DOMEventTargetHelper::DispatchEvent(event, &ignore);
}

} 
} 
