



#pragma once


#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "win_wlanLibrary.h"

class nsWifiAccessPoint;


class WindowsWifiScannerInterface {
public:
  virtual nsresult GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints) = 0;
};


class WinWifiScanner : public WindowsWifiScannerInterface {
 public:
  WinWifiScanner();
  virtual ~WinWifiScanner();

  








  nsresult GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints);

 private:
  nsAutoPtr<WinWLANLibrary> mWlanLibrary;
};
