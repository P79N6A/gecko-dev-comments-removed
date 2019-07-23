






































#include "nsWifiAccessPoint.h"
#include "nsString.h"
#include "nsMemory.h"
#include "prlog.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo *gWifiMonitorLog;
#endif
#define LOG(args)     PR_LOG(gWifiMonitorLog, PR_LOG_DEBUG, args)


NS_IMPL_THREADSAFE_ISUPPORTS1(nsWifiAccessPoint, nsIWifiAccessPoint)

nsWifiAccessPoint::nsWifiAccessPoint()
{
  
  mMac[0] = nsnull;
  mSsid[0] = nsnull;
  mSsidLen = 0;
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

NS_IMETHODIMP nsWifiAccessPoint::GetSignal(PRInt32 *aSignal)
{
  NS_ENSURE_ARG(aSignal);
  *aSignal = mSignal;
  return NS_OK;
}



PRBool AccessPointsEqual(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b)
{
  if (a.Count() != b.Count()) {
    LOG(("AccessPoint lists have different lengths\n"));
    return PR_FALSE;
  }

  for (PRInt32 i = 0; i < a.Count(); i++) {
    LOG(("++ Looking for %s\n", a[i]->mSsid));
    PRBool found = PR_FALSE;
    for (PRInt32 j = 0; j < b.Count(); j++) {
      LOG(("   %s->%s | %s->%s\n", a[i]->mSsid, b[j]->mSsid, a[i]->mMac, b[j]->mMac));
      if (!strcmp(a[i]->mSsid, b[j]->mSsid) &&
          !strcmp(a[i]->mMac, b[j]->mMac)) {
        found = PR_TRUE;
      }
    }
    if (!found)
      return PR_FALSE;
  }
  LOG(("   match!\n"));
  return PR_TRUE;
}

void ReplaceArray(nsCOMArray<nsWifiAccessPoint>& a, nsCOMArray<nsWifiAccessPoint>& b)
{
  a.Clear();

  
  for (PRInt32 i = 0; i < b.Count(); i++) {
    a.AppendObject(b[i]);
  }

  b.Clear();
}


