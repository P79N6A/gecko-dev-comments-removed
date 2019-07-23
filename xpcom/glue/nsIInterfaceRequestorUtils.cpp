





































#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

nsresult
nsGetInterface::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
  nsresult status;

  if ( mSource )
  {
    nsCOMPtr<nsIInterfaceRequestor> factoryPtr = do_QueryInterface(mSource, &status);

    if ( factoryPtr )
      status = factoryPtr->GetInterface(aIID, aInstancePtr);
    else
      status = NS_ERROR_NO_INTERFACE;
  }
  else
    status = NS_ERROR_NULL_POINTER;

  if ( NS_FAILED(status) )
    *aInstancePtr = 0;
  if ( mErrorPtr )
    *mErrorPtr = status;
  return status;
}
