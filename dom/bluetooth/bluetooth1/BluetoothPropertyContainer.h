





#ifndef mozilla_dom_bluetooth_bluetoothpropertyobject_h__
#define mozilla_dom_bluetooth_bluetoothpropertyobject_h__

#include "BluetoothCommon.h"
#include "BluetoothReplyRunnable.h"

class nsIDOMDOMRequest;
class nsPIDOMWindow;

namespace mozilla {
class ErrorResult;
namespace dom {
class DOMRequest;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;

class BluetoothPropertyContainer
{
public:
  already_AddRefed<mozilla::dom::DOMRequest>
    FirePropertyAlreadySet(nsPIDOMWindow* aOwner, ErrorResult& aRv);
  already_AddRefed<mozilla::dom::DOMRequest>
    SetProperty(nsPIDOMWindow* aOwner, const BluetoothNamedValue& aProperty,
                ErrorResult& aRv);
  virtual void SetPropertyByValue(const BluetoothNamedValue& aValue) = 0;
  nsString GetPath()
  {
    return mPath;
  }

  
  
  virtual nsrefcnt AddRef() = 0;
  virtual nsrefcnt Release() = 0;

protected:
  BluetoothPropertyContainer(BluetoothObjectType aType) :
    mObjectType(aType)
  {}

  ~BluetoothPropertyContainer()
  {}

  nsString mPath;
  BluetoothObjectType mObjectType;
};

END_BLUETOOTH_NAMESPACE

#endif
