





#include "nsSSLSocketProvider.h"
#include "nsNSSIOLayer.h"
#include "nsError.h"

nsSSLSocketProvider::nsSSLSocketProvider()
{
}

nsSSLSocketProvider::~nsSSLSocketProvider()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSSLSocketProvider, nsISocketProvider)

NS_IMETHODIMP
nsSSLSocketProvider::NewSocket(int32_t family,
                               const char *host,
                               int32_t port,
                               const char *proxyHost,
                               int32_t proxyPort,
                               uint32_t flags,
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
                                      false,
                                      flags & ANONYMOUS_CONNECT);
  return (NS_FAILED(rv)) ? NS_ERROR_SOCKET_CREATE_FAILED : NS_OK;
}


NS_IMETHODIMP
nsSSLSocketProvider::AddToSocket(int32_t family,
                                 const char *host,
                                 int32_t port,
                                 const char *proxyHost,
                                 int32_t proxyPort,
                                 uint32_t flags,
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
                                        false,
                                        flags & ANONYMOUS_CONNECT);
  
  return (NS_FAILED(rv)) ? NS_ERROR_SOCKET_CREATE_FAILED : NS_OK;
}
