



#include "nsWifiAccessPoint.h"
#include "nsString.h"
#include "nsMemory.h"
#include "mozilla/Logging.h"

extern PRLogModuleInfo *gWifiMonitorLog;
#define LOG(args)     MOZ_LOG(gWifiMonitorLog, PR_LOG_DEBUG, args)


NS_IMPL_ISUPPORTS(nsWifiAccessPoint, nsIWifiAccessPoint)

nsWifiAccessPoint::nsWifiAccessPoint()
{
  
  mMac[0] = '\0';
  mSsid[0] = '\0';
  mSsidLen = 0;
  mSignal = -1000;
}

nsWifiAccessPoint::~nsWifiAccessPoint()
{
}

NS_IMETHODIMP nsWifiAccessPoint::GetMac(nsACString& aMac)
{
  aMac.Assign(mMac);
  return NS_OK;
}

NS_IMETHODIMP nsWifiAccessPoint::GetSsid(nsAString& aSsid)
{
  
  
  CopyASCIItoUTF16(mSsid, aSsid);
  return NS_OK;
}


NS_IMETHODIMP nsWifiAccessPoint::GetRawSSID(nsACString& aRawSsid)
{
  aRawSsid.Assign(mSsid, mSsidLen); 
  return NS_OK;
}

NS_IMETHODIMP nsWifiAccessPoint::GetSignal(int32_t *aSignal)
{
  NS_ENSURE_ARG(aSignal);
  *aSignal = mSignal;
  return NS_OK;
}



bool AccessPointsEqual(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b)
{
  if (a.Count() != b.Count()) {
    LOG(("AccessPoint lists have different lengths\n"));
    return false;
  }

  for (int32_t i = 0; i < a.Count(); i++) {
    LOG(("++ Looking for %s\n", a[i]->mSsid));
    bool found = false;
    for (int32_t j = 0; j < b.Count(); j++) {
      LOG(("   %s->%s | %s->%s\n", a[i]->mSsid, b[j]->mSsid, a[i]->mMac, b[j]->mMac));
      if (!strcmp(a[i]->mSsid, b[j]->mSsid) &&
          !strcmp(a[i]->mMac, b[j]->mMac) &&
          a[i]->mSignal == b[j]->mSignal) {
        found = true;
      }
    }
    if (!found)
      return false;
  }
  LOG(("   match!\n"));
  return true;
}

void ReplaceArray(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b)
{
  a.Clear();

  
  for (int32_t i = 0; i < b.Count(); i++) {
    a.AppendObject(b[i]);
  }

  b.Clear();
}


