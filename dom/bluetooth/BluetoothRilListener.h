





#ifndef mozilla_dom_bluetooth_bluetoothrillistener_h__
#define mozilla_dom_bluetooth_bluetoothrillistener_h__

#include "BluetoothCommon.h"

#include "nsAutoPtr.h"

#include "nsIIccProvider.h"
#include "nsIMobileConnectionProvider.h"
#include "nsITelephonyService.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothRilListener;

class IccListener : public nsIIccListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCLISTENER

  IccListener() { }
  virtual ~IccListener() { }

  bool Listen(bool aStart);
  void SetOwner(BluetoothRilListener *aOwner);

private:
  BluetoothRilListener* mOwner;
};

class MobileConnectionListener : public nsIMobileConnectionListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONLISTENER

  MobileConnectionListener(uint32_t aClientId)
  : mClientId(aClientId) { }
  virtual ~MobileConnectionListener() { }

  bool Listen(bool aStart);

private:
  uint32_t mClientId;
};

class TelephonyListener : public nsITelephonyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYLISTENER

  TelephonyListener() { }
  virtual ~TelephonyListener() { }

  bool Listen(bool aStart);
};

class BluetoothRilListener
{
public:
  BluetoothRilListener();
  ~BluetoothRilListener();

  




  bool Listen(bool aStart);

  





  void ServiceChanged(uint32_t aClientId, bool aRegistered);

  


  void EnumerateCalls();

  






  uint32_t mClientId;

private:
  




  bool ListenMobileConnAndIccInfo(bool aStart);

  





  void SelectClient();

  




  nsTArray<nsRefPtr<MobileConnectionListener> > mMobileConnListeners;

  nsRefPtr<IccListener> mIccListener;
  nsRefPtr<TelephonyListener> mTelephonyListener;
};

END_BLUETOOTH_NAMESPACE

#endif
