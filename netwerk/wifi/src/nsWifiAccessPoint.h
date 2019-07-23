






































#include "nsWifiMonitor.h"
#include "nsIWifiAccessPoint.h"

#include "nsString.h"
#include "nsCOMArray.h"

#ifndef __nsWifiAccessPoint__
#define __nsWifiAccessPoint__

class nsWifiAccessPoint : public nsIWifiAccessPoint
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWIFIACCESSPOINT

  nsWifiAccessPoint();
  ~nsWifiAccessPoint();

  char mMac[18];
  int  mSignal;
  char mSsid[33];
  int  mSsidLen;

  void setSignal(int signal)
  {
    mSignal = signal;
  };

  void setMac(const unsigned char mac_as_int[6])
  {
    
    

    static const char *kMacFormatString = ("%02x-%02x-%02x-%02x-%02x-%02x");

    sprintf(mMac, kMacFormatString,
            mac_as_int[0], mac_as_int[1], mac_as_int[2],
            mac_as_int[3], mac_as_int[4], mac_as_int[5]);

    mMac[17] = 0;
  };

  void setSSID(const char* aSSID, unsigned long len) {
    if (len < sizeof(mSsid)) {
        strncpy(mSsid, aSSID, len);
        mSsid[len] = 0;
        mSsidLen = len;
    }
    else
    {
      mSsid[0] = 0;
      mSsidLen = 0;
    }
  };
};





PRBool AccessPointsEqual(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b);
void ReplaceArray(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b);


#endif
