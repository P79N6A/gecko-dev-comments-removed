





#ifndef mozilla_dom_bluetooth_bluetoothledeviceevent_h
#define mozilla_dom_bluetooth_bluetoothledeviceevent_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BluetoothLeDeviceEventBinding.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothLeDeviceEvent : public Event
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(BluetoothLeDeviceEvent,
                                                         Event)
protected:
  virtual ~BluetoothLeDeviceEvent();
  explicit BluetoothLeDeviceEvent(mozilla::dom::EventTarget* aOwner);

  nsRefPtr<BluetoothDevice> mDevice;
  int16_t mRssi;
  JS::Heap<JSObject*> mScanRecord;

public:
  virtual JSObject* WrapObjectInternal(
    JSContext* aCx,
    JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<BluetoothLeDeviceEvent>
    Constructor(EventTarget* aOwner,
                const nsAString& aType,
                BluetoothDevice* const aDevice,
                const int16_t aRssi,
                const nsTArray<uint8_t>& aScanRecord);

  static already_AddRefed<BluetoothLeDeviceEvent>
    Constructor(const GlobalObject& aGlobal,
                const nsAString& aType,
                const BluetoothLeDeviceEventInit& aEventInitDict,
                ErrorResult& aRv);

  BluetoothDevice* GetDevice() const;

  int16_t Rssi() const;

  void GetScanRecord(JSContext* cx,
                     JS::MutableHandle<JSObject*> aScanRecord,
                     ErrorResult& aRv);

  private:
    nsTArray<uint8_t> mRawScanRecord;
};

END_BLUETOOTH_NAMESPACE

#endif 
