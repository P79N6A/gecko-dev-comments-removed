





#ifndef mozilla_dom_bluetooth_bluetoothmanager_h__
#define mozilla_dom_bluetooth_bluetoothmanager_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Observer.h"
#include "BluetoothCommon.h"
#include "BluetoothPropertyContainer.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace dom {
class DOMRequest;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;

class BluetoothManager : public DOMEventTargetHelper
                       , public BluetoothSignalObserver
                       , public BluetoothPropertyContainer
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  static already_AddRefed<BluetoothManager>
    Create(nsPIDOMWindow* aWindow);
  static bool CheckPermission(nsPIDOMWindow* aWindow);
  void Notify(const BluetoothSignal& aData);
  virtual void SetPropertyByValue(const BluetoothNamedValue& aValue) override;

  bool GetEnabled(ErrorResult& aRv);

  already_AddRefed<DOMRequest> GetDefaultAdapter(ErrorResult& aRv);

  IMPL_EVENT_HANDLER(enabled);
  IMPL_EVENT_HANDLER(disabled);
  IMPL_EVENT_HANDLER(adapteradded);

  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject*
    WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void DisconnectFromOwner() override;

private:
  BluetoothManager(nsPIDOMWindow* aWindow);
  ~BluetoothManager();
};

END_BLUETOOTH_NAMESPACE

#endif
