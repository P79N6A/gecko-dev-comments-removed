




#ifndef nsAppShellSingleton_h__
#define nsAppShellSingleton_h__






















#include "nsXULAppAPI.h"

static nsIAppShell *sAppShell;

static nsresult
nsAppShellInit()
{
  NS_ASSERTION(!sAppShell, "already initialized");

  sAppShell = new nsAppShell();
  if (!sAppShell)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(sAppShell);

  nsresult rv;
  rv = static_cast<nsAppShell*>(sAppShell)->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(sAppShell);
    return rv;
  }

  return NS_OK;
}

static void
nsAppShellShutdown()
{
  NS_RELEASE(sAppShell);
}

static nsresult
nsAppShellConstructor(nsISupports *outer, const nsIID &iid, void **result)
{
  NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);
  NS_ENSURE_TRUE(sAppShell, NS_ERROR_NOT_INITIALIZED);

  return sAppShell->QueryInterface(iid, result);
}

#endif  
