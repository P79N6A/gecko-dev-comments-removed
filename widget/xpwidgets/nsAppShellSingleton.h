




#ifndef nsAppShellSingleton_h__
#define nsAppShellSingleton_h__






















#include "nsXULAppAPI.h"
#if defined(MOZ_METRO) && defined(XP_WIN)
#include "winrt/MetroAppShell.h"
#endif

static nsIAppShell *sAppShell;

static nsresult
nsAppShellInit()
{
  NS_ASSERTION(!sAppShell, "already initialized");

#if !defined(MOZ_METRO) || !defined(XP_WIN)
  sAppShell = new nsAppShell();
#else
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
    sAppShell = new MetroAppShell();
  } else {
    sAppShell = new nsAppShell();
  }
#endif
  if (!sAppShell)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(sAppShell);

  nsresult rv;
#if !defined(MOZ_METRO) || !defined(XP_WIN)
  rv = static_cast<nsAppShell*>(sAppShell)->Init();
#else
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
    rv = static_cast<MetroAppShell*>(sAppShell)->Init();
  } else {
    rv = static_cast<nsAppShell*>(sAppShell)->Init();
  }
#endif
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
