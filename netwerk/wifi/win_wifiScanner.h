



#pragma once


#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "win_wlanLibrary.h"

class nsWifiAccessPoint;

class WinWifiScanner {
 public:
  WinWifiScanner();
  ~WinWifiScanner();

  








  nsresult GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints);

 private:
  nsAutoPtr<WinWLANLibrary> mWlanLibrary;
};
