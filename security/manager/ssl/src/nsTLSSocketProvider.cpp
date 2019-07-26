





#include "nsTLSSocketProvider.h"
#include "nsNSSIOLayer.h"
#include "nsError.h"

nsTLSSocketProvider::nsTLSSocketProvider()
{
}

nsTLSSocketProvider::~nsTLSSocketProvider()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsTLSSocketProvider, nsISocketProvider)

NS_IMETHODIMP
nsTLSSocketProvider::NewSocket(int32_t family,
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
                                      true,
                                      flags & ANONYMOUS_CONNECT);
  
  return (NS_FAILED(rv)) ? NS_ERROR_SOCKET_CREATE_FAILED : NS_OK;
}


NS_IMETHODIMP
nsTLSSocketProvider::AddToSocket(int32_t family,
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
                                        true,
                                        flags & ANONYMOUS_CONNECT);
  
  return (NS_FAILED(rv)) ? NS_ERROR_SOCKET_CREATE_FAILED : NS_OK;
}
