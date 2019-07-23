





































#include <stdlib.h>
#include "nsNativeAppSupportBase.h"

#ifdef MOZ_ENABLE_LIBCONIC
#include <glib-object.h>
#endif

class nsNativeAppSupportQt : public nsNativeAppSupportBase
{
public:
  NS_IMETHOD Start(PRBool* aRetVal);
  NS_IMETHOD Stop(PRBool* aResult);
};

NS_IMETHODIMP
nsNativeAppSupportQt::Start(PRBool* aRetVal)
{
  NS_ASSERTION(gAppData, "gAppData must not be null.");

  *aRetVal = PR_TRUE;
#ifdef MOZ_ENABLE_LIBCONIC
  g_type_init();
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
