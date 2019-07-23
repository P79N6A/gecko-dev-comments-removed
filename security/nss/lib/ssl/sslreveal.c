







































#include "cert.h"
#include "ssl.h"
#include "certt.h"
#include "sslimpl.h"




CERTCertificate * 
SSL_RevealCert(PRFileDesc * fd)
{
  CERTCertificate * cert = NULL;
  sslSocket * sslsocket = NULL;

  sslsocket = ssl_FindSocket(fd);
  
  


  if (sslsocket && sslsocket->sec.peerCert)
    cert = CERT_DupCertificate(sslsocket->sec.peerCert);
  
  return cert;
}



void * 
SSL_RevealPinArg(PRFileDesc * fd)
{
  sslSocket * sslsocket = NULL;
  void * PinArg = NULL;
  
  sslsocket = ssl_FindSocket(fd);
  
  
  if (sslsocket)
    PinArg = sslsocket->pkcs11PinArg;
  
  return PinArg;
}





char * 
SSL_RevealURL(PRFileDesc * fd)
{
  sslSocket * sslsocket = NULL;
  char * url = NULL;

  sslsocket = ssl_FindSocket(fd);
  
  if (sslsocket && sslsocket->url)
    url = PL_strdup(sslsocket->url);
  
  return url;
}






SECStatus
SSL_HandshakeNegotiatedExtension(PRFileDesc * socket, 
                                 SSLExtensionType extId,
                                 PRBool *pYes)
{
  
  sslSocket * sslsocket = NULL;
  SECStatus rv = SECFailure;

  if (!pYes)
    return rv;

  sslsocket = ssl_FindSocket(socket);

  
  if (sslsocket && sslsocket->opt.useSecurity && sslsocket->firstHsDone) {
    if (sslsocket->ssl3.initialized) { 
      






      ssl_GetSSL3HandshakeLock(sslsocket);
      *pYes = ssl3_ExtensionNegotiated(sslsocket, extId);
      ssl_ReleaseSSL3HandshakeLock(sslsocket);
      rv = SECSuccess;
    }
  }

  return rv;
}
