





































#include <windows.h>

#undef GetMessage //prevent collision with nsDOMGeoPositionError::GetMessage(nsAString & aMessage)

#include <gpsapi.h>
#include "nsIGeolocationProvider.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"

typedef HANDLE (*OpenDeviceProc)(HANDLE hNewLocationData, HANDLE hDeviceStateChange, const WCHAR *szDeviceName, DWORD dwFlags);
typedef DWORD  (*CloseDeviceProc)(HANDLE hGPSDevice);
typedef DWORD  (*GetPositionProc)(HANDLE hGPSDevice, GPS_POSITION *pGPSPosition, DWORD dwMaximumAge, DWORD dwFlags);
typedef DWORD  (*GetDeviceStateProc)(GPS_DEVICE *pGPSDevice);

class WinMobileLocationProvider : public nsIGeolocationProvider,
                                  public nsITimerCallback

{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER
  NS_DECL_NSITIMERCALLBACK
  
  WinMobileLocationProvider();

  void Update(nsIDOMGeoPosition* aPosition);
  
 private:
  ~WinMobileLocationProvider();
  
  nsCOMPtr<nsIGeolocationUpdate> mCallback;
  PRPackedBool mHasSeenLocation;
  PRPackedBool mHasGPS;
  
  nsCOMPtr<nsITimer> mUpdateTimer;
  
  HINSTANCE mGPSInst;
  HANDLE mGPSDevice;
  OpenDeviceProc mOpenDevice;
  CloseDeviceProc mCloseDevice;
  GetPositionProc mGetPosition;
  GetDeviceStateProc mGetDeviceState;
};
