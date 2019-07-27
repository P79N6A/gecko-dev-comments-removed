





#ifndef mozilla_dom_bluetooth_bluetoothpairinghandle_h
#define mozilla_dom_bluetooth_bluetoothpairinghandle_h

#include "BluetoothCommon.h"
#include "nsWrapperCache.h"

namespace mozilla {
class ErrorResult;
namespace dom {
class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothPairingHandle final : public nsISupports
                                   , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothPairingHandle)

  static already_AddRefed<BluetoothPairingHandle>
    Create(nsPIDOMWindow* aOwner,
           const nsAString& aDeviceAddress,
           const nsAString& aType,
           const nsAString& aPasskey);

  nsPIDOMWindow* GetParentObject() const
  {
    return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  void GetPasskey(nsString& aPasskey) const
  {
    aPasskey = mPasskey;
  }

  
  already_AddRefed<Promise>
    SetPinCode(const nsAString& aPinCode, ErrorResult& aRv);

  
  already_AddRefed<Promise> Accept(ErrorResult& aRv);

  
  already_AddRefed<Promise> Reject(ErrorResult& aRv);

private:
  BluetoothPairingHandle(nsPIDOMWindow* aOwner,
                         const nsAString& aDeviceAddress,
                         const nsAString& aType,
                         const nsAString& aPasskey);
  ~BluetoothPairingHandle();

  






  bool GetSspVariant(BluetoothSspVariant& aVariant);

  nsCOMPtr<nsPIDOMWindow> mOwner;
  nsString mDeviceAddress;
  nsString mType;
  nsString mPasskey;
};

END_BLUETOOTH_NAMESPACE

#endif 
