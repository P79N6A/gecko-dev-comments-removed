



#include "nsWifiMonitor.h"
#include "nsIWifiAccessPoint.h"

#include "nsString.h"
#include "nsCOMArray.h"
#include "mozilla/ArrayUtils.h" 
#include "mozilla/Attributes.h"

#ifndef __nsWifiAccessPoint__
#define __nsWifiAccessPoint__

class nsWifiAccessPoint final : public nsIWifiAccessPoint
{
  ~nsWifiAccessPoint();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWIFIACCESSPOINT

  nsWifiAccessPoint();

  char mMac[18];
  int  mSignal;
  char mSsid[33];
  int  mSsidLen;

  void setSignal(int signal)
  {
    mSignal = signal;
  }

  void setMacRaw(const char* aString)
  {
    memcpy(mMac, aString, mozilla::ArrayLength(mMac));
  }

  void setMac(const unsigned char mac_as_int[6])
  {
    
    

    const unsigned char holder[6] = {0};
    if (!mac_as_int) {
      mac_as_int = holder;
    }

    static const char *kMacFormatString = ("%02x-%02x-%02x-%02x-%02x-%02x");

    sprintf(mMac, kMacFormatString,
            mac_as_int[0], mac_as_int[1], mac_as_int[2],
            mac_as_int[3], mac_as_int[4], mac_as_int[5]);

    mMac[17] = 0;
  }

  void setSSIDRaw(const char* aSSID, unsigned long len) {
    memcpy(mSsid, aSSID, mozilla::ArrayLength(mSsid));
    mSsidLen = PR_MIN(len, mozilla::ArrayLength(mSsid));
  }

  void setSSID(const char* aSSID, unsigned long len) {
    if (aSSID && (len < sizeof(mSsid))) {
        strncpy(mSsid, aSSID, len);
        mSsid[len] = 0;
        mSsidLen = len;
    }
    else
    {
      mSsid[0] = 0;
      mSsidLen = 0;
    }
  }
};





bool AccessPointsEqual(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b);
void ReplaceArray(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b);


#endif
