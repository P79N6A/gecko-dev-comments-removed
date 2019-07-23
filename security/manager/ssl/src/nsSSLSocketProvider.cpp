






































#include "nsSSLSocketProvider.h"
#include "nsNSSIOLayer.h"
#include "nsNetError.h"

nsSSLSocketProvider::nsSSLSocketProvider()
{
}

nsSSLSocketProvider::~nsSSLSocketProvider()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSSLSocketProvider, nsISocketProvider)

NS_IMETHODIMP
nsSSLSocketProvider::NewSocket(PRInt32 family,
                               const char *host,
                               PRInt32 port,
                               const char *proxyHost,
                               PRInt32 proxyPort,
                               PRUint32 flags,
                               PRFileDesc **_result,
                               nsISupports **securityInfo)
{
  nsresult rv = nsSSLIOLayerNewSocket(family,
                                      host,
                                      port,
                                      proxyHost,
                                      proxyPort,
                                      _result,
                                      securityInfo,
                                      PR_FALSE);
  return (NS_FAILED(rv)) ? NS_ERROR_SOCKET_CREATE_FAILED : NS_OK;
}


NS_IMETHODIMP
nsSSLSocketProvider::AddToSocket(PRInt32 family,
                                 const char *host,
                                 PRInt32 port,
                                 const char *proxyHost,
                                 PRInt32 proxyPort,
                                 PRUint32 flags,
                                 PRFileDesc *aSocket,
                                 nsISupports **securityInfo)
{
  nsresult rv = nsSSLIOLayerAddToSocket(family,
                                        host,
                                        port,
                                        proxyHost,
                                        proxyPort,
                                        aSocket,
                                        securityInfo,
                                        PR_FALSE);
  
  return (NS_FAILED(rv)) ? NS_ERROR_SOCKET_CREATE_FAILED : NS_OK;
}
