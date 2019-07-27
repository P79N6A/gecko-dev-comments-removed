





#ifndef mozilla_dom_bluetooth_bluetoothmanager_h__
#define mozilla_dom_bluetooth_bluetoothmanager_h__

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BluetoothAdapterEvent.h"
#include "mozilla/dom/BluetoothAttributeEvent.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Observer.h"
#include "nsISupportsImpl.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothAdapter;
class BluetoothValue;

class BluetoothManager final : public DOMEventTargetHelper
                             , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothManager,
                                           DOMEventTargetHelper)

  


  IMPL_EVENT_HANDLER(attributechanged);
  IMPL_EVENT_HANDLER(adapteradded);
  IMPL_EVENT_HANDLER(adapterremoved);

  


  



  BluetoothAdapter* GetDefaultAdapter();

  





  void GetAdapters(nsTArray<nsRefPtr<BluetoothAdapter> >& aAdapters);

  


  
  static already_AddRefed<BluetoothManager> Create(nsPIDOMWindow* aWindow);
  static bool CheckPermission(nsPIDOMWindow* aWindow);

  void Notify(const BluetoothSignal& aData); 
  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  virtual void DisconnectFromOwner() override;

  





  void AppendAdapter(const BluetoothValue& aValue);

private:
  BluetoothManager(nsPIDOMWindow* aWindow);
  ~BluetoothManager();

  


  bool DefaultAdapterExists()
  {
    return (mDefaultAdapterIndex >= 0);
  }

  




  void HandleAdapterAdded(const BluetoothValue& aValue);

  




  void HandleAdapterRemoved(const BluetoothValue& aValue);

  



  void ReselectDefaultAdapter();

  






  void DispatchAdapterEvent(const nsAString& aType,
                            const BluetoothAdapterEventInit& aInit);

  


  void DispatchAttributeEvent();

  


  


  int mDefaultAdapterIndex;

  


  nsTArray<nsRefPtr<BluetoothAdapter> > mAdapters;
};

END_BLUETOOTH_NAMESPACE

#endif
