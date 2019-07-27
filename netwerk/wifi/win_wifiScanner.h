



#pragma once


#include "nsAutoPtr.h"
#include "nsCOMArray.h"

class nsWifiAccessPoint;
class WinWLANLibrary;

class WinWifiScanner {
 public:
  WinWifiScanner();
  ~WinWifiScanner();

  








  nsresult GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints);

 private:
  nsAutoPtr<WinWLANLibrary> mWlanLibrary;
};
