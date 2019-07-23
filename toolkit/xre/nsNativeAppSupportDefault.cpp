



































#include "nsNativeAppSupportBase.h"

nsresult
NS_CreateNativeAppSupport( nsINativeAppSupport **aResult )
{
  nsNativeAppSupportBase* native = new nsNativeAppSupportBase();
  if (!native) return NS_ERROR_OUT_OF_MEMORY;

  *aResult = native;
  NS_ADDREF( *aResult );

  return NS_OK;
}
