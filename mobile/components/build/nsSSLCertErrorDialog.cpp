






































#include "nsSSLCertErrorDialog.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSSLCertErrorDialog, nsISSLCertErrorDialog)

NS_IMETHODIMP
nsSSLCertErrorDialog::ShowCertError(nsIInterfaceRequestor *ctx, 
                                    nsISSLStatus *status, 
                                    nsIX509Cert *cert, 
                                    const nsAString & textErrorMessage, 
                                    const nsAString & htmlErrorMessage, 
                                    const nsACString & hostName, 
                                    PRUint32 portNumber)
{
  



  return NS_OK;
}
