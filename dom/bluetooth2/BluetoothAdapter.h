





#ifndef mozilla_dom_bluetooth_bluetoothadapter_h__
#define mozilla_dom_bluetooth_bluetoothadapter_h__

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BluetoothAdapter2Binding.h"
#include "mozilla/dom/BluetoothDeviceEvent.h"
#include "mozilla/dom/Promise.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
class DOMRequest;
class File;
struct MediaMetaData;
struct MediaPlayStatus;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;
class BluetoothDiscoveryHandle;
class BluetoothNamedValue;
class BluetoothPairingListener;
class BluetoothSignal;
class BluetoothValue;

class BluetoothAdapter : public DOMEventTargetHelper
                       , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothAdapter,
                                           DOMEventTargetHelper)

  


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

  


  IMPL_EVENT_HANDLER(attributechanged);
  IMPL_EVENT_HANDLER(devicepaired);
  IMPL_EVENT_HANDLER(deviceunpaired);
  IMPL_EVENT_HANDLER(pairingaborted);
  IMPL_EVENT_HANDLER(a2dpstatuschanged);
  IMPL_EVENT_HANDLER(hfpstatuschanged);
  IMPL_EVENT_HANDLER(requestmediaplaystatus);
  IMPL_EVENT_HANDLER(scostatuschanged);

  


  already_AddRefed<Promise> Enable(ErrorResult& aRv);
  already_AddRefed<Promise> Disable(ErrorResult& aRv);

  already_AddRefed<Promise> SetName(const nsAString& aName, ErrorResult& aRv);
  already_AddRefed<Promise> SetDiscoverable(bool aDiscoverable,
                                            ErrorResult& aRv);
  already_AddRefed<Promise> StartDiscovery(ErrorResult& aRv);
  already_AddRefed<Promise> StopDiscovery(ErrorResult& aRv);
  already_AddRefed<Promise> Pair(const nsAString& aDeviceAddress,
                                 ErrorResult& aRv);
  already_AddRefed<Promise> Unpair(const nsAString& aDeviceAddress,
                                   ErrorResult& aRv);

  




  void GetPairedDevices(nsTArray<nsRefPtr<BluetoothDevice> >& aDevices);

  
  already_AddRefed<DOMRequest>
    Connect(BluetoothDevice& aDevice,
            const Optional<short unsigned int>& aServiceUuid,
            ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    Disconnect(BluetoothDevice& aDevice,
               const Optional<short unsigned int>& aServiceUuid,
               ErrorResult& aRv);
  already_AddRefed<DOMRequest> GetConnectedDevices(uint16_t aServiceUuid,
                                                   ErrorResult& aRv);

  
  already_AddRefed<DOMRequest> SendFile(const nsAString& aDeviceAddress,
                                        File& aBlob,
                                        ErrorResult& aRv);
  already_AddRefed<DOMRequest> StopSendingFile(const nsAString& aDeviceAddress,
                                               ErrorResult& aRv);
  already_AddRefed<DOMRequest>
    ConfirmReceivingFile(const nsAString& aDeviceAddress,
                         bool aConfirmation,
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
    SendMediaPlayStatus(const MediaPlayStatus& aMediaPlayStatus,
                        ErrorResult& aRv);

  


  static already_AddRefed<BluetoothAdapter>
    Create(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);

  void Notify(const BluetoothSignal& aParam); 
  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  virtual void DisconnectFromOwner() override;

  








  void SetDiscoveryHandleInUse(BluetoothDiscoveryHandle* aDiscoveryHandle);

private:
  BluetoothAdapter(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);
  ~BluetoothAdapter();

  




  void SetPropertyByValue(const BluetoothNamedValue& aValue);

  




  void SetAdapterState(BluetoothAdapterState aState);

  







  already_AddRefed<Promise> PairUnpair(bool aPair,
                            const nsAString& aDeviceAddress,
                            ErrorResult& aRv);

  




  void GetPairedDeviceProperties(const nsTArray<nsString>& aDeviceAddresses);

  




  void HandlePropertyChanged(const BluetoothValue& aValue);

  




  void HandleDeviceFound(const BluetoothValue& aValue);

  







  void HandleDevicePaired(const BluetoothValue& aValue);

  







  void HandleDeviceUnpaired(const BluetoothValue& aValue);

  


  void DispatchAttributeEvent(const nsTArray<nsString>& aTypes);

  






  void DispatchDeviceEvent(const nsAString& aType,
                           const BluetoothDeviceEventInit& aInit);

  




  void DispatchEmptyEvent(const nsAString& aType);

  




  BluetoothAdapterAttribute
    ConvertStringToAdapterAttribute(const nsAString& aString);

  





  bool IsAdapterAttributeChanged(BluetoothAdapterAttribute aType,
                                 const BluetoothValue& aValue);

  




  bool IsBluetoothCertifiedApp();

  


  


  BluetoothAdapterState mState;

  


  nsString mAddress;

  


  nsString mName;

  


  bool mDiscoverable;

  


  bool mDiscovering;

  


  nsRefPtr<BluetoothPairingListener> mPairingReqs;

  






  nsRefPtr<BluetoothDiscoveryHandle> mDiscoveryHandleInUse;

  















  nsTArray<nsRefPtr<BluetoothDevice> > mDevices;
};

END_BLUETOOTH_NAMESPACE

#endif
