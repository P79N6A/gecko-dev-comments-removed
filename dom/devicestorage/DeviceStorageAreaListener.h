





#ifndef mozilla_dom_DeviceStorageAreaListener_h
#define mozilla_dom_DeviceStorageAreaListener_h

#include <map>
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/DeviceStorageAreaChangedEvent.h"

namespace mozilla {
namespace dom {

class VolumeStateObserver;

class DeviceStorageAreaListener final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  IMPL_EVENT_HANDLER(storageareachanged)

  explicit DeviceStorageAreaListener(nsPIDOMWindow* aWindow);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  friend class VolumeStateObserver;

  typedef std::map<nsString, DeviceStorageAreaChangedEventOperation> StateMapType;
  StateMapType mStorageAreaStateMap;

  nsRefPtr<VolumeStateObserver> mVolumeStateObserver;

  ~DeviceStorageAreaListener();

  void DispatchStorageAreaChangedEvent(
    const nsString& aStorageName,
    DeviceStorageAreaChangedEventOperation aOperation);
};

} 
} 

#endif 
