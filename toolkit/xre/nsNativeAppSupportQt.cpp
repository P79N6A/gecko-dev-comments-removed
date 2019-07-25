





































#include <stdlib.h>
#include "nsNativeAppSupportBase.h"
#include "nsString.h"

#ifdef MOZ_ENABLE_LIBCONIC
#include <glib-object.h>
#endif

#if (MOZ_PLATFORM_MAEMO == 5)
#include <libosso.h>
#endif

class nsNativeAppSupportQt : public nsNativeAppSupportBase
{
public:
  NS_IMETHOD Start(PRBool* aRetVal);
  NS_IMETHOD Stop(PRBool* aResult);
#if (MOZ_PLATFORM_MAEMO == 5)
  
  osso_context_t *m_osso_context;
#endif
};

NS_IMETHODIMP
nsNativeAppSupportQt::Start(PRBool* aRetVal)
{
  NS_ASSERTION(gAppData, "gAppData must not be null.");

  *aRetVal = PR_TRUE;
#ifdef MOZ_ENABLE_LIBCONIC
  g_type_init();
#endif

#ifdef MOZ_PLATFORM_MAEMO
  








  nsCAutoString applicationName;
  if (gAppData->vendor) {
      applicationName.Append(gAppData->vendor);
      applicationName.Append(".");
  }
  applicationName.Append(gAppData->name);
  ToLowerCase(applicationName);

  m_osso_context = osso_initialize(applicationName.get(),
                                   gAppData->version ? gAppData->version : "1.0",
                                   PR_TRUE,
                                   nsnull);

  
  if (m_osso_context == nsnull) {
      return NS_ERROR_FAILURE;
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportQt::Stop(PRBool* aResult)
{
  NS_ENSURE_ARG(aResult);
  *aResult = PR_TRUE;

  return NS_OK;
}

nsresult
NS_CreateNativeAppSupport(nsINativeAppSupport** aResult)
{
  nsNativeAppSupportBase* native = new nsNativeAppSupportQt();
  if (!native)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = native;
  NS_ADDREF(*aResult);

  return NS_OK;
}
