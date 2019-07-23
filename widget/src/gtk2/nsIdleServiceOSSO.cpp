



































#include "nsIdleServiceOSSO.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "prtime.h"

#include <gtk/gtk.h>

NS_IMPL_ISUPPORTS2(nsIdleServiceOSSO, nsIIdleService, nsIObserver)

nsIdleServiceOSSO::nsIdleServiceOSSO()
 : mIdle(PR_FALSE)
{
  nsCOMPtr<nsIObserverService> obsServ = do_GetService("@mozilla.org/observer-service;1");
  obsServ->AddObserver(this, "system-idle", PR_FALSE);
  obsServ->AddObserver(this, "system-active", PR_FALSE);
}

nsIdleServiceOSSO::~nsIdleServiceOSSO()
{
}

NS_IMETHODIMP
nsIdleServiceOSSO::GetIdleTime(PRUint32 *aTimeDiff)
{ 
  if (mIdle) {
    *aTimeDiff = ( PR_Now() - mIdleSince ) / PR_USEC_PER_MSEC;
    return NS_OK;
  }

  *aTimeDiff = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsIdleServiceOSSO::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  if (!strcmp(aTopic, "system-idle")) {
    mIdle = PR_TRUE;
    mIdleSince = PR_Now();
  }
  else if (!strcmp(aTopic, "system-active")) {
    mIdle = PR_FALSE;
  }

  return NS_OK;
}
