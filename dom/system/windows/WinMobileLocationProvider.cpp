





































#include "nsGeoPosition.h"
#include "WinMobileLocationProvider.h"
#include "nsGeolocation.h"
#include "nsIClassInfo.h"
#include "nsDOMClassInfoID.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS2(WinMobileLocationProvider, nsIGeolocationProvider, nsITimerCallback)

WinMobileLocationProvider::WinMobileLocationProvider() :
  mGPSDevice(nsnull), 
  mOpenDevice(nsnull),
  mCloseDevice(nsnull),
  mGetPosition(nsnull),
  mGetDeviceState(nsnull),
  mHasSeenLocation(PR_FALSE),
  mHasGPS(PR_TRUE) 
{
}

WinMobileLocationProvider::~WinMobileLocationProvider()
{
}

NS_IMETHODIMP
WinMobileLocationProvider::Notify(nsITimer* aTimer)
{
  if (!mGPSDevice || !mGetPosition)
    return NS_ERROR_FAILURE;

  GPS_POSITION pos;
  memset(&pos, 0, sizeof(GPS_POSITION));
  pos.dwVersion = GPS_VERSION_1;
  pos.dwSize = sizeof(GPS_POSITION);
  
  
  
  mGetPosition(mGPSDevice, &pos, 100, 0);
  
  if ((!(pos.dwValidFields & GPS_VALID_LATITUDE) &&
       !(pos.dwValidFields & GPS_VALID_LONGITUDE) ) ||
      pos.dwFlags == GPS_DATA_FLAGS_HARDWARE_OFF ||
      pos.FixQuality == GPS_FIX_QUALITY_UNKNOWN)
    return NS_OK;

  nsRefPtr<nsGeoPosition> somewhere =
    new nsGeoPosition(pos.dblLatitude,
                      pos.dblLongitude,
                      (double)pos.flAltitudeWRTSeaLevel,
                      (double)pos.flHorizontalDilutionOfPrecision,
                      (double)pos.flVerticalDilutionOfPrecision,
                      (double)pos.flHeading,
                      (double)pos.flSpeed,
                      PR_Now());
  Update(somewhere);
  return NS_OK;
}

NS_IMETHODIMP WinMobileLocationProvider::Startup()
{
  if (mHasGPS && !mGPSInst) {
    mGPSInst = LoadLibraryW(L"gpsapi.dll");
    
    if(!mGPSInst) {
      mHasGPS = PR_FALSE;
      NS_ASSERTION(mGPSInst, "failed to load library gpsapi.dll");
      return NS_ERROR_FAILURE;
    }

    mOpenDevice  = (OpenDeviceProc) GetProcAddress(mGPSInst,"GPSOpenDevice");
    mCloseDevice = (CloseDeviceProc) GetProcAddress(mGPSInst,"GPSCloseDevice");
    mGetPosition = (GetPositionProc) GetProcAddress(mGPSInst,"GPSGetPosition");
    mGetDeviceState = (GetDeviceStateProc) GetProcAddress(mGPSInst,"GPSGetDeviceState");

    if (!mOpenDevice || !mCloseDevice || !mGetPosition || !mGetDeviceState)
      return NS_ERROR_FAILURE;

    mGPSDevice = mOpenDevice(NULL, NULL, NULL, 0);

    if (!mGPSDevice) {
      NS_ASSERTION(mGPSDevice, "GPS Device not found");
      return NS_ERROR_FAILURE;
    }
    nsresult rv;
    mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);

    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    PRInt32 update = 3000; 
    if (prefs)
      prefs->GetIntPref("geo.default.update", &update);

    mUpdateTimer->InitWithCallback(this, update, nsITimer::TYPE_REPEATING_SLACK);
  }
  return NS_OK;
}

NS_IMETHODIMP WinMobileLocationProvider::Watch(nsIGeolocationUpdate *callback)
{
  if (mCallback)
    return NS_OK;
  
  mCallback = callback;
  return NS_OK;
}

NS_IMETHODIMP WinMobileLocationProvider::Shutdown()
{
  if (mUpdateTimer)
    mUpdateTimer->Cancel();
  
  if (mGPSDevice)
    mCloseDevice(mGPSDevice);

  mGPSDevice = nsnull;
  
  mHasSeenLocation = PR_FALSE;
  
  mCallback = nsnull;
  
  FreeLibrary(mGPSInst);
  mGPSInst = nsnull;

  mOpenDevice  = nsnull;
  mCloseDevice = nsnull;
  mGetPosition = nsnull;
  mGetDeviceState = nsnull;
 
  return NS_OK;
}

void WinMobileLocationProvider::Update(nsIDOMGeoPosition* aPosition)
{
  mHasSeenLocation = PR_TRUE;
  if (mCallback)
    mCallback->Update(aPosition);
}
