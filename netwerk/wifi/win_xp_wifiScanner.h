



#ifndef WINXPWIFISCANNER_H_
#define WINXPWIFISCANNER_H_

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "win_wifiScanner.h"

class nsWifiAccessPoint;
class WindowsNdisApi;



class WinXPWifiScanner : public WindowsWifiScannerInterface {
public:
  nsresult GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints);
  virtual ~WinXPWifiScanner() {}
private:
  nsAutoPtr<WindowsNdisApi> mImplementation;
};

#endif