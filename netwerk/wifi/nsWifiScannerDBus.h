



#ifndef NSWIFIAPSCANNERDBUS_H_
#define NSWIFIAPSCANNERDBUS_H_

#include "nsCOMArray.h"

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

class nsWifiAccessPoint;

namespace mozilla {

class nsWifiScannerDBus MOZ_FINAL
{
public:
  explicit nsWifiScannerDBus(nsCOMArray<nsWifiAccessPoint>* aAccessPoints);
  ~nsWifiScannerDBus();

  nsresult Scan();

private:
  nsresult SendMessage(const char* aInterface,
                       const char* aPath,
                       const char* aFuncCall);
  nsresult IdentifyDevices(DBusMessage* aMsg);
  nsresult IdentifyDeviceType(DBusMessage* aMsg, const char* aDevicePath);
  nsresult IdentifyAccessPoints(DBusMessage* aMsg);
  nsresult IdentifyAPProperties(DBusMessage* aMsg);
  nsresult StoreSsid(DBusMessageIter* aVariant, nsWifiAccessPoint* aAp);
  nsresult SetMac(DBusMessageIter* aVariant, nsWifiAccessPoint* aAp);
  nsresult GetDBusIterator(DBusMessage* aMsg, DBusMessageIter* aIterArray);

  DBusConnection* mConnection;
  nsCOMArray<nsWifiAccessPoint>* mAccessPoints;
};

} 

#endif 
