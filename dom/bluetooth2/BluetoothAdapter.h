





#ifndef mozilla_dom_bluetooth_bluetoothadapter_h__
#define mozilla_dom_bluetooth_bluetoothadapter_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BluetoothAdapter2Binding.h"
#include "mozilla/dom/Promise.h"
#include "BluetoothCommon.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
class DOMRequest;
struct MediaMetaData;
struct MediaPlayStatus;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;
class BluetoothDiscoveryHandle;
class BluetoothSignal;
class BluetoothNamedValue;
class BluetoothPairingListener;
class BluetoothValue;

class BluetoothAdapter : public DOMEventTargetHelper
                       , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothAdapter,
                                           DOMEventTargetHelper)

  static already_AddRefed<BluetoothAdapter>
  Create(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);

  void Notify(const BluetoothSignal& aParam);

  void SetPropertyByValue(const BluetoothNamedValue& aValue);

  virtual void DisconnectFromOwner() MOZ_OVERRIDE;

  BluetoothAdapterState State() const
  {
    return mState;
  }

  void GetAddress(nsString& aAddress) const
  {
    aAddress = mAddress;
  }

  void
  GetName(nsString& aName) const
  {
    aName = mName;
  }

  bool
  Discovering() const
  {
    return mDiscovering;
  }

  bool
  Discoverable() const
  {
    return mDiscoverable;
  }

  BluetoothPairingListener* PairingReqs() const
  {
    return mPairingReqs;
  }

  








  void SetDiscoveryHandleInUse(BluetoothDiscoveryHandle* aDiscoveryHandle);

  already_AddRefed<Promise> SetName(const nsAString& aName, ErrorResult& aRv);
  already_AddRefed<Promise>
    SetDiscoverable(bool aDiscoverable, ErrorResult& aRv);
  already_AddRefed<Promise> StartDiscovery(ErrorResult& aRv);
  already_AddRefed<Promise> StopDiscovery(ErrorResult& aRv);

  already_AddRefed<Promise>
    Pair(const nsAString& aDeviceAddress, ErrorResult& aRv);
  already_AddRefed<Promise>
    Unpair(const nsAString& aDeviceAddress, ErrorResult& aRv);

  




  void GetPairedDevices(nsTArray<nsRefPtr<BluetoothDevice> >& aDevices);

  already_AddRefed<Promise> EnableDisable(bool aEnable, ErrorResult& aRv);
  already_AddRefed<Promise> Enable(ErrorResult& aRv);
  already_AddRefed<Promise> Disable(ErrorResult& aRv);

  already_AddRefed<DOMRequest>
    Connect(BluetoothDevice& aDevice,
            const Optional<short unsigned int>& aServiceUuid, ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    Disconnect(BluetoothDevice& aDevice,
               const Optional<short unsigned int>& aServiceUuid,
               ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    GetConnectedDevices(uint16_t aServiceUuid, ErrorResult& aRv);

  already_AddRefed<DOMRequest>
    SendFile(const nsAString& aDeviceAddress, nsIDOMBlob* aBlob,
             ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    StopSendingFile(const nsAString& aDeviceAddress, ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    ConfirmReceivingFile(const nsAString& aDeviceAddress, bool aConfirmation,
                         ErrorResult& aRv);

  already_AddRefed<DOMRequest> ConnectSco(ErrorResult& aRv);
  already_AddRefed<DOMRequest> DisconnectSco(ErrorResult& aRv);
  already_AddRefed<DOMRequest> IsScoConnected(ErrorResult& aRv);

  already_AddRefed<DOMRequest> AnswerWaitingCall(ErrorResult& aRv);
  already_AddRefed<DOMRequest> IgnoreWaitingCall(ErrorResult& aRv);
  already_AddRefed<DOMRequest> ToggleCalls(ErrorResult& aRv);

  already_AddRefed<DOMRequest>
    SendMediaMetaData(const MediaMetaData& aMediaMetaData, ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    SendMediaPlayStatus(const MediaPlayStatus& aMediaPlayStatus, ErrorResult& aRv);

  IMPL_EVENT_HANDLER(a2dpstatuschanged);
  IMPL_EVENT_HANDLER(hfpstatuschanged);
  IMPL_EVENT_HANDLER(pairedstatuschanged);
  IMPL_EVENT_HANDLER(requestmediaplaystatus);
  IMPL_EVENT_HANDLER(scostatuschanged);
  IMPL_EVENT_HANDLER(attributechanged);

  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject*
    WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  BluetoothAdapter(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);
  ~BluetoothAdapter();

  already_AddRefed<Promise>
    PairUnpair(bool aPair, const nsAString& aDeviceAddress, ErrorResult& aRv);

  bool IsAdapterAttributeChanged(BluetoothAdapterAttribute aType,
                                 const BluetoothValue& aValue);
  void HandleAdapterStateChanged();
  void HandlePropertyChanged(const BluetoothValue& aValue);
  void DispatchAttributeEvent(const nsTArray<nsString>& aTypes);
  BluetoothAdapterAttribute
    ConvertStringToAdapterAttribute(const nsAString& aString);

  void GetPairedDeviceProperties(const nsTArray<nsString>& aDeviceAddresses);

  void HandleDeviceFound(const BluetoothValue& aValue);
  void HandlePairingRequest(const BluetoothValue& aValue);

  














  nsTArray<nsRefPtr<BluetoothDevice> > mDevices;
  nsRefPtr<BluetoothDiscoveryHandle> mDiscoveryHandleInUse;
  nsRefPtr<BluetoothPairingListener> mPairingReqs;
  BluetoothAdapterState mState;
  nsString mAddress;
  nsString mName;
  bool mDiscoverable;
  bool mDiscovering;
};

END_BLUETOOTH_NAMESPACE

#endif
